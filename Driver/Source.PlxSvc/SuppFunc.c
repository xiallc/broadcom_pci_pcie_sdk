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
 *      SuppFunc.c
 *
 * Description:
 *
 *      Additional support functions
 *
 * Revision History:
 *
 *      08-01-13 : PLX SDK v7.10
 *
 ******************************************************************************/


#include <linux/delay.h>
#include "ApiFunc.h"
#include "PciFunc.h"
#include "PciRegs.h"
#include "SuppFunc.h"




/*******************************************************************************
 *
 * Function   :  Plx_sleep
 *
 * Description:  Function as a normal sleep. Parameter is in millisecond
 *
 ******************************************************************************/
VOID
Plx_sleep(
    U32 delay
    )
{
    mdelay(
        delay
        );
}




/*******************************************************************************
 *
 * Function   :  Plx_pow_int
 *
 * Description:  A PLX support routine to compute x^y (integers)
 *
 ******************************************************************************/
U32
Plx_pow_int(
    U32 x,
    U32 y
    )
{
    U32 value;


    // Check for '0' exponent
    if (y == 0)
        return 1;

    // Check for base of '0'
    if (x == 0)
        return 0;

    // Set start value
    value = 1;

    // Perform calculation
    while (y--)
    {
        value = value * x;
    }

    return value;
}




/*******************************************************************************
 *
 * Function   :  PlxGetExtendedCapabilityOffset
 *
 * Description:  Scans the capability list to search for a specific capability
 *
 ******************************************************************************/
U16
PlxGetExtendedCapabilityOffset(
    PLX_DEVICE_NODE *pDevice,
    U16              CapabilityId
    )
{
    U16 Offset_Cap;
    U32 RegValue;


    // Verify PCI header is not Type 2
    if (pDevice->PciHeaderType == 2)
    {
        DebugPrintf(("Device header is PCI Type 2 (Cardbus) - PCI extended capabilities not supported\n"));
        return 0;
    }

    // Get offset of first capability
    PLX_PCI_REG_READ(
        pDevice,
        0x34,           // PCI capabilities pointer
        &RegValue
        );

    // If link is down, PCI reg accesses will fail
    if (RegValue == (U32)-1)
        return 0;

    // Set first capability
    Offset_Cap = (U16)RegValue;

    // Traverse capability list searching for desired ID
    while ((Offset_Cap != 0) && (RegValue != (U32)-1))
    {
        // Get next capability
        PLX_PCI_REG_READ(
            pDevice,
            Offset_Cap,
            &RegValue
            );

        if ((U8)RegValue == (U8)CapabilityId)
        {
            // Capability found, return base offset
            return Offset_Cap;
        }

        // Jump to next capability
        Offset_Cap = (U16)((RegValue >> 8) & 0xFF);
    }

    // Capability not found
    return 0;
}




/*******************************************************************************
 *
 * Function   :  PlxPciBarResourceMap
 *
 * Description:  Maps a PCI BAR resource into kernel space
 *
 ******************************************************************************/
int
PlxPciBarResourceMap(
    PLX_DEVICE_NODE *pDevice,
    U8               BarIndex
    )
{
    PLX_PCI_BAR_PROP BarProp;


    // Verify BAR index
    switch (pDevice->PciHeaderType)
    {
        case 0:
            if ((BarIndex != 0) && (BarIndex > 5))
            {
                DebugPrintf(("Invalid PCI BAR (%d) specified\n", BarIndex));
                return (-1);
            }
            break;

        case 1:
            if ((BarIndex != 0) && (BarIndex != 1))
            {
                DebugPrintf(("PCI BAR %d does not exist on PCI type 1 headers\n", BarIndex));
                return (-1);
            }
            break;

        default:
            DebugPrintf(("PCI Header Type %d is not supported for PCI BAR mappings\n", pDevice->PciHeaderType));
            return (-1);
    }

    // Check if BAR already mapped
    if (pDevice->PciBar[BarIndex].pVa != NULL)
        return 0;

    // Get BAR properties if haven't yet
    if ((pDevice->PciBar[BarIndex].Properties.Flags & PLX_BAR_FLAG_PROBED) == 0)
    {
        if (PlxPciBarProperties(
                pDevice,
                BarIndex,
                &BarProp
                ) != ApiSuccess)
        {
            return (-1);
        }
    }

    // Verify BAR is valid and is memory space
    if ((pDevice->PciBar[BarIndex].Properties.Physical == 0) ||
        (pDevice->PciBar[BarIndex].Properties.Flags & PLX_BAR_FLAG_IO))
        return (-1);

    // Map into kernel virtual space
    pDevice->PciBar[BarIndex].pVa =
        ioremap(
            pDevice->PciBar[BarIndex].Properties.Physical,
            pDevice->PciBar[BarIndex].Properties.Size
            );

    if (pDevice->PciBar[BarIndex].pVa == NULL)
    {
        ErrorPrintf(("ERROR - Unable to map BAR %d to Kernel VA\n", BarIndex));
        return (-1);
    }

    // Flag BARs mapped to kernel space
    pDevice->bBarKernelMap = TRUE;

    DebugPrintf((
        "Mapped BAR %d ==> kernel space (VA=%p)\n",
        BarIndex, pDevice->PciBar[BarIndex].pVa
        ));

    return 0;
}




