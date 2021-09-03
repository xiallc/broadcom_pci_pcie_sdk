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

/*******************************************************************************
 *
 * File Name:
 *
 *      ApiFunc.c
 *
 * Description:
 *
 *      Implements the PLX API functions
 *
 * Revision History:
 *
 *      11-01-18 : PLX SDK v8.00
 *
 ******************************************************************************/


#include <linux/slab.h>     // For kmalloc()
#include <linux/uaccess.h>  // For copy_to/from_user()
#include "ApiFunc.h"
#include "ChipFunc.h"
#include "Eep_6000.h"
#include "Eep_8000.h"
#include "Eep_8111.h"
#include "PciFunc.h"
#include "PciRegs.h"
#include "SuppFunc.h"




/*******************************************************************************
 *
 * Function   :  PlxDeviceFind
 *
 * Description:  Search for a specific device using device key parameters
 *
 ******************************************************************************/
PLX_STATUS
PlxDeviceFind(
    DEVICE_EXTENSION *pdx,
    PLX_DEVICE_KEY   *pKey,
    U16              *pDeviceNumber
    )
{
    U16               DeviceCount;
    BOOLEAN           bMatchId;
    BOOLEAN           bMatchLoc;
    PLX_DEVICE_NODE  *pDevice;
    struct list_head *pEntry;


    DeviceCount = 0;

    pEntry = pdx->List_Devices.next;

    // Traverse list to find the desired list objects
    while (pEntry != &(pdx->List_Devices))
    {
        // Get the object
        pDevice =
            list_entry(
                pEntry,
                PLX_DEVICE_NODE,
                ListEntry
                );

        // Assume successful match
        bMatchLoc = TRUE;
        bMatchId  = TRUE;

        //
        // Compare device key information
        //

        // Compare Bus, Slot, Fn numbers
        if ( (pKey->bus      != (U8)PCI_FIELD_IGNORE) ||
             (pKey->slot     != (U8)PCI_FIELD_IGNORE) ||
             (pKey->function != (U8)PCI_FIELD_IGNORE) )
        {
            if ( (pKey->bus      != pDevice->Key.bus)  ||
                 (pKey->slot     != pDevice->Key.slot) ||
                 (pKey->function != pDevice->Key.function) )
            {
                bMatchLoc = FALSE;
            }
        }

        //
        // Compare device ID information
        //

        // Compare Vendor ID
        if (pKey->VendorId != (U16)PCI_FIELD_IGNORE)
        {
            if (pKey->VendorId != pDevice->Key.VendorId)
            {
                bMatchId = FALSE;
            }
        }

        // Compare Device ID
        if (pKey->DeviceId != (U16)PCI_FIELD_IGNORE)
        {
            if (pKey->DeviceId != pDevice->Key.DeviceId)
            {
                bMatchId = FALSE;
            }
        }

        // Subsystem ID
        if (pDevice->Key.SubVendorId != 0)
        {
            // Compare Subsystem Vendor ID
            if (pKey->SubVendorId != (U16)PCI_FIELD_IGNORE)
            {
                if (pKey->SubVendorId != pDevice->Key.SubVendorId)
                {
                    bMatchId = FALSE;
                }
            }

            // Compare Subsystem Device ID
            if (pKey->SubDeviceId != (U16)PCI_FIELD_IGNORE)
            {
                if (pKey->SubDeviceId != pDevice->Key.SubDeviceId)
                {
                    bMatchId = FALSE;
                }
            }
        }

        // Compare Revision
        if (pKey->Revision != (U8)PCI_FIELD_IGNORE)
        {
            if (pKey->Revision != pDevice->Key.Revision)
            {
                bMatchId = FALSE;
            }
        }

        // Check if match on location and ID
        if (bMatchLoc && bMatchId)
        {
            // Match found, check if it is the desired device
            if (DeviceCount == *pDeviceNumber)
            {
                // Copy the device information
                *pKey = pDevice->Key;

                DebugPrintf((
                    "Criteria matched device %04X_%04X [%02x:%02x.%x]\n",
                    pDevice->Key.DeviceId, pDevice->Key.VendorId,
                    pDevice->Key.bus, pDevice->Key.slot, pDevice->Key.function
                    ));
                return PLX_STATUS_OK;
            }

            // Increment device count
            DeviceCount++;
        }

        // Jump to next entry
        pEntry = pEntry->next;
    }

    // Return number of matched devices
    *pDeviceNumber = DeviceCount;

    DebugPrintf(("Criteria did not match any devices\n"));

    return PLX_STATUS_INVALID_OBJECT;
}




/*******************************************************************************
 *
 * Function   :  PlxChipTypeGet
 *
 * Description:  Returns PLX chip type and revision
 *
 ******************************************************************************/
PLX_STATUS
PlxChipTypeGet(
    PLX_DEVICE_NODE *pdx,
    U16             *pChipType,
    U8              *pRevision
    )
{
    *pChipType = pdx->Key.PlxChip;
    *pRevision = pdx->Key.PlxRevision;

    DebugPrintf((
        "Device %04X_%04X = %04X rev %02X\n",
        pdx->Key.DeviceId, pdx->Key.VendorId,
        *pChipType, *pRevision
        ));

    return PLX_STATUS_OK;
}




/*******************************************************************************
 *
 * Function   :  PlxChipTypeSet
 *
 * Description:  Sets the PLX chip type dynamically
 *
 ******************************************************************************/
PLX_STATUS
PlxChipTypeSet(
    PLX_DEVICE_NODE *pdx,
    U16              ChipType,
    U8              *pRevision,
    U8              *pFamily
    )
{
    U16     TempChip;
    BOOLEAN bSetUptreamNode;


    // Default to non-upstream node
    bSetUptreamNode = FALSE;

    // Default family to current value
    *pFamily = pdx->Key.PlxFamily;

    // Attempt auto-detection if requested
    if (ChipType == 0)
    {
        PlxChipTypeDetect( pdx, FALSE );
        ChipType = pdx->Key.PlxChip;
    }

    // Generalize by device type
    switch (ChipType & 0xFF00)
    {
        case 0x2300:
        case 0x3300:
        case 0x8500:
        case 0x8600:
        case 0x8700:
        case 0x9700:
            TempChip = 0x8000;
            break;

        default:
            TempChip = ChipType;
            break;
    }

    // Verify chip type
    switch (TempChip)
    {
        case 0:         // Used to clear chip type
        case 0x6140:
        case 0x6150:
        case 0x6152:
        case 0x6154:
        case 0x6156:
        case 0x6254:
        case 0x6350:
        case 0x6520:
        case 0x6540:
        case 0x8111:
        case 0x8112:
            break;

        case 0x8114:
            // Flag to update upstream node
            bSetUptreamNode = TRUE;
            break;

        case 0x8000:
            // Ensure upstream node
            if (pdx->PortProp.PortType != PLX_PORT_UPSTREAM)
            {
                DebugPrintf(("ERROR - Chip type may only be changed on upstream port\n"));
                return PLX_STATUS_UNSUPPORTED;
            }

            // Flag to update upstream node
            bSetUptreamNode = TRUE;
            break;

        default:
            DebugPrintf(("ERROR - Invalid or unsupported chip type (%04X)\n", ChipType));
            return PLX_STATUS_INVALID_DATA;
    }

    // Set the new chip type
    pdx->Key.PlxChip = ChipType;

    // Check if we should update the revision or use the default
    if ((*pRevision == (U8)-1) || (*pRevision == 0))
    {
        // Attempt to detect Revision ID
        PlxChipRevisionDetect( pdx );
    }
    else
    {
        pdx->Key.PlxRevision = *pRevision;
    }

    // Update the PLX family
    PlxChipTypeDetect(
        pdx,
        TRUE            // Only update PLX family
        );

    // Provide the updated family
    *pFamily = pdx->Key.PlxFamily;

    if (bSetUptreamNode)
    {
        DebugPrintf(("Set register access node to itself\n"));
        pdx->pRegNode = pdx;
    }
    else
    {
        pdx->pRegNode = NULL;
    }

    DebugPrintf((
        "Set device (%04X_%04X) type to %04X rev %02X\n",
        pdx->Key.DeviceId, pdx->Key.VendorId,
        pdx->Key.PlxChip, pdx->Key.PlxRevision
        ));

    return PLX_STATUS_OK;
}




/*******************************************************************************
 *
 * Function   :  PlxGetPortProperties
 *
 * Description:  Returns the current port information and status
 *
 ******************************************************************************/
PLX_STATUS
PlxGetPortProperties(
    PLX_DEVICE_NODE *pdx,
    PLX_PORT_PROP   *pPortProp
    )
{
    U16 MaxSize;
    U16 Offset_PcieCap;
    U32 RegValue;


    // Set default properties
    RtlZeroMemory(pPortProp, sizeof(PLX_PORT_PROP));

    // Get the offset of the PCI Express capability
    Offset_PcieCap =
        PlxPciFindCapability(
            pdx,
            PCI_CAP_ID_PCI_EXPRESS,
            FALSE,
            0
            );
    if (Offset_PcieCap == 0)
    {
        DebugPrintf((
            "[D%d %02X:%02X.%X] Does not contain PCIe capability\n",
            pdx->Key.domain, pdx->Key.bus, pdx->Key.slot, pdx->Key.function
            ));

        // Mark device as non-PCIe
        pPortProp->bNonPcieDevice = TRUE;

        // Default to a legacy endpoint
        if (pdx->PciHeaderType == PCI_HDR_TYPE_0)
        {
            pPortProp->PortType = PLX_PORT_LEGACY_ENDPOINT;
        }
        else
        {
            pPortProp->PortType = PLX_PORT_UNKNOWN;
        }
        return PLX_STATUS_OK;
    }

    // Get PCIe Capability
    PLX_PCI_REG_READ(
        pdx,
        Offset_PcieCap,
        &RegValue
        );

    // Get port type
    pPortProp->PortType = (U8)((RegValue >> 20) & 0xF);

    // Get PCIe Device Capabilities
    PLX_PCI_REG_READ(
        pdx,
        Offset_PcieCap + 0x04,
        &RegValue
        );

    // Get max payload size supported field
    MaxSize = (U8)(RegValue >> 0) & 0x7;

    // Set max payload size (=128 * (2 ^ MaxPaySizeField))
    pPortProp->MaxPayloadSupported = PCIE_MPS_MRR_TO_BYTES( MaxSize );

    // Get PCIe Device Control
    PLX_PCI_REG_READ(
        pdx,
        Offset_PcieCap + 0x08,
        &RegValue
        );

    // Get max payload size field
    MaxSize = (U8)(RegValue >> 5) & 0x7;

    // Set max payload size (=128 * (2 ^ MaxPaySizeField))
    pPortProp->MaxPayloadSize = PCIE_MPS_MRR_TO_BYTES( MaxSize );

    // Get max read request size field
    MaxSize = (U8)(RegValue >> 12) & 0x7;

    // Set max read request size (=128 * (2 ^ MaxReadReqSizeField))
    pPortProp->MaxReadReqSize = PCIE_MPS_MRR_TO_BYTES( MaxSize );

    // Get PCIe Link Capabilities
    PLX_PCI_REG_READ(
        pdx,
        Offset_PcieCap + 0x0C,
        &RegValue
        );

    // Get port number
    pPortProp->PortNumber = (U8)((RegValue >> 24) & 0xFF);

    // Get max link width
    pPortProp->MaxLinkWidth = (U8)((RegValue >> 4) & 0x3F);

    // Get max link speed
    pPortProp->MaxLinkSpeed = (U8)((RegValue >> 0) & 0xF);

    // Get PCIe Link Status/Control
    PLX_PCI_REG_READ(
        pdx,
        Offset_PcieCap + 0x10,
        &RegValue
        );

    // Get link width
    pPortProp->LinkWidth = (U8)((RegValue >> 20) & 0x3F);

    // Get link speed
    pPortProp->LinkSpeed = (U8)((RegValue >> 16) & 0xF);

    /**********************************************************
     * In MIRA 3300 Enhanced mode, link width for the DS port & USB EP
     * is incorrectly reported as x0. Override with Max Width.
     *********************************************************/
    if ((pdx->Key.PlxFamily == PLX_FAMILY_MIRA) &&
        ((pdx->Key.PlxChip & 0xFF00) == 0x3300) &&
        (pdx->Key.DeviceMode == PLX_PORT_ENDPOINT) &&
        (pPortProp->LinkWidth == 0))
    {
        DebugPrintf((
            "NOTE - Override reported link width (x%d) with Max width (x%d)\n",
            pPortProp->LinkWidth, pPortProp->MaxLinkWidth
            ));
        pPortProp->LinkWidth = pPortProp->MaxLinkWidth;
    }

    /**********************************************************
     * If using port bifurication, Draco 2 DS ports may report
     * incorrect port numbers. Override with slot number.
     *********************************************************/
    if ((pdx->Key.PlxFamily == PLX_FAMILY_DRACO_2) &&
        (pPortProp->PortType == PLX_PORT_DOWNSTREAM) &&
        (pPortProp->PortNumber != pdx->Key.slot))
    {
        DebugPrintf((
            "NOTE - Override reported port num (%d) with slot num (%d)\n",
            pPortProp->PortNumber, pdx->Key.slot
            ));
        pPortProp->PortNumber = pdx->Key.slot;
    }

    DebugPrintf((
        "[D%d %02X:%02X.%X] P=%d T=%d MPS=%d/%d MRR=%d L=G%dx%d/G%dx%d\n",
        pdx->Key.domain, pdx->Key.bus, pdx->Key.slot, pdx->Key.function,
        pPortProp->PortNumber, pPortProp->PortType,
        pPortProp->MaxPayloadSize, pPortProp->MaxPayloadSupported,
        pPortProp->MaxReadReqSize,
        pPortProp->LinkSpeed, pPortProp->LinkWidth,
        pPortProp->MaxLinkSpeed, pPortProp->MaxLinkWidth
        ));

    return PLX_STATUS_OK;
}




