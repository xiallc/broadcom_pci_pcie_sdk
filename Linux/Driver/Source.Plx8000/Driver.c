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
 *      09-01-10 : PLX SDK v6.40
 *
 *****************************************************************************/


#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/sched.h>
#include <linux/version.h>
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,0))
    #include <linux/vermagic.h>
#endif
#include "ApiFunc.h"
#include "Dispatch.h"
#include "Driver.h"
#include "PciFunc.h"
#include "PlxChipFn.h"
#include "PlxInterrupt.h"
#include "Plx_sysdep.h"
#include "SuppFunc.h"




/***********************************************
 *               Globals
 **********************************************/
// Pointer to the main driver object
DRIVER_OBJECT *pGbl_DriverObject_8000;




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
    PLX_PHYSICAL_MEM PhysicalMem;


    DebugPrintf_Cont(("\n"));
    DebugPrintf(("<========================================================>\n"));
    DebugPrintf((
        "PLX driver v%d.%02d (%d-bit) - built on %s %s\n",
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
    pGbl_DriverObject->DispatchTable.ioctl   = Dispatch_IoControl;
    pGbl_DriverObject->DispatchTable.mmap    = Dispatch_mmap;
    pGbl_DriverObject->DispatchTable.open    = Dispatch_open;
    pGbl_DriverObject->DispatchTable.release = Dispatch_release;

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

    // Probe ACPI tables for PCI Express mechanism
    PlxProbeForEcamBase();

    // Scan the system for supported devices
    PlxDeviceListBuild(
        pGbl_DriverObject
        );

    // Check if any devices were found
    if (pGbl_DriverObject->DeviceCount == 0)
    {
        ErrorPrintf(("ERROR - No supported devices found\n"));

        // Unload the driver
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
        DebugPrintf(("Allocating common buffer\n"));

        // Allocate common buffer
        PlxPciPhysicalMemoryAllocate(
            pGbl_DriverObject->DeviceObject->DeviceExtension,  // Assign buffer to first device
            &PhysicalMem,
            TRUE,                                              // Smaller buffer is ok
            pGbl_DriverObject                                  // Assign Driver object as owner
            );
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
    DebugPrintf(("Unloading driver...\n"));

    // Release common buffer
    if (pGbl_DriverObject->CommonBuffer.Size != 0)
    {
        DebugPrintf(("De-allocating Common Buffer\n"));

        // Release the buffer
        Plx_dma_buffer_free(
            pGbl_DriverObject->DeviceObject->DeviceExtension,   // First device
            &(pGbl_DriverObject->CommonBuffer)
            );
    }

    // Get the device list
    fdo = pGbl_DriverObject->DeviceObject;

    // Remove all devices
    while (fdo != NULL)
    {
        // Store next device
        pNext = fdo->NextDevice;

        // Stop device and release its resources
        StopDevice(
            fdo
            );

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
        "Releasing global driver object (%p)\n",
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
    RtlZeroMemory(
        fdo,
        sizeof(DEVICE_OBJECT)
        );

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
    RtlZeroMemory(
        pdx,
        sizeof(DEVICE_EXTENSION)
        );

    // Store parent device object
    pdx->pDeviceObject = fdo;

    // Save the OS-supplied PCI object
    pdx->pPciDevice = pPciDev;

    // Set initial device device state
    pdx->State = PLX_STATE_STOPPED;

    // Set initial power state
    pdx->PowerState = PowerDeviceD0;

    // Store device location information
    pdx->Key.bus          = pPciDev->bus->number;
    pdx->Key.slot         = PCI_SLOT(pPciDev->devfn);
    pdx->Key.function     = PCI_FUNC(pPciDev->devfn);
    pdx->Key.DeviceId     = pPciDev->device;
    pdx->Key.VendorId     = pPciDev->vendor;
    pdx->Key.SubVendorId  = pPciDev->subsystem_vendor;
    pdx->Key.SubDeviceId  = pPciDev->subsystem_device;
    pdx->Key.DeviceNumber = pDriverObject->DeviceCount;

    // Update Revision ID
    PLX_PCI_REG_READ(
        pdx,
        0x08,        // PCI Revision ID
        &RegValue
        );

    pdx->Key.Revision = (U8)(RegValue & 0xFF);

    // Determine & store the PLX chip type
    PlxChipTypeDetect(
        pdx
        );

    /************************************************************
     * Determine which side of NT port we are located on
     *
     * In Linux, devices other than 0 will get started later.
     * If DeviceFind is called, this will lead to an incomplete
     * NT port identification since the device hasn't been fully
     * probed. NT port identification is added here for now when
     * the device is added.  This will be removed in future when
     * PLX drivers are updated to use PCI registration.
     ***********************************************************/
    PlxDetermineNtPortSide(
        pdx
        );

    // Build device name
    sprintf(
        pdx->LinkName,
        PLX_DRIVER_NAME "-%d",
        pDriverObject->DeviceCount
        );

#if (LINUX_VERSION_CODE < KERNEL_VERSION(2,5,0))
    // Initialize task element for ISR DPC queueing
    pdx->Task_DpcForIsr.sync    = 0;
    pdx->Task_DpcForIsr.routine = DpcForIsr;               // DPC routine
    pdx->Task_DpcForIsr.data    = &(pdx->Task_DpcForIsr);  // DPC parameter
#else
    // Initialize work queue for ISR DPC queueing
    PLX_INIT_WORK(
        &(pdx->Task_DpcForIsr),
        DpcForIsr,                // DPC routine
        &(pdx->Task_DpcForIsr)    // DPC parameter (pre-2.6.20 only)
        );
#endif

    // Initialize device open mutex
    Plx_sema_init( &(pdx->Mutex_DeviceOpen), 1 );

    // Initialize ISR spinlock
    spin_lock_init(
        &(pdx->Lock_Isr)
        );

    // Initialize interrupt wait list
    INIT_LIST_HEAD(
        &(pdx->List_WaitObjects)
        );

    spin_lock_init(
        &(pdx->Lock_WaitObjectsList)
        );

    // Initialize physical memories list
    INIT_LIST_HEAD(
        &(pdx->List_PhysicalMem)
        );

    spin_lock_init(
        &(pdx->Lock_PhysicalMemList)
        );

    // Set buffer allocation mask
    if (Plx_dma_set_coherent_mask( pdx, PLX_DMA_BIT_MASK(32) ) != 0)
    {
        ErrorPrintf(("WARNING - Set DMA coherent mask failed\n"));
    }


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
        pdx->LinkName
        ));

    DebugPrintf_Cont(("\n"));

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

    DebugPrintf((
        "Removing device (%s)\n",
        pdx->LinkName
        ));

    // Acquire Device List lock
    spin_lock(
        &(fdo->DriverObject->Lock_DeviceList)
        );

    // Get device list head
    pDevice = fdo->DriverObject->DeviceObject;

    if (pDevice == NULL)
    {
        // Release Device List lock
        spin_unlock(
            &(fdo->DriverObject->Lock_DeviceList)
            );

        ErrorPrintf(("ERROR - Unable to remove device, device list is empty\n"));
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
                // Release Device List lock
                spin_unlock(
                    &(fdo->DriverObject->Lock_DeviceList)
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
        &(fdo->DriverObject->Lock_DeviceList)
        );

    // Disable the device
    pci_disable_device( pdx->pPciDevice );
    DebugPrintf(("Disabled PCI device\n"));

    DebugPrintf((
        "Deleting device object (%p)\n",
        fdo
        ));

    // Release device object
    kfree(
        fdo
        );

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
    U8                ResourceCount;
    DEVICE_EXTENSION *pdx;


    if (fdo->DeviceExtension->State == PLX_STATE_STARTED)
        return 0;

    DebugPrintf(("Starting device...\n"));

    pdx           = fdo->DeviceExtension;
    ResourceCount = 0;

    for (i = 0; i < PCI_NUM_BARS_TYPE_00; ++i)
    {
        // Verify the address is valid
        if (pci_resource_start(
                pdx->pPciDevice,
                i
                ) == 0)
        {
            continue;
        }

        DebugPrintf(("   Resource %02d\n", ResourceCount));

        // Increment resource count
        ResourceCount++;

        // Get PCI physical address
        pdx->PciBar[i].Properties.Physical =
            pci_resource_start(
                pdx->pPciDevice,
                i
                );

        // Determine resource type
        if (pci_resource_flags(
                pdx->pPciDevice,
                i
                ) & IORESOURCE_IO)
        {
            DebugPrintf(("     Type     : I/O Port\n"));

            // Make sure flags are cleared properly
            pdx->PciBar[i].Properties.Physical &= ~(0x3);
            pdx->PciBar[i].Properties.bIoSpace  = TRUE;
        }
        else
        {
            DebugPrintf(("     Type     : Memory Space\n"));

            // Make sure flags are cleared properly
            pdx->PciBar[i].Properties.Physical &= ~(0xf);
            pdx->PciBar[i].Properties.bIoSpace  = FALSE;
        }

        // Get the actual BAR value
        PLX_PCI_REG_READ(
            pdx,
            0x10 + (i * sizeof(U32)),
            &(pdx->PciBar[i].Properties.BarValue)
            );

        DebugPrintf((
            "     PCI BAR %d: %08X\n",
            i, pdx->PciBar[i].Properties.BarValue
            ));

        DebugPrintf((
            "     Phys Addr: %08lX\n",
            (PLX_UINT_PTR)pdx->PciBar[i].Properties.Physical
            ));

        // Get the size
        pdx->PciBar[i].Properties.Size =
            pci_resource_len(
                pdx->pPciDevice,
                i
                );

        if (pdx->PciBar[i].Properties.Size >= (1 << 10))
        {
            DebugPrintf((
                "     Size     : %08lx  (%ld Kb)\n",
                (PLX_UINT_PTR)pdx->PciBar[i].Properties.Size,
                (PLX_UINT_PTR)pdx->PciBar[i].Properties.Size >> 10
                ));
        }
        else
        {
            DebugPrintf((
                "     Size     : %08lx  (%ld bytes)\n",
                (PLX_UINT_PTR)pdx->PciBar[i].Properties.Size,
                (PLX_UINT_PTR)pdx->PciBar[i].Properties.Size
                ));
        }

        // Set prefetch flag
        if ((pdx->PciBar[i].Properties.bIoSpace == FALSE) &&
            (pci_resource_flags(pdx->pPciDevice, i) & IORESOURCE_PREFETCH))
        {
            pdx->PciBar[i].Properties.bPrefetchable = TRUE;
        }

        DebugPrintf((
            "     Prefetch?: %s\n",
            (pdx->PciBar[i].Properties.bPrefetchable) ? "Yes" : "No"
            ));

        // Claim and map the resource
        rc =
            PlxPciBarResourceMap(
                pdx,
                i
                );

        if (rc == 0)
        {
            if (pdx->PciBar[i].Properties.bIoSpace == FALSE)
            {
                DebugPrintf((
                    "     Kernel VA: %p\n",
                    pdx->PciBar[i].pVa
                    ));
            }
        }
        else
        {
            if (pdx->PciBar[i].Properties.bIoSpace == FALSE)
            {
                ErrorPrintf(("     Kernel VA: ERROR - Unable to map space to Kernel VA\n"));
            }
        }
    }

    // Check for interrupt resource
    if (pdx->pPciDevice->irq != 0)
    {
        DebugPrintf(("   Resource %02d\n", ResourceCount));
        DebugPrintf(("     Type     : Interrupt\n"));
        DebugPrintf(("     IRQ      : %02d [%02Xh]\n",
                     pdx->pPciDevice->irq, pdx->pPciDevice->irq));

        // Increment resource count
        ResourceCount++;
    }

    // Map BAR 0 for register access
    if (PlxMapRegisterBar(
            pdx
            ) != TRUE)
    {
        DebugPrintf(("ERROR - Unable to map BAR 0 for register access\n"));
        return (-ENOMEM);
    }

    // Determine which side of NT port we are located on
    PlxDetermineNtPortSide(
        pdx
        );

    // Implement work-around for NT BAR shadow errata
    PlxErrataWorkAround_NtBarShadow(
        pdx
        );

    // Implement work-around for NT Link-side captured requester ID errata
    PlxErrataWorkAround_NtCapturedRequesterID(
        pdx
        );

    // Sets the interrupt register offsets based on chip type
    PlxChipSetInterruptRegisterOffsets(
        pdx
        );

    // Disable all interrupts
    PlxChipInterruptsDisable(
        pdx
        );

    // Install the ISR if available
    if (pdx->pPciDevice->irq != 0)
    {
        rc =
            request_irq(
                pdx->pPciDevice->irq,    // The device IRQ
                OnInterrupt,             // Interrupt handler
                PLX_IRQF_SHARED,         // Flags, support interrupt sharing
                PLX_DRIVER_NAME,         // The driver name
                pdx                      // Parameter to the ISR
                );

        if (rc != 0)
        {
            ErrorPrintf(("ERROR - Unable to install ISR\n"));
        }
        else
        {
            DebugPrintf(("Installed ISR for interrupt\n"));

            // Flag that the interrupt resource was claimed
            pdx->Flag_Interrupt = TRUE;

            // Re-enable interrupts
            PlxChipInterruptsEnable(
                pdx
                );
        }
    }
    else
    {
        DebugPrintf(("Device is not using a PCI interrupt resource\n"));
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

    // Only stop devices which have been started
    if (pdx->State == PLX_STATE_STOPPED)
    {
        return;
    }

    // Notify ISR not to schedule any more DPCs
    pdx->State = PLX_STATE_STOPPING;

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

    DebugPrintf(("Releasing device resources...\n"));

    if (pdx->Flag_Interrupt == TRUE)
    {
        // Disable all interrupts
        PlxChipInterruptsDisable(
            pdx
            );

        DebugPrintf((
            "Removing interrupt handler (IRQ = %02d [%02Xh])\n",
            pdx->pPciDevice->irq, pdx->pPciDevice->irq
            ));

        // Release IRQ
        free_irq(
            pdx->pPciDevice->irq,
            pdx
            );

        // Flag that the interrupt resource was released
        pdx->Flag_Interrupt = FALSE;
    }

    // Mark registers are no longer mapped
    pdx->pRegVa = NULL;

    // Unmap I/O regions from kernel space (No local register access after this)
    PlxPciBarResourcesUnmap(
        pdx
        );

    // Unmap the upstream BAR 0 space for NT virtual port
    if ((pdx->pRegVa != NULL) && (pdx->UpstreamBarSize != 0))
    {
        DebugPrintf((
            "Unmap upstream port PCI BAR 0 (VA=%p)\n",
            pdx->pRegVa
            ));

        iounmap(
            pdx->pRegVa
            );
        pdx->UpstreamBarSize = 0;
    }

    // Update device state
    pdx->State = PLX_STATE_STOPPED;
}




/*******************************************************************************
 *
 * Function   :  PlxDeviceListBuild
 *
 * Description:  Scan OS-generated PCI device list to search for supported devices
 *
 ******************************************************************************/
int
PlxDeviceListBuild(
    DRIVER_OBJECT *pDriverObject
    )
{
    int             status;
    int             DeviceCount;
    struct pci_dev *pPciDev;


    DebugPrintf(("Scanning PCI bus for supported devices\n"));

    // Clear device count
    DeviceCount = 0;

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
            "Scanning - %04X %04X  [b:%02x s:%02x f:%02x]\n",
            pPciDev->device, pPciDev->vendor,
            pPciDev->bus->number, PCI_SLOT(pPciDev->devfn),
            PCI_FUNC(pPciDev->devfn)
            ));

        // Check if device is supported
        if (IsSupportedDevice(
                pDriverObject,
                pPciDev
                ))
        {
            DebugPrintf(("SUPPORTED - Adding device to list\n")); 

            // Add the device to the device list
            status =
                AddDevice(
                    pDriverObject,
                    pPciDev
                    );

            if (status == 0)
            {
                // Increment number of devices found
                DeviceCount++;
            }
        }

        // Jump to next device
        pPciDev =
            Plx_pci_get_device(
                PCI_ANY_ID,
                PCI_ANY_ID,
                pPciDev     // Ref count will be decremented for this device
                );
    }

    DebugPrintf((
        "Device Scan: %d supported device(s) found\n",
        DeviceCount
        ));

    return DeviceCount;
}
