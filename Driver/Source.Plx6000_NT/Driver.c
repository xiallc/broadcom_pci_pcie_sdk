/*******************************************************************************
 * Copyright 2013-2021 Broadcom Inc
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
 *      Driver.c
 *
 * Description:
 *
 *      Initializes the driver and claims system resources for the device
 *
 * Revision History:
 *
 *      03-01-19 : PCI/PCIe SDK v8.00
 *
 *****************************************************************************/


#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/sched.h>
#include <linux/version.h>
/***********************************************************
 * vermagic.h
 *
 * Drivers use the UTS_RELEASE macro to print the current
 * kernel version.
 * The definition of UTS_RELEASE was included through header
 * file vermagic.h. Starting from 5.8.0, this header cannot
 * be included anymore. Include utsrelease.h directly.
 **********************************************************/
#if (LINUX_VERSION_CODE < KERNEL_VERSION(5,8,0))
    #include <linux/vermagic.h>
#else
    #include <generated/utsrelease.h>
#endif
#include "ApiFunc.h"
#include "Dispatch.h"
#include "Driver.h"
#include "PciFunc.h"
#include "PciRegs.h"
#include "PlxChipFn.h"
#include "PlxInterrupt.h"
#include "Plx_sysdep.h"
#include "SuppFunc.h"




/***********************************************
 *               Globals
 **********************************************/
// Pointer to the main driver object
DRIVER_OBJECT *pGbl_DriverObject_6000;

// Setup the PCI device table to probe
static struct pci_device_id PlxPciIdTable[] =
{
    // VenID  DevID       SubVenID    SubDevID    Class     ClassMask
    { 0x10B5, 0x6541, PCI_ANY_ID, PCI_ANY_ID, 0x000000, 0x000000 },
    { 0x10B5, 0x6542, PCI_ANY_ID, PCI_ANY_ID, 0x000000, 0x000000 },
    { 0x3388, 0x0021, PCI_ANY_ID, PCI_ANY_ID, 0x000000, 0x000000 },
    { 0x3388, 0x0029, PCI_ANY_ID, PCI_ANY_ID, 0x000000, 0x000000 },
    { 0 } // Required last entry
};
MODULE_DEVICE_TABLE(pci, PlxPciIdTable);

// Driver registration functions
static struct pci_driver PlxPciDriver =
{
    .name     = "PLX 6000 NT Driver",
    .id_table = PlxPciIdTable,
    .probe    = PlxDispatch_Probe,
    .remove   = PlxDispatch_Remove,
    .shutdown = NULL
};




/*******************************************************************************
 *
 * Function   :  Plx_init_module
 *
 * Description:  Entry point for the driver
 *
 ******************************************************************************/
