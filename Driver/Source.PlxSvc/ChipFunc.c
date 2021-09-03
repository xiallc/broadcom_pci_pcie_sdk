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
 *      ChipFunc.c
 *
 * Description:
 *
 *      This file contains the PLX chip-specific functions
 *
 * Revision History:
 *
 *      03-01-18 : PLX SDK v8.00
 *
 ******************************************************************************/


#include "ApiFunc.h"
#include "ChipFunc.h"
#include "DrvDefs.h"
#include "PciFunc.h"
#include "PciRegs.h"
#include "SuppFunc.h"




/******************************************************************************
 *
 * Function   :  PlxRegisterRead_8111
 *
 * Description:  Reads an 8111 PLX-specific register
 *
 *****************************************************************************/
U32
PlxRegisterRead_8111(
    PLX_DEVICE_NODE *pNode,
    U32              offset,
    PLX_STATUS      *pStatus
    )
{
    U32 value;
    U32 RegSave;


    // Verify register offset
    if ((offset < 0x1000) || (offset > 0x1064))
    {
        if (pStatus != NULL)
        {
            *pStatus = PLX_STATUS_INVALID_OFFSET;
        }
        return 0;
    }

    // Adjust offset
    offset -= 0x1000;

    // Save the current index register
    PLX_PCI_REG_READ( pNode, 0x84, &RegSave );

    // Set the new index
    PLX_PCI_REG_WRITE( pNode, 0x84, offset );

    // Get the value
    PLX_PCI_REG_READ( pNode, 0x88, &value );

    // Restore the current index register
    PLX_PCI_REG_WRITE( pNode, 0x84, RegSave );

    if (pStatus != NULL)
    {
        *pStatus = PLX_STATUS_OK;
    }

    return value;
}




/******************************************************************************
 *
 * Function   :  PlxRegisterWrite_8111
 *
 * Description:  Writes to an 8111 PLX-specific control register
 *
 *****************************************************************************/
PLX_STATUS
PlxRegisterWrite_8111(
    PLX_DEVICE_NODE *pNode,
    U32              offset,
    U32              value
    )
{
    U32 RegSave;


    // Verify register offset
    if ((offset < 0x1000) || (offset > 0x1064))
    {
        return PLX_STATUS_INVALID_OFFSET;
    }

    // Adjust offset
    offset -= 0x1000;

    // Save the current index register
    PLX_PCI_REG_READ( pNode, 0x84, &RegSave );

    // Set the new index
    PLX_PCI_REG_WRITE( pNode, 0x84, offset );

    // Write the value
    PLX_PCI_REG_WRITE( pNode, 0x88, value );

    // Restore the current index register
    PLX_PCI_REG_WRITE( pNode, 0x84, RegSave );

    return PLX_STATUS_OK;
}




/******************************************************************************
 *
 * Function   :  PlxRegisterRead_8000
 *
 * Description:  Reads an 8000 PLX-specific register
 *
 *****************************************************************************/
