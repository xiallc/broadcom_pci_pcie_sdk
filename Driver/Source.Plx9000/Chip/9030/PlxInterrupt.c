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
 *      05-01-13 : PLX SDK v7.10
 *
 ******************************************************************************/


#include "PlxChipFn.h"
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
    U32               RegPciInt;
    U32               InterruptSource;
    DEVICE_EXTENSION *pdx;


    // Get the device extension
    pdx = (DEVICE_EXTENSION *)dev_id;

    // Disable interrupts and acquire lock 
    spin_lock( &(pdx->Lock_Isr) ); 

    // Read interrupt status register
    RegPciInt =
        PLX_9000_REG_READ(
            pdx,
            PCI9030_INT_CTRL_STAT
            );

    /****************************************************
     * If the chip is in a low power state, then local
     * register reads are disabled and will always return
     * 0xFFFFFFFF.  If the PLX chip's interrupt is shared
     * with another PCI device, the PXL ISR may continue
     * to be called.  This case is handled to avoid
     * erroneous reporting of an active interrupt.
     ***************************************************/
    if (RegPciInt == 0xFFFFFFFF)
    {
        spin_unlock( &(pdx->Lock_Isr) );
        return IRQ_RETVAL(IRQ_NONE);
    }

    // Check for master PCI interrupt enable
    if ((RegPciInt & (1 << 6)) == 0)
    {
        spin_unlock( &(pdx->Lock_Isr) );
        return IRQ_RETVAL(IRQ_NONE);
    }

    // Verify that an interrupt is truly active

    // Clear the interrupt type flag
    InterruptSource = INTR_TYPE_NONE;

    // Check if Local Interrupt 1 is active and not masked
    if ((RegPciInt & (1 << 2)) && (RegPciInt & (1 << 0)))
    {
        InterruptSource |= INTR_TYPE_LOCAL_1;
    }

    // Check if Local Interrupt 2 is active and not masked
    if ((RegPciInt & (1 << 5)) && (RegPciInt & (1 << 3)))
    {
        InterruptSource |= INTR_TYPE_LOCAL_2;
    }

    // Software Interrupt
    if (RegPciInt & (1 << 7))
    {
        InterruptSource |= INTR_TYPE_SOFTWARE;
    }

    // Return if no interrupts are active
    if (InterruptSource == INTR_TYPE_NONE)
    {
        spin_unlock( &(pdx->Lock_Isr) );
        return IRQ_RETVAL(IRQ_NONE);
    }

    // At this point, the device interrupt is verified

    // Mask the PCI Interrupt
    PLX_9000_REG_WRITE(
        pdx,
        PCI9030_INT_CTRL_STAT,
        RegPciInt & ~(1 << 6)
        );

    // Re-enable interrupts and release lock 
    spin_unlock( &(pdx->Lock_Isr) ); 

    //
    // Schedule deferred procedure (DPC) to complete interrupt processing
    //

    // Provide interrupt source to DPC
    pdx->Source_Ints = InterruptSource;

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
    U32                 RegValue;
    PLX_REG_DATA        RegData;
    unsigned long       flags;
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
    if ((pdx->State != PLX_STATE_STARTED) || (pdx->PciBar[0].pVa == NULL))
    {
        DebugPrintf(("DPC aborted, device is stopping\n"));

        // Flag DPC is no longer pending
        pdx->bDpcPending = FALSE;

        return;
    }

    // Get interrupt source
    IntData.Source_Ints     = pdx->Source_Ints;
    IntData.Source_Doorbell = 0;

    // Synchronize access to Interrupt Control/Status Register
    RegData.BitsToSet   = 0;
    RegData.BitsToClear = 0;

    // Get current interrupt status
    RegValue =
        PLX_9000_REG_READ(
            pdx,
            PCI9030_INT_CTRL_STAT
            );

    // Local Interrupt 1
    if (IntData.Source_Ints & INTR_TYPE_LOCAL_1)
    {
        // Check if this is an edge-triggered interrupt
        if ((RegValue & (1 << 1)) && (RegValue & (1 << 8)))
        {
            // Clear edge-triggered interrupt
            RegData.BitsToSet |= (1 << 10);
        }
        else
        {
            // Mask Local Interrupt 1
            RegData.BitsToClear |= (1 << 0);
        }
    }

    // Local Interrupt 2
    if (IntData.Source_Ints & INTR_TYPE_LOCAL_2)
    {
        // Check if this is an edge-triggered interrupt
        if ((RegValue & (1 << 4)) && (RegValue & (1 << 9)))
        {
            // Clear edge-triggered interrupt
            RegData.BitsToSet |= (1 << 11);
        }
        else
        {
            // Mask Local Interrupt 2
            RegData.BitsToClear |= (1 << 3);
        }
    }

    // Software Interrupt
    if (IntData.Source_Ints & INTR_TYPE_SOFTWARE)
    {
        // Clear the software interrupt
        RegData.BitsToClear |= (1 << 7);
    }

    // Clear any active interrupts
    if (RegData.BitsToSet || RegData.BitsToClear)
    {
        spin_lock_irqsave( &(pdx->Lock_Isr), flags );

        RegValue =
            PLX_9000_REG_READ(
                pdx,
                PCI9030_INT_CTRL_STAT
                );

        RegValue |= RegData.BitsToSet;
        RegValue &= ~(RegData.BitsToClear);

        PLX_9000_REG_WRITE(
            pdx,
            PCI9030_INT_CTRL_STAT,
            RegValue
            );

        spin_unlock_irqrestore( &(pdx->Lock_Isr), flags );
    }

    // Signal any objects waiting for notification
    PlxSignalNotifications(
        pdx,
        &IntData
        );

    // Re-enable interrupts
    PlxChipInterruptsEnable(
        pdx
        );

    // Flag a DPC is no longer pending
    pdx->bDpcPending = FALSE;
}
