#ifndef __PCI_TYPES_H
#define __PCI_TYPES_H

/*******************************************************************************
 * Copyright 2013-2017 Avago Technologies
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
 *      PciTypes.h
 *
 * Description:
 *
 *      This file defines the basic types
 *
 * Revision:
 *
 *      06-01-19 : PLX SDK v8.00
 *
 ******************************************************************************/


#if defined(PLX_WDM_DRIVER)
    #include <wdm.h>            // WDM Driver types
#endif

#if defined(PLX_NT_DRIVER)
    #include <ntddk.h>          // NT Kernel Mode Driver (ie PLX Service)
#endif

#if defined(PLX_MSWINDOWS)
    #if !defined(PLX_DRIVER)
        #include <wtypes.h>     // Windows application level types
    #endif
#endif

// Must be placed before <linux/types.h> to prevent compile errors
#if defined(PLX_LINUX) && !defined(PLX_LINUX_DRIVER)
    #include <memory.h>         // To automatically add mem*() set of functions
#endif

#if defined(PLX_LINUX) || defined(PLX_LINUX_DRIVER)
    #include <linux/types.h>    // Linux types
#endif

#if defined(PLX_LINUX)
    #include <limits.h>         // For MAX_SCHEDULE_TIMEOUT in Linux applications
#endif


