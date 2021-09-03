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

/******************************************************************************
 *
 * File Name:
 *
 *      Dispatch.c
 *
 * Description:
 *
 *      This file routes incoming I/O Request packets
 *
 * Revision History:
 *
 *      03-01-19 : PCI/PCIe SDK v8.00
 *
 ******************************************************************************/


#include <linux/uaccess.h>  // For copy_to/from_user()
#include "ApiFunc.h"
#include "Dispatch.h"
#include "Driver.h"
#include "PciFunc.h"
#include "PlxIoctl.h"
#include "SuppFunc.h"




/******************************************************************************
 *
 * Function   :  Dispatch_open
 *
 * Description:  Handle open() which allows applications to create a
 *               connection to the driver
 *
 ******************************************************************************/
int
Dispatch_open(
    struct inode *inode,
    struct file  *filp
    )
{
    U8             i;
    DEVICE_OBJECT *fdo;


    DebugPrintf_Cont((" \n"));
    DebugPrintf(("Received message ==> OPEN_DEVICE\n"));

    if (iminor(inode) == PLX_MNGMT_INTERFACE)
    {
        DebugPrintf(("Open Management interface...\n"));

        // Store the driver object in the private data
        filp->private_data = pGbl_DriverObject;
    }
    else
    {
        // Select desired device from device list
        i   = iminor(inode);
        fdo = pGbl_DriverObject->DeviceObject;

        while (i-- && fdo != NULL)
        {
           fdo = fdo->NextDevice;
        }

        if (fdo == NULL)
        {
            ErrorPrintf(("WARNING - Attempt to open non-existent device\n"));
            return (-ENODEV);
        }

        DebugPrintf((
            "Open device (%s)...\n",
            fdo->DeviceExtension->LinkName
            ));

        // Store device object for future calls
        filp->private_data = fdo;
    }

    DebugPrintf(("...device opened\n"));
    return 0;
}




/******************************************************************************
 *
 * Function   :  Dispatch_release
 *
 * Description:  Handle close() call, which closes the connection between the
 *               application and drivers.
 *
 ******************************************************************************/
int
Dispatch_release(
    struct inode *inode,
    struct file  *filp
    )
{
    DEVICE_OBJECT *fdo;


    DebugPrintf_Cont((" \n"));
    DebugPrintf(("Received message ==> CLOSE_DEVICE\n"));

    if (iminor(inode) == PLX_MNGMT_INTERFACE)
    {
        DebugPrintf(("Close Management interface...\n"));

        // Clear the driver object from the private data
        filp->private_data = NULL;
    }
    else
    {
        // Get the device object
        fdo = (DEVICE_OBJECT *)(filp->private_data);

        DebugPrintf((
            "Close device (%s)...\n",
            fdo->DeviceExtension->LinkName
            ));

        // Release any pending notifications owned by proccess
        PlxNotificationCancel(
            fdo->DeviceExtension,
            NULL,
            filp
            );

        // Release any physical memory allocated by process
        PlxPciPhysicalMemoryFreeAll_ByOwner(
            fdo->DeviceExtension,
            filp
            );
    }

    DebugPrintf(("...device closed\n"));
    return 0;
}




/******************************************************************************
 *
 * Function   :  Dispatch_mmap
 *
 * Description:  Maps a PCI space into user virtual space
 *
 ******************************************************************************/
