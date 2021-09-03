/*******************************************************************************
 * Copyright 2013-2019 Broadcom Inc
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

/*******************************************************************************
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
 *      03-01-19 : PCI/PCIe SDK v8.00
 *
 ******************************************************************************/


#include "ApiFunc.h"
#include "PciFunc.h"
#include "PciRegs.h"
#include "PlxChipFn.h"
#include "PlxInterrupt.h"
#include "SuppFunc.h"




/******************************************************************************
 *
 * Function   :  PlxChipSetInterruptRegisterOffsets
 *
 * Description:  Determines the IRQ register offsets, based on chip type, for the ISR
 *
 *****************************************************************************/
BOOLEAN
PlxChipSetInterruptRegisterOffsets(
    DEVICE_EXTENSION *pdx
    )
{
    U32 OffsetIrqBase;


    /*****************************************************************
     * The registers used are the IRQ Set Mask and IRQ Clear Mask.
     * Calculated offsets are based on the following:
     *
     *    Virtual-side IRQ Set         -- Base + 00h
     *    Virtual-side IRQ Clear       -- Base + 04h
     *    Virtual-side IRQ Set Mask    -- Base + 08h
     *    Virtual-side IRQ Clear Mask  -- Base + 0Ch
     *    Link-side IRQ Set            -- Base + 10h
     *    Link-side IRQ Clear          -- Base + 14h
     *    Link-side IRQ Set Mask       -- Base + 18h
     *    Link-side IRQ Clear Mask     -- Base + 1Ch
     *
     ****************************************************************/

    // Set NT Virtual-side link error interrupt offsets
    pdx->Offset_LE_IntMask   = pdx->Offset_RegBase + 0xFE4;
    pdx->Offset_LE_IntStatus = pdx->Offset_RegBase + 0xFE0;

    // Add offset based on chip type
    switch (pdx->Key.PlxChip & 0xFF00)
    {
        case 0x8500:
            OffsetIrqBase = pdx->Offset_RegBase + 0x90;
            break;

        case 0x8600:
        case 0x8700:
        case 0x9700:
            OffsetIrqBase = pdx->Offset_RegBase + 0xC4C;
            break;

        case 0:
        default:
            ErrorPrintf(("Error: Invalid PLX chip, unable to determine IRQ register offsets\n"));
            return FALSE;
    }

    // If link side, skip over Virtual-side registers
    if (pdx->Key.PlxPortType == PLX_SPEC_PORT_NT_LINK)
    {
        OffsetIrqBase += 0x10;
    }

    // Set final offsets
    pdx->Offset_DB_IntStatus    = OffsetIrqBase + 0x00;
    pdx->Offset_DB_IntClear     = OffsetIrqBase + 0x04;
    pdx->Offset_DB_IntMaskSet   = OffsetIrqBase + 0x08;
    pdx->Offset_DB_IntMaskClear = OffsetIrqBase + 0x0C;

    return TRUE;
}




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
    // Enable NT Virtual Link-side Error status interrupts
    if (((pdx->Key.PlxChip & 0xFF00) != 0x8500) &&
         (pdx->Key.PlxPortType == PLX_SPEC_PORT_NT_VIRTUAL))
    {
        PLX_8000_REG_WRITE(
            pdx,
            pdx->Offset_LE_IntMask,
            0x0
            );
    }

    // Enable doorbell interrupts
    PLX_8000_REG_WRITE(
        pdx,
        pdx->Offset_DB_IntMaskClear,
        0xFFFF
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
    // Disable NT Virtual Link-side Error status interrupts
    if (((pdx->Key.PlxChip & 0xFF00) != 0x8500) &&
         (pdx->Key.PlxPortType == PLX_SPEC_PORT_NT_VIRTUAL))
    {
        PLX_8000_REG_WRITE(
            pdx,
            pdx->Offset_LE_IntMask,
            0xF
            );
    }

    // Disable doorbell interrupts
    PLX_8000_REG_WRITE(
        pdx,
        pdx->Offset_DB_IntMaskSet,
        0xFFFF
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




/*******************************************************************************
 *
 * Function   :  PlxDetermineNtMode
 *
 * Description:  Detemines NT chip mode (legacy vs DS P2P)
 *
 ******************************************************************************/
VOID
PlxDetermineNtMode(
    DEVICE_EXTENSION *pdx
    )
{
    U32 RegValue;


    // Set device mode
    pdx->Key.DeviceMode = PLX_CHIP_MODE_STD_NT_DS_P2P;
    switch (pdx->Key.PlxFamily)
    {
        case PLX_FAMILY_VEGA:
        case PLX_FAMILY_VEGA_LITE:
        case PLX_FAMILY_DENEB:
            pdx->Key.DeviceMode = PLX_CHIP_MODE_STD_LEGACY_NT;
            break;

        case PLX_FAMILY_SIRIUS:
            // Verify NT P2P mode (1DC[6])
            RegValue = PLX_8000_REG_READ( pdx, (pdx->Offset_RegBase + 0x1DC) );
            if ((RegValue & (1 << 6)) == 0)
            {
                pdx->Key.DeviceMode = PLX_CHIP_MODE_STD_LEGACY_NT;
            }
            break;

        case PLX_FAMILY_CYGNUS:
            // Verify NT P2P mode (360[14]=1)
            RegValue = PLX_8000_REG_READ( pdx, (pdx->Offset_RegBase + 0x360) );
            if ((RegValue & (1 << 14)) == 0)
            {
                pdx->Key.DeviceMode = PLX_CHIP_MODE_STD_LEGACY_NT;
            }
            break;
    }

    DebugPrintf((
        "Device in NT %s mode\n",
        (pdx->Key.DeviceMode == PLX_CHIP_MODE_STD_LEGACY_NT) ? "LEGACY" :
            (pdx->Key.DeviceMode == PLX_CHIP_MODE_STD_NT_DS_P2P) ? "STANDARD P2P" :
            "UNKNOWN"
        ));
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
    U32     RegPci;
    U32     RegSave;
    U32     RegExpected;
    BOOLEAN bTempMap;


    bTempMap = FALSE;

    // Default to unknown side
    pdx->Key.PlxPortType = PLX_SPEC_PORT_UNKNOWN;

    // Check if BAR 0 exists
    if (pci_resource_start( pdx->pPciDevice, 0 ) == 0)
    {
        // If BAR 0 not enabled, this is 8500 virtual-side
        pdx->Key.PlxPortType = PLX_SPEC_PORT_NT_VIRTUAL;
        pdx->Offset_RegBase = 0x10000;
    }
    else
    {
        if (pdx->pRegVa == NULL)
        {
            DebugPrintf(("Map BAR 0 temporarily to determine NT side\n"));

            // Map BAR 0
            pdx->pRegVa =
                ioremap(
                    pci_resource_start( pdx->pPciDevice, 0 ),
                    pci_resource_len( pdx->pPciDevice, 0 )
                    );

            if (pdx->pRegVa == NULL)
            {
                DebugPrintf(("Error: Unable to map BAR 0 for NT determination\n"));
                return FALSE;
            }

            // Flag temporary mapping
            bTempMap = TRUE;
        }

        // Set NT base offset
        switch (pdx->Key.PlxFamily)
        {
            case PLX_FAMILY_SCOUT:
            case PLX_FAMILY_DRACO_2:
            case PLX_FAMILY_CAPELLA_1:
            case PLX_FAMILY_CAPELLA_2:
                // Read NT ID register
                PLX_PCI_REG_READ( pdx, 0xC8C, &RegPci );

                // Check which NT port
                if (RegPci & (1 << 0))
                {
                    pdx->Offset_RegBase = 0x3C000;   // NT 1
                }
                else
                {
                    pdx->Offset_RegBase = 0x3E000;   // NT 0
                }

                // Determine NT Virtual or Link
                if (RegPci & (1 << 31))
                {
                    pdx->Key.PlxPortType = PLX_SPEC_PORT_NT_LINK;
                }
                else
                {
                    pdx->Key.PlxPortType = PLX_SPEC_PORT_NT_VIRTUAL;
                }
                break;

            case PLX_FAMILY_DRACO_1:
                // NT ID register not implemented, revert to probe algorithm
                pdx->Offset_RegBase = 0x3E000;
                break;

            case PLX_FAMILY_CYGNUS:
                pdx->Offset_RegBase = 0x3E000;
                break;

            default:
                if (((pdx->Key.PlxChip & 0xFF00) == 0x8500) ||
                    ((pdx->Key.PlxChip & 0xFF00) == 0x8600))
                {
                    pdx->Offset_RegBase = 0x10000;
                }
                else
                {
                    ErrorPrintf(("ERROR: NT detection not implemented for %04X\n", pdx->Key.PlxChip));
                    return FALSE;
                }
                break;
        }

        /**********************************************************
         * For non-NT reporting PLX chips, the following algorithm
         * attempts to detect the NT port type.
         *
         * 1. Write 1FEh to 3Ch thru BAR 0 to NTV port
         * 2. Perform PCI read of 3Ch of device
         * 3. If values match, then NTV port, otherwise, NTL
         *
         * NOTE: Some OSes have been determined to block reads & writes
         * of register 3Ch.  So, if the register is updated internally,
         * the OS won't actually perform a PCI configuration read to
         * get the updated value.  As a result, the PLX driver bypasses
         * the OS in this case to ensure accurate reading of PCI 3Ch.
         *********************************************************/
        if (pdx->Key.PlxPortType == PLX_SPEC_PORT_UNKNOWN)
        {
            // Default to NT virtual side
            pdx->Key.PlxPortType = PLX_SPEC_PORT_NT_VIRTUAL;

            // Store original value
            RegSave = PLX_8000_REG_READ( pdx, (pdx->Offset_RegBase + PCI_REG_INT_PIN_LN) );

            // Replace lower byte with FEh
            RegExpected = (RegSave & ~((U32)0xFF)) | 0xFE;

            // Write expected value
            PLX_8000_REG_WRITE( pdx, (pdx->Offset_RegBase + PCI_REG_INT_PIN_LN), RegExpected );

            // Some chips have an internal latency when updating a
            //  mem-mapped register to PCI config space. Some dummy
            //  register reads are used to account for the latency.
            PLX_8000_REG_READ( pdx, (pdx->Offset_RegBase + PCI_REG_INT_PIN_LN) );
            PLX_8000_REG_READ( pdx, (pdx->Offset_RegBase + PCI_REG_INT_PIN_LN) );

            // Read register through PCI config cycle (bypassing OS)
            PlxPciRegisterRead_BypassOS(
                pdx->Key.domain,
                pdx->Key.bus,
                pdx->Key.slot,
                pdx->Key.function,
                PCI_REG_INT_PIN_LN,
                &RegPci
                );

            // Restore original value
            PLX_8000_REG_WRITE( pdx, (pdx->Offset_RegBase + PCI_REG_INT_PIN_LN), RegSave );

            // If PCI register does not match expected value, port is link side
            if (RegPci != RegExpected)
            {
                pdx->Key.PlxPortType = PLX_SPEC_PORT_NT_LINK;
            }
        }
    }

    // Release temporary mapping of BAR 0 & reset values
    if (bTempMap)
    {
        iounmap( pdx->pRegVa );
        pdx->pRegVa = NULL;
    }

    // Adjust offset for NT Link port
    if (pdx->Key.PlxPortType == PLX_SPEC_PORT_NT_LINK)
    {
        pdx->Offset_RegBase += 0x1000;
    }

    DebugPrintf((
        "NT port is %s-side (NT base=%Xh)\n",
        (pdx->Key.PlxPortType == PLX_SPEC_PORT_NT_LINK) ? "LINK" :
            (pdx->Key.PlxPortType == PLX_SPEC_PORT_NT_VIRTUAL) ? "VIRTUAL" :
            "UNKNOWN",
        pdx->Offset_RegBase
        ));

    if (pdx->Key.PlxPortType == PLX_SPEC_PORT_UNKNOWN)
    {
        DebugPrintf(("Error: Unable to determine NT side\n"));
        return FALSE;
    }

    return TRUE;
}




/******************************************************************************
 *
 * Function   :  PlxMapRegisterBar
 *
 * Description:  Maps the correct BAR space for register accesses
 *
 ******************************************************************************/
BOOLEAN
PlxMapRegisterBar(
    DEVICE_EXTENSION *pdx
    )
{
    U64             PciBar;
    U32             BarSize;
    struct pci_dev *pPciDevice;


    if (pdx->PciBar[0].pVa != NULL)
    {
        // Use PCI BAR 0 kernel address
        pdx->pRegVa = pdx->PciBar[0].pVa;
    }
    else
    {
        // Locate Upstream port
        pPciDevice =
            Plx_pci_get_domain_bus_and_slot(
                pdx->Key.domain,
                pdx->Key.bus - 1,
                PCI_DEVFN( 0, 0 )
                );
        if (pPciDevice == NULL)
        {
            DebugPrintf((
                "ERROR - Unable to locate upstream port at [%02X:00.0]\n",
                pdx->Key.bus - 1
                ));
            return FALSE;
        }

        // Make sure upstream port is enabled
        if (pci_enable_device( pPciDevice ) != 0)
        {
            ErrorPrintf(("WARNING - Upstream port enable failed\n"));
        }

        // Get PCI BAR 0 of upstream port
        PciBar = pci_resource_start( pPciDevice, 0 );
        if (PciBar == 0)
        {
            return FALSE;
        }

        // Get BAR size
        BarSize = pci_resource_len( pPciDevice, 0 );

        // Decrement reference count to device
        pci_dev_put( pPciDevice );

        // Save BAR size
        pdx->UpstreamBarSize = BarSize;

        // Map the space
        pdx->pRegVa = ioremap( PciBar, BarSize );
        if (pdx->pRegVa == NULL)
        {
            return FALSE;
        }

        DebugPrintf((
            "Mapped upstream BAR 0 (%08lX) ==> PLX regs (VA=%p %dKB)\n",
            (PLX_UINT_PTR)PciBar, pdx->pRegVa, (BarSize >> 10)
            ));
    }

    return TRUE;
}




/*******************************************************************************
 *
 * Function   :  PlxErrataWorkAround_NtBarShadow
 *
 * Description:  Implements work-around for NT BAR shadow errata
 *
 ******************************************************************************/
VOID
PlxErrataWorkAround_NtBarShadow(
    DEVICE_EXTENSION *pdx
    )
{
    U8  i;
    U16 offset;
    U32 RegValue;


    /*****************************************************
     * This work-around handles the errata for NT Virtual
     * side BAR registers.  For BARs 2-5, the BAR space
     * is not accessible until the shadow registers are
     * updated manually.
     *
     * The procedure is to first read the BAR Setup registers
     * and write them back to themselves.  Then the PCI BAR
     * register is read and the same value is written back.
     * This updates the internal shadow registers of the PLX
     * chip and makes the BAR space accessible for NT data
     * transfers.
     *
     * Note: The BAR setup register must be written before
     *       the PCI BAR register.
     ****************************************************/

    // Erratum only applies to 8500 series devices
    if ((pdx->Key.PlxChip & 0xFF00) != 0x8500)
    {
        return;
    }

    // If link side, not errata so return
    if (pdx->Key.PlxPortType == PLX_SPEC_PORT_NT_LINK)
    {
        return;
    }

    DebugPrintf(("Implementing NT Virtual-side BAR shadow errata work-around...\n"));

    // Read & write-back BAR setup and BAR registers for BARs 2-5
    for (i = 2; i <= 5; i++)
    {
        // Set offset for NT BAR setup register (D4h = Virtual-side BAR 2 Setup)
        offset = 0xD4 + ((i-2) * sizeof(U32));

        // Get Virtual-side BAR setup register
        PLX_PCI_REG_READ(
            pdx,
            offset,
            &RegValue
            );

        // Check if BAR is enabled
        if (RegValue & (1 << 31))
        {
            // Write BAR Setup back to itself
            PLX_PCI_REG_WRITE(
                pdx,
                offset,
                RegValue
                );

            // Read the corresponding PCI BAR
            PLX_PCI_REG_READ(
                pdx,
                PCI_REG_BAR_0 + (i * sizeof(U32)),
                &RegValue
                );

            // Write PCI BAR back to itself
            PLX_PCI_REG_WRITE(
                pdx,
                PCI_REG_BAR_0 + (i * sizeof(U32)),
                RegValue
                );
        }
    }
}




/*******************************************************************************
 *
 * Function   :  PlxErrataWorkAround_NtCapturedRequesterID
 *
 * Description:  Implements work-around for NT Link-side captured requester ID
 *
 ******************************************************************************/
VOID
PlxErrataWorkAround_NtCapturedRequesterID(
    DEVICE_EXTENSION *pdx
    )
{
    U8            NonNtPort;
    PLX_PORT_PROP PortProp;


    // Erratum only applies to 8500 series devices
    if ((pdx->Key.PlxChip & 0xFF00) != 0x8500)
    {
        return;
    }

    // If virtual side, not errata so return
    if (pdx->Key.PlxPortType == PLX_SPEC_PORT_NT_VIRTUAL)
    {
        return;
    }

    DebugPrintf(("Implementing NT Link-side captured Requester ID work-around...\n"));

    // Get the port number to determine which Non-NT port to update
    PlxGetPortProperties( pdx, &PortProp );

    if ((U16)PortProp.PortNumber <= 3)
    {
        NonNtPort = 8;
    }
    else
    {
        NonNtPort = 0;
    }

    // Update captured bus/device number
    PlxRegisterWrite(
        pdx,
        (NonNtPort * (0x1000)) + 0xDF4,
        ((pdx->Key.bus << 0) & 0xFF) | ((pdx->Key.slot << 8) & 0x1F),
        FALSE          // Don't adjust offset for port
        );

    DebugPrintf((
        "Updated Non-NT station (port %d) with captured ReqID [%02x:%02x.0]\n",
        NonNtPort, pdx->Key.bus, pdx->Key.slot
        ));
}