/*******************************************************************************
 *
 * Function   :  PlxPciBarResourcesUnmap
 *
 * Description:  Unmap all mapped PCI BAR memory for a device
 *
 ******************************************************************************/
int
PlxPciBarResourcesUnmap(
    PLX_DEVICE_NODE *pDevice,
    U8               BarIndex
    )
{
    U8 i;
    U8 MaxBar;


    if (pDevice->PciHeaderType == 0)
        MaxBar = PCI_NUM_BARS_TYPE_00;
    else if (pDevice->PciHeaderType == 1)
        MaxBar = PCI_NUM_BARS_TYPE_01;
    else
        return (-1);

    for (i = 0; i < MaxBar; i++)
    {
        if ((BarIndex == (U8)-1) || (BarIndex == i))
        {
            // Unmap the space from Kernel space if previously mapped
            if (pDevice->PciBar[i].pVa == NULL)
            {
                if (BarIndex != (U8)-1)
                    return (-1);
            }
            else
            {
                DebugPrintf((
                    "Unmap BAR %d from kernel space (VA=%p)\n",
                    i, pDevice->PciBar[i].pVa
                    ));

                // Unmap from kernel space
                iounmap(
                    pDevice->PciBar[i].pVa
                    );

                // Clear BAR information
                pDevice->PciBar[i].pVa = NULL;
            }
        }
    }

    // Flag BARs no longer mapped
    pDevice->bBarKernelMap = FALSE;

    return 0;
}




/*******************************************************************************
 *
 * Function   :  PlxResourcesReleaseAll
 *
 * Description:  Releases resources for any device that has aquired any
 *
 ******************************************************************************/
int
PlxResourcesReleaseAll(
    DEVICE_EXTENSION *pdx
    )
{
    struct list_head *pEntry;
    PLX_DEVICE_NODE  *pDevice;


    pEntry = pdx->List_Devices.next;

    // Traverse list to find the desired object
    while (pEntry != &(pdx->List_Devices))
    {
        // Get the object
        pDevice =
            list_entry(
                pEntry,
                PLX_DEVICE_NODE,
                ListEntry
                );

        if ((pDevice->bBarKernelMap) || (pDevice->MapRequestPending))
        {
            DebugPrintf((
                "Release resources for %p (%04X_%04X [b:%02x s:%02x f:%x])\n",
                pDevice, pDevice->Key.DeviceId, pDevice->Key.VendorId,
                pDevice->Key.bus, pDevice->Key.slot, pDevice->Key.function
                ));
        }

        if (pDevice->MapRequestPending)
        {
            // Remove any pending mapping requests
            PlxUserMappingRequestFreeAll_ByNode(
                pdx,
                pDevice
                );
        }

        if (pDevice->bBarKernelMap == TRUE)
        {
            // Unmap any kernel mappings to PCI BAR spaces
            PlxPciBarResourcesUnmap(
                pDevice,
                (U8)-1          // Specify to unmap all PCI BARs
                );
        }

        // Jump to next entry
        pEntry = pEntry->next;
    }

    return 0;
}




/*******************************************************************************
 *
 * Function   :  PlxUserMappingRequestFreeAll_ByNode
 *
 * Description:  Removes any pending mapping requests assigned to a specific node
 *
 ******************************************************************************/