int __init
Plx_init_module(
    void
    )
{
    int              status;
    PLX_PHYSICAL_MEM PhysicalMem;


    InfoPrintf_Cont(("\n"));
    InfoPrintf(("<========================================================>\n"));
    InfoPrintf((
        "PLX %s driver v%d.%02d (%d-bit)\n",
        __stringify(PLX_CHIP), PLX_SDK_VERSION_MAJOR, PLX_SDK_VERSION_MINOR,
        (U32)(sizeof(PLX_UINT_PTR) * 8)
        ));
    InfoPrintf(("Supports Linux kernel v%s\n", UTS_RELEASE));

    // Allocate memory for the Driver Object
    pGbl_DriverObject =
        kmalloc(
            sizeof(DRIVER_OBJECT),
            GFP_KERNEL
            );

    if (pGbl_DriverObject == NULL)
    {
        ErrorPrintf(("ERROR - memory allocation for Driver Object failed\n"));
        return (-ENOMEM);
    }

    DebugPrintf((
        "Allocated global driver object (%p)\n",
        pGbl_DriverObject
        ));

    // Clear the driver object
    RtlZeroMemory( pGbl_DriverObject, sizeof(DRIVER_OBJECT) );

    // Initialize driver object
    pGbl_DriverObject->DeviceObject = NULL;
    pGbl_DriverObject->DeviceCount  = 0;

    // Fill in the appropriate dispatch handlers
    pGbl_DriverObject->DispatchTable.owner   = THIS_MODULE;
    pGbl_DriverObject->DispatchTable.mmap    = Dispatch_mmap;
    pGbl_DriverObject->DispatchTable.open    = Dispatch_open;
    pGbl_DriverObject->DispatchTable.release = Dispatch_release;

    // Use newer IOCTL functions
    pGbl_DriverObject->DispatchTable.unlocked_ioctl = Dispatch_IoControl;
    pGbl_DriverObject->DispatchTable.compat_ioctl   = Dispatch_IoControl;

    // Initialize spin locks
    spin_lock_init(
        &(pGbl_DriverObject->Lock_DeviceList)
        );

    /*********************************************************
     * Register the driver with the OS
     *
     * NOTE: This driver still uses the old method for registering
     * the device (register_chrdev) for compatability with 2.4 kernels.
     * A future version of the driver may use the new interface
     * (cdev_init, cdev_add, & cdev_del).
     ********************************************************/
    pGbl_DriverObject->MajorID =
        register_chrdev(
            0,              // 0 = system chooses Major ID
            PLX_DRIVER_NAME,
            &(pGbl_DriverObject->DispatchTable)
            );

    DebugPrintf((
        "Registered driver (MajorID = %03d)\n",
        pGbl_DriverObject->MajorID
        ));

    // Register the driver
    status = pci_register_driver( &PlxPciDriver );
    if (status != 0)
    {
        ErrorPrintf(("ERROR: Unable to register driver (status=%d)\n", status));
        Plx_cleanup_module();
        return status;
    }

    // Flag driver was registered
    pGbl_DriverObject->bPciDriverReg = TRUE;

    // Check if any devices were found
    if (pGbl_DriverObject->DeviceCount == 0)
    {
        ErrorPrintf(("ERROR - No supported devices found\n"));
        Plx_cleanup_module();
        return (-ENODEV);
    }

    // Initialize common buffer
    pGbl_DriverObject->CommonBuffer.Size = 0;

    // Set requested size
    PhysicalMem.Size = DEFAULT_SIZE_COMMON_BUFFER;

    // Allocate common buffer
    if (PhysicalMem.Size != 0)
    {
        // Allocate common buffer
        PlxPciPhysicalMemoryAllocate(
            pGbl_DriverObject->DeviceObject->DeviceExtension,  // Assign buffer to first device
            &PhysicalMem,
            TRUE,                                              // Smaller buffer is ok
            pGbl_DriverObject                                  // Assign Driver object as owner
            );
    }

    DebugPrintf(("   --------------------\n"));
    DebugPrintf((
        "Added: %d device%s\n",
        pGbl_DriverObject->DeviceCount, (pGbl_DriverObject->DeviceCount > 1) ? "s" : ""
        ));
    InfoPrintf(("...driver loaded\n\n"));
    return 0;
}




/*******************************************************************************
 *
 * Function   :  Plx_cleanup_module
 *
 * Description:  Unload the driver
 *
 ******************************************************************************/
