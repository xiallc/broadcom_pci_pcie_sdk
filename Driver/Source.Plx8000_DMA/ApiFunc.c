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


#include <linux/uaccess.h>  // For copy_to/from_user()
#include <linux/sched.h>    // For MAX_SCHED_TIMEOUT & TASK_UNINTERRUPTIBLE
#include "ApiFunc.h"
#include "PciFunc.h"
#include "PciRegs.h"
#include "PlxChipFn.h"
#include "PlxInterrupt.h"
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
    U16            DeviceCount;
    BOOLEAN        bMatchId;
    BOOLEAN        bMatchLoc;
    DEVICE_OBJECT *fdo;


    DeviceCount = 0;

    // Get first device instance in list
    fdo = pdx->pDeviceObject->DriverObject->DeviceObject;

    // Compare with items in device list
    while (fdo != NULL)
    {
        // Get the device extension
        pdx = fdo->DeviceExtension;

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
            if ( (pKey->bus      != pdx->Key.bus)  ||
                 (pKey->slot     != pdx->Key.slot) ||
                 (pKey->function != pdx->Key.function) )
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
            if (pKey->VendorId != pdx->Key.VendorId)
            {
                bMatchId = FALSE;
            }
        }

        // Compare Device ID
        if (pKey->DeviceId != (U16)PCI_FIELD_IGNORE)
        {
            if (pKey->DeviceId != pdx->Key.DeviceId)
            {
                bMatchId = FALSE;
            }
        }

        // Compare Subsystem Vendor ID
        if (pKey->SubVendorId != (U16)PCI_FIELD_IGNORE)
        {
            if (pKey->SubVendorId != pdx->Key.SubVendorId)
            {
                bMatchId = FALSE;
            }
        }

        // Compare Subsystem Device ID
        if (pKey->SubDeviceId != (U16)PCI_FIELD_IGNORE)
        {
            if (pKey->SubDeviceId != pdx->Key.SubDeviceId)
            {
                bMatchId = FALSE;
            }
        }

        // Compare Revision
        if (pKey->Revision != (U8)PCI_FIELD_IGNORE)
        {
            if (pKey->Revision != pdx->Key.Revision)
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
                *pKey = pdx->Key;

                DebugPrintf((
                    "Criteria matched device %04X_%04X [%02x:%02x.%x]\n",
                    pdx->Key.DeviceId, pdx->Key.VendorId,
                    pdx->Key.bus, pdx->Key.slot, pdx->Key.function
                    ));
                return PLX_STATUS_OK;
            }

            // Increment device count
            DeviceCount++;
        }

        // Jump to next entry
        fdo = fdo->NextDevice;
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
    DEVICE_EXTENSION *pdx,
    U16              *pChipType,
    U8               *pRevision
    )
{
    *pChipType = pdx->Key.PlxChip;
    *pRevision = pdx->Key.PlxRevision;

    DebugPrintf((
        "PLX chip = %04X rev %02X\n",
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
    DEVICE_EXTENSION *pdx,
    U16               ChipType,
    U8                Revision
    )
{
    // Setting the PLX chip type is not supported in this PnP driver
    return PLX_STATUS_UNSUPPORTED;
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
    DEVICE_EXTENSION *pdx,
    PLX_PORT_PROP    *pPortProp
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
        pPortProp->PortType = PLX_PORT_LEGACY_ENDPOINT;
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
    DEVICE_EXTENSION *pdx
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
    DEVICE_EXTENSION *pdx,
    U32               offset,
    PLX_STATUS       *pStatus,
    BOOLEAN           bAdjustForPort
    )
{
    // Verify offset
    if ((offset & 0x3) || (offset >= pdx->PciBar[0].Properties.Size))
    {
        DebugPrintf(("ERROR - Invalid register offset (%X)\n", offset));
        if (pStatus)
        {
            *pStatus = PLX_STATUS_INVALID_OFFSET;
        }
        return 0;
    }

    if (pStatus)
    {
        *pStatus = PLX_STATUS_OK;
    }

    return PLX_DMA_REG_READ( pdx, offset );
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
    DEVICE_EXTENSION *pdx,
    U32               offset,
    U32               value,
    BOOLEAN           bAdjustForPort
    )
{
    // Verify offset
    if ((offset & 0x3) || (offset >= pdx->PciBar[0].Properties.Size))
    {
        DebugPrintf(("ERROR - Invalid register offset (%X)\n", offset));
        return PLX_STATUS_INVALID_OFFSET;
    }

    PLX_DMA_REG_WRITE( pdx, offset, value );

    return PLX_STATUS_OK;
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
    DEVICE_EXTENSION *pdx,
    U8                BarIndex,
    PLX_PCI_BAR_PROP *pBarProperties
    )
{
    // Verify BAR number
    switch (BarIndex)
    {
        case 0:
        case 1:
        case 2:
        case 3:
        case 4:
        case 5:
            break;

        default:
            return PLX_STATUS_INVALID_ACCESS;
    }

    // Return BAR properties
    *pBarProperties = pdx->PciBar[BarIndex].Properties;

    // Display BAR properties if enabled
    if (pdx->PciBar[BarIndex].Properties.Size == 0)
    {
        DebugPrintf(("BAR %d is disabled\n", BarIndex));
        return PLX_STATUS_OK;
    }

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

    DebugPrintf((
        "    Property : %sPrefetchable %d-bit\n",
        (pdx->PciBar[BarIndex].Properties.Flags & PLX_BAR_FLAG_PREFETCHABLE) ? "" : "Non-",
        (pdx->PciBar[BarIndex].Properties.Flags & PLX_BAR_FLAG_64_BIT) ? 64 : 32
        ));

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
    DEVICE_EXTENSION *pdx,
    U8                BarIndex,
    VOID             *pUserVa,
    VOID             *pOwner
    )
{
    // Handled in Dispatch_mmap() in Linux
    return PLX_STATUS_UNSUPPORTED;
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
    DEVICE_EXTENSION *pdx,
    VOID             *UserVa,
    VOID             *pOwner
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
    DEVICE_EXTENSION *pdx,
    U8               *pStatus
    )
{
    // Device does not support access to EEPROM
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
    DEVICE_EXTENSION *pdx,
    U8               *pFlag
    )
{
    // Device does not support access to EEPROM
    return PLX_STATUS_UNSUPPORTED;
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
    DEVICE_EXTENSION *pdx,
    U8               *pWidth
    )
{
    // Device does not support access to EEPROM
    return PLX_STATUS_UNSUPPORTED;
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
    DEVICE_EXTENSION *pdx,
    U8                width
    )
{
    // Device does not support access to EEPROM
    return PLX_STATUS_UNSUPPORTED;
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
    DEVICE_EXTENSION *pdx,
    U32              *pCrc,
    U8               *pCrcStatus
    )
{
    // Clear return value
    *pCrc       = 0;
    *pCrcStatus = PLX_CRC_UNSUPPORTED;

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
    DEVICE_EXTENSION *pdx,
    U32              *pCrc,
    BOOLEAN           bUpdateEeprom
    )
{
    // Clear return value
    *pCrc = 0;

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
    DEVICE_EXTENSION *pdx,
    U32               offset,
    U32              *pValue
    )
{
    // Device does not support access to EEPROM
    return PLX_STATUS_UNSUPPORTED;
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
    DEVICE_EXTENSION *pdx,
    U32               offset,
    U32               value
    )
{
    // Device does not support access to EEPROM
    return PLX_STATUS_UNSUPPORTED;
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
    DEVICE_EXTENSION *pdx,
    U32               offset,
    U16              *pValue
    )
{
    // Device does not support access to EEPROM
    return PLX_STATUS_UNSUPPORTED;
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
    DEVICE_EXTENSION *pdx,
    U32               offset,
    U16               value
    )
{
    // Device does not support access to EEPROM
    return PLX_STATUS_UNSUPPORTED;
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
    DEVICE_EXTENSION *pdx,
    PLX_PHYSICAL_MEM *pPciMem,
    BOOLEAN           bSmallerOk,
    VOID             *pOwner
    )
{
    U32                  DecrementAmount;
    PLX_PHYS_MEM_OBJECT *pMemObject;


    // Initialize buffer information
    pPciMem->UserAddr     = 0;
    pPciMem->PhysicalAddr = 0;
    pPciMem->CpuPhysical  = 0;

    /*******************************************************
     * Verify size
     *
     * A size of 0 is valid because this function may
     * be called to allocate a common buffer of size 0;
     * therefore, the information is reset & return sucess.
     ******************************************************/
    if (pPciMem->Size == 0)
    {
        return PLX_STATUS_OK;
    }

    // Allocate memory for new list object
    pMemObject =
        kmalloc(
            sizeof(PLX_PHYS_MEM_OBJECT),
            GFP_KERNEL
            );
    if (pMemObject == NULL)
    {
        DebugPrintf(("ERROR - Memory allocation for list object failed\n"));
        return PLX_STATUS_INSUFFICIENT_RES;
    }

    // Clear object
    RtlZeroMemory( pMemObject, sizeof(PLX_PHYS_MEM_OBJECT) );

    // Set buffer request size
    pMemObject->Size = pPciMem->Size;

    // Setup amount to reduce on failure
    DecrementAmount = (pPciMem->Size / 10);

    DebugPrintf((
        "Attempt to allocate physical memory (%dKB)\n",
        (pPciMem->Size >> 10)
        ));

    do
    {
        // Attempt to allocate the buffer
        pMemObject->pKernelVa =
            Plx_dma_buffer_alloc(
                pdx,
                pMemObject
                );

        if (pMemObject->pKernelVa == NULL)
        {
            // Reduce memory request size if requested
            if (bSmallerOk && (pMemObject->Size > PAGE_SIZE))
            {
                pMemObject->Size -= DecrementAmount;
            }
            else
            {
                ErrorPrintf(("ERROR - Physical memory allocation failed\n"));
                kfree( pMemObject );
                pPciMem->Size = 0;
                return PLX_STATUS_INSUFFICIENT_RES;
            }
        }
    }
    while (pMemObject->pKernelVa == NULL);

    // Record buffer owner
    pMemObject->pOwner = pOwner;

    // Assign buffer to device if provided
    if (pOwner != pGbl_DriverObject)
    {
        // Return buffer information
        pPciMem->Size         = pMemObject->Size;
        pPciMem->PhysicalAddr = pMemObject->BusPhysical;
        pPciMem->CpuPhysical  = pMemObject->CpuPhysical;

        // Add buffer object to list
        spin_lock(
            &(pdx->Lock_PhysicalMemList)
            );

        list_add_tail(
            &(pMemObject->ListEntry),
            &(pdx->List_PhysicalMem)
            );

        spin_unlock(
            &(pdx->Lock_PhysicalMemList)
            );
    }
    else
    {
        // Store common buffer information
        pGbl_DriverObject->CommonBuffer = *pMemObject;

        // Release the list object
        kfree( pMemObject );
    }

    return PLX_STATUS_OK;
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
    DEVICE_EXTENSION *pdx,
    PLX_PHYSICAL_MEM *pPciMem
    )
{
    struct list_head    *pEntry;
    PLX_PHYS_MEM_OBJECT *pMemObject;


    spin_lock( &(pdx->Lock_PhysicalMemList) );

    pEntry = pdx->List_PhysicalMem.next;

    // Traverse list to find the desired list object
    while (pEntry != &(pdx->List_PhysicalMem))
    {
        // Get the object
        pMemObject =
            list_entry(
                pEntry,
                PLX_PHYS_MEM_OBJECT,
                ListEntry
                );

        // Check if the physical addresses matches
        if (pMemObject->BusPhysical == pPciMem->PhysicalAddr)
        {
            // Remove the object from the list
            list_del( pEntry );

            spin_unlock( &(pdx->Lock_PhysicalMemList) );

            // Release the buffer
            Plx_dma_buffer_free( pdx, pMemObject );

            // Release the list object
            kfree( pMemObject );

            return PLX_STATUS_OK;
        }

        // Jump to next item in the list
        pEntry = pEntry->next;
    }

    spin_unlock( &(pdx->Lock_PhysicalMemList) );

    DebugPrintf(("ERROR - buffer object not found in list\n"));

    return PLX_STATUS_INVALID_DATA;
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
    DEVICE_EXTENSION *pdx,
    PLX_PHYSICAL_MEM *pPciMem,
    VOID             *pOwner
    )
{
    // Handled in Dispatch_mmap() in Linux
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
    DEVICE_EXTENSION *pdx,
    PLX_PHYSICAL_MEM *pPciMem,
    VOID             *pOwner
    )
{
    // Handled by user-level API in Linux
    return PLX_STATUS_UNSUPPORTED;
}




/*******************************************************************************
 *
 * Function   :  PlxInterruptEnable
 *
 * Description:  Enables specific interupts of the PLX Chip
 *
 ******************************************************************************/
PLX_STATUS
PlxInterruptEnable(
    DEVICE_EXTENSION *pdx,
    PLX_INTERRUPT    *pPlxIntr
    )
{
    U8           channel;
    PLX_REG_DATA RegData;


    // Setup to synchronize access to interrupt register
    RegData.pdx         = pdx;
    RegData.BitsToClear = 0;

    // Enable interrupts for each channel
    for (channel = 0; channel < pdx->NumDmaChannels; channel++)
    {
        // Determine DMA interrupt control register offset
        RegData.offset = 0x23C + (channel * 0x100);

        // Clear bit field
        RegData.BitsToSet = 0;

        // Enable desired interrupts
        if (pPlxIntr->DmaPauseDone & (1 << channel))
        {
            RegData.BitsToSet |= (1 << 4);
        }

        if (pPlxIntr->DmaAbortDone & (1 << channel))
        {
            RegData.BitsToSet |= (1 << 3);
        }

        if (pPlxIntr->DmaImmedStopDone & (1 << channel))
        {
            RegData.BitsToSet |= (1 << 5);
        }

        if (pPlxIntr->DmaInvalidDescr & (1 << channel))
        {
            RegData.BitsToSet |= (1 << 1);
        }

        if (pPlxIntr->DmaError & (1 << channel))
        {
            RegData.BitsToSet |= (1 << 0);
        }

        // Write register values if they have changed
        if (RegData.BitsToSet != 0)
        {
            // Synchronize write of Interrupt Control/Status Register
            PlxSynchronizedRegisterModify(
                &RegData
                );
        }
    }

    return PLX_STATUS_OK;
}




/*******************************************************************************
 *
 * Function   :  PlxInterruptDisable
 *
 * Description:  Disables specific interrupts of the PLX Chip
 *
 ******************************************************************************/
PLX_STATUS
PlxInterruptDisable(
    DEVICE_EXTENSION *pdx,
    PLX_INTERRUPT    *pPlxIntr
    )
{
    U8           channel;
    PLX_REG_DATA RegData;


    // Setup to synchronize access to interrupt register
    RegData.pdx       = pdx;
    RegData.BitsToSet = 0;

    // Enable interrupts for each channel
    for (channel = 0; channel < pdx->NumDmaChannels; channel++)
    {
        // Determine DMA interrupt control register offset
        RegData.offset = 0x23C + (channel * 0x100);

        // Clear bit field
        RegData.BitsToClear = 0;

        // Enable desired interrupts
        if (pPlxIntr->DmaPauseDone & (1 << channel))
        {
            RegData.BitsToClear |= (1 << 4);
        }

        if (pPlxIntr->DmaAbortDone & (1 << channel))
        {
            RegData.BitsToClear |= (1 << 3);
        }

        if (pPlxIntr->DmaImmedStopDone & (1 << channel))
        {
            RegData.BitsToClear |= (1 << 5);
        }

        if (pPlxIntr->DmaInvalidDescr & (1 << channel))
        {
            RegData.BitsToClear |= (1 << 1);
        }

        if (pPlxIntr->DmaError & (1 << channel))
        {
            RegData.BitsToClear |= (1 << 0);
        }

        // Write register values if they have changed
        if (RegData.BitsToClear != 0)
        {
            // Synchronize write of Interrupt Control/Status Register
            PlxSynchronizedRegisterModify(
                &RegData
                );
        }
    }

    return PLX_STATUS_OK;
}




/*******************************************************************************
 *
 * Function   :  PlxNotificationRegisterFor
 *
 * Description:  Registers a wait object for notification on interrupt(s)
 *
 ******************************************************************************/
PLX_STATUS
PlxNotificationRegisterFor(
    DEVICE_EXTENSION  *pdx,
    PLX_INTERRUPT     *pPlxIntr,
    VOID             **pUserWaitObject,
    VOID              *pOwner
    )
{
    unsigned long    flags;
    PLX_WAIT_OBJECT *pWaitObject;


    // Allocate a new wait object
    pWaitObject =
        kmalloc(
            sizeof(PLX_WAIT_OBJECT),
            GFP_KERNEL
            );

    if (pWaitObject == NULL)
    {
        DebugPrintf(("ERROR - Allocation for interrupt wait object failed\n"));
        return PLX_STATUS_INSUFFICIENT_RES;
    }

    // Provide the wait object to the user app
    *pUserWaitObject = pWaitObject;

    // Record the owner
    pWaitObject->pOwner = pOwner;

    // Mark the object as waiting
    pWaitObject->state = PLX_STATE_WAITING;

    // Clear number of sleeping threads
    atomic_set( &pWaitObject->SleepCount, 0 );

    // Initialize wait queue
    init_waitqueue_head( &(pWaitObject->WaitQueue) );

    // Clear interrupt source
    pWaitObject->Source_Ints = INTR_TYPE_NONE;

    // Set interrupt notification flags
    PlxChipSetInterruptNotifyFlags(
        pdx,
        pPlxIntr,
        pWaitObject
        );

    // Add to list of waiting objects
    spin_lock_irqsave(
        &(pdx->Lock_WaitObjectsList),
        flags
        );

    list_add_tail(
        &(pWaitObject->ListEntry),
        &(pdx->List_WaitObjects)
        );

    spin_unlock_irqrestore(
        &(pdx->Lock_WaitObjectsList),
        flags
        );

    DebugPrintf((
        "Registered interrupt wait object (%p)\n",
        pWaitObject
        ));

    return PLX_STATUS_OK;
}




/*******************************************************************************
 *
 * Function   :  PlxNotificationWait
 *
 * Description:  Put the process to sleep until wake-up event occurs or timeout
 *
 ******************************************************************************/
PLX_STATUS
PlxNotificationWait(
    DEVICE_EXTENSION *pdx,
    VOID             *pUserWaitObject,
    PLX_UINT_PTR      Timeout_ms
    )
{
    long              Wait_rc;
    PLX_STATUS        rc;
    PLX_UINT_PTR      Timeout_sec;
    unsigned long     flags;
    struct list_head *pEntry;
    PLX_WAIT_OBJECT  *pWaitObject;


    // Find the wait object in the list
    spin_lock_irqsave(
        &(pdx->Lock_WaitObjectsList),
        flags
        );

    pEntry = pdx->List_WaitObjects.next;

    // Find the wait object and wait for wake-up event
    while (pEntry != &(pdx->List_WaitObjects))
    {
        // Get the wait object
        pWaitObject =
            list_entry(
                pEntry,
                PLX_WAIT_OBJECT,
                ListEntry
                );

        // Check if the object address matches the Tag
        if (pWaitObject == pUserWaitObject)
        {
            spin_unlock_irqrestore(
                &(pdx->Lock_WaitObjectsList),
                flags
                );

            DebugPrintf((
                "Waiting for Interrupt wait object (%p) to wake-up\n",
                pWaitObject
                ));

            /*********************************************************
             * Convert milliseconds to jiffies.  The following
             * formula is used:
             *
             *                      ms * HZ
             *           jiffies = ---------
             *                       1,000
             *
             *
             *  where:  HZ      = System-defined clock ticks per second
             *          ms      = Timeout in milliseconds
             *          jiffies = Number of HZ's per second
             *
             *  Note: Since the timeout is stored as a "long" integer,
             *        the conversion to jiffies is split into two operations.
             *        The first is on number of seconds and the second on
             *        the remaining millisecond precision.  This mimimizes
             *        overflow when the specified timeout is large and also
             *        keeps millisecond precision.
             ********************************************************/

            // Perform conversion if not infinite wait
            if (Timeout_ms != PLX_TIMEOUT_INFINITE)
            {
                // Get number of seconds
                Timeout_sec = Timeout_ms / 1000;

                // Store milliseconds precision
                Timeout_ms = Timeout_ms - (Timeout_sec * 1000);

                // Convert to jiffies
                Timeout_sec = Timeout_sec * HZ;
                Timeout_ms  = (Timeout_ms * HZ) / 1000;

                // Compute total jiffies
                Timeout_ms = Timeout_sec + Timeout_ms;
            }

            // Timeout parameter is signed and can't be negative
            if ((signed long)Timeout_ms < 0)
            {
                // Shift out negative bit
                Timeout_ms = Timeout_ms >> 1;
            }

            // Increment number of sleeping threads
            atomic_inc( &pWaitObject->SleepCount );

            do
            {
                // Wait for interrupt event
                Wait_rc =
                    wait_event_interruptible_timeout(
                        pWaitObject->WaitQueue,
                        (pWaitObject->state != PLX_STATE_WAITING),
                        Timeout_ms
                        );
            }
            while ((Wait_rc == 0) && (Timeout_ms == PLX_TIMEOUT_INFINITE));

            if (Wait_rc > 0)
            {
                // Condition met or interrupt occurred
                DebugPrintf(("Interrupt wait object awakened\n"));
                rc = PLX_STATUS_OK;
            }
            else if (Wait_rc == 0)
            {
                // Timeout reached
                DebugPrintf(("Timeout waiting for interrupt\n"));
                rc = PLX_STATUS_TIMEOUT;
            }
            else
            {
                // Interrupted by a signal
                DebugPrintf(("Interrupt wait object interrupted by signal or error\n"));
                rc = PLX_STATUS_CANCELED;
            }

            // If object is in triggered state, rest to waiting state
            if (pWaitObject->state == PLX_STATE_TRIGGERED)
            {
                pWaitObject->state = PLX_STATE_WAITING;
            }

            // Decrement number of sleeping threads
            atomic_dec( &pWaitObject->SleepCount );

            return rc;
        }

        // Jump to next item in the list
        pEntry = pEntry->next;
    }

    spin_unlock_irqrestore(
        &(pdx->Lock_WaitObjectsList),
        flags
        );

    DebugPrintf((
        "Interrupt wait object (%p) not found or previously canceled\n",
        pUserWaitObject
        ));

    // Object not found at this point
    return PLX_STATUS_FAILED;
}




/*******************************************************************************
 *
 * Function   :  PlxNotificationStatus
 *
 * Description:  Returns the interrupt(s) that have caused notification events
 *
 ******************************************************************************/
PLX_STATUS
PlxNotificationStatus(
    DEVICE_EXTENSION *pdx,
    VOID             *pUserWaitObject,
    PLX_INTERRUPT    *pPlxIntr
    )
{
    unsigned long       flags;
    struct list_head   *pEntry;
    PLX_WAIT_OBJECT    *pWaitObject;
    PLX_INTERRUPT_DATA  IntData;


    spin_lock_irqsave(
        &(pdx->Lock_WaitObjectsList),
        flags
        );

    pEntry = pdx->List_WaitObjects.next;

    // Traverse list to find the desired list object
    while (pEntry != &(pdx->List_WaitObjects))
    {
        // Get the object
        pWaitObject =
            list_entry(
                pEntry,
                PLX_WAIT_OBJECT,
                ListEntry
                );

        // Check if desired object
        if (pWaitObject == pUserWaitObject)
        {
            // Copy the interrupt sources
            IntData.Source_Ints = pWaitObject->Source_Ints;

            // Reset interrupt sources
            pWaitObject->Source_Ints = INTR_TYPE_NONE;

            spin_unlock_irqrestore(
                &(pdx->Lock_WaitObjectsList),
                flags
                );

            DebugPrintf((
                "Returning status for interrupt wait object (%p)...\n",
                pWaitObject
                ));

            // Set triggered interrupts
            PlxChipSetInterruptStatusFlags(
                pdx,
                &IntData,
                pPlxIntr
                );

            return PLX_STATUS_OK;
        }

        // Jump to next item in the list
        pEntry = pEntry->next;
    }

    spin_unlock_irqrestore(
        &(pdx->Lock_WaitObjectsList),
        flags
        );

    return PLX_STATUS_FAILED;
}




/*******************************************************************************
 *
 * Function   :  PlxNotificationCancel
 *
 * Description:  Cancels a registered notification event
 *
 ******************************************************************************/
PLX_STATUS
PlxNotificationCancel(
    DEVICE_EXTENSION *pdx,
    VOID             *pUserWaitObject,
    VOID             *pOwner
    )
{
    U32               LoopCount;
    BOOLEAN           bRemove;
    unsigned long     flags;
    struct list_head *pEntry;
    PLX_WAIT_OBJECT  *pWaitObject;


    spin_lock_irqsave(
        &(pdx->Lock_WaitObjectsList),
        flags
        );

    pEntry = pdx->List_WaitObjects.next;

    // Find the object and remove it
    while (pEntry != &(pdx->List_WaitObjects))
    {
        // Get the object
        pWaitObject =
            list_entry(
                pEntry,
                PLX_WAIT_OBJECT,
                ListEntry
                );

        // Default to not remove
        bRemove = FALSE;

        // Determine if object should be removed
        if (pOwner == pWaitObject->pOwner)
        {
            if (pUserWaitObject == NULL)
            {
                bRemove = TRUE;
            }
            else if (pWaitObject == pUserWaitObject)
            {
                bRemove = TRUE;
            }
        }

        // Remove object
        if (bRemove)
        {
            DebugPrintf((
                "Removing interrupt wait object (%p)...\n",
                pWaitObject
                ));

            // Remove the object from the list
            list_del(
                pEntry
                );

            spin_unlock_irqrestore(
                &(pdx->Lock_WaitObjectsList),
                flags
                );

            // Set loop count
            LoopCount = 20;

            // Wake-up processes if wait object is pending
            if (atomic_read(&pWaitObject->SleepCount) != 0)
            {
                DebugPrintf(("Wait object is pending in another thread, forcing wake up\n"));

                // Mark object for deletion
                pWaitObject->state = PLX_STATE_MARKED_FOR_DELETE;

                // Wake-up any process waiting on the object
                wake_up_interruptible(
                    &(pWaitObject->WaitQueue)
                    );

                do
                {
                    // Set current task as uninterruptible
                    set_current_state(TASK_UNINTERRUPTIBLE);

                    // Relieve timeslice to allow pending thread to wake up
                    schedule_timeout( Plx_ms_to_jiffies( 10 ) );

                    // Decrement counter
                    LoopCount--;
                }
                while (LoopCount && (atomic_read(&pWaitObject->SleepCount) != 0));
            }


            if (LoopCount == 0)
            {
                DebugPrintf(("ERROR: Timeout waiting for pending thread, unable to free wait object\n"));
            }
            else
            {
                // Release the list object
                kfree(
                    pWaitObject
                    );
            }

            // Return if removing only a specific object
            if (pUserWaitObject != NULL)
            {
                return PLX_STATUS_OK;
            }

            // Reset to beginning of list
            spin_lock_irqsave(
                &(pdx->Lock_WaitObjectsList),
                flags
                );

            pEntry = pdx->List_WaitObjects.next;
        }
        else
        {
            // Jump to next item in the list
            pEntry = pEntry->next;
        }
    }

    spin_unlock_irqrestore(
        &(pdx->Lock_WaitObjectsList),
        flags
        );

    return PLX_STATUS_FAILED;
}




/******************************************************************************
 *
 * Function   :  PlxDmaChannelOpen
 *
 * Description:  Open a DMA channel
 *
 ******************************************************************************/
PLX_STATUS
PlxDmaChannelOpen(
    DEVICE_EXTENSION *pdx,
    U8                channel,
    VOID             *pOwner
    )
{
    U32 RegValue;


    // Verify valid DMA channel
    switch (channel)
    {
        case 0:
        case 1:
        case 2:
        case 3:
            if (channel >= pdx->NumDmaChannels)
            {
                DebugPrintf((
                    "Error - Channel %d exceeds max supported (%d)\n",
                    channel, (pdx->NumDmaChannels - 1)
                    ));
                return PLX_STATUS_INVALID_ACCESS;
            }
            break;

        default:
            DebugPrintf(("ERROR - Invalid DMA channel\n"));
            return PLX_STATUS_INVALID_ACCESS;
    }

    if (pdx->Key.PlxFamily == PLX_FAMILY_SIRIUS)
    {
        // Verify channel is enabled in hardware
        RegValue = PLX_DMA_REG_READ( pdx, 0x1FC );

        // Check DMA channel setup (1FC[1:0])
        switch (RegValue & 0x3)
        {
            case 0:
                // All 4 channels active
                break;

            case 1:
                // Only channel 0 active
                if (channel != 0)
                {
                    return PLX_STATUS_INVALID_ACCESS;
                }
                break;

            case 2:
                // Channels 0 & 2 active
                if ((channel != 0) && (channel != 2))
                {
                    return PLX_STATUS_INVALID_ACCESS;
                }
                break;

            case 3:
                // Channels 0, 1, & 2 active
                if (channel == 3)
                {
                    return PLX_STATUS_INVALID_ACCESS;
                }
                break;
        }
    }

    spin_lock(
        &(pdx->Lock_Dma[channel])
        );

    // Verify that we can open the channel
    if (pdx->DmaInfo[channel].bOpen)
    {
        DebugPrintf(("ERROR - DMA channel already opened\n"));
        spin_unlock(
            &(pdx->Lock_Dma[channel])
            );
        return PLX_STATUS_INVALID_ACCESS;
    }

    // Open the channel
    pdx->DmaInfo[channel].bOpen = TRUE;

    // Record the Owner
    pdx->DmaInfo[channel].pOwner = pOwner;

    // No SGL DMA is pending
    pdx->DmaInfo[channel].bSglPending = FALSE;

    spin_unlock(
        &(pdx->Lock_Dma[channel])
        );

    DebugPrintf((
        "Opened DMA channel %d\n",
        channel
        ));

    return PLX_STATUS_OK;
}




/******************************************************************************
 *
 * Function   :  PlxDmaGetProperties
 *
 * Description:  Gets the current DMA properties
 *
 ******************************************************************************/
PLX_STATUS
PlxDmaGetProperties(
    DEVICE_EXTENSION *pdx,
    U8                channel,
    PLX_DMA_PROP     *pProp
    )
{
    U32 RegValue;
    U32 OffsetDmaBase;


    // Verify valid DMA channel
    switch (channel)
    {
        case 0:
        case 1:
        case 2:
        case 3:
            if (channel >= pdx->NumDmaChannels)
            {
                DebugPrintf((
                    "Error - Channel %d exceeds max supported (%d)\n",
                    channel, (pdx->NumDmaChannels - 1)
                    ));
                return PLX_STATUS_INVALID_ACCESS;
            }
            break;

        default:
            DebugPrintf(("ERROR - Invalid DMA channel\n"));
            return PLX_STATUS_INVALID_ACCESS;
    }

    // Set the channel's base register offset (200h, 300h, etc)
    OffsetDmaBase = 0x200 + (channel * 0x100);

    // Clear properties
    RtlZeroMemory( pProp, sizeof(PLX_DMA_PROP) );

    // Channel bandwith
    RegValue = PLX_DMA_REG_READ( pdx, OffsetDmaBase + 0x2C );
    pProp->ReadReqDelayClocks = (U16)(RegValue >> 0) & 0xFFFF;
    if (pdx->Key.PlxFamily != PLX_FAMILY_SIRIUS)
        pProp->MaxDestWriteSize = (U8)((RegValue >> 24) & 0x7);

    // Max prefetch
    RegValue = PLX_DMA_REG_READ( pdx, OffsetDmaBase + 0x34 );
    pProp->MaxDescriptorFetch = (U8)(RegValue >> 0) & 0xFF;

    // DMA control/status
    RegValue = PLX_DMA_REG_READ( pdx, OffsetDmaBase + 0x38 );
    pProp->CplStatusWriteBack  = (U8)(RegValue >>  2) & 0x1;
    pProp->RingWrapDelayTime   = (U8)(RegValue >> 13) & 0x7;
    pProp->MaxSrcXferSize      = (U8)(RegValue >> 16) & 0x7;
    pProp->TrafficClass        = (U8)(RegValue >> 19) & 0x7;
    pProp->RelOrderDescrRead   = (U8)(RegValue >> 22) & 0x1;
    pProp->RelOrderDataReadReq = (U8)(RegValue >> 23) & 0x1;
    pProp->RelOrderDescrWrite  = (U8)(RegValue >> 24) & 0x1;
    pProp->RelOrderDataWrite   = (U8)(RegValue >> 25) & 0x1;
    pProp->NoSnoopDescrRead    = (U8)(RegValue >> 26) & 0x1;
    pProp->NoSnoopDataReadReq  = (U8)(RegValue >> 27) & 0x1;
    pProp->NoSnoopDescrWrite   = (U8)(RegValue >> 28) & 0x1;
    pProp->NoSnoopDataWrite    = (U8)(RegValue >> 29) & 0x1;

    if (pdx->Key.PlxFamily == PLX_FAMILY_SIRIUS)
    {
        pProp->DescriptorMode = (U8)(RegValue >> 4) & 0x1;
        pProp->RingHaltAtEnd  = (U8)(RegValue >> 5) & 0x1;
    }
    else
    {
        pProp->RingHaltAtEnd  = (U8)(RegValue >> 4) & 0x1;
        pProp->DescriptorMode = (U8)(RegValue >> 5) & 0x3;

        // DMA Descr mode (0=block, 1=SGL on-chip, 2=SGL off-chip, 3=reserved)
        if (pProp->DescriptorMode == 1)
        {
            pProp->DescriptorMode = PLX_DMA_MODE_SGL_INTERNAL;
        }
        else if (pProp->DescriptorMode == 2)
        {
            pProp->DescriptorMode = PLX_DMA_MODE_SGL;
        }
    }

    // Max pending requests
    RegValue = PLX_DMA_REG_READ( pdx, OffsetDmaBase + 0x54 );
    pProp->MaxPendingReadReq = (U8)(RegValue >> 0) & 0x3F;

    return PLX_STATUS_OK;
}




/******************************************************************************
 *
 * Function   :  PlxDmaSetProperties
 *
 * Description:  Sets the current DMA properties
 *
 ******************************************************************************/
PLX_STATUS
PlxDmaSetProperties(
    DEVICE_EXTENSION *pdx,
    U8                channel,
    PLX_DMA_PROP     *pProp,
    VOID             *pOwner
    )
{
    U32        OffsetDmaBase;
    U32        RegValue;
    PLX_STATUS status;


    // Verify DMA channel is available
    status =
        PlxDmaStatus(
            pdx,
            channel,
            pOwner
            );

    if (status != PLX_STATUS_COMPLETE)
    {
        DebugPrintf(("ERROR - DMA unavailable or in-progress\n"));
        return status;
    }

    // Set the channel's base register offset (200h, 300h, etc)
    OffsetDmaBase = 0x200 + (channel * 0x100);

    // Channel bandwith
    RegValue = PLX_DMA_REG_READ( pdx, OffsetDmaBase + 0x2C );
    RegValue &= ~(0x0700FFFF);
    RegValue |= (pProp->ReadReqDelayClocks & 0xFFFF) << 0;
    if (pdx->Key.PlxFamily != PLX_FAMILY_SIRIUS)
    {
        RegValue |= (U32)(pProp->MaxDestWriteSize & 0x7) << 24;
    }
    PLX_DMA_REG_WRITE( pdx, OffsetDmaBase + 0x2C, RegValue );

    // Max prefetch
    RegValue = PLX_DMA_REG_READ( pdx, OffsetDmaBase + 0x34 );
    RegValue &= ~(0xFF);
    RegValue |= (pProp->MaxDescriptorFetch & 0xFF) << 0;
    PLX_DMA_REG_WRITE( pdx, OffsetDmaBase + 0x34, RegValue );

    // DMA control/status
    RegValue = PLX_DMA_REG_READ( pdx, OffsetDmaBase + 0x38 );
    RegValue &= ~(0x3FFFE034);
    RegValue |= (pProp->CplStatusWriteBack  & 0x1) <<  2;
    RegValue |= (pProp->RingWrapDelayTime   & 0x7) << 13;
    RegValue |= (pProp->MaxSrcXferSize      & 0x7) << 16;
    RegValue |= (pProp->TrafficClass        & 0x7) << 19;
    RegValue |= (pProp->RelOrderDescrRead   & 0x1) << 22;
    RegValue |= (pProp->RelOrderDataReadReq & 0x1) << 23;
    RegValue |= (pProp->RelOrderDescrWrite  & 0x1) << 24;
    RegValue |= (pProp->RelOrderDataWrite   & 0x1) << 25;
    RegValue |= (pProp->NoSnoopDescrRead    & 0x1) << 26;
    RegValue |= (pProp->NoSnoopDataReadReq  & 0x1) << 27;
    RegValue |= (pProp->NoSnoopDescrWrite   & 0x1) << 28;
    RegValue |= (pProp->NoSnoopDataWrite    & 0x1) << 29;
    if (pdx->Key.PlxFamily == PLX_FAMILY_SIRIUS)
    {
        RegValue |= (pProp->DescriptorMode & 0x1) << 4;
        RegValue |= (pProp->RingHaltAtEnd  & 0x1) << 5;
    }
    else
    {
        RegValue |= (pProp->RingHaltAtEnd  & 0x1) << 4;
        pProp->DescriptorMode = (U8)(RegValue >> 5) & 0x3;

        // DMA Descr mode (0=block, 1=SGL on-chip, 2=SGL off-chip, 3=reserved)
        if (pProp->DescriptorMode == PLX_DMA_MODE_SGL)
        {
            pProp->DescriptorMode = 2;
        }
        else if (pProp->DescriptorMode == PLX_DMA_MODE_SGL_INTERNAL)
        {
            pProp->DescriptorMode = 1;
        }
        RegValue |= (pProp->DescriptorMode & 0x3) << 5;
    }
    PLX_DMA_REG_WRITE( pdx, OffsetDmaBase + 0x38, RegValue );

    // Max pending requests
    RegValue = PLX_DMA_REG_READ( pdx, OffsetDmaBase + 0x54 );
    RegValue &= ~((0x3F << 16) | (0x3F << 0));
    RegValue |= (pProp->MaxPendingReadReq & 0x3F) >> 0;
    PLX_DMA_REG_WRITE( pdx, OffsetDmaBase + 0x54, RegValue );

    return PLX_STATUS_OK;
}




/******************************************************************************
 *
 * Function   :  PlxDmaControl
 *
 * Description:  Control the DMA engine
 *
 ******************************************************************************/
PLX_STATUS
PlxDmaControl(
    DEVICE_EXTENSION *pdx,
    U8                channel,
    PLX_DMA_COMMAND   command,
    VOID             *pOwner
    )
{
    U16 OffsetDmaBase;
    U32 LoopCount;
    U32 RegStatus;


    // Verify valid DMA channel
    switch (channel)
    {
        case 0:
        case 1:
        case 2:
        case 3:
            if (channel >= pdx->NumDmaChannels)
            {
                DebugPrintf((
                    "Error - Channel %d exceeds max supported (%d)\n",
                    channel, (pdx->NumDmaChannels - 1)
                    ));
                return PLX_STATUS_INVALID_ACCESS;
            }
            break;

        default:
            DebugPrintf(("ERROR - Invalid DMA channel\n"));
            return PLX_STATUS_INVALID_ACCESS;
    }

    // Verify owner
    if ((pdx->DmaInfo[channel].bOpen) && (pdx->DmaInfo[channel].pOwner != pOwner))
    {
        DebugPrintf(("ERROR - DMA owned by different process\n"));
        return PLX_STATUS_IN_USE;
    }

    // Set the channel's base register offset (200h, 300h, etc)
    OffsetDmaBase = 0x200 + (channel * 0x100);

    // Get DMA status
    RegStatus = PLX_DMA_REG_READ( pdx, OffsetDmaBase + 0x38 );

    // Clear any active status bits ([31,12:8])
    RegStatus &= ~((1 << 31) | (0x1F << 8));

    switch (command)
    {
        case DmaPause:
            DebugPrintf(("Pausing DMA channel %d\n", channel));

            // Return if already paused
            if (RegStatus & (1 << 0))
            {
                return PLX_STATUS_OK;
            }

            // Clear pause done status ([9]) & request pause ([0])
            RegStatus |= (1 << 9) | (1 << 0);

            PLX_DMA_REG_WRITE( pdx, OffsetDmaBase + 0x38, RegStatus );

            // Set poll count
            LoopCount = 500;

            // Poll pause done bit
            do
            {
                RegStatus = PLX_DMA_REG_READ( pdx, OffsetDmaBase + 0x38 );

                LoopCount--;
                if (LoopCount == 0)
                {
                    return PLX_STATUS_TIMEOUT;
                }
            }
            while ((RegStatus & (1 << 9)) == 0);

            // Clear any active status bits ([31,12:8])
            RegStatus &= ~((1 << 31) | (0x1F << 8));

            // Clear pause done status ([9])
            RegStatus |= (1 << 9);

            PLX_DMA_REG_WRITE( pdx, OffsetDmaBase + 0x38, RegStatus );
            break;

        case DmaPauseImmediate:
            DebugPrintf(("Pausing DMA channel %d immediately\n", channel));

            // Clear stop done done status ([12])
            RegStatus |= (1 << 12);

            PLX_DMA_REG_WRITE( pdx, OffsetDmaBase + 0x38, RegStatus );

            // Get DMA bandwith register
            RegStatus = PLX_DMA_REG_READ( pdx, OffsetDmaBase + 0x2C );

            // Return if already stopped
            if (RegStatus & (1 << 16))
            {
                return PLX_STATUS_OK;
            }

            // Halt DMA from issuing more reads ([16])
            RegStatus |= (1 << 16);

            PLX_DMA_REG_WRITE( pdx, OffsetDmaBase + 0x2C, RegStatus );

            // Set poll count
            LoopCount = 500;

            // Poll immediate stop done bit ([12])
            do
            {
                RegStatus = PLX_DMA_REG_READ( pdx, OffsetDmaBase + 0x38 );

                LoopCount--;
                if (LoopCount == 0)
                {
                    return PLX_STATUS_TIMEOUT;
                }
            }
            while ((RegStatus & (1 << 12)) == 0);

            // Clear any active status bits ([31,12:8])
            RegStatus &= ~((1 << 31) | (0x1F << 8));

            // Clear stop done status ([12])
            RegStatus |= (1 << 12);

            PLX_DMA_REG_WRITE( pdx, OffsetDmaBase + 0x38, RegStatus );
            break;

        case DmaResume:
            DebugPrintf(("Resuming DMA channel %d\n", channel));

            // Clear pause status [9]
            RegStatus |= (1 << 9);

            // Clear pause ([0])
            RegStatus &= ~(1 << 0);

            // Resume transfer
            PLX_DMA_REG_WRITE( pdx, OffsetDmaBase + 0x38, RegStatus );

            // Get DMA bandwith register
            RegStatus = PLX_DMA_REG_READ( pdx, OffsetDmaBase + 0x2C );

            // Resume if stopped
            if (RegStatus & (1 << 16))
            {
                // Resume DMA reads ([16])
                RegStatus &= ~(1 << 16);

                PLX_DMA_REG_WRITE( pdx, OffsetDmaBase + 0x2C, RegStatus );
            }
            break;

        case DmaAbort:
            DebugPrintf(("Aborting DMA channel %d\n", channel));

            // Clear abort status ([10]) & set abort ([1])
            RegStatus |= (1 << 10) | (1 << 1);

            // Abort transfer
            PLX_DMA_REG_WRITE( pdx, OffsetDmaBase + 0x38, RegStatus );

            // Set poll count
            LoopCount = 500;

            // Poll abort done bit
            do
            {
                RegStatus = PLX_DMA_REG_READ( pdx, OffsetDmaBase + 0x38 );

                LoopCount--;
                if (LoopCount == 0)
                {
                    return PLX_STATUS_TIMEOUT;
                }
            }
            while ((RegStatus & (1 << 10)) == 0);

            // Clear any active status bits ([31,12:8])
            RegStatus &= ~((1 << 31) | (0x1F << 8));

            // Clear abort done status ([10])
            RegStatus |= (1 << 10);

            // Clear abort ([1])
            RegStatus &= ~(1 << 1);

            PLX_DMA_REG_WRITE( pdx, OffsetDmaBase + 0x38, RegStatus );
            break;

        default:
            return PLX_STATUS_INVALID_DATA;
    }

    return PLX_STATUS_OK;
}




/******************************************************************************
 *
 * Function   :  PlxDmaStatus
 *
 * Description:  Get status of a DMA channel
 *
 ******************************************************************************/
PLX_STATUS
PlxDmaStatus(
    DEVICE_EXTENSION *pdx,
    U8                channel,
    VOID             *pOwner
    )
{
    U16 OffsetDmaBase;
    U32 RegValue;


    // Verify valid DMA channel
    switch (channel)
    {
        case 0:
        case 1:
        case 2:
        case 3:
            if (channel >= pdx->NumDmaChannels)
            {
                DebugPrintf((
                    "Error - Channel %d exceeds max supported (%d)\n",
                    channel, (pdx->NumDmaChannels - 1)
                    ));
                return PLX_STATUS_INVALID_ACCESS;
            }
            break;

        default:
            DebugPrintf(("ERROR - Invalid DMA channel\n"));
            return PLX_STATUS_INVALID_ACCESS;
    }

    // Verify ownership if requested
    if (pOwner != NULL)
    {
        // Verify DMA has been opened
        if (pdx->DmaInfo[channel].bOpen == FALSE)
        {
            DebugPrintf(("ERROR - DMA channel has not been opened\n"));
            return PLX_STATUS_INVALID_ACCESS;
        }

        // Verify owner
        if (pdx->DmaInfo[channel].pOwner != pOwner)
        {
            DebugPrintf(("ERROR - DMA owned by different process\n"));
            return PLX_STATUS_IN_USE;
        }
    }

    // Verify channel is enabled in hardware
    if (pdx->Key.PlxFamily == PLX_FAMILY_SIRIUS)
    {
        RegValue = PLX_DMA_REG_READ( pdx, 0x1FC );

        // Check DMA channel setup (1FC[1:0])
        switch (RegValue & 0x3)
        {
            case 0:
                // All 4 channels active
                break;

            case 1:
                // Only channel 0 active
                if (channel != 0)
                {
                    return PLX_STATUS_INVALID_ACCESS;
                }
                break;

            case 2:
                // Channels 0 & 2 active
                if ((channel != 0) && (channel != 2))
                {
                    return PLX_STATUS_INVALID_ACCESS;
                }
                break;

            case 3:
                // Channels 0, 1, & 2 active
                if (channel == 3)
                {
                    return PLX_STATUS_INVALID_ACCESS;
                }
                break;
        }
    }

    // Set the channel's base register offset (200h, 300h, etc)
    OffsetDmaBase = 0x200 + (channel * 0x100);

    // Get DMA status
    RegValue = PLX_DMA_REG_READ( pdx, OffsetDmaBase + 0x38 );

    // Check if DMA is in progress
    if (RegValue & (1 << 30))
    {
        // Check if DMA is paused
        if (RegValue & (1 << 0))
        {
            DebugPrintf(("DMA is paused\n"));
            return PLX_STATUS_PAUSED;
        }

        DebugPrintf(("DMA is in-progress\n"));
        return PLX_STATUS_IN_PROGRESS;
    }

    return PLX_STATUS_COMPLETE;
}




/******************************************************************************
 *
 * Function   :  PlxDmaTransferBlock
 *
 * Description:  Performs DMA block transfer
 *
 ******************************************************************************/
PLX_STATUS
PlxDmaTransferBlock(
    DEVICE_EXTENSION *pdx,
    U8                channel,
    PLX_DMA_PARAMS   *pParams,
    VOID             *pOwner
    )
{
    U16        OffsetDmaBase;
    U32        RegValue;
    PLX_STATUS status;


    // Verify DMA channel is available
    status =
        PlxDmaStatus(
            pdx,
            channel,
            pOwner
            );

    if (status != PLX_STATUS_COMPLETE)
    {
        DebugPrintf(("ERROR - DMA unavailable or in-progress\n"));
        return status;
    }

    DebugPrintf((
        "Ch %d - DMA %08X_%08X --> %08X_%08X (%d bytes)\n",
        channel, PLX_64_HIGH_32(pParams->AddrSource), PLX_64_LOW_32(pParams->AddrSource),
        PLX_64_HIGH_32(pParams->AddrDest), PLX_64_LOW_32(pParams->AddrDest),
        pParams->ByteCount
        ));

    // Set the channel's base register offset (200h, 300h, etc)
    OffsetDmaBase = 0x200 + (channel * 0x100);

    spin_lock(
        &(pdx->Lock_Dma[channel])
        );

    // Write Source Address
    PLX_DMA_REG_WRITE( pdx, OffsetDmaBase + 0x0, PLX_64_LOW_32(pParams->AddrSource) );
    PLX_DMA_REG_WRITE( pdx, OffsetDmaBase + 0x4, PLX_64_HIGH_32(pParams->AddrSource) );

    // Write Destination Address
    PLX_DMA_REG_WRITE( pdx, OffsetDmaBase + 0x8, PLX_64_LOW_32(pParams->AddrDest) );
    PLX_DMA_REG_WRITE( pdx, OffsetDmaBase + 0xC, PLX_64_HIGH_32(pParams->AddrDest) );

    // Set Transfer Count & address & interrupt options
    RegValue =
        (1                       << 31) |   // Valid bit
        (pParams->bConstAddrSrc  << 29) |   // Keep source address constant
        (pParams->bConstAddrDest << 28) |   // Keep destination address constant
        (pParams->ByteCount      <<  0);    // Byte count
    if (pParams->bIgnoreBlockInt == 0)
    {
        RegValue |= (1 << 30);
    }
    PLX_DMA_REG_WRITE( pdx, OffsetDmaBase + 0x10, RegValue );

    // Get DMA control/status
    RegValue = PLX_DMA_REG_READ( pdx, OffsetDmaBase + 0x38 );

    // Set DMA to block mode
    if (pdx->Key.PlxFamily == PLX_FAMILY_SIRIUS)
    {
        RegValue &= ~(1 << 4);
    }
    else
    {
        RegValue &= ~(3 << 5);
    }

    // Make sure descriptor write-back ([2]) is disabled
    RegValue &= ~(1 << 2);

    // Clear any active status bits ([31,12:8])
    RegValue |= ((1 << 31) | (0x1F << 8));

    spin_unlock(
        &(pdx->Lock_Dma[channel])
        );

    DebugPrintf(("Start DMA transfer...\n"));

    // Start DMA ([3])
    PLX_DMA_REG_WRITE( pdx, OffsetDmaBase + 0x38, RegValue | (1 << 3) );

    return PLX_STATUS_OK;
}




/******************************************************************************
 *
 * Function   :  PlxDmaTransferUserBuffer
 *
 * Description:  Transfers a user-mode buffer using SGL DMA
 *
 ******************************************************************************/
PLX_STATUS
PlxDmaTransferUserBuffer(
    DEVICE_EXTENSION *pdx,
    U8                channel,
    PLX_DMA_PARAMS   *pParams,
    VOID             *pOwner
    )
{
    U16        OffsetDmaBase;
    U32        RegValue;
    U32        NumDescriptors;
    U64        SglPciAddress;
    PLX_STATUS status;


    // Verify DMA channel is available
    status =
        PlxDmaStatus(
            pdx,
            channel,
            pOwner
            );

    if (status != PLX_STATUS_COMPLETE)
    {
        DebugPrintf(("ERROR - DMA unavailable or in-progress\n"));
        return status;
    }

    spin_lock(
        &(pdx->Lock_Dma[channel])
        );

    // Verify an SGL DMA transfer is not pending
    if (pdx->DmaInfo[channel].bSglPending)
    {
        DebugPrintf(("ERROR - An SGL DMA transfer is currently pending\n"));
        spin_unlock( &(pdx->Lock_Dma[channel]) );
        return PLX_STATUS_IN_PROGRESS;
    }

    // Set the SGL DMA pending flag
    pdx->DmaInfo[channel].bSglPending = TRUE;

    spin_unlock(
        &(pdx->Lock_Dma[channel])
        );

    // Page-lock user buffer & build SGL
    status =
        PlxLockBufferAndBuildSgl(
            pdx,
            channel,
            pParams,
            &SglPciAddress,
            &NumDescriptors
            );

    if (status != PLX_STATUS_OK)
    {
        DebugPrintf(("ERROR - Unable to lock buffer and build SGL list\n"));
        pdx->DmaInfo[channel].bSglPending = FALSE;
        return status;
    }

    // Make sure DMA descriptors are set to external ([2] = 0)
    if (pdx->Key.PlxFamily == PLX_FAMILY_SIRIUS)
    {
        RegValue = PLX_DMA_REG_READ( pdx, 0x1FC );
        PLX_DMA_REG_WRITE( pdx, 0x1FC, RegValue & ~(1 << 2) );
    }

    // Set the channel's base register offset (200h, 300h, etc)
    OffsetDmaBase = 0x200 + (channel * 0x100);

    // Verify DMA prefetch doesn't exceed descriptor count & is a multiple of 4
    if (NumDescriptors < 4)
    {
        RegValue = 1;
    }
    else if (NumDescriptors >= 256)
    {
        RegValue = 0;
    }
    else
    {
        RegValue = (NumDescriptors & (U8)~0x3);
    }
    PLX_DMA_REG_WRITE( pdx, OffsetDmaBase + 0x34, RegValue );

    spin_lock(
        &(pdx->Lock_Dma[channel])
        );

    // Clear all DMA registers
    PLX_DMA_REG_WRITE( pdx, OffsetDmaBase + 0x00, 0 );
    PLX_DMA_REG_WRITE( pdx, OffsetDmaBase + 0x04, 0 );
    PLX_DMA_REG_WRITE( pdx, OffsetDmaBase + 0x08, 0 );
    PLX_DMA_REG_WRITE( pdx, OffsetDmaBase + 0x0C, 0 );
    PLX_DMA_REG_WRITE( pdx, OffsetDmaBase + 0x10, 0 );

    // Descriptor ring address
    PLX_DMA_REG_WRITE( pdx, OffsetDmaBase + 0x14, PLX_64_LOW_32(SglPciAddress) );
    PLX_DMA_REG_WRITE( pdx, OffsetDmaBase + 0x18, PLX_64_HIGH_32(SglPciAddress) );

    // Current descriptor address
    PLX_DMA_REG_WRITE( pdx, OffsetDmaBase + 0x1C, PLX_64_LOW_32(SglPciAddress) );

    // Descriptor ring size
    PLX_DMA_REG_WRITE( pdx, OffsetDmaBase + 0x20, NumDescriptors );

    // Current descriptor transfer size
    PLX_DMA_REG_WRITE( pdx, OffsetDmaBase + 0x28, 0 );

    // Disable invalid descriptor interrupt (x3C[1])
    RegValue = PLX_DMA_REG_READ( pdx, OffsetDmaBase + 0x3C );
    RegValue &= ~(1 << 1);
    PLX_DMA_REG_WRITE( pdx, OffsetDmaBase + 0x3C, RegValue );

    // Get DMA control/status
    RegValue = PLX_DMA_REG_READ( pdx, OffsetDmaBase + 0x38 );

    // Make sure descriptor write-back ([2]) is disabled
    RegValue &= ~(1 << 2);

    // Clear any active status bits ([31,12:8])
    RegValue |= ((1 << 31) | (0x1F << 8));

    // Enable SGL off-chip mode & descriptor fetch stops at end
    if (pdx->Key.PlxFamily == PLX_FAMILY_SIRIUS)
    {
        RegValue |= (1 << 5) | (1 << 4);        // SGL mode (4) & descriptor halt mode (5)
    }
    else
    {
        RegValue &= ~(3 << 5);
        RegValue |= (2 << 5) | (1 << 4);        // SGL mode ([6:5]) & descriptor halt mode (4)
    }

    spin_unlock(
        &(pdx->Lock_Dma[channel])
        );

    DebugPrintf(("Start DMA transfer...\n"));

    // Start DMA (x38[3])
    PLX_DMA_REG_WRITE( pdx, OffsetDmaBase + 0x38, RegValue | (1 << 3) );

    return PLX_STATUS_OK;
}




/******************************************************************************
 *
 * Function   :  PlxDmaChannelClose
 *
 * Description:  Close a previously opened channel
 *
 ******************************************************************************/
PLX_STATUS
PlxDmaChannelClose(
    DEVICE_EXTENSION *pdx,
    U8                channel,
    BOOLEAN           bCheckInProgress,
    VOID             *pOwner
    )
{
    PLX_STATUS status;


    DebugPrintf(("Closing DMA channel %d...\n", channel));

    // Check DMA status
    status =
        PlxDmaStatus(
            pdx,
            channel,
            pOwner
            );

    // Verify DMA is not in progress
    if (status != PLX_STATUS_COMPLETE)
    {
        // Check for errors
        if ((status != PLX_STATUS_IN_PROGRESS) && (status != PLX_STATUS_PAUSED))
        {
            return status;
        }

        // DMA is still in progress
        if (bCheckInProgress)
        {
            return status;
        }

        DebugPrintf(("DMA in progress, aborting...\n"));

        // Force DMA abort, which may generate a DMA done interrupt
        PlxDmaControl(
            pdx,
            channel,
            DmaAbort,
            pOwner
            );

        // Small delay to let driver cleanup if DMA interrupts
        Plx_sleep( 100 );
    }

    spin_lock(
        &(pdx->Lock_Dma[channel])
        );

    // Close the channel
    pdx->DmaInfo[channel].bOpen = FALSE;

    // Clear owner information
    pdx->DmaInfo[channel].pOwner = NULL;

    spin_unlock(
        &(pdx->Lock_Dma[channel])
        );

    // If DMA is hung, an SGL transfer could be pending, so release user buffer
    if (pdx->DmaInfo[channel].bSglPending)
    {
        PlxSglDmaTransferComplete(
            pdx,
            channel
            );
    }

    // Release memory previously used for SGL descriptors
    if (pdx->DmaInfo[channel].SglBuffer.pKernelVa != NULL)
    {
        DebugPrintf(("Releasing memory used for SGL descriptors...\n"));

        Plx_dma_buffer_free(
            pdx,
            &pdx->DmaInfo[channel].SglBuffer
            );
    }

    return PLX_STATUS_OK;
}