int
Dispatch_mmap(
    struct file           *filp,
    struct vm_area_struct *vma
    )
{
    int               rc;
    off_t             offset;
    BOOLEAN           bDeviceMem;
    U64               AddressToMap;
    DEVICE_EXTENSION *pdx;


    DebugPrintf_Cont((" \n"));
    DebugPrintf(("Received message ===> MMAP\n"));

    // Get device extension
    pdx = ((DEVICE_OBJECT*)(filp->private_data))->DeviceExtension;

    // Get the supplied offset
    offset = vma->vm_pgoff;

    // Determine if mapping to a PCI BAR or system memory
    switch (offset)
    {
        case 0:
        case 1:
        case 2:
        case 3:
        case 4:
        case 5:
            // Verify space is not I/O
            if (pdx->PciBar[offset].Properties.Flags & PLX_BAR_FLAG_IO)
            {
                DebugPrintf((
                    "ERROR - PCI BAR %d is an I/O space, cannot map to user space\n",
                    (U8)offset
                    ));
                return -ENODEV;
            }

            DebugPrintf(("Map PCI BAR %d...\n", (U8)offset));

            // Use the BAR physical address for the mapping
            AddressToMap = pdx->PciBar[offset].Properties.Physical;

            // Flag that the mapping is to IO memory
            bDeviceMem = TRUE;
            break;

        default:
            // Use provided offset as CPU physical address for mapping
            AddressToMap = (U64)offset << PAGE_SHIFT;

            // Flag that the mapping is to system memory
            bDeviceMem = FALSE;
            break;
    }

    // Verify physical address
    if (AddressToMap == 0)
    {
        DebugPrintf((
            "ERROR - Invalid physical (%08llx), cannot map to user space\n",
            AddressToMap
            ));
        return -ENODEV;
    }

    /***********************************************************
     * Attempt to map the region
     *
     * NOTE:
     *
     * Due to variations in the remap function between kernel releases
     * and distributions, a PLX macro is used to simplify code
     * readability.  For additional information about the macro
     * expansions, refer to the file "Plx_sysdep.h".
     **********************************************************/

    // Set the region as page-locked
    vma->vm_flags |= VM_RESERVED;

    if (bDeviceMem)
    {
        // Set flag for I/O resource
        vma->vm_flags |= VM_IO;

        // Set caching based on BAR properties
        if (pdx->PciBar[offset].Properties.Flags & PLX_BAR_FLAG_PREFETCHABLE)
        {
            vma->vm_page_prot = pgprot_writecombine( vma->vm_page_prot );
        }
        else
        {
            vma->vm_page_prot = pgprot_noncached( vma->vm_page_prot );
        }

        // Map device memory
        rc =
            io_remap_pfn_range(
                vma,
                vma->vm_start,
                AddressToMap >> PAGE_SHIFT,
                vma->vm_end - vma->vm_start,
                vma->vm_page_prot
                );
    }
    else
    {
        // Map system memory
        rc =
            remap_pfn_range(
                vma,
                vma->vm_start,
                AddressToMap >> PAGE_SHIFT,
                vma->vm_end - vma->vm_start,
                vma->vm_page_prot
                );
    }

    if (rc != 0)
    {
        ErrorPrintf((
            "ERROR - Unable to map Physical (%08llx) ==> User space\n",
            AddressToMap
            ));
    }
    else
    {
        DebugPrintf((
            "Mapped Phys (%08llx) ==> User VA (%08lx)\n",
            AddressToMap, vma->vm_start
            ));
    }

    DebugPrintf(("...Completed message\n"));
    return rc;
}




/******************************************************************************
 *
 * Function   :  Dispatch_IoControl
 *
 * Description:  Processes the IOCTL messages sent to this device
 *
 ******************************************************************************/