void
Plx_cleanup_module(
    void
    )
{
    InfoPrintf_Cont(("\n"));
    InfoPrintf((
        "Unload: PLX %s driver v%d.%02d\n",
        __stringify(PLX_CHIP), PLX_SDK_VERSION_MAJOR, PLX_SDK_VERSION_MINOR
        ));

    // Release common buffer
    if (pGbl_DriverObject->CommonBuffer.Size != 0)
    {
        DebugPrintf(("De-allocate Common Buffer\n"));

        // Release the buffer
        Plx_dma_buffer_free(
            pGbl_DriverObject->DeviceObject->DeviceExtension,   // First device
            &(pGbl_DriverObject->CommonBuffer)
            );
    }

    // De-register driver
    if (pGbl_DriverObject->bPciDriverReg)
    {
        pci_unregister_driver( &PlxPciDriver );
    }

    DebugPrintf((
        "De-register driver (MajorID = %03d)\n",
        pGbl_DriverObject->MajorID
        ));

    /*********************************************************
     * De-register the driver with the OS
     *
     * NOTE: This driver still uses the old method for de-registering
     * the device (unregister_chrdev) for compatability with 2.4 kernels.
     * A future version of the driver may use the new interface
     * (cdev_init, cdev_add, & cdev_del).
     ********************************************************/
    unregister_chrdev(
        pGbl_DriverObject->MajorID,
        PLX_DRIVER_NAME
        );

    DebugPrintf((
        "Release global driver object (%p)\n",
        pGbl_DriverObject
        ));

    // Release driver object
    kfree( pGbl_DriverObject );
    pGbl_DriverObject = NULL;

    InfoPrintf(("...driver unloaded\n"));
}




/**************************************************
 * The module_XX macros are only needed if the
 * driver entry and exit routine differ from the
 * default of init_module() and exit_module().
 * These macros must be placed AFTER the module code.
 **************************************************/
module_init(Plx_init_module);
module_exit(Plx_cleanup_module);




/*******************************************************************************
 *
 * Function   :  PlxDispatch_Probe
 *
 * Description:  Called by the OS to determine if a device is supported by the driver
 *
 ******************************************************************************/
int
PlxDispatch_Probe(
    struct pci_dev             *pDev,
    const struct pci_device_id *pID
    )
{
    int status;
    U32 RegValue;


    // Default to supported device
    status = 0;

    DebugPrintf(("   --------------------\n"));
    DebugPrintf((
        "Probe: %04X %04X [D%x %02X:%02X.%X]\n",
        pDev->device, pDev->vendor, pci_domain_nr(pDev->bus),
        pDev->bus->number, PCI_SLOT(pDev->devfn), PCI_FUNC(pDev->devfn)
        ));

    // Perform sanity check by reading Dev/Ven ID
    PlxPciRegRead_ByLoc(
        pci_domain_nr(pDev->bus),
        pDev->bus->number,
        PCI_SLOT(pDev->devfn),
        PCI_FUNC(pDev->devfn),
        PCI_REG_DEV_VEN_ID,
        &RegValue,
        sizeof(U32)
        );
    if (RegValue == (U32)-1)
    {
        ErrorPrintf(("ERROR: Device is not responding to PCI accesses\n"));
        return -ENODEV;
    }

    // Only 6000-series device supported
    if ((pDev->device & 0xF000) != 0x6000)
    {
        status = -ENODEV;
    }

    // Make sure the PCI Header type is 0
    if (pDev->hdr_type != 0)
    {
        status = -ENODEV;
    }

    // Must be a function 0 device
    if (PCI_FUNC(pDev->devfn) != 0)
    {
        status = -ENODEV;
    }

    // Class code must be "Other bridge device"
    if (pDev->class != 0x068000)
    {
        status = -ENODEV;
    }

    if (status != 0)
    {
        DebugPrintf(("Probe: -- Unsupported Device --\n"));
        return status;
    }

    // Add the device to the device list
    status =
        AddDevice(
            pGbl_DriverObject,
            pDev
            );

    return status;
}




/*******************************************************************************
 *
 * Function   :  PlxDispatch_Remove
 *
 * Description:  Called by the OS to remove a device from the driver
 *
 ******************************************************************************/
void
PlxDispatch_Remove(
    struct pci_dev *pDev
    )
{
    DEVICE_OBJECT *fdo;


    // Find the device in the list
    fdo = pGbl_DriverObject->DeviceObject;

    // Remove all devices
    while (fdo != NULL)
    {
        if (fdo->DeviceExtension->pPciDevice == pDev)
        {
            // Delete the device & remove from device list
            RemoveDevice( fdo );
            return;
        }

        // Jump to next device object
        fdo = fdo->NextDevice;
    }
}




