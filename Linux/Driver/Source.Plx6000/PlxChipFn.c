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
 *      05-01-10 : PLX SDK v6.40
 *
 ******************************************************************************/


#include "PciFunc.h"
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
    U32 RegValue;


    // Enable doorbell interrupts
    PLX_PCI_REG_READ(
        pdx,
        0xc4,
        &RegValue
        );

    RegValue |= 0xFFFF;

    PLX_PCI_REG_WRITE(
        pdx,
        0xc4,
        RegValue
        );

    // Enable Message, S_RSTIN, S_PME, & GPIO interrupts
    PLX_PCI_REG_WRITE(
        pdx,
        0xc8,
        0xFF000000
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
    U32 RegValue;


    // Disable doorbell interrupts
    PLX_PCI_REG_READ(
        pdx,
        0xc4,
        &RegValue
        );

    RegValue &= ~0xFFFF;

    PLX_PCI_REG_WRITE(
        pdx,
        0xc4,
        RegValue
        );

    // Disable Message, S_RSTIN, S_PME, & GPIO interrupts
    PLX_PCI_REG_WRITE(
        pdx,
        0xc8,
        0x00000000
        );

    return TRUE;
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
    U32 DevVenId;


    // Set default values
    pdx->Key.PlxChip     = 0;
    pdx->Key.PlxRevision = pdx->Key.Revision;
    pdx->Key.PlxFamily   = PLX_FAMILY_BRIDGE_PCI_P2P;

    DevVenId = ((U32)pdx->Key.DeviceId << 16) | pdx->Key.VendorId;

    switch (DevVenId)
    {
        case 0x00213388:        // 6254 - NT mode
            if (pdx->Key.Revision == 0x4)
            {
                pdx->Key.PlxChip     = 0x6254;
                pdx->Key.PlxRevision = 0xBB;
            }
            break;

        case 0x00293388:        // 6540 - NT mode
        case 0x654110B5:        // 6540 - NT mode
        case 0x654210B5:        // 6540 - NT mode (Secondary side)
            pdx->Key.PlxChip = 0x6540;

            if (pdx->Key.Revision == 0x2)
                pdx->Key.PlxRevision = 0xBB;
            break;

        default:
            DebugPrintf(("ERROR - Unable to determine chip type\n"));
            return ApiInvalidDeviceInfo;
    }

    DebugPrintf((
        "Device %04X_%04X = %04X rev %02X\n",
        pdx->Key.DeviceId, pdx->Key.VendorId,
        pdx->Key.PlxChip, pdx->Key.PlxRevision
        ));

    return ApiSuccess;
}




/******************************************************************************
 *
 * Function   :  PlxDetermineNtPortSide
 *
 * Description:  Determines whether the NT port is Virtual or Link side
 *
 ******************************************************************************/
BOOLEAN
PlxDetermineNtPortSide(
    DEVICE_EXTENSION *pdx
    )
{
    // On 6000-series devices, bit 0 set if primary side & bit 1 set for secondary
    if (pdx->Key.DeviceId & (1 << 0))
        pdx->Key.NTPortType = PLX_NT_PORT_PRIMARY;
    else if (pdx->Key.DeviceId & (1 << 1))
        pdx->Key.NTPortType = PLX_NT_PORT_SECONDARY;
    else
    {
        DebugPrintf(("Error: Unable to determine NT side\n"));
        return FALSE;
    }

    DebugPrintf((
        "NT port is %s-side\n",
        (pdx->Key.NTPortType == PLX_NT_PORT_PRIMARY) ? "Primary" : "Secondary"
        ));

    return TRUE;
}
