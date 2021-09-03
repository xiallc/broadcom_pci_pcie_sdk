#ifndef __DRIVER_DEFS_H
#define __DRIVER_DEFS_H

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
 *      DrvDefs.h
 *
 * Description:
 *
 *      Common definitions used in the driver
 *
 * Revision History:
 *
 *      05-01-13 : PLX SDK v7.10
 *
 ******************************************************************************/


#include <asm/io.h>
#include <linux/fs.h>
#include <linux/list.h>
#include <linux/spinlock.h>
#include "Plx.h"
#include "PlxTypes.h"
#include "Plx_sysdep.h"




/**********************************************
 *               Definitions
 *********************************************/
#define PLX_DRIVER_NAME                     "PlxSvc"
#define PLX_MNGMT_INTERFACE                 0xff          // Minor number of Management interface
#define PLX_MAX_NAME_LENGTH                 0x20          // Max length of registered device name



/***********************************************************
 * The following definition makes the global object pointer
 * unique for each driver.  This is required to avoid a name
 * conflict in the global kernel namespace if multiple PLX
 * drivers are loaded, but still allows a common codebase.
 **********************************************************/
#define pGbl_DriverObject                   pGbl_DriverObject_PlxSvc



// Macros to support Kernel-level logging in Debug builds
#if defined(PLX_DEBUG)
    #define DebugPrintf(arg)                _Debug_Print_Macro      arg
    #define DebugPrintf_Cont(arg)           _Debug_Print_Cont_Macro arg
#else
    #define DebugPrintf(arg)                do { } while(0)
    #define DebugPrintf_Cont(arg)           do { } while(0)
#endif
#define ErrorPrintf(arg)                    _Error_Print_Macro      arg
#define ErrorPrintf_Cont(arg)               _Error_Print_Cont_Macro arg

#define _Debug_Print_Macro(fmt, args...)             printk(KERN_DEBUG   PLX_DRIVER_NAME ": " fmt, ## args)
#define _Error_Print_Macro(fmt, args...)             printk(KERN_WARNING PLX_DRIVER_NAME ": " fmt, ## args)
#if (LINUX_VERSION_CODE < KERNEL_VERSION(2,6,24))
    #define _Debug_Print_Cont_Macro(fmt, args...)    printk(KERN_DEBUG   "\b\b\b   \b\b\b" fmt, ## args)
    #define _Error_Print_Cont_Macro(fmt, args...)    printk(KERN_WARNING "\b\b\b   \b\b\b" fmt, ## args)
#else
    #define _Debug_Print_Cont_Macro(fmt, args...)    printk(KERN_CONT  fmt, ## args)
    #define _Error_Print_Cont_Macro(fmt, args...)    printk(KERN_CONT  fmt, ## args)
#endif



// Macros for I/O port access
#define IO_PORT_READ_8(port)                        inb((port))
#define IO_PORT_READ_16(port)                       inw((port))
#define IO_PORT_READ_32(port)                       inl((port))
#define IO_PORT_WRITE_8(port, val)                  outb((val), (port))
#define IO_PORT_WRITE_16(port, val)                 outw((val), (port))
#define IO_PORT_WRITE_32(port, val)                 outl((val), (port))


/***********************************************************
 * Macros for device memory access
 *
 * ioreadX() and iowriteX() functions were added to kernel 2.6,
 * but do not seem to be present in all kernel distributions.
 **********************************************************/
#if defined(ioread8)
    #define PHYS_MEM_READ_8                         ioread8
    #define PHYS_MEM_READ_16                        ioread16
    #define PHYS_MEM_READ_32                        ioread32
    #define PHYS_MEM_WRITE_8(addr, data)            iowrite8 ( (data), (addr) )
    #define PHYS_MEM_WRITE_16(addr, data)           iowrite16( (data), (addr) )
    #define PHYS_MEM_WRITE_32(addr, data)           iowrite32( (data), (addr) )