/*******************************************************************************
 *
 * Function   :  AddDevice
 *
 * Description:  Add a new device object to the driver
 *
 ******************************************************************************/
int
AddDevice(
    DRIVER_OBJECT  *pDriverObject,
    struct pci_dev *pPciDev
    )
{
    int               status;
    U32               RegValue;
    DEVICE_OBJECT    *fdo;
    DEVICE_OBJECT    *pDevice;
    DEVICE_EXTENSION *pdx;


    // Allocate memory for the device object
    fdo =
        kmalloc(
            sizeof(DEVICE_OBJECT),
            GFP_KERNEL
            );

    if (fdo == NULL)
    {
        ErrorPrintf(("ERROR - memory allocation for device object failed\n"));
        return (-ENOMEM);
    }

    // Initialize device object
    RtlZeroMemory( fdo, sizeof(DEVICE_OBJECT) );

    fdo->DriverObject    = pDriverObject;         // Save parent driver object
    fdo->DeviceExtension = &(fdo->DeviceInfo);

    // Enable the device
    if (pci_enable_device( pPciDev ) == 0)
    {
        DebugPrintf(("Enabled PCI device\n"));
    }
    else
    {
        ErrorPrintf(("WARNING - PCI device enable failed\n"));
    }

    // Enable bus mastering
    pci_set_master( pPciDev );

    //
    // Initialize the device extension
    //

    pdx = fdo->DeviceExtension;

    // Clear device extension
    RtlZeroMemory( pdx, sizeof(DEVICE_EXTENSION) );

    // Store parent device object
    pdx->pDeviceObject = fdo;

    // Save the OS-supplied PCI object
    pdx->pPciDevice = pPciDev;

    // Set initial device device state
    pdx->State = PLX_STATE_STOPPED;

    // Set initial power state
    pdx->PowerState = PowerDeviceD0;

    // Store device location information
    pdx->Key.domain       = pci_domain_nr(pPciDev->bus);
    pdx->Key.bus          = pPciDev->bus->number;
    pdx->Key.slot         = PCI_SLOT(pPciDev->devfn);
    pdx->Key.function     = PCI_FUNC(pPciDev->devfn);
    pdx->Key.DeviceId     = pPciDev->device;
    pdx->Key.VendorId     = pPciDev->vendor;
    pdx->Key.SubVendorId  = pPciDev->subsystem_vendor;
    pdx->Key.SubDeviceId  = pPciDev->subsystem_device;
    pdx->Key.DeviceNumber = pDriverObject->DeviceCount;

    // Set API access mode
    pdx->Key.ApiMode = PLX_API_MODE_PCI;

    // Update Revision ID
    PLX_PCI_REG_READ( pdx, PCI_REG_CLASS_REV, &RegValue );
    pdx->Key.Revision = (U8)(RegValue & 0xFF);

    // Set device mode
    pdx->Key.DeviceMode = PLX_CHIP_MODE_STD_LEGACY_NT;

    // Set PLX-specific port type ([1:0] 1=Primary 2=Secondary)
    if (pdx->Key.DeviceId & (1 << 0))
    {
        pdx->Key.PlxPortType = PLX_SPEC_PORT_NT_VIRTUAL;
    }
    else
    {
        pdx->Key.PlxPortType = PLX_SPEC_PORT_NT_LINK;
    }

    // Build device name
    sprintf(
        pdx->LinkName,
        PLX_DRIVER_NAME "-%d",
        pDriverObject->DeviceCount
        );

    // Initialize work queue for ISR DPC queueing
    PLX_INIT_WORK(
        &(pdx->Task_DpcForIsr),
        DpcForIsr,                // DPC routine
        &(pdx->Task_DpcForIsr)    // DPC parameter (pre-2.6.20 only)
        );

    // Initialize ISR spinlock
    spin_lock_init( &(pdx->Lock_Isr) );

    // Initialize interrupt wait list
    INIT_LIST_HEAD( &(pdx->List_WaitObjects) );
    spin_lock_init( &(pdx->Lock_WaitObjectsList) );

    // Initialize physical memories list
    INIT_LIST_HEAD( &(pdx->List_PhysicalMem) );
    spin_lock_init( &(pdx->Lock_PhysicalMemList) );

    // Set buffer allocation mask
    if (Plx_dma_set_coherent_mask( pdx, PLX_DMA_BIT_MASK(32) ) != 0)
    {
        ErrorPrintf(("WARNING - Set DMA coherent mask failed\n"));
    }

    //
    // Add to driver device list
    //

    // Acquire Device List lock
    spin_lock( &(pDriverObject->Lock_DeviceList) );

    // Get device list head
    pDevice = pDriverObject->DeviceObject;

    if (pDevice == NULL)
    {
        // Add device as first in list
        pDriverObject->DeviceObject = fdo;
    }
    else
    {
        // Go to end of list
        while (pDevice->NextDevice != NULL)
        {
            pDevice = pDevice->NextDevice;
        }

        // Add device to end of list
        pDevice->NextDevice = fdo;
    }

    // Increment device count
    pDriverObject->DeviceCount++;

    // Release Device List lock
    spin_unlock( &(pDriverObject->Lock_DeviceList) );

    DebugPrintf(("Created Device (%s)\n", pdx->LinkName));

    // Start the device
    status = StartDevice( fdo );
    if (status != 0)
    {
        RemoveDevice( fdo );
        return status;
    }

    return 0;
}