long
Dispatch_IoControl(
    struct file   *filp,
    unsigned int   cmd,
    unsigned long  args
    )
{
    int               status;
    VOID             *pOwner;
    PLX_PARAMS        IoBuffer;
    PLX_PARAMS       *pIoBuffer;
    DEVICE_EXTENSION *pdx;


    DebugPrintf_Cont((" \n"));

    // Get the device extension
    if (iminor(filp->f_path.dentry->d_inode) == PLX_MNGMT_INTERFACE)
    {
        // Management interface node only supports some IOCTLS
        pdx = NULL;
    }
    else
    {
        pdx = ((DEVICE_OBJECT*)(filp->private_data))->DeviceExtension;
    }

    // Copy the I/O Control message from user space
    status =
        copy_from_user(
            &IoBuffer,
            (PLX_PARAMS*)args,
            sizeof(PLX_PARAMS)
            );

    if (status != 0)
    {
        ErrorPrintf((
            "ERROR - Unable to copy user I/O message data\n"
            ));

        return (-EFAULT);
    }

    // Track the owner
    pOwner = filp;

    pIoBuffer = &IoBuffer;

    DebugPrintf(("Received PLX message ===> "));

    // Handle the PLX specific message
    switch (cmd)
    {
        /******************************************
         * Driver Query Functions
         *****************************************/
        case PLX_IOCTL_PCI_DEVICE_FIND:
            DebugPrintf_Cont(("PLX_IOCTL_PCI_DEVICE_FIND\n"));

            pIoBuffer->ReturnCode =
                PlxDeviceFind(
                    pdx,
                    &(pIoBuffer->Key),
                    PLX_CAST_64_TO_16_PTR( &(pIoBuffer->value[0]) )
                    );
            break;

        case PLX_IOCTL_DRIVER_VERSION:
            DebugPrintf_Cont(("PLX_IOCTL_DRIVER_VERSION\n"));

            pIoBuffer->value[0] =
                (PLX_SDK_VERSION_MAJOR << 16) |
                (PLX_SDK_VERSION_MINOR <<  8) |
                (0                     <<  0);
            break;

        case PLX_IOCTL_CHIP_TYPE_GET:
            DebugPrintf_Cont(("PLX_IOCTL_CHIP_TYPE_GET\n"));

            pIoBuffer->ReturnCode =
                PlxChipTypeGet(
                    pdx,
                    PLX_CAST_64_TO_16_PTR( &(pIoBuffer->value[0]) ),
                    PLX_CAST_64_TO_8_PTR ( &(pIoBuffer->value[1]) )
                    );
            break;

        case PLX_IOCTL_CHIP_TYPE_SET:
            DebugPrintf_Cont(("PLX_IOCTL_CHIP_TYPE_SET\n"));

            pIoBuffer->ReturnCode =
                PlxChipTypeSet(
                    pdx,
                    (U16)pIoBuffer->value[0],
                    (U8)pIoBuffer->value[1]
                    );
            break;

        case PLX_IOCTL_GET_PORT_PROPERTIES:
            DebugPrintf_Cont(("PLX_IOCTL_GET_PORT_PROPERTIES\n"));

            pIoBuffer->ReturnCode =
                PlxGetPortProperties(
                    pdx,
                    &(pIoBuffer->u.PortProp)
                    );
            break;


        /******************************************
         * Device Control Functions
         *****************************************/
        case PLX_IOCTL_PCI_DEVICE_RESET:
            DebugPrintf_Cont(("PLX_IOCTL_PCI_DEVICE_RESET\n"));

            pIoBuffer->ReturnCode =
                PlxPciDeviceReset(
                    pdx
                    );
            break;


        /******************************************
         * PCI Register Access Functions
         *****************************************/
        case PLX_IOCTL_PCI_REGISTER_READ:
            DebugPrintf_Cont(("PLX_IOCTL_PCI_REGISTER_READ\n"));

            pIoBuffer->ReturnCode =
                PlxPciRegisterRead_UseOS(
                    pdx->pPciDevice,
                    (U16)pIoBuffer->value[0],
                    PLX_CAST_64_TO_32_PTR( &(pIoBuffer->value[1]) )
                    );

            DebugPrintf((
                "PCI Reg %03X = %08X\n",
                (U16)pIoBuffer->value[0],
                (U32)pIoBuffer->value[1]
                ));
            break;

        case PLX_IOCTL_PCI_REGISTER_WRITE:
            DebugPrintf_Cont(("PLX_IOCTL_PCI_REGISTER_WRITE\n"));

            pIoBuffer->ReturnCode =
                PlxPciRegisterWrite_UseOS(
                    pdx->pPciDevice,
                    (U16)pIoBuffer->value[0],
                    (U32)pIoBuffer->value[1]
                    );

            DebugPrintf((
                "Wrote %08X to PCI Reg %03X\n",
                (U32)pIoBuffer->value[1],
                (U16)pIoBuffer->value[0]
                ));
            break;

        case PLX_IOCTL_PCI_REG_READ_BYPASS_OS:
            DebugPrintf_Cont(("PLX_IOCTL_PCI_REG_READ_BYPASS_OS\n"));

            pIoBuffer->ReturnCode =
                PlxPciRegisterRead_BypassOS(
                    pIoBuffer->Key.domain,
                    pIoBuffer->Key.bus,
                    pIoBuffer->Key.slot,
                    pIoBuffer->Key.function,
                    (U16)pIoBuffer->value[0],
                    PLX_CAST_64_TO_32_PTR( &(pIoBuffer->value[1]) )
                    );
            break;

        case PLX_IOCTL_PCI_REG_WRITE_BYPASS_OS:
            DebugPrintf_Cont(("PLX_IOCTL_PCI_REG_WRITE_BYPASS_OS\n"));

            pIoBuffer->ReturnCode =
                PlxPciRegisterWrite_BypassOS(
                    pIoBuffer->Key.domain,
                    pIoBuffer->Key.bus,
                    pIoBuffer->Key.slot,
                    pIoBuffer->Key.function,
                    (U16)pIoBuffer->value[0],
                    (U32)pIoBuffer->value[1]
                    );
            break;


        /******************************************
         * PLX-specific Register Access Functions
         *****************************************/
        case PLX_IOCTL_REGISTER_READ:
            DebugPrintf_Cont(("PLX_IOCTL_REGISTER_READ\n"));

            pIoBuffer->value[1] =
                PlxRegisterRead(
                    pdx,
                    (U32)pIoBuffer->value[0],
                    &(pIoBuffer->ReturnCode),
                    TRUE        // Adjust offset based on port
                    );

            DebugPrintf((
                "PLX Reg %03X = %08X\n",
                (U32)pIoBuffer->value[0],
                (U32)pIoBuffer->value[1]
                ));
            break;

        case PLX_IOCTL_REGISTER_WRITE:
            DebugPrintf_Cont(("PLX_IOCTL_REGISTER_WRITE\n"));

            pIoBuffer->ReturnCode =
                PlxRegisterWrite(
                    pdx,
                    (U32)pIoBuffer->value[0],
                    (U32)pIoBuffer->value[1],
                    TRUE        // Adjust offset based on port
                    );

            DebugPrintf((
                "Wrote %08X to PLX Reg %03X\n",
                (U32)pIoBuffer->value[1],
                (U32)pIoBuffer->value[0]
                ));
            break;

        case PLX_IOCTL_MAPPED_REGISTER_READ:
            DebugPrintf_Cont(("PLX_IOCTL_MAPPED_REGISTER_READ\n"));

            pIoBuffer->value[1] =
                PlxRegisterRead(
                    pdx,
                    (U32)pIoBuffer->value[0],
                    &(pIoBuffer->ReturnCode),
                    FALSE       // Don't adjust offset based on port
                    );

            DebugPrintf((
                "PLX Mapped Reg %03X = %08X\n",
                (U32)pIoBuffer->value[0],
                (U32)pIoBuffer->value[1]
                ));
            break;

        case PLX_IOCTL_MAPPED_REGISTER_WRITE:
            DebugPrintf_Cont(("PLX_IOCTL_MAPPED_REGISTER_WRITE\n"));

            pIoBuffer->ReturnCode =
                PlxRegisterWrite(
                    pdx,
                    (U32)pIoBuffer->value[0],
                    (U32)pIoBuffer->value[1],
                    FALSE       // Don't adjust offset based on port
                    );

            DebugPrintf((
                "Wrote %08X to PLX Mapped Reg %03X\n",
                (U32)pIoBuffer->value[1],
                (U32)pIoBuffer->value[0]
                ));
            break;


        /******************************************
         * PCI Mapping Functions
         *****************************************/
        case PLX_IOCTL_PCI_BAR_PROPERTIES:
            DebugPrintf_Cont(("PLX_IOCTL_PCI_BAR_PROPERTIES\n"));

            pIoBuffer->ReturnCode =
                PlxPciBarProperties(
                    pdx,
                    (U8)(pIoBuffer->value[0]),
                    &(pIoBuffer->u.BarProp)
                    );
            break;

        case PLX_IOCTL_PCI_BAR_MAP:
            DebugPrintf_Cont(("PLX_IOCTL_PCI_BAR_MAP\n"));

            pIoBuffer->ReturnCode =
                PlxPciBarMap(
                    pdx,
                    (U8)(pIoBuffer->value[0]),
                    &(pIoBuffer->value[1]),
                    pOwner
                    );
            break;

        case PLX_IOCTL_PCI_BAR_UNMAP:
            DebugPrintf_Cont(("PLX_IOCTL_PCI_BAR_UNMAP\n"));

            pIoBuffer->ReturnCode =
                PlxPciBarUnmap(
                    pdx,
                    PLX_INT_TO_PTR(pIoBuffer->value[1]),
                    pOwner
                    );
            break;


        /******************************************
         * Serial EEPROM Access Functions
         *****************************************/
        case PLX_IOCTL_EEPROM_PRESENT:
            DebugPrintf_Cont(("PLX_IOCTL_EEPROM_PRESENT\n"));

            pIoBuffer->ReturnCode =
                PlxEepromPresent(
                    pdx,
                    PLX_CAST_64_TO_8_PTR( &(pIoBuffer->value[0]) )
                    );
            break;

        case PLX_IOCTL_EEPROM_PROBE:
            DebugPrintf_Cont(("PLX_IOCTL_EEPROM_PROBE\n"));

            pIoBuffer->ReturnCode =
                PlxEepromProbe(
                    pdx,
                    PLX_CAST_64_TO_8_PTR( &(pIoBuffer->value[0]) )
                    );
            break;

        case PLX_IOCTL_EEPROM_CRC_GET:
            DebugPrintf_Cont(("PLX_IOCTL_EEPROM_CRC_GET\n"));

            pIoBuffer->ReturnCode =
                PlxEepromCrcGet(
                    pdx,
                    PLX_CAST_64_TO_32_PTR( &(pIoBuffer->value[0]) ),
                    PLX_CAST_64_TO_8_PTR ( &(pIoBuffer->value[1]) )
                    );
            break;

        case PLX_IOCTL_EEPROM_CRC_UPDATE:
            DebugPrintf_Cont(("PLX_IOCTL_EEPROM_CRC_UPDATE\n"));

            pIoBuffer->ReturnCode =
                PlxEepromCrcUpdate(
                    pdx,
                    PLX_CAST_64_TO_32_PTR( &(pIoBuffer->value[0]) ),
                    (BOOLEAN)pIoBuffer->value[1]
                    );
            break;

        case PLX_IOCTL_EEPROM_READ_BY_OFFSET:
            DebugPrintf_Cont(("PLX_IOCTL_EEPROM_READ_BY_OFFSET\n"));

            pIoBuffer->ReturnCode =
                PlxEepromReadByOffset(
                    pdx,
                    (U32)pIoBuffer->value[0],
                    PLX_CAST_64_TO_32_PTR( &(pIoBuffer->value[1]) )
                    );

            DebugPrintf((
                "EEPROM Offset %02X = %08X\n",
                (U32)pIoBuffer->value[0],
                (U32)pIoBuffer->value[1]
                ));
            break;

        case PLX_IOCTL_EEPROM_WRITE_BY_OFFSET:
            DebugPrintf_Cont(("PLX_IOCTL_EEPROM_WRITE_BY_OFFSET\n"));

            pIoBuffer->ReturnCode =
                PlxEepromWriteByOffset(
                    pdx,
                    (U32)pIoBuffer->value[0],
                    (U32)pIoBuffer->value[1]
                    );

            DebugPrintf((
                "Wrote %08X to EEPROM Offset %02X\n",
                (U32)pIoBuffer->value[1],
                (U32)pIoBuffer->value[0]
                ));
            break;

        case PLX_IOCTL_EEPROM_READ_BY_OFFSET_16:
            DebugPrintf_Cont(("PLX_IOCTL_EEPROM_READ_BY_OFFSET_16\n"));

            pIoBuffer->ReturnCode =
                PlxEepromReadByOffset_16(
                    pdx,
                    (U32)pIoBuffer->value[0],
                    PLX_CAST_64_TO_16_PTR( &(pIoBuffer->value[1]) )
                    );

            DebugPrintf((
                "EEPROM Offset %02X = %04X\n",
                (U32)pIoBuffer->value[0],
                (U16)pIoBuffer->value[1]
                ));
            break;

        case PLX_IOCTL_EEPROM_WRITE_BY_OFFSET_16:
            DebugPrintf_Cont(("PLX_IOCTL_EEPROM_WRITE_BY_OFFSET_16\n"));

            pIoBuffer->ReturnCode =
                PlxEepromWriteByOffset_16(
                    pdx,
                    (U32)pIoBuffer->value[0],
                    (U16)pIoBuffer->value[1]
                    );

            DebugPrintf((
                "Wrote %04X to EEPROM Offset %02X\n",
                (U16)pIoBuffer->value[1],
                (U32)pIoBuffer->value[0]
                ));
            break;


        /******************************************
         * I/O Port Access Functions
         *****************************************/
        case PLX_IOCTL_IO_PORT_READ:
            DebugPrintf_Cont(("PLX_IOCTL_IO_PORT_READ\n"));

            pIoBuffer->ReturnCode =
                PlxPciIoPortTransfer(
                    pIoBuffer->value[0],
                    PLX_INT_TO_PTR(pIoBuffer->u.TxParams.UserVa),
                    pIoBuffer->u.TxParams.ByteCount,
                    (PLX_ACCESS_TYPE)pIoBuffer->value[1],
                    TRUE           // Specify read operation
                    );
            break;

        case PLX_IOCTL_IO_PORT_WRITE:
            DebugPrintf_Cont(("PLX_IOCTL_IO_PORT_WRITE\n"));

            pIoBuffer->ReturnCode =
                PlxPciIoPortTransfer(
                    pIoBuffer->value[0],
                    PLX_INT_TO_PTR(pIoBuffer->u.TxParams.UserVa),
                    pIoBuffer->u.TxParams.ByteCount,
                    (PLX_ACCESS_TYPE)pIoBuffer->value[1],
                    FALSE          // Specify write operation
                    );
            break;


        /******************************************
         * Physical Memory Functions
         *****************************************/
        case PLX_IOCTL_PHYSICAL_MEM_ALLOCATE:
            DebugPrintf_Cont(("PLX_IOCTL_PHYSICAL_MEM_ALLOCATE\n"));

            pIoBuffer->ReturnCode =
                PlxPciPhysicalMemoryAllocate(
                    pdx,
                    &(pIoBuffer->u.PciMemory),
                    (BOOLEAN)(pIoBuffer->value[0]),
                    pOwner
                    );
            break;

        case PLX_IOCTL_PHYSICAL_MEM_FREE:
            DebugPrintf_Cont(("PLX_IOCTL_PHYSICAL_MEM_FREE\n"));

            pIoBuffer->ReturnCode =
                PlxPciPhysicalMemoryFree(
                    pdx,
                    &(pIoBuffer->u.PciMemory)
                    );
            break;

        case PLX_IOCTL_PHYSICAL_MEM_MAP:
            DebugPrintf_Cont(("PLX_IOCTL_PHYSICAL_MEM_MAP\n"));

            pIoBuffer->ReturnCode =
                PlxPciPhysicalMemoryMap(
                    pdx,
                    &(pIoBuffer->u.PciMemory),
                    pOwner
                    );
            break;

        case PLX_IOCTL_PHYSICAL_MEM_UNMAP:
            DebugPrintf_Cont(("PLX_IOCTL_PHYSICAL_MEM_UNMAP\n"));

            pIoBuffer->ReturnCode =
                PlxPciPhysicalMemoryUnmap(
                    pdx,
                    &(pIoBuffer->u.PciMemory),
                    pOwner
                    );
            break;

        case PLX_IOCTL_COMMON_BUFFER_PROPERTIES:
            DebugPrintf_Cont(("PLX_IOCTL_COMMON_BUFFER_PROPERTIES\n"));

            pIoBuffer->ReturnCode = PLX_STATUS_OK;

            // Return buffer information
            pIoBuffer->u.PciMemory.PhysicalAddr =
                     pGbl_DriverObject->CommonBuffer.BusPhysical;
            pIoBuffer->u.PciMemory.CpuPhysical =
                     pGbl_DriverObject->CommonBuffer.CpuPhysical;
            pIoBuffer->u.PciMemory.Size =
                     pGbl_DriverObject->CommonBuffer.Size;
            break;


        /******************************************
         * Interrupt Support Functions
         *****************************************/
        case PLX_IOCTL_INTR_ENABLE:
            DebugPrintf_Cont(("PLX_IOCTL_INTR_ENABLE\n"));

            pIoBuffer->ReturnCode =
                PlxInterruptEnable(
                    pdx,
                    &(pIoBuffer->u.PlxIntr)
                    );
            break;

        case PLX_IOCTL_INTR_DISABLE:
            DebugPrintf_Cont(("PLX_IOCTL_INTR_DISABLE\n"));

            pIoBuffer->ReturnCode =
                PlxInterruptDisable(
                    pdx,
                    &(pIoBuffer->u.PlxIntr)
                    );
            break;

        case PLX_IOCTL_NOTIFICATION_REGISTER_FOR:
            DebugPrintf_Cont(("PLX_IOCTL_NOTIFICATION_REGISTER_FOR\n"));

            pIoBuffer->ReturnCode =
                PlxNotificationRegisterFor(
                    pdx,
                    &(pIoBuffer->u.PlxIntr),
                    (VOID**)&(pIoBuffer->value[0]),
                    pOwner
                    );
            break;

        case PLX_IOCTL_NOTIFICATION_WAIT:
            DebugPrintf_Cont(("PLX_IOCTL_NOTIFICATION_WAIT\n"));

            pIoBuffer->ReturnCode =
                PlxNotificationWait(
                    pdx,
                    PLX_INT_TO_PTR(pIoBuffer->value[0]),
                    (PLX_UINT_PTR)pIoBuffer->value[1]
                    );
            break;

        case PLX_IOCTL_NOTIFICATION_STATUS:
            DebugPrintf_Cont(("PLX_IOCTL_NOTIFICATION_STATUS\n"));

            pIoBuffer->ReturnCode =
                PlxNotificationStatus(
                    pdx,
                    PLX_INT_TO_PTR(pIoBuffer->value[0]),
                    &(pIoBuffer->u.PlxIntr)
                    );
            break;

        case PLX_IOCTL_NOTIFICATION_CANCEL:
            DebugPrintf_Cont(("PLX_IOCTL_NOTIFICATION_CANCEL\n"));

            pIoBuffer->ReturnCode =
                PlxNotificationCancel(
                    pdx,
                    PLX_INT_TO_PTR(pIoBuffer->value[0]),
                    pOwner
                    );
            break;


        /******************************************
         * Unsupported Messages
         *****************************************/
        default:
            DebugPrintf_Cont((
                "Unsupported PLX_IOCTL_Xxx (%02d)\n",
                _IOC_NR(cmd)
                ));

            pIoBuffer->ReturnCode = PLX_STATUS_UNSUPPORTED;
            break;
    }

    DebugPrintf(("...Completed message\n"));

    status =
        copy_to_user(
            (PLX_PARAMS*)args,
            pIoBuffer,
            sizeof(PLX_PARAMS)
            );

    return status;
}