U32
PlxRegisterRead_8000(
    PLX_DEVICE_NODE *pNode,
    U32              offset,
    PLX_STATUS      *pStatus,
    BOOLEAN          bAdjustForPort
    )
{
    int rc;
    U32 OffsetAdjustment;


    // Verify that register access is setup
    if (pNode->pRegNode == NULL)
    {
        DebugPrintf(("ERROR: Register access not setup\n"));
        if (pStatus != NULL)
        {
            *pStatus = PLX_STATUS_INVALID_DATA;
        }
        return 0;
    }


    // In fabric mode chips, downstream ports may not point to GEP for
    // register access due to device detection order, so update here
    if ( (pNode->pRegNode->pRegNode != NULL) &&
         (pNode->pRegNode != pNode->pRegNode->pRegNode) )
    {
        // Synchronize reg node to current, correct one
        pNode->pRegNode = pNode->pRegNode->pRegNode;
    }

    // Check if BAR 0 has been mapped
    if (pNode->pRegNode->PciBar[0].pVa == NULL)
    {
        DebugPrintf(("Map BAR 0 for PLX reg access\n"));

        // Attempt to map BAR 0
        rc = PlxPciBarResourceMap( pNode->pRegNode, 0 );
        if (rc != 0)
        {
            DebugPrintf(("ERROR: Unable to map BAR 0 for PLX registers\n"));
            if (pStatus != NULL)
            {
                *pStatus = PLX_STATUS_INSUFFICIENT_RES;
            }
            return 0;
        }
    }

    // Adjust offset for port if requested
    if (bAdjustForPort)
    {
        OffsetAdjustment = 0;

        if ((pNode->Key.PlxPortType == PLX_SPEC_PORT_UPSTREAM) ||
            (pNode->Key.PlxPortType == PLX_SPEC_PORT_DOWNSTREAM))
        {
            // Update port properties if haven't yet
            if (pNode->PortProp.PortType == PLX_PORT_UNKNOWN)
            {
                PlxGetPortProperties( pNode, &pNode->PortProp );
            }

            // Adjust the offset based on port number
            OffsetAdjustment = (pNode->PortProp.PortNumber * (4 * 1024));

            // Port-specific registers start at offset 8MB in Atlas
            if (pNode->Key.PlxFamily == PLX_FAMILY_ATLAS)
            {
                OffsetAdjustment += 0x800000;
            }
        }
        else if ((pNode->Key.PlxPortType == PLX_SPEC_PORT_NT_VIRTUAL) ||
                 (pNode->Key.PlxPortType == PLX_SPEC_PORT_NT_LINK))
        {
            // Add base for NT port
            OffsetAdjustment = pNode->Offset_NtRegBase;
        }

        // For MIRA enhanced mode, USB EP regs start at 0 instead of port 3
        if ((pNode->Key.PlxFamily == PLX_FAMILY_MIRA) &&
            (pNode->PciHeaderType == PCI_HDR_TYPE_0) &&
            (pNode->PortProp.PortNumber == 3))
        {
            DebugPrintf(("Override offset adjust for MIRA USB EP (3000 ==> 0)\n"));
            OffsetAdjustment = 0;
        }

        DebugPrintf((
            "Adjust offset by %02X for port %d\n",
            (int)OffsetAdjustment, pNode->PortProp.PortNumber
            ));

        offset += OffsetAdjustment;
    }

    // Verify offset
    if (offset >= pNode->pRegNode->PciBar[0].Properties.Size)
    {
        DebugPrintf(("Error - Offset (%02X) exceeds maximum\n", (unsigned)offset));
        if (pStatus != NULL)
        {
            *pStatus = PLX_STATUS_INVALID_OFFSET;
        }
        return 0;
    }

    if (pStatus != NULL)
    {
        *pStatus = PLX_STATUS_OK;
    }

    // For Draco 1, some register cause problems if accessed
    if (pNode->Key.PlxFamily == PLX_FAMILY_DRACO_1)
    {
        if ((offset == 0x856C)  || (offset == 0x8570) ||
            (offset == 0x1056C) || (offset == 0x10570))
        {
            return 0;
        }
    }

    return PHYS_MEM_READ_32( pNode->pRegNode->PciBar[0].pVa + offset );
}




/******************************************************************************
 *
 * Function   :  PlxRegisterWrite_8000
 *
 * Description:  Writes to an 8000 PLX-specific control register
 *
 *****************************************************************************/
