/*******************************************************************************
 * Copyright 2013-2015 Avago Technologies
 * Copyright (c) 2009 to 2012 PLX Technology Inc.  All rights reserved.
 *
 * This software is available to you under a choice of one of two
 * licenses.  You may choose to be licensed under the terms of the GNU
 * General Public License (GPL) Version 2, available from the file
 * COPYING in the main directorY of this source tree, or the
 * BSD license below:
 *
 *     Redistribution and use in source and binary forms, with or
 *     without modification, are permitted provided that the following
 *     conditions are met:
 *
 *      - Redistributions of source code must retain the above
 *        copyright notice, this list of conditions and the following
 *        disclaimer.
 *
 *      - Redistributions in binary form must reproduce the above
 *        copyright notice, this list of conditions and the following
 *        disclaimer in the documentation and/or other materials
 *        provided with the distribution.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 * NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS
 * BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN
 * ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
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
            PCI9050_INT_CTRL_STAT
            );

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
        PCI9050_INT_CTRL_STAT,
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
 * Note       :  The 9052 supports Edge-triggerable interrupts as well as level
 *               triggered interrupts.  The 9050 only supports level triggered
 *               interrupts.  The interrupt masking code below handles both cases.
 *               If the chip is a 9050, the same code is used but should work
 *               ok since edge triggerable interrupts will always be disabled.
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
            PCI9050_INT_CTRL_STAT
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
                PCI9050_INT_CTRL_STAT
                );

        RegValue |= RegData.BitsToSet;
        RegValue &= ~(RegData.BitsToClear);

        PLX_9000_REG_WRITE(
            pdx,
            PCI9050_INT_CTRL_STAT,
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
