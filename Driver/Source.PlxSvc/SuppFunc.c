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
 *      09-01-10 : PLX SDK v6.40
 *
 ******************************************************************************/


#include <linux/delay.h>
#include "ApiFunc.h"
#include "PciFunc.h"
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
    PLX_DEVICE_NODE *pNode,
    U16              CapabilityId
    )
{
    U16 Offset_Cap;
    U32 RegValue;


    // Verify PCI header is not Type 2
    if (pNode->PciHeaderType == 2)
    {
        DebugPrintf(("Device header is PCI Type 2 (Cardbus) - PCI extended capabilities not supported\n"));
        return 0;
    }

    // Get offset of first capability
    PLX_PCI_REG_READ(
        pNode,
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
            pNode,
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
    PLX_DEVICE_NODE *pNode,
    U8               BarIndex
    )
{
    PLX_PCI_BAR_PROP BarProp;


    // Verify BAR index
    switch (pNode->PciHeaderType)
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
            DebugPrintf(("PCI Header Type %d is not supported for PCI BAR mappings\n", pNode->PciHeaderType));
            return (-1);
    }

    // Check if BAR already mapped
    if (pNode->PciBar[BarIndex].pVa != NULL)
    {
        return 0;
    }

    DebugPrintf((
        "Mapping PCI BAR %d to kernel space...\n",
        BarIndex
        ));

    // Get BAR properties
    if (PlxPciBarProperties(
            pNode,
            BarIndex,
            &BarProp
            ) != ApiSuccess)
    {
        return (-1);
    }

    // Verify BAR is valid and is memory space
    if ((BarProp.Physical == 0) || (BarProp.bIoSpace))
    {
        return (-1);
    }

    // Store BAR properties
    pNode->PciBar[BarIndex].Properties = BarProp;

    // Map into kernel virtual space
    pNode->PciBar[BarIndex].pVa =
        ioremap(
            pNode->PciBar[BarIndex].Properties.Physical,
            pNode->PciBar[BarIndex].Properties.Size
            );

    if (pNode->PciBar[BarIndex].pVa == NULL)
    {
        ErrorPrintf(("    Kernel VA: ERROR - Unable to map space to Kernel VA\n"));
        return (-1);
    }

    // Flag BARs mapped to kernel space
    pNode->bBarKernelMap = TRUE;

    DebugPrintf((
        "    Kernel VA: %p\n",
        pNode->PciBar[BarIndex].pVa
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
    PLX_DEVICE_NODE *pNode,
    U8               BarIndex
    )
{
    U8 i;
    U8 MaxBar;


    if (pNode->PciHeaderType == 0)
        MaxBar = PCI_NUM_BARS_TYPE_00;
    else if (pNode->PciHeaderType == 1)
        MaxBar = PCI_NUM_BARS_TYPE_01;
    else
        return (-1);

    for (i = 0; i < MaxBar; i++)
    {
        if ((BarIndex == (U8)-1) || (BarIndex == i))
        {
            // Unmap the space from Kernel space if previously mapped
            if (pNode->PciBar[i].pVa == NULL)
            {
                if (BarIndex != (U8)-1)
                    return (-1);
            }
            else
            {
                DebugPrintf((
                    "Unmap BAR %d from kernel space (VA=%p)\n",
                    i, pNode->PciBar[i].pVa
                    ));

                // Unmap from kernel space
                iounmap(
                    pNode->PciBar[i].pVa
                    );

                // Clear BAR information
                RtlZeroMemory(
                    &(pNode->PciBar[i]),
                    sizeof(PLX_PCI_BAR_INFO)
                    );
            }
        }
    }

    // Flag BARs no longer mapped
    pNode->bBarKernelMap = FALSE;

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
    PLX_DEVICE_NODE  *pNode;


    pEntry = pdx->List_Devices.next;

    // Traverse list to find the desired object
    while (pEntry != &(pdx->List_Devices))
    {
        // Get the object
        pNode =
            list_entry(
                pEntry,
                PLX_DEVICE_NODE,
                ListEntry
                );

        if ((pNode->bBarKernelMap) || (pNode->MapRequestPending))
        {
            DebugPrintf((
                "Release resources for %p (%04X_%04X [b:%02x s:%02x f:%02x])\n",
                pNode, pNode->Key.DeviceId, pNode->Key.VendorId,
                pNode->Key.bus, pNode->Key.slot, pNode->Key.function
                ));
        }

        if (pNode->MapRequestPending)
        {
            // Remove any pending mapping requests
            PlxUserMappingRequestFreeAll_ByNode(
                pdx,
                pNode
                );
        }

        if (pNode->bBarKernelMap == TRUE)
        {
            // Unmap any kernel mappings to PCI BAR spaces
            PlxPciBarResourcesUnmap(
                pNode,
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
    PLX_DEVICE_NODE  *pNode
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

        if (pMapObject->pNode == pNode)
        {
            // Decrement request counter
            pNode->MapRequestPending--;

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
    U32               RegValue;
    struct pci_dev   *pPciDev;
    PLX_DEVICE_NODE  *pNode;
    struct list_head *pEntry;
    DEVICE_EXTENSION *pdx;


    DebugPrintf(("Scanning PCI bus for devices...\n"));

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
            "Adding - %04X %04X  [b:%02x s:%02x f:%02x]\n",
            pPciDev->device, pPciDev->vendor,
            pPciDev->bus->number, PCI_SLOT(pPciDev->devfn),
            PCI_FUNC(pPciDev->devfn)
            ));

        // Increment number of devices found
        DeviceCount++;

        // Allocate memory for new node
        pNode =
            kmalloc(
                sizeof(PLX_DEVICE_NODE),
                GFP_KERNEL
                );

        // Clear node
        RtlZeroMemory(
            pNode,
            sizeof(PLX_DEVICE_NODE)
            );

        // Fill device structure with the PCI information
        pNode->pPciDevice      = pPciDev;
        pNode->Key.bus         = pPciDev->bus->number;
        pNode->Key.slot        = PCI_SLOT(pPciDev->devfn);
        pNode->Key.function    = PCI_FUNC(pPciDev->devfn);
        pNode->Key.VendorId    = pPciDev->vendor;
        pNode->Key.DeviceId    = pPciDev->device;
        pNode->Key.SubVendorId = pPciDev->subsystem_vendor;
        pNode->Key.SubDeviceId = pPciDev->subsystem_device;

        // Save parent device object
        pNode->pdx = pdx;

        // Get PCI Revision
        PlxPciRegisterRead_BypassOS(
            pNode->Key.bus,
            pNode->Key.slot,
            pNode->Key.function,
            0x08,
            &RegValue
            );

        pNode->Key.Revision = (U8)RegValue;

        // Get PCI header type
        PlxPciRegisterRead_BypassOS(
            pNode->Key.bus,
            pNode->Key.slot,
            pNode->Key.function,
            0x0c,
            &RegValue
            );

        // Set header type field
        pNode->PciHeaderType = (U8)((RegValue >> 16) & 0x7F);

        // Add node to device list
        list_add_tail(
            &(pNode->ListEntry),
            &(pdx->List_Devices)
            );

        // Set PLX chip version
        PlxChipTypeDetect(
            pNode
            );

        // Set port number as unknown
        pNode->PortNumber = (U8)-1;

        // Set default EEPROM width (for 8111/8112 series)
        pNode->Default_EepWidth = 2;

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
        pNode =
            list_entry(
                pEntry,
                PLX_DEVICE_NODE,
                ListEntry
                );

        // Find & assign parent P2P device
        PlxAssignParentDevice(
            pdx,
            pNode
            );

        // Set register access device back to itself
        pNode->pRegNode = pNode;

        // For some 8000 devices, upstream port must be used for reg access
        if (((pNode->Key.PlxChip & 0xFF00) == 0x8400) ||
            ((pNode->Key.PlxChip & 0xFF00) == 0x8500) ||
            ((pNode->Key.PlxChip & 0xFF00) == 0x8600) ||
            ((pNode->Key.PlxChip & 0xFF00) == 0x8700))
        {
            // Read BAR 0
            PLX_PCI_REG_READ(
                pNode,
                0x10,       // BAR 0
                &RegValue
                );

            // If BAR 0 not enabled, use parent upstream port
            if (RegValue == 0)
                pNode->pRegNode = pNode->pParent;
        }

        // For 6000 & 8000 NT ports, determine NT port side
        if (((pNode->Key.PlxChip & 0xF000) == 0x6000) ||
            ((pNode->Key.PlxChip & 0xFF00) == 0x8500) ||
            ((pNode->Key.PlxChip & 0xFF00) == 0x8600) ||
            ((pNode->Key.PlxChip & 0xFF00) == 0x8700))
        {
            if ((pNode->PciHeaderType == 0) && (pNode->Key.function == 0))
                PlxDetermineNtPortSide( pNode );
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
    PLX_DEVICE_NODE  *pNode
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
        if (pNode->Key.bus == OwnedBus)
        {
            pNode->pParent = pCurrNode;
            return pCurrNode;
        }

        // Jump to next entry
        pEntry = pEntry->next;
    }

    return NULL;
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
    PLX_DEVICE_NODE *pNode
    )
{
    U8  *pRegVa;
    U32  RegPci;
    U32  RegSave;
    U32  RegPciBar0;
    U32  RegExpected;


    // Default to non-NT port
    pNode->Key.NTPortType = PLX_NT_PORT_NONE;

    // Check for DMA controller
    if (pNode->Key.function >= 1)
        return TRUE;

    // Check for 6000-NT device
    if (((pNode->Key.PlxChip & 0xF000) == 0x6000) &&
         (pNode->PciHeaderType == 0))
    {
        if (pNode->Key.DeviceId & (1 << 0))
            pNode->Key.NTPortType = PLX_NT_PORT_PRIMARY;
        else
            pNode->Key.NTPortType = PLX_NT_PORT_SECONDARY;

        return TRUE;
    }

    // Get BAR 0
    PLX_PCI_REG_READ(
        pNode,
        0x10,       // BAR 0
        &RegPciBar0
        );

    // Check if BAR 0 exists
    if (RegPciBar0 == 0)
    {
        // If BAR 0 not enabled, this is 8500 virtual-side
        pNode->Key.NTPortType   = PLX_NT_PORT_VIRTUAL;
        pNode->Offset_NtRegBase = 0x10000;
    }
    else
    {
        // Verify access to internal registers
        if (pNode->pRegNode == NULL)
        {
            DebugPrintf(("Error: Register access not setup\n"));
            return FALSE;
        }

        // Set NT base offset
        switch (pNode->Key.PlxFamily)
        {
            case PLX_FAMILY_SCOUT:
            case PLX_FAMILY_DRACO_2:
                // Read NT ID register
                PLX_PCI_REG_READ( pNode, 0xC8C, &RegPci );

                // Check which NT port
                if (RegPci & (1 << 0))
                    pNode->Offset_NtRegBase = 0x3C000;   // NT 1
                else
                    pNode->Offset_NtRegBase = 0x3E000;   // NT 0

                // Determine NT Virtual or Link
                if (RegPci & (1 << 31))
                    pNode->Key.NTPortType = PLX_NT_PORT_LINK;
                else
                    pNode->Key.NTPortType = PLX_NT_PORT_VIRTUAL;
                break;

            case PLX_FAMILY_DRACO_1:
                // NT ID register not implemented, revert to probe algorithm
                pNode->Offset_NtRegBase = 0x3E000;
                break;

            case PLX_FAMILY_CYGNUS:
                pNode->Offset_NtRegBase = 0x3E000;
                break;

            default:
                pNode->Offset_NtRegBase = 0x10000;
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
        if (pNode->Key.NTPortType == PLX_NT_PORT_NONE)
        {
            // Default to NT Virtual side
            pNode->Key.NTPortType = PLX_NT_PORT_VIRTUAL;

            // Get current BAR 0 kernel virtual address
            pRegVa = pNode->pRegNode->PciBar[0].pVa;

            // Get PCI interrupt register through memory-mapped BAR 0
            if (pRegVa == NULL)
            {
                // Clear non-address bits
                RegPciBar0 &= ~0xF;

                // Set register offset
                pRegVa = PLX_INT_TO_PTR(RegPciBar0 + pNode->Offset_NtRegBase + 0x3C);

                // Store original value
                RegSave = (U32)PlxPhysicalMemRead( pRegVa, sizeof(U32) );

                // Replace lower byte with FEh
                RegExpected = (RegSave & ~((U32)0xFF)) | 0xFE;

                // Write expected value
                PlxPhysicalMemWrite( pRegVa, RegExpected, sizeof(U32) );

                // Read register through PCI config cycle (bypass OS)
                PlxPciRegisterRead_BypassOS(
                    pNode->Key.bus,
                    pNode->Key.slot,
                    pNode->Key.function,
                    0x3C,
                    &RegPci
                    );

                // Restore original value
                PlxPhysicalMemWrite( pRegVa, RegSave, sizeof(U32) );
            }
            else
            {
                // Set register offset
                pRegVa = pRegVa + pNode->Offset_NtRegBase + 0x3C;

                // Store original value
                RegSave = PHYS_MEM_READ_32( (U32*)pRegVa );

                // Replace lower byte with FEh
                RegExpected = (RegSave & ~((U32)0xFF)) | 0xFE;

                // Write expected value
                PHYS_MEM_WRITE_32( (U32*)pRegVa, RegExpected );

                // Read register through PCI config cycle (bypass OS)
                PlxPciRegisterRead_BypassOS(
                    pNode->Key.bus,
                    pNode->Key.slot,
                    pNode->Key.function,
                    0x3C,
                    &RegPci
                    );

                // Restore original value
                PHYS_MEM_WRITE_32( (U32*)pRegVa, RegSave );
            }

            // If PCI register does not match expected value, port is link side
            if (RegPci != RegExpected)
                pNode->Key.NTPortType = PLX_NT_PORT_LINK;
        }
    }

    // Adjust offset for NT Link port
    if (pNode->Key.NTPortType == PLX_NT_PORT_LINK)
        pNode->Offset_NtRegBase += 0x1000;

    DebugPrintf((
        "%04X NT port at [b:%02x s:%02x f:%02x] is %s-side (NT base=%Xh)\n",
        pNode->Key.PlxChip, pNode->Key.bus, pNode->Key.slot, pNode->Key.function,
        (pNode->Key.NTPortType == PLX_NT_PORT_LINK) ? "Link" : "Virtual",
        pNode->Offset_NtRegBase
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
    PLX_DEVICE_NODE  *pNode;


    pEntry = pdx->List_Devices.next;

    // Traverse list to find the desired object
    while (pEntry != &(pdx->List_Devices))
    {
        // Get the object
        pNode =
            list_entry(
                pEntry,
                PLX_DEVICE_NODE,
                ListEntry
                );

        if ((pNode->Key.bus      == pKey->bus) &&
            (pNode->Key.slot     == pKey->slot) &&
            (pNode->Key.function == pKey->function))
        {
            return pNode;
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
    PLX_DEVICE_NODE *pNode
    )
{
    U8  i;
    U16 offset[] = {0xE0,0x958,0xB78,0x0};
    U16 DeviceId;
    U32 RegValue;


    // Default revision to PCI revision
    pNode->Key.PlxRevision = pNode->Key.Revision;

    i = 0;

    while (offset[i] != 0)
    {
        // Check for hard-coded ID
        PLX_PCI_REG_READ(
            pNode,
            offset[i],
            &RegValue
            );

        if ((RegValue & 0xFFFF) == PLX_VENDOR_ID)
        {
            pNode->Key.PlxChip = (U16)(RegValue >> 16);

            // Some PLX chips did not update hard-coded ID in revisions
            if ((pNode->Key.PlxChip != 0x8612) &&
                (pNode->Key.PlxChip != 0x8616) &&
                (pNode->Key.PlxChip != 0x8624) &&
                (pNode->Key.PlxChip != 0x8632) &&
                (pNode->Key.PlxChip != 0x8647) &&
                (pNode->Key.PlxChip != 0x8648))
            {
                // PLX revision should be in next register
                PLX_PCI_REG_READ(
                    pNode,
                    offset[i] + sizeof(U32),
                    &RegValue
                    );

                pNode->Key.PlxRevision = (U8)(RegValue & 0xFF);
            }
            goto _PlxChipAssignFamily;
        }

        // Go to next offset
        i++;
    }

    // Get current device ID
    DeviceId = pNode->Key.DeviceId;

    // Generalize for PLX 8000 devices
    if (pNode->Key.VendorId == PLX_VENDOR_ID)
    {
        // Attempt to use Subsytem ID for DMA devices that don't have hard-coded ID
        if ((DeviceId == 0x87D0) || (DeviceId == 0x87E0))
        {
            // Get PCI Subsystem ID
            PLX_PCI_REG_READ(
                pNode,
                0x2C,
                &RegValue
                );

            if ((RegValue & 0xFF00FFFF) == 0x870010B5)
            {
                pNode->Key.PlxChip = (U16)(RegValue >> 16);
                goto _PlxChipAssignFamily;
            }
        }

        // Don't include 8311 RDK
        if (DeviceId != 0x86e1)
        {
            if (((DeviceId & 0xFF00) == 0x8400) ||
                ((DeviceId & 0xFF00) == 0x8500) ||
                ((DeviceId & 0xFF00) == 0x8600) ||
                ((DeviceId & 0xFF00) == 0x8700) ||
                ((DeviceId & 0xFFF0) == 0x8110))
            {
                DeviceId = 0x8000;
            }
        }
    }

    // Hard-coded ID doesn't exist, so use Device/Vendor ID
    switch (((U32)DeviceId << 16) | pNode->Key.VendorId)
    {
        case 0x800010B5:        // All 8000 series devices
            // DMA devices that don't have hard-coded ID
            if ((pNode->Key.DeviceId == 0x87D0) || (pNode->Key.DeviceId == 0x87E0))
                pNode->Key.PlxChip = 0x8700;
            else
                pNode->Key.PlxChip = pNode->Key.DeviceId;
            break;

        case 0x905010b5:        // 9050/9052
        case 0x520110b5:        // PLX 9052 RDK
            pNode->Key.PlxChip = 0x9050;
            break;

        case 0x903010b5:        // 9030
        case 0x300110b5:        // PLX 9030 RDK
        case 0x30c110b5:        // PLX 9030 RDK - cPCI
            pNode->Key.PlxChip = 0x9030;
            break;

        case 0x908010b5:        // 9080
        case 0x040110b5:        // PLX 9080-401B RDK
        case 0x086010b5:        // PLX 9080-860 RDK
            pNode->Key.PlxChip = 0x9080;
            break;

        case 0x905410b5:        // 9054
        case 0x540610b5:        // PLX 9054 RDK-LITE
        case 0x186010b5:        // PLX 9054-860 RDK
        case 0xc86010b5:        // PLX 9054-860 RDK - cPCI
            pNode->Key.PlxChip = 0x9054;
            break;

        case 0x905610b5:        // 9056
        case 0x560110b5:        // PLX 9056 RDK-LITE
        case 0x56c210b5:        // PLX 9056-860 RDK
            pNode->Key.PlxChip = 0x9056;
            break;

        case 0x965610b5:        // 9656
        case 0x960110b5:        // PLX 9656 RDK-LITE
        case 0x96c210b5:        // PLX 9656-860 RDK
            pNode->Key.PlxChip = 0x9656;
            break;

        case 0x831110b5:        // 8311
        case 0x86e110b5:        // PLX 8311 RDK
            pNode->Key.PlxChip = 0x8311;
            break;

        case 0x00213388:        // 6140/6152/6254(NT)
            if (pNode->PciHeaderType == 0)
            {
                pNode->Key.PlxChip = 0x6254;
            }
            else
            {
                // Get 6152 VPD register
                PLX_PCI_REG_READ(
                    pNode,
                    0xA0,
                    &RegValue
                    );

                if ((RegValue & 0xF) == 0x03)    // CAP_ID_VPD
                {
                    pNode->Key.PlxChip = 0x6152;
                }
                else
                {
                    pNode->Key.PlxChip = 0x6140;
                }
            }
            break;

        case 0x00223388:        // 6150/6350
        case 0x00a23388:        // 6350
            if (pNode->Key.PlxRevision == 0x20)
            {
                pNode->Key.PlxChip = 0x6350;
            }
            else
            {
                pNode->Key.PlxChip = 0x6150;
            }
            break;

        case 0x00263388:        // 6154
            pNode->Key.PlxChip = 0x6154;
            break;

        case 0x00313388:        // 6156
            pNode->Key.PlxChip = 0x6156;
            break;

        case 0x00203388:        // 6254
            pNode->Key.PlxChip = 0x6254;
            break;

        case 0x00303388:        // 6520
        case 0x652010B5:        // 6520
            pNode->Key.PlxChip = 0x6520;
            break;

        case 0x00283388:        // 6540      - Transparent mode
        case 0x654010B5:        // 6540/6466 - Transparent mode
        case 0x00293388:        // 6540      - Non-transparent mode
        case 0x654110B5:        // 6540/6466 - Non-transparent mode
        case 0x654210B5:        // 6540/6466 - Non-transparent mode
            pNode->Key.PlxChip = 0x6540;
            break;
    }

    // Detect the PLX chip revision
    PlxChipRevisionDetect(
        pNode
        );

_PlxChipAssignFamily:

    switch (pNode->Key.PlxChip)
    {
        case 0x9050:        // 9000 series
        case 0x9030:
        case 0x9080:
        case 0x9054:
        case 0x9056:
        case 0x9656:
        case 0x8311:
            pNode->Key.PlxFamily = PLX_FAMILY_BRIDGE_P2L;
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
            pNode->Key.PlxFamily = PLX_FAMILY_BRIDGE_PCI_P2P;
            break;

        case 0x8111:
        case 0x8112:
        case 0x8114:
            pNode->Key.PlxFamily = PLX_FAMILY_BRIDGE_PCIE_P2P;
            break;

        case 0x8525:
        case 0x8533:
        case 0x8547:
        case 0x8548:
            pNode->Key.PlxFamily = PLX_FAMILY_ALTAIR;
            break;

        case 0x8505:
        case 0x8509:
            pNode->Key.PlxFamily = PLX_FAMILY_ALTAIR_XL;
            break;

        case 0x8516:
        case 0x8524:
        case 0x8532:
            pNode->Key.PlxFamily = PLX_FAMILY_VEGA;
            break;

        case 0x8508:
        case 0x8512:
        case 0x8517:
        case 0x8518:
            pNode->Key.PlxFamily = PLX_FAMILY_VEGA_LITE;
            break;

        case 0x8612:
        case 0x8616:
        case 0x8624:
        case 0x8632:
        case 0x8647:
        case 0x8648:
            pNode->Key.PlxFamily = PLX_FAMILY_DENEB;
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
            pNode->Key.PlxFamily = PLX_FAMILY_SIRIUS;
            break;

        case 0x8625:
        case 0x8636:
        case 0x8649:
        case 0x8664:
        case 0x8680:
        case 0x8696:
            pNode->Key.PlxFamily = PLX_FAMILY_CYGNUS;
            break;

        case 0x8700:
            // DMA devices that don't have hard-coded ID
            if ((pNode->Key.DeviceId == 0x87D0) || (pNode->Key.DeviceId == 0x87E0))
                pNode->Key.PlxFamily = PLX_FAMILY_DRACO_1;
            else
                pNode->Key.PlxFamily = PLX_FAMILY_SCOUT;
            break;

        case 0x8408:
        case 0x8416:
        case 0x8712:
        case 0x8716:
        case 0x8724:
        case 0x8732:
        case 0x8747:
        case 0x8748:
            pNode->Key.PlxFamily = PLX_FAMILY_DRACO_1;
            break;

        case 0x8713:
        case 0x8717:
        case 0x8725:
        case 0x8733:
        case 0x8749:
            pNode->Key.PlxFamily = PLX_FAMILY_DRACO_2;
            break;

        case 0:
            pNode->Key.PlxFamily = PLX_FAMILY_NONE;
            break;

        default:
            pNode->Key.PlxFamily = PLX_FAMILY_UNKNOWN;
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
    PLX_DEVICE_NODE *pNode
    )
{
    // Default revision to value in chip
    pNode->Key.PlxRevision = pNode->Key.Revision;

    // Determine if PLX revision should override default
    switch (pNode->Key.PlxChip)
    {
        case 0x8111:
            if (pNode->Key.PlxRevision == 0x10)
                pNode->Key.PlxRevision = 0xAA;
            else if (pNode->Key.PlxRevision == 0x20)
                pNode->Key.PlxRevision = 0xBA;
            else if (pNode->Key.PlxRevision == 0x21)
                pNode->Key.PlxRevision = 0xBB;
            break;

        case 0x8112:
            pNode->Key.PlxRevision = 0xAA;
            break;

        case 0x9050:
            if (pNode->Key.PlxRevision == 0x2)
                pNode->Key.PlxRevision = 2;
            else
                pNode->Key.PlxRevision = 1;
            break;

        case 0x9030:
            pNode->Key.PlxRevision = 0xAA;
            break;

        case 0x9080:
            pNode->Key.PlxRevision = 3;
            break;

        case 0x9056:
        case 0x9656:
            // Just use default revision for these chips
            break;

        case 0x9054:
            // AA & AB versions have same revision ID
            if ((pNode->Key.PlxRevision == 0x1) ||
                (pNode->Key.PlxRevision == 0xA) ||
                (pNode->Key.PlxRevision == 0xB))
            {
                pNode->Key.PlxRevision = 0xAB;
            }
            else
            {
                pNode->Key.PlxRevision = 0xAC;
            }
            break;

        case 0x8311:
            pNode->Key.PlxRevision = 0xAA;
            break;

        case 0x6140:
            if (pNode->Key.PlxRevision == 0x12)
            {
                pNode->Key.PlxRevision = 0xAA;
            }
            else
            {
                // Revision 0x13 only other
                pNode->Key.PlxRevision = 0xDA;
            }
            break;

        case 0x6150:
            if (pNode->Key.PlxRevision == 0x4)
                pNode->Key.PlxRevision = 0xBB;
            break;

        case 0x6152:
            switch (pNode->Key.PlxRevision)
            {
                case 0x13:
                    pNode->Key.PlxRevision = 0xBA;
                    break;

                case 0x14:
                    pNode->Key.PlxRevision = 0xCA;
                    break;

                case 0x15:
                    pNode->Key.PlxRevision = 0xCC;
                    break;

                case 0x16:
                    pNode->Key.PlxRevision = 0xDA;
                    break;
            }
            break;

        case 0x6154:
            if (pNode->Key.PlxRevision == 0x4)
                pNode->Key.PlxRevision = 0xBB;
            break;

        case 0x6350:
            if (pNode->Key.PlxRevision == 0x20)
                pNode->Key.PlxRevision = 0xAA;
            break;

        case 0x6156:
            if (pNode->Key.PlxRevision == 0x1)
                pNode->Key.PlxRevision = 0xDA;
            break;

        case 0x6254:
            if (pNode->Key.PlxRevision == 0x4)
                pNode->Key.PlxRevision = 0xBB;
            break;

        case 0x6520:
            if (pNode->Key.PlxRevision == 0x2)
                pNode->Key.PlxRevision = 0xBB;
            break;

        case 0x6540:
            if (pNode->Key.PlxRevision == 0x2)
                pNode->Key.PlxRevision = 0xBB;
            break;
    }
}
