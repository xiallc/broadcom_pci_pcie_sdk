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
 *      02-01-13 : PLX SDK v7.00
 *
 ******************************************************************************/


#include "ApiFunc.h"
#include "ChipFunc.h"
#include "Eep_6000.h"
#include "Eep_8000.h"
#include "Eep_8111.h"
#include "PciFunc.h"
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
    U32               RegValue;
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

        // Compare Bus number
        if (pKey->bus != (U8)PCI_FIELD_IGNORE)
        {
            if (pKey->bus != pDevice->Key.bus)
            {
                bMatchLoc = FALSE;
            }
        }

        // Compare Slot number
        if (pKey->slot != (U8)PCI_FIELD_IGNORE)
        {
            if (pKey->slot != pDevice->Key.slot)
            {
                bMatchLoc = FALSE;
            }
        }

        // Compare Function number
        if (pKey->function != (U8)PCI_FIELD_IGNORE)
        {
            if (pKey->function != pDevice->Key.function)
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

        // Subsystem ID only valid in PCI header 0 type
        if (pDevice->PciHeaderType == 0)
        {
            // Get the Subsystem Device/Vendor ID
            if (pDevice->Key.SubVendorId == 0)
            {
                PLX_PCI_REG_READ(
                    pDevice,
                    0x2c,        // PCI Subsystem ID
                    &RegValue
                    );

                pDevice->Key.SubVendorId = (U16)(RegValue & 0xffff);
                pDevice->Key.SubDeviceId = (U16)(RegValue >> 16);
            }

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

        // Get the Revision ID if haven't yet
        if (pDevice->Key.Revision == 0)
        {
            PLX_PCI_REG_READ(
                pDevice,
                0x08,        // PCI Revision ID
                &RegValue
                );

            pDevice->Key.Revision = (U8)(RegValue & 0xFF);
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
                    "Criteria matched device %04X_%04X [b:%02x s:%02x f:%x]\n",
                    pDevice->Key.DeviceId, pDevice->Key.VendorId,
                    pDevice->Key.bus, pDevice->Key.slot, pDevice->Key.function
                    ));

                return ApiSuccess;
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

    return ApiInvalidDeviceInfo;
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

    return ApiSuccess;
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
    U16           TempChip;
    BOOLEAN       bSetUptreamNode;
    PLX_PORT_PROP PortProp;


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
            // Get port properties to ensure upstream node
            PlxGetPortProperties(
                pdx,
                &PortProp
                );

            if (PortProp.PortType != PLX_PORT_UPSTREAM)
            {
                DebugPrintf(("ERROR - Chip type may only be changed on upstream port\n"));
                return ApiUnsupportedFunction;
            }

            // Flag to update upstream node
            bSetUptreamNode = TRUE;
            break;

        default:
            DebugPrintf((
                "ERROR - Invalid or unsupported chip type (%04X)\n",
                ChipType
                ));
            return ApiInvalidData;
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

    return ApiSuccess;
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
        PlxGetExtendedCapabilityOffset(
            pdx,
            0x10       // CAP_ID_PCI_EXPRESS
            );

    if (Offset_PcieCap == 0)
    {
        DebugPrintf(("Device does not support PCI Express Capability\n"));

        // Mark device as non-PCIe
        pPortProp->bNonPcieDevice = TRUE;

        // Default to a legacy endpoint
        if (pdx->PciHeaderType == 0)
            pPortProp->PortType = PLX_PORT_LEGACY_ENDPOINT;
        else
            pPortProp->PortType = PLX_PORT_UNKNOWN;

        return ApiSuccess;
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
    if (MaxSize <= 5)
        pPortProp->MaxPayloadSupported = 128 * ((U16)Plx_pow_int(2, MaxSize));

    // Get PCIe Device Control
    PLX_PCI_REG_READ(
        pdx,
        Offset_PcieCap + 0x08,
        &RegValue
        );

    // Get max payload size field
    MaxSize = (U8)(RegValue >> 5) & 0x7;

    // Set max payload size (=128 * (2 ^ MaxPaySizeField))
    if (MaxSize <= 5)
        pPortProp->MaxPayloadSize = 128 * ((U16)Plx_pow_int(2, MaxSize));

    // Get max read request size field
    MaxSize = (U8)(RegValue >> 12) & 0x7;

    // Set max read request size (=128 * (2 ^ MaxReadReqSizeField))
    if (MaxSize <= 5)
        pPortProp->MaxReadReqSize = 128 * ((U16)Plx_pow_int(2, MaxSize));

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

    // Store the port number in the device properties
    pdx->PortNumber = pPortProp->PortNumber;

    DebugPrintf((
        "Type=%d Num=%d MaxPd=%d/%d MaxRdReq=%d LW=x%d/x%d LS=%d/%d\n",
        pPortProp->PortType, pPortProp->PortNumber,
        pPortProp->MaxPayloadSize, pPortProp->MaxPayloadSupported,
        pPortProp->MaxReadReqSize,
        pPortProp->LinkWidth, pPortProp->MaxLinkWidth,
        pPortProp->LinkSpeed, pPortProp->MaxLinkSpeed
        ));

    return ApiSuccess;
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
    return ApiUnsupportedFunction;
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
        *pStatus = ApiUnsupportedFunction;

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

    return ApiUnsupportedFunction;
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
        case 0:
            if ((BarIndex != 0) && (BarIndex > 5))
                return ApiInvalidIndex;
            break;

        case 1:
            if ((BarIndex != 0) && (BarIndex != 1))
            {
                DebugPrintf(("BAR %d does not exist on PCI type 1 header\n", BarIndex));
                return ApiInvalidIndex;
            }
            break;

        default:
            return ApiInvalidIndex;
    }

    // Return BAR properties
    *pBarProperties = pdx->PciBar[BarIndex].Properties;

    // Do nothing if upper 32-bits of 64-bit BAR
    if (pdx->PciBar[BarIndex].Properties.Flags & PLX_BAR_FLAG_UPPER_32)
    {
        DebugPrintf(("BAR %d is upper address of 64-bit BAR %d\n", BarIndex, BarIndex-1));
        return ApiSuccess;
    }

    // Display BAR properties if enabled
    if (pdx->PciBar[BarIndex].Properties.Size == 0)
    {
        DebugPrintf(("BAR %d is disabled\n", BarIndex));
        return ApiSuccess;
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
        "    Size     : %08llX (%lld %s)\n",
        pdx->PciBar[BarIndex].Properties.Size,
        pdx->PciBar[BarIndex].Properties.Size < ((U64)1 << 10) ?
            pdx->PciBar[BarIndex].Properties.Size :
            pdx->PciBar[BarIndex].Properties.Size >> 10,
        pdx->PciBar[BarIndex].Properties.Size < ((U64)1 << 10) ? "Bytes" : "KB"
        ));

    if (pdx->PciBar[BarIndex].Properties.Flags & PLX_BAR_FLAG_MEM)
    {
        DebugPrintf((
            "    Property : %sPrefetchable %d-bit\n",
            (pdx->PciBar[BarIndex].Properties.Flags & PLX_BAR_FLAG_PREFETCHABLE) ? "" : "Non-",
            (pdx->PciBar[BarIndex].Properties.Flags & PLX_BAR_FLAG_64_BIT) ? 64 : 32
            ));
    }

    return ApiSuccess;
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
        return ApiInsufficientResources;
    }

    // Save the intended device node
    pObject->pDevice = pdx;

    // Record the desired BAR index
    pObject->BarIndex = BarIndex;

    // Increment request counter
    pdx->MapRequestPending++;

    // Add to list of map objects
    spin_lock(
        &(pdx->pdx->Lock_MapParamsList)
        );

    list_add_tail(
        &(pObject->ListEntry),
        &(pdx->pdx->List_MapParams)
        );

    spin_unlock(
        &(pdx->pdx->Lock_MapParamsList)
        );

    DebugPrintf((
        "Added map object (%p) to list (node=%p)\n",
        pObject, pdx
        ));

    return ApiSuccess;
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
    return ApiUnsupportedFunction;
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
                *pStatus = PLX_EEPROM_STATUS_VALID;
            else
                *pStatus = PLX_EEPROM_STATUS_NONE;

            return ApiSuccess;

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

    return ApiUnsupportedFunction;
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
                OffsetProbe = 0x3EC + sizeof(U32);
            else
                OffsetProbe = 0x378 + sizeof(U32);
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
            OffsetProbe = 0x10;     // No CRC, just use any reasonable address
            break;

        case 0:
        default:
            DebugPrintf((
                "ERROR - Not a supported PLX device (%04X)\n",
                pdx->Key.PlxChip
                ));
            return ApiUnsupportedFunction;
    }

    DebugPrintf(("Probe EEPROM at offset %02xh\n", OffsetProbe));

    // Get the current value
    status =
        PlxEepromReadByOffset(
            pdx,
            OffsetProbe,
            &ValueOriginal
            );

    if (status != ApiSuccess)
        return status;

    // Prepare inverse value to write
    ValueWrite = ~(ValueOriginal);

    // Write inverse of original value
    status =
        PlxEepromWriteByOffset(
            pdx,
            OffsetProbe,
            ValueWrite
            );

    if (status != ApiSuccess)
        return status;

    // Read updated value
    status =
        PlxEepromReadByOffset(
            pdx,
            OffsetProbe,
            &ValueRead
            );

    if (status != ApiSuccess)
        return status;

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

    return ApiSuccess;
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
            return ApiUnsupportedFunction;
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
            return ApiInvalidData;
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
            return ApiUnsupportedFunction;
    }

    DebugPrintf((
       "%s EEPROM address width to %dB\n",
       (status == ApiSuccess) ? "Set" : "ERROR - Unable to set",
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

    switch (pdx->Key.PlxChip & 0xF000)
    {
        case 0x8000:
            return Plx8000_EepromCrcGet(
                pdx,
                pCrc,
                pCrcStatus
                );
    }

    // CRC not supported
    return ApiUnsupportedFunction;
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

    switch (pdx->Key.PlxChip & 0xF000)
    {
        case 0x8000:
            return Plx8000_EepromCrcUpdate(
                pdx,
                pCrc,
                bUpdateEeprom
                );
    }

    // CRC not supported
    return ApiUnsupportedFunction;
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
        return ApiInvalidOffset;

    // Generalize by device type
    switch (pdx->Key.PlxChip & 0xFF00)
    {
        case 0x2300:
        case 0x3300:
        case 0x8500:
        case 0x8600:
        case 0x8700:
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
            return ApiUnsupportedFunction;
    }

    return ApiSuccess;
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
        return ApiInvalidOffset;

    // Generalize by device type
    switch (pdx->Key.PlxChip & 0xFF00)
    {
        case 0x2300:
        case 0x3300:
        case 0x8500:
        case 0x8600:
        case 0x8700:
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
            return ApiUnsupportedFunction;
    }

    return ApiSuccess;
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
        return ApiInvalidOffset;

    // Generalize by device type
    switch (pdx->Key.PlxChip & 0xFF00)
    {
        case 0x2300:
        case 0x3300:
        case 0x8500:
        case 0x8600:
        case 0x8700:
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

            if (status != ApiSuccess)
                return status;

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
            return ApiUnsupportedFunction;
    }

    return ApiSuccess;
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
        return ApiInvalidOffset;

    // Generalize by device type
    switch (pdx->Key.PlxChip & 0xFF00)
    {
        case 0x2300:
        case 0x3300:
        case 0x8500:
        case 0x8600:
        case 0x8700:
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

            if (status != ApiSuccess)
                return status;

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
            return ApiUnsupportedFunction;
    }

    return ApiSuccess;
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
    U8 AccessSize;


    if (pBuffer == NULL)
        return ApiNullParam;

    // Verify size & type
    switch (AccessType)
    {
        case BitSize8:
            AccessSize = sizeof(U8);
            break;

        case BitSize16:
            if (IoPort & (1 << 0))
            {
                DebugPrintf(("ERROR - I/O port not aligned on 16-bit boundary\n"));
                return ApiInvalidAddress;
            }

            if (SizeInBytes & (1 << 0))
            {
                DebugPrintf(("ERROR - Byte count not aligned on 16-bit boundary\n"));
                return ApiInvalidSize;
            }
            AccessSize = sizeof(U16);
            break;

        case BitSize32:
            if (IoPort & 0x3)
            {
                DebugPrintf(("ERROR - I/O port not aligned on 32-bit boundary\n"));
                return ApiInvalidAddress;
            }

            if (SizeInBytes & 0x3)
            {
                DebugPrintf(("ERROR - Byte count not aligned on 32-bit boundary\n"));
                return ApiInvalidSize;
            }
            AccessSize = sizeof(U32);
            break;

        default:
            return ApiInvalidAccessType;
    }

    while (SizeInBytes)
    {
        if (bReadOperation)
        {
            switch (AccessType)
            {
                case BitSize8:
                    *(U8*)pBuffer = IO_PORT_READ_8( IoPort );
                    break;

                case BitSize16:
                    *(U16*)pBuffer = IO_PORT_READ_16( IoPort );
                    break;

                case BitSize32:
                    *(U32*)pBuffer = IO_PORT_READ_32( IoPort );
                    break;

                default:
                    // Added to avoid compiler warnings
                    break;
            }
        }
        else
        {
            switch (AccessType)
            {
                case BitSize8:
                    IO_PORT_WRITE_8(
                        IoPort,
                        *(U8*)pBuffer
                        );
                    break;

                case BitSize16:
                    IO_PORT_WRITE_16(
                        IoPort,
                        *(U16*)pBuffer
                        );
                    break;

                case BitSize32:
                    IO_PORT_WRITE_32(
                        IoPort,
                        *(U32*)pBuffer
                        );
                    break;

                default:
                    // Added to avoid compiler warnings
                    break;
            }
        }

        // Adjust pointer & byte count
        pBuffer      = (VOID*)((PLX_UINT_PTR)pBuffer + AccessSize);
        SizeInBytes -= AccessSize;
    }

    return ApiSuccess;
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
    return ApiUnsupportedFunction;
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
    return ApiUnsupportedFunction;
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
    return ApiUnsupportedFunction;
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
    return ApiUnsupportedFunction;
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
    U8            PortsPerStation;
    PLX_PORT_PROP PortProp;


    // Clear performance object
    RtlZeroMemory( pPerfProp, sizeof(PLX_PERF_PROP) );

    // Verify supported device & set number of ports per station
    switch (pdx->Key.PlxFamily)
    {
        case PLX_FAMILY_DENEB:
        case PLX_FAMILY_CYGNUS:
            PortsPerStation = 4;
            break;

        case PLX_FAMILY_SIRIUS:
            PortsPerStation = 16;
            break;

        case PLX_FAMILY_DRACO_1:
        case PLX_FAMILY_DRACO_2:
            PortsPerStation = 8;    // Device actually only uses 6 ports out of 8
            break;

        case PLX_FAMILY_CAPELLA_1:
            if ((pdx->Key.PlxChip == 0x8714) || (pdx->Key.PlxChip == 0x8718))
                PortsPerStation = 5;
            else
                PortsPerStation = 4;
            break;

        case PLX_FAMILY_MIRA:
            PortsPerStation = 4;
            break;

        default:
            DebugPrintf(("ERROR - Unsupported PLX chip (%04X)\n", pdx->Key.PlxChip));
            return ApiUnsupportedFunction;
    }

    // Get port properties
    PlxGetPortProperties(
        pdx,
        &PortProp
        );

    if (PortProp.PortNumber >= 24)
    {
        DebugPrintf(("ERROR - Port number exceeds maximum (%d)\n", (24-1)));
        return ApiUnsupportedFunction;
    }

    // Store relevant port properties for later calculations
    pPerfProp->PortNumber = PortProp.PortNumber;
    pPerfProp->LinkWidth  = PortProp.LinkWidth;
    pPerfProp->LinkSpeed  = PortProp.LinkSpeed;

    // Determine station and port number within station
    pPerfProp->Station     = (U8)(PortProp.PortNumber / PortsPerStation);
    pPerfProp->StationPort = (U8)(PortProp.PortNumber % PortsPerStation);

    return ApiSuccess;
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
    U16 Offset_Control;
    U32 offset;
    U32 RegValue;
    U32 RegCommand;
    U32 NumStations;
    U32 PortsPerStation;


    // Verify supported device
    switch (pdx->Key.PlxFamily)
    {
        case PLX_FAMILY_DENEB:
        case PLX_FAMILY_SIRIUS:
            Offset_Control = 0x568;
            break;

        case PLX_FAMILY_CYGNUS:
        case PLX_FAMILY_DRACO_1:
        case PLX_FAMILY_DRACO_2:
        case PLX_FAMILY_CAPELLA_1:
            Offset_Control = 0x3E0;
            break;

        case PLX_FAMILY_MIRA:
            if (pdx->Key.DeviceMode == PLX_PORT_LEGACY_ENDPOINT)
                Offset_Control = 0x1568;
            else
                Offset_Control = 0x568;
            break;

        default:
            DebugPrintf(("ERROR - Unsupported PLX chip (%04X)\n", pdx->Key.PlxChip));
            return ApiUnsupportedFunction;
    }

    switch (command)
    {
        case PLX_PERF_CMD_START:
            DebugPrintf(("Reset & enable monitor with infinite sampling\n"));
            RegCommand = (1 << 31) | (1 << 30) | (1 << 28) | (1 << 27);
            break;

        case PLX_PERF_CMD_STOP:
            DebugPrintf(("Reset & disable monitor\n"));
            RegCommand = (1 << 30);
            break;

        default:
            return ApiInvalidData;
    }

    // Added to avoid compiler warning
    Bit_EgressEn    = 0;
    NumStations     = 0;
    PortsPerStation = 0;

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
                offset = 0x1664;
            else
                offset = 0x664;

            // Clear 664[29:20] to enable all counters
            RegValue = PLX_8000_REG_READ( pdx, offset );
            PLX_8000_REG_WRITE( pdx, offset, RegValue & ~(0x3FF << 20) );
            break;

        case PLX_FAMILY_CYGNUS:
        case PLX_FAMILY_DRACO_1:
        case PLX_FAMILY_DRACO_2:
        case PLX_FAMILY_CAPELLA_1:
            // Set device configuration
            if (pdx->Key.PlxFamily == PLX_FAMILY_CYGNUS)
            {
                Bit_EgressEn    = 7;
                NumStations     = 6;
                PortsPerStation = 4;
            }
            else if ((pdx->Key.PlxFamily == PLX_FAMILY_DRACO_1) ||
                     (pdx->Key.PlxFamily == PLX_FAMILY_DRACO_2))
            {
                Bit_EgressEn    = 6;
                NumStations     = 3;
                PortsPerStation = 8;    // Device actually only uses 6 ports out of 8

                // Set 3F0[9:8] to disable probe mode interval timer
                // & avoid RAM pointer corruption
                if (command == PLX_PERF_CMD_START)
                {
                    RegValue = PLX_8000_REG_READ( pdx, 0x3F0 );
                    PLX_8000_REG_WRITE( pdx, 0x3F0, RegValue | (3 << 8) );
                }
            }
            else if (pdx->Key.PlxFamily == PLX_FAMILY_CAPELLA_1)
            {
                Bit_EgressEn = 6;
                NumStations  = 6;
                if ((pdx->Key.PlxChip == 0x8714) || (pdx->Key.PlxChip == 0x8718))
                    PortsPerStation = 5;
                else
                    PortsPerStation = 4;
            }

            // Enable/Disable Performance Counter in each station
            for (offset = 0; offset < (NumStations * (PortsPerStation * 0x1000)); offset += (PortsPerStation * 0x1000))
            {
                // Ingress ports
                RegValue = PLX_8000_REG_READ( pdx, offset + 0x768 );

                if (command == PLX_PERF_CMD_START)
                    PLX_8000_REG_WRITE( pdx, offset + 0x768, RegValue | (1 << 29) );
                else
                    PLX_8000_REG_WRITE( pdx, offset + 0x768, RegValue & ~(1 << 29) );

                // Egress ports
                RegValue = PLX_8000_REG_READ( pdx, offset + 0xF30 );

                if (command == PLX_PERF_CMD_START)
                    PLX_8000_REG_WRITE( pdx, offset + 0xF30, RegValue | (1 << Bit_EgressEn) );
                else
                    PLX_8000_REG_WRITE( pdx, offset + 0xF30, RegValue & ~(1 << Bit_EgressEn) );
            }
            break;
    }

    // Update monitor
    PLX_8000_REG_WRITE(
        pdx,
        Offset_Control,
        RegCommand
        );

    return ApiSuccess;
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
    U16 Offset_Control;


    // Verify supported device
    switch (pdx->Key.PlxFamily)
    {
        case PLX_FAMILY_DENEB:
        case PLX_FAMILY_SIRIUS:
            Offset_Control = 0x568;
            break;

        case PLX_FAMILY_CYGNUS:
        case PLX_FAMILY_DRACO_1:
        case PLX_FAMILY_DRACO_2:
        case PLX_FAMILY_CAPELLA_1:
            Offset_Control = 0x3E0;
            break;

        case PLX_FAMILY_MIRA:
            if (pdx->Key.DeviceMode == PLX_PORT_LEGACY_ENDPOINT)
                Offset_Control = 0x1568;
            else
                Offset_Control = 0x568;
            break;

        default:
            DebugPrintf(("ERROR - Unsupported PLX chip (%04X)\n", pdx->Key.PlxChip));
            return ApiUnsupportedFunction;
    }

    // Reset (30) & enable monitor (31) & infinite sampling (28) & start (27)
    PLX_8000_REG_WRITE(
        pdx,
        Offset_Control,
        (1 << 31) | (1 << 30) | (1 << 28) | (1 << 27)
        );

    return ApiSuccess;
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
 *   NPDW  = Non-Posted DWords (Read TLP Dwords)
 *   CPLH  = Number of Completion Headers (CPL TLPs)
 *   CPLDW = Number of Completion DWords
 *   DLLP  = Number of DLLPs
 *   PHY   = PHY Layer (always 0)
 *   PLD   = USB endpoint payload count
 *   RAW   = USB endpoint raw byte count
 *   PKT   = USB endpoint packet count
 *
 *          Deneb & Cygnus                  Draco                     Sirius
 *     --------------------------   -----------------------   -------------------------
 *         14 counters/port           14 counters/port          13 counters/port
 *          4 ports/station            6 ports/station          16 ports/station
 *   Deneb: 3 stations (12 ports)      3 stations (18 ports)     1 station (16 ports)
 *  Cygnus: 6 stations (24 ports)
 *         56 counters/station        84 counters/station      208 counters/station
 * Deneb: 168 counters (56 * 3)      252 counters (84 * 3)     208 counters (208 * 1)
 *Cygnus: 336 counters (56 * 6)
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
 *          |-----------------|        |-----------------|        |/\/\/\/\/\/\/\/\/\|
 *        28| Port 2 IN PH    |      28| Port 2 IN PH    |        |       :          |
 *          |       :         |        |       :         |        |       :          |
 *          |       :         |        |       :         |        |       :          |
 *        38| Port 2 IN CPLDW |      38| Port 2 IN CPLDW |        |       :          |
 *          |-----------------|        |-----------------|        |       :          |
 *        3C| Port 3 IN PH    |      3C| Port 3 IN PH    |        |/\/\/\/\/\/\/\/\/\|
 *          |       :         |        |       :         |     12C| Port 15 IN PH    |
 *          |       :         |        |       :         |        |       :          |
 *        4C| Port 3 IN CPLDW |      4C| Port 3 IN CPLDW |        |       :          |
 *          |-----------------|        |-----------------|     13C| Port 15 IN CPLDW |
 *        50| Port 0 EG PH    |      50| Port 4 IN PH    |        |------------------|
 *        54| Port 0 EG PDW   |        |       :         |     140| Port 0 EG PH     |
 *        58| Port 0 EG NPDW  |        |       :         |     144| Port 0 EG PDW    |
 *        5C| Port 0 EG CPLH  |      60| Port 4 IN CPLDW |     148| Port 0 EG NPDW   |
 *        60| Port 0 EG CPLDW |        |-----------------|     14C| Port 0 EG CPLH   |
 *          |-----------------|      64| Port 5 IN PH    |     150| Port 0 EG CPLDW  |
 *        64| Port 1 EG PH    |        |       :         |        |------------------|
 *          |       :         |        |       :         |     154| Port 1 EG PH     |
 *          |       :         |      74| Port 5 IN CPLDW |        |       :          |
 *        74| Port 1 EG CPLDW |        |-----------------|        |       :          |
 *          |-----------------|      78| Port 0 EG PH    |     164| Port 1 EG CPLDW  |
 *        78| Port 2 EG PH    |      7C| Port 0 EG PDW   |        |/\/\/\/\/\/\/\/\/\|
 *          |       :         |      80| Port 0 EG NPDW  |        |       :          |
 *          |       :         |      84| Port 0 EG CPLH  |        |       :          |
 *        88| Port 2 EG CPLDW |      88| Port 0 EG CPLDW |        |       :          |
 *          |-----------------|        |-----------------|        |       :          |
 *        8C| Port 3 EG PH    |      8C| Port 1 EG PH    |        |       :          |
 *          |       :         |        |       :         |        |/\/\/\/\/\/\/\/\/\|
 *          |       :         |        |       :         |     26C| Port 15 EG PH    |
 *        9C| Port 3 EG CPLDW |      9C| Port 1 EG CPLDW |        |       :          |
 *          |-----------------|        |-----------------|        |       :          |
 *        A0| Port 0 IN DLLP  |      A0| Port 2 EG PH    |     27C| Port 15 EG CPLDW |
 *        A4| Port 1 IN DLLP  |        |       :         |        |------------------|
 *        A8| Port 2 IN DLLP  |        |       :         |     280| Port  0 IN DLLP  |
 *        AC| Port 3 IN DLLP  |      B0| Port 2 EG CPLDW |     284| Port  2 IN DLLP  |
 *          |-----------------|        |-----------------|     288| Port  4 IN DLLP  |
 *        B0| Port 0 EG DLLP  |      B4| Port 3 EG PH    |     28C| Port  6 IN DLLP  |
 *        B4| Port 1 EG DLLP  |        |       :         |     290| Port  8 IN DLLP  |
 *        B8| Port 2 EG DLLP  |        |       :         |     294| Port 10 IN DLLP  |
 *        BC| Port 3 EG DLLP  |      C4| Port 3 EG CPLDW |     298| Port 12 IN DLLP  |
 *          |-----------------|        |-----------------|     29C| Port 14 IN DLLP  |
 *        C0| Port 0 IN PHY   |      C8| Port 4 EG PH    |        |------------------|
 *        C4| Port 1 IN PHY   |        |       :         |     2A0| Port  0 EG DLLP  |
 *        C8| Port 2 IN PHY   |        |       :         |     2A4| Port  2 EG DLLP  |
 *        CC| Port 3 IN PHY   |      D8| Port 4 EG CPLDW |     2A8| Port  4 EG DLLP  |
 *          |-----------------|        |-----------------|     2AC| Port  6 EG DLLP  |
 *        D0| Port 0 EG PHY   |      DC| Port 5 EG PH    |     2B0| Port  8 EG DLLP  |
 *        D4| Port 1 EG PHY   |        |       :         |     2B4| Port 10 EG DLLP  |
 *        D8| Port 2 EG PHY   |        |       :         |     2B8| Port 12 EG DLLP  |
 *        DC| Port 3 EG PHY   |      EC| Port 5 EG CPLDW |     2BC| Port 14 EG DLLP  |
 *           -----------------         |-----------------|        |------------------|
 *                                   F0| Port 0 IN DLLP  |     2C0| Port  1 IN DLLP  |
 *                                   F4| Port 1 IN DLLP  |     2C4| Port  3 IN DLLP  |
 *             Mira                  F8| Port 2 IN DLLP  |     2C8| Port  5 IN DLLP  |
 *     --------------------------    FC| Port 3 IN DLLP  |     2CC| Port  7 IN DLLP  |
 *        14 PCIe counters/port     100| Port 4 IN DLLP  |     2D0| Port  9 IN DLLP  |
 *         4 ports/station          104| Port 5 IN DLLP  |     2D4| Port 11 IN DLLP  |
 *         1 stations (4 ports)        |-----------------|     2D8| Port 13 IN DLLP  |
 *                                  108| Port 0 EG DLLP  |     2DC| Port 15 IN DLLP  |
 *        86 counters/station       10C| Port 1 EG DLLP  |        |------------------|
 *        86 counters (86 * 1)      110| Port 2 EG DLLP  |     2E0| Port  1 EG DLLP  |
 *                                  114| Port 3 EG DLLP  |     2E4| Port  3 EG DLLP  |
 *       off     Counter            118| Port 4 EG DLLP  |     2E8| Port  5 EG DLLP  |
 *           -----------------      11C| Port 5 EG DLLP  |     2EC| Port  7 EG DLLP  |
 *         0| Port 0 IN PH    |        |-----------------|     2F0| Port  9 EG DLLP  |
 *         4| Port 0 IN PDW   |     120| Port 0 IN PHY   |     2F4| Port 11 EG DLLP  |
 *         8| Port 0 IN NPDW  |     124| Port 1 IN PHY   |     2F8| Port 13 EG DLLP  |
 *         C| Port 0 IN CPLH  |     128| Port 2 IN PHY   |     2FC| Port 15 EG DLLP  |
 *        10| Port 0 IN CPLDW |     12C| Port 3 IN PHY   |        |------------------|
 *          |-----------------|     130| Port 4 IN PHY   |     300| Port 0 PHY       |
 *        14| Port 1 IN PH    |     134| Port 5 IN PHY   |        |       :          |
 *          |       :         |        |-----------------|        |       :          |
 *          |       :         |     138| Port 0 EG PHY   |     33C| Port 15 PHY      |
 *        24| Port 1 IN CPLDW |     13C| Port 1 EG PHY   |         ------------------
 *          |-----------------|     140| Port 2 EG PHY   |
 *        28| Port 2 IN PH    |     144| Port 3 EG PHY   |
 *          |       :         |     148| Port 4 EG PHY   |
 *          |       :         |     14C| Port 5 EG PHY   |
 *        38| Port 2 IN CPLDW |         ----------------- 
 *          |-----------------|
 *        3C| Port 3 IN PH    |
 *          |       :         |
 *          |       :         |
 *        4C| Port 3 IN CPLDW |
 *          |-----------------|
 *        50| Port 0 EG PH    |
 *        54| Port 0 EG PDW   |
 *        58| Port 0 EG NPDW  |
 *        5C| Port 0 EG CPLH  |
 *        60| Port 0 EG CPLDW |
 *          |-----------------|
 *        64| Port 1 EG PH    |
 *          |       :         |
 *          |       :         |
 *        74| Port 1 EG CPLDW |
 *          |-----------------|
 *        78| Port 2 EG PH    |
 *          |       :         |
 *          |       :         |
 *        88| Port 2 EG CPLDW |
 *          |-----------------|
 *        8C| Port 3 EG PH    |
 *          |       :         |
 *          |       :         |
 *        9C| Port 3 EG CPLDW |
 *          |-----------------|
 *        A0| Port 0 IN DLLP  |
 *        A4| Port 1 IN DLLP  |
 *        A8| Port 2 IN DLLP  |
 *        AC| Port 3 IN DLLP  |
 *          |-----------------|
 *        B0| Port 0 EG DLLP  |
 *        B4| Port 1 EG DLLP  |
 *        B8| Port 2 EG DLLP  |
 *        BC| Port 3 EG DLLP  |
 *          |-----------------|
 *        C0| GPEP_0 IN PLD   |
 *        C4| GPEP_0 IN RAW   |
 *        C8| GPEP_0 IN PKT   |
 *          |-----------------|
 *        CC| GPEP_1 IN PLD   |
 *        D0| GPEP_1 IN RAW   |
 *        D4| GPEP_1 IN PKT   |
 *          |-----------------|
 *        D8| GPEP_2 IN PLD   |
 *        DC| GPEP_2 IN RAW   |
 *        E0| GPEP_2 IN PKT   |
 *          |-----------------|
 *        E4| GPEP_3 IN PLD   |
 *        E8| GPEP_3 IN RAW   |
 *        EC| GPEP_3 IN PKT   |
 *          |-----------------|
 *        F0| GPEP_0 OUT PLD  |
 *        F4| GPEP_0 OUT RAW  |
 *        F8| GPEP_0 OUT PKT  |
 *          |-----------------|
 *        FC| GPEP_1 OUT PLD  |
 *       100| GPEP_1 OUT RAW  |
 *       104| GPEP_1 OUT PKT  |
 *          |-----------------|
 *       108| GPEP_2 OUT PLD  |
 *       10C| GPEP_2 OUT RAW  |
 *       110| GPEP_2 OUT PKT  |
 *          |-----------------|
 *       114| GPEP_3 OUT PLD  |
 *       118| GPEP_3 OUT RAW  |
 *       11C| GPEP_3 OUT PKT  |
 *          |-----------------|
 *       120| EP_0 IN PLD     |
 *       124| EP_0 IN RAW     |
 *       128| EP_0 IN PKT     |
 *          |-----------------|
 *       12C| EP_0 OUT PLD    |
 *       130| EP_0 OUT RAW    |
 *       134| EP_0 OUT PKT    |
 *          |-----------------|
 *       138| PHY (always 0)  |
 *       13C| PHY (always 0)  |
 *           -----------------
 ******************************************************************************/
PLX_STATUS
PlxPciPerformanceGetCounters(
    PLX_DEVICE_NODE *pdx,
    PLX_PERF_PROP   *pPerfProps,
    U8               NumOfObjects
    )
{
    U8          NumPorts;
    U8          NumCounters;
    U8          PortsPerStation;
    U16         i;
    U16         index;
    U16         IndexBase;
    U16         Offset_Fifo;
    U16         Offset_RamCtrl;
    U32         RegValue;
    U32        *pCounter;
    U32        *pCounter_Prev;
    U32         Counter_PrevTmp[14];
    S64         TmpValue;
    static U32  Counters[336];     // Cygnus currently max (24 ports * 14 counters/port)


    // Setup parameters for reading counters
    switch (pdx->Key.PlxFamily)
    {
        case PLX_FAMILY_DENEB:
            Offset_RamCtrl  = 0x618;
            Offset_Fifo     = 0x628;
            NumPorts        = 12;
            NumCounters     = 14;
            PortsPerStation = 4;
            break;

        case PLX_FAMILY_SIRIUS:
            Offset_RamCtrl  = 0x618;
            Offset_Fifo     = 0x628;
            NumPorts        = 16;
            NumCounters     = 13;
            PortsPerStation = 16;
            break;

        case PLX_FAMILY_CYGNUS:
            Offset_RamCtrl  = 0x3F0;
            Offset_Fifo     = 0x3E4;
            NumPorts        = 24;
            NumCounters     = 14;
            PortsPerStation = 4;
            break;

        case PLX_FAMILY_MIRA:
            Offset_RamCtrl  = 0x618;
            Offset_Fifo     = 0x628;
            NumPorts        = 4;
            NumCounters     = 12;
            PortsPerStation = 4;

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
            NumPorts        = 18;
            NumCounters     = 14;
            PortsPerStation = 6;
            break;

        case PLX_FAMILY_CAPELLA_1:
            Offset_RamCtrl  = 0x3F0;
            Offset_Fifo     = 0x3E4;
            NumPorts        = 24;
            NumCounters     = 14;
            if ((pdx->Key.PlxChip == 0x8714) || (pdx->Key.PlxChip == 0x8718))
                PortsPerStation = 5;
            else
                PortsPerStation = 4;
            break;

        default:
            DebugPrintf(("ERROR - Unsupported PLX chip (%04X)\n", pdx->Key.PlxChip));
            return ApiUnsupportedFunction;
    }

    // RAM control default - Set capture type [5:4] to 2 & enable RAM (bit 0)
    RegValue = (2 << 4) | (1 << 0);

    // Reset RAM read pointer (bit 2)
    PLX_8000_REG_WRITE(
        pdx,
        Offset_RamCtrl,
        RegValue | (1 << 2)
        );

    // Read in all counters
    i = 0;
    while (i < (NumPorts * NumCounters))
    {
        // Get next counter
        Counters[i] =
            PLX_8000_REG_READ(
                pdx,
                Offset_Fifo
                );

        // Jump to next counter
        i++;
    }

    // Assign counter values to enabled ports
    i = 0;
    while (i < NumOfObjects)
    {
        // Make a copy of the previous values before overwriting them
        RtlCopyMemory(
            Counter_PrevTmp,
            &(pPerfProps[i].Prev_IngressPostedHeader),
            14 * sizeof(U32)    // All 14 counters in structure
            );

        // Save current values to previous
        RtlCopyMemory(
            &(pPerfProps[i].Prev_IngressPostedHeader),
            &(pPerfProps[i].IngressPostedHeader),
            14 * sizeof(U32)    // All 14 counters in structure
            );

        // Calculate starting index for counters based on port in station
        IndexBase = pPerfProps[i].Station * (NumCounters * PortsPerStation);

        // Ingress counters start at index 0 from base
        index = IndexBase + 0 + (pPerfProps[i].StationPort * 5);

        // Get Ingress counters (5 DW/port)
        pPerfProps[i].IngressPostedHeader = Counters[index + 0];
        pPerfProps[i].IngressPostedDW     = Counters[index + 1];
        pPerfProps[i].IngressNonpostedDW  = Counters[index + 2];
        pPerfProps[i].IngressCplHeader    = Counters[index + 3];
        pPerfProps[i].IngressCplDW        = Counters[index + 4];

        // Egress counters start after ingress
        index = IndexBase + (5 * PortsPerStation) + (pPerfProps[i].StationPort * 5);

        // Get Egress counters (5 DW/port)
        pPerfProps[i].EgressPostedHeader = Counters[index + 0];
        pPerfProps[i].EgressPostedDW     = Counters[index + 1];
        pPerfProps[i].EgressNonpostedDW  = Counters[index + 2];
        pPerfProps[i].EgressCplHeader    = Counters[index + 3];
        pPerfProps[i].EgressCplDW        = Counters[index + 4];

        // DLLP Ingress counters start after egress
        index = IndexBase + (10 * PortsPerStation);

        // DLLP counter location depends upon chip
        if (pdx->Key.PlxFamily == PLX_FAMILY_SIRIUS)
        {
            // Even port number DLLP counters are first
            index += (pPerfProps[i].StationPort / 2);

            // Odd port number DLLP counters follow even ports
            if (pPerfProps[i].StationPort & (1 << 0))
                index += PortsPerStation;
        }
        else
        {
            index += pPerfProps[i].StationPort;
        }

        // Get DLLP Ingress counters (1 DW/port)
        pPerfProps[i].IngressDllp = Counters[index];

        // Egress DLLP counters follow Ingress
        if (pdx->Key.PlxFamily == PLX_FAMILY_SIRIUS)
        {
            // Even ports are grouped together
            index += (PortsPerStation / 2);
        }
        else
        {
            index += PortsPerStation;
        }

        // Get DLLP Egress counters (1 DW/port)
        pPerfProps[i].EgressDllp = Counters[index];

        // Any PHY counters are always 0, so ignore any values
        pPerfProps[i].IngressPhy = 0;
        pPerfProps[i].EgressPhy  = 0;

        /**********************************************************
         * In some cases on Draco 1 chips, device may incorrectly
         * report a counter as 0. The following code checks the
         * current & previous counters to detect this case. If the
         * issue is present, the previous value is used instead to
         * minimize data reporting errors.
         *********************************************************/
        if ((pdx->Key.PlxFamily == PLX_FAMILY_DRACO_1) &&
            (pPerfProps[i].LinkWidth != 0))
        {
            // Setup initial pointers to stored counters
            pCounter      = &pPerfProps[i].IngressPostedHeader;
            pCounter_Prev = &pPerfProps[i].Prev_IngressPostedHeader;

            // Verify each counter & use previous on error
            for (index = 0; index < 14; index++)
            {
                if (((*pCounter == 0) && (*pCounter_Prev != 0)) || (*pCounter == 0x4C041301))
                {
                    // Store 64-bit counter in case of wrapping
                    TmpValue = *pCounter_Prev;
                    if (*pCounter_Prev < Counter_PrevTmp[index])
                        TmpValue += ((S64)1 << 32);

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

    return ApiSuccess;
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
    pMHProp->SwitchMode = PLX_SWITCH_MODE_STANDARD;

    // Verify supported device
    switch (pdx->Key.PlxFamily)
    {
        case PLX_FAMILY_CYGNUS:
        case PLX_FAMILY_DRACO_1:
        case PLX_FAMILY_DRACO_2:
        case PLX_FAMILY_CAPELLA_1:
            break;

        default:
            DebugPrintf(("ERROR - Device (%04X) doesn't support multi-host\n", pdx->Key.PlxChip));
            return ApiUnsupportedFunction;
    }

    // Attempt to read management port configuration
    RegValue =
        PLX_8000_REG_READ(
            pdx,
            0x354
            );

    // Device properties are only available from the management port
    if (RegValue == 0)
    {
        // In Multi-Host mode, but not management port
        pMHProp->SwitchMode = PLX_SWITCH_MODE_MULTI_HOST;
        DebugPrintf(("Device is in multi-host mode, but not management port\n"));
        return ApiSuccess;
    }

    // Report this is management port regardless of mode
    pMHProp->bIsMgmtPort = TRUE;

    // Store management port info
    pMHProp->MgmtPortNumActive    = (U8)((RegValue >> 0) & 0x1F);
    pMHProp->MgmtPortNumRedundant = (U8)((RegValue >> 8) & 0x1F);

    if (RegValue & (1 << 5))
        pMHProp->bMgmtPortActiveEn = TRUE;

    if (RegValue & (1 << 13))
        pMHProp->bMgmtPortRedundantEn = TRUE;

    // Get active VS mask
    RegVSEnable =
        PLX_8000_REG_READ(
            pdx,
            0x358
            );

    // Provide active VS's
    pMHProp->VS_EnabledMask = (U16)RegVSEnable;

    TotalVS = 0;

    // Count number of active virtual switches
    for (i=0; i<8; i++)
    {
        // Check if VS is active
        if (RegVSEnable & (1 << i))
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
            pMHProp->VS_DownstreamPorts[i] &= ~(1 << pMHProp->VS_UpstreamPortNum[i]);
        }
    }

    // If more than one VS is active, then multi-host mode
    if (TotalVS == 1)
    {
        DebugPrintf(("Device is in standard mode\n"));
    }
    else
    {
        pMHProp->SwitchMode = PLX_SWITCH_MODE_MULTI_HOST;

        DebugPrintf((
            "Mode        : Mult-Host\n"
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

    return ApiSuccess;
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
    status =
        PlxMH_GetProperties(
            pdx,
            &MHProp
            );

    if (status != ApiSuccess)
        return status;

    // Operation only available from Multi-Host management port
    if ((MHProp.SwitchMode != PLX_SWITCH_MODE_MULTI_HOST) || (MHProp.bIsMgmtPort == FALSE))
        return ApiUnsupportedFunction;

    DebugPrintf((
        "Migrate DS ports (%08X) from VS%d ==> VS%d %s\n",
        (int)DsPortMask, VS_Source, VS_Dest,
        (bResetSrc) ? "& reset source port" : ""
        ));

    if ((VS_Source >= 8) || (VS_Dest >= 8))
    {
        DebugPrintf(("ERROR - Source or Dest VS are not valid\n"));
        return ApiInvalidIndex;
    }

    // Verify source VS is enabled
    if ((MHProp.VS_EnabledMask & (1 << VS_Source)) == 0)
    {
        DebugPrintf(("ERROR - Source VS (%d) not enabled\n", VS_Source));
        return ApiDeviceDisabled;
    }

    // Verify DS ports to move currently owned by source port
    if ((MHProp.VS_DownstreamPorts[VS_Source] & DsPortMask) != DsPortMask)
    {
        DebugPrintf(("ERROR - One or more DS ports not owned by source VS\n"));
        return ApiInvalidData;
    }

    // Migrate DS ports
    for (i=0; i<24; i++)
    {
        // Migrate port from source to destination if requested
        if (DsPortMask & (1 << i))
        {
            // Remove port from source
            MHProp.VS_DownstreamPorts[VS_Source] &= ~(1 << i);

            // Add port to destination
            MHProp.VS_DownstreamPorts[VS_Dest] |= (1 << i);
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
    if ((MHProp.VS_EnabledMask & (1 << VS_Dest)) == 0)
    {
        DebugPrintf(("Enable destination VS%d\n", VS_Dest));

        PLX_8000_REG_WRITE(
            pdx,
            0x358,
            MHProp.VS_EnabledMask | (1 << VS_Dest)
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
            RegValue | (1 << VS_Source)
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

    return ApiSuccess;
}
