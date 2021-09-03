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
 *      01-01-14 : PLX SDK v7.20
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
    U8                channel;
    U16               OffsetStatus;
    U32               RegStatus;
    U32               IntSource;
    BOOLEAN           bIntActive;
    DEVICE_EXTENSION *pdx;


    // Get the device extension
    pdx = (DEVICE_EXTENSION *)dev_id;

    // Disable interrupts and acquire lock 
    spin_lock( &(pdx->Lock_Isr) ); 

    // Default to no interrupt active
    bIntActive = FALSE;

    // Check each channel for active interrupt
    for (channel = 0; channel < pdx->NumDmaChannels; channel++)
    {
        // Determine DMA status register offset
        OffsetStatus = 0x23C + (channel * 0x100);

        // Read interrupt status register for channel
        RegStatus = PLX_DMA_REG_READ( pdx, OffsetStatus );

        // Clear the interrupt type flag
        IntSource = INTR_TYPE_NONE;

        // Check if error interrupt is active ([16]) and enabled([0])
        if ((RegStatus & (1 << 16)) && (RegStatus & (1 << 0)))
            IntSource |= INTR_TYPE_DMA_ERROR;
        else
            RegStatus &= ~(1 << 16);

        // Check if invalid descriptor interrupt is active ([17]) and enabled([1])
        if ((RegStatus & (1 << 17)) && (RegStatus & (1 << 1)))
            IntSource |= INTR_TYPE_DESCR_INVALID;
        else
            RegStatus &= ~(1 << 17);

        // Check if abort done interrupt is active ([19]) and enabled([3])
        if ((RegStatus & (1 << 19)) && (RegStatus & (1 << 3)))
            IntSource |= INTR_TYPE_ABORT_DONE;
        else
            RegStatus &= ~(1 << 19);

        // Check if pause done interrupt is active ([20]) and enabled([4])
        if ((RegStatus & (1 << 20)) && (RegStatus & (1 << 4)))
            IntSource |= INTR_TYPE_PAUSE_DONE;
        else
            RegStatus &= ~(1 << 20);

        // Check if immediate stop interrupt is active ([21]) and enabled([5])
        if ((RegStatus & (1 << 21)) && (RegStatus & (1 << 5)))
            IntSource |= INTR_TYPE_IMMED_STOP_DONE;
        else
            RegStatus &= ~(1 << 21);

        // Check if descriptor/DMA done interrupt is active ([18])
        if (RegStatus & (1 << 18))
            IntSource |= INTR_TYPE_DESCR_DMA_DONE;

        // Clear interrupt
        if (IntSource != INTR_TYPE_NONE)
        {
            // Write register back to itself to clear active interrupts
            PLX_DMA_REG_WRITE( pdx, OffsetStatus, RegStatus );

            // Flag an interrupt was detected
            bIntActive = TRUE;

            // Store pending interrupts
            pdx->Source_Ints |= (IntSource << (channel * 8));
        }
    }

    // Re-enable interrupts and release lock 
    spin_unlock( &(pdx->Lock_Isr) ); 

    // Return if no interrupt active
    if (bIntActive == FALSE)
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
    U8                  channel;
    U32                 IntStatus;
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
    if ((pdx->State != PLX_STATE_STARTED) || (pdx->pRegVa == NULL))
    {
        DebugPrintf(("DPC aborted, device is stopping\n"));
        pdx->bDpcPending = FALSE;
        return;
    }

    // Setup for synchonized access to interrupt source
    IntData.pdx = pdx;

    // Get current pending interrupt sources
    PlxSynchronizedGetInterruptSource(
        &IntData
        );

    // Cleanup after SGL DMA
    for (channel = 0; channel < pdx->NumDmaChannels; channel++)
    {
        // Get active interrupts for channel
        IntStatus = (IntData.Source_Ints >> (channel * 8)) & 0xFF;

        // Check if DMA completed for a driver SGL transfer & cleanup
        if ((IntStatus & INTR_TYPE_DESCR_DMA_DONE) &&
            (pdx->DmaInfo[channel].bSglPending))
        {
            PlxSglDmaTransferComplete(
                pdx,
                channel
                );
        }
    }

    // Signal any objects waiting for notification
    PlxSignalNotifications(
        pdx,
        &IntData
        );

    // Flag a DPC is no longer pending
    pdx->bDpcPending = FALSE;
}