/*******************************************************************************
 *
 * Function   :  RemoveDevice
 *
 * Description:  Remove a functional device object
 *
 ******************************************************************************/
int
RemoveDevice(
    DEVICE_OBJECT *fdo
    )
{
    DEVICE_OBJECT    *pDevice;
    DEVICE_EXTENSION *pdx;


    pdx = fdo->DeviceExtension;

    // Stop device and release its resources
    StopDevice( fdo );

    DebugPrintf((
        "Remove: %04X %04X [D%x %02X:%02X.%X] (%s)\n",
        pdx->Key.DeviceId, pdx->Key.VendorId, pdx->Key.domain,
        pdx->Key.bus, pdx->Key.slot, pdx->Key.function,
        pdx->LinkName
        ));

    // Acquire Device List lock
    spin_lock( &(fdo->DriverObject->Lock_DeviceList) );

    // Get device list head
    pDevice = fdo->DriverObject->DeviceObject;

    if (pDevice == NULL)
    {
        spin_unlock( &(fdo->DriverObject->Lock_DeviceList) );
        ErrorPrintf(("ERROR - Unable to remove device, device list empty\n"));
        return (-ENODEV);
    }

    if (pDevice == fdo)
    {
        // Remove device from first in list
        fdo->DriverObject->DeviceObject = fdo->NextDevice;
    }
    else
    {
        // Scan list for the device
        while (pDevice->NextDevice != fdo)
        {
            pDevice = pDevice->NextDevice;

            if (pDevice == NULL)
            {
                spin_unlock( &(fdo->DriverObject->Lock_DeviceList) );
                ErrorPrintf(("ERROR - Device (%p) not found in list\n", fdo));
                return (-ENODEV);
            }
        }

        // Remove device from list
        pDevice->NextDevice = fdo->NextDevice;
    }

    // Decrement device count
    pGbl_DriverObject->DeviceCount--;

    // Release Device List lock
    spin_unlock( &(fdo->DriverObject->Lock_DeviceList) );

    // Disable the device
    DebugPrintf(("Disable device\n"));
    pci_disable_device( pdx->pPciDevice );

    // Release device object
    DebugPrintf(("Delete device object (%p)\n", fdo));
    kfree( fdo );

    DebugPrintf(("   --------------------\n"));
    return 0;
}