/*******************************************************************************
 *
 * Function   :  PlxPciDeviceReset
 *
 * Description:  Resets a device using software reset feature of PLX chip
 *
 ******************************************************************************/
PLX_STATUS
PlxPciDeviceReset(
    PLX_DEVICE_NODE *pdx
    )
{
    // Device reset not implemented
    return PLX_STATUS_UNSUPPORTED;
}




/*******************************************************************************
 *
 * Function   :  PlxRegisterRead
 *
 * Description:  Reads a PLX-specific register
 *
 ******************************************************************************/
U32
PlxRegisterRead(
    PLX_DEVICE_NODE *pdx,
    U32              offset,
    PLX_STATUS      *pStatus,
    BOOLEAN          bAdjustForPort
    )
{
    U16 TempChip;


    // Generalize by device type
    switch (pdx->Key.PlxChip & 0xFF00)
    {
        case 0x2300:
        case 0x3300:
        case 0x8500:
        case 0x8600:
        case 0x8700:
        case 0x9700:
        case 0xC000:
            TempChip = 0x8000;
            break;

        default:
            TempChip = pdx->Key.PlxChip;
            break;
    }

    // Verify device is supported
    switch (TempChip)
    {
        case 0x8111:
        case 0x8112:
            return PlxRegisterRead_8111(
                pdx,
                offset,
                pStatus
                );
            break;

        case 0x8114:
        case 0x8000:
            return PlxRegisterRead_8000(
                pdx,
                offset,
                pStatus,
                bAdjustForPort
                );
    }

    if (pStatus != NULL)
    {
        *pStatus = PLX_STATUS_UNSUPPORTED;
    }

    return 0;
}




/*******************************************************************************
 *
 * Function   :  PlxRegisterWrite
 *
 * Description:  Writes to a PLX-specific register
 *
 ******************************************************************************/
PLX_STATUS
PlxRegisterWrite(
    PLX_DEVICE_NODE *pdx,
    U32              offset,
    U32              value,
    BOOLEAN          bAdjustForPort
    )
{
    U16 TempChip;


    // Generalize by device type
    switch (pdx->Key.PlxChip & 0xFF00)
    {
        case 0x2300:
        case 0x3300:
        case 0x8500:
        case 0x8600:
        case 0x8700:
        case 0x9700:
        case 0xC000:
            TempChip = 0x8000;
            break;

        default:
            TempChip = pdx->Key.PlxChip;
            break;
    }

    // Verify device is supported
    switch (TempChip)
    {
        case 0x8111:
        case 0x8112:
            return PlxRegisterWrite_8111(
                pdx,
                offset,
                value
                );
            break;

        case 0x8114:
        case 0x8000:
            return PlxRegisterWrite_8000(
                pdx,
                offset,
                value,
                bAdjustForPort
                );
    }

    return PLX_STATUS_UNSUPPORTED;
}




/*******************************************************************************
 *
 * Function   :  PlxPciBarProperties
 *
 * Description:  Returns the properties of a PCI BAR space
 *
 ******************************************************************************/
PLX_STATUS
PlxPciBarProperties(
    PLX_DEVICE_NODE  *pdx,
    U8                BarIndex,
    PLX_PCI_BAR_PROP *pBarProperties
    )
{
    // Verify BAR index
    switch (pdx->PciHeaderType)
    {
        case PCI_HDR_TYPE_0:
            if ((BarIndex != 0) && (BarIndex > 5))
            {
                return PLX_STATUS_INVALID_ACCESS;
            }
            break;

        case PCI_HDR_TYPE_1:
            if ((BarIndex != 0) && (BarIndex != 1))
            {
                DebugPrintf(("BAR %d does not exist on PCI type 1 header\n", BarIndex));
                return PLX_STATUS_INVALID_ACCESS;
            }
            break;

        default:
            return PLX_STATUS_INVALID_ACCESS;
    }

    // Return BAR properties
    *pBarProperties = pdx->PciBar[BarIndex].Properties;

    // Do nothing if upper 32-bits of 64-bit BAR
    if (pdx->PciBar[BarIndex].Properties.Flags & PLX_BAR_FLAG_UPPER_32)
    {
        DebugPrintf(("BAR %d is upper address of 64-bit BAR %d\n", BarIndex, BarIndex-1));
        return PLX_STATUS_OK;
    }

    // Display BAR properties if enabled
    if (pdx->PciBar[BarIndex].Properties.BarValue == 0)
    {
        DebugPrintf(("BAR %d is disabled\n", BarIndex));
        return PLX_STATUS_OK;
    }

    DebugPrintf((
        "    Type     : %s\n",
        (pdx->PciBar[BarIndex].Properties.Flags & PLX_BAR_FLAG_IO) ? "I/O" : "Memory"
        ));

    DebugPrintf((
        "    PCI BAR %d: %08llX\n",
        BarIndex, pdx->PciBar[BarIndex].Properties.BarValue
        ));

    DebugPrintf((
        "    Phys Addr: %08llX\n",
        pdx->PciBar[BarIndex].Properties.Physical
        ));

    DebugPrintf((
        "    Size     : %llXh (%lld%s)\n",
        pdx->PciBar[BarIndex].Properties.Size,
        pdx->PciBar[BarIndex].Properties.Size > ((U64)1 << 30) ?
           (pdx->PciBar[BarIndex].Properties.Size >> 30) :
           pdx->PciBar[BarIndex].Properties.Size > ((U64)1 << 20) ?
           (pdx->PciBar[BarIndex].Properties.Size >> 20) :
           pdx->PciBar[BarIndex].Properties.Size > ((U64)1 << 10) ?
           (pdx->PciBar[BarIndex].Properties.Size >> 10) :
           pdx->PciBar[BarIndex].Properties.Size,
        pdx->PciBar[BarIndex].Properties.Size > ((U64)1 << 30) ? "GB" :
           pdx->PciBar[BarIndex].Properties.Size > ((U64)1 << 20) ? "MB" :
           pdx->PciBar[BarIndex].Properties.Size > ((U64)1 << 10) ? "KB" : "B"
        ));

    if (pdx->PciBar[BarIndex].Properties.Flags & PLX_BAR_FLAG_MEM)
    {
        DebugPrintf((
            "    Property : %sPrefetchable %d-bit\n",
            (pdx->PciBar[BarIndex].Properties.Flags & PLX_BAR_FLAG_PREFETCHABLE) ? "" : "Non-",
            (pdx->PciBar[BarIndex].Properties.Flags & PLX_BAR_FLAG_64_BIT) ? 64 : 32
            ));
    }

    return PLX_STATUS_OK;
}




/*******************************************************************************
 *
 * Function   :  PlxPciBarMap
 *
 * Description:  Map a PCI BAR Space into User virtual space
 *
 ******************************************************************************/
PLX_STATUS
PlxPciBarMap(
    PLX_DEVICE_NODE *pdx,
    U8               BarIndex,
    VOID            *pUserVa
    )
{
    PLX_USER_MAPPING *pObject;


    /*************************************************************
     * In the service driver, this function simply adds an entry
     * to a list with additional information needed when the
     * API calls mmap and Dispatch_mmap executes.
     ************************************************************/

    pObject =
        kmalloc(
            sizeof(PLX_USER_MAPPING),
            GFP_KERNEL
            );

    if (pObject == NULL)
    {
        DebugPrintf(("ERROR - memory allocation for map object failed\n"));
        return PLX_STATUS_INSUFFICIENT_RES;
    }

    // Save the intended device node
    pObject->pDevice = pdx;

    // Record the desired BAR index
    pObject->BarIndex = BarIndex;

    // Increment request counter
    pdx->MapRequestPending++;

    // Add to list of map objects
    spin_lock(
        &(pdx->pdo->Lock_MapParamsList)
        );

    list_add_tail(
        &(pObject->ListEntry),
        &(pdx->pdo->List_MapParams)
        );

    spin_unlock(
        &(pdx->pdo->Lock_MapParamsList)
        );

    DebugPrintf((
        "Added map object (%p) to list (node=%p)\n",
        pObject, pdx
        ));

    return PLX_STATUS_OK;
}




/*******************************************************************************
 *
 * Function   :  PlxPciBarUnmap
 *
 * Description:  Unmap a previously mapped PCI BAR Space from User virtual space
 *
 ******************************************************************************/
PLX_STATUS
PlxPciBarUnmap(
    PLX_DEVICE_NODE *pdx,
    VOID            *UserVa
    )
{
    // Handled at user API level in Linux
    return PLX_STATUS_UNSUPPORTED;
}




/*******************************************************************************
 *
 * Function   :  PlxEepromPresent
 *
 * Description:  Returns the state of the EEPROM as reported by the PLX device
 *
 ******************************************************************************/
PLX_STATUS
PlxEepromPresent(
    PLX_DEVICE_NODE *pdx,
    U8              *pStatus
    )
{
    U16     TempChip;
    BOOLEAN bFlag;


    // Default to no EEPROM present
    *pStatus = PLX_EEPROM_STATUS_NONE;

    // Generalize by device type
    switch (pdx->Key.PlxChip & 0xFF00)
    {
        case 0x2300:
        case 0x3300:
        case 0x8500:
        case 0x8600:
        case 0x8700:
        case 0x9700:
            TempChip = 0x8000;
            break;

        default:
            TempChip = pdx->Key.PlxChip;
            break;
    }

    switch (TempChip)
    {
        case 0x6150:
        case 0x6152:
        case 0x6154:
        case 0x6156:
        case 0x6254:
        case 0x6350:
        case 0x6520:
        case 0x6540:
            // 6000 series doesn't report EEPROM presence, so probe for it
            PlxEepromProbe(
                pdx,
                &bFlag
                );

            if (bFlag)
            {
                *pStatus = PLX_EEPROM_STATUS_VALID;
            }
            else
            {
                *pStatus = PLX_EEPROM_STATUS_NONE;
            }
            return PLX_STATUS_OK;

        case 0x8111:
        case 0x8112:
            return Plx8111_EepromPresent(
                pdx,
                pStatus
                );

        case 0x8114:
        case 0x8000:
            return Plx8000_EepromPresent(
                pdx,
                pStatus
                );
    }

    return PLX_STATUS_UNSUPPORTED;
}




/*******************************************************************************
 *
 * Function   :  PlxEepromProbe
 *
 * Description:  Probes for the presence of an EEPROM
 *
 ******************************************************************************/