VOID
PlxUserMappingRequestFreeAll_ByNode(
    DEVICE_EXTENSION *pdx,
    PLX_DEVICE_NODE  *pDevice
    )
{
    struct list_head *pEntry;
    PLX_USER_MAPPING *pMapObject;


    spin_lock(
        &(pdx->Lock_MapParamsList)
        );

    // Find the mapping parameters from the call to mmap
    pEntry = pdx->List_MapParams.next;

    // Traverse list to find the desired list objects
    while (pEntry != &(pdx->List_MapParams))
    {
        // Get the object
        pMapObject =
            list_entry(
                pEntry,
                PLX_USER_MAPPING,
                ListEntry
                );

        if (pMapObject->pDevice == pDevice)
        {
            // Decrement request counter
            pDevice->MapRequestPending--;

            // Remove the object from the list
            list_del(
                pEntry
                );

            DebugPrintf((
                "Removed request (%p) to map to PCI BAR %d\n",
                pMapObject, pMapObject->BarIndex
                ));

            // Release the object
            kfree(
                pMapObject
                );

            // Restart parsing the list from the beginning
            pEntry = pdx->List_MapParams.next;
        }
        else
        {
            // Jump to next item in the list
            pEntry = pEntry->next;
        }
    }

    spin_unlock(
        &(pdx->Lock_MapParamsList)
        );
}




/*******************************************************************************
 *
 * Function   :  PlxDeviceListBuild
 *
 * Description:  Scan the PCI Bus for PCI devices
 *
 ******************************************************************************/