/*******************************************************************************
 *
 * Function   :  StartDevice
 *
 * Description:  Start a device
 *
 ******************************************************************************/
int
StartDevice(
    DEVICE_OBJECT *fdo
    )
{
    int               rc;
    U8                i;
    U8                ResCount;
    U32               RegValue;
    DEVICE_EXTENSION *pdx;


    pdx      = fdo->DeviceExtension;
    ResCount = 0;

    DebugPrintf((
        "Start: %04X %04X [D%x %02X:%02X.%X]\n",
        pdx->Key.DeviceId, pdx->Key.VendorId,
        pdx->Key.domain, pdx->Key.bus, pdx->Key.slot, pdx->Key.function
        ));

    // Update device state
    pdx->State = PLX_STATE_STARTING;

    for (i = 0; i < PCI_NUM_BARS_TYPE_00; ++i)
    {
        // Get BAR address
        pdx->PciBar[i].Properties.Physical = pci_resource_start( pdx->pPciDevice, i );

        // Skip if BAR is disabled
        if (pdx->PciBar[i].Properties.Physical == 0)
        {
            continue;
        }

        DebugPrintf(("   Resource %02d\n", ResCount));

        // Increment resource count
        ResCount++;

        // Determine resource type
        if (pci_resource_flags( pdx->pPciDevice, i ) & IORESOURCE_IO)
        {
            DebugPrintf(("     Type     : I/O\n"));

            // Make sure flags are cleared properly
            pdx->PciBar[i].Properties.Physical &= ~(0x3);
            pdx->PciBar[i].Properties.Flags     = PLX_BAR_FLAG_IO;
        }
        else
        {
            DebugPrintf(("     Type     : Memory\n"));

            // Make sure flags are cleared properly
            pdx->PciBar[i].Properties.Physical &= ~(0xf);
            pdx->PciBar[i].Properties.Flags     = PLX_BAR_FLAG_MEM;
        }

        // Set BAR as already probed
        pdx->PciBar[i].Properties.Flags |= PLX_BAR_FLAG_PROBED;

        // Get the actual BAR value
        PLX_PCI_REG_READ( pdx, 0x10 + (i * sizeof(U32)), &RegValue );
        pdx->PciBar[i].Properties.BarValue = RegValue;

        // If 64-bit BAR, get upper address
        if (pci_resource_flags(pdx->pPciDevice, i) & IORESOURCE_MEM_64)
        {
            PLX_PCI_REG_READ( pdx, 0x10 + ((i+1) * sizeof(U32)), &RegValue );
            pdx->PciBar[i].Properties.BarValue |= (U64)RegValue << 32;
        }

        DebugPrintf((
            "     PCI BAR %d: %08llX\n",
            i, pdx->PciBar[i].Properties.BarValue
            ));

        DebugPrintf((
            "     Phys Addr: %08llX\n",
            pdx->PciBar[i].Properties.Physical
            ));

        // Get the size
        pdx->PciBar[i].Properties.Size = pci_resource_len( pdx->pPciDevice, i );

        DebugPrintf((
            "     Size     : %llxh (%lld%s)\n",
            pdx->PciBar[i].Properties.Size,
            (pdx->PciBar[i].Properties.Size > ((U64)1 << 30)) ?
              (pdx->PciBar[i].Properties.Size >> 30) :
              (pdx->PciBar[i].Properties.Size > ((U64)1 << 20)) ?
              (pdx->PciBar[i].Properties.Size >> 20) :
              (pdx->PciBar[i].Properties.Size > ((U64)1 << 10)) ?
              (pdx->PciBar[i].Properties.Size >> 10) :
              pdx->PciBar[i].Properties.Size,
            (pdx->PciBar[i].Properties.Size > ((U64)1 << 30)) ? "GB" :
              (pdx->PciBar[i].Properties.Size > ((U64)1 << 20)) ? "MB" :
              (pdx->PciBar[i].Properties.Size > ((U64)1 << 10)) ? "KB" : "B"
            ));

        // Set flags
        if (pdx->PciBar[i].Properties.Flags & PLX_BAR_FLAG_MEM)
        {
            if (pci_resource_flags(pdx->pPciDevice, i) & IORESOURCE_MEM_64)
            {
                pdx->PciBar[i].Properties.Flags |= PLX_BAR_FLAG_64_BIT;
            }
            else
            {
                pdx->PciBar[i].Properties.Flags |= PLX_BAR_FLAG_32_BIT;
            }

            if (pci_resource_flags(pdx->pPciDevice, i) & IORESOURCE_PREFETCH)
            {
                pdx->PciBar[i].Properties.Flags |= PLX_BAR_FLAG_PREFETCHABLE;
            }

            DebugPrintf((
                "     Property : %sPrefetchable %d-bit\n",
                (pdx->PciBar[i].Properties.Flags & PLX_BAR_FLAG_PREFETCHABLE) ? "" : "Non-",
                (pdx->PciBar[i].Properties.Flags & PLX_BAR_FLAG_64_BIT) ? 64 : 32
                ));
        }

        // Claim and map the resource
        rc = PlxPciBarResourceMap( pdx, i );

        if (pdx->PciBar[i].Properties.Flags & PLX_BAR_FLAG_MEM)
        {
            if (rc == 0)
            {
                DebugPrintf(("     Kernel VA: %p\n", pdx->PciBar[i].pVa));
            }
            else
            {
                ErrorPrintf(("     Kernel VA: ERROR - Unable to map to kernel VA\n"));
            }
        }
    }

    // Determine & store the PLX chip type
    PlxChipTypeDetect( pdx );

    // Determine which side of NT port we are located on
    PlxDetermineNtPortSide( pdx );

    // Disable all interrupts
    PlxChipInterruptsDisable( pdx );

    // Default interrupt to none
    pdx->IrqType = PLX_IRQ_TYPE_NONE;

    // Store PCI IRQ
    pdx->IrqPci = pdx->pPciDevice->irq;

    // Install the ISR if available
    if (pdx->pPciDevice->irq == 0)
    {
        DebugPrintf(("Device not using a PCI interrupt resource\n"));
    }
    else
    {
        // Attempt to enable MSI interrupt
        rc = Plx_pci_enable_msi( pdx->pPciDevice );
        if (rc == 0)
        {
            DebugPrintf((
                "MSI enabled, change IRQ %02d --> %02d\n",
                pdx->IrqPci, pdx->pPciDevice->irq
                ));
            pdx->IrqType = PLX_IRQ_TYPE_MSI;
        }
        else
        {
            DebugPrintf(("MSI not supported (code=%d), default to INTx\n", rc));
            pdx->IrqType = PLX_IRQ_TYPE_INTX;
        }

        DebugPrintf(("   Resource %02d\n", ResCount));
        DebugPrintf(("     Type     : Interrupt (%s)\n", (pdx->IrqType == PLX_IRQ_TYPE_MSI) ? "MSI" : "PCI INTx" ));
        DebugPrintf(("     IRQ      : %02d [%02Xh]\n", pdx->pPciDevice->irq, pdx->pPciDevice->irq));

        // If MSI enabled, display MSI address & data
        if (pdx->IrqType == PLX_IRQ_TYPE_MSI)
        {
            // Get MSI capability ID
            pdx->OffsetCap_MSI = pci_find_capability( pdx->pPciDevice, PCI_CAP_ID_MSI );

            if (pdx->OffsetCap_MSI != 0)
            {
                PLX_PCI_REG_READ( pdx, pdx->OffsetCap_MSI + 0x8, &RegValue);
                DebugPrintf(("     MSI Addr : %04X", RegValue));
                PLX_PCI_REG_READ( pdx, pdx->OffsetCap_MSI + 0x4, &RegValue);
                DebugPrintf_Cont(("_%08X\n", RegValue));
                PLX_PCI_REG_READ( pdx, pdx->OffsetCap_MSI + 0xC, &RegValue);
                DebugPrintf(("     MSI Data : %04X\n", RegValue));
            }
        }

        // Install the ISR
        rc =
            request_irq(
                pdx->pPciDevice->irq,    // The device IRQ
                OnInterrupt,             // Interrupt handler
                IRQF_SHARED,             // Flags, support interrupt sharing
                PLX_DRIVER_NAME,         // The driver name
                pdx                      // Parameter to the ISR
                );

        if (rc != 0)
        {
            ErrorPrintf(("ERROR - Unable to install ISR\n"));
            if (pdx->IrqType == PLX_IRQ_TYPE_MSI)
            {
                Plx_pci_disable_msi( pdx->pPciDevice );
            }
            pdx->IrqType = PLX_IRQ_TYPE_NONE;
        }
        else
        {
            DebugPrintf(("Installed ISR for interrupt\n"));

            // Enable interrupts on success
            PlxChipInterruptsEnable( pdx );
        }
    }

    // Update device state
    pdx->State = PLX_STATE_STARTED;

    return 0;
}