PLX_STATUS
PlxRegisterWrite_8000(
    PLX_DEVICE_NODE *pNode,
    U32              offset,
    U32              value,
    BOOLEAN          bAdjustForPort
    )
{
    int rc;
    U32 OffsetAdjustment;


    // Verify that register access is setup
    if (pNode->pRegNode == NULL)
    {
        DebugPrintf(("ERROR: Register access not setup, unable to access PLX registers\n"));
        return PLX_STATUS_INVALID_DATA;
    }

    // In fabric mode chips, downstream ports may not point to GEP for
    // register access due to device detection order, so update here
    if ( (pNode->pRegNode->pRegNode != NULL) &&
         (pNode->pRegNode != pNode->pRegNode->pRegNode) )
    {
        // Synchronize reg node to current, correct one
        pNode->pRegNode = pNode->pRegNode->pRegNode;
    }

    // Check if BAR 0 has been mapped
    if (pNode->pRegNode->PciBar[0].pVa == NULL)
    {
        DebugPrintf(("Map BAR 0 for PLX reg access\n"));

        // Attempt to map BAR 0
        rc = PlxPciBarResourceMap( pNode->pRegNode, 0 );
        if (rc != 0)
        {
            DebugPrintf(("ERROR: Unable to map BAR 0 for PLX registers\n"));
            return PLX_STATUS_INSUFFICIENT_RES;
        }
    }

    // Adjust offset for port if requested
    if (bAdjustForPort)
    {
        OffsetAdjustment = 0;

        if ((pNode->Key.PlxPortType == PLX_SPEC_PORT_UPSTREAM) ||
            (pNode->Key.PlxPortType == PLX_SPEC_PORT_DOWNSTREAM))
        {
            // Update port properties if haven't yet
            if (pNode->PortProp.PortType == PLX_PORT_UNKNOWN)
            {
                PlxGetPortProperties( pNode, &pNode->PortProp );
            }

            // Adjust the offset based on port number
            OffsetAdjustment = (pNode->PortProp.PortNumber * (4 * 1024));

            // Port-specific registers start at offset 8MB in Atlas
            if (pNode->Key.PlxFamily == PLX_FAMILY_ATLAS)
            {
                OffsetAdjustment += 0x800000;
            }
        }
        else if ((pNode->Key.PlxPortType == PLX_SPEC_PORT_NT_VIRTUAL) ||
                 (pNode->Key.PlxPortType == PLX_SPEC_PORT_NT_LINK))
        {
            // Add base for NT port
            OffsetAdjustment = pNode->Offset_NtRegBase;
        }

        // For MIRA enhanced mode, USB EP regs start at 0 instead of port 3
        if ((pNode->Key.PlxFamily == PLX_FAMILY_MIRA) &&
            (pNode->PciHeaderType == PCI_HDR_TYPE_0) &&
            (pNode->PortProp.PortNumber == 3))
        {
            DebugPrintf(("Override offset adjust for MIRA USB EP (3000 ==> 0)\n"));
            OffsetAdjustment = 0;
        }

        DebugPrintf((
            "Adjust offset by %02X for port %d\n",
            (int)OffsetAdjustment, pNode->PortProp.PortNumber
            ));

        offset += OffsetAdjustment;
    }

    // Verify offset
    if (offset >= pNode->pRegNode->PciBar[0].Properties.Size)
    {
        DebugPrintf(("Error - Offset (%02X) exceeds maximum\n", (unsigned)offset));
        return PLX_STATUS_INVALID_OFFSET;
    }

    // For Draco 1, some register cause problems if accessed
    if (pNode->Key.PlxFamily == PLX_FAMILY_DRACO_1)
    {
        if ((offset == 0x856C)  || (offset == 0x8570) ||
            (offset == 0x1056C) || (offset == 0x10570))
        {
            return PLX_STATUS_OK;
        }
    }

    PHYS_MEM_WRITE_32( pNode->pRegNode->PciBar[0].pVa + offset, value );

    return PLX_STATUS_OK;
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
    PLX_DEVICE_NODE *pDevice,
    BOOLEAN          bOnlySetFamily
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


    // Jump to set family if requested
    if (bOnlySetFamily)
    {
        goto _PlxChipAssignFamily;
    }

    // Default revision to PCI revision
    pDevice->Key.PlxRevision = pDevice->Key.Revision;

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
                pDevice,
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
                    pDevice,
                    offset + sizeof(U32),
                    &regValue
                    );
                vsecID = (U16)(regValue >> 0);

                // Valid ID is 1
                if (vsecID != 1)
                {
                    offset = 0;
                }

                // Some Intel devices have a VSEC that matches, so ignore
                if (pDevice->Key.VendorId == PLX_PCI_VENDOR_ID_INTEL)
                {
                    offset = 0;
                }
            }
            else
            {
                // 0h[31:24] is used for VSEC ID since not used per PCI spec
                PLX_PCI_REG_READ(
                    pDevice,
                    offset,
                    &regValue
                    );
                vsecID = (U8)(regValue >> 24);

                // Valid IDs are 0 or 1 & is final capability
                if ( ((vsecID != 0) && (vsecID != 1)) ||
                     ((U8)(regValue >> 8) != 0) )
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
                    pDevice,
                    offset,
                    &regValue
                    );

                if (((regValue & 0xFFFF) == PLX_PCI_VENDOR_ID_PLX) ||
                    ((regValue & 0xFFFF) == PLX_PCI_VENDOR_ID_LSI))
                {
                    pDevice->Key.PlxChip = (U16)(regValue >> 16);

                    // Some chips do not have updated hard-coded revision ID
                    if ((pDevice->Key.PlxChip != 0x8612) &&
                        (pDevice->Key.PlxChip != 0x8616) &&
                        (pDevice->Key.PlxChip != 0x8624) &&
                        (pDevice->Key.PlxChip != 0x8632) &&
                        (pDevice->Key.PlxChip != 0x8647) &&
                        (pDevice->Key.PlxChip != 0x8648))
                    {
                        // Revision should be in next register
                        PLX_PCI_REG_READ(
                            pDevice,
                            offset + sizeof(U32),
                            &regValue
                            );

                        pDevice->Key.PlxRevision = (U8)(regValue & 0xFF);
                    }

                    // Override revision ID if necessary
                    if ( ((pDevice->Key.PlxChip & 0xFF00) == 0xC000) &&
                          (pDevice->Key.PlxRevision == 0xAA) )
                    {
                        pDevice->Key.PlxRevision = 0xA0;
                    }

                    // Override MPT EP chip ID
                    if (pDevice->Key.PlxChip == 0x00B2)
                    {
                        // Use the device or subsystem ID, depending upon which is C0xxh
                        if ((pDevice->Key.DeviceId & 0xFF00) == 0xC000)
                        {
                            pDevice->Key.PlxChip = pDevice->Key.DeviceId;
                        }
                        else if ((pDevice->Key.SubDeviceId & 0xFF00) == 0xC000)
                        {
                            pDevice->Key.PlxChip = pDevice->Key.SubDeviceId;
                        }
                        else
                        {
                            // MPT in BSW doesn't report C0xx in config space, so just pick one
                            pDevice->Key.PlxChip = 0xC012;
                        }
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
    deviceId = pDevice->Key.DeviceId;

    // Group some devices
    if ((pDevice->Key.VendorId == PLX_PCI_VENDOR_ID_PLX) ||
        (pDevice->Key.VendorId == PLX_PCI_VENDOR_ID_LSI))
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
    switch (((U32)deviceId << 16) | pDevice->Key.VendorId)
    {
        case 0x800010B5:        // All 8000 series devices
        case 0x80001000:        // All Avago devices
            pDevice->Key.PlxChip = pDevice->Key.DeviceId;

            // For DMA & NT-Virtual with special ID, use subsystem ID
            if ((pDevice->Key.DeviceId == 0x87B0) ||
                (pDevice->Key.DeviceId == 0x87D0))
            {
                pDevice->Key.PlxChip = pDevice->Key.SubDeviceId;
            }
            break;

        case 0x00B21000:
            // Added for base mode MPT that doesn't yet have PCIe VSEC
            if ( (pDevice->Key.SubVendorId == PLX_PCI_VENDOR_ID_LSI) &&
                 ((pDevice->Key.SubDeviceId & 0xFF00) == 0xA000) )
            {
                pDevice->Key.PlxChip = 0xC010;
            }
            break;

        case 0x905010b5:        // 9050/9052
        case 0x520110b5:        // PLX 9052 RDK
            pDevice->Key.PlxChip = 0x9050;
            break;

        case 0x903010b5:        // 9030
        case 0x300110b5:        // PLX 9030 RDK
        case 0x30c110b5:        // PLX 9030 RDK - cPCI
            pDevice->Key.PlxChip = 0x9030;
            break;

        case 0x908010b5:        // 9080
        case 0x040110b5:        // PLX 9080-401B RDK
        case 0x086010b5:        // PLX 9080-860 RDK
            pDevice->Key.PlxChip = 0x9080;
            break;

        case 0x905410b5:        // 9054
        case 0x540610b5:        // PLX 9054 RDK-LITE
        case 0x186010b5:        // PLX 9054-860 RDK
        case 0xc86010b5:        // PLX 9054-860 RDK - cPCI
            pDevice->Key.PlxChip = 0x9054;
            break;

        case 0x905610b5:        // 9056
        case 0x560110b5:        // PLX 9056 RDK-LITE
        case 0x56c210b5:        // PLX 9056-860 RDK
            pDevice->Key.PlxChip = 0x9056;
            break;

        case 0x965610b5:        // 9656
        case 0x960110b5:        // PLX 9656 RDK-LITE
        case 0x96c210b5:        // PLX 9656-860 RDK
            pDevice->Key.PlxChip = 0x9656;
            break;

        case 0x831110b5:        // 8311
        case 0x86e110b5:        // PLX 8311 RDK
            pDevice->Key.PlxChip = 0x8311;
            break;

        case 0x00213388:        // 6140/6152/6254(NT)
            // Get PCI header type
            PLX_PCI_REG_READ(
                pDevice,
                PCI_REG_HDR_CACHE_LN,
                &regValue
                );
            regValue = (U8)((regValue >> 16) & 0x7F);

            if (regValue == PCI_HDR_TYPE_0)
            {
                pDevice->Key.PlxChip = 0x6254;
            }
            else
            {
                // Get 6152 VPD register
                PLX_PCI_REG_READ(
                    pDevice,
                    0xA0,
                    &regValue
                    );

                if ((regValue & 0xF) == PCI_CAP_ID_VPD)
                {
                    pDevice->Key.PlxChip = 0x6152;
                }
                else
                {
                    pDevice->Key.PlxChip = 0x6140;
                }
            }
            break;

        case 0x00223388:        // 6150/6350
        case 0x00a23388:        // 6350
            if (pDevice->Key.PlxRevision == 0x20)
            {
                pDevice->Key.PlxChip = 0x6350;
            }
            else
            {
                pDevice->Key.PlxChip = 0x6150;
            }
            break;

        case 0x00263388:        // 6154
            pDevice->Key.PlxChip = 0x6154;
            break;

        case 0x00313388:        // 6156
            pDevice->Key.PlxChip = 0x6156;
            break;

        case 0x00203388:        // 6254
            pDevice->Key.PlxChip = 0x6254;
            break;

        case 0x00303388:        // 6520
        case 0x652010B5:        // 6520
            pDevice->Key.PlxChip = 0x6520;
            break;

        case 0x00283388:        // 6540      - Transparent mode
        case 0x654010B5:        // 6540/6466 - Transparent mode
        case 0x00293388:        // 6540      - Non-transparent mode
        case 0x654110B5:        // 6540/6466 - Non-transparent mode
        case 0x654210B5:        // 6540/6466 - Non-transparent mode
            pDevice->Key.PlxChip = 0x6540;
            break;

        // Cases where PCIe regs not accessible, use SubID
        case 0x100810B5:        // PLX Synthetic Enabler EP
        case 0x100910B5:        // PLX GEP
            pDevice->Key.PlxChip = pDevice->Key.SubDeviceId;
            break;
    }

    // Detect the PLX chip revision
    PlxChipRevisionDetect( pDevice );

_PlxChipAssignFamily:

    switch (pDevice->Key.PlxChip)
    {
        case 0x9050:        // 9000 series
        case 0x9030:
        case 0x9080:
        case 0x9054:
        case 0x9056:
        case 0x9656:
        case 0x8311:
            pDevice->Key.PlxFamily = PLX_FAMILY_BRIDGE_P2L;
            break;

        case 0x6140:        // 6000 series
        case 0x6150:
        case 0x6152:
        case 0x6154:
        case 0x6156:
        case 0x6254:
        case 0x6350:
        case 0x6520:
        case 0x6540:
        case 0x6466:
            pDevice->Key.PlxFamily = PLX_FAMILY_BRIDGE_PCI_P2P;
            break;

        case 0x8111:
        case 0x8112:
        case 0x8114:
            pDevice->Key.PlxFamily = PLX_FAMILY_BRIDGE_PCIE_P2P;
            break;

        case 0x8525:
        case 0x8533:
        case 0x8547:
        case 0x8548:
            pDevice->Key.PlxFamily = PLX_FAMILY_ALTAIR;
            break;

        case 0x8505:
        case 0x8509:
            pDevice->Key.PlxFamily = PLX_FAMILY_ALTAIR_XL;
            break;

        case 0x8516:
        case 0x8524:
        case 0x8532:
            pDevice->Key.PlxFamily = PLX_FAMILY_VEGA;
            break;

        case 0x8508:
        case 0x8512:
        case 0x8517:
        case 0x8518:
            pDevice->Key.PlxFamily = PLX_FAMILY_VEGA_LITE;
            break;

        case 0x8612:
        case 0x8616:
        case 0x8624:
        case 0x8632:
        case 0x8647:
        case 0x8648:
            pDevice->Key.PlxFamily = PLX_FAMILY_DENEB;
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
            pDevice->Key.PlxFamily = PLX_FAMILY_SIRIUS;
            break;

        case 0x8625:
        case 0x8636:
        case 0x8649:
        case 0x8664:
        case 0x8680:
        case 0x8696:
            pDevice->Key.PlxFamily = PLX_FAMILY_CYGNUS;
            break;

        case 0x8700:
            // DMA devices that don't have hard-coded ID
            if ((pDevice->Key.DeviceId == 0x87D0) || (pDevice->Key.DeviceId == 0x87E0))
            {
                pDevice->Key.PlxFamily = PLX_FAMILY_DRACO_1;
            }
            else
            {
                pDevice->Key.PlxFamily = PLX_FAMILY_SCOUT;
            }
            break;

        case 0x8712:
        case 0x8716:
        case 0x8723:
        case 0x8724:
        case 0x8732:
        case 0x8747:
        case 0x8748:
            if (pDevice->Key.PlxRevision == 0xAA)
            {
                pDevice->Key.PlxFamily = PLX_FAMILY_DRACO_1;
            }
            else
            {
                pDevice->Key.PlxFamily = PLX_FAMILY_DRACO_2;
            }
            break;

        case 0x8713:
        case 0x8717:
        case 0x8725:
        case 0x8733:
        case 0x8749:
            pDevice->Key.PlxFamily = PLX_FAMILY_DRACO_2;
            break;

        case 0x2380:
        case 0x3380:
        case 0x3382:
        case 0x8603:
        case 0x8605:
            pDevice->Key.PlxFamily = PLX_FAMILY_MIRA;
            break;

        case 0x8714:
        case 0x8718:
        case 0x8734:
        case 0x8750:
        case 0x8764:
        case 0x8780:
        case 0x8796:
            pDevice->Key.PlxFamily = PLX_FAMILY_CAPELLA_1;
            break;

        case 0x9712:
        case 0x9716:
        case 0x9733:
        case 0x9749:
        case 0x9750:
        case 0x9765:
        case 0x9781:
        case 0x9797:
            pDevice->Key.PlxFamily = PLX_FAMILY_CAPELLA_2;
            break;

        case 0xC010:
        case 0xC011:
        case 0xC012:
            pDevice->Key.PlxFamily = PLX_FAMILY_ATLAS;
            break;

        case 0:
            pDevice->Key.PlxFamily = PLX_FAMILY_NONE;
            break;

        default:
            DebugPrintf(("ERROR - PLX Family not set for %04X\n", pDevice->Key.PlxChip));
            pDevice->Key.PlxFamily = PLX_FAMILY_UNKNOWN;
            break;
    }

    return PLX_STATUS_OK;
}




/*******************************************************************************
 *
 * Function   :  PlxChipRevisionDetect
 *
 * Description:  Attempts to detect the PLX chip revision
 *
 ******************************************************************************/
VOID
PlxChipRevisionDetect(
    PLX_DEVICE_NODE *pDevice
    )
{
    // Default revision to value in chip
    pDevice->Key.PlxRevision = pDevice->Key.Revision;

    // Determine if PLX revision should override default
    switch (pDevice->Key.PlxChip)
    {
        case 0x8111:
            if (pDevice->Key.PlxRevision == 0x10)
            {
                pDevice->Key.PlxRevision = 0xAA;
            }
            else if (pDevice->Key.PlxRevision == 0x20)
            {
                pDevice->Key.PlxRevision = 0xBA;
            }
            else if (pDevice->Key.PlxRevision == 0x21)
            {
                pDevice->Key.PlxRevision = 0xBB;
            }
            break;

        case 0x8112:
            pDevice->Key.PlxRevision = 0xAA;
            break;

        case 0x9050:
            if (pDevice->Key.PlxRevision == 0x2)
            {
                pDevice->Key.PlxRevision = 2;
            }
            else
            {
                pDevice->Key.PlxRevision = 1;
            }
            break;

        case 0x9030:
            pDevice->Key.PlxRevision = 0xAA;
            break;

        case 0x9080:
            pDevice->Key.PlxRevision = 3;
            break;

        case 0x9056:
        case 0x9656:
            break;

        case 0x9054:
            // AA & AB versions have same revision ID
            if ((pDevice->Key.PlxRevision == 0x1) ||
                (pDevice->Key.PlxRevision == 0xA) ||
                (pDevice->Key.PlxRevision == 0xB))
            {
                pDevice->Key.PlxRevision = 0xAB;
            }
            else
            {
                pDevice->Key.PlxRevision = 0xAC;
            }
            break;

        case 0x8311:
            pDevice->Key.PlxRevision = 0xAA;
            break;

        case 0x6140:
            if (pDevice->Key.PlxRevision == 0x12)
            {
                pDevice->Key.PlxRevision = 0xAA;
            }
            else
            {
                // Revision 0x13 only other
                pDevice->Key.PlxRevision = 0xDA;
            }
            break;

        case 0x6150:
            if (pDevice->Key.PlxRevision == 0x4)
            {
                pDevice->Key.PlxRevision = 0xBB;
            }
            break;

        case 0x6152:
            switch (pDevice->Key.PlxRevision)
            {
                case 0x13:
                    pDevice->Key.PlxRevision = 0xBA;
                    break;

                case 0x14:
                    pDevice->Key.PlxRevision = 0xCA;
                    break;

                case 0x15:
                    pDevice->Key.PlxRevision = 0xCC;
                    break;

                case 0x16:
                    pDevice->Key.PlxRevision = 0xDA;
                    break;
            }
            break;

        case 0x6154:
            if (pDevice->Key.PlxRevision == 0x4)
            {
                pDevice->Key.PlxRevision = 0xBB;
            }
            break;

        case 0x6350:
            if (pDevice->Key.PlxRevision == 0x20)
            {
                pDevice->Key.PlxRevision = 0xAA;
            }
            break;

        case 0x6156:
            if (pDevice->Key.PlxRevision == 0x1)
            {
                pDevice->Key.PlxRevision = 0xDA;
            }
            break;

        case 0x6254:
            if (pDevice->Key.PlxRevision == 0x4)
            {
                pDevice->Key.PlxRevision = 0xBB;
            }
            break;

        case 0x6520:
            if (pDevice->Key.PlxRevision == 0x2)
            {
                pDevice->Key.PlxRevision = 0xBB;
            }
            break;

        case 0x6540:
            if (pDevice->Key.PlxRevision == 0x2)
            {
                pDevice->Key.PlxRevision = 0xBB;
            }
            break;
    }
}
