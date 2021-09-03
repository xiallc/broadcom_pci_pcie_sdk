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
 *      SuppFunc.c
 *
 * Description:
 *
 *      Additional support functions
 *
 * Revision History:
 *
 *      01-01-18 : PLX SDK v8.00
 *
 ******************************************************************************/


#include <linux/delay.h>        // For mdelay
#include <linux/slab.h>         // For kmalloc/kfree
#include "ApiFunc.h"
#include "ChipFunc.h"
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
    mdelay( delay );
}




/*******************************************************************************
 *
 * Function   :  PlxPciFindCapability
 *
 * Description:  Scans a device's capability list to find the base offset of the
 *               specified PCI or PCIe extended capability. If the capability ID
 *               is PCI or PCIe vendor-specific (VSEC), an optional instance
 *               number can be provided since a device could have multiple VSEC
 *               capabilities. A value of 0 returns the 1st matching instance but
 *               the parameter is ignored for non-VSEC capability search.
 *
 ******************************************************************************/
U16
PlxPciFindCapability(
    PLX_DEVICE_NODE *pDevice,
    U16              CapID,
    U8               bPCIeCap,
    U8               InstanceNum
    )
{
    U8  matchCount;
    U16 offset;
    U16 currID;
    U16 probeLimit;
    U32 regVal;


    // Verify PCI header is not Type 2
    if ((pDevice->PciHeaderType != PCI_HDR_TYPE_0) &&
        (pDevice->PciHeaderType != PCI_HDR_TYPE_1))
    {
        ErrorPrintf((
            "Error: PCI header type (%d) not 0 or 1\n",
            pDevice->PciHeaderType
            ));
        return 0;
    }

    // Get PCI command register (04h)
    PLX_PCI_REG_READ( pDevice, PCI_REG_CMD_STAT, &regVal );

    // Verify device responds to PCI accesses (in case link down)
    if (regVal == (U32)-1)
    {
        return 0;
    }

    // Verify device supports extended capabilities (04h[20])
    if ((regVal & ((U32)1 << 20)) == 0)
    {
        return 0;
    }

    // Set capability pointer offset
    if (bPCIeCap)
    {
        // PCIe capabilities must start at 100h
        offset = PCIE_REG_CAP_PTR;

        // Ignore instance number for non-VSEC capabilities
        if (CapID != PCIE_CAP_ID_VENDOR_SPECIFIC)
        {
            InstanceNum = 0;
        }
    }
    else
    {
        // Get offset of first capability from capability pointer (34h[7:0])
        PLX_PCI_REG_READ( pDevice, PCI_REG_CAP_PTR, &regVal );

        // Set first capability offset
        offset = (U8)regVal;

        // Ignore instance number for non-VSEC capabilities
        if (CapID != PCI_CAP_ID_VENDOR_SPECIFIC)
        {
            InstanceNum = 0;
        }
    }

    // Start with 1st match
    matchCount = 0;

    // Set max probe limit
    probeLimit = 100;

    // Traverse capability list searching for desired ID
    while ((offset != 0) && (regVal != (U32)-1))
    {
        // Get next capability
        PLX_PCI_REG_READ( pDevice, offset, &regVal );

        // Verify capability is valid
        if ((regVal == 0) || (regVal == (U32)-1))
        {
            return 0;
        }

        // Some chipset pass 100h+ to PCI devices which decode
        // back to 0-FFh, so check if 100h matches Dev/Ven ID(0)
        if ((offset == PCIE_REG_CAP_PTR) &&
            (regVal == (((U32)pDevice->Key.DeviceId << 16) |
                              pDevice->Key.VendorId)))
        {
            return 0;
        }

        // Extract the capability ID
        if (bPCIeCap)
        {
            // PCIe ID in [15:0]
            currID = (U16)((regVal >> 0) & 0xFFFF);
        }
        else
        {
            // PCI ID in [7:0]
            currID = (U16)((regVal >> 0) & 0xFF);
        }

        // Compare with desired capability
        if (currID == CapID)
        {
            // Verify correct instance
            if (InstanceNum == matchCount)
            {
                // Capability found, return base offset
                return offset;
            }

            // Increment count of matches
            matchCount++;
        }

        // Enforce an upper bound on probe in case device has issues
        probeLimit--;
        if (probeLimit == 0)
        {
            return 0;
        }

        // Jump to next capability
        if (bPCIeCap)
        {
            // PCIe next cap offset in [31:20]
            offset = (U16)((regVal >> 20) & 0xFFF);
        }
        else
        {
            // PCI next cap offset in [15:8]
            offset = (U8)((regVal >> 8) & 0xFF);
        }
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
        case PCI_HDR_TYPE_0:
            if ((BarIndex != 0) && (BarIndex > 5))
            {
                DebugPrintf(("Invalid PCI BAR (%d) specified\n", BarIndex));
                return (-1);
            }
            break;

        case PCI_HDR_TYPE_1:
            if ((BarIndex != 0) && (BarIndex != 1))
            {
                DebugPrintf(("PCI BAR %d does not exist on PCI type 1 headers\n", BarIndex));
                return (-1);
            }
            break;

        default:
            DebugPrintf(("PCI Header Type %d is not supported\n", pDevice->PciHeaderType));
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
                ) != PLX_STATUS_OK)
        {
            return (-1);
        }
    }

    // Verify BAR is valid and is memory space
    if ((pDevice->PciBar[BarIndex].Properties.Physical == 0) ||
        (pDevice->PciBar[BarIndex].Properties.Size == 0) ||
        (pDevice->PciBar[BarIndex].Properties.Flags & PLX_BAR_FLAG_IO))
    {
        return (-1);
    }

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


    if (pDevice->PciHeaderType == PCI_HDR_TYPE_0)
    {
        MaxBar = PCI_NUM_BARS_TYPE_00;
    }
    else if (pDevice->PciHeaderType == PCI_HDR_TYPE_1)
    {
        MaxBar = PCI_NUM_BARS_TYPE_01;
    }
    else
    {
        return (-1);
    }

    for (i = 0; i < MaxBar; i++)
    {
        if ((BarIndex == (U8)-1) || (BarIndex == i))
        {
            // Unmap the space from Kernel space if previously mapped
            if (pDevice->PciBar[i].pVa != NULL)
            {
                DebugPrintf((
                    "Unmap BAR %d from kernel space (VA=%p)\n",
                    i, pDevice->PciBar[i].pVa
                    ));
                iounmap( pDevice->PciBar[i].pVa );
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
                "Release resources for %p (%04X_%04X [D%d %02X:%02X.%X])\n",
                pDevice, pDevice->Key.DeviceId, pDevice->Key.VendorId,
                pDevice->Key.domain, pDevice->Key.bus,
                pDevice->Key.slot, pDevice->Key.function
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
U16
PlxDeviceListBuild(
    DRIVER_OBJECT *pDriverObject
    )
{
    U16               DeviceCount;
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
        pci_get_device(
            PCI_ANY_ID,
            PCI_ANY_ID,
            NULL
            );

    while (pPciDev)
    {
        // Verify non-VF device still responds before adding to list
        if (Plx_pcie_is_virtfn( pPciDev ) == FALSE)
        {
            pci_read_config_dword( pPciDev, PCI_REG_DEV_VEN_ID, &RegValue );
            if ( (RegValue == 0) || (RegValue == (U32)-1) )
            {
                DebugPrintf((
                    "SKIP: %04X %04X [D%d %02X:%02X.%X] - Non-responsive\n",
                    pPciDev->device, pPciDev->vendor,
                    pci_domain_nr(pPciDev->bus),
                    pPciDev->bus->number, PCI_SLOT(pPciDev->devfn),
                    PCI_FUNC(pPciDev->devfn)
                    ));
                // Continue to next device
                pPciDev = pci_get_device( PCI_ANY_ID, PCI_ANY_ID, pPciDev );
                continue;
            }
        }

        DebugPrintf((
            "Add: %04X %04X [D%d %02X:%02X.%X]\n",
            pPciDev->device, pPciDev->vendor,
            pci_domain_nr(pPciDev->bus),
            pPciDev->bus->number, PCI_SLOT(pPciDev->devfn),
            PCI_FUNC(pPciDev->devfn)
            ));

        // Increment number of devices found
        DeviceCount++;

        // Allocate memory for new node
        pDevice = kmalloc( sizeof(PLX_DEVICE_NODE), GFP_KERNEL );

        // Clear node
        RtlZeroMemory( pDevice, sizeof(PLX_DEVICE_NODE) );

        // Fill device structure with the PCI information
        pDevice->pPciDevice      = pPciDev;
        pDevice->Key.domain      = pci_domain_nr(pPciDev->bus);
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
        pDevice->pdo = pdx;

        // Get PCI Class code & Revision (08h)
        PLX_PCI_REG_READ( pDevice, PCI_REG_CLASS_REV, &RegValue );
        pDevice->Key.Revision = (U8)RegValue;
        pDevice->PciClass     = (RegValue >> 8);

        // Get PCI header type (0Ch)
        PLX_PCI_REG_READ( pDevice, PCI_REG_HDR_CACHE_LN, &RegValue );
        pDevice->PciHeaderType = (U8)((RegValue >> 16) & 0x7F);

        // BIOS may not enable PLX devices so do it manually
        if (pDevice->Key.VendorId == PLX_PCI_VENDOR_ID_PLX)
        {
            PLX_PCI_REG_READ( pDevice, PCI_REG_CMD_STAT, &RegValue );
            if ((RegValue & 0x7) == 0)
            {
                // Enable device but don't clear any PCI error status
                RegValue = (RegValue & (~0x1F << 27)) | 0x7;
                PLX_PCI_REG_WRITE( pDevice, PCI_REG_CMD_STAT, RegValue );
            }
        }

        // Set subsystem ID offset
        if (pDevice->PciHeaderType == PCI_HDR_TYPE_0)
        {
            offset = PCI_REG_TO_SUBSYS_ID;
        }
        else if (pDevice->PciHeaderType == PCI_HDR_TYPE_1)
        {
            // For PCI type 1, get Subsystem ID from capability
            offset =
                PlxPciFindCapability(
                    pDevice,
                    PCI_CAP_ID_BRIDGE_SUB_ID,
                    FALSE,
                    0
                    );
            if (offset != 0)
            {
                offset += 0x04;
            }
        }
        else
        {
            offset = 0;
        }

        // Get subsystem ID
        if ((pDevice->Key.SubVendorId == 0) && (offset != 0))
        {
            PLX_PCI_REG_READ( pDevice, offset, &RegValue );
            pDevice->Key.SubVendorId = (U16)(RegValue >> 0);
            pDevice->Key.SubDeviceId = (U16)(RegValue >> 16);
        }

        // Add node to device list
        list_add_tail(
            &(pDevice->ListEntry),
            &(pdx->List_Devices)
            );

        // Probe the PCI BAR spaces, mainly to check for 64-bit properties
        PlxProbePciBarSpaces( pDevice );

        // Set PLX chip version
        PlxChipTypeDetect( pDevice, FALSE );

        // Flag port properties as not yet probed
        pDevice->PortProp.PortType = PLX_PORT_UNKNOWN;

        // Set default EEPROM width (for 8111/8112 series)
        pDevice->Default_EepWidth = 2;

        // OS may place switch ports in low power state (D3)
        if ( (pDevice->Key.PlxChip != 0) &&
             (pDevice->PciHeaderType == PCI_HDR_TYPE_1) )
        {
            // Get power management capability
            offset =
                PlxPciFindCapability(
                    pDevice,
                    PCI_CAP_ID_POWER_MAN,
                    FALSE,
                    0
                    );
            if (offset != 0)
            {
                // Ensure PM state is D0 (PM 04h[1:0]=0)
                PLX_PCI_REG_READ( pDevice, offset + 0x4, &RegValue );
                if ((RegValue & 0x3) != PCI_CAP_PM_STATE_D0)
                {
                    RegValue &= ~(U32)0x3;
                    RegValue |= (PCI_CAP_PM_STATE_D0 << 0);
                    PLX_PCI_REG_WRITE( pDevice, offset + 0x4, RegValue );
                }
            }
        }

        // Jump to next device
        pPciDev =
            pci_get_device(
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
        PlxAssignParentDevice( pdx, pDevice );

        // For PLX chips, get port properties & set PLX-specific port type
        if (pDevice->Key.PlxChip)
        {
            // Get port properties
            PlxGetPortProperties( pDevice, &pDevice->PortProp );

            // Setup for future internal register access
            PlxSetupRegisterAccess( pDevice );

            // Determine additional port type details
            PlxDeterminePlxPortType( pDevice );
        }

        // Default to unknown mode
        pDevice->Key.DeviceMode = PLX_PORT_UNKNOWN;

        // For Mira USB, determine whether this is in Legacy mode (USB root EP) or
        // Enhanced Mode (PCIe Switch + USB EP). Some features not available in Enhanced mode.
        if (pDevice->Key.PlxFamily == PLX_FAMILY_MIRA)
        {
            if (pDevice->PciHeaderType == PCI_HDR_TYPE_1)
            {
                // For US/DS ports, device must be in Enhanced mode
                pDevice->Key.DeviceMode = PLX_PORT_ENDPOINT;
            }
            else
            {
                // For EP, 90h[11] will report mode (1=Enhanced/0=Legacy)
                RegValue = PlxRegisterRead( pDevice, 0x90, NULL, FALSE );
                if (RegValue & (1 << 11))
                {
                    pDevice->Key.DeviceMode = PLX_PORT_ENDPOINT;
                }
                else
                {
                    pDevice->Key.DeviceMode = PLX_PORT_LEGACY_ENDPOINT;
                }
            }

            DebugPrintf((
                "MIRA Device @ [%02x:%02x] in %s mode\n",
                pDevice->Key.bus, pDevice->Key.slot,
                (pDevice->Key.DeviceMode == PLX_PORT_ENDPOINT) ? "Enhanced" : "Legacy"
                ));
        }

        // Jump to next entry
        pEntry = pEntry->next;
    }

    DebugPrintf(("   --------------------\n"));
    DebugPrintf((
        "Added: %d device%s\n",
        DeviceCount, (DeviceCount > 1) ? "s" : ""
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
        if (pCurrNode->PciHeaderType == PCI_HDR_TYPE_1)
        {
            PLX_PCI_REG_READ(
                pCurrNode,
                PCI_REG_T1_PRIM_SEC_BUS,
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
        case PCI_HDR_TYPE_0:
            NumBars = PCI_NUM_BARS_TYPE_00;
            break;

        case PCI_HDR_TYPE_1:
            NumBars = PCI_NUM_BARS_TYPE_01;
            break;

        default:
            // PCI Type 2 not supported
            DebugPrintf(("NOTE: Probe of PCI Type 2 headers (CardBus) not supported\n"));
            return;
    }

    bBarUpper64 = FALSE;

    // Probe the BARs in order
    for (i = 0; i < NumBars; i++)
    {
        // Read PCI BAR
        PLX_PCI_REG_READ(
            pdx,
            PCI_REG_BAR_0 + (i * sizeof(U32)),
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
            {
                continue;
            }

            // Store BAR value
            pdx->PciBar[i].Properties.BarValue = PciBar;

            // Set BAR properties
            pdx->PciBar[i].Properties.Physical = pci_resource_start( pdx->pPciDevice, i );
            pdx->PciBar[i].Properties.Size     = pci_resource_len( pdx->pPciDevice, i );

            if (PciBar & (1 << 0))
            {
                pdx->PciBar[i].Properties.Flags = PLX_BAR_FLAG_IO;
            }
            else
            {
                pdx->PciBar[i].Properties.Flags = PLX_BAR_FLAG_MEM;
            }

            // Set BAR flags
            if (pdx->PciBar[i].Properties.Flags & PLX_BAR_FLAG_MEM)
            {
                if (((PciBar >> 1) & 3) == 0)
                {
                    pdx->PciBar[i].Properties.Flags |= PLX_BAR_FLAG_32_BIT;
                }
                else if (((PciBar >> 1) & 3) == 1)
                {
                    pdx->PciBar[i].Properties.Flags |= PLX_BAR_FLAG_BELOW_1MB;
                }
                else if (((PciBar >> 1) & 3) == 2)
                {
                    pdx->PciBar[i].Properties.Flags |= PLX_BAR_FLAG_64_BIT;
                }

                if (PciBar & (1 << 3))
                {
                    pdx->PciBar[i].Properties.Flags |= PLX_BAR_FLAG_PREFETCHABLE;
                }

                /**************************************************************
                 * In case of PLX chips, since BAR 0 always used for register
                 * access, do not allow prefetchable flag to avoid future
                 * virtual mappings of the BAR as write-combinable.
                 *************************************************************/
                if ((i == 0) && pdx->Key.PlxChip)
                {
                    pdx->PciBar[i].Properties.Flags &= ~PLX_BAR_FLAG_PREFETCHABLE;
                }
            }

            // If 64-bit memory BAR, set flag for next BAR
            if (pdx->PciBar[i].Properties.Flags & PLX_BAR_FLAG_64_BIT)
            {
                bBarUpper64 = TRUE;
            }
        }
    }
}




/******************************************************************************
 *
 * Function   :  PlxDeterminePlxPortType
 *
 * Description:  Determines the PLX-specific port type
 *
 ******************************************************************************/
BOOLEAN
PlxDeterminePlxPortType(
    PLX_DEVICE_NODE *pDevice
    )
{
    U8  *pRegVa;
    U32  RegPci;
    U32  RegSave;
    U32  RegPciBar0;
    U32  RegExpected;


    // Default to unknown type
    pDevice->Key.PlxPortType = PLX_SPEC_PORT_UNKNOWN;

    // Type 1 PCI devices
    if (pDevice->PciHeaderType == PCI_HDR_TYPE_1)
    {
        if ((pDevice->Key.PlxFamily == PLX_FAMILY_BRIDGE_PCI_P2P) ||
            (pDevice->Key.PlxFamily == PLX_FAMILY_BRIDGE_PCIE_P2P))
        {
            pDevice->Key.PlxPortType = PLX_SPEC_PORT_P2P_BRIDGE;
        }
        else
        {
            // Update port properties if haven't yet
            if (pDevice->PortProp.PortType == PLX_PORT_UNKNOWN)
            {
                PlxGetPortProperties( pDevice, &pDevice->PortProp );
            }

            // Differentiate between UP/DS
            if (pDevice->PortProp.PortType == PLX_PORT_UPSTREAM)
            {
                pDevice->Key.PlxPortType = PLX_SPEC_PORT_UPSTREAM;
            }
            else if (pDevice->PortProp.PortType == PLX_PORT_DOWNSTREAM)
            {
                pDevice->Key.PlxPortType = PLX_SPEC_PORT_DOWNSTREAM;
            }
        }

        return TRUE;
    }

    //
    // Check various endpoint types
    //

    if ((pDevice->Key.DeviceId == 0x1009) ||
        (pDevice->Key.SubDeviceId == 0x1009))
    {
        // GEP
        pDevice->Key.PlxPortType = PLX_SPEC_PORT_GEP;
    }
    else if (pDevice->Key.SubDeviceId == 0x1005)
    {
        // Synthetic NIC VF
        pDevice->Key.PlxPortType = PLX_SPEC_PORT_SYNTH_NIC;
    }
    else if (pDevice->Key.SubDeviceId == 0x1004)
    {
        // Synthetic TWC EP
        pDevice->Key.PlxPortType = PLX_SPEC_PORT_SYNTH_TWC;
    }
    else if (pDevice->Key.SubDeviceId == 0x2004)
    {
        // Synthetic NT 2.0 EP
        pDevice->Key.PlxPortType = PLX_SPEC_PORT_SYNTH_NT;
    }
    else if (pDevice->Key.SubDeviceId == 0x2005)
    {
        // Synthetic gDMA EP
        pDevice->Key.PlxPortType = PLX_SPEC_PORT_SYNTH_GDMA;
    }
    else if ((pDevice->Key.DeviceId == 0x1008) ||
             (pDevice->Key.DeviceId == 0x02AB) ||
             (pDevice->Key.DeviceId == 0x02B2))
    {
        // Synthetic Enabler EP
        pDevice->Key.PlxPortType = PLX_SPEC_PORT_SYNTH_EN_EP;
    }
    else if ( ((pDevice->Key.DeviceId & 0xFF00) == 0xC000) &&
              ((pDevice->Key.SubDeviceId == 0x00B2) ||
               (pDevice->Key.SubDeviceId == 0x0032)) )  // Incorrect ID on FPGA
    {
        // MPT EP without SES support
        pDevice->Key.PlxPortType = PLX_SPEC_PORT_MPT_NO_SES;
    }
    else if (pDevice->Key.DeviceId == 0x00B2)
    {
        // Search for a PCI vendor-specific capability
        RegPci =
            PlxPciFindCapability(
                pDevice,
                PCI_CAP_ID_VENDOR_SPECIFIC,
                FALSE,
                0
                );

        // MPT SAS controller, synthetic if has PCI VSEC capability
        if (RegPci == 0)
        {
            pDevice->Key.PlxPortType = PLX_SPEC_PORT_MPT;
        }
        else
        {
            pDevice->Key.PlxPortType = PLX_SPEC_PORT_SYNTH_MPT;
        }
    }
    else if (pDevice->Key.PlxFamily == PLX_FAMILY_BRIDGE_PCI_P2P)
    {
        // 6000-NT EP
        if (pDevice->Key.DeviceId & (1 << 0))
        {
            pDevice->Key.PlxPortType = PLX_SPEC_PORT_NT_VIRTUAL;
        }
        else
        {
            pDevice->Key.PlxPortType = PLX_SPEC_PORT_NT_LINK;
        }
    }
    else if (pDevice->Key.PlxFamily == PLX_FAMILY_BRIDGE_P2L)
    {
        // 9000/8311 EPs
        pDevice->Key.PlxPortType = PLX_SPEC_PORT_LEGACY_EP;
    }
    else if ((pDevice->PciClass == 0x088000) && (pDevice->Key.function >= 1))
    {
        // Legacy DMA controller
        pDevice->Key.PlxPortType = PLX_SPEC_PORT_DMA;
    }

    // Return if already determined
    if (pDevice->Key.PlxPortType != PLX_SPEC_PORT_UNKNOWN)
    {
        return TRUE;
    }


    /*******************************************
     * EP below this point can only be PLX NT
     ******************************************/

    // Get BAR 0
    PLX_PCI_REG_READ(
        pDevice,
        PCI_REG_BAR_0,
        &RegPciBar0
        );

    // Check if BAR 0 exists
    if (RegPciBar0 == 0)
    {
        // If BAR 0 not enabled, this is 8500 virtual-side
        pDevice->Key.PlxPortType  = PLX_SPEC_PORT_NT_VIRTUAL;
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
            case PLX_FAMILY_VEGA:
            case PLX_FAMILY_VEGA_LITE:
            case PLX_FAMILY_DENEB:
            case PLX_FAMILY_SIRIUS:
                pDevice->Offset_NtRegBase = 0x10000;
                break;

            case PLX_FAMILY_DRACO_1:
                // NT ID register not implemented, revert to probe algorithm
                pDevice->Offset_NtRegBase = 0x3E000;
                break;

            case PLX_FAMILY_CYGNUS:
                pDevice->Offset_NtRegBase = 0x3E000;
                break;

            case PLX_FAMILY_SCOUT:
            case PLX_FAMILY_DRACO_2:
            case PLX_FAMILY_CAPELLA_1:
            case PLX_FAMILY_CAPELLA_2:
                // Read NT ID register
                PLX_PCI_REG_READ( pDevice, 0xC8C, &RegPci );

                // Check which NT port
                if (RegPci & (1 << 0))
                {
                    pDevice->Offset_NtRegBase = 0x3C000;   // NT 1
                }
                else
                {
                    pDevice->Offset_NtRegBase = 0x3E000;   // NT 0
                }

                // Determine NT Virtual or Link
                if (RegPci & (1 << 31))
                {
                    pDevice->Key.PlxPortType = PLX_SPEC_PORT_NT_LINK;
                }
                else
                {
                    pDevice->Key.PlxPortType = PLX_SPEC_PORT_NT_VIRTUAL;
                }
                break;

            default:
                ErrorPrintf(("ERROR: NT detection not implemented for %04X\n", pDevice->Key.PlxChip));
                return FALSE;
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
        if (pDevice->Key.PlxPortType == PLX_SPEC_PORT_UNKNOWN)
        {
            // Default to NT Virtual side
            pDevice->Key.PlxPortType = PLX_SPEC_PORT_NT_VIRTUAL;

            // Get current BAR 0 kernel virtual address
            pRegVa = pDevice->pRegNode->PciBar[0].pVa;

            // Get PCI interrupt register through memory-mapped BAR 0
            if (pRegVa == NULL)
            {
                // Clear non-address bits
                RegPciBar0 &= ~0xF;

                // Set register offset
                pRegVa = PLX_INT_TO_PTR(RegPciBar0 + pDevice->Offset_NtRegBase + PCI_REG_INT_PIN_LN);

                // Store original value
                RegSave = (U32)PlxPhysicalMemRead( PLX_PTR_TO_INT(pRegVa), sizeof(U32) );

                // Replace lower byte with FEh
                RegExpected = (RegSave & ~((U32)0xFF)) | 0xFE;

                // Write expected value
                PlxPhysicalMemWrite( PLX_PTR_TO_INT(pRegVa), RegExpected, sizeof(U32) );

                // Some chips have an internal latency when updating a
                //  mem-mapped register to PCI config space. Some dummy
                //  register reads are used to account for the latency.
                PlxPhysicalMemRead( PLX_PTR_TO_INT(pRegVa), sizeof(U32) );
                PlxPhysicalMemRead( PLX_PTR_TO_INT(pRegVa), sizeof(U32) );

                // Read register through PCI config cycle (bypass OS)
                PlxPciRegisterRead_BypassOS(
                    pDevice->Key.domain,
                    pDevice->Key.bus,
                    pDevice->Key.slot,
                    pDevice->Key.function,
                    PCI_REG_INT_PIN_LN,
                    &RegPci
                    );

                // Restore original value
                PlxPhysicalMemWrite( PLX_PTR_TO_INT(pRegVa), RegSave, sizeof(U32) );
            }
            else
            {
                // Set register offset
                pRegVa = pRegVa + pDevice->Offset_NtRegBase + PCI_REG_INT_PIN_LN;

                // Store original value
                RegSave = PHYS_MEM_READ_32( (U32*)pRegVa );

                // Replace lower byte with FEh
                RegExpected = (RegSave & ~((U32)0xFF)) | 0xFE;

                // Write expected value
                PHYS_MEM_WRITE_32( (U32*)pRegVa, RegExpected );

                // Read register through PCI config cycle (bypass OS)
                PlxPciRegisterRead_BypassOS(
                    pDevice->Key.domain,
                    pDevice->Key.bus,
                    pDevice->Key.slot,
                    pDevice->Key.function,
                    PCI_REG_INT_PIN_LN,
                    &RegPci
                    );

                // Restore original value
                PHYS_MEM_WRITE_32( (U32*)pRegVa, RegSave );
            }

            // If PCI register does not match expected value, port is link side
            if (RegPci != RegExpected)
            {
                pDevice->Key.PlxPortType = PLX_SPEC_PORT_NT_LINK;
            }
        }
    }

    // Adjust offset for NT Link port
    if (pDevice->Key.PlxPortType == PLX_SPEC_PORT_NT_LINK)
    {
        pDevice->Offset_NtRegBase += 0x1000;
    }

    DebugPrintf((
        "[D%d %02X:%02X.0] %04X NT is %s-side (NT base=%Xh)\n",
        pDevice->Key.domain, pDevice->Key.bus, pDevice->Key.slot,
        pDevice->Key.PlxChip,
        (pDevice->Key.PlxPortType == PLX_SPEC_PORT_NT_LINK) ? "LINK" : "VIRTUAL",
        (int)pDevice->Offset_NtRegBase
        ));

    return TRUE;
}




/******************************************************************************
 *
 * Function   :  PlxSetupRegisterAccess
 *
 * Description:  Sets up the device object for future internal register access
 *
 ******************************************************************************/
BOOLEAN
PlxSetupRegisterAccess(
    PLX_DEVICE_NODE *pdx
    )
{
    PLX_DEVICE_NODE *pdxParent;


    // Update port properties if haven't yet
    if (pdx->PortProp.PortType == PLX_PORT_UNKNOWN)
    {
        PlxGetPortProperties( pdx, &pdx->PortProp );
    }

    // Default register access device back to itself
    pdx->pRegNode = pdx;

    // Nothing else to do for non-PCIe devices or have BAR 0
    if ( (pdx->PortProp.bNonPcieDevice == TRUE) ||
         (pdx->PciBar[0].Properties.BarValue != 0) )
    {
        // If GEP, then fabric mode, so update upstream port to GEP for BAR 0
        if (pdx->Key.DeviceId == 0x1009)
        {
            if ( (pdx->Key.PlxFamily == PLX_FAMILY_CAPELLA_2) ||
                 (pdx->Key.PlxFamily == PLX_FAMILY_ATLAS) )
            {
                if (pdx->pParent && pdx->pParent->pParent)
                {
                    // GEP -> DS -> UP
                    pdx->pParent->pParent->pRegNode = pdx;
                }
            }
        }
        return TRUE;
    }

    //
    // For PCIe switch devices, first upstream port or GEP (fabric mode) BAR 0 is used
    //

    // Start at current device
    pdxParent = pdx;

    do
    {
        // Verify parent is valid
        if (pdxParent->pParent == NULL)
        {
            return FALSE;
        }

        // If parent is different chip, halt traversal & use current device for access
        if (pdx->Key.PlxChip != pdxParent->pParent->Key.PlxChip)
        {
            pdx->pRegNode = pdxParent->pRegNode;
            return FALSE;
        }

        // Go to next parent device
        pdxParent = pdxParent->pParent;

        // If parent has BAR 0, use for access
        if (pdxParent->PciBar[0].Properties.BarValue != 0)
        {
            pdx->pRegNode = pdxParent;
            return TRUE;
        }

        // If parent has already determined its access, copy it.
        if ( (pdxParent->pRegNode != NULL) &&
             (pdxParent->pRegNode->PciBar[0].Properties.BarValue != 0) )
        {
            pdx->pRegNode = pdxParent->pRegNode;
            return TRUE;
        }
    }
    while (pdxParent);

    ErrorPrintf((
        "ERROR: No register access node found for [D%d %02X:%02X.%X]\n",
        pdx->Key.domain, pdx->Key.bus, pdx->Key.slot, pdx->Key.function
        ));
    return FALSE;
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

        if ( (pDevice->Key.domain   == pKey->domain) &&
             (pDevice->Key.bus      == pKey->bus)    &&
             (pDevice->Key.slot     == pKey->slot)   &&
             (pDevice->Key.function == pKey->function) )
        {
            return pDevice;
        }

        // Jump to next entry
        pEntry = pEntry->next;
    }

    return NULL;
}
