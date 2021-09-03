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
 *      Driver.c
 *
 * Description:
 *
 *      Initializes the driver and claims system resources for the device
 *
 * Revision History:
 *
 *      07-01-12 : PLX SDK v7.00
 *
 *****************************************************************************/


#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/init.h>
#include <linux/version.h>
#include <linux/vermagic.h>
#include "Dispatch.h"
#include "Driver.h"
#include "PciFunc.h"
#include "Plx_sysdep.h"
#include "SuppFunc.h"




/***********************************************
 *               Globals
 **********************************************/
DRIVER_OBJECT *pGbl_DriverObject_PlxSvc;       // Pointer to the main driver object




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
    int status;


    DebugPrintf_Cont(("\n"));
    DebugPrintf(("<========================================================>\n"));
    DebugPrintf((
        "PLX Service driver v%d.%02d (%d-bit) - built %s %s\n",
        PLX_SDK_VERSION_MAJOR, PLX_SDK_VERSION_MINOR,
        (U32)(sizeof(PLX_UINT_PTR) * 8),
        __DATE__, __TIME__
        ));

    DebugPrintf((
        "Supports Linux kernel version %s\n",
        UTS_RELEASE
        ));

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
    RtlZeroMemory(
        pGbl_DriverObject,
        sizeof(DRIVER_OBJECT)
        );

    // Initialize driver object
    pGbl_DriverObject->DeviceObject = NULL;
    pGbl_DriverObject->DeviceCount  = 0;

    // Fill in the appropriate dispatch handlers
    pGbl_DriverObject->DispatchTable.owner   = THIS_MODULE;
    pGbl_DriverObject->DispatchTable.mmap    = Dispatch_mmap;
    pGbl_DriverObject->DispatchTable.open    = Dispatch_open;
    pGbl_DriverObject->DispatchTable.release = Dispatch_release;

#if (LINUX_VERSION_CODE < KERNEL_VERSION(2,6,36))
    pGbl_DriverObject->DispatchTable.ioctl = Dispatch_IoControl;
#else
    pGbl_DriverObject->DispatchTable.unlocked_ioctl = Dispatch_IoControl;
#endif

    // Initialize spin locks
    spin_lock_init(
        &(pGbl_DriverObject->Lock_DeviceList)
        );

    // Register the driver with the OS
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

    // Add a single device object
    status =
        AddDevice(
            pGbl_DriverObject,
            NULL
            );

    if (status != 0)
    {
        // Unload the driver
        Plx_cleanup_module();
        return (-ENODEV);
    }

    // Probe ACPI tables for PCI Express mechanism
    PlxProbeForEcamBase();

    // Scan the PCI bus to build list of devices
    pGbl_DriverObject->DeviceCount =
        PlxDeviceListBuild(
            pGbl_DriverObject
            );

    // Check if any devices were found
    if (pGbl_DriverObject->DeviceCount == 0)
    {
        ErrorPrintf(("ERROR - No PCI devices found\n"));

        // Unload the driver
        Plx_cleanup_module();

        return (-ENODEV);
    }

    DebugPrintf(("...driver loaded\n"));

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
    DEVICE_OBJECT *fdo;
    DEVICE_OBJECT *pNext;


    DebugPrintf_Cont(("\n"));
    DebugPrintf(("Unload driver...\n"));

    // Get the device list
    fdo = pGbl_DriverObject->DeviceObject;

    // Remove all devices
    while (fdo != NULL)
    {
        // Store next device
        pNext = fdo->NextDevice;

        // Delete the device & remove from device list
        RemoveDevice(
            fdo
            );

        // Jump to next device object
        fdo = pNext;
    }

    DebugPrintf((
        "De-register driver (MajorID = %03d)\n",
        pGbl_DriverObject->MajorID
        ));

    unregister_chrdev(
        pGbl_DriverObject->MajorID,
        PLX_DRIVER_NAME
        );

    DebugPrintf((
        "Release global driver object (%p)\n",
        pGbl_DriverObject
        ));

    // Release driver object
    kfree(
        pGbl_DriverObject
        );

    pGbl_DriverObject = NULL;

    DebugPrintf(("...driver unloaded\n"));
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
    RtlZeroMemory(
        fdo,
        sizeof(DEVICE_OBJECT)
        );

    fdo->DriverObject    = pDriverObject;        // Save parent driver object
    fdo->DeviceExtension = &(fdo->DeviceInfo);


    //
    // Initialize the device extension
    //

    pdx = fdo->DeviceExtension;

    // Clear device extension
    RtlZeroMemory( pdx, sizeof(DEVICE_EXTENSION) );

    // Store parent device object
    pdx->pDeviceObject = fdo;

    // Initialize device open mutex
    Plx_sema_init( &(pdx->Mutex_DeviceOpen), 1 );

    // Initialize device list
    INIT_LIST_HEAD(
        &(pdx->List_Devices)
        );

    // Initialize map parameters list
    INIT_LIST_HEAD(
        &(pdx->List_MapParams)
        );

    spin_lock_init(
        &(pdx->Lock_MapParamsList)
        );


    //
    // Add to driver device list
    //

    // Acquire Device List lock
    spin_lock(
        &(pDriverObject->Lock_DeviceList)
        );

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
            pDevice = pDevice->NextDevice;

        // Add device to end of list
        pDevice->NextDevice = fdo;
    }

    // Increment device count
    pDriverObject->DeviceCount++;

    // Release Device List lock
    spin_unlock(
        &(pDriverObject->Lock_DeviceList)
        );

    DebugPrintf((
        "Created Device (%s)\n",
        PLX_DRIVER_NAME
        ));

    return 0;
}