PLX_STATUS
PlxEepromProbe(
    PLX_DEVICE_NODE *pdx,
    U8              *pFlag
    )
{
    U16        TempChip;
    U16        OffsetProbe;
    U32        ValueRead;
    U32        ValueWrite;
    U32        ValueOriginal;
    PLX_STATUS status;


    // Default to no EEPROM present
    *pFlag = FALSE;

    // Generalize by device type
    switch (pdx->Key.PlxChip & 0xFF00)
    {
        case 0x2300:
        case 0x3300:
        case 0x8600:
        case 0x8700:
        case 0x9700:
            TempChip = pdx->Key.PlxChip & 0xFF00;
            break;

        default:
            TempChip = pdx->Key.PlxChip;
            break;
    }

    // Determine EEPROM offset to use for probe (e.g. after CRC)
    switch (TempChip)
    {
        case 0x6150:
        case 0x6152:
        case 0x6154:
        case 0x6156:
        case 0x6254:
        case 0x6350:
        case 0x6520:
        case 0x6540:
            OffsetProbe = 0x50;     // Use area outside of PLX portion
            break;

        case 0x8114:
            if (pdx->Key.PlxRevision >= 0xBA)
            {
                OffsetProbe = 0x3EC + sizeof(U32);
            }
            else
            {
                OffsetProbe = 0x378 + sizeof(U32);
            }
            break;

        case 0x8508:
        case 0x8512:
        case 0x8517:
        case 0x8518:
            OffsetProbe = (0x78F * sizeof(U32)) + sizeof(U32);
            break;

        case 0x8516:
        case 0x8524:
        case 0x8532:
            OffsetProbe = (0xBE4 * sizeof(U32)) + sizeof(U32);
            break;

        case 0x2300:
        case 0x3300:
        case 0x8111:
        case 0x8112:
        case 0x8505:
        case 0x8509:
        case 0x8525:
        case 0x8533:
        case 0x8547:
        case 0x8548:
        case 0x8600:
        case 0x8700:
        case 0x9700:
            OffsetProbe = 0x10;     // No CRC, just use any reasonable address
            break;

        case 0:
        default:
            DebugPrintf((
                "ERROR - Not a supported PLX device (%04X)\n",
                pdx->Key.PlxChip
                ));
            return PLX_STATUS_UNSUPPORTED;
    }

    DebugPrintf(("Probe EEPROM at offset %02xh\n", OffsetProbe));

    // Get the current value
    status =
        PlxEepromReadByOffset(
            pdx,
            OffsetProbe,
            &ValueOriginal
            );

    if (status != PLX_STATUS_OK)
    {
        return status;
    }

    // Prepare inverse value to write
    ValueWrite = ~(ValueOriginal);

    // Write inverse of original value
    status =
        PlxEepromWriteByOffset(
            pdx,
            OffsetProbe,
            ValueWrite
            );

    if (status != PLX_STATUS_OK)
    {
        return status;
    }

    // Read updated value
    status =
        PlxEepromReadByOffset(
            pdx,
            OffsetProbe,
            &ValueRead
            );

    if (status != PLX_STATUS_OK)
    {
        return status;
    }

    // Check if value was written properly
    if (ValueRead == ValueWrite)
    {
        DebugPrintf(("Probe detected an EEPROM present\n"));

        *pFlag = TRUE;

        // Restore the original value
        PlxEepromWriteByOffset(
            pdx,
            OffsetProbe,
            ValueOriginal
            );
    }
    else
    {
        DebugPrintf(("Probe did not detect an EEPROM\n"));
    }

    return PLX_STATUS_OK;
}




/*******************************************************************************
 *
 * Function   :  PlxEepromGetAddressWidth
 *
 * Description:  Returns the current EEPROM address width
 *
 ******************************************************************************/
PLX_STATUS
PlxEepromGetAddressWidth(
    PLX_DEVICE_NODE *pdx,
    U8              *pWidth
    )
{
    PLX_STATUS status;


    switch (pdx->Key.PlxChip & 0xFF00)
    {
        case 0x8100:
            status =
                Plx8111_EepromGetAddressWidth(
                    pdx,
                    pWidth
                    );
            break;

        case 0x2300:
        case 0x3300:
        case 0x8500:
        case 0x8600:
        case 0x8700:
        case 0x9700:
            status =
                Plx8000_EepromGetAddressWidth(
                    pdx,
                    pWidth
                    );
            break;

        default:
            DebugPrintf((
                "ERROR - Chip (%04X) does not support address width\n",
                pdx->Key.PlxChip
                ));
            return PLX_STATUS_UNSUPPORTED;
    }

    DebugPrintf(("EEPROM address width = %dB\n", *pWidth));
    return status;
}




/*******************************************************************************
 *
 * Function   :  PlxEepromSetAddressWidth
 *
 * Description:  Sets a new EEPROM address width
 *
 ******************************************************************************/
PLX_STATUS
PlxEepromSetAddressWidth(
    PLX_DEVICE_NODE *pdx,
    U8               width
    )
{
    PLX_STATUS status;


    // Verify the width
    switch (width)
    {
        case 1:
        case 2:
        case 3:
            break;

        default:
            DebugPrintf(("ERROR - Invalid address width (%d)\n", width));
            return PLX_STATUS_INVALID_DATA;
    }

    switch (pdx->Key.PlxChip & 0xFF00)
    {
        case 0x8100:
            status =
                Plx8111_EepromSetAddressWidth(
                    pdx,
                    width
                    );
            break;

        case 0x2300:
        case 0x3300:
        case 0x8600:
        case 0x8700:
        case 0x9700:
            status =
                Plx8000_EepromSetAddressWidth(
                    pdx,
                    width
                    );
            break;

        default:
            DebugPrintf((
                "ERROR - Chip (%04X) does not support address width override\n",
                pdx->Key.PlxChip
                ));
            return PLX_STATUS_UNSUPPORTED;
    }

    DebugPrintf((
       "%s EEPROM address width to %dB\n",
       (status == PLX_STATUS_OK) ? "Set" : "ERROR - Unable to set",
       width
       ));

    return status;
}




/*******************************************************************************
 *
 * Function   :  PlxEepromCrcGet
 *
 * Description:  Returns the EEPROM CRC and its status
 *
 ******************************************************************************/
PLX_STATUS
PlxEepromCrcGet(
    PLX_DEVICE_NODE *pdx,
    U32             *pCrc,
    U8              *pCrcStatus
    )
{
    // Clear return value
    *pCrc       = 0;
    *pCrcStatus = PLX_CRC_UNSUPPORTED;

    switch (pdx->Key.PlxChip & 0xFF00)
    {
        case 0x8500:
        case 0x8700:
        case 0x9700:
            return Plx8000_EepromCrcGet(
                pdx,
                pCrc,
                pCrcStatus
                );
    }

    // CRC not supported
    return PLX_STATUS_UNSUPPORTED;
}




/*******************************************************************************
 *
 * Function   :  PlxEepromCrcUpdate
 *
 * Description:  Updates the EEPROM CRC
 *
 ******************************************************************************/
PLX_STATUS
PlxEepromCrcUpdate(
    PLX_DEVICE_NODE *pdx,
    U32             *pCrc,
    BOOLEAN          bUpdateEeprom
    )
{
    // Clear return value
    *pCrc = 0;

    switch (pdx->Key.PlxChip & 0xFF00)
    {
        case 0x8500:
        case 0x8700:
        case 0x9700:
            return Plx8000_EepromCrcUpdate(
                pdx,
                pCrc,
                bUpdateEeprom
                );
    }

    // CRC not supported
    return PLX_STATUS_UNSUPPORTED;
}




/*******************************************************************************
 *
 * Function   :  PlxEepromReadByOffset
 *
 * Description:  Read a 32-bit value from the EEPROM at a specified offset
 *
 ******************************************************************************/
PLX_STATUS
PlxEepromReadByOffset(
    PLX_DEVICE_NODE *pdx,
    U32              offset,
    U32             *pValue
    )
{
    U16 TempChip;


    // Set default return value
    *pValue = 0;

    // Make sure offset is aligned on 32-bit boundary
    if (offset & (3 << 0))
    {
        return PLX_STATUS_INVALID_OFFSET;
    }

    // Generalize by device type
    switch (pdx->Key.PlxChip & 0xFF00)
    {
        case 0x2300:
        case 0x3300:
        case 0x8500:
        case 0x8600:
        case 0x8700:
        case 0x9700:
            TempChip = 0x8000;
            break;

        default:
            TempChip = pdx->Key.PlxChip;
            break;
    }

    switch (TempChip)
    {
        case 0x6150:
        case 0x6152:
        case 0x6154:
        case 0x6156:
        case 0x6254:
        case 0x6350:
        case 0x6520:
        case 0x6540:
        case 0x8111:
        case 0x8112:
            /******************************************
             * For devices that do not support 32-bit
             * EEPROM accesses, use 2 16-bit accesses
             *****************************************/
            PlxEepromReadByOffset_16(
                pdx,
                offset,
                (U16*)pValue
                );

            PlxEepromReadByOffset_16(
                pdx,
                offset + sizeof(U16),
                (U16*)((U8*)pValue + sizeof(U16))
                );
            break;

        case 0x8114:
        case 0x8000:
            return Plx8000_EepromReadByOffset(
                pdx,
                offset,
                pValue
                );

        case 0x6140:  // 6140 does not support an EEPROM
        default:
            return PLX_STATUS_UNSUPPORTED;
    }

    return PLX_STATUS_OK;
}




/*******************************************************************************
 *
 * Function   :  PlxEepromWriteByOffset
 *
 * Description:  Write a 32-bit value to the EEPROM at a specified offset
 *
 ******************************************************************************/
PLX_STATUS
PlxEepromWriteByOffset(
    PLX_DEVICE_NODE *pdx,
    U32              offset,
    U32              value
    )
{
    U16 TempChip;


    // Make sure offset is aligned on 32-bit boundary
    if (offset & (3 << 0))
    {
        return PLX_STATUS_INVALID_OFFSET;
    }

    // Generalize by device type
    switch (pdx->Key.PlxChip & 0xFF00)
    {
        case 0x2300:
        case 0x3300:
        case 0x8500:
        case 0x8600:
        case 0x8700:
        case 0x9700:
            TempChip = 0x8000;
            break;

        default:
            TempChip = pdx->Key.PlxChip;
            break;
    }

    switch (TempChip)
    {
        case 0x6150:
        case 0x6152:
        case 0x6154:
        case 0x6156:
        case 0x6254:
        case 0x6350:
        case 0x6520:
        case 0x6540:
        case 0x8111:
        case 0x8112:
            /******************************************
             * For devices that do not support 32-bit
             * EEPROM accesses, use 2 16-bit accesses
             *****************************************/
            PlxEepromWriteByOffset_16(
                pdx,
                offset,
                (U16)value
                );

            PlxEepromWriteByOffset_16(
                pdx,
                offset + sizeof(U16),
                (U16)(value >> 16)
                );
            break;

        case 0x8114:
        case 0x8000:
            return Plx8000_EepromWriteByOffset(
                pdx,
                offset,
                value
                );

        case 0x6140:  // 6140 does not support an EEPROM
        default:
            return PLX_STATUS_UNSUPPORTED;
    }

    return PLX_STATUS_OK;
}




/*******************************************************************************
 *
 * Function   :  PlxEepromReadByOffset_16
 *
 * Description:  Read a 16-bit value from the EEPROM at a specified offset
 *
 ******************************************************************************/
PLX_STATUS
PlxEepromReadByOffset_16(
    PLX_DEVICE_NODE *pdx,
    U32              offset,
    U16             *pValue
    )
{
    U16        TempChip;
    U32        Value_32;
    PLX_STATUS status;


    // Set default return value
    *pValue = 0;

    // Make sure offset is aligned on 16-bit boundary
    if (offset & (1 << 0))
    {
        return PLX_STATUS_INVALID_OFFSET;
    }

    // Generalize by device type
    switch (pdx->Key.PlxChip & 0xFF00)
    {
        case 0x2300:
        case 0x3300:
        case 0x8500:
        case 0x8600:
        case 0x8700:
        case 0x9700:
            TempChip = 0x8000;
            break;

        default:
            TempChip = pdx->Key.PlxChip;
            break;
    }

    switch (TempChip)
    {
        case 0x6150:
        case 0x6152:
        case 0x6154:
        case 0x6156:
        case 0x6254:
        case 0x6350:
        case 0x6520:
        case 0x6540:
            return Plx6000_EepromReadByOffset_16(
                pdx,
                offset,
                pValue
                );

        case 0x8111:
        case 0x8112:
            return Plx8111_EepromReadByOffset_16(
                pdx,
                offset,
                pValue
                );

        case 0x8114:
        case 0x8000:
            /******************************************
             * For devices that do not support 16-bit
             * EEPROM accesses, use 32-bit access
             *****************************************/

            // Get 32-bit value
            status =
                PlxEepromReadByOffset(
                    pdx,
                    (offset & ~0x3),
                    &Value_32
                    );

            if (status != PLX_STATUS_OK)
            {
                return status;
            }

            // Return desired 16-bit portion
            if (offset & 0x3)
            {
                *pValue = (U16)(Value_32 >> 16);
            }
            else
            {
                *pValue = (U16)Value_32;
            }
            break;

        case 0x6140:  // 6140 does not support an EEPROM
        default:
            DebugPrintf((
                "ERROR - Device (%04X) does not support 16-bit EEPROM access\n",
                pdx->Key.PlxChip
                ));
            return PLX_STATUS_UNSUPPORTED;
    }

    return PLX_STATUS_OK;
}




