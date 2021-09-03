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
#include <linux/mm.h>
#include <linux/version.h>
#include <linux/workqueue.h>
#include "Plx.h"
#include "PlxChip.h"
#include "PlxTypes.h"
#include "Plx_sysdep.h"




/**********************************************
 *               Definitions
 *********************************************/
#define PLX_MNGMT_INTERFACE                 0xff          // Minor number of Management interface
#define PLX_MAX_NAME_LENGTH                 0x20          // Max length of registered device name
#define MIN_WORKING_POWER_STATE             PowerDeviceD2 // Minimum state required for local register access

// Used for build of SGL descriptors
#define SGL_DESC_IDX_PCI_LOW                0
#define SGL_DESC_IDX_LOC_ADDR               1
#define SGL_DESC_IDX_COUNT                  2
#define SGL_DESC_IDX_NEXT_DESC              3
#define SGL_DESC_IDX_PCI_HIGH               4

// Used to dump SGL descriptors in debug mode  (0 = Do Not Display   1 = Display SGL Descriptors)
#if defined(PLX_DISPLAY_SGL)
    #define PLX_DEBUG_DISPLAY_SGL_DESCR     1
#else
    #define PLX_DEBUG_DISPLAY_SGL_DESCR     0
#endif




/***********************************************************
 * The following definition makes the global object pointer
 * unique for each driver.  This is required to avoid a name
 * conflict in the global kernel namespace if multiple PLX
 * drivers are loaded, but still allows a common codebase.
 **********************************************************/
#if (PLX_CHIP == 9050)
    #define pGbl_DriverObject               pGbl_DriverObject_9050
#elif (PLX_CHIP == 9030)
    #define pGbl_DriverObject               pGbl_DriverObject_9030
#elif (PLX_CHIP == 9080)
    #define pGbl_DriverObject               pGbl_DriverObject_9080
#elif (PLX_CHIP == 9054)
    #define pGbl_DriverObject               pGbl_DriverObject_9054
#elif (PLX_CHIP == 9056)
    #define pGbl_DriverObject               pGbl_DriverObject_9056
#elif (PLX_CHIP == 9656)
    #define pGbl_DriverObject               pGbl_DriverObject_9656
#elif (PLX_CHIP == 8311)
    #define pGbl_DriverObject               pGbl_DriverObject_8311
#endif




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



// Macros for Memory access to/from user-space addresses
#define DEV_MEM_TO_USER_8( VaUser, VaDev, count)    Plx_dev_mem_to_user_8(  (U8*)(VaUser),  (U8*)(VaDev), (count))
#define DEV_MEM_TO_USER_16(VaUser, VaDev, count)    Plx_dev_mem_to_user_16((U16*)(VaUser), (U16*)(VaDev), (count))
#define DEV_MEM_TO_USER_32(VaUser, VaDev, count)    Plx_dev_mem_to_user_32((U32*)(VaUser), (U32*)(VaDev), (count))
#define USER_TO_DEV_MEM_8( VaDev, VaUser, count)    Plx_user_to_dev_mem_8(  (U8*)(VaDev),   (U8*)(VaUser), (count))
#define USER_TO_DEV_MEM_16(VaDev, VaUser, count)    Plx_user_to_dev_mem_16((U16*)(VaDev),  (U16*)(VaUser), (count))
#define USER_TO_DEV_MEM_32(VaDev, VaUser, count)    Plx_user_to_dev_mem_32((U32*)(VaDev),  (U32*)(VaUser), (count))



// Macros for I/O port access
#define IO_PORT_READ_8(port)                        inb( (U16)(port) )
#define IO_PORT_READ_16(port)                       inw( (U16)(port) )
#define IO_PORT_READ_32(port)                       inl( (U16)(port) )
#define IO_PORT_WRITE_8(port, val)                  outb( (val), (U16)(port) )
#define IO_PORT_WRITE_16(port, val)                 outw( (val), (U16)(port) )
#define IO_PORT_WRITE_32(port, val)                 outl( (val), (U16)(port) )



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
#define PLX_9000_REG_READ(pdx, offset)   \
    PHYS_MEM_READ_32(                    \
        (U32*)((pdx)->pRegVa + (offset)) \
        )

#define PLX_9000_REG_WRITE(pdx, offset, value) \
    PHYS_MEM_WRITE_32(                         \
        (U32*)((pdx)->pRegVa + (offset)),      \
        (value)                                \
        )



// PCI Interrupt wait object
typedef struct _PLX_WAIT_OBJECT
{
    struct list_head   ListEntry;
    VOID              *pOwner;
    U32                Notify_Flags;            // Registered interrupt(s) for notification
    U32                Notify_Doorbell;         // Registered doorbell interrupt(s) for notification
    U32                Source_Ints;             // Interrupt(s) that caused notification
    U32                Source_Doorbell;         // Doorbells that caused notification
    PLX_STATE          state;                   // Current state of the object
    atomic_t           SleepCount;              // Number of currently sleeping threads for this object
    wait_queue_head_t  WaitQueue;
} PLX_WAIT_OBJECT;


