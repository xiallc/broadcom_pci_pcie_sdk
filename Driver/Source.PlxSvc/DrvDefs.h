#ifndef __DRIVER_DEFS_H
#define __DRIVER_DEFS_H

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
 *      DrvDefs.h
 *
 * Description:
 *
 *      Common definitions used in the driver
 *
 * Revision History:
 *
 *      03-01-18 : PLX SDK v8.00
 *
 ******************************************************************************/


#include <asm/io.h>
#include <linux/fs.h>
#include <linux/list.h>
#include <linux/spinlock.h>
#include "PciRegs.h"
#include "PlxTypes.h"
#include "Plx_sysdep.h"




/**********************************************
 *               Definitions
 *********************************************/
#define PLX_DRIVER_NAME                     "PlxSvc"
#define PLX_MNGMT_INTERFACE                 0xff          // Minor number of Management interface
#define PLX_MAX_NAME_LENGTH                 0x20          // Max length of registered device name

// Atlas PEX registers start at offset 8MB in BAR 0
#define ATLAS_PEX_REGS_BASE_OFFSET          0x800000



/***********************************************************
 * The following definition makes the global object pointer
 * unique for each driver.  This is required to avoid a name
 * conflict in the global kernel namespace if multiple PLX
 * drivers are loaded, but still allows a common codebase.
 **********************************************************/
#define pGbl_DriverObject                   pGbl_DriverObject_PlxSvc



// Macros to support Kernel-level logging in Debug builds
#if defined(PLX_DEBUG)
    #define DebugPrintf(arg)                         _Debug_Print_Macro      arg
    #define DebugPrintf_Cont(arg)                    _Debug_Print_Cont_Macro arg
#else
    #define DebugPrintf(arg)                         do { } while(0)
    #define DebugPrintf_Cont(arg)                    do { } while(0)
#endif
#define InfoPrintf(arg)                              _Info_Print_Macro       arg
#define InfoPrintf_Cont(arg)                         _Info_Print_Cont_Macro  arg
#define ErrorPrintf(arg)                             _Error_Print_Macro      arg
#define ErrorPrintf_Cont(arg)                        _Error_Print_Cont_Macro arg

#define _Debug_Print_Macro(fmt, args...)             printk(KERN_DEBUG PLX_DRIVER_NAME ": " fmt, ## args)
#define _Info_Print_Macro(fmt, args...)              printk(KERN_INFO  PLX_DRIVER_NAME ": " fmt, ## args)
#define _Error_Print_Macro(fmt, args...)             printk(KERN_ERR   PLX_DRIVER_NAME ": " fmt, ## args)
#if (LINUX_VERSION_CODE < KERNEL_VERSION(2,6,24))
    #define _Debug_Print_Cont_Macro(fmt, args...)    printk(KERN_DEBUG "\b\b\b   \b\b\b" fmt, ## args)
    #define _Info_Print_Cont_Macro(fmt, args...)     printk(KERN_INFO  "\b\b\b   \b\b\b" fmt, ## args)
    #define _Error_Print_Cont_Macro(fmt, args...)    printk(KERN_ERR   "\b\b\b   \b\b\b" fmt, ## args)
#else
    #define _Debug_Print_Cont_Macro(fmt, args...)    printk(KERN_CONT  fmt, ## args)
    #define _Info_Print_Cont_Macro(fmt, args...)     printk(KERN_CONT  fmt, ## args)
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
    struct _DEVICE_EXTENSION *pdo;                          // Pointer to parent device object
    struct _PLX_DEVICE_NODE  *pParent;                      // The parent P2P port
    struct _PLX_DEVICE_NODE  *pRegNode;                     // The device to use for register access
    PLX_DEVICE_KEY            Key;                          // Device location & identification
    PLX_PORT_PROP             PortProp;                     // Port properties
    U8                        PciHeaderType;                // PCI header type
    U32                       PciClass;                     // PCI class code
    PLX_PCI_BAR_INFO          PciBar[PCI_NUM_BARS_TYPE_00]; // PCI BARs information
    U8                        bBarKernelMap;                // Flag whether BARs have been mapped to kernel
    U8                        Default_EepWidth;             // Default width for EEPROM access
    U8                        MapRequestPending;            // Number of pending user mapping requests
    U32                       Offset_NtRegBase;             // The NT register base offset
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
    spinlock_t              Lock_DeviceList;  // Spinlock for device list
    U16                     DeviceCount;      // Number of devices in list
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