/*******************************************************************************
 *
 * Function   :  PlxEepromWriteByOffset_16
 *
 * Description:  Write a 16-bit value to the EEPROM at a specified offset
 *
 ******************************************************************************/
PLX_STATUS
PlxEepromWriteByOffset_16(
    PLX_DEVICE_NODE *pdx,
    U32              offset,
    U16              value
    )
{
    U16        TempChip;
    U32        Value_32;
    PLX_STATUS status;


    // Make sure offset is aligned on 16-bit boundary
    if (offset & (1 << 0))
    {
        return PLX_STATUS_INVALID_OFFSET;
    }

    // Generalize by device type
    switch (pdx->Key.PlxChip & 0xFF00)
    {
        case 0x2300:
        case 0x3300:
        case 0x8500:
        case 0x8600:
        case 0x8700:
        case 0x9700:
            TempChip = 0x8000;
            break;

        default:
            TempChip = pdx->Key.PlxChip;
            break;
    }

    switch (TempChip)
    {
        case 0x6150:
        case 0x6152:
        case 0x6154:
        case 0x6156:
        case 0x6254:
        case 0x6350:
        case 0x6520:
        case 0x6540:
            return Plx6000_EepromWriteByOffset_16(
                pdx,
                offset,
                value
                );

        case 0x8111:
        case 0x8112:
            return Plx8111_EepromWriteByOffset_16(
                pdx,
                offset,
                value
                );

        case 0x8114:
        case 0x8000:
            /******************************************
             * For devices that do not support 16-bit
             * EEPROM accesses, use 32-bit access
             *****************************************/

            // Get current 32-bit value
            status =
                PlxEepromReadByOffset(
                    pdx,
                    (offset & ~0x3),
                    &Value_32
                    );

            if (status != PLX_STATUS_OK)
            {
                return status;
            }

            // Insert new 16-bit value in correct location
            if (offset & 0x3)
            {
                Value_32 = ((U32)value << 16) | (Value_32 & 0xFFFF);
            }
            else
            {
                Value_32 = ((U32)value) | (Value_32 & 0xFFFF0000);
            }

            // Update EEPROM
            return PlxEepromWriteByOffset(
                pdx,
                (offset & ~0x3),
                Value_32
                );

        case 0x6140:  // 6140 does not support an EEPROM
        default:
            DebugPrintf((
                "ERROR - Device (%04X) does not support 16-bit EEPROM access\n",
                pdx->Key.PlxChip
                ));
            return PLX_STATUS_UNSUPPORTED;
    }

    return PLX_STATUS_OK;
}




/*******************************************************************************
 *
 * Function   :  PlxPciIoPortTransfer
 *
 * Description:  Read or Write from/to an I/O port
 *
 ******************************************************************************/
PLX_STATUS
PlxPciIoPortTransfer(
    U64              IoPort,
    VOID            *pBuffer,
    U32              SizeInBytes,
    PLX_ACCESS_TYPE  AccessType,
    BOOLEAN          bReadOperation
    )
{
    U8  AlignMask;
    U8  AccessSize;
    U32 Value;


    if (pBuffer == NULL)
    {
        return PLX_STATUS_NULL_PARAM;
    }

    // Verify size & type
    switch (AccessType)
    {
        case BitSize8:
            AlignMask  = 0;
            AccessSize = sizeof(U8);
            break;

        case BitSize16:
            AlignMask  = (1 << 0);
            AccessSize = sizeof(U16);
            break;

        case BitSize32:
            AlignMask  = (3 << 0);
            AccessSize = sizeof(U32);
            break;

        default:
            return PLX_STATUS_INVALID_ACCESS;
    }

    // Verify alignments
    if (IoPort & AlignMask)
    {
        DebugPrintf(("ERROR - I/O port not %d-bit aligned\n", (AccessSize * 8)));
        return PLX_STATUS_INVALID_ADDR;
    }

    if (SizeInBytes & AlignMask)
    {
        DebugPrintf(("ERROR - Byte count not %d-bit aligned\n", (AccessSize * 8)));
        return PLX_STATUS_INVALID_SIZE;
    }

    // Perform operation
    while (SizeInBytes)
    {
        if (bReadOperation)
        {
            switch (AccessType)
            {
                case BitSize8:
                    Value = IO_PORT_READ_8( IoPort );
                    break;

                case BitSize16:
                    Value = IO_PORT_READ_16( IoPort );
                    break;

                case BitSize32:
                    Value = IO_PORT_READ_32( IoPort );
                    break;

                default:
                    // Added to avoid compiler warnings
                    break;
            }

            // Copy value to user buffer
            if (copy_to_user( pBuffer, &Value, AccessSize ) != 0)
            {
                return PLX_STATUS_INVALID_ACCESS;
            }
        }
        else
        {
            // Copy next value from user buffer
            if (copy_from_user( &Value, pBuffer, AccessSize ) != 0)
            {
                return PLX_STATUS_INVALID_ACCESS;
            }

            switch (AccessType)
            {
                case BitSize8:
                    IO_PORT_WRITE_8( IoPort, (U8)Value );
                    break;

                case BitSize16:
                    IO_PORT_WRITE_16( IoPort, (U16)Value );
                    break;

                case BitSize32:
                    IO_PORT_WRITE_32( IoPort, (U32)Value );
                    break;

                default:
                    // Added to avoid compiler warnings
                    break;
            }
        }

        // Adjust pointer & byte count
        pBuffer      = (U8*)pBuffer + AccessSize;
        SizeInBytes -= AccessSize;
    }

    return PLX_STATUS_OK;
}




/*******************************************************************************
 *
 * Function   :  PlxPciPhysicalMemoryAllocate
 *
 * Description:  Allocate physically contiguous page-locked memory
 *
 ******************************************************************************/
PLX_STATUS
PlxPciPhysicalMemoryAllocate(
    PLX_DEVICE_NODE  *pdx,
    PLX_PHYSICAL_MEM *pPciMem,
    BOOLEAN           bSmallerOk
    )
{
    return PLX_STATUS_UNSUPPORTED;
}




/*******************************************************************************
 *
 * Function   :  PlxPciPhysicalMemoryFree
 *
 * Description:  Free previously allocated physically contiguous page-locked memory
 *
 ******************************************************************************/
PLX_STATUS
PlxPciPhysicalMemoryFree(
    PLX_DEVICE_NODE  *pdx,
    PLX_PHYSICAL_MEM *pPciMem
    )
{
    return PLX_STATUS_UNSUPPORTED;
}




/*******************************************************************************
 *
 * Function   :  PlxPciPhysicalMemoryMap
 *
 * Description:  Maps physical memory to User virtual address space
 *
 ******************************************************************************/
PLX_STATUS
PlxPciPhysicalMemoryMap(
    PLX_DEVICE_NODE  *pdx,
    PLX_PHYSICAL_MEM *pPciMem
    )
{
    return PLX_STATUS_UNSUPPORTED;
}




/*******************************************************************************
 *
 * Function   :  PlxPciPhysicalMemoryUnmap
 *
 * Description:  Unmap physical memory from User virtual address space
 *
 ******************************************************************************/
PLX_STATUS
PlxPciPhysicalMemoryUnmap(
    PLX_DEVICE_NODE  *pdx,
    PLX_PHYSICAL_MEM *pPciMem
    )
{
    return PLX_STATUS_UNSUPPORTED;
}




/*******************************************************************************
 *
 * Function   :  PlxPciPerformanceInitializeProperties
 *
 * Description:  Initializes the performance properties for a device
 *
 ******************************************************************************/
PLX_STATUS
PlxPciPerformanceInitializeProperties(
    PLX_DEVICE_NODE *pdx,
    PLX_PERF_PROP   *pPerfProp
    )
{
    U8            StnPortCount;
    PLX_PERF_PROP PerfProp;


    // Clear performance object
    RtlZeroMemory( &PerfProp, sizeof(PLX_PERF_PROP) );

    // Verify supported device & set number of ports per station
    switch (pdx->Key.PlxFamily)
    {
        case PLX_FAMILY_DENEB:
        case PLX_FAMILY_CYGNUS:
        case PLX_FAMILY_MIRA:
        case PLX_FAMILY_CAPELLA_1:
        case PLX_FAMILY_CAPELLA_2:
            StnPortCount = 4;
            break;

        case PLX_FAMILY_SIRIUS:
        case PLX_FAMILY_ATLAS:
            StnPortCount = 16;
            break;

        case PLX_FAMILY_DRACO_1:
        case PLX_FAMILY_DRACO_2:
            StnPortCount = 8;    // Device actually only uses 6 ports out of 8
            break;

        default:
            DebugPrintf(("ERROR - Unsupported PLX chip (%04X)\n", pdx->Key.PlxChip));
            return PLX_STATUS_UNSUPPORTED;
    }

    // Update port properties
    PlxGetPortProperties( pdx, &pdx->PortProp );

    if (pdx->PortProp.PortNumber >= PERF_MAX_PORTS)
    {
        DebugPrintf(("ERROR - Port number exceeds maximum (%d)\n", (PERF_MAX_PORTS-1)));
        return PLX_STATUS_UNSUPPORTED;
    }

    // Store PLX chip family
    PerfProp.PlxFamily = pdx->Key.PlxFamily;

    // Store relevant port properties for later calculations
    PerfProp.PortNumber = pdx->PortProp.PortNumber;
    PerfProp.LinkWidth  = pdx->PortProp.LinkWidth;
    PerfProp.LinkSpeed  = pdx->PortProp.LinkSpeed;

    // Determine station and port number within station
    PerfProp.Station     = (U8)(pdx->PortProp.PortNumber / StnPortCount);
    PerfProp.StationPort = (U8)(pdx->PortProp.PortNumber % StnPortCount);

    // Copy result to user buffer
    if (copy_to_user( pPerfProp, &PerfProp, sizeof(PLX_PERF_PROP) ) != 0)
    {
        return PLX_STATUS_INVALID_ACCESS;
    }

    return PLX_STATUS_OK;
}




/*******************************************************************************
 *
 * Function   :  PlxPciPerformanceMonitorControl
 *
 * Description:  Controls PLX Performance Monitor
 *
 ******************************************************************************/
