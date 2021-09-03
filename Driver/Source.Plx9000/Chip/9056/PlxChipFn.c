/*******************************************************************************
 * Copyright 2013-2018 Avago Technologies
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
 *      PlxChipFn.c
 *
 * Description:
 *
 *      Contains PLX chip-specific support functions
 *
 * Revision History:
 *
 *      01-01-18 : PLX SDK v8.00
 *
 ******************************************************************************/


#include "PlxChipFn.h"
#include "PlxInterrupt.h"
#include "SuppFunc.h"




/******************************************************************************
 *
 * Function   :  PlxChipInterruptsEnable
 *
 * Description:  Globally enables PLX chip interrupts
 *
 *****************************************************************************/
BOOLEAN
PlxChipInterruptsEnable(
    DEVICE_EXTENSION *pdx
    )
{
    PLX_REG_DATA RegData;


    // Setup for synchronized register access
    RegData.pdx         = pdx;
    RegData.offset      = PCI9056_INT_CTRL_STAT;
    RegData.BitsToSet   = (1 << 8);
    RegData.BitsToClear = 0;

    PlxSynchronizedRegisterModify(
        &RegData
        );

    return TRUE;
}




/******************************************************************************
 *
 * Function   :  PlxChipInterruptsDisable
 *
 * Description:  Globally disables PLX chip interrupts
 *
 *****************************************************************************/
BOOLEAN
PlxChipInterruptsDisable(
    DEVICE_EXTENSION *pdx
    )
{
    PLX_REG_DATA RegData;


    // Setup for synchronized register access
    RegData.pdx         = pdx;
    RegData.offset      = PCI9056_INT_CTRL_STAT;
    RegData.BitsToSet   = 0;
    RegData.BitsToClear = (1 << 8);

    PlxSynchronizedRegisterModify(
        &RegData
        );

    return TRUE;
}




/******************************************************************************
 *
 * Function   :  PlxChipSetInterruptNotifyFlags
 *
 * Description:  Sets the interrupt notification flags of a wait object
 *
 ******************************************************************************/
VOID
PlxChipSetInterruptNotifyFlags(
    PLX_INTERRUPT   *pPlxIntr,
    PLX_WAIT_OBJECT *pWaitObject
    )
{
    // Clear notify events
    pWaitObject->Notify_Flags = INTR_TYPE_NONE;

    if (pPlxIntr->PciAbort)
        pWaitObject->Notify_Flags |= INTR_TYPE_PCI_ABORT;

    if (pPlxIntr->LocalToPci & (1 << 0))
        pWaitObject->Notify_Flags |= INTR_TYPE_LOCAL_1;

    if (pPlxIntr->DmaDone & (1 << 0))
        pWaitObject->Notify_Flags |= INTR_TYPE_DMA_0;

    if (pPlxIntr->DmaDone & (1 << 1))
        pWaitObject->Notify_Flags |= INTR_TYPE_DMA_1;

    if (pPlxIntr->MuOutboundPost)
        pWaitObject->Notify_Flags |= INTR_TYPE_OUTBOUND_POST;

    pWaitObject->Notify_Doorbell = pPlxIntr->Doorbell;
}




/******************************************************************************
 *
 * Function   :  PlxChipSetInterruptStatusFlags
 *
 * Description:  Sets the interrupts that triggered notification
 *
 ******************************************************************************/
VOID
PlxChipSetInterruptStatusFlags(
    PLX_INTERRUPT_DATA *pIntData,
    PLX_INTERRUPT      *pPlxIntr
    )
{
    // Clear all interrupt flags
    RtlZeroMemory(
        pPlxIntr,
        sizeof(PLX_INTERRUPT)
        );

    if (pIntData->Source_Ints & INTR_TYPE_PCI_ABORT)
        pPlxIntr->PciAbort = 1;

    if (pIntData->Source_Ints & INTR_TYPE_LOCAL_1)
        pPlxIntr->LocalToPci = (1 << 0);

    if (pIntData->Source_Ints & INTR_TYPE_DMA_0)
        pPlxIntr->DmaDone |= (1 << 0);

    if (pIntData->Source_Ints & INTR_TYPE_DMA_1)
        pPlxIntr->DmaDone |= (1 << 1);

    if (pIntData->Source_Ints & INTR_TYPE_OUTBOUND_POST)
        pPlxIntr->MuOutboundPost = 1;

    pPlxIntr->Doorbell = pIntData->Source_Doorbell;
}




