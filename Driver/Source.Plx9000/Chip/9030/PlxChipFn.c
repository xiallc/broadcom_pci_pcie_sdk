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
 *      PlxChipFn.c
 *
 * Description:
 *
 *      Contains PLX chip-specific support functions
 *
 * Revision History:
 *
 *      02-01-14 : PLX SDK v7.20
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
    RegData.offset      = PCI9030_INT_CTRL_STAT;
    RegData.BitsToSet   = (1 << 6);
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
    RegData.offset      = PCI9030_INT_CTRL_STAT;
    RegData.BitsToSet   = 0;
    RegData.BitsToClear = (1 << 6);

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
    pWaitObject->Notify_Flags    = INTR_TYPE_NONE;
    pWaitObject->Notify_Doorbell = 0;

    if (pPlxIntr->LocalToPci & (1 << 0))
        pWaitObject->Notify_Flags |= INTR_TYPE_LOCAL_1;

    if (pPlxIntr->LocalToPci & (1 << 1))
        pWaitObject->Notify_Flags |= INTR_TYPE_LOCAL_2;

    if (pPlxIntr->SwInterrupt)
        pWaitObject->Notify_Flags |= INTR_TYPE_SOFTWARE;
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

    if (pIntData->Source_Ints & INTR_TYPE_LOCAL_1)
        pPlxIntr->LocalToPci |= (1 << 0);

    if (pIntData->Source_Ints & INTR_TYPE_LOCAL_2)
        pPlxIntr->LocalToPci |= (1 << 1);

    if (pIntData->Source_Ints & INTR_TYPE_SOFTWARE)
        pPlxIntr->SwInterrupt = 1;
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
    pdx->Key.PlxChip     = PLX_CHIP_TYPE;
    pdx->Key.PlxRevision = 0xAA;
    pdx->Key.PlxFamily   = PLX_FAMILY_BRIDGE_P2L;

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
    switch (BarIndex)
    {
        case 2:
            *pOffset_RegRemap = PCI9030_REMAP_SPACE0;
            return;

        case 3:
            *pOffset_RegRemap = PCI9030_REMAP_SPACE1;
            return;

        case 4:
            *pOffset_RegRemap = PCI9030_REMAP_SPACE2;
            return;

        case 5:
            *pOffset_RegRemap = PCI9030_REMAP_SPACE3;
            return;
    }

    DebugPrintf(("ERROR - Invalid Space\n"));

    // BAR not supported
    *pOffset_RegRemap = (U16)-1;
}