PLX_STATUS
PlxPciPerformanceMonitorControl(
    PLX_DEVICE_NODE *pdx,
    PLX_PERF_CMD     command
    )
{
    U8  Bit_EgressEn;
    U8  bStationBased;
    U8  bEgressAllPorts;
    U32 i;
    U32 offset;
    U32 pexBase;
    U32 RegValue;
    U32 RegCommand;
    U32 StnCount;
    U32 StnPortCount;
    U32 Offset_Control;


    // Set default base offset to PCIe port registers
    pexBase = 0;

    // Verify supported device
    switch (pdx->Key.PlxFamily)
    {
        case PLX_FAMILY_DENEB:
        case PLX_FAMILY_SIRIUS:
        case PLX_FAMILY_MIRA:
            Offset_Control = 0x568;

            // Offset changes for MIRA in legacy EP mode
            if ((pdx->Key.PlxFamily == PLX_FAMILY_MIRA) &&
                (pdx->Key.DeviceMode == PLX_PORT_LEGACY_ENDPOINT))
            {
                Offset_Control = 0x1568;
            }
            break;

        case PLX_FAMILY_CYGNUS:
        case PLX_FAMILY_DRACO_1:
        case PLX_FAMILY_DRACO_2:
        case PLX_FAMILY_CAPELLA_1:
        case PLX_FAMILY_CAPELLA_2:
            Offset_Control = 0x3E0;
            break;

        case PLX_FAMILY_ATLAS:
            pexBase        = ATLAS_PEX_REGS_BASE_OFFSET;
            Offset_Control = 0x3E0;
            break;

        default:
            DebugPrintf(("ERROR - Unsupported PLX chip (%04X)\n", pdx->Key.PlxChip));
            return PLX_STATUS_UNSUPPORTED;
    }

    switch (command)
    {
        case PLX_PERF_CMD_START:
            DebugPrintf(("Reset & enable monitor with infinite sampling\n"));
            RegCommand = ((U32)1 << 31) | ((U32)1 << 30) | ((U32)1 << 28) | ((U32)1 << 27);
            break;

        case PLX_PERF_CMD_STOP:
            DebugPrintf(("Reset & disable monitor\n"));
            RegCommand = ((U32)1 << 30);
            break;

        default:
            return PLX_STATUS_INVALID_DATA;
    }

    // Added to avoid compiler warning
    Bit_EgressEn = 0;
    StnCount     = 0;
    StnPortCount = 0;

    // Default to single station access
    bStationBased   = FALSE;
    bEgressAllPorts = FALSE;

    // Set control offset & enable/disable counters in stations
    switch (pdx->Key.PlxFamily)
    {
        case PLX_FAMILY_MIRA:
            /*************************************************************
             * For MIRA, there are filters available to control counting
             * of different packet types. The PLX API doesn't currently
             * support this filtering so all are enabled.
             *
             * PCIe filters in 664h[29:20] of P0
             *   20: Disable MWr 32 TLP counter
             *   21: Disable MWr 64 TLP counter
             *   22: Disable Msg TLP counter
             *   23: Disable MRd 32 TLP counter
             *   24: Disable MRd 64 TLP counter
             *   25: Disable other NP TLP counter
             *   26: Disable ACK DLLP counting
             *   27: Disable Update-FC P DLLP counter
             *   28: Disable Update-FC NP DLLP counter
             *   29: Disable Update-FC CPL DLLP counter
             ************************************************************/
            // In MIRA legacy EP mode, PCIe registers start at 1000h
            if (pdx->Key.DeviceMode == PLX_PORT_LEGACY_ENDPOINT)
            {
                offset = 0x1664;
            }
            else
            {
                offset = 0x664;
            }

            // Clear 664[29:20] to enable all counters
            RegValue = PLX_8000_REG_READ( pdx, offset );
            PLX_8000_REG_WRITE( pdx, offset, RegValue & ~(0x3FF << 20) );
            break;

        case PLX_FAMILY_CYGNUS:
        case PLX_FAMILY_DRACO_1:
        case PLX_FAMILY_DRACO_2:
        case PLX_FAMILY_CAPELLA_1:
        case PLX_FAMILY_CAPELLA_2:
        case PLX_FAMILY_ATLAS:
            // Set device configuration
            if (pdx->Key.PlxFamily == PLX_FAMILY_CYGNUS)
            {
                Bit_EgressEn = 7;
                StnCount     = 6;
                StnPortCount = 4;
            }
            else if ((pdx->Key.PlxFamily == PLX_FAMILY_DRACO_1) ||
                     (pdx->Key.PlxFamily == PLX_FAMILY_DRACO_2))
            {
                Bit_EgressEn = 6;
                StnCount     = 3;
                StnPortCount = 8;    // Device actually only uses 6 ports out of 8
            }
            else if ((pdx->Key.PlxFamily == PLX_FAMILY_CAPELLA_1) ||
                     (pdx->Key.PlxFamily == PLX_FAMILY_CAPELLA_2) ||
                     (pdx->Key.PlxFamily == PLX_FAMILY_ATLAS))
            {
                Bit_EgressEn    = 6;
                StnCount        = 6;
                StnPortCount    = 4;
                bStationBased   = TRUE;
                bEgressAllPorts = TRUE;

                // Override station port count for Atlas
                if (pdx->Key.PlxFamily == PLX_FAMILY_ATLAS)
                {
                    StnPortCount = 16;
                }

                // Disable probe mode (350h[8]=0)
                if (command == PLX_PERF_CMD_START)
                {
                    RegValue = PLX_8000_REG_READ( pdx, pexBase + 0x350 );
                    PLX_8000_REG_WRITE( pdx, pexBase + 0x350, RegValue & ~((U32)1 << 8) );
                }
            }

            // For certain chips, set 3F0h[9:8]=3 to disable probe mode interval
            // timer & avoid RAM pointer corruption
            if ( (command == PLX_PERF_CMD_START) &&
                 ((pdx->Key.PlxFamily == PLX_FAMILY_DRACO_1)   ||
                  (pdx->Key.PlxFamily == PLX_FAMILY_DRACO_2)   ||
                  (pdx->Key.PlxFamily == PLX_FAMILY_CAPELLA_1) ||
                  (pdx->Key.PlxFamily == PLX_FAMILY_CAPELLA_2) ||
                  (pdx->Key.PlxFamily == PLX_FAMILY_ATLAS)) )
            {
                RegValue = PLX_8000_REG_READ( pdx, pexBase + 0x3F0 );
                PLX_8000_REG_WRITE( pdx, pexBase + 0x3F0, RegValue | (3 << 8) );
            }

            // Enable/Disable Performance Counter in each station (or ports if applicable)
            for (i = 0; i < (StnCount * StnPortCount); i++)
            {
                // Set port base offset
                offset = pexBase + (i * 0x1000);

                // Ingress ports (in station port 0 only)
                if ((i % StnPortCount) == 0)
                {
                    RegValue = PLX_8000_REG_READ( pdx, offset + 0x768 );

                    if (command == PLX_PERF_CMD_START)
                    {
                        PLX_8000_REG_WRITE( pdx, offset + 0x768, RegValue | ((U32)1 << 29) );
                    }
                    else
                    {
                        PLX_8000_REG_WRITE( pdx, offset + 0x768, RegValue & ~((U32)1 << 29) );
                    }
                }

                // Egress ports
                if (bEgressAllPorts || ((i % StnPortCount) == 0))
                {
                    RegValue = PLX_8000_REG_READ( pdx, offset + 0xF30 );

                    // On Atlas, F30h[21] is egress credit enable but always reads 0, so ensure remains set
                    if (pdx->Key.PlxFamily == PLX_FAMILY_ATLAS)
                    {
                        RegValue |= ((U32)1 << 21);
                    }

                    if (command == PLX_PERF_CMD_START)
                    {
                        PLX_8000_REG_WRITE( pdx, offset + 0xF30, RegValue | ((U32)1 << Bit_EgressEn) );
                    }
                    else
                    {
                        PLX_8000_REG_WRITE( pdx, offset + 0xF30, RegValue & ~((U32)1 << Bit_EgressEn) );
                    }
                }
            }
            break;
    }

    // Update monitor
    for (i = 0; i < StnCount; i++)
    {
        if ((i == 0) || bStationBased)
        {
            PLX_8000_REG_WRITE(
                pdx,
                pexBase + Offset_Control + (i * StnPortCount * 0x1000),
                RegCommand
                );
        }
    }

    return PLX_STATUS_OK;
}




/*******************************************************************************
 *
 * Function   :  PlxPciPerformanceResetCounters
 *
 * Description:  Resets the internal performance counters
 *
 ******************************************************************************/
PLX_STATUS
PlxPciPerformanceResetCounters(
    PLX_DEVICE_NODE *pdx
    )
{
    U32 i;
    U32 StnCount;
    U32 StnPortCount;
    U32 Offset_Control;


    // Verify supported device
    switch (pdx->Key.PlxFamily)
    {
        case PLX_FAMILY_DENEB:
        case PLX_FAMILY_SIRIUS:
        case PLX_FAMILY_MIRA:
            Offset_Control = 0x568;

            // Offset changes for MIRA in legacy EP mode
            if ((pdx->Key.PlxFamily == PLX_FAMILY_MIRA) &&
                (pdx->Key.DeviceMode == PLX_PORT_LEGACY_ENDPOINT))
            {
                Offset_Control = 0x1568;
            }
            break;

        case PLX_FAMILY_CYGNUS:
        case PLX_FAMILY_DRACO_1:
        case PLX_FAMILY_DRACO_2:
        case PLX_FAMILY_CAPELLA_1:
        case PLX_FAMILY_CAPELLA_2:
            Offset_Control = 0x3E0;
            break;

        case PLX_FAMILY_ATLAS:
            Offset_Control = ATLAS_PEX_REGS_BASE_OFFSET + 0x3E0;
            break;

        default:
            DebugPrintf(("ERROR - Unsupported PLX chip (%04X)\n", pdx->Key.PlxChip));
            return PLX_STATUS_UNSUPPORTED;
    }

    // Set station counts for station-based monitor chips
    if ((pdx->Key.PlxFamily == PLX_FAMILY_CAPELLA_1) ||
        (pdx->Key.PlxFamily == PLX_FAMILY_CAPELLA_2))
    {
        StnCount     = 6;
        StnPortCount = 4;
    }
    else if (pdx->Key.PlxFamily == PLX_FAMILY_ATLAS)
    {
        StnCount     = 6;
        StnPortCount = 16;
    }
    else
    {
        StnCount     = 1;
        StnPortCount = 0;
    }

    // Enable monitor in each station
    for (i = 0; i < StnCount; i++)
    {
        // Reset (30) & enable monitor (31) & infinite sampling (28) & start (27)
        PLX_8000_REG_WRITE(
            pdx,
            Offset_Control + (i * StnPortCount * 0x1000),
            ((U32)1 << 31) | ((U32)1 << 30) | ((U32)1 << 28) | ((U32)1 << 27)
            );
    }

    return PLX_STATUS_OK;
}