/*******************************************************************************
 *
 * Function   :  RemoveDevice
 *
 * Description:  Remove a device object from device list and delete it
 *
 ******************************************************************************/
int
RemoveDevice(
    DEVICE_OBJECT *fdo
    )
{
    struct list_head *pEntry;
    DEVICE_OBJECT    *pDevice;
    PLX_DEVICE_NODE  *pNode;
    DEVICE_EXTENSION *pdx;


    pdx = fdo->DeviceExtension;

    DebugPrintf(("Delete supported devices list\n"));

    // Free device list
    while (!list_empty(
                &(pdx->List_Devices)
                ))
    {
        // Get next list item
        pEntry = pdx->List_Devices.next;

        // Get the device node
        pNode =
            list_entry(
                pEntry,
                PLX_DEVICE_NODE,
                ListEntry
                );

        // Remove node from list
        list_del(
            pEntry
            );

        // Release the object
        kfree(
            pNode
            );
    }

    DebugPrintf((
        "Remove device (%s)\n",
        PLX_DRIVER_NAME
        ));

    // Acquire Device List lock
    spin_lock(
        &(pGbl_DriverObject->Lock_DeviceList)
        );

    // Get device list head
    pDevice = pGbl_DriverObject->DeviceObject;

    if (pDevice == NULL)
    {
        // Release Device List lock
        spin_unlock(
            &(pGbl_DriverObject->Lock_DeviceList)
            );

        ErrorPrintf(("ERROR - Unable to remove device, device list is empty\n"));
        return (-ENODEV);
    }

    if (pDevice == fdo)
    {
        // Remove device from first in list
        pGbl_DriverObject->DeviceObject = fdo->NextDevice;
    }
    else
    {
        // Scan list for the device
        while (pDevice->NextDevice != fdo)
        {
            pDevice = pDevice->NextDevice;

            if (pDevice == NULL)
            {
                // Release Device List lock
                spin_unlock(
                    &(pGbl_DriverObject->Lock_DeviceList)
                    );

                ErrorPrintf((
                    "ERROR - Device object (%p) not found in device list\n",
                    fdo
                    ));

                return (-ENODEV);
            }
        }

        // Remove device from list
        pDevice->NextDevice = fdo->NextDevice;
    }

    // Decrement device count
    pGbl_DriverObject->DeviceCount--;

    // Release Device List lock
    spin_unlock(
        &(pGbl_DriverObject->Lock_DeviceList)
        );

    DebugPrintf((
        "Delete device object (%p)\n",
        fdo
        ));

    // Delete the device object
    kfree(
        fdo
        );

    return 0;
}
