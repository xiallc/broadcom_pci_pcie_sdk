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
#include "Eep_6000.h"
#include "PciFunc.h"
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
    DebugPrintf(("Device does not support PCI Express Capability\n"));

    // Set default properties
    RtlZeroMemory(pPortProp, sizeof(PLX_PORT_PROP));

    // Mark device as non-PCIe
    pPortProp->bNonPcieDevice = TRUE;

    // Default to a legacy endpoint
    pPortProp->PortType = PLX_PORT_LEGACY_ENDPOINT;

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
    // 6000 series devices do not have PLX-specific local registers
    *pStatus = PLX_STATUS_UNSUPPORTED;
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
    DEVICE_EXTENSION *pdx,
    U32               offset,
    U32               value,
    BOOLEAN           bAdjustForPort
    )
{
    // 6000 series devices do not have PLX-specific local registers
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
    BOOLEAN bFlag;


    /**************************************************************
     * Since PCI 6000 devices do not report whether an EEPROM
     * is present or not, the driver manually probes for it.
     *************************************************************/

    // Probe for EEPROM
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
    U16        OffsetProbe;
    U16        ValueRead;
    U16        ValueWrite;
    U16        ValueOriginal;
    PLX_STATUS status;


    // Default to no EEPROM present
    *pFlag = FALSE;

    // Set EEPROM offset to use for probe
    OffsetProbe = 0x50;     // Use area outside of PLX portion

    DebugPrintf(("Probe EEPROM at offset %02xh\n", OffsetProbe));

    // Get the current value
    status =
        PlxEepromReadByOffset_16(
            pdx,
            OffsetProbe,
            &ValueOriginal
            );

    if (status != PLX_STATUS_OK)
    {
        return status;
    }

    // Prepare inverse value to write
    ValueWrite = (U16)~ValueOriginal;

    // Write inverse of original value
    status =
        PlxEepromWriteByOffset_16(
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
        PlxEepromReadByOffset_16(
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
        PlxEepromWriteByOffset_16(
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
    // Make sure offset is aligned on 32-bit boundary
    if (offset & (3 << 0))
    {
        *pValue = 0;
        return PLX_STATUS_INVALID_OFFSET;
    }

    Plx6000_EepromReadByOffset_16(
        pdx,
        offset,
        (U16*)pValue
        );

    Plx6000_EepromReadByOffset_16(
        pdx,
        offset + sizeof(U16),
        (U16*)((U8*)pValue + sizeof(U16))
        );

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
    DEVICE_EXTENSION *pdx,
    U32               offset,
    U32               value
    )
{
    // Make sure offset is aligned on 32-bit boundary
    if (offset & (3 << 0))
    {
        return PLX_STATUS_INVALID_OFFSET;
    }

    Plx6000_EepromWriteByOffset_16(
        pdx,
        offset,
        (U16)value
        );

    Plx6000_EepromWriteByOffset_16(
        pdx,
        offset + sizeof(U16),
        (U16)(value >> 16)
        );

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
    DEVICE_EXTENSION *pdx,
    U32               offset,
    U16              *pValue
    )
{
    // Make sure offset is aligned on 16-bit boundary
    if (offset & (1 << 0))
    {
        *pValue = 0;
        return PLX_STATUS_INVALID_OFFSET;
    }

    return Plx6000_EepromReadByOffset_16(
        pdx,
        offset,
        pValue
        );
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
    // Make sure offset is aligned on 16-bit boundary
    if (offset & (1 << 0))
    {
        return PLX_STATUS_INVALID_OFFSET;
    }

    return Plx6000_EepromWriteByOffset_16(
        pdx,
        offset,
        value
        );
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
    U32          RegValue;
    PLX_REG_DATA RegData;


    // Only 16 doorbell interrupts are supported
    pPlxIntr->Doorbell &= 0xFFFF;

    // Enable doorbell interrupts
    if (pPlxIntr->Doorbell)
    {
        PLX_PCI_REG_READ(
            pdx,
            0xc4,
            &RegValue
            );

        RegValue |= pPlxIntr->Doorbell;

        PLX_PCI_REG_WRITE(
            pdx,
            0xc4,
            RegValue
            );
    }

    // Setup to synchronize access to interrupt register
    RegData.pdx         = pdx;
    RegData.offset      = 0xc8;
    RegData.BitsToSet   = 0;
    RegData.BitsToClear = 0;

    if (pPlxIntr->Message & (1 << 0))
    {
        RegData.BitsToSet |= (1 << 24);
    }

    if (pPlxIntr->Message & (1 << 1))
    {
        RegData.BitsToSet |= (1 << 25);
    }

    if (pPlxIntr->Message & (1 << 2))
    {
        RegData.BitsToSet |= (1 << 26);
    }

    if (pPlxIntr->Message & (1 << 3))
    {
        RegData.BitsToSet |= (1 << 27);
    }

    if (pPlxIntr->ResetDeassert)
    {
        RegData.BitsToSet |= (1 << 28);
    }

    if (pPlxIntr->PmeDeassert)
    {
        RegData.BitsToSet |= (1 << 29);
    }

    if (pPlxIntr->GPIO_14_15)
    {
        RegData.BitsToSet |= (1 << 30);
    }

    if (pPlxIntr->GPIO_4_5)
    {
        RegData.BitsToSet |= (1 << 31);
    }

    // Write register values if they have changed
    if (RegData.BitsToSet != 0)
    {
        // Synchronize write to interrupt register
        PlxSynchronizedRegisterModify(
            &RegData
            );
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
    U32          RegValue;
    PLX_REG_DATA RegData;


    // Only 16 doorbell interrupts are supported
    pPlxIntr->Doorbell &= 0xFFFF;

    // Disable doorbell interrupts
    if (pPlxIntr->Doorbell)
    {
        PLX_PCI_REG_READ(
            pdx,
            0xc4,
            &RegValue
            );

        // Clear doorbell interrupts that are set
        RegValue &= 0xFFFF0000 | (~pPlxIntr->Doorbell & 0xFFFF);

        PLX_PCI_REG_WRITE(
            pdx,
            0xc4,
            RegValue
            );
    }

    // Setup to synchronize access to interrupt register
    RegData.pdx         = pdx;
    RegData.offset      = 0xc8;
    RegData.BitsToSet   = 0;
    RegData.BitsToClear = 0;

    if (pPlxIntr->Message & (1 << 0))
    {
        RegData.BitsToClear |= (1 << 24);
    }

    if (pPlxIntr->Message & (1 << 1))
    {
        RegData.BitsToClear |= (1 << 25);
    }

    if (pPlxIntr->Message & (1 << 2))
    {
        RegData.BitsToClear |= (1 << 26);
    }

    if (pPlxIntr->Message & (1 << 3))
    {
        RegData.BitsToClear |= (1 << 27);
    }

    if (pPlxIntr->ResetDeassert)
    {
        RegData.BitsToClear |= (1 << 28);
    }

    if (pPlxIntr->PmeDeassert)
    {
        RegData.BitsToClear |= (1 << 29);
    }

    if (pPlxIntr->GPIO_14_15)
    {
        RegData.BitsToClear |= (1 << 30);
    }

    if (pPlxIntr->GPIO_4_5)
    {
        RegData.BitsToClear |= (1 << 31);
    }

    // Write register values if they have changed
    if (RegData.BitsToClear != 0)
    {
        // Synchronize write to interrupt register
        PlxSynchronizedRegisterModify(
            &RegData
            );
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
    pWaitObject->Source_Ints     = INTR_TYPE_NONE;
    pWaitObject->Source_Doorbell = 0;

    // Set interrupt notification flags
    pWaitObject->Notify_Flags    = INTR_TYPE_NONE;
    pWaitObject->Notify_Doorbell = pPlxIntr->Doorbell;

    if (pPlxIntr->ResetDeassert)
    {
        pWaitObject->Notify_Flags |= INTR_TYPE_RSTIN;
    }

    if (pPlxIntr->PmeDeassert)
    {
        pWaitObject->Notify_Flags |= INTR_TYPE_PME;
    }

    if (pPlxIntr->GPIO_4_5)
    {
        pWaitObject->Notify_Flags |= INTR_TYPE_GPIO_4_5;
    }

    if (pPlxIntr->GPIO_14_15)
    {
        pWaitObject->Notify_Flags |= INTR_TYPE_GPIO_14_15;
    }

    if (pPlxIntr->Message & (1 << 0))
    {
        pWaitObject->Notify_Flags |= INTR_TYPE_MESSAGE_0;
    }

    if (pPlxIntr->Message & (1 << 1))
    {
        pWaitObject->Notify_Flags |= INTR_TYPE_MESSAGE_1;
    }

    if (pPlxIntr->Message & (1 << 2))
    {
        pWaitObject->Notify_Flags |= INTR_TYPE_MESSAGE_2;
    }

    if (pPlxIntr->Message & (1 << 3))
    {
        pWaitObject->Notify_Flags |= INTR_TYPE_MESSAGE_3;
    }

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
            IntData.Source_Ints     = pWaitObject->Source_Ints;
            IntData.Source_Doorbell = pWaitObject->Source_Doorbell;

            // Reset interrupt sources
            pWaitObject->Source_Ints     = INTR_TYPE_NONE;
            pWaitObject->Source_Doorbell = 0;

            spin_unlock_irqrestore(
                &(pdx->Lock_WaitObjectsList),
                flags
                );

            DebugPrintf((
                "Returning status for interrupt wait object (%p)...\n",
                pWaitObject
                ));

            // Set triggered interrupts
            RtlZeroMemory( pPlxIntr, sizeof(PLX_INTERRUPT) );

            pPlxIntr->Doorbell = IntData.Source_Doorbell;

            if (IntData.Source_Ints & INTR_TYPE_RSTIN)
            {
                pPlxIntr->ResetDeassert = 1;
            }

            if (IntData.Source_Ints & INTR_TYPE_PME)
            {
                pPlxIntr->PmeDeassert = 1;
            }

            if (IntData.Source_Ints & INTR_TYPE_GPIO_4_5)
            {
                pPlxIntr->GPIO_4_5 = 1;
            }

            if (IntData.Source_Ints & INTR_TYPE_GPIO_14_15)
            {
                pPlxIntr->GPIO_14_15 = 1;
            }

            if (IntData.Source_Ints & INTR_TYPE_MESSAGE_0)
            {
                pPlxIntr->Message |= (1 << 0);
            }

            if (IntData.Source_Ints & INTR_TYPE_MESSAGE_1)
            {
                pPlxIntr->Message |= (1 << 1);
            }

            if (IntData.Source_Ints & INTR_TYPE_MESSAGE_2)
            {
                pPlxIntr->Message |= (1 << 2);
            }

            if (IntData.Source_Ints & INTR_TYPE_MESSAGE_3)
            {
                pPlxIntr->Message |= (1 << 3);
            }

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