// Argument for interrupt source access functions
typedef struct _PLX_INTERRUPT_DATA
{
    struct _DEVICE_EXTENSION *pdx;
    U32                       Source_Ints;
    U32                       Source_Doorbell;
} PLX_INTERRUPT_DATA;


// Information about contiguous, page-locked buffers
typedef struct _PLX_PHYS_MEM_OBJECT
{
    struct list_head  ListEntry;
    VOID             *pOwner;
    U8               *pKernelVa;
    U64               CpuPhysical;              // CPU Physical Address
    U64               BusPhysical;              // Bus Physical Address
    U32               Size;                     // Buffer size
} PLX_PHYS_MEM_OBJECT;


// PCI BAR Space information
typedef struct _PLX_PCI_BAR_INFO
{
    U8               *pVa;                      // BAR Kernel Virtual Address
    PLX_PCI_BAR_PROP  Properties;               // BAR Properties
    BOOLEAN           bResourceClaimed;         // Was driver able to claim region?
} PLX_PCI_BAR_INFO;


// DMA channel information 
typedef struct _PLX_DMA_INFO
{
    VOID                 *pOwner;               // Object that requested to open the channel
    BOOLEAN               bOpen;                // Flag to note if DMA channel is open
    BOOLEAN               bSglPending;          // Flag to note if an SGL DMA is pending
    BOOLEAN               bConstAddrLocal;      // Flag to keep track if local address remains constant
    U32                   NumPages;             // Number of pages mapped for user buffer
    U32                   InitialOffset;        // Initial offset of user buffer
    U32                   BufferSize;           // Total size of the user buffer
    int                   direction;            // The direction of the transfer
    struct page         **PageList;             // List of locked user pages
    PLX_PHYS_MEM_OBJECT   SglBuffer;            // Current SGL descriptor list buffer
} PLX_DMA_INFO;


// Argument for ISR synchronized register access
typedef struct _PLX_REG_DATA
{
    struct _DEVICE_EXTENSION *pdx;
    U32                       offset;
    U32                       BitsToSet;
    U32                       BitsToClear;
} PLX_REG_DATA;


// All relevant information about the device
typedef struct _DEVICE_EXTENSION
{
    struct _DEVICE_OBJECT *pDeviceObject;                 // Pointer to parent device object
    struct pci_dev        *pPciDevice;                    // Pointer to OS-supplied PCI device information
    PLX_STATE              State;                         // Start/Stop state of the device
    PLX_DEVICE_KEY         Key;                           // Device location & identification
    char                   LinkName[PLX_MAX_NAME_LENGTH];
    PLX_PCI_BAR_INFO       PciBar[PCI_NUM_BARS_TYPE_00];
    DEVICE_POWER_STATE     PowerState;                    // Power management information

    spinlock_t             Lock_Isr;                      // Spinlock used to sync with ISR
    struct work_struct     Task_DpcForIsr;                // Task queue used by ISR to queue DPC
    BOOLEAN                bDpcPending;                   // Flag whether a DPC task is scheduled
    PLX_IRQ_TYPE           IrqType;                       // Type of interrupt used
    U8                     IrqPci;                        // Original PCI IRQ Line assigned to device
    U32                    Source_Ints;                   // Interrupts detected by ISR
    U32                    Source_Doorbell;               // Doorbell interrupts detected by ISR
    U8                    *pRegVa;                        // Virtual address to registers

    struct list_head       List_WaitObjects;              // List of registered notification objects
    spinlock_t             Lock_WaitObjectsList;          // Spinlock for notification objects list

    struct list_head       List_PhysicalMem;              // List of user-allocated physical memory
    spinlock_t             Lock_PhysicalMemList;          // Spinlock for physical memory list

#if defined(PLX_DMA_SUPPORT)
    PLX_DMA_INFO           DmaInfo[NUM_DMA_CHANNELS];     // DMA properties and lock
    spinlock_t             Lock_Dma[NUM_DMA_CHANNELS];
#endif

} DEVICE_EXTENSION; 


// Main driver object
typedef struct _DRIVER_OBJECT
{
    struct _DEVICE_OBJECT  *DeviceObject;     // Pointer to first device in list
    U8                      DeviceCount;      // Number of devices in list
    spinlock_t              Lock_DeviceList;  // Spinlock for device list
    int                     MajorID;          // The OS-assigned driver Major ID
    BOOLEAN                 bPciDriverReg;    // Flag whether the driver was registered as PCI
    PLX_PHYS_MEM_OBJECT     CommonBuffer;     // Contiguous memory to be shared by all processes
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
