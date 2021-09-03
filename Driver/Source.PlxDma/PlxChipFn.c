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
    U8  channel;
    U16 OffsetIntCtrl;
    U32 RegValue;


    for (channel = 0; channel < pdx->NumDmaChannels; channel++)
    {
        // Determine DMA interrupt control register offset
        OffsetIntCtrl = 0x23C + (channel * 0x100);

        // Enable all possible interrupts
        RegValue = PLX_DMA_REG_READ( pdx, OffsetIntCtrl );

        RegValue |= ( (1 << 5) | (1 << 4) | (1 << 3) | (1 << 1) | (1 << 0) );

        PLX_DMA_REG_WRITE( pdx, OffsetIntCtrl, RegValue );
    }

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
    U8  channel;
    U16 OffsetIntCtrl;
    U32 RegValue;


    for (channel = 0; channel < pdx->NumDmaChannels; channel++)
    {
        // Determine DMA interrupt control register offset
        OffsetIntCtrl = 0x23C + (channel * 0x100);

        // Mask all possible interrupts
        RegValue = PLX_DMA_REG_READ( pdx, OffsetIntCtrl );

        RegValue &= ~( (1 << 5) | (1 << 4) | (1 << 3) | (1 << 1) | (1 << 0) );

        PLX_DMA_REG_WRITE( pdx, OffsetIntCtrl, RegValue );
    }

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
    DEVICE_EXTENSION *pdx,
    PLX_INTERRUPT    *pPlxIntr,
    PLX_WAIT_OBJECT  *pWaitObject
    )
{
    U8 channel;
    U8 BitShift;


    // Clear notify events
    pWaitObject->Notify_Flags = INTR_TYPE_NONE;

    for (channel = 0; channel < pdx->NumDmaChannels; channel++)
    {
        // Set bit shift
        BitShift = (channel * 8);

        // Set interrupt flags
        if (pPlxIntr->DmaDone & (1 << channel))
            pWaitObject->Notify_Flags |= (INTR_TYPE_DESCR_DMA_DONE << BitShift);

        if (pPlxIntr->DmaPauseDone & (1 << channel))
            pWaitObject->Notify_Flags |= (INTR_TYPE_PAUSE_DONE << BitShift);

        if (pPlxIntr->DmaAbortDone & (1 << channel))
            pWaitObject->Notify_Flags |= (INTR_TYPE_ABORT_DONE << BitShift);

        if (pPlxIntr->DmaImmedStopDone & (1 << channel))
            pWaitObject->Notify_Flags |= (INTR_TYPE_IMMED_STOP_DONE << BitShift);

        if (pPlxIntr->DmaInvalidDescr & (1 << channel))
            pWaitObject->Notify_Flags |= (INTR_TYPE_DESCR_INVALID << BitShift);

        if (pPlxIntr->DmaError & (1 << channel))
            pWaitObject->Notify_Flags |= (INTR_TYPE_DMA_ERROR << BitShift);
    }
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
    DEVICE_EXTENSION   *pdx,
    PLX_INTERRUPT_DATA *pIntData,
    PLX_INTERRUPT      *pPlxIntr
    )
{
    U8 channel;
    U8 BitShift;


    // Clear all interrupt flags
    RtlZeroMemory(
        pPlxIntr,
        sizeof(PLX_INTERRUPT)
        );

    for (channel = 0; channel < pdx->NumDmaChannels; channel++)
    {
        // Set bit shift
        BitShift = (channel * 8);

        // Set interrupt flags
        if (pIntData->Source_Ints & (INTR_TYPE_DESCR_DMA_DONE << BitShift))
            pPlxIntr->DmaDone |= (1 << channel);

        if (pIntData->Source_Ints & (INTR_TYPE_PAUSE_DONE << BitShift))
            pPlxIntr->DmaPauseDone |= (1 << channel);

        if (pIntData->Source_Ints & (INTR_TYPE_ABORT_DONE << BitShift))
            pPlxIntr->DmaAbortDone |= (1 << channel);

        if (pIntData->Source_Ints & (INTR_TYPE_IMMED_STOP_DONE << BitShift))
            pPlxIntr->DmaImmedStopDone |= (1 << channel);

        if (pIntData->Source_Ints & (INTR_TYPE_DESCR_INVALID << BitShift))
            pPlxIntr->DmaInvalidDescr |= (1 << channel);

        if (pIntData->Source_Ints & (INTR_TYPE_DMA_ERROR << BitShift))
            pPlxIntr->DmaError |= (1 << channel);
    }
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
    // Update PLX chip information
    pdx->Key.PlxChip     = pdx->Key.DeviceId;
    pdx->Key.PlxRevision = pdx->Key.Revision;

    // Attempt to use Subsytem ID for DMA devices that don't have hard-coded ID
    if ((pdx->Key.DeviceId == 0x87D0) || (pdx->Key.DeviceId == 0x87E0))
    {
        if ((pdx->Key.SubVendorId == PLX_VENDOR_ID) &&
            ((pdx->Key.SubDeviceId & 0xFF00) == 0x8700))
        {
            pdx->Key.PlxChip = pdx->Key.SubDeviceId;
        }
    }

    switch (pdx->Key.PlxChip)
    {
        case 0x8609:
        case 0x8615:
        case 0x8618:
            pdx->Key.PlxFamily = PLX_FAMILY_SIRIUS;
            break;

        case 0x8700:
            pdx->Key.PlxFamily = PLX_FAMILY_SCOUT;
            break;

        case 0x8712:
        case 0x8716:
        case 0x8724:
        case 0x8732:
        case 0x8747:
        case 0x8748:
            pdx->Key.PlxFamily = PLX_FAMILY_DRACO_1;
            break;

        case 0x8713:
        case 0x8715:
        case 0x8717:
        case 0x8725:
        case 0x8727:
        case 0x8733:
        case 0x8749:
        case 0x87D0:
        case 0x87E0:
            pdx->Key.PlxFamily = PLX_FAMILY_DRACO_2;
            break;

        default:
            pdx->Key.PlxFamily = PLX_FAMILY_UNKNOWN;
            break;
    }

    DebugPrintf((
        "Device %04X_%04X = %04X rev %02X\n",
        pdx->Key.DeviceId, pdx->Key.VendorId,
        pdx->Key.PlxChip, pdx->Key.PlxRevision
        ));

    return ApiSuccess;
}