/*******************************************************************************
 *
 * Function   :  PlxPciPerformanceGetCounters
 *
 * Description:  Retrieves a snapshot of the Performance Counters for selected ports
 *
 * Notes      :  The counters are retrieved from the PLX chip as a preset structure.
 *               Each register read returns the next value from the sequence.  The
 *               structure varies between some PLX chips.  Below is a diagram of each.
 *
 *   IN    = Ingress port
 *   EG    = Egress port
 *   PH    = Number of Posted Headers (Write TLPs)
 *   PDW   = Number of Posted DWords
 *   NPH   = Number of Non-Posted Headers
 *   NPDW  = Non-Posted DWords (Read TLP Dwords)
 *   CPLH  = Number of Completion Headers (CPL TLPs)
 *   CPLDW = Number of Completion DWords
 *   DLLP  = Number of DLLPs
 *   PHY   = PHY Layer (always 0)
 *   PLD   = USB endpoint payload count
 *   RAW   = USB endpoint raw byte count
 *   PKT   = USB endpoint packet count
 *
 *           Deneb & Cygnus                  Draco                     Sirius
 *      --------------------------   ----------------------    ----------------------
 *          14 counters/port          14 counters/port          13 counters/port
 *           4 ports/station           6 ports/station          16 ports/station
 *    Deneb: 3 stations (12 ports)     3 stations (18 ports)     1 station (16 ports)
 *   Cygnus: 6 stations (24 ports)
 *          56 counters/station       84 counters/station      208 counters/station
 *  Deneb: 168 counters (56 * 3)     252 counters (84 * 3)     208 counters (208 * 1)
 * Cygnus: 336 counters (56 * 6)
 *
 *       off     Counter            off     Counter            off      Counter
 *           -----------------          -----------------          ------------------
 *         0| Port 0 IN PH    |       0| Port 0 IN PH    |       0| Port 0 IN PH     |
 *         4| Port 0 IN PDW   |       4| Port 0 IN PDW   |       4| Port 0 IN PDW    |
 *         8| Port 0 IN NPDW  |       8| Port 0 IN NPDW  |       8| Port 0 IN NPDW   |
 *         C| Port 0 IN CPLH  |       C| Port 0 IN CPLH  |       C| Port 0 IN CPLH   |
 *        10| Port 0 IN CPLDW |      10| Port 0 IN CPLDW |      10| Port 0 IN CPLDW  |
 *          |-----------------|        |-----------------|        |------------------|
 *        14| Port 1 IN PH    |      14| Port 1 IN PH    |      14| Port 1 IN PH     |
 *          |       :         |        |       :         |        |       :          |
 *          |       :         |        |       :         |        |       :          |
 *        24| Port 1 IN CPLDW |      24| Port 1 IN CPLDW |      24| Port 1 IN CPLDW  |
 *          |-----------------|        |/\/\/\/\/\/\/\/\/|        |/\/\/\/\/\/\/\/\/\|
 *        28| Port 2 IN PH    |        |       :         |        |       :          |
 *          |       :         |        |       :         |        |       :          |
 *          |       :         |        |       :         |        |       :          |
 *        38| Port 2 IN CPLDW |        |       :         |        |       :          |
 *          |-----------------|        |       :         |        |       :          |
 *        3C| Port 3 IN PH    |        |/\/\/\/\/\/\/\/\/|        |/\/\/\/\/\/\/\/\/\|
 *          |       :         |      64| Port 5 IN PH    |     12C| Port 15 IN PH    |
 *          |       :         |        |       :         |        |       :          |
 *        4C| Port 3 IN CPLDW |        |       :         |        |       :          |
 *          |-----------------|      74| Port 5 IN CPLDW |     13C| Port 15 IN CPLDW |
 *        50| Port 0 EG PH    |        |-----------------|        |------------------|
 *        54| Port 0 EG PDW   |      78| Port 0 EG PH    |     140| Port 0 EG PH     |
 *        58| Port 0 EG NPDW  |      7C| Port 0 EG PDW   |     144| Port 0 EG PDW    |
 *        5C| Port 0 EG CPLH  |      80| Port 0 EG NPDW  |     148| Port 0 EG NPDW   |
 *        60| Port 0 EG CPLDW |      84| Port 0 EG CPLH  |     14C| Port 0 EG CPLH   |
 *          |-----------------|      88| Port 0 EG CPLDW |     150| Port 0 EG CPLDW  |
 *        64| Port 1 EG PH    |        |-----------------|        |------------------|
 *          |       :         |      8C| Port 1 EG PH    |     154| Port 1 EG PH     |
 *          |       :         |        |       :         |        |       :          |
 *        74| Port 1 EG CPLDW |        |       :         |        |       :          |
 *          |-----------------|      9C| Port 1 EG CPLDW |     164| Port 1 EG CPLDW  |
 *        78| Port 2 EG PH    |        |/\/\/\/\/\/\/\/\/|        |/\/\/\/\/\/\/\/\/\|
 *          |       :         |        |       :         |        |       :          |
 *          |       :         |        |       :         |        |       :          |
 *        88| Port 2 EG CPLDW |        |       :         |        |       :          |
 *          |-----------------|        |       :         |        |       :          |
 *        8C| Port 3 EG PH    |        |       :         |        |       :          |
 *          |       :         |        |/\/\/\/\/\/\/\/\/|        |/\/\/\/\/\/\/\/\/\|
 *          |       :         |      DC| Port 5 EG PH    |     26C| Port 15 EG PH    |
 *        9C| Port 3 EG CPLDW |        |       :         |        |       :          |
 *          |-----------------|        |       :         |        |       :          |
 *        A0| Port 0 IN DLLP  |      EC| Port 5 EG CPLDW |     27C| Port 15 EG CPLDW |
 *        A4| Port 1 IN DLLP  |        |-----------------|        |------------------|
 *        A8| Port 2 IN DLLP  |      F0| Port 0 IN DLLP  |     280| Port  0 IN DLLP  |
 *        AC| Port 3 IN DLLP  |      F4| Port 1 IN DLLP  |     284| Port  2 IN DLLP  |
 *          |-----------------|      F8| Port 2 IN DLLP  |     288| Port  4 IN DLLP  |
 *        B0| Port 0 EG DLLP  |      FC| Port 3 IN DLLP  |     28C| Port  6 IN DLLP  |
 *        B4| Port 1 EG DLLP  |     100| Port 4 IN DLLP  |     290| Port  8 IN DLLP  |
 *        B8| Port 2 EG DLLP  |     104| Port 5 IN DLLP  |     294| Port 10 IN DLLP  |
 *        BC| Port 3 EG DLLP  |        |-----------------|     298| Port 12 IN DLLP  |
 *          |-----------------|     108| Port 0 EG DLLP  |     29C| Port 14 IN DLLP  |
 *        C0| Port 0 IN PHY   |     10C| Port 1 EG DLLP  |        |------------------|
 *        C4| Port 1 IN PHY   |     110| Port 2 EG DLLP  |     2A0| Port  0 EG DLLP  |
 *        C8| Port 2 IN PHY   |     114| Port 3 EG DLLP  |     2A4| Port  2 EG DLLP  |
 *        CC| Port 3 IN PHY   |     118| Port 4 EG DLLP  |     2A8| Port  4 EG DLLP  |
 *          |-----------------|     11C| Port 5 EG DLLP  |     2AC| Port  6 EG DLLP  |
 *        D0| Port 0 EG PHY   |        |-----------------|     2B0| Port  8 EG DLLP  |
 *        D4| Port 1 EG PHY   |     120| Port 0 IN PHY   |     2B4| Port 10 EG DLLP  |
 *        D8| Port 2 EG PHY   |     124| Port 1 IN PHY   |     2B8| Port 12 EG DLLP  |
 *        DC| Port 3 EG PHY   |     128| Port 2 IN PHY   |     2BC| Port 14 EG DLLP  |
 *           -----------------      12C| Port 3 IN PHY   |        |------------------|
 *                                  130| Port 4 IN PHY   |     2C0| Port  1 IN DLLP  |
 *                                  134| Port 5 IN PHY   |     2C4| Port  3 IN DLLP  |
 *                                     |-----------------|     2C8| Port  5 IN DLLP  |
 *                                  138| Port 0 EG PHY   |     2CC| Port  7 IN DLLP  |
 *                                  13C| Port 1 EG PHY   |     2D0| Port  9 IN DLLP  |
 *                                  140| Port 2 EG PHY   |     2D4| Port 11 IN DLLP  |
 *                                  144| Port 3 EG PHY   |     2D8| Port 13 IN DLLP  |
 *                                  148| Port 4 EG PHY   |     2DC| Port 15 IN DLLP  |
 *                                  14C| Port 5 EG PHY   |        |------------------|
 *                                      -----------------      2E0| Port  1 EG DLLP  |
 *                                                             2E4| Port  3 EG DLLP  |
 *                                                             2E8| Port  5 EG DLLP  |
 *                                                             2EC| Port  7 EG DLLP  |
 *                                                             2F0| Port  9 EG DLLP  |
 *                                                             2F4| Port 11 EG DLLP  |
 *                                                             2F8| Port 13 EG DLLP  |
 *                                                             2FC| Port 15 EG DLLP  |
 *                                                                |------------------|
 *                                                             300| Port 0 PHY       |
 *                                                                |       :          |
 *                                                                |       :          |
 *                                                             33C| Port 15 PHY      |
 *                                                                 ------------------
 *
 *       Mira                        Capella-1/2                            Atlas
 * -----------------------     -----------------------               -----------------------
 *  14 PCIe counters/port      14 counters/port                      14 counters/port
 *   4 ports/station            4 pts/stn but 5 in RAM               16 ports/station
 *   1 stations (4 ports)       6 stn (24 ports)                      6 stations (96 ports)
 *                            *SW uses 30 pts to read all
 *  86 counters/station                                             224 counters/station
 *  86 counters (86 * 1)        70 counters/station               1,344 counters (224 * 6)
 *                             420 counters (70 * 6)
 *
 * off     Counter             off     Counter                      off     Counter
 *     -----------------           -----------------                    -----------------
 *   0| Port 0 IN PH    |        0| Port 0 IN PH    |                 0| Port 0 IN PH    |
 *   4| Port 0 IN PDW   |        4| Port 0 IN PDW   |                 4| Port 0 IN PDW   |
 *   8| Port 0 IN NPDW  |        8| Port 0 IN NPDW  |                 8| Port 0 IN NPH   |
 *   C| Port 0 IN CPLH  |        C| Port 0 IN CPLH  |                 C| Port 0 IN NPDW  |
 *  10| Port 0 IN CPLDW |       10| Port 0 IN CPLDW |                10| Port 0 IN CPLH  |
 *    |-----------------|         |-----------------|                14| Port 0 IN CPLDW |
 *  14| Port 1 IN PH    |       14| Port 1 IN PH    |                  |-----------------|
 *    |       :         |         |       :         |                18| Port 1 IN PH    |
 *    |       :         |         |       :         |                  |       :         |
 *  24| Port 1 IN CPLDW |       24| Port 1 IN CPLDW |                  |       :         |
 *    |-----------------|         |/\/\/\/\/\/\/\/\/|                2C| Port 1 IN CPLDW |
 *  28| Port 2 IN PH    |         |       :         |                  |/\/\/\/\/\/\/\/\/|
 *    |       :         |         |       :         |                  |       :         |
 *    |       :         |         |       :         |                  |       :         |
 *  38| Port 2 IN CPLDW |         |       :         |                  |       :         |
 *    |-----------------|         |       :         |                  |       :         |
 *  3C| Port 3 IN PH    |         |/\/\/\/\/\/\/\/\/|                  |       :         |
 *    |       :         |       50| Port 4 IN PH    |                  |/\/\/\/\/\/\/\/\/|
 *    |       :         |         |       :         |               168| Port 15 IN PH   |
 *  4C| Port 3 IN CPLDW |         |       :         |                  |       :         |
 *    |-----------------|       60| Port 4 IN CPLDW |                  |       :         |
 *  50| Port 0 EG PH    |         |-----------------|               17C| Port 15 IN CPDW |
 *  54| Port 0 EG PDW   |       64| Port 0 EG PH    |                  |-----------------|
 *  58| Port 0 EG NPDW  |       68| Port 0 EG PDW   |               180| Port 0 EG PH    |
 *  5C| Port 0 EG CPLH  |       6c| Port 0 EG NPDW  |               184| Port 0 EG PDW   |
 *  60| Port 0 EG CPLDW |       70| Port 0 EG CPLH  |               188| Port 0 EG NPH   |
 *    |-----------------|       74| Port 0 EG CPLDW |               18C| Port 0 EG NPDW  |
 *  64| Port 1 EG PH    |         |-----------------|               190| Port 0 EG CPLH  |
 *    |       :         |       78| Port 1 EG PH    |               194| Port 0 EG CPLDW |
 *    |       :         |         |       :         |                  |-----------------|
 *  74| Port 1 EG CPLDW |         |       :         |               198| Port 1 EG PH    |
 *    |-----------------|       88| Port 1 EG CPLDW |                  |       :         |
 *  78| Port 2 EG PH    |         |/\/\/\/\/\/\/\/\/|                  |       :         |
 *    |       :         |         |       :         |               1AC| Port 1 EG CPLDW |
 *    |       :         |         |       :         |                  |/\/\/\/\/\/\/\/\/|
 *  88| Port 2 EG CPLDW |         |       :         |                  |       :         |
 *    |-----------------|         |       :         |                  |       :         |
 *  8C| Port 3 EG PH    |         |       :         |                  |       :         |
 *    |       :         |         |/\/\/\/\/\/\/\/\/|                  |       :         |
 *    |       :         |         |-----------------|                  |       :         |
 *  9C| Port 3 EG CPLDW |       B4| Port 4 EG PH    |                  |/\/\/\/\/\/\/\/\/|
 *    |-----------------|         |       :         |                  |-----------------|
 *  A0| Port 0 IN DLLP  |         |       :         |               2E8| Port 4 EG PH    |
 *  A4| Port 1 IN DLLP  |       C4| Port 4 EG CPLDW |                  |       :         |
 *  A8| Port 2 IN DLLP  |         |-----------------|                  |       :         |
 *  AC| Port 3 IN DLLP  |       C8| Port 0 IN DLLP  |               2FC| Port 4 EG CPLDW |
 *    |-----------------|       CC| Port 1 IN DLLP  |                  |-----------------|
 *  B0| Port 0 EG DLLP  |       D0| Port 2 IN DLLP  |               300| Port 0 IN DLLP  |
 *  B4| Port 1 EG DLLP  |       D4| Port 3 IN DLLP  |               304| Port 1 IN DLLP  |
 *  B8| Port 2 EG DLLP  |       D8| Port 4 IN DLLP  |               308| Port 2 IN DLLP  |
 *  BC| Port 3 EG DLLP  |         |-----------------|               30C| Port 3 IN DLLP  |
 *    |-----------------|       DC| -- Invalid --   |               310| Port 4 IN DLLP  |
 *  C0| GPEP_0 IN PLD   |       E0| Port 0 EG DLLP  |               314| Port 5 IN DLLP  |
 *  C4| GPEP_0 IN RAW   |       E4| Port 1 EG DLLP  |               318| Port 6 IN DLLP  |
 *  C8| GPEP_0 IN PKT   |       E8| Port 2 EG DLLP  |               31C| Port 7 IN DLLP  |
 *    |-----------------|       EC| Port 3 EG DLLP  |*P4 EG DLLP    320| Port 8 IN DLLP  |
 *  CC| GPEP_1 IN PLD   |         |-----------------|  missing      324| Port 9 IN DLLP  |
 *  D0| GPEP_1 IN RAW   |       F0| Port 0 IN PHY   |               328| Port 10 IN DLLP |
 *  D4| GPEP_1 IN PKT   |       F4| Port 1 IN PHY   |               32C| Port 11 IN DLLP |
 *    |-----------------|       F8| Port 2 IN PHY   |               330| Port 12 IN DLLP |
 *  D8| GPEP_2 IN PLD   |       FC| Port 3 IN PHY   |               334| Port 13 IN DLLP |
 *  DC| GPEP_2 IN RAW   |      100| Port 4 IN PHY   |               338| Port 14 IN DLLP |
 *  E0| GPEP_2 IN PKT   |         |-----------------|               33C| Port 15 IN DLLP |
 *    |-----------------|      104| Port 0 EG PHY   |                  |-----------------|
 *  E4| GPEP_3 IN PLD   |      108| Port 1 EG PHY   |               340| Port 0 EG DLLP  |
 *  E8| GPEP_3 IN RAW   |      10C| Port 2 EG PHY   |               344| Port 1 EG DLLP  |
 *  EC| GPEP_3 IN PKT   |      110| Port 3 EG PHY   |               348| Port 2 EG DLLP  |
 *    |-----------------|      114| Port 4 EG PHY   |               34C| Port 3 EG DLLP  |
 *  F0| GPEP_0 OUT PLD  |          -----------------                350| Port 4 EG DLLP  |
 *  F4| GPEP_0 OUT RAW  |                                           354| Port 5 EG DLLP  |
 *  F8| GPEP_0 OUT PKT  |                                           358| Port 6 EG DLLP  |
 *    |-----------------|                                           35C| Port 7 EG DLLP  |
 *  FC| GPEP_1 OUT PLD  |                                           360| Port 8 EG DLLP  |
 * 100| GPEP_1 OUT RAW  |                                           364| Port 9 EG DLLP  |
 * 104| GPEP_1 OUT PKT  |                                           368| Port 10 EG DLLP |
 *    |-----------------|                                           36C| Port 11 EG DLLP |
 * 108| GPEP_2 OUT PLD  |                                           370| Port 12 EG DLLP |
 * 10C| GPEP_2 OUT RAW  |                                           374| Port 13 EG DLLP |
 * 110| GPEP_2 OUT PKT  |                                           378| Port 14 EG DLLP |
 *    |-----------------|                                           37C| Port 15 EG DLLP |
 * 114| GPEP_3 OUT PLD  |                                               -----------------
 * 118| GPEP_3 OUT RAW  |
 * 11C| GPEP_3 OUT PKT  |
 *    |-----------------|
 * 120| EP_0 IN PLD     |
 * 124| EP_0 IN RAW     |
 * 128| EP_0 IN PKT     |
 *    |-----------------|
 * 12C| EP_0 OUT PLD    |
 * 130| EP_0 OUT RAW    |
 * 134| EP_0 OUT PKT    |
 *    |-----------------|
 * 138| PHY (always 0)  |
 * 13C| PHY (always 0)  |
 *     -----------------
 *
 ******************************************************************************/
