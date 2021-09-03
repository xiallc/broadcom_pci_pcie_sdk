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
 *      PlxChipFn.c
 *
 * Description:
 *
 *      Contains PLX chip-specific support functions
 *
 * Revision History:
 *
 *      03-01-10 : PLX SDK v6.40
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

    return ApiSuccess;
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