/*******************************************************************************
 *
 * Function   :  StopDevice
 *
 * Description:  Stop a device
 *
 ******************************************************************************/
VOID
StopDevice(
    DEVICE_OBJECT *fdo
    )
{
    U16               LoopCount;
    DEVICE_EXTENSION *pdx;


    pdx = fdo->DeviceExtension;

    // Only stop devices in one of the start states
    if ((pdx->State != PLX_STATE_STARTED) && (pdx->State != PLX_STATE_STARTING))
    {
        return;
    }

    DebugPrintf(("---------- %s ----------\n", pdx->LinkName));
    DebugPrintf((
        "Stop: %04X %04X [D%x %02X:%02X.%X]\n",
        pdx->Key.DeviceId, pdx->Key.VendorId,
        pdx->Key.domain, pdx->Key.bus, pdx->Key.slot, pdx->Key.function
        ));

    // Notify ISR not to schedule any more DPCs
    pdx->State = PLX_STATE_STOPPING;

    // Disable all interrupts
    PlxChipInterruptsDisable( pdx );

    if (pdx->bDpcPending)
    {
        DebugPrintf(("DPC routine pending, waiting for it to complete...\n"));

        // Set wait count for DPC
        LoopCount = 5;
    }

    // If DPC routine pending, attempt to let it complete before releasing resources
    while ((pdx->bDpcPending) && (LoopCount != 0))
    {
        // Decrement counter
        LoopCount--;

        // Flush any work queue items
        Plx_flush_work( &(pdx->Task_DpcForIsr) );

        // Set current task as uninterruptible
        set_current_state(TASK_UNINTERRUPTIBLE);

        // Relieve timeslice to allow DPC to complete
        schedule_timeout( Plx_ms_to_jiffies( 100 ) );
    }

    // Release interrupt resources
    if (pdx->IrqType != PLX_IRQ_TYPE_NONE)
    {
        DebugPrintf((
            "Remove ISR (IRQ = %02d [%02Xh])\n",
            pdx->pPciDevice->irq, pdx->pPciDevice->irq
            ));
        free_irq( pdx->pPciDevice->irq, pdx );

        if (pdx->IrqType == PLX_IRQ_TYPE_MSI)
        {
            DebugPrintf(("Disable MSI..."));
            Plx_pci_disable_msi( pdx->pPciDevice );
            DebugPrintf_Cont(("Ok (Restored IRQ=%02d)\n", pdx->pPciDevice->irq));
        }

        // Mark the interrupt resource released
        pdx->IrqType = PLX_IRQ_TYPE_NONE;
    }

    // Unmap I/O regions from kernel space (No register access after this)
    PlxPciBarResourcesUnmap( pdx );

    // Update device state
    pdx->State = PLX_STATE_STOPPED;
}