/*******************************************************************************
 *
 * Function   :  PlxChipTypeDetect
 *
 * Description:  Attempts to determine PLX chip type and revision
 *
 ******************************************************************************/
PLX_STATUS
PlxChipTypeDetect(
    DEVICE_EXTENSION *pdx
    )
{
    U32 RegValue;


    // Set default values
    pdx->Key.PlxChip     = PLX_CHIP_TYPE;
    pdx->Key.PlxRevision = pdx->Key.Revision;
    pdx->Key.PlxFamily   = PLX_FAMILY_BRIDGE_P2L;

    // Check hard-coded ID
    RegValue =
        PLX_9000_REG_READ(
            pdx,
            0x70
            );

    if ((RegValue & 0xFFFF) == PLX_PCI_VENDOR_ID_PLX)
    {
        pdx->Key.PlxChip = (U16)(RegValue >> 16);

        // Get revision
        RegValue =
            PLX_9000_REG_READ(
                pdx,
                0x74           // Revision ID
                );

        pdx->Key.PlxRevision = (U8)RegValue;
    }

    DebugPrintf((
        "Device %04X_%04X = %04X rev %02X\n",
        pdx->Key.DeviceId, pdx->Key.VendorId,
        pdx->Key.PlxChip, pdx->Key.PlxRevision
        ));

    return PLX_STATUS_OK;
}




/******************************************************************************
 *
 * Function   :  PlxChipGetRemapOffset
 *
 * Description:  Returns the remap register offset for a PCI BAR space
 *
 ******************************************************************************/
VOID
PlxChipGetRemapOffset(
    DEVICE_EXTENSION *pdx,
    U8                BarIndex,
    U16              *pOffset_RegRemap
    )
{
    U32     RegValue;
    BOOLEAN bBarsShifted;


    // Check if BAR2/BAR3 are shifted to BAR0/BAR1
    RegValue =
        PLX_9000_REG_READ(
            pdx,
            PCI9056_ENDIAN_DESC
            );

    if ((RegValue & 0x300) == 0x200)
    {
        bBarsShifted = TRUE;
    }
    else
    {
        bBarsShifted = FALSE;
    }

    switch (BarIndex)
    {
        case 0:
            /**************************************************
             * Space 1 is a special case.  If the I2O Decode
             * enable bit is set, BAR3 is moved to BAR0
             *************************************************/

            // Check if I2O decode is enabled
            RegValue =
                PLX_9000_REG_READ(
                    pdx,
                    PCI9056_FIFO_CTRL_STAT
                    );

            if (RegValue & (1 << 0))
            {
                // I2O Decode is enbled, use BAR0 for Space 1
                *pOffset_RegRemap = PCI9056_SPACE1_REMAP;
                return;
            }
            break;

        case 1:
            // BAR 1 could be Space 0 if shifted
            if (bBarsShifted)
            {
                *pOffset_RegRemap = PCI9056_SPACE0_REMAP;
                return;
            }
            break;

        case 2:
            // BAR 2 could be Space 0 or Space 1 if shifted
            if (bBarsShifted)
                *pOffset_RegRemap = PCI9056_SPACE1_REMAP;
            else
                *pOffset_RegRemap = PCI9056_SPACE0_REMAP;
            return;

        case 3:
            // BAR 3 can only be Space 1
            *pOffset_RegRemap = PCI9056_SPACE1_REMAP;
            return;
    }

    DebugPrintf(("ERROR - Invalid Space\n"));

    // BAR not supported
    *pOffset_RegRemap = (U16)-1;
}
