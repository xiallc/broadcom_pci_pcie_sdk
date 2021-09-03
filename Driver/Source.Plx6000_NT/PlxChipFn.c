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
            {
                pdx->Key.PlxRevision = 0xBB;
            }
            break;

        default:
            DebugPrintf(("ERROR - Unable to determine chip type\n"));
            return PLX_STATUS_INVALID_OBJECT;
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
    {
        pdx->Key.PlxPortType = PLX_SPEC_PORT_NT_VIRTUAL;
    }
    else if (pdx->Key.DeviceId & (1 << 1))
    {
        pdx->Key.PlxPortType = PLX_SPEC_PORT_NT_LINK;
    }
    else
    {
        DebugPrintf(("Error: Unable to determine NT side\n"));
        return FALSE;
    }

    DebugPrintf((
        "NT port is %s-side\n",
        (pdx->Key.PlxPortType == PLX_SPEC_PORT_NT_VIRTUAL) ? "Primary" : "Secondary"
        ));

    return TRUE;
}