#ifdef __cplusplus
extern "C" {
#endif



/*******************************************
 *   Linux Application Level Definitions
 ******************************************/
#if defined(PLX_LINUX)
    typedef __s8                  S8;
    typedef __u8                  U8;
    typedef __s16                 S16;
    typedef __u16                 U16;
    typedef __s32                 S32;
    typedef __u32                 U32;
    typedef __s64                 S64;
    typedef __u64                 U64;
    typedef signed long           PLX_INT_PTR;        // For 32/64-bit code compatability
    typedef unsigned long         PLX_UINT_PTR;
    typedef int                   HANDLE;
    typedef int                   PLX_DRIVER_HANDLE;  // Linux-specific driver handle

    #define INVALID_HANDLE_VALUE  (HANDLE)-1

    #if !defined(MAX_SCHEDULE_TIMEOUT)
        #define MAX_SCHEDULE_TIMEOUT    LONG_MAX
    #endif
#endif



/*******************************************
 *    Linux Kernel Level Definitions
 ******************************************/
#if defined(PLX_LINUX_DRIVER)
    typedef s8                    S8;
    typedef u8                    U8;
    typedef s16                   S16;
    typedef u16                   U16;
    typedef s32                   S32;
    typedef u32                   U32;
    typedef s64                   S64;
    typedef u64                   U64;
    typedef signed long           PLX_INT_PTR;        // For 32/64-bit code compatability
    typedef unsigned long         PLX_UINT_PTR;
    typedef int                   PLX_DRIVER_HANDLE;  // Linux-specific driver handle
#endif



/*******************************************
 *      Windows Type Definitions
 ******************************************/
#if defined(PLX_MSWINDOWS)
    typedef signed char           S8;
    typedef unsigned char         U8;
    typedef signed short          S16;
    typedef unsigned short        U16;
    typedef signed long           S32;
    typedef unsigned long         U32;
    typedef signed __int64        S64;
    typedef unsigned __int64      U64;
    typedef INT_PTR               PLX_INT_PTR;        // For 32/64-bit code compatability
    typedef UINT_PTR              PLX_UINT_PTR;
    typedef HANDLE                PLX_DRIVER_HANDLE;  // Windows-specific driver handle

    #if defined(_DEBUG)
        #define PLX_DEBUG
    #endif
#endif



/*******************************************
 *    Windows WDM Driver Compatability
 ******************************************/
#if defined(PLX_WDM_DRIVER)
    // RtlIsNtDdiVersionAvailable supported in Windows Vista & higher
    #if (WINVER < 0x600)
        #define RtlIsNtDdiVersionAvailable(ver)     IoIsWdmVersionAvailable( (U8)(ver >> 24), (U8)(ver >> 16) )

        // Windows versions taken from SdkDdkVer.h
        #define NTDDI_WIN2K                         0x01100000  // WDM=1.10 Winver=5.00
        #define NTDDI_WINXP                         0x01200000  // WDM=1.20 Winver=5.01
        #define NTDDI_WS03                          0x01300000  // WDM=1.30 Winver=5.02
    #endif

    #if (WINVER < 0x601)
        #define NTDDI_WIN6                          0x06000000
        #define NTDDI_WIN6SP1                       0x06000100
        #define NTDDI_VISTA                         NTDDI_WIN6
        #define NTDDI_WS08                          NTDDI_WIN6SP1
        #define NTDDI_WIN7                          0x06010000
    #endif

    #if (WINVER < 0x602)
        #define NTDDI_WIN8                          0x06020000
        #define NTDDI_WIN10                         0x0A000000
    #endif

    #if (WINVER < 0x603)
        #define NTDDI_WINBLUE                       0x06030000  // Windows 8.1
    #endif

    #if (WINVER < 0xA00)
        #define NTDDI_WIN10                         0x0A000000
    #endif

    // Additional Win8+ DDK definitions
    #if (NTDDI_VER < NTDDI_WIN8)
        // More POOL_TYPEs added, needed for no-execute
        typedef enum _PLX_POOL_TYPE
        {
            NonPagedPoolBase                      = 0,
            NonPagedPoolBaseMustSucceed           = NonPagedPoolBase + 2,
            NonPagedPoolBaseCacheAligned          = NonPagedPoolBase + 4,
            NonPagedPoolBaseCacheAlignedMustS     = NonPagedPoolBase + 6,

            NonPagedPoolNx                        = 512,
            NonPagedPoolNxCacheAligned            = NonPagedPoolNx + 4,
            NonPagedPoolSessionNx                 = NonPagedPoolNx + 32
        } PLX_POOL_TYPE;

        // Additional -OR- flags for MM_PAGE_PRIORITY
        #define MdlMappingNoWrite       0x80000000  // Create the mapping as nowrite
        #define MdlMappingNoExecute     0x40000000  // Create the mapping as noexecute
    #endif

    #if (NTDDI_VER < NTDDI_WIN7)
        // Win7 DDK added typedef's for registered functions for declaration
        typedef
        NTSTATUS
        DRIVER_INITIALIZE(
            struct _DRIVER_OBJECT *DriverObject,
            PUNICODE_STRING RegistryPath
            );

        typedef
        VOID
        DRIVER_UNLOAD(
            struct _DRIVER_OBJECT *DriverObject
            );

        typedef
        NTSTATUS
        DRIVER_ADD_DEVICE(
            struct _DRIVER_OBJECT *DriverObject,
            struct _DEVICE_OBJECT *PhysicalDeviceObject
            );

        typedef
        NTSTATUS
        DRIVER_DISPATCH(
            struct _DEVICE_OBJECT *DeviceObject,
            struct _IRP *Irp
            );

        typedef
        VOID
        DRIVER_CANCEL(
            struct _DEVICE_OBJECT *DeviceObject,
            struct _IRP *Irp
            );

        typedef
        BOOLEAN
        KSERVICE_ROUTINE(
            struct _KINTERRUPT *Interrupt,
            PVOID ServiceContext
            );

        typedef
        VOID
        KDEFERRED_ROUTINE(
            struct _KDPC *Dpc,
            PVOID DeferredContext,
            PVOID SystemArgument1,
            PVOID SystemArgument2
            );

        typedef
        BOOLEAN
        KSYNCHRONIZE_ROUTINE (
            PVOID SynchronizeContext
            );

        typedef
        NTSTATUS
        IO_COMPLETION_ROUTINE (
            PDEVICE_OBJECT DeviceObject,
            PIRP Irp,
            PVOID Context
            );

        typedef
        VOID
        IO_WORKITEM_ROUTINE (
            PDEVICE_OBJECT DeviceObject,
            PVOID Context
            );

        typedef
        VOID
        REQUEST_POWER_COMPLETE (
            PDEVICE_OBJECT DeviceObject,
            UCHAR MinorFunction,
            POWER_STATE PowerState,
            PVOID Context,
            PIO_STATUS_BLOCK IoStatus
            );

    #endif
#endif



/*******************************************
 *        DOS Type Definitions
 ******************************************/
#if defined(PLX_DOS)
    typedef signed char           S8;
    typedef unsigned char         U8;
    typedef signed short          S16;
    typedef unsigned short        U16;
    typedef signed long           S32;
    typedef unsigned long         U32;
    typedef signed long long      S64;
    typedef unsigned long long    U64;
    typedef S32                   PLX_INT_PTR;        // For 32/64-bit code compatability
    typedef U32                   PLX_UINT_PTR;
    typedef unsigned long         HANDLE;
    typedef HANDLE                PLX_DRIVER_HANDLE;
    #define INVALID_HANDLE_VALUE  0

    #if !defined(_far)
        #define _far
    #endif
#endif



/*******************************************
 *    Volatile Basic Type Definitions
 ******************************************/
typedef volatile S8           VS8;
typedef volatile U8           VU8;
typedef volatile S16          VS16;
typedef volatile U16          VU16;
typedef volatile S32          VS32;
typedef volatile U32          VU32;
typedef volatile S64          VS64;
typedef volatile U64          VU64;



/*******************************************
 * Definitions used for ACPI & ECAM probe
 ******************************************/
// Used to scan ROM for services
#define BIOS_MEM_START                  0x000E0000
#define BIOS_MEM_END                    0x00100000

// ACPI probe states
#define ACPI_PCIE_NOT_PROBED            0
#define ACPI_PCIE_BYPASS_OS_OK          1
#define ACPI_PCIE_DEFAULT_TO_OS         2
#define ACPI_PCIE_ALWAYS_USE_OS         3

// ECAM
#define ECAM_PROBE_ADDR_START           0x80000000
#define ECAM_PROBE_ADDR_END             0xFF000000
#define ECAM_PROBE_ADDR_INCR            0x01000000

// Number of PCI registers to compare
#define ECAM_PROBE_REG_CMP_COUNT        4

// ECAM address offset
#define ECAM_DEVICE_REG_OFFSET( bus, dev, fn, off ) \
               (U32)( ( (bus) << 20) | \
                      ( (dev) << 15) | \
                      ( (fn)  << 12) | \
                      ( (off) <<  0) )

// ACPI RSDT v1.0 structure
typedef struct _ACPI_RSDT_v1_0
{
    U32 Signature;
    U32 Length;
    U8  Revision;
    U8  Oem_Id[6];
    U8  Oem_Table_Id[8];
    U32 Oem_Revision;
    U32 Creator_Id;
    U32 Creator_Revision;
} ACPI_RSDT_v1_0;




#ifdef __cplusplus
}
#endif

#endif
