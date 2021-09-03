/*******************************************************************************
 * Copyright 2013-2016 Avago Technologies
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
 *      12-01-16 : PLX SDK v7.25
 *
 ******************************************************************************/


#include "PciFunc.h"
#include "PciRegs.h"
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
    U8  bSearch;
    U8  bPCIeCap;
    U8  instanceNum;
    U8  offsetChipId;
    U16 capID;
    U16 vsecID;
    U16 offset;
    U16 deviceId;
    U32 regValue;


    // Default revision to PCI revision
    pdx->Key.PlxRevision = pdx->Key.Revision;

    // Default to search for 1st PCI VSEC
    capID        = PCI_CAP_ID_VENDOR_SPECIFIC;
    bPCIeCap     = FALSE;
    instanceNum  = 0;
    offsetChipId = 0x18;    // PCI VSEC: ID @ 18h in capability

    /*********************************************************************
     * Search device's PCI/PCIe VSEC capabilities for hard-coded ID. VSEC
     * ID capability start offset varies between different chips. Most
     * possible offsets include DCh, E0h, 950h, & B70h.
     ********************************************************************/
    bSearch = TRUE;
    do
    {
        // Get next VSEC capability instance
        offset =
            PlxPciFindCapability(
                pdx,
                capID,
                bPCIeCap,
                instanceNum
                );

        if (offset == 0)
        {
            // No VSEC found
            if (bPCIeCap == TRUE)
            {
                // Have already scanned both PCI/PCIe, halt search
                bSearch = FALSE;
            }
            else
            {
                // PCI VSEC search ended, jump to PCIe
                capID        = PCIE_CAP_ID_VENDOR_SPECIFIC;
                bPCIeCap     = TRUE;
                instanceNum  = 0;
                offsetChipId = 0x8; // PCIe VSEC: ID @ 08h in capability
            }
        }
        else
        {
            // Check VSEC-specific version
            if (bPCIeCap == TRUE)
            {
                // 4h[15:0] is used for VSEC ID per PCIe
                PLX_PCI_REG_READ(
                    pdx,
                    offset + sizeof(U32),
                    &regValue
                    );
                vsecID = (U16)(regValue >> 0);

                // Valid ID is 1
                if (vsecID != 1)
                {
                    offset = 0;
                }
            }
            else
            {
                // 0h[31:24] is used for VSEC ID since not used per PCI spec
                PLX_PCI_REG_READ(
                    pdx,
                    offset,
                    &regValue
                    );
                vsecID = (U8)(regValue >> 24);

                // Valid IDs are 0 or 1
                if ((vsecID != 0) && (vsecID != 1))
                {
                    offset = 0;
                }
            }

            // If VSEC ID is valid, continue to check for hard-coded ID
            if (offset != 0)
            {
                // Set to chip ID offset
                offset += offsetChipId;

                // Check for hard-coded ID
                PLX_PCI_REG_READ(
                    pdx,
                    offset,
                    &regValue
                    );

                if (((regValue & 0xFFFF) == PLX_PCI_VENDOR_ID_PLX) ||
                    ((regValue & 0xFFFF) == PLX_PCI_VENDOR_ID_LSI))
                {
                    pdx->Key.PlxChip = (U16)(regValue >> 16);

                    // Some chips do not have updated hard-coded revision ID
                    if ((pdx->Key.PlxChip != 0x8612) &&
                        (pdx->Key.PlxChip != 0x8616) &&
                        (pdx->Key.PlxChip != 0x8624) &&
                        (pdx->Key.PlxChip != 0x8632) &&
                        (pdx->Key.PlxChip != 0x8647) &&
                        (pdx->Key.PlxChip != 0x8648))
                    {
                        // Revision should be in next register
                        PLX_PCI_REG_READ(
                            pdx,
                            offset + sizeof(U32),
                            &regValue
                            );

                        pdx->Key.PlxRevision = (U8)(regValue & 0xFF);
                    }

                    // Override revision ID if necessary
                    if ( ((pdx->Key.PlxChip & 0xFF00) == 0xC000) &&
                          (pdx->Key.PlxRevision == 0xAA) )
                    {
                        pdx->Key.PlxRevision = 0xA0;
                    }

                    // Skip to assigning family
                    goto _PlxChipAssignFamily;
                }
            }

            // VSEC does not contain hard-coded ID, so try next instance
            instanceNum++;
        }
    }
    while (bSearch == TRUE);

    //
    // Hard-coded ID doesn't exist, revert to Device/Vendor ID
    //

    // Get current device ID
    deviceId = pdx->Key.DeviceId;

    // Since hard-coded ID doesn't exist, use Device ID
    pdx->Key.PlxChip = pdx->Key.DeviceId;

    // Group some devices
    if ((pdx->Key.VendorId == PLX_PCI_VENDOR_ID_PLX) ||
        (pdx->Key.VendorId == PLX_PCI_VENDOR_ID_LSI))
    {
        switch (deviceId & 0xFF00)
        {
            case 0x2300:
            case 0x3300:
            case 0x8500:
            case 0x8600:
            case 0x8700:
            case 0x9700:
            case 0xC000:
            case 0x8100:
                // Don't include 8311 RDK
                if (deviceId != 0x86e1)
                {
                    deviceId = 0x8000;
                }
                break;
        }
    }
    // Compare Device/Vendor ID
    switch (((U32)deviceId << 16) | pdx->Key.VendorId)
    {
        case 0x800010B5:        // All 8000 series devices
        case 0x80001000:        // All Avago devices
            pdx->Key.PlxChip = pdx->Key.DeviceId;

            // For DMA & NT-Virtual with special ID, use subsystem ID
            if ((pdx->Key.DeviceId == 0x87B0) ||
                (pdx->Key.DeviceId == 0x87D0))
            {
                pdx->Key.PlxChip = pdx->Key.SubDeviceId;
            }
            break;
    }

