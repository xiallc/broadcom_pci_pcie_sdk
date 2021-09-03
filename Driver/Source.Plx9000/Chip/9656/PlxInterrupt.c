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


#include "PciFunc.h"
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
    U32               RegValue;
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
            PCI9656_INT_CTRL_STAT
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
    if ((RegPciInt & (1 << 8)) == 0)
    {
        spin_unlock( &(pdx->Lock_Isr) );
        return IRQ_RETVAL(IRQ_NONE);
    }

    // Verify that an interrupt is truly active

    // Clear the interrupt type flag
    InterruptSource = INTR_TYPE_NONE;

    // Check if PCI Doorbell Interrupt is active and not masked
    if ((RegPciInt & (1 << 13)) && (RegPciInt & (1 << 9)))
    {
        InterruptSource |= INTR_TYPE_DOORBELL;
    }

    // Check if PCI Abort Interrupt is active and not masked
    if ((RegPciInt & (1 << 14)) && (RegPciInt & (1 << 10)))
    {
        InterruptSource |= INTR_TYPE_PCI_ABORT;
    }

    // Check if Local->PCI Interrupt is active and not masked
    if ((RegPciInt & (1 << 15)) && (RegPciInt & (1 << 11)))
    {
        InterruptSource |= INTR_TYPE_LOCAL_1;
    }

    // Check if DMA Channel 0 Interrupt is active and not masked
    if ((RegPciInt & (1 << 21)) && (RegPciInt & (1 << 18)))
    {
        // Verify the DMA interrupt is routed to PCI
        RegValue =
            PLX_9000_REG_READ(
                pdx,
                PCI9656_DMA0_MODE
                );

        if (RegValue & (1 << 17))
        {
            InterruptSource |= INTR_TYPE_DMA_0;
        }
    }

    // Check if DMA Channel 1 Interrupt is active and not masked
    if ((RegPciInt & (1 << 22)) && (RegPciInt & (1 << 19)))
    {
        // Verify the DMA interrupt is routed to PCI
        RegValue =
            PLX_9000_REG_READ(
                pdx,
                PCI9656_DMA1_MODE
                );

        if (RegValue & (1 << 17))
        {
            InterruptSource |= INTR_TYPE_DMA_1;
        }
    }

    // Check if MU Outbound Post interrupt is active
    RegValue =
        PLX_9000_REG_READ(
            pdx,
            PCI9656_OUTPOST_INT_STAT
            );

    if (RegValue & (1 << 3))
    {
        // Check if MU Outbound Post interrupt is not masked
        RegValue =
            PLX_9000_REG_READ(
                pdx,
                PCI9656_OUTPOST_INT_MASK
                );

        if ((RegValue & (1 << 3)) == 0)
        {
            InterruptSource |= INTR_TYPE_OUTBOUND_POST;
        }
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
        PCI9656_INT_CTRL_STAT,
        RegPciInt & ~(1 << 8)
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

    // Local Interrupt 1
    if (IntData.Source_Ints & INTR_TYPE_LOCAL_1)
    {
        // Synchronize access to Interrupt Control/Status Register
        spin_lock_irqsave( &(pdx->Lock_Isr), flags );

        // Mask local interrupt 1 since true source is unknown
        RegValue =
            PLX_9000_REG_READ(
                pdx,
                PCI9656_INT_CTRL_STAT
                );

        RegValue &= ~(1 << 11);

        PLX_9000_REG_WRITE(
            pdx,
            PCI9656_INT_CTRL_STAT,
            RegValue
            );

        spin_unlock_irqrestore( &(pdx->Lock_Isr), flags );
    }

    // Doorbell Interrupt
    if (IntData.Source_Ints & INTR_TYPE_DOORBELL)
    {
        // Get Doorbell register
        RegValue =
            PLX_9000_REG_READ(
                pdx,
                PCI9656_PCI_DOORBELL
                );

        // Clear Doorbell interrupt
        PLX_9000_REG_WRITE(
            pdx,
            PCI9656_PCI_DOORBELL,
            RegValue
            );

        // Save the doorbell value
        IntData.Source_Doorbell = RegValue;
    }

    // PCI Abort interrupt
    if (IntData.Source_Ints & INTR_TYPE_PCI_ABORT)
    {
        // Get the PCI Command register
        PLX_PCI_REG_READ(
            pdx,
            0x04,
            &RegValue
            );

        // Write to back to clear PCI Abort
        PLX_PCI_REG_WRITE(
            pdx,
            0x04,
            RegValue
            );
    }

    // DMA Channel 0 interrupt
    if (IntData.Source_Ints & INTR_TYPE_DMA_0)
    {
        // Get DMA Control/Status
        RegValue =
            PLX_9000_REG_READ(
                pdx,
                PCI9656_DMA_COMMAND_STAT
                );

        // Clear DMA interrupt
        PLX_9000_REG_WRITE(
            pdx,
            PCI9656_DMA_COMMAND_STAT,
            RegValue | (1 << 3)
            );

        RegValue =
            PLX_9000_REG_READ(
                pdx,
                PCI9656_DMA0_MODE
                );

        // Check if SGL is enabled & cleanup
        if (RegValue & (1 << 9))
        {
            PlxSglDmaTransferComplete(
                pdx,
                0
                );
        }
    }

    // DMA Channel 1 interrupt
    if (IntData.Source_Ints & INTR_TYPE_DMA_1)
    {
        // Get DMA Control/Status
        RegValue =
            PLX_9000_REG_READ(
                pdx,
                PCI9656_DMA_COMMAND_STAT
                );

        // Clear DMA interrupt
        PLX_9000_REG_WRITE(
            pdx,
            PCI9656_DMA_COMMAND_STAT,
            RegValue | (1 << 11)
            );

        RegValue =
            PLX_9000_REG_READ(
                pdx,
                PCI9656_DMA1_MODE
                );

        // Check if SGL is enabled & cleanup
        if (RegValue & (1 << 9))
        {
            PlxSglDmaTransferComplete(
                pdx,
                1
                );
        }
    }

    // Outbound post FIFO interrupt
    if (IntData.Source_Ints & INTR_TYPE_OUTBOUND_POST)
    {
        // Mask Outbound Post interrupt
        PLX_9000_REG_WRITE(
            pdx,
            PCI9656_OUTPOST_INT_MASK,
            (1 << 3)
            );
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