PLX_STATUS
PlxPciPerformanceGetCounters(
    PLX_DEVICE_NODE *pdx,
    PLX_PERF_PROP   *pPerfProps,
    U8               NumOfObjects
    )
{
    U8             NumCounters;
    U8             CurrStation;
    U8             StnCount;
    U8             StnPortCount;
    U8             bStationBased;
    U8             RamStnPortCount;
    U8             InEgPerPortCount;
    U16            i;
    U16            index;
    U16            IndexBase;
    U32            Offset_Fifo;
    U32            Offset_RamCtrl;
    U32            RegValue;
    U32           *pCounter;
    U32           *pCounters;
    U32           *pCounter_Prev;
    U32            Counter_PrevTmp[PERF_COUNTERS_PER_PORT];
    S64            TmpValue;
    PLX_STATUS     status;
    PLX_PERF_PROP *pTmpPerfProps;


    // Default to single station access
    bStationBased = FALSE;

    // Assume station port count in RAM is identical to station port count
    RamStnPortCount = 0;

    // Most chips have 5 ingress & egress counters per port
    InEgPerPortCount = 5;

    // Setup parameters for reading counters
    switch (pdx->Key.PlxFamily)
    {
        case PLX_FAMILY_DENEB:
            Offset_RamCtrl  = 0x618;
            Offset_Fifo     = 0x628;
            NumCounters     = 14;
            StnCount        = 3;
            StnPortCount    = 4;
            break;

        case PLX_FAMILY_SIRIUS:
            Offset_RamCtrl  = 0x618;
            Offset_Fifo     = 0x628;
            NumCounters     = 13;
            StnCount        = 1;
            StnPortCount    = 16;
            break;

        case PLX_FAMILY_CYGNUS:
            Offset_RamCtrl  = 0x3F0;
            Offset_Fifo     = 0x3E4;
            NumCounters     = 14;
            StnCount        = 6;
            StnPortCount    = 4;
            break;

        case PLX_FAMILY_MIRA:
            Offset_RamCtrl  = 0x618;
            Offset_Fifo     = 0x628;
            NumCounters     = 12;
            StnCount        = 1;
            StnPortCount    = 4;

            // In MIRA legacy EP mode, PCIe registers start at 1000h
            if (pdx->Key.DeviceMode == PLX_PORT_LEGACY_ENDPOINT)
            {
                Offset_RamCtrl += 0x1000;
                Offset_Fifo    += 0x1000;
            }
            break;

        case PLX_FAMILY_DRACO_1:
        case PLX_FAMILY_DRACO_2:
            Offset_RamCtrl  = 0x3F0;
            Offset_Fifo     = 0x3E4;
            NumCounters     = 14;
            StnCount        = 3;
            StnPortCount    = 6;
            break;

        case PLX_FAMILY_CAPELLA_1:
        case PLX_FAMILY_CAPELLA_2:
            Offset_RamCtrl  = 0x3F0;
            Offset_Fifo     = 0x3E4;
            NumCounters     = 14;
            StnCount        = 6;
            StnPortCount    = 4;
            RamStnPortCount = 5;
            bStationBased   = TRUE;
            break;

        case PLX_FAMILY_ATLAS:
            Offset_RamCtrl   = ATLAS_PEX_REGS_BASE_OFFSET + 0x3F0;
            Offset_Fifo      = ATLAS_PEX_REGS_BASE_OFFSET + 0x3E4;
            NumCounters      = 14;
            StnCount         = 6;
            StnPortCount     = 16;
            bStationBased    = TRUE;
            InEgPerPortCount = 6;    // Non-Posted header count added
            break;

        default:
            DebugPrintf(("ERROR - Unsupported PLX chip (%04X)\n", pdx->Key.PlxChip));
            return PLX_STATUS_UNSUPPORTED;
    }

    // Set RAM station port count if matches station port count
    if (RamStnPortCount == 0)
    {
        RamStnPortCount = StnPortCount;
    }

    // Allocate buffer to contain counter data for all ports
    pCounters = kmalloc( (StnCount * RamStnPortCount) * NumCounters * sizeof(U32), GFP_KERNEL );
    if (pCounters == NULL)
    {
        return PLX_STATUS_INSUFFICIENT_RES;
    }

    // Allocate temporary kernel buffer to hold performance properties
    pTmpPerfProps = kmalloc( sizeof(PLX_PERF_PROP) * NumOfObjects, GFP_KERNEL );
    if (pTmpPerfProps == NULL)
    {
        kfree( pCounters );
        return PLX_STATUS_INSUFFICIENT_RES;
    }

    // Copy performance properties from user buffer
    if (copy_from_user(
            pTmpPerfProps,
            pPerfProps,
            sizeof(PLX_PERF_PROP) * NumOfObjects
            ) != 0)
    {
        ErrorPrintf(("ERROR - Unable to copy counter data from user buffer\n"));
        kfree( pCounters );
        kfree( pTmpPerfProps );
        return PLX_STATUS_INVALID_ACCESS;
    }

    // RAM control
    RegValue = ((U32)2 << 4) |   // Capture type ([5:4])
               ((U32)1 << 2) |   // Reset read pointer
               ((U32)1 << 0);    // Enable RAM

    // Reset RAM read pointer
    for (i = 0; i < StnCount; i++)
    {
        if ((i == 0) || bStationBased)
        {
            PLX_8000_REG_WRITE(
                pdx,
                Offset_RamCtrl + (i * StnPortCount * 0x1000),
                RegValue
                );
        }
    }

    // Read in all counters
    i           = 0;
    CurrStation = 0;
    while (i < (StnCount * RamStnPortCount * NumCounters))
    {
        // Check if reached station boundary
        if ((i % (NumCounters * RamStnPortCount)) == 0)
        {
            DebugPrintf_Cont(("\n"));
            if (i == 0)
            {
                DebugPrintf(("           Counters\n"));
                DebugPrintf(("----------------------------------\n"));
            }
            else
            {
                // Increment to next station
                CurrStation++;

                // For station based counters use register in station port 0
                if (bStationBased)
                {
                    Offset_Fifo += (StnPortCount * 0x1000);
                }
            }

            DebugPrintf(("Station %d:\n", CurrStation));
            DebugPrintf(("%03X:", (U16)(i * sizeof(U32))));
        }
        else if ((i % 4) == 0)
        {
            DebugPrintf_Cont(("\n"));
            DebugPrintf(("%03X:", (U16)(i * sizeof(U32))));
        }

        // Get next counter
        pCounters[i] = PLX_8000_REG_READ( pdx, Offset_Fifo );
        DebugPrintf_Cont((" %08X", pCounters[i]));

        // Jump to next counter
        i++;
    }
    DebugPrintf_Cont(("\n"));

    // Assign counter values to enabled ports
    i = 0;
    while (i < NumOfObjects)
    {
        // Verify the station & port numbers are within valid range
        if ( (pTmpPerfProps[i].Station >= StnCount) ||
             (pTmpPerfProps[i].StationPort >= RamStnPortCount) )
        {
            ErrorPrintf((
                "ERROR - Station or station port invalid in perf object %d\n",
                i
                ));
            // Skip to next object
            i++;
            continue;
        }

        // Make a copy of the previous values before overwriting them
        RtlCopyMemory(
            Counter_PrevTmp,
            &(pTmpPerfProps[i].Prev_IngressPostedHeader),
            PERF_COUNTERS_PER_PORT * sizeof(U32)    // All 14 counters in structure
            );

        // Save current values to previous
        RtlCopyMemory(
            &(pTmpPerfProps[i].Prev_IngressPostedHeader),
            &(pTmpPerfProps[i].IngressPostedHeader),
            PERF_COUNTERS_PER_PORT * sizeof(U32)    // All 14 counters in structure
            );

        // Calculate starting index for counters based on port in station
        IndexBase = pTmpPerfProps[i].Station * (NumCounters * RamStnPortCount);

        // Ingress counters start at index 0 from base
        index = IndexBase + 0 + (pTmpPerfProps[i].StationPort * InEgPerPortCount);

        // Get ingress counters (5 or 6 DW/port)
        pTmpPerfProps[i].IngressPostedHeader = pCounters[index++];  // 0
        pTmpPerfProps[i].IngressPostedDW     = pCounters[index++];  // 1
        if (pdx->Key.PlxFamily == PLX_FAMILY_ATLAS)
        {
            // NP header added in Atlas
            pTmpPerfProps[i].IngressNonpostedHdr  = pCounters[index++];  // 2
        }
        pTmpPerfProps[i].IngressNonpostedDW  = pCounters[index++];  // 2 or 3
        pTmpPerfProps[i].IngressCplHeader    = pCounters[index++];  // 3 or 4
        pTmpPerfProps[i].IngressCplDW        = pCounters[index];    // 4 or 5

        // Egress counters start after ingress
        index = IndexBase + (InEgPerPortCount * RamStnPortCount)
                          + (pTmpPerfProps[i].StationPort * InEgPerPortCount);

        // Get egress counters (5 or 6 DW/port)
        pTmpPerfProps[i].EgressPostedHeader = pCounters[index++];   // 0
        pTmpPerfProps[i].EgressPostedDW     = pCounters[index++];   // 1
        if (pdx->Key.PlxFamily == PLX_FAMILY_ATLAS)
        {
            // NP header added in Atlas
            pTmpPerfProps[i].EgressNonpostedHdr  = pCounters[index++];  // 2
        }
        pTmpPerfProps[i].EgressNonpostedDW  = pCounters[index++];   // 2 or 3
        pTmpPerfProps[i].EgressCplHeader    = pCounters[index++];   // 3 or 4
        pTmpPerfProps[i].EgressCplDW        = pCounters[index++];   // 4 or 5

        // DLLP ingress counters start after egress
        index = IndexBase + ((InEgPerPortCount * 2) * RamStnPortCount);

        // DLLP counter location depends upon chip
        if (pdx->Key.PlxFamily == PLX_FAMILY_SIRIUS)
        {
            // Even port number DLLP counters are first
            index += (pTmpPerfProps[i].StationPort / 2);

            // Odd port number DLLP counters follow even ports
            if (pTmpPerfProps[i].StationPort & (1 << 0))
            {
                index += RamStnPortCount;
            }
        }
        else
        {
            index += pTmpPerfProps[i].StationPort;
        }

        // Get DLLP ingress counters (1 DW/port)
        pTmpPerfProps[i].IngressDllp = pCounters[index];

        // Egress DLLP counters follow Ingress
        if (pdx->Key.PlxFamily == PLX_FAMILY_SIRIUS)
        {
            // Even ports are grouped together
            index += (RamStnPortCount / 2);
        }
        else
        {
            index += RamStnPortCount;
        }

        // For Capella, egress DLLP skips one offset & port 4 is lost
        if ((pdx->Key.PlxFamily == PLX_FAMILY_CAPELLA_1) ||
            (pdx->Key.PlxFamily == PLX_FAMILY_CAPELLA_2))
        {
            index++;
        }

        // Get DLLP egress counters (1 DW/port)
        pTmpPerfProps[i].EgressDllp = pCounters[index];

        /**********************************************************
         * In some cases on Draco 1 chips, device may incorrectly
         * report a counter as 0. The following code checks the
         * current & previous counters to detect this case. If the
         * issue is present, the previous value is used instead to
         * minimize data reporting errors.
         *********************************************************/
        if ((pdx->Key.PlxFamily == PLX_FAMILY_DRACO_1) &&
            (pTmpPerfProps[i].LinkWidth != 0))
        {
            // Setup initial pointers to stored counters
            pCounter      = &pTmpPerfProps[i].IngressPostedHeader;
            pCounter_Prev = &pTmpPerfProps[i].Prev_IngressPostedHeader;

            // Verify each counter & use previous on error
            for (index = 0; index < PERF_COUNTERS_PER_PORT; index++)
            {
                if (((*pCounter == 0) && (*pCounter_Prev != 0)) || (*pCounter == 0x4C041301))
                {
                    // Store 64-bit counter in case of wrapping
                    TmpValue = *pCounter_Prev;
                    if (*pCounter_Prev < Counter_PrevTmp[index])
                    {
                        TmpValue += ((S64)1 << 32);
                    }

                    // Re-use difference between previous 2 counter values
                    *pCounter = *pCounter_Prev + (U32)(TmpValue - Counter_PrevTmp[index]);
                }

                // Increment to next counter
                pCounter++;
                pCounter_Prev++;
            }
        }

        // Go to next performance object
        i++;
    }

    // Copy performance properties to user buffer
    if (copy_to_user(
            pPerfProps,
            pTmpPerfProps,
            sizeof(PLX_PERF_PROP) * NumOfObjects
            ) != 0)
    {
        ErrorPrintf(("ERROR - Unable to copy counter data to user buffer\n"));
        status = PLX_STATUS_INVALID_ACCESS;
    }
    else
    {
        status = PLX_STATUS_OK;
    }

    // Release temporary buffers
    kfree( pCounters );
    kfree( pTmpPerfProps );

    return status;
}