#else
    #define PHYS_MEM_READ_8                         readb
    #define PHYS_MEM_READ_16                        readw
    #define PHYS_MEM_READ_32                        readl
    #define PHYS_MEM_WRITE_8(addr, data)            writeb( (data), (addr) )
    #define PHYS_MEM_WRITE_16(addr, data)           writew( (data), (addr) )
    #define PHYS_MEM_WRITE_32(addr, data)           writel( (data), (addr) )
#endif



// Macros for PLX chip register access
#define PLX_8111_REG_READ(pNode, offset)            PlxRegisterRead_8111 ((pNode), (offset), NULL)
#define PLX_8111_REG_WRITE(pNode, offset, val)      PlxRegisterWrite_8111((pNode), (offset), (val))

#define PLX_8000_REG_READ(pNode, offset)            PlxRegisterRead_8000 ((pNode), (offset), NULL, FALSE)
#define PLX_8000_REG_WRITE(pNode, offset, val)      PlxRegisterWrite_8000((pNode), (offset), (val), FALSE)



// User mapping request parameters
typedef struct _PLX_USER_MAPPING
{
    struct list_head         ListEntry;
    struct _PLX_DEVICE_NODE *pDevice;
    U8                       BarIndex;
} PLX_USER_MAPPING;


// PCI BAR Space information
typedef struct _PLX_PCI_BAR_INFO
{
    U8               *pVa;                      // BAR Kernel Virtual Address
    PLX_PCI_BAR_PROP  Properties;               // BAR Properties
} PLX_PCI_BAR_INFO;


// Device node information
typedef struct _PLX_DEVICE_NODE
{
    struct list_head          ListEntry;
    struct pci_dev           *pPciDevice;                   // Pointer to OS-supplied PCI device information
    struct _DEVICE_EXTENSION *pdx;                          // Pointer to parent device object
    struct _PLX_DEVICE_NODE  *pParent;                      // The parent P2P port
    struct _PLX_DEVICE_NODE  *pRegNode;                     // The device to use for register access
    PLX_DEVICE_KEY            Key;                          // Device location & identification
    U8                        PciHeaderType;                // PCI header type
    U32                       PciClass;                     // PCI class code
    PLX_PCI_BAR_INFO          PciBar[PCI_NUM_BARS_TYPE_00]; // PCI BARs information
    U8                        bBarKernelMap;                // Flag whether BARs have been mapped to kernel
    U8                        PortNumber;                   // PCIe Port number of device
    U8                        Default_EepWidth;             // Default width for EEPROM access
    U32                       Offset_NtRegBase;             // The NT register base offset
    U8                        MapRequestPending;            // Number of pending user mapping requests
} PLX_DEVICE_NODE;


// All relevant information about the device
typedef struct _DEVICE_EXTENSION
{
    struct _DEVICE_OBJECT *pDeviceObject;          // Device object this extension belongs to
    U8                     OpenCount;              // Count of open connections to the device
    struct semaphore       Mutex_DeviceOpen;       // Mutex for opening/closing the device
    struct list_head       List_Devices;           // List of detected devices
    struct list_head       List_MapParams;         // Stores information about an upcoming mapping request
    spinlock_t             Lock_MapParamsList;     // Spinlock for map parameters list
} DEVICE_EXTENSION; 


// Main driver object
typedef struct _DRIVER_OBJECT
{
    struct _DEVICE_OBJECT  *DeviceObject;     // Pointer to first device in list
    U8                      DeviceCount;      // Number of devices in list
    spinlock_t              Lock_DeviceList;  // Spinlock for device list
    int                     MajorID;          // The OS-assigned driver Major ID
    struct file_operations  DispatchTable;    // Driver dispatch table
} DRIVER_OBJECT;


// The device object
typedef struct _DEVICE_OBJECT
{
    struct _DEVICE_OBJECT *NextDevice;       // Pointer to next device in list
    DRIVER_OBJECT         *DriverObject;     // Pointer to parent driver object
    DEVICE_EXTENSION      *DeviceExtension;  // Pointer to device information
    DEVICE_EXTENSION       DeviceInfo;       // Device information
} DEVICE_OBJECT;




/**********************************************
 *               Globals
 *********************************************/
extern DRIVER_OBJECT *pGbl_DriverObject;       // Pointer to the main driver object



#endif