U8
PlxDeviceListBuild(
    DRIVER_OBJECT *pDriverObject
    )
{
    U8                DeviceCount;
    U16               offset;
    U32               RegValue;
    struct pci_dev   *pPciDev;
    PLX_DEVICE_NODE  *pDevice;
    struct list_head *pEntry;
    DEVICE_EXTENSION *pdx;


    DebugPrintf(("Scan for devices...\n"));

    // Clear device count
    DeviceCount = 0;

    // Get device extension
    pdx = pDriverObject->DeviceObject->DeviceExtension;

    // Get OS-generated list of PCI devices
    pPciDev =
        Plx_pci_get_device(
            PCI_ANY_ID,
            PCI_ANY_ID,
            NULL
            );

    while (pPciDev)
    {
        DebugPrintf((
            "Add - %04X %04X  [b:%02x s:%02x f:%x]\n",
            pPciDev->device, pPciDev->vendor,
            pPciDev->bus->number, PCI_SLOT(pPciDev->devfn),
            PCI_FUNC(pPciDev->devfn)
            ));

        // Increment number of devices found
        DeviceCount++;

        // Allocate memory for new node
        pDevice =
            kmalloc(
                sizeof(PLX_DEVICE_NODE),
                GFP_KERNEL
                );

        // Clear node
        RtlZeroMemory(
            pDevice,
            sizeof(PLX_DEVICE_NODE)
            );

        // Fill device structure with the PCI information
        pDevice->pPciDevice      = pPciDev;
        pDevice->Key.bus         = pPciDev->bus->number;
        pDevice->Key.slot        = PCI_SLOT(pPciDev->devfn);
        pDevice->Key.function    = PCI_FUNC(pPciDev->devfn);
        pDevice->Key.VendorId    = pPciDev->vendor;
        pDevice->Key.DeviceId    = pPciDev->device;
        pDevice->Key.SubVendorId = pPciDev->subsystem_vendor;
        pDevice->Key.SubDeviceId = pPciDev->subsystem_device;

        // Set API access mode
        pDevice->Key.ApiMode = PLX_API_MODE_PCI;

        // Save parent device object
        pDevice->pdx = pdx;

        // Get PCI Class code & Revision
        PlxPciRegisterRead_BypassOS(
            pDevice->Key.bus,
            pDevice->Key.slot,
            pDevice->Key.function,
            0x08,
            &RegValue
            );

        pDevice->Key.Revision = (U8)RegValue;
        pDevice->PciClass     = (RegValue >> 8);

        // Get PCI header type
        PlxPciRegisterRead_BypassOS(
            pDevice->Key.bus,
            pDevice->Key.slot,
            pDevice->Key.function,
            0x0c,
            &RegValue
            );

        // Set header type field
        pDevice->PciHeaderType = (U8)((RegValue >> 16) & 0x7F);

        // BIOS may not enable PLX devices so do it manually
        if (pDevice->Key.VendorId == PLX_VENDOR_ID)
        {
            PLX_PCI_REG_READ(
                pDevice,
                0x04,
                &RegValue
                );

            if ((RegValue & 0x7) == 0)
            {
                // Enable device but don't clear any PCI error status
                RegValue = (RegValue & (~0x1F << 27)) | 0x7;

                PLX_PCI_REG_WRITE(
                    pDevice,
                    0x04,
                    RegValue
                    );
            }
        }

        // For Upstream/Downstream port, get Subsystem ID from capability
        if ((pDevice->Key.VendorId == PLX_VENDOR_ID) && (pDevice->PciHeaderType == 1))
        {
            offset = PlxGetExtendedCapabilityOffset(pDevice, CAP_ID_BRIDGE_SUB_ID);

            if (offset != 0)
            {
                PLX_PCI_REG_READ( pDevice, offset + 0x04, &RegValue );
                pDevice->Key.SubVendorId = (U16)(RegValue >> 0);
                pDevice->Key.SubDeviceId = (U16)(RegValue >> 16);
            }
        }

        // Add node to device list
        list_add_tail(
            &(pDevice->ListEntry),
            &(pdx->List_Devices)
            );

        // Probe the PCI BAR spaces, mainly to check for 64-bit properties
        PlxProbePciBarSpaces( pDevice );

        // Set PLX chip version
        PlxChipTypeDetect(
            pDevice,
            FALSE
            );

        // Set port number as unknown
        pDevice->PortNumber = (U8)-1;

        // Set default EEPROM width (for 8111/8112 series)
        pDevice->Default_EepWidth = 2;

        // Jump to next device
        pPciDev =
            Plx_pci_get_device(
                PCI_ANY_ID,
                PCI_ANY_ID,
                pPciDev     // Ref count will be decremented for this device
                );
    }

    // Get device list
    pEntry = pdx->List_Devices.next;

    // Assign parent for each device
    while (pEntry != &(pdx->List_Devices))
    {
        // Get the object
        pDevice =
            list_entry(
                pEntry,
                PLX_DEVICE_NODE,
                ListEntry
                );

        // Find & assign parent P2P device
        PlxAssignParentDevice(
            pdx,
            pDevice
            );

        // Set register access device back to itself
        pDevice->pRegNode = pDevice;

        // For some 8000 devices, upstream port must be used for reg access
        if (((pDevice->Key.PlxChip & 0xFF00) == 0x2300) ||
            ((pDevice->Key.PlxChip & 0xFF00) == 0x3300) ||
            ((pDevice->Key.PlxChip & 0xFF00) == 0x8500) ||
            ((pDevice->Key.PlxChip & 0xFF00) == 0x8600) ||
            ((pDevice->Key.PlxChip & 0xFF00) == 0x8700))
        {
            // Read BAR 0
            PLX_PCI_REG_READ(
                pDevice,
                0x10,       // BAR 0
                &RegValue
                );

            // If BAR 0 not enabled, use parent upstream port
            if (RegValue == 0)
                pDevice->pRegNode = pDevice->pParent;
        }

        // For 6000 & 8000 NT ports, determine NT port side
        if (((pDevice->Key.PlxChip & 0xF000) == 0x6000) ||
            ((pDevice->Key.PlxChip & 0xFF00) == 0x8500) ||
            ((pDevice->Key.PlxChip & 0xFF00) == 0x8600) ||
            ((pDevice->Key.PlxChip & 0xFF00) == 0x8700))
        {
            if ((pDevice->PciHeaderType == 0) && (pDevice->PciClass == 0x068000))
                PlxDetermineNtPortSide( pDevice );
        }

        // Default to unknown mode
        pDevice->Key.DeviceMode = PLX_PORT_UNKNOWN;

        // For Mira USB, determine whether this is in Legacy mode (USB root EP) or
        // Enhanced Mode (PCIe Switch + USB EP). Some features not available in Enhanced mode.
        if (pDevice->Key.PlxFamily == PLX_FAMILY_MIRA)
        {
            if (pDevice->PciHeaderType == 1)
            {
                // For US/DS ports, device must be in Enhanced mode
                pDevice->Key.DeviceMode = PLX_PORT_ENDPOINT;
            }
            else
            {
                // For EP, 90h[11] will report mode (1=Enhanced/0=Legacy)
                RegValue = PlxRegisterRead( pDevice, 0x90, NULL, FALSE );
                if (RegValue & (1 << 11))
                    pDevice->Key.DeviceMode = PLX_PORT_ENDPOINT;
                else
                    pDevice->Key.DeviceMode = PLX_PORT_LEGACY_ENDPOINT;
            }

            DebugPrintf((
                "MIRA Device @ [b:%02x s:%02x] in %s mode\n",
                pDevice->Key.bus, pDevice->Key.slot,
                (pDevice->Key.DeviceMode == PLX_PORT_ENDPOINT) ? "Enhanced" : "Legacy"
                ));
        }

        // Jump to next entry
        pEntry = pEntry->next;
    }

    DebugPrintf((
        "Device Scan: %d device(s) found\n",
        DeviceCount
        ));

    return DeviceCount;
}




