/*******************************************************************************
 * Copyright (c) PLX Technology, Inc.
 *
 * PLX Technology Inc. licenses this source file under the GNU Lesser General Public
 * License (LGPL) version 2.  This source file may be modified or redistributed
 * under the terms of the LGPL and without express permission from PLX Technology.
 *
 * PLX Technology, Inc. provides this software AS IS, WITHOUT ANY WARRANTY,
 * EXPRESS OR IMPLIED, INCLUDING, WITHOUT LIMITATION, ANY WARRANTY OF
 * MERCHANTABILITY OR FITNESS FOR A PARTICULAR PURPOSE.  PLX makes no guarantee
 * or representations regarding the use of, or the results of the use of,
 * the software and documentation in terms of correctness, accuracy,
 * reliability, currentness, or otherwise; and you rely on the software,
 * documentation and results solely at your own risk.
 *
 * IN NO EVENT SHALL PLX BE LIABLE FOR ANY LOSS OF USE, LOSS OF BUSINESS,
 * LOSS OF PROFITS, INDIRECT, INCIDENTAL, SPECIAL OR CONSEQUENTIAL DAMAGES
 * OF ANY KIND.
 *
 ******************************************************************************/

/******************************************************************************
 *
 * File Name:
 *
 *      PlxInterrupt.c
 *
 * Description:
 *
 *      This file handles interrupts for the PLX device
 *
 * Revision History:
 *
 *      09-01-10 : PLX SDK v6.40
 *
 ******************************************************************************/


#include "DrvDefs.h"
#include "PciFunc.h"
#include "PlxInterrupt.h"
#include "SuppFunc.h"




/*******************************************************************************
 *
 * Function   :  OnInterrupt
 *
 * Description:  The Interrupt Service Routine for the PLX device
 *
 ******************************************************************************/
