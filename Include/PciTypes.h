#ifndef __PCI_TYPES_H
#define __PCI_TYPES_H

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
 *      PciTypes.h
 *
 * Description:
 *
 *      This file defines the basic types
 *
 * Revision:
 *
 *      02-01-12 : PLX SDK v7.00
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
    #define PLX_SIZE_64           8
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
    #define PLX_SIZE_64           8
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
    typedef signed _int64         S64;
    typedef unsigned _int64       U64;
    typedef INT_PTR               PLX_INT_PTR;        // For 32/64-bit code compatability
    typedef UINT_PTR              PLX_UINT_PTR;

    typedef HANDLE                PLX_DRIVER_HANDLE;  // Windows-specific driver handle
    #define PLX_SIZE_64           8

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
    #endif

    #if (WINVER < 0x601)
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
    #define PLX_SIZE_64           8

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




#ifdef __cplusplus
}
#endif

#endif