/*******************************************************************************
 *
 * Function   :  PlxAssignParentDevice
 *
 * Description:  Traverse device list and return the node associated by key
 *
 ******************************************************************************/
PLX_DEVICE_NODE *
PlxAssignParentDevice(
    DEVICE_EXTENSION *pdx,
    PLX_DEVICE_NODE  *pDevice
    )
{
    U8                OwnedBus;
    U32               RegValue;
    PLX_DEVICE_NODE  *pCurrNode;
    struct list_head *pEntry;


    OwnedBus = 0x0;

    pEntry = pdx->List_Devices.next;

    // Traverse list to find the desired object
    while (pEntry != &(pdx->List_Devices))
    {
        // Get the object
        pCurrNode =
            list_entry(
                pEntry,
                PLX_DEVICE_NODE,
                ListEntry
                );

        // If this is a P2P device, get its owned buses
        if (pCurrNode->PciHeaderType == 1)
        {
            PLX_PCI_REG_READ(
                pCurrNode,
                0x18,       // Primary/Secondary/Subordinate bus numbers
                &RegValue
                );

            // Get secondary bus
            OwnedBus = (U8)(RegValue >> 8);
        }

        // Check if device exists on owned bus
        if (pDevice->Key.bus == OwnedBus)
        {
            pDevice->pParent = pCurrNode;
            return pCurrNode;
        }

        // Jump to next entry
        pEntry = pEntry->next;
    }

    return NULL;
}




/*******************************************************************************
 *
 * Function   :  PlxProbePciBarSpaces
 *
 * Description:  Probe the PCI BARs of a device & set properties
 *
 ******************************************************************************/