/*******************************************************************************
 *
 * Function   :  PlxMH_GetProperties
 *
 * Description:  Controls PLX Performance Monitor
 *
 ******************************************************************************/
PLX_STATUS
PlxMH_GetProperties(
    PLX_DEVICE_NODE     *pdx,
    PLX_MULTI_HOST_PROP *pMHProp
    )
{
    U8  i;
    U8  TotalVS;
    U32 RegValue;
    U32 RegVSEnable;


    // Clear properties
    RtlZeroMemory( pMHProp, sizeof(PLX_MULTI_HOST_PROP) );

    // Default to standard mode
    pMHProp->SwitchMode = PLX_CHIP_MODE_STANDARD;

    // Verify supported device
    switch (pdx->Key.PlxFamily)
    {
        case PLX_FAMILY_CYGNUS:
        case PLX_FAMILY_DRACO_1:
        case PLX_FAMILY_DRACO_2:
        case PLX_FAMILY_CAPELLA_1:
        case PLX_FAMILY_CAPELLA_2:
            break;

        default:
            DebugPrintf(("ERROR - Device (%04X) doesn't support VS mode\n", pdx->Key.PlxChip));
            return PLX_STATUS_UNSUPPORTED;
    }

    // Attempt to read management port configuration
    RegValue = PLX_8000_REG_READ( pdx, 0x354 );

    // Get active VS mask
    RegVSEnable = PLX_8000_REG_READ( pdx, 0x358 );

    // Device properties are only available from the management port
    if ((RegValue == 0) && ((RegVSEnable & ~((U32)1 << 0)) == 0))
    {
        // In VS mode, but not management port
        pMHProp->SwitchMode = PLX_CHIP_MODE_VIRT_SW;
        DebugPrintf(("Device is in VS mode, but not management port\n"));
        return PLX_STATUS_OK;
    }

    // Report this is management port regardless of mode
    pMHProp->bIsMgmtPort = TRUE;

    // Store management port info
    pMHProp->MgmtPortNumActive    = (U8)((RegValue >> 0) & 0x1F);
    pMHProp->MgmtPortNumRedundant = (U8)((RegValue >> 8) & 0x1F);

    // Determine which management ports are active
    if (RegValue & ((U32)1 << 5))
    {
        pMHProp->bMgmtPortActiveEn = TRUE;
    }

    if (RegValue & ((U32)1 << 13))
    {
        pMHProp->bMgmtPortRedundantEn = TRUE;
    }

    // Provide active VS's
    pMHProp->VS_EnabledMask = (U16)RegVSEnable;

    TotalVS = 0;

    // Count number of active virtual switches
    for (i = 0; i < 8; i++)
    {
        // Check if VS is active
        if (RegVSEnable & ((U32)1 << i))
        {
            // Increment count
            TotalVS++;

            // Get VS upstream port ([4:0])
            RegValue =
                PLX_8000_REG_READ(
                    pdx,
                    0x360 + (i * sizeof(U32))
                    );

            pMHProp->VS_UpstreamPortNum[i] = (U8)((RegValue >> 0) & 0x1F);

            // Get VS downstream port vector ([23:0])
            RegValue =
                PLX_8000_REG_READ(
                    pdx,
                    0x380 + (i * sizeof(U32))
                    );

            pMHProp->VS_DownstreamPorts[i] = RegValue & 0x00FFFFFF;

            // Remove upstream port from downstream vectors
            pMHProp->VS_DownstreamPorts[i] &= ~((U32)1 << pMHProp->VS_UpstreamPortNum[i]);
        }
    }

    // If more than one VS is active, then VS mode
    if (TotalVS == 1)
    {
        DebugPrintf(("Device is in standard mode\n"));
    }
    else
    {
        pMHProp->SwitchMode = PLX_CHIP_MODE_VIRT_SW;

        DebugPrintf((
            "\n"
            "Mode        : Virtual Switch\n"
            "Enabled VS  : %04X\n"
            "Active Mgmt : %d (%s)\n"
            "Backup Mgmt : %d (%s)\n"
            "VS UP-DS pts: 0:%02d-%08X 1:%02d-%08X 2:%02d-%08X 3:%02d-%08X\n"
            "              4:%02d-%08X 5:%02d-%08X 6:%02d-%08X 7:%02d-%08X\n",
            pMHProp->VS_EnabledMask,
            pMHProp->MgmtPortNumActive,
            (pMHProp->bMgmtPortActiveEn) ? "enabled" : "disabled",
            pMHProp->MgmtPortNumRedundant,
            (pMHProp->bMgmtPortRedundantEn) ? "enabled" : "disabled",
            pMHProp->VS_UpstreamPortNum[0], (int)pMHProp->VS_DownstreamPorts[0],
            pMHProp->VS_UpstreamPortNum[1], (int)pMHProp->VS_DownstreamPorts[1],
            pMHProp->VS_UpstreamPortNum[2], (int)pMHProp->VS_DownstreamPorts[2],
            pMHProp->VS_UpstreamPortNum[3], (int)pMHProp->VS_DownstreamPorts[3],
            pMHProp->VS_UpstreamPortNum[4], (int)pMHProp->VS_DownstreamPorts[4],
            pMHProp->VS_UpstreamPortNum[5], (int)pMHProp->VS_DownstreamPorts[5],
            pMHProp->VS_UpstreamPortNum[6], (int)pMHProp->VS_DownstreamPorts[6],
            pMHProp->VS_UpstreamPortNum[7], (int)pMHProp->VS_DownstreamPorts[7]
            ));
    }

    return PLX_STATUS_OK;
}




/*******************************************************************************
 *
 * Function   :  PlxMH_MigrateDsPorts
 *
 * Description:  Migrates one or more downstream ports from one VS to another
 *
 ******************************************************************************/
PLX_STATUS
PlxMH_MigrateDsPorts(
    PLX_DEVICE_NODE *pdx,
    U16              VS_Source,
    U16              VS_Dest,
    U32              DsPortMask,
    BOOLEAN          bResetSrc
    )
{
    U8                  i;
    U32                 RegValue;
    PLX_STATUS          status;
    PLX_MULTI_HOST_PROP MHProp;


    // Get current MH properties
    status = PlxMH_GetProperties( pdx, &MHProp );
    if (status != PLX_STATUS_OK)
    {
        return status;
    }

    // Operation only available from VS management port
    if ((MHProp.SwitchMode != PLX_CHIP_MODE_VIRT_SW) || (MHProp.bIsMgmtPort == FALSE))
    {
        return PLX_STATUS_UNSUPPORTED;
    }

    DebugPrintf((
        "Migrate DS ports (%08X) from VS%d ==> VS%d %s\n",
        (int)DsPortMask, VS_Source, VS_Dest,
        (bResetSrc) ? "& reset source port" : ""
        ));

    if ((VS_Source >= 8) || (VS_Dest >= 8))
    {
        DebugPrintf(("ERROR - Source or Dest VS are not valid\n"));
        return PLX_STATUS_INVALID_DATA;
    }

    // Verify source VS is enabled
    if ((MHProp.VS_EnabledMask & ((U32)1 << VS_Source)) == 0)
    {
        DebugPrintf(("ERROR - Source VS (%d) not enabled\n", VS_Source));
        return PLX_STATUS_DISABLED;
    }

    // Verify DS ports to move currently owned by source port
    if ((MHProp.VS_DownstreamPorts[VS_Source] & DsPortMask) != DsPortMask)
    {
        DebugPrintf(("ERROR - One or more DS ports not owned by source VS\n"));
        return PLX_STATUS_INVALID_DATA;
    }

    // Migrate DS ports
    for (i = 0; i < (sizeof(U32) * 8); i++)
    {
        // Migrate port from source to destination if requested
        if (DsPortMask & ((U32)1 << i))
        {
            // Remove port from source
            MHProp.VS_DownstreamPorts[VS_Source] &= ~((U32)1 << i);

            // Add port to destination
            MHProp.VS_DownstreamPorts[VS_Dest] |= ((U32)1 << i);
        }
    }

    // Update source & destination ports
    PLX_8000_REG_WRITE(
        pdx,
        0x380 + (VS_Source * sizeof(U32)),
        MHProp.VS_DownstreamPorts[VS_Source]
        );

    PLX_8000_REG_WRITE(
        pdx,
        0x380 + (VS_Dest * sizeof(U32)),
        MHProp.VS_DownstreamPorts[VS_Dest]
        );

    // Make sure destination VS is enabled
    if ((MHProp.VS_EnabledMask & ((U32)1 << VS_Dest)) == 0)
    {
        DebugPrintf(("Enable destination VS%d\n", VS_Dest));
        PLX_8000_REG_WRITE(
            pdx,
            0x358,
            MHProp.VS_EnabledMask | ((U32)1 << VS_Dest)
            );
    }

    // Reset source port if requested
    if (bResetSrc)
    {
        RegValue =
            PLX_8000_REG_READ(
                pdx,
                0x3A0
                );

        // Put VS into reset
        PLX_8000_REG_WRITE(
            pdx,
            0x3A0,
            RegValue | ((U32)1 << VS_Source)
            );

        // Keep in reset for a short time
        Plx_sleep( 10 );

        // Take VS out of reset
        PLX_8000_REG_WRITE(
            pdx,
            0x3A0,
            RegValue & ~((U32)1 << VS_Source)
            );
    }

    return PLX_STATUS_OK;
}
