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
 *      07-01-14 : PLX SDK v7.20
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
    U32               RegMask_LE;
    U32               RegMask_DB;
    U32               RegStatus_LE;
    U32               RegStatus_DB;
    DEVICE_EXTENSION *pdx;


    // Get the device extension
    pdx = (DEVICE_EXTENSION *)dev_id;

    // Disable interrupts and acquire lock 
    spin_lock( &(pdx->Lock_Isr) );

    // Get doorbell interrupt mask
    RegMask_DB =
        PLX_8000_REG_READ(
            pdx,
            pdx->Offset_DB_IntMaskSet
            );

    // Invert mask to make it an 'enable'
    RegMask_DB = ~RegMask_DB;

    // Get doorbell interrupt status
    RegStatus_DB =
        PLX_8000_REG_READ(
            pdx,
            pdx->Offset_DB_IntStatus
            );

    // Clear unused bits
    RegMask_DB   &= 0xFFFF;
    RegStatus_DB &= 0xFFFF;

    // Determine active doorbell interrupts
    RegStatus_DB = RegMask_DB & RegStatus_DB;

    // For NT virtual side, check Link Error status only if no DB interrupts
    if ((RegStatus_DB != 0) ||
        ((pdx->Key.PlxChip & 0xFF00) == 0x8500) ||
        (pdx->Key.PlxPortType == PLX_SPEC_PORT_NT_LINK))
    {
        RegMask_LE   = 0;
        RegStatus_LE = 0;
    }
    else
    {
        // Get Link error interrupt mask
        RegMask_LE =
            PLX_8000_REG_READ(
                pdx,
                pdx->Offset_LE_IntMask
                );

        // Invert mask to make it an 'enable'
        RegMask_LE = ~RegMask_LE;

        // Get Link error interrupt status
        RegStatus_LE =
            PLX_8000_REG_READ(
                pdx,
                pdx->Offset_LE_IntStatus
                );

        // Clear unused bits
        RegMask_LE   &= 0xF;
        RegStatus_LE &= 0xF;

        // Determine active link error interrupts
        RegStatus_LE = RegMask_LE & RegStatus_LE;
    }

    // Return if no interrupt active
    if ((RegStatus_DB == 0) && (RegStatus_LE == 0))
    {
        spin_unlock( &(pdx->Lock_Isr) );
        return IRQ_RETVAL(IRQ_NONE);
    }

    // At this point, the device interrupt is verified

    if (RegStatus_DB != 0)
    {
        // Store the doorbell value
        pdx->Source_Doorbell |= RegStatus_DB;

        // Clear active doorbell interrupts
        PLX_8000_REG_WRITE(
            pdx,
            pdx->Offset_DB_IntClear,
            RegStatus_DB
            );
    }

    if (RegStatus_LE != 0)
    {
        // Store active interrupts
        if (RegStatus_LE & (1 << 0))
            pdx->Source_Ints |= INTR_TYPE_LE_CORRECTABLE;

        if (RegStatus_LE & (1 << 1))
            pdx->Source_Ints |= INTR_TYPE_LE_UNCORRECTABLE;

        if (RegStatus_LE & (1 << 2))
            pdx->Source_Ints |= INTR_TYPE_LE_LINK_STATE_CHANGE;

        if (RegStatus_LE & (1 << 3))
            pdx->Source_Ints |= INTR_TYPE_LE_UNCORR_ERR_MSG;

        // Clear active link error interrupts
        PLX_8000_REG_WRITE(
            pdx,
            pdx->Offset_LE_IntStatus,
            RegStatus_LE
            );
    }

    // Re-enable interrupts and release lock 
    spin_unlock( &(pdx->Lock_Isr) );

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

    // Signal any objects waiting for notification
    PlxSignalNotifications(
        pdx,
        &IntData
        );

    // Flag a DPC is no longer pending
    pdx->bDpcPending = FALSE;
}