irqreturn_t
OnInterrupt(
    int   irq,
    void *dev_id
  #if (LINUX_VERSION_CODE < KERNEL_VERSION(2,6,19))
  , struct pt_regs *regs
  #endif
    )
{
    U32               TmpValue;
    U32               RegEnable_DB;
    U32               RegEnable_Ints;
    U32               RegStatus;
    BOOLEAN           bInterrupt;
    DEVICE_EXTENSION *pdx;


    // Get the device extension
    pdx = (DEVICE_EXTENSION *)dev_id;

    // Disable interrupts and acquire lock 
    spin_lock( &(pdx->Lock_Isr)); 

    // Assume no interrupt
    bInterrupt = FALSE;

    // Get interrupt enable
    PLX_PCI_REG_READ(
        pdx,
        0xc4,
        &RegEnable_DB
        );

    PLX_PCI_REG_READ(
        pdx,
        0xc8,
        &RegEnable_Ints
        );

    // Get interrupt status
    PLX_PCI_REG_READ(
        pdx,
        0xcc,
        &RegStatus
        );

    // Check for doorbell interrupt
    TmpValue = (RegEnable_DB & 0xFFFF) & (RegStatus & 0xFFFF);

    if (TmpValue)
    {
        // Ignore active doorbell interrupt if still pending
        if ((RegEnable_DB >> 16) & TmpValue)
        {
            // Doorbell interrupt still pending, ignore it
        }
        else
        {
            // Clear active doorbell interrupts
            PLX_PCI_REG_WRITE(
                pdx,
                0xcc,
                (RegStatus & 0xFF000000) | TmpValue
                );

            // Flag doorbell interrupt active
            pdx->Source_Doorbell |= TmpValue;

            // Flag interrupt found
            bInterrupt = TRUE;
        }
    }

    // Check for messages, RSTIN, PME, & GPIO interrupts
    TmpValue = ((RegEnable_Ints >> 24) & (RegStatus >> 16)) & 0xFF;

    if (TmpValue)
    {
        if (TmpValue & (1 << 0))
        {
            // Flag message interrupt active
            pdx->Source_Ints |= INTR_TYPE_MESSAGE_0;
        }

        if (TmpValue & (1 << 1))
        {
            // Flag message interrupt active
            pdx->Source_Ints |= INTR_TYPE_MESSAGE_1;
        }

        if (TmpValue & (1 << 2))
        {
            // Flag message interrupt active
            pdx->Source_Ints |= INTR_TYPE_MESSAGE_2;
        }

        if (TmpValue & (1 << 3))
        {
            // Flag message interrupt active
            pdx->Source_Ints |= INTR_TYPE_MESSAGE_3;
        }

        if (TmpValue & (1 << 4))
        {
            // Flag RSTIN de-assertion detected
            pdx->Source_Ints |= INTR_TYPE_RSTIN;
        }

        if (TmpValue & (1 << 5))
        {
            // Flag PME de-assertion detected
            pdx->Source_Ints |= INTR_TYPE_PME;
        }

        // GPIO 14/15 interrupt
        if (TmpValue & (1 << 6))
        {
            // Interrupt active, must disable/mask since can't clear source
            RegEnable_Ints &= ~((1 << 6) << 24);

            PLX_PCI_REG_WRITE(
                pdx,
                0xc8,
                (RegEnable_Ints & 0xFF000000)
                );

            // Flag GPIO 14/15 interrupt active
            pdx->Source_Ints |= INTR_TYPE_GPIO_14_15;
        }

        // GPIO 4/5 interrupt
        if (TmpValue & (1 << 7))
        {
            // Interrupt active, must disable/mask since can't clear source
            RegEnable_Ints &= ~((1 << 7) << 24);

            PLX_PCI_REG_WRITE(
                pdx,
                0xc8,
                (RegEnable_Ints & 0xFF000000)
                );

            // Flag GPIO 4/5 interrupt active
            pdx->Source_Ints |= INTR_TYPE_GPIO_4_5;
        }

        // Clear active interrupts
        PLX_PCI_REG_WRITE(
            pdx,
            0xcc,
            (RegStatus & 0x00FF0000) | (TmpValue << 16)
            );

        // Flag interrupt found
        bInterrupt = TRUE;
    }

    // Re-enable interrupts and release lock 
    spin_unlock( &(pdx->Lock_Isr) ); 

    // Return if no interrupt active
    if (bInterrupt == FALSE)
        return IRQ_RETVAL(IRQ_NONE);

    //
    // Schedule deferred procedure (DPC) to complete interrupt processing
    //

    // If device is no longer started, do not schedule a DPC
    if (pdx->State != PLX_STATE_STARTED)
        return IRQ_RETVAL(IRQ_HANDLED);

    // Add task to system work queue
    schedule_work(
        &(pdx->Task_DpcForIsr)
        );

    // Flag a DPC is pending
    pdx->bDpcPending = TRUE;

    return IRQ_RETVAL(IRQ_HANDLED);
}




/*******************************************************************************
 *
 * Function   :  DpcForIsr
 *
 * Description:  This routine will be triggered by the ISR to service an interrupt
 *
 ******************************************************************************/
VOID
DpcForIsr(
    PLX_DPC_PARAM *pArg1
    )
{
    DEVICE_EXTENSION   *pdx;
    PLX_INTERRUPT_DATA  IntData;


    // Get the device extension
    pdx =
        container_of(
            pArg1,
            DEVICE_EXTENSION,
            Task_DpcForIsr
            );

    // Abort DPC if device is being stopped and resources released
    if (pdx->State != PLX_STATE_STARTED)
    {
        DebugPrintf(("DPC aborted, device is stopping\n"));

        // Flag DPC is no longer pending
        pdx->bDpcPending = FALSE;

        return;
    }

    // Setup for synchonized access to interrupt source
    IntData.pdx = pdx;

    // Get current pending interrupt sources
    PlxSynchronizedGetInterruptSource(
        &IntData
        );

    // Signal any objects waiting for notification
    PlxSignalNotifications(
        pdx,
        &IntData
        );

    // Flag a DPC is no longer pending
    pdx->bDpcPending = FALSE;
}