VOID
PlxProbePciBarSpaces(
    PLX_DEVICE_NODE *pdx
    )
{
    U8       i;
    U8       NumBars;
    U32      PciBar;
    BOOLEAN  bBarUpper64;


    // Set max BAR index
    switch (pdx->PciHeaderType)
    {
        case 0:
            NumBars = 6;
            break;

        case 1:
            NumBars = 2;
            break;

        default:
            // PCI Type 2 not supported
            DebugPrintf(("NOTE: Probe of PCI Type 2 headers (CardBus) not supported\n"));
            return;
    }

    bBarUpper64 = FALSE;

    // Probe the BARs in order
    for (i=0; i<NumBars; i++)
    {
        // Read PCI BAR
        PLX_PCI_REG_READ(
            pdx,
            0x10 + (i * sizeof(U32)),
            &PciBar
            );

        // Handle upper 32-bits of 64-bit BAR
        if (bBarUpper64)
        {
            // Put upper 32-bit address in previous BAR
            pdx->PciBar[i-1].Properties.BarValue |= ((U64)PciBar << 32);

            // Mark as upper 32-bit
            pdx->PciBar[i].Properties.Flags |= PLX_BAR_FLAG_UPPER_32 | PLX_BAR_FLAG_MEM;

            // Set BAR as already probed so size not checked in future
            pdx->PciBar[i].Properties.Flags |= PLX_BAR_FLAG_PROBED;

            // Reset upper 64-bit flag
            bBarUpper64 = FALSE;
        }
        else
        {
            // Do nothing if BAR is disabled
            if (PciBar == 0)
                continue;

            // Store BAR value
            pdx->PciBar[i].Properties.BarValue = PciBar;

            // Set BAR properties
            pdx->PciBar[i].Properties.Physical = pci_resource_start( pdx->pPciDevice, i );
            pdx->PciBar[i].Properties.Size     = pci_resource_len( pdx->pPciDevice, i );

            // Set BAR flags
            if (PciBar & (1 << 0))
                pdx->PciBar[i].Properties.Flags = PLX_BAR_FLAG_IO;
            else
                pdx->PciBar[i].Properties.Flags = PLX_BAR_FLAG_MEM;

            // Set BAR flags
            if (pdx->PciBar[i].Properties.Flags & PLX_BAR_FLAG_MEM)
            {
                if (((PciBar >> 1) & 3) == 0)
                    pdx->PciBar[i].Properties.Flags |= PLX_BAR_FLAG_32_BIT;
                else if (((PciBar >> 1) & 3) == 1)
                    pdx->PciBar[i].Properties.Flags |= PLX_BAR_FLAG_BELOW_1MB;
                else if (((PciBar >> 1) & 3) == 2)
                    pdx->PciBar[i].Properties.Flags |= PLX_BAR_FLAG_64_BIT;

                if (PciBar & (1 << 3))
                    pdx->PciBar[i].Properties.Flags |= PLX_BAR_FLAG_PREFETCHABLE;
            }

            // If 64-bit memory BAR, set flag for next BAR
            if (pdx->PciBar[i].Properties.Flags & PLX_BAR_FLAG_64_BIT)
                bBarUpper64 = TRUE;
        }
    }
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
    PLX_DEVICE_NODE *pDevice
    )
{
    U8  *pRegVa;
    U32  RegPci;
    U32  RegSave;
    U32  RegPciBar0;
    U32  RegExpected;


    // Default to non-NT port
    pDevice->Key.NTPortType = PLX_NT_PORT_NONE;

    // Check for DMA controller
    if (pDevice->Key.function >= 1)
        return TRUE;

    // Check for 6000-NT device
    if (((pDevice->Key.PlxChip & 0xF000) == 0x6000) &&
         (pDevice->PciHeaderType == 0))
    {
        if (pDevice->Key.DeviceId & (1 << 0))
            pDevice->Key.NTPortType = PLX_NT_PORT_PRIMARY;
        else
            pDevice->Key.NTPortType = PLX_NT_PORT_SECONDARY;

        return TRUE;
    }

    // Get BAR 0
    PLX_PCI_REG_READ(
        pDevice,
        0x10,       // BAR 0
        &RegPciBar0
        );

    // Check if BAR 0 exists
    if (RegPciBar0 == 0)
    {
        // If BAR 0 not enabled, this is 8500 virtual-side
        pDevice->Key.NTPortType   = PLX_NT_PORT_VIRTUAL;
        pDevice->Offset_NtRegBase = 0x10000;
    }
    else
    {
        // Verify access to internal registers
        if (pDevice->pRegNode == NULL)
        {
            DebugPrintf(("Error: Register access not setup\n"));
            return FALSE;
        }

        // Set NT base offset
        switch (pDevice->Key.PlxFamily)
        {
            case PLX_FAMILY_SCOUT:
            case PLX_FAMILY_DRACO_2:
            case PLX_FAMILY_CAPELLA_1:
                // Read NT ID register
                PLX_PCI_REG_READ( pDevice, 0xC8C, &RegPci );

                // Check which NT port
                if (RegPci & (1 << 0))
                    pDevice->Offset_NtRegBase = 0x3C000;   // NT 1
                else
                    pDevice->Offset_NtRegBase = 0x3E000;   // NT 0

                // Determine NT Virtual or Link
                if (RegPci & (1 << 31))
                    pDevice->Key.NTPortType = PLX_NT_PORT_LINK;
                else
                    pDevice->Key.NTPortType = PLX_NT_PORT_VIRTUAL;
                break;

            case PLX_FAMILY_DRACO_1:
                // NT ID register not implemented, revert to probe algorithm
                pDevice->Offset_NtRegBase = 0x3E000;
                break;

            case PLX_FAMILY_CYGNUS:
                pDevice->Offset_NtRegBase = 0x3E000;
                break;

            default:
                if (((pDevice->Key.PlxChip & 0xFF00) == 0x8500) ||
                    ((pDevice->Key.PlxChip & 0xFF00) == 0x8600))
                    pDevice->Offset_NtRegBase = 0x10000;
                else
                {
                    ErrorPrintf(("ERROR: NT detection not implemented for %04X\n", pDevice->Key.PlxChip));
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
        if (pDevice->Key.NTPortType == PLX_NT_PORT_NONE)
        {
            // Default to NT Virtual side
            pDevice->Key.NTPortType = PLX_NT_PORT_VIRTUAL;

            // Get current BAR 0 kernel virtual address
            pRegVa = pDevice->pRegNode->PciBar[0].pVa;

            // Get PCI interrupt register through memory-mapped BAR 0
            if (pRegVa == NULL)
            {
                // Clear non-address bits
                RegPciBar0 &= ~0xF;

                // Set register offset
                pRegVa = PLX_INT_TO_PTR(RegPciBar0 + pDevice->Offset_NtRegBase + 0x3C);

                // Store original value
                RegSave = (U32)PlxPhysicalMemRead( pRegVa, sizeof(U32) );

                // Replace lower byte with FEh
                RegExpected = (RegSave & ~((U32)0xFF)) | 0xFE;

                // Write expected value
                PlxPhysicalMemWrite( pRegVa, RegExpected, sizeof(U32) );

                // Some chips have an internal latency when updating a
                //  mem-mapped register to PCI config space. Some dummy
                //  register reads are used to account for the latency.
                PlxPhysicalMemRead( pRegVa, sizeof(U32) );
                PlxPhysicalMemRead( pRegVa, sizeof(U32) );

                // Read register through PCI config cycle (bypass OS)
                PlxPciRegisterRead_BypassOS(
                    pDevice->Key.bus,
                    pDevice->Key.slot,
                    pDevice->Key.function,
                    0x3C,
                    &RegPci
                    );

                // Restore original value
                PlxPhysicalMemWrite( pRegVa, RegSave, sizeof(U32) );
            }
            else
            {
                // Set register offset
                pRegVa = pRegVa + pDevice->Offset_NtRegBase + 0x3C;

                // Store original value
                RegSave = PHYS_MEM_READ_32( (U32*)pRegVa );

                // Replace lower byte with FEh
                RegExpected = (RegSave & ~((U32)0xFF)) | 0xFE;

                // Write expected value
                PHYS_MEM_WRITE_32( (U32*)pRegVa, RegExpected );

                // Read register through PCI config cycle (bypass OS)
                PlxPciRegisterRead_BypassOS(
                    pDevice->Key.bus,
                    pDevice->Key.slot,
                    pDevice->Key.function,
                    0x3C,
                    &RegPci
                    );

                // Restore original value
                PHYS_MEM_WRITE_32( (U32*)pRegVa, RegSave );
            }

            // If PCI register does not match expected value, port is link side
            if (RegPci != RegExpected)
                pDevice->Key.NTPortType = PLX_NT_PORT_LINK;
        }
    }

    // Adjust offset for NT Link port
    if (pDevice->Key.NTPortType == PLX_NT_PORT_LINK)
        pDevice->Offset_NtRegBase += 0x1000;

    DebugPrintf((
        "%04X NT @ [b:%02x s:%02x] is %s-side (NT base=%Xh)\n",
        pDevice->Key.PlxChip, pDevice->Key.bus, pDevice->Key.slot,
        (pDevice->Key.NTPortType == PLX_NT_PORT_LINK) ? "Link" : "Virtual",
        (int)pDevice->Offset_NtRegBase
        ));

    return TRUE;
}




/*******************************************************************************
 *
 * Function   :  GetDeviceNodeFromKey
 *
 * Description:  Traverse device list and return the node associated by key
 *
 ******************************************************************************/
PLX_DEVICE_NODE *
GetDeviceNodeFromKey(
    DEVICE_EXTENSION *pdx,
    PLX_DEVICE_KEY   *pKey
    )
{
    struct list_head *pEntry;
    PLX_DEVICE_NODE  *pDevice;


    pEntry = pdx->List_Devices.next;

    // Traverse list to find the desired object
    while (pEntry != &(pdx->List_Devices))
    {
        // Get the object
        pDevice =
            list_entry(
                pEntry,
                PLX_DEVICE_NODE,
                ListEntry
                );

        if ((pDevice->Key.bus      == pKey->bus) &&
            (pDevice->Key.slot     == pKey->slot) &&
            (pDevice->Key.function == pKey->function))
        {
            return pDevice;
        }

        // Jump to next entry
        pEntry = pEntry->next;
    }

    return NULL;
}




/*******************************************************************************
 *
 * Function   :  PlxChipTypeDetect
 *
 * Description:  Attempts to determine PLX chip type and revision
 *
 ******************************************************************************/
BOOLEAN
PlxChipTypeDetect(
    PLX_DEVICE_NODE *pDevice,
    BOOLEAN          bOnlySetFamily
    )
{
    U8  i;
    U16 offset[] = {0xE0,0x958,0xB78,0xF8,0x0};
    U16 DeviceId;
    U32 RegValue;


    // Jump to set family if requested
    if (bOnlySetFamily)
        goto _PlxChipAssignFamily;

    // Default revision to PCI revision
    pDevice->Key.PlxRevision = pDevice->Key.Revision;

    i = 0;

    while (offset[i] != 0)
    {
        // Check for hard-coded ID
        PLX_PCI_REG_READ(
            pDevice,
            offset[i],
            &RegValue
            );

        if ((RegValue & 0xFFFF) == PLX_VENDOR_ID)
        {
            pDevice->Key.PlxChip = (U16)(RegValue >> 16);

            // Some chips do not have updated hard-coded revision ID
            if ((pDevice->Key.PlxChip != 0x8612) &&
                (pDevice->Key.PlxChip != 0x8616) &&
                (pDevice->Key.PlxChip != 0x8624) &&
                (pDevice->Key.PlxChip != 0x8632) &&
                (pDevice->Key.PlxChip != 0x8647) &&
                (pDevice->Key.PlxChip != 0x8648))
            {
                // PLX revision should be in next register
                PLX_PCI_REG_READ(
                    pDevice,
                    offset[i] + sizeof(U32),
                    &RegValue
                    );

                pDevice->Key.PlxRevision = (U8)(RegValue & 0xFF);
            }

            // Skip to assigning family
            goto _PlxChipAssignFamily;
        }

        // Go to next offset
        i++;
    }

    // Get current device ID
    DeviceId = pDevice->Key.DeviceId;

    // Generalize for PLX 8000 devices
    if (pDevice->Key.VendorId == PLX_VENDOR_ID)
    {
        switch (DeviceId & 0xFF00)
        {
            case 0x2300:
            case 0x3300:
            case 0x8500:
            case 0x8600:
            case 0x8700:
            case 0x8100:
                // Don't include 8311 RDK
                if (DeviceId != 0x86e1)
                    DeviceId = 0x8000;
                break;
        }
    }

    // Hard-coded ID doesn't exist, so use Device/Vendor ID
    switch (((U32)DeviceId << 16) | pDevice->Key.VendorId)
    {
        case 0x800010B5:        // All 8000 series devices
            pDevice->Key.PlxChip = pDevice->Key.DeviceId;
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
            if (pDevice->PciHeaderType == 0)
            {
                pDevice->Key.PlxChip = 0x6254;
            }
            else
            {
                // Get 6152 VPD register
                PLX_PCI_REG_READ(
                    pDevice,
                    0xA0,
                    &RegValue
                    );

                if ((RegValue & 0xF) == 0x03)    // CAP_ID_VPD
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
    PlxChipRevisionDetect(
        pDevice
        );

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
                pDevice->Key.PlxFamily = PLX_FAMILY_DRACO_1;
            else
                pDevice->Key.PlxFamily = PLX_FAMILY_SCOUT;
            break;

        case 0x8712:
        case 0x8716:
        case 0x8723:
        case 0x8724:
        case 0x8732:
        case 0x8747:
        case 0x8748:
            if (pDevice->Key.PlxRevision == 0xAA)
                pDevice->Key.PlxFamily = PLX_FAMILY_DRACO_1;
            else
                pDevice->Key.PlxFamily = PLX_FAMILY_DRACO_2;
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

        case 0x8715:
        case 0x8719:
        case 0x8735:
        case 0x8751:
        case 0x8765:
        case 0x8781:
        case 0x8797:
            pDevice->Key.PlxFamily = PLX_FAMILY_CAPELLA_2;
            break;

        case 0:
            pDevice->Key.PlxFamily = PLX_FAMILY_NONE;
            break;

        default:
            DebugPrintf(("ERROR - PLX Family not set for %04X\n", pDevice->Key.PlxChip));
            pDevice->Key.PlxFamily = PLX_FAMILY_UNKNOWN;
            break;
    }

    return TRUE;
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
                pDevice->Key.PlxRevision = 0xAA;
            else if (pDevice->Key.PlxRevision == 0x20)
                pDevice->Key.PlxRevision = 0xBA;
            else if (pDevice->Key.PlxRevision == 0x21)
                pDevice->Key.PlxRevision = 0xBB;
            break;

        case 0x8112:
            pDevice->Key.PlxRevision = 0xAA;
            break;

        case 0x9050:
            if (pDevice->Key.PlxRevision == 0x2)
                pDevice->Key.PlxRevision = 2;
            else
                pDevice->Key.PlxRevision = 1;
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
                pDevice->Key.PlxRevision = 0xBB;
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
                pDevice->Key.PlxRevision = 0xBB;
            break;

        case 0x6350:
            if (pDevice->Key.PlxRevision == 0x20)
                pDevice->Key.PlxRevision = 0xAA;
            break;

        case 0x6156:
            if (pDevice->Key.PlxRevision == 0x1)
                pDevice->Key.PlxRevision = 0xDA;
            break;

        case 0x6254:
            if (pDevice->Key.PlxRevision == 0x4)
                pDevice->Key.PlxRevision = 0xBB;
            break;

        case 0x6520:
            if (pDevice->Key.PlxRevision == 0x2)
                pDevice->Key.PlxRevision = 0xBB;
            break;

        case 0x6540:
            if (pDevice->Key.PlxRevision == 0x2)
                pDevice->Key.PlxRevision = 0xBB;
            break;
    }
}