_PlxChipAssignFamily:

    switch (pdx->Key.PlxChip)
    {
        case 0x8525:
        case 0x8533:
        case 0x8547:
        case 0x8548:
            pdx->Key.PlxFamily = PLX_FAMILY_ALTAIR;
            break;

        case 0x8505:
        case 0x8509:
            pdx->Key.PlxFamily = PLX_FAMILY_ALTAIR_XL;
            break;

        case 0x8516:
        case 0x8524:
        case 0x8532:
            pdx->Key.PlxFamily = PLX_FAMILY_VEGA;
            break;

        case 0x8508:
        case 0x8512:
        case 0x8517:
        case 0x8518:
            pdx->Key.PlxFamily = PLX_FAMILY_VEGA_LITE;
            break;

        case 0x8612:
        case 0x8616:
        case 0x8624:
        case 0x8632:
        case 0x8647:
        case 0x8648:
            pdx->Key.PlxFamily = PLX_FAMILY_DENEB;
            break;

        case 0x8604:
        case 0x8606:
        case 0x8608:
        case 0x8609:
        case 0x8613:
        case 0x8614:
        case 0x8615:
        case 0x8617:
        case 0x8618:
        case 0x8619:
            pdx->Key.PlxFamily = PLX_FAMILY_SIRIUS;
            break;

        case 0x8625:
        case 0x8636:
        case 0x8649:
        case 0x8664:
        case 0x8680:
        case 0x8696:
            pdx->Key.PlxFamily = PLX_FAMILY_CYGNUS;
            break;

        case 0x8700:
            // DMA devices that don't have hard-coded ID
            if ((pdx->Key.DeviceId == 0x87D0) || (pdx->Key.DeviceId == 0x87E0))
            {
                pdx->Key.PlxFamily = PLX_FAMILY_DRACO_1;
            }
            else
            {
                pdx->Key.PlxFamily = PLX_FAMILY_SCOUT;
            }
            break;

        case 0x8712:
        case 0x8716:
        case 0x8724:
        case 0x8732:
        case 0x8747:
        case 0x8748:
            if (pdx->Key.PlxRevision == 0xAA)
            {
                pdx->Key.PlxFamily = PLX_FAMILY_DRACO_1;
            }
            else
            {
                pdx->Key.PlxFamily = PLX_FAMILY_DRACO_2;
            }
            break;

        case 0x8713:
        case 0x8717:
        case 0x8725:
        case 0x8733:
        case 0x8749:
            pdx->Key.PlxFamily = PLX_FAMILY_DRACO_2;
            break;

        case 0x2380:
        case 0x3380:
        case 0x3382:
        case 0x8603:
        case 0x8605:
            pdx->Key.PlxFamily = PLX_FAMILY_MIRA;
            break;

        case 0x8714:
        case 0x8718:
        case 0x8734:
        case 0x8750:
        case 0x8764:
        case 0x8780:
        case 0x8796:
            pdx->Key.PlxFamily = PLX_FAMILY_CAPELLA_1;
            break;

        case 0x9712:
        case 0x9713:
        case 0x9716:
        case 0x9717:
        case 0x9733:
        case 0x9734:
        case 0x9749:
        case 0x9750:
        case 0x9765:
        case 0x9766:
        case 0x9781:
        case 0x9782:
        case 0x9797:
        case 0x9798:
            pdx->Key.PlxFamily = PLX_FAMILY_CAPELLA_2;
            break;

        case 0xC010:
        case 0xC011:
        case 0xC012:
            pdx->Key.PlxFamily = PLX_FAMILY_ATLAS;

        case 0:
            pdx->Key.PlxFamily = PLX_FAMILY_NONE;
            break;

        default:
            DebugPrintf(("ERROR - PLX Family not set for %04X\n", pdx->Key.PlxChip));
            pdx->Key.PlxFamily = PLX_FAMILY_UNKNOWN;
            break;
    }

    DebugPrintf((
        "Device %04X_%04X = %04X rev %02X\n",
        pdx->Key.DeviceId, pdx->Key.VendorId,
        pdx->Key.PlxChip, pdx->Key.PlxRevision
        ));

    return PLX_STATUS_OK;
}
