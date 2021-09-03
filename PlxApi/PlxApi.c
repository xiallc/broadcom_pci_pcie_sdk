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
 *     PlxApi.c
 *
 * Description:
 *
 *     This file contains all the PLX API functions
 *
 * Revision:
 *
 *     08-01-13: PLX SDK v7.10
 *
 *****************************************************************************/

/*******************************************************************************
 *
 * PCI API utilization of fields in PLX_DEVICE_KEY
 *
 *  ApiIndex        - Index into PlxDrivers[] for driver name
 *  DeviceNumber    - Device number in registerd driver name (e.g. Plx9054-1)
 *  ApiMode         - Mode API is using to connect to device (PCI,I2C,TCP,etc)
 *
 ******************************************************************************/


#if defined(_WIN32)
    /*******************************************************************
     * Some standard C functions (e.g. strcpy) have recently been declared
     * deprecated in favor of new secure counterparts (e.g. strcpy_s).
     * Currently, the PLX API still uses some deprecated functions.  To
     * avoid numerous warnings during Visual Studio compilation, the
     * items below were added.  These may be removed in future.
     *******************************************************************/
    #define _CRT_SECURE_NO_DEPRECATE        // Prevents deprecation warnings during compile
    #pragma warning( once : 4996 )          // Limits deprecation warnings to display only once
#endif

#if defined(PLX_LINUX)
    #include <sys/mman.h>
#endif

#if defined(PLX_DOS)
    #include <stdio.h>
    #include <stdlib.h>
    #include <string.h>
    #include "Dispatch.h"
    #include "Driver.h"
#endif

#include <stdarg.h>            // For va_start/va_end
#include "PlxApi.h"
#include "PlxApiDebug.h"
#include "PlxApiI2cAa.h"
#include "PlxIoctl.h"




/**********************************************
 *               Definitions
 *********************************************/
#define PLX_SVC_DRIVER_NAME             "PlxSvc"            // PLX PCI Service driver name


#if defined(PLX_MSWINDOWS)

    #define DRIVER_PATH                 "\\\\.\\"           // Path to drivers
    #define Driver_Disconnect           CloseHandle

#elif defined(PLX_LINUX)

    #if !defined(PAGE_MASK)
        #define PAGE_MASK               (~(getpagesize() - 1))
    #endif

    #if !defined(PAGE_ALIGN)
        #define PAGE_ALIGN(addr)        (((addr) + getpagesize() - 1) & PAGE_MASK)
    #endif

    #define DRIVER_PATH                 "/dev/plx/"         // Path to drivers
    #define Driver_Disconnect           close

#elif defined(PLX_DOS)

    #define DRIVER_PATH                 ""                  // Path to drivers
    #define Driver_Disconnect           CloseHandle

    // Functions not supported in DOS
    VOID CloseHandle(HANDLE hDevice)
    {
        Dispatch_Cleanup( hDevice );
        hDevice = INVALID_HANDLE_VALUE;
    }

#endif


// List of PLX drivers to search for
static char *PlxDrivers[] = 
{
    "Plx8000_NT",
    "Plx8000_DMA",
    "Plx9050",
    "Plx9030",
    "Plx9080",
    "Plx9054",
    "Plx9056",
    "Plx9656",
    "Plx8311",
    "Plx6000_NT",
    "Plx8000_ND",
    PLX_SVC_DRIVER_NAME,    // PLX PCI service must be last driver
    "0"                     // Must be last item to mark end of list
};




/**********************************************
 *       Private Function Prototypes
 *********************************************/
static BOOLEAN
Driver_Connect(
    PLX_DRIVER_HANDLE *pHandle,
    U8                 ApiIndex,
    U16                DeviceNumber
    );

static S32
PlxIoMessage(
    PLX_DEVICE_OBJECT *pDevice,
    U32                IoControlCode,
    VOID              *pBuffer
    );




#if defined(PLX_MSWINDOWS)
/*****************************************************************************
 *
 * Function   :  DllMain
 *
 * Description:  DllMain is called by Windows each time a process or
 *               thread attaches or detaches to/from this DLL.
 *
 *****************************************************************************/
BOOLEAN WINAPI
DllMain(
    HANDLE hInst, 
    U32    ReasonForCall,
    LPVOID lpReserved
    )
{
    // Added to prevent compiler warning
    if (hInst == INVALID_HANDLE_VALUE)
    {
    }

    if (lpReserved == NULL)
    {
    }

    switch (ReasonForCall)
    {
        case DLL_PROCESS_ATTACH:
            DebugPrintf_Cont(("\n"));
            DebugPrintf(("<=======================================>\n"));
            DebugPrintf(("DllMain( DLL_PROCESS_ATTACH )\n"));
            DebugPrintf((
                "PLX API DLL v%d.%02d (%d-bit) - built %s %s\n",
                PLX_SDK_VERSION_MAJOR, PLX_SDK_VERSION_MINOR,
                (U32)(sizeof(PLX_UINT_PTR) * 8),
                __DATE__, __TIME__
                ));
            break;

        case DLL_PROCESS_DETACH:
            DebugPrintf(("DllMain( DLL_PROCESS_DETACH )\n"));
            DebugPrintf(("<=======================================>\n"));
            break;

        case DLL_THREAD_ATTACH:
            DebugPrintf(("DllMain( DLL_THREAD_ATTACH )\n"));
            break;

        case DLL_THREAD_DETACH:
            DebugPrintf(("DllMain( DLL_THREAD_DETACH )\n"));
            break;

        default:
            break;
    }

    DebugPrintf(("...Exit DllMain\n"));

    return TRUE;
}
#endif




/******************************************************************************
 *
 * Function   :  PlxPci_DeviceOpen
 *
 * Description:  Selects a device
 *
 *****************************************************************************/
PLX_STATUS
PlxPci_DeviceOpen(
    PLX_DEVICE_KEY    *pKey,
    PLX_DEVICE_OBJECT *pDevice
    )
{
    U8         VerMajor;
    U8         VerMinor;
    U8         VerRevision;
    PLX_STATUS rc;


    if ((pDevice == NULL) || (pKey == NULL))
        return ApiNullParam;

    // Make sure device object is not already in use
    if (IsObjectValid(pDevice))
        return ApiInvalidDeviceInfo;

    // Clear device object
    RtlZeroMemory( pDevice, sizeof(PLX_DEVICE_OBJECT) );

    // Mark object as invalid
    ObjectInvalidate( pDevice );

    // Copy key information
    pDevice->Key = *pKey;

    // Check for non-PCI mode
    if (IsObjectValid(pKey))
    {
        if (pKey->ApiMode == PLX_API_MODE_I2C_AARDVARK)
        {
            return PlxI2c_DeviceOpen( pDevice );
        }
    }
    else
    {
        // Default to PCI mode & fill in missing key information
        rc =
            PlxPci_DeviceFindEx(
                &(pDevice->Key),
                0,                      // First matching device,
                PLX_API_MODE_PCI,
                NULL                    // Mode properties ignored in PCI mode
                );

        if (rc != ApiSuccess)
        {
            return rc;
        }
    }

    // Connect to driver
    if (Driver_Connect(
            &pDevice->hDevice,
            pDevice->Key.ApiIndex,
            pDevice->Key.DeviceNumber
            ) == FALSE)
    {
        return ApiInvalidDeviceInfo;
    }

    // Mark object as valid
    ObjectValidate( pDevice );

    // Verify the driver version
    PlxPci_DriverVersion(
        pDevice,
        &VerMajor,
        &VerMinor,
        &VerRevision
        );

    // Update version until API updates
    VerMinor = (VerMinor * 10) + VerRevision;

    // Make sure the driver matches the DLL
    if ((VerMajor != PLX_SDK_VERSION_MAJOR) ||
        (VerMinor != PLX_SDK_VERSION_MINOR))
    {
        // Close the handle
        Driver_Disconnect( pDevice->hDevice );

        // Mark object as invalid
        ObjectInvalidate( pDevice );

        return ApiInvalidDriverVersion;
    }

    return ApiSuccess;
}




/******************************************************************************
 *
 * Function   :  PlxPci_DeviceClose
 *
 * Description:  Closes a previously opened device
 *
 *****************************************************************************/
PLX_STATUS
PlxPci_DeviceClose(
    PLX_DEVICE_OBJECT *pDevice
    )
{
    if (pDevice == NULL)
        return ApiNullParam;

    // Verify device object
    if (!IsObjectValid(pDevice))
        return ApiInvalidDeviceInfo;

    // Check for non-PCI mode
    if (pDevice->Key.ApiMode == PLX_API_MODE_I2C_AARDVARK)
    {
        PlxI2c_DeviceClose( pDevice );
    }
    else
    {
        // Close the handle
        Driver_Disconnect( pDevice->hDevice );
    }

    // Mark object as invalid
    ObjectInvalidate( pDevice );

    // Mark object key as invalid
    ObjectInvalidate( &(pDevice->Key) );

    return ApiSuccess;
}




/******************************************************************************
 *
 * Function   :  PlxPci_DeviceFind
 *
 * Description:  Locates a specific device
 *
 *****************************************************************************/
PLX_STATUS
PlxPci_DeviceFind(
    PLX_DEVICE_KEY *pKey,
    U16             DeviceNumber
    )
{
    U8                i;
    U16               TotalMatches;
    BOOLEAN           bConnect;
    BOOLEAN           bDriverOpened;
    PLX_PARAMS        IoBuffer;
    PLX_DEVICE_OBJECT Device;


    // Check for null pointers
    if (pKey == NULL)
    {
        return ApiNullParam;
    }

    i             = 0;
    TotalMatches  = 0;
    bDriverOpened = FALSE;

    RtlZeroMemory( &Device, sizeof(PLX_DEVICE_OBJECT) );

    // Legacy function only support PLX driver over PCI
    Device.Key.ApiMode = PLX_API_MODE_PCI;

    // Scan through present drivers for matches
    while (PlxDrivers[i][0] != '0')
    {
        // Connect to driver
        bConnect =
            Driver_Connect(
                &(Device.hDevice),
                i,                  // Driver index
                0                   // Device index in driver
                );

        if (bConnect)
        {
            //
            // Driver is open.  Find any devices that match
            //
            bDriverOpened = TRUE;

            RtlZeroMemory( &IoBuffer, sizeof(PLX_PARAMS) );

            // Copy search criteria
            IoBuffer.Key = *pKey;

            IoBuffer.value[0] = DeviceNumber - TotalMatches;

            PlxIoMessage(
                &Device,
                PLX_IOCTL_PCI_DEVICE_FIND,
                &IoBuffer
                );

            // Release driver connection
            Driver_Disconnect( Device.hDevice );

            // Return if specified device was found
            if (IoBuffer.ReturnCode == ApiSuccess)
            {
                // Copy device key information
                *pKey = IoBuffer.Key;

                // Store driver name index
                pKey->ApiIndex = (U8)i;

                // Validate key
                ObjectValidate( pKey );

                return ApiSuccess;
            }

            // Add number of matches to total
            TotalMatches += (U16)IoBuffer.value[0];
        }

        // Increment to next driver
        i++;
    }

    if (bDriverOpened == FALSE)
    {
        return ApiNoActiveDriver;
    }

    return ApiInvalidDeviceInfo;
}




/******************************************************************************
 *
 * Function   :  PlxPci_DeviceFindEx
 *
 * Description:  Extended function to locate a specific device via an API mode
 *
 *****************************************************************************/
PLX_STATUS
PlxPci_DeviceFindEx(
    PLX_DEVICE_KEY *pKey,
    U16             DeviceNumber,
    PLX_API_MODE    ApiMode,
    PLX_MODE_PROP  *pModeProp
    )
{
    // For access through PCI, revert to legacy function
    if (ApiMode == PLX_API_MODE_PCI)
    {
        return PlxPci_DeviceFind(
            pKey,
            DeviceNumber
            );
    }

    // Access through I2C Aardvark USB
    if (ApiMode == PLX_API_MODE_I2C_AARDVARK)
    {
        return PlxI2c_DeviceFindEx(
            pKey,
            DeviceNumber,
            pModeProp
            );
    }

    if (ApiMode == PLX_API_MODE_TCP)
    {
        // Not yet supported
        return ApiUnsupportedFunction;
    }

    return ApiInvalidAccessType;
}




/******************************************************************************
 *
 * Function   :  PlxPci_I2cGetPorts
 *
 * Description:  Returns status of active I2C ports
 *
 *****************************************************************************/
PLX_STATUS
PlxPci_I2cGetPorts(
    PLX_API_MODE  ApiMode,
    U32          *pI2cPorts
    )
{
    // Only Aardvark I2C currently supported
    if (ApiMode != PLX_API_MODE_I2C_AARDVARK)
        return ApiInvalidAccessType;

    return PlxI2c_I2cGetPorts(
        ApiMode,
        pI2cPorts
        );
}




/******************************************************************************
 *
 * Function   :  PlxPci_ApiVersion
 *
 * Description:  Return the API Library version number
 *
 *****************************************************************************/
PLX_STATUS
PlxPci_ApiVersion(
    U8 *pVersionMajor,
    U8 *pVersionMinor,
    U8 *pVersionRevision
    )
{
    if ((pVersionMajor    == NULL) ||
        (pVersionMinor    == NULL) ||
        (pVersionRevision == NULL))
    {
        return ApiNullParam;
    }

    *pVersionMajor    = PLX_SDK_VERSION_MAJOR;
    *pVersionMinor    = PLX_SDK_VERSION_MINOR / 10;
    *pVersionRevision = PLX_SDK_VERSION_MINOR - (*pVersionMinor * 10);

    return ApiSuccess;
}




/******************************************************************************
 *
 * Function   :  PlxPci_I2cVersion
 *
 * Description:  Returns I2C version information
 *
 *****************************************************************************/
PLX_STATUS
PlxPci_I2cVersion(
    U16          I2cPort,
    PLX_VERSION *pVersion
    )
{
    return PlxI2c_I2cVersion(
        I2cPort,
        pVersion
        );
}




/******************************************************************************
 *
 * Function   :  PlxPci_DriverVersion
 *
 * Description:  Return the driver version number
 *
 *****************************************************************************/
PLX_STATUS
PlxPci_DriverVersion(
    PLX_DEVICE_OBJECT *pDevice,
    U8                *pVersionMajor,
    U8                *pVersionMinor,
    U8                *pVersionRevision
    )
{
    PLX_PARAMS IoBuffer;


    if ((pVersionMajor    == NULL) ||
        (pVersionMinor    == NULL) ||
        (pVersionRevision == NULL))
    {
        return ApiNullParam;
    }

    // Clear version information in case of error
    *pVersionMajor    = 0;
    *pVersionMinor    = 0;
    *pVersionRevision = 0;

    // Verify device object
    if (!IsObjectValid(pDevice))
        return ApiInvalidDeviceInfo;

    RtlZeroMemory( &IoBuffer, sizeof(PLX_PARAMS) );

    PlxIoMessage(
        pDevice,
        PLX_IOCTL_DRIVER_VERSION,
        &IoBuffer
        );

    *pVersionMajor    = (U8)((IoBuffer.value[0] >> 16) & 0xFF);
    *pVersionMinor    = (U8)((IoBuffer.value[0] >>  8) & 0xFF) / 10;
    *pVersionRevision = (U8)((IoBuffer.value[0] >>  8) & 0xFF) - (*pVersionMinor * 10);

    return ApiSuccess;
}




/******************************************************************************
 *
 * Function   :  PlxPci_DriverProperties
 *
 * Description:  Returns driver properties
 *
 *****************************************************************************/
PLX_STATUS
PlxPci_DriverProperties(
    PLX_DEVICE_OBJECT *pDevice,
    PLX_DRIVER_PROP   *pDriverProp
    )
{
    PLX_PARAMS IoBuffer;


    if (pDriverProp == NULL)
        return ApiNullParam;

    // Verify device object
    if (!IsObjectValid(pDevice))
        return ApiInvalidDeviceInfo;

    RtlZeroMemory( &IoBuffer, sizeof(PLX_PARAMS) );

#if 0
// DBG - Driver message to get properties not supported yet
    PlxIoMessage(
        pDevice,
        PLX_IOCTL_DRIVER_PROPERTIES,
        &IoBuffer
        );

    if (IoBuffer.ReturnCode == ApiSuccess)
        *pDriverProp = IoBuffer.u.DriverProp;
#else
    // Bypass for now
    IoBuffer.ReturnCode = ApiSuccess;

    // Set driver version
    pDriverProp->Version =
        (PLX_SDK_VERSION_MAJOR << 16) |
        (PLX_SDK_VERSION_MINOR <<  8);

    // Default to PnP driver
    pDriverProp->bIsServiceDriver = FALSE;

    if (pDevice->Key.ApiMode == PLX_API_MODE_PCI)
    {
        // Copy driver name
        strcpy(
            pDriverProp->Name,
            PlxDrivers[pDevice->Key.ApiIndex]
            );

        // Build full name
        sprintf(
            pDriverProp->FullName,
            "%s_v%d%02d-%d",
            pDriverProp->Name,
            PLX_SDK_VERSION_MAJOR,
            PLX_SDK_VERSION_MINOR,
            pDevice->Key.DeviceNumber
            );

        // Determine if service driver
        if (strcmp(
                PlxDrivers[pDevice->Key.ApiIndex],
                PLX_SVC_DRIVER_NAME
                ) == 0)
        {
            pDriverProp->bIsServiceDriver = TRUE;
        }
    }
    else if (pDevice->Key.ApiMode == PLX_API_MODE_I2C_AARDVARK)
    {
        // Fill in properties for I2C
        pDriverProp->bIsServiceDriver = TRUE;

        strcpy(
            pDriverProp->Name,
            "PlxI2cAardvark"
            );
    }
#endif

    return IoBuffer.ReturnCode;
}




/******************************************************************************
 *
 * Function   :  PlxPci_DriverScheduleRescan
 *
 * Description:  Request the driver to schedule a rescan to rebuild its device list
 *
 *****************************************************************************/
PLX_STATUS
PlxPci_DriverScheduleRescan(
    PLX_DEVICE_OBJECT *pDevice
    )
{
    PLX_PARAMS IoBuffer;


    // Verify device object
    if (!IsObjectValid(pDevice))
        return ApiInvalidDeviceInfo;

    RtlZeroMemory( &IoBuffer, sizeof(PLX_PARAMS) );

    PlxIoMessage(
        pDevice,
        PLX_IOCTL_DRIVER_SCHEDULE_RESCAN,
        &IoBuffer
        );

    return IoBuffer.ReturnCode;
}




/******************************************************************************
 *
 * Function   :  PlxPci_ChipTypeGet
 *
 * Description:  Get the PLX Chip type and revision
 *
 *****************************************************************************/
PLX_STATUS
PlxPci_ChipTypeGet(
    PLX_DEVICE_OBJECT *pDevice,
    U16               *pChipType,
    U8                *pRevision
    )
{
    PLX_PARAMS IoBuffer;


    if ((pChipType == NULL) ||
        (pRevision == NULL))
    {
        return ApiNullParam;
    }

    // Verify device object
    if (!IsObjectValid(pDevice))
        return ApiInvalidDeviceInfo;

    RtlZeroMemory( &IoBuffer, sizeof(PLX_PARAMS) );

    IoBuffer.Key = pDevice->Key;

    PlxIoMessage(
        pDevice,
        PLX_IOCTL_CHIP_TYPE_GET,
        &IoBuffer
        );

    *pChipType = (U16)IoBuffer.value[0];
    *pRevision = (U8)IoBuffer.value[1];

    // Update device properties
    pDevice->Key.PlxChip     = *pChipType;
    pDevice->Key.PlxRevision = *pRevision;

    return ApiSuccess;
}




/******************************************************************************
 *
 * Function   :  PlxPci_ChipTypeSet
 *
 * Description:  Dynamically sets the PLX Chip type and revision
 *
 *****************************************************************************/
PLX_STATUS
PlxPci_ChipTypeSet(
    PLX_DEVICE_OBJECT *pDevice,
    U16                ChipType,
    U8                 Revision
    )
{
    PLX_PARAMS IoBuffer;


    // Verify device object
    if (!IsObjectValid(pDevice))
        return ApiInvalidDeviceInfo;

    RtlZeroMemory( &IoBuffer, sizeof(PLX_PARAMS) );

    IoBuffer.Key      = pDevice->Key;
    IoBuffer.value[0] = ChipType;
    IoBuffer.value[1] = Revision;

    PlxIoMessage(
        pDevice,
        PLX_IOCTL_CHIP_TYPE_SET,
        &IoBuffer
        );

    // If successful, update device properties
    if (IoBuffer.ReturnCode == ApiSuccess)
    {
        pDevice->Key.PlxChip     = ChipType;
        pDevice->Key.PlxRevision = (U8)IoBuffer.value[1];
        pDevice->Key.PlxFamily   = (U8)IoBuffer.value[2];
    }

    return IoBuffer.ReturnCode;
}




/******************************************************************************
 *
 * Function   :  PlxPci_ChipGetPortMask
 *
 * Description:  Returns the PLX chip port mask
 *
 *****************************************************************************/
PLX_STATUS
PlxPci_ChipGetPortMask(
    U16  PlxChip,
    U8   PlxRevision,
    U64 *pPortMask
    )
{
    if (pPortMask == NULL)
        return ApiNullParam;

    switch (PlxChip)
    {
        case 0x2380:
        case 0x3380:
        case 0x3382:
            *pPortMask  = 0x00000007;  // 0-2,PCEI2USB,USB
            *pPortMask |= ((U64)1 << PLX_FLAG_PORT_PCIE_TO_USB) |
                          ((U64)1 << PLX_FLAG_PORT_USB);
            break;

        case 0x8505:
            *pPortMask  = 0x0000001F;  // 0-4
            break;

        case 0x8509:
            *pPortMask  = 0x000000FF;  // 0-7
            break;

        case 0x8508:
        case 0x8512:
        case 0x8517:
        case 0x8518:
            *pPortMask  = 0x0000001F;  // 0-4,NT
            *pPortMask |= ((U64)1 << PLX_FLAG_PORT_NT_LINK_0);
            break;

        case 0x8516:
            *pPortMask  = 0x0000000F;  // 0-3,NT
            *pPortMask |= ((U64)1 << PLX_FLAG_PORT_NT_LINK_0);
            break;

        case 0x8524:
            *pPortMask  = 0x00000F03;  // 0,1,8-11,NT
            *pPortMask |= ((U64)1 << PLX_FLAG_PORT_NT_LINK_0);
            break;

        case 0x8525:
            *pPortMask  = 0x00000706;  // 1,2,8-10
            break;

        case 0x8532:
            *pPortMask  = 0x00000F0F;  // 0-3,8-11,NT
            *pPortMask |= ((U64)1 << PLX_FLAG_PORT_NT_LINK_0);
            break;

        case 0x8533:
            *pPortMask  = 0x00000707;  // 0-2,8-10
            break;

        case 0x8547:
            *pPortMask  = 0x00001101;  // 0,8,12
            break;

        case 0x8548:
            *pPortMask  = 0x00007707;  // 0-2,8-10,12-14
            break;

        case 0x8603:
            *pPortMask  = 0x00000007;  // 0-2
            break;

        case 0x8605:
            *pPortMask  = 0x0000000F;  // 0-3
            break;

        case 0x8604:
            *pPortMask  = 0x00000033;  // 0,1,4,5,NT,NTB(BA)
            *pPortMask |= ((U64)1 << PLX_FLAG_PORT_NT_LINK_0);
            if (PlxRevision != 0xAA)
                *pPortMask |= ((U64)1 << PLX_FLAG_PORT_NT_DS_P2P);
            break;

        case 0x8606:
            *pPortMask  = 0x000002B3;  // 0,1,4,5,7,9,NT,NTB(BA)
            *pPortMask |= ((U64)1 << PLX_FLAG_PORT_NT_LINK_0);
            if (PlxRevision != 0xAA)
                *pPortMask |= ((U64)1 << PLX_FLAG_PORT_NT_DS_P2P);
            break;

        case 0x8608:
            *pPortMask  = 0x000003F3;  // 0,1,4-9,NT,NTB(BA)
            *pPortMask |= ((U64)1 << PLX_FLAG_PORT_NT_LINK_0);
            if (PlxRevision != 0xAA)
                *pPortMask |= ((U64)1 << PLX_FLAG_PORT_NT_DS_P2P);
            break;

        case 0x8609:
            *pPortMask  = 0x000003F3;  // 0,1,4-9,DMA,NT,NTB
            *pPortMask |= ((U64)1 << PLX_FLAG_PORT_NT_LINK_0) |
                          ((U64)1 << PLX_FLAG_PORT_NT_DS_P2P) |
                          ((U64)1 << PLX_FLAG_PORT_DMA_0);
            break;

        case 0x8612:
            *pPortMask  = 0x00000033;  // 0,1,4,5,NT
            *pPortMask |= ((U64)1 << PLX_FLAG_PORT_NT_LINK_0);
            break;

        case 0x8613:
            *pPortMask  = 0x00000007;  // 0-2,NT,NTB
            *pPortMask |= ((U64)1 << PLX_FLAG_PORT_NT_LINK_0) |
                          ((U64)1 << PLX_FLAG_PORT_NT_DS_P2P);
            break;

        case 0x8614:
            *pPortMask  = 0x000057F7;  // 0-2,4-10,12,14,NT,NTB(BA)
            *pPortMask |= ((U64)1 << PLX_FLAG_PORT_NT_LINK_0);
            if (PlxRevision != 0xAA)
                *pPortMask |= ((U64)1 << PLX_FLAG_PORT_NT_DS_P2P);
            break;

        case 0x8615:
            *pPortMask  = 0x000057F7;  // 0-2,4-10,12,14,DMA,NT,NTB
            *pPortMask |= ((U64)1 << PLX_FLAG_PORT_NT_LINK_0) |
                          ((U64)1 << PLX_FLAG_PORT_NT_DS_P2P) |
                          ((U64)1 << PLX_FLAG_PORT_DMA_0);
            break;

        case 0x8616:
            *pPortMask  = 0x00000073;  // 0,1,4-6,NT
            *pPortMask |= ((U64)1 << PLX_FLAG_PORT_NT_LINK_0);
            break;

        case 0x8617:
            *pPortMask  = 0x0000000F;  // 0-3,NT,NTB
            *pPortMask |= ((U64)1 << PLX_FLAG_PORT_NT_LINK_0) |
                          ((U64)1 << PLX_FLAG_PORT_NT_DS_P2P);
            break;

        case 0x8618:
            *pPortMask  = 0x0000FFFF;  // 0-15,NT,NTB(BA)
            *pPortMask |= ((U64)1 << PLX_FLAG_PORT_NT_LINK_0);
            if (PlxRevision != 0xAA)
                *pPortMask |= ((U64)1 << PLX_FLAG_PORT_NT_DS_P2P);
            break;

        case 0x8619:
            *pPortMask  = 0x0000FFFF;  // 0-15,DMA,NT,NTB
            *pPortMask |= ((U64)1 << PLX_FLAG_PORT_NT_LINK_0) |
                          ((U64)1 << PLX_FLAG_PORT_NT_DS_P2P) |
                          ((U64)1 << PLX_FLAG_PORT_DMA_0);
            break;

        case 0x8624:
            *pPortMask  = 0x00000373;  // 0,1,4-6,8,9,NT
            *pPortMask |= ((U64)1 << PLX_FLAG_PORT_NT_LINK_0);
            break;

        case 0x8632:
            *pPortMask  = 0x00000FFF;  // 0-11
            break;

        case 0x8647:
            *pPortMask  = 0x00000111;  // 0,4,8
            break;

        case 0x8648:
            *pPortMask  = 0x00000FFF;  // 0-11,NT
            *pPortMask |= ((U64)1 << PLX_FLAG_PORT_NT_LINK_0);
            break;

        case 0x8649:
            *pPortMask  = 0x00FF000F;  // 0-3,16-23,NT,NTB
            *pPortMask |= ((U64)1 << PLX_FLAG_PORT_NT_LINK_0) |
                          ((U64)1 << PLX_FLAG_PORT_NT_VIRTUAL_0);
            break;

        case 0x8664:
            *pPortMask  = 0x00FF00FF;  // 0-7,16-23,NT,NTB
            *pPortMask |= ((U64)1 << PLX_FLAG_PORT_NT_LINK_0) |
                          ((U64)1 << PLX_FLAG_PORT_NT_DS_P2P);
            break;

        case 0x8680:
            *pPortMask  = 0x00FF0FFF;  // 0-11,16-23,NT,NTB
            *pPortMask |= ((U64)1 << PLX_FLAG_PORT_NT_LINK_0) |
                          ((U64)1 << PLX_FLAG_PORT_NT_VIRTUAL_0);
            break;

        case 0x8625:
        case 0x8636:
        case 0x8696:
            *pPortMask  = 0x00FFFFFF;  // 0-23,NT,NTB
            *pPortMask |= ((U64)1 << PLX_FLAG_PORT_NT_LINK_0) |
                          ((U64)1 << PLX_FLAG_PORT_NT_VIRTUAL_0);
            break;

        case 0x8700:
            *pPortMask  = 0x0000000F;  // 0-3
            *pPortMask |= ((U64)1 << PLX_FLAG_PORT_NT_LINK_0) |
                          ((U64)1 << PLX_FLAG_PORT_NT_VIRTUAL_0);
            break;

        case 0x8712:
            *pPortMask  = 0x00003F3F;  // 0-5,8-13,NT
            *pPortMask |= ((U64)1 << PLX_FLAG_PORT_NT_LINK_0) |
                          ((U64)1 << PLX_FLAG_PORT_NT_VIRTUAL_0);
            break;

        case 0x8713:
            *pPortMask  = 0x00003F3F;  // 0-5,8-13,NT 0/1,DMA 0-4
            *pPortMask |= ((U64)1 << PLX_FLAG_PORT_NT_LINK_0) |
                          ((U64)1 << PLX_FLAG_PORT_NT_LINK_1) |
                          ((U64)1 << PLX_FLAG_PORT_NT_VIRTUAL_0) |
                          ((U64)1 << PLX_FLAG_PORT_NT_VIRTUAL_1) |
                          ((U64)1 << PLX_FLAG_PORT_DMA_0) |
                          ((U64)1 << PLX_FLAG_PORT_DMA_1) |
                          ((U64)1 << PLX_FLAG_PORT_DMA_2) |
                          ((U64)1 << PLX_FLAG_PORT_DMA_3) |
                          ((U64)1 << PLX_FLAG_PORT_ALUT_0) |
                          ((U64)1 << PLX_FLAG_PORT_ALUT_1) |
                          ((U64)1 << PLX_FLAG_PORT_ALUT_2) |
                          ((U64)1 << PLX_FLAG_PORT_ALUT_3);
            break;

        case 0x8716:
            *pPortMask  = 0x0000000F;  // 0-3,NT
            *pPortMask |= ((U64)1 << PLX_FLAG_PORT_NT_LINK_0) |
                          ((U64)1 << PLX_FLAG_PORT_NT_VIRTUAL_0);
            break;

        case 0x8717:
            *pPortMask  = 0x00003F3F;  // 0-5,8-13,NT 0/1,DMA 0-4
            *pPortMask |= ((U64)1 << PLX_FLAG_PORT_NT_LINK_0) |
                          ((U64)1 << PLX_FLAG_PORT_NT_LINK_1) |
                          ((U64)1 << PLX_FLAG_PORT_NT_VIRTUAL_0) |
                          ((U64)1 << PLX_FLAG_PORT_NT_VIRTUAL_1) |
                          ((U64)1 << PLX_FLAG_PORT_DMA_0) |
                          ((U64)1 << PLX_FLAG_PORT_DMA_1) |
                          ((U64)1 << PLX_FLAG_PORT_DMA_2) |
                          ((U64)1 << PLX_FLAG_PORT_DMA_3) |
                          ((U64)1 << PLX_FLAG_PORT_ALUT_0) |
                          ((U64)1 << PLX_FLAG_PORT_ALUT_1) |
                          ((U64)1 << PLX_FLAG_PORT_ALUT_2) |
                          ((U64)1 << PLX_FLAG_PORT_ALUT_3);
            break;

        case 0x8714:
        case 0x8718:
            *pPortMask  = 0x0000001F;  // 0-4,NT 0,ALUT 0-3
            *pPortMask |= ((U64)1 << PLX_FLAG_PORT_NT_LINK_0) |
                          ((U64)1 << PLX_FLAG_PORT_NT_VIRTUAL_0) |
                          ((U64)1 << PLX_FLAG_PORT_ALUT_0) |
                          ((U64)1 << PLX_FLAG_PORT_ALUT_1) |
                          ((U64)1 << PLX_FLAG_PORT_ALUT_2) |
                          ((U64)1 << PLX_FLAG_PORT_ALUT_3);
            break;

        case 0x8723:
        case 0x8724:
            *pPortMask  = 0x0000070F;  // 0-3,8-10,NT
            *pPortMask |= ((U64)1 << PLX_FLAG_PORT_NT_LINK_0) |
                          ((U64)1 << PLX_FLAG_PORT_NT_VIRTUAL_0);
            break;

        case 0x8725:
            *pPortMask  = 0x00003F3F;  // 0-5,8-13,NT 0/1,DMA 0-4
            *pPortMask |= ((U64)1 << PLX_FLAG_PORT_NT_LINK_0) |
                          ((U64)1 << PLX_FLAG_PORT_NT_LINK_1) |
                          ((U64)1 << PLX_FLAG_PORT_NT_VIRTUAL_0) |
                          ((U64)1 << PLX_FLAG_PORT_NT_VIRTUAL_1) |
                          ((U64)1 << PLX_FLAG_PORT_DMA_0) |
                          ((U64)1 << PLX_FLAG_PORT_DMA_1) |
                          ((U64)1 << PLX_FLAG_PORT_DMA_2) |
                          ((U64)1 << PLX_FLAG_PORT_DMA_3) |
                          ((U64)1 << PLX_FLAG_PORT_ALUT_0) |
                          ((U64)1 << PLX_FLAG_PORT_ALUT_1) |
                          ((U64)1 << PLX_FLAG_PORT_ALUT_2) |
                          ((U64)1 << PLX_FLAG_PORT_ALUT_3);
            break;

        case 0x8732:
            *pPortMask  = 0x00000F0F;  // 0-3,8-11,NT
            *pPortMask |= ((U64)1 << PLX_FLAG_PORT_NT_LINK_0) |
                          ((U64)1 << PLX_FLAG_PORT_NT_VIRTUAL_0);
            break;

        case 0x8733:
            *pPortMask  = 0x003F3F3F;  // 0-5,8-13,16-21,NT 0/1,DMA 0-4
            *pPortMask |= ((U64)1 << PLX_FLAG_PORT_NT_LINK_0) |
                          ((U64)1 << PLX_FLAG_PORT_NT_LINK_1) |
                          ((U64)1 << PLX_FLAG_PORT_NT_VIRTUAL_0) |
                          ((U64)1 << PLX_FLAG_PORT_NT_VIRTUAL_1) |
                          ((U64)1 << PLX_FLAG_PORT_DMA_0) |
                          ((U64)1 << PLX_FLAG_PORT_DMA_1) |
                          ((U64)1 << PLX_FLAG_PORT_DMA_2) |
                          ((U64)1 << PLX_FLAG_PORT_DMA_3) |
                          ((U64)1 << PLX_FLAG_PORT_ALUT_0) |
                          ((U64)1 << PLX_FLAG_PORT_ALUT_1) |
                          ((U64)1 << PLX_FLAG_PORT_ALUT_2) |
                          ((U64)1 << PLX_FLAG_PORT_ALUT_3);
            break;

        case 0x8734:
            *pPortMask  = 0x000000FF;  // 0-7,NT 0/1,ALUT 0-3,VS_S0-1
            *pPortMask |= ((U64)1 << PLX_FLAG_PORT_NT_LINK_0) |
                          ((U64)1 << PLX_FLAG_PORT_NT_LINK_1) |
                          ((U64)1 << PLX_FLAG_PORT_NT_VIRTUAL_0) |
                          ((U64)1 << PLX_FLAG_PORT_NT_VIRTUAL_1) |
                          ((U64)1 << PLX_FLAG_PORT_ALUT_0) |
                          ((U64)1 << PLX_FLAG_PORT_ALUT_1) |
                          ((U64)1 << PLX_FLAG_PORT_ALUT_2) |
                          ((U64)1 << PLX_FLAG_PORT_ALUT_3);
            break;

        case 0x8747:
            *pPortMask  = 0x00030303;  // 0,1,8,9,16,17
            break;

        case 0x8748:
            *pPortMask  = 0x000F0F0F;  // 0-3,8-11,16-19,NT
            *pPortMask |= ((U64)1 << PLX_FLAG_PORT_NT_LINK_0) |
                          ((U64)1 << PLX_FLAG_PORT_NT_VIRTUAL_0);
            break;

        case 0x8749:
            *pPortMask  = 0x003F3F3F;  // 0-5,8-13,16-21,NT 0/1,DMA 0-4
            *pPortMask |= ((U64)1 << PLX_FLAG_PORT_NT_LINK_0) |
                          ((U64)1 << PLX_FLAG_PORT_NT_LINK_1) |
                          ((U64)1 << PLX_FLAG_PORT_NT_VIRTUAL_0) |
                          ((U64)1 << PLX_FLAG_PORT_NT_VIRTUAL_1) |
                          ((U64)1 << PLX_FLAG_PORT_DMA_0) |
                          ((U64)1 << PLX_FLAG_PORT_DMA_1) |
                          ((U64)1 << PLX_FLAG_PORT_DMA_2) |
                          ((U64)1 << PLX_FLAG_PORT_DMA_3) |
                          ((U64)1 << PLX_FLAG_PORT_ALUT_0) |
                          ((U64)1 << PLX_FLAG_PORT_ALUT_1) |
                          ((U64)1 << PLX_FLAG_PORT_ALUT_2) |
                          ((U64)1 << PLX_FLAG_PORT_ALUT_3);
            break;

        case 0x8750:
            *pPortMask  = 0x00000FFF;  // 0-11,NT 0/1,ALUT 0-3,VS_S0-2
            *pPortMask |= ((U64)1 << PLX_FLAG_PORT_NT_LINK_0) |
                          ((U64)1 << PLX_FLAG_PORT_NT_LINK_1) |
                          ((U64)1 << PLX_FLAG_PORT_NT_VIRTUAL_0) |
                          ((U64)1 << PLX_FLAG_PORT_NT_VIRTUAL_1) |
                          ((U64)1 << PLX_FLAG_PORT_ALUT_0) |
                          ((U64)1 << PLX_FLAG_PORT_ALUT_1) |
                          ((U64)1 << PLX_FLAG_PORT_ALUT_2) |
                          ((U64)1 << PLX_FLAG_PORT_ALUT_3);
            break;

        case 0x8764:
            *pPortMask  = 0x0000FFFF;  // 0-15,NT 0/1,ALUT 0-3,VS_S0-3
            *pPortMask |= ((U64)1 << PLX_FLAG_PORT_NT_LINK_0) |
                          ((U64)1 << PLX_FLAG_PORT_NT_LINK_1) |
                          ((U64)1 << PLX_FLAG_PORT_NT_VIRTUAL_0) |
                          ((U64)1 << PLX_FLAG_PORT_NT_VIRTUAL_1) |
                          ((U64)1 << PLX_FLAG_PORT_ALUT_0) |
                          ((U64)1 << PLX_FLAG_PORT_ALUT_1) |
                          ((U64)1 << PLX_FLAG_PORT_ALUT_2) |
                          ((U64)1 << PLX_FLAG_PORT_ALUT_3);
            break;

        case 0x8780:
            *pPortMask  = 0x000FFFFF;  // 0-19,NT 0/1,ALUT 0-3,VS_S0-4
            *pPortMask |= ((U64)1 << PLX_FLAG_PORT_NT_LINK_0) |
                          ((U64)1 << PLX_FLAG_PORT_NT_LINK_1) |
                          ((U64)1 << PLX_FLAG_PORT_NT_VIRTUAL_0) |
                          ((U64)1 << PLX_FLAG_PORT_NT_VIRTUAL_1) |
                          ((U64)1 << PLX_FLAG_PORT_ALUT_0) |
                          ((U64)1 << PLX_FLAG_PORT_ALUT_1) |
                          ((U64)1 << PLX_FLAG_PORT_ALUT_2) |
                          ((U64)1 << PLX_FLAG_PORT_ALUT_3);
            break;

        case 0x8796:
            *pPortMask  = 0x00FFFFFF;  // 0-23,NT 0/1,ALUT 0-3,VS_S0-5
            *pPortMask |= ((U64)1 << PLX_FLAG_PORT_NT_LINK_0) |
                          ((U64)1 << PLX_FLAG_PORT_NT_LINK_1) |
                          ((U64)1 << PLX_FLAG_PORT_NT_VIRTUAL_0) |
                          ((U64)1 << PLX_FLAG_PORT_NT_VIRTUAL_1) |
                          ((U64)1 << PLX_FLAG_PORT_ALUT_0) |
                          ((U64)1 << PLX_FLAG_PORT_ALUT_1) |
                          ((U64)1 << PLX_FLAG_PORT_ALUT_2) |
                          ((U64)1 << PLX_FLAG_PORT_ALUT_3);
            break;

        case 0x8715:
        case 0x8719:
        case 0x8735:
        case 0x8751:
        case 0x8765:
        case 0x8781:
        case 0x8797:
            *pPortMask  = 0x0000000F;
            break;

        default:
            // For unsupported chips, set default
            *pPortMask = 0x00000001;  // 0
            return ApiUnsupportedFunction;
    }

    return ApiSuccess;
}




/******************************************************************************
 *
 * Function   :  PlxPci_GetPortProperties
 *
 * Description:  Returns the current port information and status
 *
 *****************************************************************************/
PLX_STATUS
PlxPci_GetPortProperties(
    PLX_DEVICE_OBJECT *pDevice,
    PLX_PORT_PROP     *pPortProp
    )
{
    PLX_PARAMS IoBuffer;


    if (pPortProp == NULL)
    {
        return ApiNullParam;
    }

    // Verify device object
    if (!IsObjectValid(pDevice))
        return ApiInvalidDeviceInfo;

    RtlZeroMemory( &IoBuffer, sizeof(PLX_PARAMS) );

    IoBuffer.Key = pDevice->Key;

    PlxIoMessage(
        pDevice,
        PLX_IOCTL_GET_PORT_PROPERTIES,
        &IoBuffer
        );

    if (IoBuffer.ReturnCode == ApiSuccess)
    {
        // Return port information
        *pPortProp = IoBuffer.u.PortProp;
    }
    else
    {
        // Set default value for properties
        RtlZeroMemory( pPortProp, sizeof(PLX_PORT_PROP) );
        pPortProp->PortType = PLX_PORT_UNKNOWN;
    }

    return IoBuffer.ReturnCode;
}




/******************************************************************************
 *
 * Function   :  PlxPci_DeviceReset
 *
 * Description:  Resets a PCI board using the software reset feature
 *
 *****************************************************************************/
PLX_STATUS
PlxPci_DeviceReset(
    PLX_DEVICE_OBJECT *pDevice
    )
{
    PLX_PARAMS IoBuffer;


    // Verify device object
    if (!IsObjectValid(pDevice))
        return ApiInvalidDeviceInfo;

    RtlZeroMemory( &IoBuffer, sizeof(PLX_PARAMS) );

    IoBuffer.Key = pDevice->Key;

    PlxIoMessage(
        pDevice,
        PLX_IOCTL_PCI_DEVICE_RESET,
        &IoBuffer
        );

    return IoBuffer.ReturnCode;
}




/******************************************************************************
 *
 * Function   :  PlxPci_PciRegisterRead
 *
 * Description:  Reads a PCI configuration register at the specified offset
 *
 *****************************************************************************/
U32
PlxPci_PciRegisterRead(
    U8          bus,
    U8          slot,
    U8          function,
    U16         offset,
    PLX_STATUS *pStatus
    )
{
    return PlxPci_PciRegisterRead_BypassOS(
               bus,
               slot,
               function,
               offset,
               pStatus
               );
}




/******************************************************************************
 *
 * Function   :  PlxPci_PciRegisterWrite
 *
 * Description:  Writes to a PCI configuration register at the specified offset
 *
 *****************************************************************************/
PLX_STATUS
PlxPci_PciRegisterWrite(
    U8  bus,
    U8  slot,
    U8  function,
    U16 offset,
    U32 value
    )
{
    return PlxPci_PciRegisterWrite_BypassOS(
               bus,
               slot,
               function,
               offset,
               value
               );
}




/******************************************************************************
 *
 * Function   :  PlxPci_PciRegisterReadFast
 *
 * Description:  Reads a PCI configuration register only from an open device
 *
 *****************************************************************************/
U32
PlxPci_PciRegisterReadFast(
    PLX_DEVICE_OBJECT *pDevice,
    U16                offset,
    PLX_STATUS        *pStatus
    )
{
    PLX_PARAMS IoBuffer;


    // Verify device object
    if (!IsObjectValid(pDevice))
    {
        if (pStatus != NULL)
            *pStatus = ApiInvalidDeviceInfo;
        return (U32)-1;
    }

    RtlZeroMemory( &IoBuffer, sizeof(PLX_PARAMS) );

    IoBuffer.Key      = pDevice->Key;
    IoBuffer.value[0] = offset;
    IoBuffer.value[1] = (U32)-1;

    PlxIoMessage(
        pDevice,
        PLX_IOCTL_PCI_REGISTER_READ,
        &IoBuffer
        );

    if (pStatus != NULL)
        *pStatus = IoBuffer.ReturnCode;

    return (U32)IoBuffer.value[1];
}




/******************************************************************************
 *
 * Function   :  PlxPci_PciRegisterWriteFast
 *
 * Description:  Writes a PCI configuration register only on the selected device
 *
 *****************************************************************************/
PLX_STATUS
PlxPci_PciRegisterWriteFast(
    PLX_DEVICE_OBJECT *pDevice,
    U16                offset,
    U32                value
    )
{
    PLX_PARAMS IoBuffer;


    // Verify device object
    if (!IsObjectValid(pDevice))
        return ApiInvalidDeviceInfo;

    RtlZeroMemory( &IoBuffer, sizeof(PLX_PARAMS) );

    IoBuffer.Key      = pDevice->Key;
    IoBuffer.value[0] = offset;
    IoBuffer.value[1] = value;

    PlxIoMessage(
        pDevice,
        PLX_IOCTL_PCI_REGISTER_WRITE,
        &IoBuffer
        );

    return IoBuffer.ReturnCode;
}




/******************************************************************************
 *
 * Function   :  PlxPci_PciRegisterRead_BypassOS
 *
 * Description:  Reads a PCI register by bypassing the OS
 *
 *****************************************************************************/
U32
PlxPci_PciRegisterRead_BypassOS(
    U8          bus,
    U8          slot,
    U8          function,
    U16         offset,
    PLX_STATUS *pStatus
    )
{
    PLX_PARAMS        IoBuffer;
    PLX_STATUS        rc;
    PLX_DEVICE_OBJECT Device;


    // Setup to select any device
    memset( &IoBuffer.Key, PCI_FIELD_IGNORE, sizeof(PLX_DEVICE_KEY) );
    RtlZeroMemory( &Device, sizeof(PLX_DEVICE_OBJECT) );

    rc =
        PlxPci_DeviceOpen(
            &IoBuffer.Key,
            &Device
            );

    if (rc != ApiSuccess)
    {
        if (pStatus != NULL)
            *pStatus = rc;

        return (U32)-1;
    }

    RtlZeroMemory( &IoBuffer, sizeof(PLX_PARAMS) );

    IoBuffer.Key.bus      = bus;
    IoBuffer.Key.slot     = slot;
    IoBuffer.Key.function = function;
    IoBuffer.value[0]     = offset;
    IoBuffer.value[1]     = (U32)-1;

    PlxIoMessage(
        &Device,
        PLX_IOCTL_PCI_REG_READ_BYPASS_OS,
        &IoBuffer
        );

    if (pStatus != NULL)
        *pStatus = IoBuffer.ReturnCode;

    PlxPci_DeviceClose(
        &Device
        );

    return (U32)IoBuffer.value[1];
}




/******************************************************************************
 *
 * Function   :  PlxPci_PciRegisterWrite_BypassOS
 *
 * Description:  Writes to a PCI register by bypassing the OS
 *
 *****************************************************************************/
PLX_STATUS
PlxPci_PciRegisterWrite_BypassOS(
    U8  bus,
    U8  slot,
    U8  function,
    U16 offset,
    U32 value
    )
{
    PLX_PARAMS        IoBuffer;
    PLX_STATUS        rc;
    PLX_DEVICE_OBJECT Device;


    // Setup to select any device
    memset( &IoBuffer.Key, PCI_FIELD_IGNORE, sizeof(PLX_DEVICE_KEY) );
    RtlZeroMemory( &Device, sizeof(PLX_DEVICE_OBJECT) );

    rc =
        PlxPci_DeviceOpen(
            &IoBuffer.Key,
            &Device
            );

    if (rc != ApiSuccess)
        return rc;

    RtlZeroMemory( &IoBuffer, sizeof(PLX_PARAMS) );

    IoBuffer.Key.bus      = bus;
    IoBuffer.Key.slot     = slot;
    IoBuffer.Key.function = function;
    IoBuffer.value[0]     = offset;
    IoBuffer.value[1]     = value;

    PlxIoMessage(
        &Device,
        PLX_IOCTL_PCI_REG_WRITE_BYPASS_OS,
        &IoBuffer
        );

    PlxPci_DeviceClose(
        &Device
        );

    return IoBuffer.ReturnCode;
}




/******************************************************************************
 *
 * Function   :  PlxPci_PlxRegisterRead
 *
 * Description:  Reads the PLX-specific register from a specified offset
 *
 *****************************************************************************/
U32
PlxPci_PlxRegisterRead(
    PLX_DEVICE_OBJECT *pDevice,
    U32                offset,
    PLX_STATUS        *pStatus
    )
{
    PLX_PARAMS IoBuffer;


    // Verify device object
    if (!IsObjectValid(pDevice))
    {
        if (pStatus != NULL)
            *pStatus = ApiInvalidDeviceInfo;
        return (U32)-1;
    }

    RtlZeroMemory( &IoBuffer, sizeof(PLX_PARAMS) );

    IoBuffer.Key      = pDevice->Key;
    IoBuffer.value[0] = offset;

    PlxIoMessage(
        pDevice,
        PLX_IOCTL_REGISTER_READ,
        &IoBuffer
        );

    if (pStatus != NULL)
        *pStatus = IoBuffer.ReturnCode;

    return (U32)IoBuffer.value[1];
}




/******************************************************************************
 *
 * Function   :  PlxPci_PlxRegisterWrite
 *
 * Description:  Writes to a PLX-specific register at a specified offset
 *
 *****************************************************************************/
PLX_STATUS
PlxPci_PlxRegisterWrite(
    PLX_DEVICE_OBJECT *pDevice,
    U32                offset,
    U32                value
    )
{
    PLX_PARAMS IoBuffer;


    // Verify device object
    if (!IsObjectValid(pDevice))
        return ApiInvalidDeviceInfo;

    RtlZeroMemory( &IoBuffer, sizeof(PLX_PARAMS) );

    IoBuffer.Key      = pDevice->Key;
    IoBuffer.value[0] = offset;
    IoBuffer.value[1] = value;

    PlxIoMessage(
        pDevice,
        PLX_IOCTL_REGISTER_WRITE,
        &IoBuffer
        );

    return IoBuffer.ReturnCode;
}




/******************************************************************************
 *
 * Function   :  PlxPci_PlxMappedRegisterRead
 *
 * Description:  Reads a PLX-specific register mapped from upstream port BAR 0
 *
 *****************************************************************************/
U32
PlxPci_PlxMappedRegisterRead(
    PLX_DEVICE_OBJECT *pDevice,
    U32                offset,
    PLX_STATUS        *pStatus
    )
{
    PLX_PARAMS IoBuffer;


    // Verify device object
    if (!IsObjectValid(pDevice))
        return ApiInvalidDeviceInfo;

    RtlZeroMemory( &IoBuffer, sizeof(PLX_PARAMS) );

    IoBuffer.Key      = pDevice->Key;
    IoBuffer.value[0] = offset;

    PlxIoMessage(
        pDevice,
        PLX_IOCTL_MAPPED_REGISTER_READ,
        &IoBuffer
        );

    if (pStatus != NULL)
        *pStatus = IoBuffer.ReturnCode;

    return (U32)IoBuffer.value[1];
}




/******************************************************************************
 *
 * Function   :  PlxPci_PlxMappedRegisterWrite
 *
 * Description:  Writes to a PLX-specific register mapped from upstream port BAR 0
 *
 *****************************************************************************/
PLX_STATUS
PlxPci_PlxMappedRegisterWrite(
    PLX_DEVICE_OBJECT *pDevice,
    U32                offset,
    U32                value
    )
{
    PLX_PARAMS IoBuffer;


    // Verify device object
    if (!IsObjectValid(pDevice))
        return ApiInvalidDeviceInfo;

    RtlZeroMemory( &IoBuffer, sizeof(PLX_PARAMS) );

    IoBuffer.Key      = pDevice->Key;
    IoBuffer.value[0] = offset;
    IoBuffer.value[1] = value;

    PlxIoMessage(
        pDevice,
        PLX_IOCTL_MAPPED_REGISTER_WRITE,
        &IoBuffer
        );

    return IoBuffer.ReturnCode;
}




/******************************************************************************
 *
 * Function   :  PlxPci_PlxMailboxRead
 *
 * Description:  Reads a PLX mailbox register
 *
 *****************************************************************************/
U32
PlxPci_PlxMailboxRead(
    PLX_DEVICE_OBJECT *pDevice,
    U16                mailbox,
    PLX_STATUS        *pStatus
    )
{
    PLX_PARAMS IoBuffer;


    // Verify device object
    if (!IsObjectValid(pDevice))
    {
        if (pStatus != NULL)
            *pStatus = ApiInvalidDeviceInfo;
        return (U32)-1;
    }

    RtlZeroMemory( &IoBuffer, sizeof(PLX_PARAMS) );

    IoBuffer.Key      = pDevice->Key;
    IoBuffer.value[0] = mailbox;

    PlxIoMessage(
        pDevice,
        PLX_IOCTL_MAILBOX_READ,
        &IoBuffer
        );

    if (pStatus != NULL)
        *pStatus = IoBuffer.ReturnCode;

    return (U32)IoBuffer.value[1];
}




/******************************************************************************
 *
 * Function   :  PlxPci_PlxMailboxWrite
 *
 * Description:  Writes to a PLX mailbox register
 *
 *****************************************************************************/
PLX_STATUS
PlxPci_PlxMailboxWrite(
    PLX_DEVICE_OBJECT *pDevice,
    U16                mailbox,
    U32                value
    )
{
    PLX_PARAMS IoBuffer;


    // Verify device object
    if (!IsObjectValid(pDevice))
        return ApiInvalidDeviceInfo;

    RtlZeroMemory( &IoBuffer, sizeof(PLX_PARAMS) );

    IoBuffer.Key      = pDevice->Key;
    IoBuffer.value[0] = mailbox;
    IoBuffer.value[1] = value;

    PlxIoMessage(
        pDevice,
        PLX_IOCTL_MAILBOX_WRITE,
        &IoBuffer
        );

    return IoBuffer.ReturnCode;
}




/******************************************************************************
 *
 * Function   :  PlxPci_PciBarProperties
 *
 * Description:  Returns the properties of a PCI BAR space
 *
 *****************************************************************************/
PLX_STATUS
PlxPci_PciBarProperties(
    PLX_DEVICE_OBJECT *pDevice,
    U8                 BarIndex,
    PLX_PCI_BAR_PROP  *pBarProp
    )
{
    PLX_PARAMS IoBuffer;


    if (pBarProp == NULL)
        return ApiNullParam;

    // Set default value
    RtlZeroMemory( pBarProp, sizeof(PLX_PCI_BAR_PROP) );

    // Verify device object
    if (!IsObjectValid(pDevice))
        return ApiInvalidDeviceInfo;

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
            return ApiInvalidIndex;
    }

    RtlZeroMemory( &IoBuffer, sizeof(PLX_PARAMS) );

    IoBuffer.Key      = pDevice->Key;
    IoBuffer.value[0] = BarIndex;

    PlxIoMessage(
        pDevice,
        PLX_IOCTL_PCI_BAR_PROPERTIES,
        &IoBuffer
        );

    if (IoBuffer.ReturnCode == ApiSuccess)
    {
        // Copy properties
        *pBarProp = IoBuffer.u.BarProp;
    }

    return IoBuffer.ReturnCode;
}




/******************************************************************************
 *
 * Function   :  PlxPci_PciBarMap
 *
 * Description:  Maps a PCI BAR space into user virtual space.
 *
 *****************************************************************************/
PLX_STATUS
PlxPci_PciBarMap(
    PLX_DEVICE_OBJECT  *pDevice,
    U8                  BarIndex,
    VOID              **pVa
    )
{
    U32              BarOffset;
    PLX_PARAMS       IoBuffer;
    PLX_STATUS       rc;
    PLX_PCI_BAR_PROP BarProp;


    if (pVa == NULL)
        return ApiNullParam;

    // Set default value
    *(PLX_UINT_PTR*)pVa = 0;

    // Verify device object
    if (!IsObjectValid(pDevice))
        return ApiInvalidDeviceInfo;

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
            return ApiInvalidIndex;
    }

    // Check if mapping has already been performed
    if (pDevice->PciBarVa[BarIndex] != 0)
    {
        *(PLX_UINT_PTR*)pVa = (PLX_UINT_PTR)pDevice->PciBarVa[BarIndex];

        // Increment map count
        pDevice->BarMapRef[BarIndex]++;

        return ApiSuccess;
    }

    // Get the PCI BAR properties
    rc =
        PlxPci_PciBarProperties(
            pDevice,
            BarIndex,
            &BarProp
            );

    if (rc != ApiSuccess)
        return rc;

    // Verify BAR exists and is memory type
    if ((BarProp.Physical == 0) || (BarProp.Size == 0) || (BarProp.Flags & PLX_BAR_FLAG_IO))
    {
        return ApiInvalidPciSpace;
    }

    // Calculate starting offset from page boundary
    BarOffset = BarProp.Physical & ~PAGE_MASK;

    // For service driver, need to send additional data before mmap
    if (strcmp(
            PlxDrivers[pDevice->Key.ApiIndex],
            PLX_SVC_DRIVER_NAME
            ) == 0)
    {
        RtlZeroMemory( &IoBuffer, sizeof(PLX_PARAMS) );

        IoBuffer.Key      = pDevice->Key;
        IoBuffer.value[0] = BarIndex;      // Specify BAR to map

        PlxIoMessage(
            pDevice,
            PLX_IOCTL_PCI_BAR_MAP,
            &IoBuffer
            );

        if (IoBuffer.ReturnCode != ApiSuccess)
        {
            pDevice->PciBarVa[BarIndex] = 0;
            return IoBuffer.ReturnCode;
        }
    }

    // Get a valid virtual address
    pDevice->PciBarVa[BarIndex] =
        (PLX_UINT_PTR)mmap(
            0,
            PAGE_ALIGN((PLX_UINT_PTR)BarProp.Size),    // Round size to next page boundary
            PROT_READ | PROT_WRITE,
            MAP_SHARED,
            pDevice->hDevice,
            BarIndex * getpagesize()
            );

    if (pDevice->PciBarVa[BarIndex] == (PLX_UINT_PTR)MAP_FAILED)
    {
        pDevice->PciBarVa[BarIndex] = 0;
        return ApiInsufficientResources;
    }

    // Store BAR properties
    pDevice->PciBar[BarIndex] = BarProp;

    // Add the offset if any
    pDevice->PciBarVa[BarIndex] += BarOffset;

    // Provide virtual address
    *(PLX_UINT_PTR*)pVa = pDevice->PciBarVa[BarIndex];

    // Set map count
    pDevice->BarMapRef[BarIndex] = 1;

    return ApiSuccess;
}




/******************************************************************************
 *
 * Function   :  PlxPci_PciBarUnmap
 *
 * Description:  Unmaps a PCI BAR space from user virtual space
 *
 *****************************************************************************/
PLX_STATUS
PlxPci_PciBarUnmap(
    PLX_DEVICE_OBJECT  *pDevice,
    VOID              **pVa
    )
{
    int          rc;
    U8           BarIndex;
    PLX_UINT_PTR BarVa;


    if (pVa == NULL)
        return ApiNullParam;

    if (*(PLX_UINT_PTR*)pVa == 0)
        return ApiInvalidAddress;

    // Verify device object
    if (!IsObjectValid(pDevice))
        return ApiInvalidDeviceInfo;

    // Search for actual mapped address
    for (BarIndex = 0; BarIndex < PCI_NUM_BARS_TYPE_00; BarIndex++)
    {
        BarVa = (PLX_UINT_PTR)pDevice->PciBarVa[BarIndex];

        // Compare virtual address
        if (*(PLX_UINT_PTR*)pVa == BarVa)
        {
            // Decrement map count
            pDevice->BarMapRef[BarIndex]--;

            // Unmap the space if no longer referenced
            if (pDevice->BarMapRef[BarIndex] == 0)
            {
                // Remove offset
                BarVa = BarVa & PAGE_MASK;

                // Unmap the space
                rc =
                    munmap(
                        (VOID*)BarVa,
                        PAGE_ALIGN((PLX_UINT_PTR)pDevice->PciBar[BarIndex].Size)
                        );

                if (rc != 0)
                {
                    return ApiInvalidAddress;
                }

                // Clear internal data
                pDevice->PciBarVa[BarIndex] = 0;
            }

            // Clear address
            *(PLX_UINT_PTR*)pVa = 0;

            return ApiSuccess;
        }
    }

    return ApiInvalidAddress;
}




/******************************************************************************
 *
 * Function   :  PlxPci_EepromPresent
 *
 * Description:  Returns the state of the EEPROM as reported by the PLX device
 *
 *****************************************************************************/
PLX_EEPROM_STATUS
PlxPci_EepromPresent(
    PLX_DEVICE_OBJECT *pDevice,
    PLX_STATUS        *pStatus
    )
{
    PLX_PARAMS IoBuffer;


    // Verify device object
    if (!IsObjectValid(pDevice))
    {
        if (pStatus != NULL)
            *pStatus = ApiInvalidDeviceInfo;
        return PLX_EEPROM_STATUS_NONE;
    }

    RtlZeroMemory( &IoBuffer, sizeof(PLX_PARAMS) );

    IoBuffer.Key = pDevice->Key;

    PlxIoMessage(
        pDevice,
        PLX_IOCTL_EEPROM_PRESENT,
        &IoBuffer
        );

    if (pStatus != NULL)
        *pStatus = IoBuffer.ReturnCode;

    return (PLX_EEPROM_STATUS)IoBuffer.value[0];
}




/******************************************************************************
 *
 * Function   :  PlxPci_EepromProbe
 *
 * Description:  Probes for the presence of an EEPROM
 *
 *****************************************************************************/
BOOLEAN
PlxPci_EepromProbe(
    PLX_DEVICE_OBJECT *pDevice,
    PLX_STATUS        *pStatus
    )
{
    PLX_PARAMS IoBuffer;


    // Verify device object
    if (!IsObjectValid(pDevice))
    {
        if (pStatus != NULL)
            *pStatus = ApiInvalidDeviceInfo;
        return FALSE;
    }

    RtlZeroMemory( &IoBuffer, sizeof(PLX_PARAMS) );

    IoBuffer.Key = pDevice->Key;

    PlxIoMessage(
        pDevice,
        PLX_IOCTL_EEPROM_PROBE,
        &IoBuffer
        );

    if (pStatus != NULL)
        *pStatus = IoBuffer.ReturnCode;

    return (BOOLEAN)IoBuffer.value[0];
}




/******************************************************************************
 *
 * Function   :  PlxPci_EepromCrcGet
 *
 * Description:  Returns the current value of the CRC and its status
 *
 *****************************************************************************/
PLX_STATUS
PlxPci_EepromCrcGet(
    PLX_DEVICE_OBJECT *pDevice,
    U32               *pCrc,
    U8                *pCrcStatus
    )
{
    PLX_PARAMS IoBuffer;


    // Verify device object
    if (!IsObjectValid(pDevice))
    {
        if (pCrc != NULL)
            *pCrc = 0;
        return ApiInvalidDeviceInfo;
    }

    RtlZeroMemory( &IoBuffer, sizeof(PLX_PARAMS) );

    IoBuffer.Key = pDevice->Key;

    PlxIoMessage(
        pDevice,
        PLX_IOCTL_EEPROM_CRC_GET,
        &IoBuffer
        );

    if (pCrc != NULL)
        *pCrc = (U32)IoBuffer.value[0];

    if (pCrcStatus != NULL)
        *pCrcStatus = (U8)IoBuffer.value[1];

    return IoBuffer.ReturnCode;
}




/******************************************************************************
 *
 * Function   :  PlxPci_EepromCrcUpdate
 *
 * Description:  Calculates a new EEPROM CRC and updates the value
 *
 *****************************************************************************/
PLX_STATUS
PlxPci_EepromCrcUpdate(
    PLX_DEVICE_OBJECT *pDevice,
    U32               *pCrc,
    BOOLEAN            bUpdateEeprom
    )
{
    PLX_PARAMS IoBuffer;


    // Verify device object
    if (!IsObjectValid(pDevice))
    {
        if (pCrc != NULL)
            *pCrc = 0;
        return ApiInvalidDeviceInfo;
    }

    RtlZeroMemory( &IoBuffer, sizeof(PLX_PARAMS) );

    IoBuffer.Key      = pDevice->Key;
    IoBuffer.value[1] = bUpdateEeprom;

    PlxIoMessage(
        pDevice,
        PLX_IOCTL_EEPROM_CRC_UPDATE,
        &IoBuffer
        );

    if (pCrc != NULL)
        *pCrc = (U32)IoBuffer.value[0];

    return IoBuffer.ReturnCode;
}




/******************************************************************************
 *
 * Function   :  PlxPci_EepromGetAddressWidth
 *
 * Description:  Gets the current EEPROM address width
 *
 *****************************************************************************/
PLX_STATUS
PlxPci_EepromGetAddressWidth(
    PLX_DEVICE_OBJECT *pDevice,
    U8                *pWidth
    )
{
    PLX_PARAMS IoBuffer;


    // Verify device object
    if (!IsObjectValid(pDevice))
        return ApiInvalidDeviceInfo;

    if (pWidth == NULL)
        return ApiNullParam;

    RtlZeroMemory( &IoBuffer, sizeof(PLX_PARAMS) );

    IoBuffer.Key = pDevice->Key;

    PlxIoMessage(
        pDevice,
        PLX_IOCTL_EEPROM_GET_ADDRESS_WIDTH,
        &IoBuffer
        );

    *pWidth = (U8)IoBuffer.value[0];

    return IoBuffer.ReturnCode;
}




/******************************************************************************
 *
 * Function   :  PlxPci_EepromSetAddressWidth
 *
 * Description:  Sets the EEPROM address width
 *
 *****************************************************************************/
PLX_STATUS
PlxPci_EepromSetAddressWidth(
    PLX_DEVICE_OBJECT *pDevice,
    U8                 width
    )
{
    PLX_PARAMS IoBuffer;


    // Verify device object
    if (!IsObjectValid(pDevice))
        return ApiInvalidDeviceInfo;

    RtlZeroMemory( &IoBuffer, sizeof(PLX_PARAMS) );

    IoBuffer.Key      = pDevice->Key;
    IoBuffer.value[0] = width;

    PlxIoMessage(
        pDevice,
        PLX_IOCTL_EEPROM_SET_ADDRESS_WIDTH,
        &IoBuffer
        );

    return IoBuffer.ReturnCode;
}




/******************************************************************************
 *
 * Function   :  PlxPci_EepromReadByOffset
 *
 * Description:  Read a value from a specified offset of the EEPROM
 *
 *****************************************************************************/
PLX_STATUS
PlxPci_EepromReadByOffset(
    PLX_DEVICE_OBJECT *pDevice,
    U32                offset,
    U32               *pValue
    )
{
    PLX_PARAMS IoBuffer;


    if (pValue == NULL)
        return ApiNullParam;

    // Verify device object
    if (!IsObjectValid(pDevice))
        return ApiInvalidDeviceInfo;

    RtlZeroMemory( &IoBuffer, sizeof(PLX_PARAMS) );

    IoBuffer.Key      = pDevice->Key;
    IoBuffer.value[0] = offset;

    PlxIoMessage(
        pDevice,
        PLX_IOCTL_EEPROM_READ_BY_OFFSET,
        &IoBuffer
        );

    if (IoBuffer.ReturnCode == ApiSuccess)
        *pValue = (U32)IoBuffer.value[1];
    else
        *pValue = 0;

    return IoBuffer.ReturnCode;
}




/******************************************************************************
 *
 * Function   :  PlxPci_EepromWriteByOffset
 *
 * Description:  Write a value to a specified offset of the EEPROM
 *
 *****************************************************************************/
PLX_STATUS
PlxPci_EepromWriteByOffset(
    PLX_DEVICE_OBJECT *pDevice,
    U32                offset,
    U32                value
    )
{
    PLX_PARAMS IoBuffer;


    // Verify device object
    if (!IsObjectValid(pDevice))
        return ApiInvalidDeviceInfo;

    RtlZeroMemory( &IoBuffer, sizeof(PLX_PARAMS) );

    IoBuffer.Key      = pDevice->Key;
    IoBuffer.value[0] = offset;
    IoBuffer.value[1] = value;

    PlxIoMessage(
        pDevice,
        PLX_IOCTL_EEPROM_WRITE_BY_OFFSET,
        &IoBuffer
        );

    return IoBuffer.ReturnCode;
}




/******************************************************************************
 *
 * Function   :  PlxPci_EepromReadByOffset_16
 *
 * Description:  Read a 16-bit value from a specified offset of the EEPROM
 *
 *****************************************************************************/
PLX_STATUS
PlxPci_EepromReadByOffset_16(
    PLX_DEVICE_OBJECT *pDevice,
    U32                offset,
    U16               *pValue
    )
{
    PLX_PARAMS IoBuffer;


    if (pValue == NULL)
        return ApiNullParam;

    // Verify device object
    if (!IsObjectValid(pDevice))
        return ApiInvalidDeviceInfo;

    RtlZeroMemory( &IoBuffer, sizeof(PLX_PARAMS) );

    IoBuffer.Key      = pDevice->Key;
    IoBuffer.value[0] = offset;

    PlxIoMessage(
        pDevice,
        PLX_IOCTL_EEPROM_READ_BY_OFFSET_16,
        &IoBuffer
        );

    if (IoBuffer.ReturnCode == ApiSuccess)
        *pValue = (U16)IoBuffer.value[1];
    else
        *pValue = 0;

    return IoBuffer.ReturnCode;
}




/******************************************************************************
 *
 * Function   :  PlxPci_EepromWriteByOffset_16
 *
 * Description:  Write a 16-bit value to a specified offset of the EEPROM
 *
 *****************************************************************************/
PLX_STATUS
PlxPci_EepromWriteByOffset_16(
    PLX_DEVICE_OBJECT *pDevice,
    U32                offset,
    U16                value
    )
{
    PLX_PARAMS IoBuffer;


    // Verify device object
    if (!IsObjectValid(pDevice))
        return ApiInvalidDeviceInfo;

    RtlZeroMemory( &IoBuffer, sizeof(PLX_PARAMS) );

    IoBuffer.Key      = pDevice->Key;
    IoBuffer.value[0] = offset;
    IoBuffer.value[1] = value;

    PlxIoMessage(
        pDevice,
        PLX_IOCTL_EEPROM_WRITE_BY_OFFSET_16,
        &IoBuffer
        );

    return IoBuffer.ReturnCode;
}




/******************************************************************************
 *
 * Function   :  PlxPci_IoPortRead
 *
 * Description:  Reads from a specified I/O port
 *
 *****************************************************************************/
PLX_STATUS
PlxPci_IoPortRead(
    PLX_DEVICE_OBJECT *pDevice,
    U64                port,
    VOID              *pBuffer,
    U32                ByteCount,
    PLX_ACCESS_TYPE    AccessType
    )
{
    PLX_PARAMS IoBuffer;


    if (pBuffer == NULL)
        return ApiNullParam;

    // Verify device object
    if (!IsObjectValid(pDevice))
        return ApiInvalidDeviceInfo;

    RtlZeroMemory( &IoBuffer, sizeof(PLX_PARAMS) );

    IoBuffer.value[0]             = port;
    IoBuffer.value[1]             = AccessType;
    IoBuffer.u.TxParams.UserVa    = (PLX_UINT_PTR)pBuffer;
    IoBuffer.u.TxParams.ByteCount = ByteCount;

    PlxIoMessage(
        pDevice,
        PLX_IOCTL_IO_PORT_READ,
        &IoBuffer
        );

    return IoBuffer.ReturnCode;
}




/******************************************************************************
 *
 * Function   :  PlxPci_IoPortWrite
 *
 * Description:  Writes to a specified I/O port
 *
 *****************************************************************************/
PLX_STATUS
PlxPci_IoPortWrite(
    PLX_DEVICE_OBJECT *pDevice,
    U64                port,
    VOID              *pBuffer,
    U32                ByteCount,
    PLX_ACCESS_TYPE    AccessType
    )
{
    PLX_PARAMS IoBuffer;


    if (pBuffer == NULL)
        return ApiNullParam;

    // Verify device object
    if (!IsObjectValid(pDevice))
        return ApiInvalidDeviceInfo;

    RtlZeroMemory( &IoBuffer, sizeof(PLX_PARAMS) );

    IoBuffer.value[0]             = port;
    IoBuffer.value[1]             = AccessType;
    IoBuffer.u.TxParams.UserVa    = (PLX_UINT_PTR)pBuffer;
    IoBuffer.u.TxParams.ByteCount = ByteCount;

    PlxIoMessage(
        pDevice,
        PLX_IOCTL_IO_PORT_WRITE,
        &IoBuffer
        );

    return IoBuffer.ReturnCode;
}




/******************************************************************************
 *
 * Function   :  PlxPci_PciBarSpaceRead
 *
 * Description:  Reads data from a specified PCI BAR space
 *
 *****************************************************************************/
PLX_STATUS
PlxPci_PciBarSpaceRead(
    PLX_DEVICE_OBJECT *pDevice,
    U8                 BarIndex,
    U32                offset,
    VOID              *pBuffer,
    U32                ByteCount,
    PLX_ACCESS_TYPE    AccessType,
    BOOLEAN            bOffsetAsLocalAddr
    )
{
    PLX_PARAMS IoBuffer;


    if (pBuffer == NULL)
        return ApiNullParam;

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
            return ApiInvalidIndex;
    }

    // Verify device object
    if (!IsObjectValid(pDevice))
        return ApiInvalidDeviceInfo;

    RtlZeroMemory( &IoBuffer, sizeof(PLX_PARAMS) );

    IoBuffer.value[0]             = BarIndex;
    IoBuffer.value[1]             = AccessType;
    IoBuffer.value[2]             = bOffsetAsLocalAddr;
    IoBuffer.u.TxParams.LocalAddr = offset;
    IoBuffer.u.TxParams.ByteCount = ByteCount;
    IoBuffer.u.TxParams.UserVa    = (PLX_UINT_PTR)pBuffer;

    PlxIoMessage(
        pDevice,
        PLX_IOCTL_PCI_BAR_SPACE_READ,
        &IoBuffer
        );

    return IoBuffer.ReturnCode;
}




/******************************************************************************
 *
 * Function   :  PlxPci_PciBarSpaceWrite
 *
 * Description:  Writes data to a specified PCI BAR space
 *
 *****************************************************************************/
PLX_STATUS
PlxPci_PciBarSpaceWrite(
    PLX_DEVICE_OBJECT *pDevice,
    U8                 BarIndex,
    U32                offset,
    VOID              *pBuffer,
    U32                ByteCount,
    PLX_ACCESS_TYPE    AccessType,
    BOOLEAN            bOffsetAsLocalAddr
    )
{
    PLX_PARAMS IoBuffer;


    if (pBuffer == NULL)
        return ApiNullParam;

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
            return ApiInvalidIndex;
    }

    // Verify device object
    if (!IsObjectValid(pDevice))
        return ApiInvalidDeviceInfo;

    RtlZeroMemory( &IoBuffer, sizeof(PLX_PARAMS) );

    IoBuffer.value[0]             = BarIndex;
    IoBuffer.value[1]             = AccessType;
    IoBuffer.value[2]             = bOffsetAsLocalAddr;
    IoBuffer.u.TxParams.LocalAddr = offset;
    IoBuffer.u.TxParams.ByteCount = ByteCount;
    IoBuffer.u.TxParams.UserVa    = (PLX_UINT_PTR)pBuffer;

    PlxIoMessage(
        pDevice,
        PLX_IOCTL_PCI_BAR_SPACE_WRITE,
        &IoBuffer
        );

    return IoBuffer.ReturnCode;
}




/******************************************************************************
 *
 * Function   :  PlxPci_PhysicalMemoryAllocate
 *
 * Description:  Allocate a physically contigous page-locked buffer
 *
 *****************************************************************************/
PLX_STATUS
PlxPci_PhysicalMemoryAllocate(
    PLX_DEVICE_OBJECT *pDevice,
    PLX_PHYSICAL_MEM  *pMemoryInfo,
    BOOLEAN            bSmallerOk
    )
{
    PLX_PARAMS IoBuffer;


    if (pMemoryInfo == NULL)
        return ApiNullParam;

    // Verify device object
    if (!IsObjectValid(pDevice))
        return ApiInvalidDeviceInfo;

    RtlZeroMemory( &IoBuffer, sizeof(PLX_PARAMS) );

    IoBuffer.Key         = pDevice->Key;
    IoBuffer.value[0]    = bSmallerOk;
    IoBuffer.u.PciMemory = *pMemoryInfo;

    PlxIoMessage(
        pDevice,
        PLX_IOCTL_PHYSICAL_MEM_ALLOCATE,
        &IoBuffer
        );

    // Copy buffer information
    *pMemoryInfo = IoBuffer.u.PciMemory;

    return IoBuffer.ReturnCode;
}




/******************************************************************************
 *
 * Function   :  PlxPci_PhysicalMemoryFree
 *
 * Description:  Free a previously allocated physically contigous buffer
 *
 *****************************************************************************/
PLX_STATUS
PlxPci_PhysicalMemoryFree(
    PLX_DEVICE_OBJECT *pDevice,
    PLX_PHYSICAL_MEM  *pMemoryInfo
    )
{
    PLX_PARAMS IoBuffer;


    if (pMemoryInfo == NULL)
        return ApiNullParam;

    // Verify device object
    if (!IsObjectValid(pDevice))
        return ApiInvalidDeviceInfo;

    // Unmap the buffer if it was previously mapped to user space
    if (pMemoryInfo->UserAddr != 0)
    {
        PlxPci_PhysicalMemoryUnmap(
            pDevice,
            pMemoryInfo
            );
    }

    RtlZeroMemory( &IoBuffer, sizeof(PLX_PARAMS) );

    IoBuffer.Key         = pDevice->Key;
    IoBuffer.u.PciMemory = *pMemoryInfo;

    PlxIoMessage(
        pDevice,
        PLX_IOCTL_PHYSICAL_MEM_FREE,
        &IoBuffer
        );

    // Clear buffer information
    RtlZeroMemory( pMemoryInfo, sizeof(PLX_PHYSICAL_MEM) );

    return IoBuffer.ReturnCode;
}




/******************************************************************************
 *
 * Function   :  PlxPci_PhysicalMemoryMap
 *
 * Description:  Maps a page-locked buffer into user virtual space
 *
 *****************************************************************************/
PLX_STATUS
PlxPci_PhysicalMemoryMap(
    PLX_DEVICE_OBJECT *pDevice,
    PLX_PHYSICAL_MEM  *pMemoryInfo
    )
{
    if (pMemoryInfo == NULL)
        return ApiNullParam;

    // Set default return value
    pMemoryInfo->UserAddr = 0;

    // Verify device object
    if (!IsObjectValid(pDevice))
        return ApiInvalidDeviceInfo;

    // Verify buffer object
    if ((pMemoryInfo->CpuPhysical == 0) ||
        (pMemoryInfo->Size        == 0))
    {
        return ApiInvalidData;
    }

    // Map the buffer to user space
    pMemoryInfo->UserAddr =
        (PLX_UINT_PTR)mmap(
            0,
            pMemoryInfo->Size,
            PROT_READ | PROT_WRITE,
            MAP_SHARED,
            pDevice->hDevice,
            pMemoryInfo->CpuPhysical     // CPU Physical address of buffer
            );

    if (pMemoryInfo->UserAddr == (PLX_UINT_PTR)MAP_FAILED)
    {
        pMemoryInfo->UserAddr = 0;

        return ApiInsufficientResources;
    }

    return ApiSuccess;
}




/******************************************************************************
 *
 * Function   :  PlxPci_PhysicalMemoryUnmap
 *
 * Description:  Unmaps a page-locked buffer from user virtual space
 *
 *****************************************************************************/
PLX_STATUS
PlxPci_PhysicalMemoryUnmap(
    PLX_DEVICE_OBJECT *pDevice,
    PLX_PHYSICAL_MEM  *pMemoryInfo
    )
{
    int rc;


    if (pMemoryInfo == NULL)
        return ApiNullParam;

    // Verify device object
    if (!IsObjectValid(pDevice))
        return ApiInvalidDeviceInfo;

    // Verify buffer object
    if ((pMemoryInfo->CpuPhysical == 0) ||
        (pMemoryInfo->Size        == 0))
    {
        return ApiInvalidData;
    }

    // Verify virtual address
    if (pMemoryInfo->UserAddr == 0)
        return ApiInvalidAddress;

    // Unmap buffer from virtual space
    rc =
        munmap(
            PLX_INT_TO_PTR(pMemoryInfo->UserAddr),
            pMemoryInfo->Size
            );

    if (rc != 0)
    {
        return ApiInvalidAddress;
    }

    // Clear buffer address
    pMemoryInfo->UserAddr = 0;

    return ApiSuccess;
}




/******************************************************************************
 *
 * Function   :  PlxPci_CommonBufferProperties
 *
 * Description:  Retrieves the common buffer properties for a given device
 *
 *****************************************************************************/
PLX_STATUS
PlxPci_CommonBufferProperties(
    PLX_DEVICE_OBJECT *pDevice,
    PLX_PHYSICAL_MEM  *pMemoryInfo
    )
{
    PLX_PARAMS IoBuffer;


    if (pMemoryInfo == NULL)
        return ApiNullParam;

    // Verify device object
    if (!IsObjectValid(pDevice))
        return ApiInvalidDeviceInfo;

    // If we already have the information, just copy it
    if (pDevice->CommonBuffer.PhysicalAddr != 0)
    {
        *pMemoryInfo = pDevice->CommonBuffer;
        return ApiSuccess;
    }

    // Clear properties in case not supported in driver
    RtlZeroMemory( pMemoryInfo, sizeof(PLX_PHYSICAL_MEM) );

    RtlZeroMemory( &IoBuffer, sizeof(PLX_PARAMS) );

    PlxIoMessage(
        pDevice,
        PLX_IOCTL_COMMON_BUFFER_PROPERTIES,
        &IoBuffer
        );

    if (IoBuffer.ReturnCode != ApiSuccess)
        return IoBuffer.ReturnCode;

    // Copy buffer properties
    *pMemoryInfo = IoBuffer.u.PciMemory;

    // Make sure virtual address is clear
    pMemoryInfo->UserAddr = 0;

    // Save the data for any future calls
    pDevice->CommonBuffer = *pMemoryInfo;

    return ApiSuccess;
}




/******************************************************************************
 *
 * Function   :  PlxPci_CommonBufferMap
 *
 * Description:  Maps the common buffer into user virtual space
 *
 *****************************************************************************/
PLX_STATUS
PlxPci_CommonBufferMap(
    PLX_DEVICE_OBJECT  *pDevice,
    VOID              **pVa
    )
{
    PLX_STATUS       rc;
    PLX_PHYSICAL_MEM MemInfo;


    if (pVa == NULL)
        return ApiNullParam;

    // Set default return value
    *(PLX_UINT_PTR*)pVa = 0;

    // Verify device object
    if (!IsObjectValid(pDevice))
        return ApiInvalidDeviceInfo;

    // If buffer was previously mapped, just copy it
    if (pDevice->CommonBuffer.UserAddr != 0)
    {
        *(PLX_UINT_PTR*)pVa = (PLX_UINT_PTR)pDevice->CommonBuffer.UserAddr;
        return ApiSuccess;
    }

    // Check for valid common buffer info
    if (pDevice->CommonBuffer.PhysicalAddr == 0)
    {
        // Get buffer properties
        rc =
            PlxPci_CommonBufferProperties(
                pDevice,
                &MemInfo
                );

        if (rc != ApiSuccess)
            return rc;
    }
    else
    {
        // Copy buffer properties
        MemInfo = pDevice->CommonBuffer;
    }

    rc =
        PlxPci_PhysicalMemoryMap(
            pDevice,
            &MemInfo
            );

    if (rc != ApiSuccess)
    {
        *(PLX_UINT_PTR*)pVa = 0;
        return rc;
    }

    // Pass virtual address
    *(PLX_UINT_PTR*)pVa = (PLX_UINT_PTR)MemInfo.UserAddr;

    // Save the data for any future calls
    pDevice->CommonBuffer.UserAddr = MemInfo.UserAddr;

    return ApiSuccess;
}




/******************************************************************************
 *
 * Function   :  PlxPci_CommonBufferUnmap
 *
 * Description:  Unmaps the common buffer from user virtual space
 *
 *****************************************************************************/
PLX_STATUS
PlxPci_CommonBufferUnmap(
    PLX_DEVICE_OBJECT  *pDevice,
    VOID              **pVa
    )
{
    PLX_STATUS       rc;
    PLX_PHYSICAL_MEM MemInfo;


    if (pVa == NULL)
        return ApiNullParam;

    if (*(PLX_UINT_PTR*)pVa == 0)
        return ApiInvalidAddress;

    // Verify device object
    if (!IsObjectValid(pDevice))
        return ApiInvalidDeviceInfo;

    // Verify virtual address
    if (pDevice->CommonBuffer.UserAddr != *(PLX_UINT_PTR*)pVa)
    {
        return ApiInvalidAddress;
    }

    // Copy buffer properties
    MemInfo = pDevice->CommonBuffer;

    // Unmap the buffer
    rc =
        PlxPci_PhysicalMemoryUnmap(
            pDevice,
            &MemInfo
            );

    if (rc == ApiSuccess)
    {
        // Clear internal data
        pDevice->CommonBuffer.UserAddr = 0;

        // Clear buffer address
        *(PLX_UINT_PTR*)pVa = 0;
    }

    return rc;
}




/******************************************************************************
 *
 * Function   :  PlxPci_InterruptEnable
 *
 * Description:  Enable specific interrupts of the PLX chip
 *
 *****************************************************************************/
PLX_STATUS
PlxPci_InterruptEnable(
    PLX_DEVICE_OBJECT *pDevice,
    PLX_INTERRUPT     *pPlxIntr
    )
{
    PLX_PARAMS IoBuffer;


    // Check for null pointers
    if (pPlxIntr == NULL)
        return ApiNullParam;

    // Verify device object
    if (!IsObjectValid(pDevice))
        return ApiInvalidDeviceInfo;

    RtlZeroMemory( &IoBuffer, sizeof(PLX_PARAMS) );

    IoBuffer.u.PlxIntr = *pPlxIntr;

    PlxIoMessage(
        pDevice,
        PLX_IOCTL_INTR_ENABLE,
        &IoBuffer
        );

    return IoBuffer.ReturnCode;
}




/******************************************************************************
 *
 * Function   :  PlxPci_InterruptDisable
 *
 * Description:  Disable specific interrupts of the PLX chip
 *
 *****************************************************************************/
PLX_STATUS
PlxPci_InterruptDisable(
    PLX_DEVICE_OBJECT *pDevice,
    PLX_INTERRUPT     *pPlxIntr
    )
{
    PLX_PARAMS IoBuffer;


    // Check for null pointers
    if (pPlxIntr == NULL)
        return ApiNullParam;

    // Verify device object
    if (!IsObjectValid(pDevice))
        return ApiInvalidDeviceInfo;

    RtlZeroMemory( &IoBuffer, sizeof(PLX_PARAMS) );

    IoBuffer.u.PlxIntr = *pPlxIntr;

    PlxIoMessage(
        pDevice,
        PLX_IOCTL_INTR_DISABLE,
        &IoBuffer
        );

    return IoBuffer.ReturnCode;
}




/******************************************************************************
 *
 * Function   :  PlxPci_NotificationRegisterFor
 *
 * Description:  Registers for event notification
 *
 *****************************************************************************/
PLX_STATUS
PlxPci_NotificationRegisterFor(
    PLX_DEVICE_OBJECT *pDevice,
    PLX_INTERRUPT     *pPlxIntr,
    PLX_NOTIFY_OBJECT *pEvent
    )
{
#if defined(PLX_DOS)

    // Notification events not supported in DOS
    return ApiUnsupportedFunction;

#elif defined(PLX_LINUX)

    PLX_PARAMS IoBuffer;


    // Check for null pointers
    if ((pPlxIntr == NULL) || (pEvent == NULL))
        return ApiNullParam;

    // Verify device object
    if (!IsObjectValid(pDevice))
        return ApiInvalidDeviceInfo;

    RtlZeroMemory( &IoBuffer, sizeof(PLX_PARAMS) );

    IoBuffer.u.PlxIntr = *pPlxIntr;

    PlxIoMessage(
        pDevice,
        PLX_IOCTL_NOTIFICATION_REGISTER_FOR,
        &IoBuffer
        );

    if (IoBuffer.ReturnCode == ApiSuccess)
    {
        // Mark object as valid if successful
        ObjectValidate( pEvent );

        // Store driver object
        pEvent->pWaitObject = IoBuffer.value[0];
    }

    return IoBuffer.ReturnCode;

#endif
}




/******************************************************************************
 *
 * Function   :  PlxPci_NotificationWait
 *
 * Description:  Waits for a registered notification event to signal
 *
 *****************************************************************************/
PLX_STATUS
PlxPci_NotificationWait(
    PLX_DEVICE_OBJECT *pDevice,
    PLX_NOTIFY_OBJECT *pEvent,
    U64                Timeout_ms
    )
{
#if defined(PLX_DOS)

    // Notification events not supported in DOS
    return ApiUnsupportedFunction;

#elif defined(PLX_LINUX)

    PLX_PARAMS IoBuffer;


    // Verify notify object
    if (pEvent == NULL)
        return ApiNullParam;

    // Verify device object
    if (!IsObjectValid(pDevice))
        return ApiInvalidDeviceInfo;

    // Verify event object
    if (!IsObjectValid(pEvent))
        return ApiFailed;

    RtlZeroMemory( &IoBuffer, sizeof(PLX_PARAMS) );

    IoBuffer.value[0] = pEvent->pWaitObject;
    IoBuffer.value[1] = Timeout_ms;

    // Send message to driver
    PlxIoMessage(
        pDevice,
        PLX_IOCTL_NOTIFICATION_WAIT,
        &IoBuffer
        );

    return IoBuffer.ReturnCode;

#endif
}




/******************************************************************************
 *
 * Function   :  PlxPci_NotificationStatus
 *
 * Description:  Returns the interrupt(s) that have caused notification events
 *
 *****************************************************************************/
PLX_STATUS
PlxPci_NotificationStatus(
    PLX_DEVICE_OBJECT *pDevice,
    PLX_NOTIFY_OBJECT *pEvent,
    PLX_INTERRUPT     *pPlxIntr
    )
{
    PLX_PARAMS IoBuffer;


    // Verify notify object
    if ((pPlxIntr == NULL) || (pEvent == NULL))
        return ApiNullParam;

    // Verify device object
    if (!IsObjectValid(pDevice))
        return ApiInvalidDeviceInfo;

    // Verify event object
    if (!IsObjectValid(pEvent))
        return ApiFailed;

    RtlZeroMemory( &IoBuffer, sizeof(PLX_PARAMS) );

    IoBuffer.value[0] = pEvent->pWaitObject;

    PlxIoMessage(
        pDevice,
        PLX_IOCTL_NOTIFICATION_STATUS,
        &IoBuffer
        );

    if (IoBuffer.ReturnCode == ApiSuccess)
    {
        // Return interrupt sources
        *pPlxIntr = IoBuffer.u.PlxIntr;
    }

    return IoBuffer.ReturnCode;
}




/******************************************************************************
 *
 * Function   :  PlxPci_NotificationCancel
 *
 * Description:  Cancels a previously registered notification event
 *
 *****************************************************************************/
PLX_STATUS
PlxPci_NotificationCancel(
    PLX_DEVICE_OBJECT *pDevice,
    PLX_NOTIFY_OBJECT *pEvent
    )
{
    PLX_PARAMS IoBuffer;


    // Verify notify object
    if (pEvent == NULL)
        return ApiNullParam;

    // Verify device object
    if (!IsObjectValid(pDevice))
        return ApiInvalidDeviceInfo;

    // Verify event object
    if (!IsObjectValid(pEvent))
        return ApiFailed;

    RtlZeroMemory( &IoBuffer, sizeof(PLX_PARAMS) );

    IoBuffer.value[0] = pEvent->pWaitObject;

    PlxIoMessage(
        pDevice,
        PLX_IOCTL_NOTIFICATION_CANCEL,
        &IoBuffer
        );

    if (IoBuffer.ReturnCode == ApiSuccess)
    {
        // Mark object as invalid
        ObjectInvalidate( pEvent );
    }

    return IoBuffer.ReturnCode;
}




/******************************************************************************
 *
 * Function   :  PlxPci_VpdRead
 *
 * Description:  Read VPD data at the specified offset
 *
 *****************************************************************************/
U32
PlxPci_VpdRead(
    PLX_DEVICE_OBJECT *pDevice,
    U16                offset,
    PLX_STATUS        *pStatus
    )
{
    PLX_PARAMS IoBuffer;


    // Verify device object
    if (!IsObjectValid(pDevice))
    {
        if (pStatus != NULL)
            *pStatus = ApiInvalidDeviceInfo;
        return (U32)-1;
    }

    RtlZeroMemory( &IoBuffer, sizeof(PLX_PARAMS) );

    IoBuffer.Key      = pDevice->Key;
    IoBuffer.value[0] = offset;

    PlxIoMessage(
        pDevice,
        PLX_IOCTL_VPD_READ,
        &IoBuffer
        );

    if (pStatus != NULL)
        *pStatus = IoBuffer.ReturnCode;

    return (U32)IoBuffer.value[1];
}




/******************************************************************************
 *
 * Function   :  PlxPci_VpdWrite
 *
 * Description:  Write VPD data to the specified offset
 *
 *****************************************************************************/
PLX_STATUS
PlxPci_VpdWrite(
    PLX_DEVICE_OBJECT *pDevice,
    U16                offset,
    U32                value
    )
{
    PLX_PARAMS IoBuffer;


    // Verify device object
    if (!IsObjectValid(pDevice))
        return ApiInvalidDeviceInfo;

    RtlZeroMemory( &IoBuffer, sizeof(PLX_PARAMS) );

    IoBuffer.Key      = pDevice->Key;
    IoBuffer.value[0] = offset;
    IoBuffer.value[1] = value;

    PlxIoMessage(
        pDevice,
        PLX_IOCTL_VPD_WRITE,
        &IoBuffer
        );

    return IoBuffer.ReturnCode;
}




/******************************************************************************
 *
 * Function   :  PlxPci_DmaChannelOpen
 *
 * Description:  Open a channel to perform DMA transfers
 *
 *****************************************************************************/
PLX_STATUS
PlxPci_DmaChannelOpen(
    PLX_DEVICE_OBJECT *pDevice,
    U8                 channel,
    PLX_DMA_PROP      *pDmaProp
    )
{
    PLX_PARAMS IoBuffer;


    // Verify device object
    if (!IsObjectValid(pDevice))
        return ApiInvalidDeviceInfo;

    RtlZeroMemory( &IoBuffer, sizeof(PLX_PARAMS) );

    IoBuffer.value[0] = channel;

    PlxIoMessage(
        pDevice,
        PLX_IOCTL_DMA_CHANNEL_OPEN,
        &IoBuffer
        );

    // If channel opened, update properties if provided
    if (IoBuffer.ReturnCode == ApiSuccess)
    {
        if (pDmaProp != NULL)
        {
            IoBuffer.ReturnCode =
                PlxPci_DmaSetProperties(
                    pDevice,
                    channel,
                    pDmaProp
                    );
        }
    }

    return IoBuffer.ReturnCode;
}




/******************************************************************************
 *
 * Function   :  PlxPci_DmaGetProperties
 *
 * Description:  Gets the current properties of a DMA channel
 *
 *****************************************************************************/
PLX_STATUS
PlxPci_DmaGetProperties(
    PLX_DEVICE_OBJECT *pDevice,
    U8                 channel,
    PLX_DMA_PROP      *pDmaProp
    )
{
    PLX_PARAMS IoBuffer;


    if (pDmaProp == NULL)
        return ApiNullParam;

    // Verify device object
    if (!IsObjectValid(pDevice))
        return ApiInvalidDeviceInfo;

    RtlZeroMemory( &IoBuffer, sizeof(PLX_PARAMS) );

    IoBuffer.value[0] = channel;

    PlxIoMessage(
        pDevice,
        PLX_IOCTL_DMA_GET_PROPERTIES,
        &IoBuffer
        );

    // Return properties on success
    if (IoBuffer.ReturnCode == ApiSuccess)
    {
        *pDmaProp = IoBuffer.u.DmaProp;
    }

    return IoBuffer.ReturnCode;
}




/******************************************************************************
 *
 * Function   :  PlxPci_DmaSetProperties
 *
 * Description:  Sets the properties of a DMA channel
 *
 *****************************************************************************/
PLX_STATUS
PlxPci_DmaSetProperties(
    PLX_DEVICE_OBJECT *pDevice,
    U8                 channel,
    PLX_DMA_PROP      *pDmaProp
    )
{
    PLX_PARAMS IoBuffer;


    if (pDmaProp == NULL)
        return ApiNullParam;

    // Verify device object
    if (!IsObjectValid(pDevice))
        return ApiInvalidDeviceInfo;

    RtlZeroMemory( &IoBuffer, sizeof(PLX_PARAMS) );

    IoBuffer.value[0]  = channel;
    IoBuffer.u.DmaProp = *pDmaProp;

    PlxIoMessage(
        pDevice,
        PLX_IOCTL_DMA_SET_PROPERTIES,
        &IoBuffer
        );

    return IoBuffer.ReturnCode;
}




/******************************************************************************
 *
 * Function   :  PlxPci_DmaControl
 *
 * Description:  Perform a command on a specified DMA channel
 *
 *****************************************************************************/
PLX_STATUS
PlxPci_DmaControl(
    PLX_DEVICE_OBJECT *pDevice,
    U8                 channel,
    PLX_DMA_COMMAND    command
    )
{
    PLX_PARAMS IoBuffer;


    // Verify device object
    if (!IsObjectValid(pDevice))
        return ApiInvalidDeviceInfo;

    RtlZeroMemory( &IoBuffer, sizeof(PLX_PARAMS) );

    IoBuffer.value[0] = channel;
    IoBuffer.value[1] = command;

    PlxIoMessage(
        pDevice,
        PLX_IOCTL_DMA_CONTROL,
        &IoBuffer
        );

    return IoBuffer.ReturnCode;
}




/******************************************************************************
 *
 * Function   :  PlxPci_DmaStatus
 *
 * Description:  Read the status of a DMA channel
 *
 *****************************************************************************/
PLX_STATUS
PlxPci_DmaStatus(
    PLX_DEVICE_OBJECT *pDevice,
    U8                 channel
    )
{
    PLX_PARAMS IoBuffer;


    // Verify device object
    if (!IsObjectValid(pDevice))
        return ApiInvalidDeviceInfo;

    RtlZeroMemory( &IoBuffer, sizeof(PLX_PARAMS) );

    IoBuffer.value[0] = channel;

    PlxIoMessage(
        pDevice,
        PLX_IOCTL_DMA_STATUS,
        &IoBuffer
        );

    return IoBuffer.ReturnCode;
}




/******************************************************************************
 *
 * Function   :  PlxPci_DmaTransferBlock
 *
 * Description:  Performs a Block DMA transfer
 *
 *****************************************************************************/
PLX_STATUS
PlxPci_DmaTransferBlock(
    PLX_DEVICE_OBJECT *pDevice,
    U8                 channel,
    PLX_DMA_PARAMS    *pDmaParams,
    U64                Timeout_ms
    )
{
    BOOLEAN           bIgnoreInt;
    PLX_PARAMS        IoBuffer;
    PLX_STATUS        rc;
    PLX_INTERRUPT     PlxIntr;
    PLX_NOTIFY_OBJECT Event;


    if (pDmaParams == NULL)
        return ApiNullParam;

    // Verify device object
    if (!IsObjectValid(pDevice))
        return ApiInvalidDeviceInfo;

    // Added to avoid compiler warning
    bIgnoreInt = FALSE;

    // Setup to wait for interrupt if requested
    if (Timeout_ms != 0)
    {
        // Clear interrupt fields
        RtlZeroMemory( &PlxIntr, sizeof(PLX_INTERRUPT) );

        // Setup for DMA done interrupt
        if (((S8)channel >= 0) && ((S8)channel < 4))
            PlxIntr.DmaDone = (1 << channel);
        else
            return ApiDmaChannelInvalid;

        // Register to wait for DMA interrupt
        PlxPci_NotificationRegisterFor(
            pDevice,
            &PlxIntr,
            &Event
            );

        // Store original value of interrupt ignore
        bIgnoreInt = pDmaParams->bIgnoreBlockInt;

        // Since user asks API to wait, interrupt can't be ignored
        pDmaParams->bIgnoreBlockInt = FALSE;
    }

    RtlZeroMemory( &IoBuffer, sizeof(PLX_PARAMS) );

    IoBuffer.value[0]   = channel;
    IoBuffer.u.TxParams = *pDmaParams;

    PlxIoMessage(
        pDevice,
        PLX_IOCTL_DMA_TRANSFER_BLOCK,
        &IoBuffer
        );

    rc = IoBuffer.ReturnCode;

    // Don't wait for completion if requested not to
    if (Timeout_ms == 0)
    {
        return rc;
    }
    else
    {
        // Restore original value of interrupt ignore
        pDmaParams->bIgnoreBlockInt = bIgnoreInt;
    }

    // Wait for completion if requested
    if (rc == ApiSuccess)
    {
        rc =
            PlxPci_NotificationWait(
                pDevice,
                &Event,
                Timeout_ms
                );

        switch (rc)
        {
            case ApiSuccess:
                // DMA transfer completed
                break;

            case ApiWaitTimeout:
                rc = ApiWaitTimeout;
                break;

            case ApiWaitCanceled:
                rc = ApiFailed;
                break;

            default:
                // Added to avoid compiler warning
                break;
        }
    }

    // Cancel event notification
    PlxPci_NotificationCancel(
        pDevice,
        &Event
        );

    return rc;
}




/******************************************************************************
 *
 * Function   :  PlxPci_DmaTransferUserBuffer
 *
 * Description:  Transfers a user-mode buffer using DMA
 *
 *****************************************************************************/
PLX_STATUS
PlxPci_DmaTransferUserBuffer(
    PLX_DEVICE_OBJECT *pDevice,
    U8                 channel,
    PLX_DMA_PARAMS    *pDmaParams,
    U64                Timeout_ms
    )
{
    PLX_PARAMS        IoBuffer;
    PLX_STATUS        rc;
    PLX_INTERRUPT     PlxIntr;
    PLX_NOTIFY_OBJECT Event;


    if (pDmaParams == NULL)
        return ApiNullParam;

    // Verify device object
    if (!IsObjectValid(pDevice))
        return ApiInvalidDeviceInfo;

    // Setup to wait for interrupt if requested
    if (Timeout_ms != 0)
    {
        // Clear interrupt fields
        RtlZeroMemory( &PlxIntr, sizeof(PLX_INTERRUPT) );

        // Setup for DMA done interrupt
        if (((S8)channel >= 0) && ((S8)channel < 4))
            PlxIntr.DmaDone = (1 << channel);
        else
            return ApiDmaChannelInvalid;

        // Register to wait for DMA interrupt
        PlxPci_NotificationRegisterFor(
            pDevice,
            &PlxIntr,
            &Event
            );
    }

    RtlZeroMemory( &IoBuffer, sizeof(PLX_PARAMS) );

    IoBuffer.value[0]   = channel;
    IoBuffer.u.TxParams = *pDmaParams;

    PlxIoMessage(
        pDevice,
        PLX_IOCTL_DMA_TRANSFER_USER_BUFFER,
        &IoBuffer
        );

    rc = IoBuffer.ReturnCode;

    // Don't wait for completion if requested not to
    if (Timeout_ms == 0)
        return rc;

    // Wait for completion if requested
    if (rc == ApiSuccess)
    {
        rc =
            PlxPci_NotificationWait(
                pDevice,
                &Event,
                Timeout_ms
                );

        switch (rc)
        {
            case ApiSuccess:
                // DMA transfer completed
                break;

            case ApiWaitTimeout:
                rc = ApiWaitTimeout;
                break;

            case ApiWaitCanceled:
                rc = ApiFailed;
                break;

            default:
                // Added to avoid compiler warning
                break;
        }
    }

    // Cancel event notification
    PlxPci_NotificationCancel(
        pDevice,
        &Event
        );

    return rc;
}




/******************************************************************************
 *
 * Function   :  PlxPci_DmaChannelClose
 *
 * Description:  Close a previously opened DMA channel.
 *
 *****************************************************************************/
PLX_STATUS
PlxPci_DmaChannelClose(
    PLX_DEVICE_OBJECT *pDevice,
    U8                 channel
    )
{
    PLX_PARAMS IoBuffer;


    // Verify device object
    if (!IsObjectValid(pDevice))
        return ApiInvalidDeviceInfo;

    RtlZeroMemory( &IoBuffer, sizeof(PLX_PARAMS) );

    IoBuffer.value[0] = channel;

    PlxIoMessage(
        pDevice,
        PLX_IOCTL_DMA_CHANNEL_CLOSE,
        &IoBuffer
        );

    return IoBuffer.ReturnCode;
}




/******************************************************************************
 *
 * Function   :  PlxPci_PerformanceInitializeProperties
 *
 * Description:  Initilalize the performance properties for a device
 *
 *****************************************************************************/
PLX_STATUS
PlxPci_PerformanceInitializeProperties(
    PLX_DEVICE_OBJECT *pDevice,
    PLX_PERF_PROP     *pPerfProp
    )
{
    PLX_PARAMS IoBuffer;


    if ((pDevice == NULL) || (pPerfProp == NULL))
        return ApiNullParam;

    // Verify device object
    if (!IsObjectValid(pDevice))
        return ApiInvalidDeviceInfo;

    // Make sure Performance object is not already in use
    if (IsObjectValid(pPerfProp))
        return ApiInvalidDeviceInfo;

    RtlZeroMemory( &IoBuffer, sizeof(PLX_PARAMS) );

    IoBuffer.Key      = pDevice->Key;
    IoBuffer.value[0] = (PLX_UINT_PTR)pPerfProp;

    PlxIoMessage(
        pDevice,
        PLX_IOCTL_PERFORMANCE_INIT_PROPERTIES,
        &IoBuffer
        );

    if (IoBuffer.ReturnCode != ApiSuccess)
        return IoBuffer.ReturnCode;

    // Mark object as valid
    ObjectValidate( pPerfProp );

    return ApiSuccess;
}




/******************************************************************************
 *
 * Function   :  PlxPci_PerformanceMonitorControl
 *
 * Description:  Control the performance monitor
 *
 *****************************************************************************/
PLX_STATUS
PlxPci_PerformanceMonitorControl(
    PLX_DEVICE_OBJECT *pDevice,
    PLX_PERF_CMD       command
    )
{
    PLX_PARAMS IoBuffer;


    if (pDevice == NULL)
        return ApiNullParam;

    // Verify device object
    if (!IsObjectValid(pDevice))
        return ApiInvalidDeviceInfo;

    RtlZeroMemory( &IoBuffer, sizeof(PLX_PARAMS) );

    IoBuffer.Key      = pDevice->Key;
    IoBuffer.value[0] = command;

    PlxIoMessage(
        pDevice,
        PLX_IOCTL_PERFORMANCE_MONITOR_CTRL,
        &IoBuffer
        );

    return IoBuffer.ReturnCode;
}




/******************************************************************************
 *
 * Function   :  PlxPci_PerformanceResetCounters
 *
 * Description:  Resets the performance counters for selected ports
 *
 *****************************************************************************/
PLX_STATUS
PlxPci_PerformanceResetCounters(
    PLX_DEVICE_OBJECT *pDevice,
    PLX_PERF_PROP     *pPerfProps,
    U8                 NumOfObjects
    )
{
    U8         i;
    PLX_PARAMS IoBuffer;


    if ((pDevice == NULL) || (pPerfProps == NULL))
        return ApiNullParam;

    // Verify device object
    if (!IsObjectValid(pDevice))
        return ApiInvalidDeviceInfo;

    // Make sure each Performance object is valid
    for (i=0; i<NumOfObjects; i++)
    {
        if (!IsObjectValid(&pPerfProps[i]))
            return ApiInvalidDeviceInfo;

        // Clear all current and previous counter values (14 counters)
        RtlZeroMemory(
            &pPerfProps[i].IngressPostedHeader,
            2 * (14 * sizeof(U32))
            );
    }

    RtlZeroMemory( &IoBuffer, sizeof(PLX_PARAMS) );

    // Send message to reset counters in device
    IoBuffer.Key = pDevice->Key;

    PlxIoMessage(
        pDevice,
        PLX_IOCTL_PERFORMANCE_RESET_COUNTERS,
        &IoBuffer
        );

    return IoBuffer.ReturnCode;
}




/******************************************************************************
 *
 * Function   :  PlxPci_PerformanceGetCounters
 *
 * Description:  Retrieves a snapshot of the performance counters for selected ports
 *
 *****************************************************************************/
PLX_STATUS
PlxPci_PerformanceGetCounters(
    PLX_DEVICE_OBJECT *pDevice,
    PLX_PERF_PROP     *pPerfProps,
    U8                 NumOfObjects
    )
{
    U8         i;
    PLX_PARAMS IoBuffer;


    if ((pDevice == NULL) || (pPerfProps == NULL))
        return ApiNullParam;

    // Verify device object
    if (!IsObjectValid(pDevice))
        return ApiInvalidDeviceInfo;

    // Make sure each Performance object is valid
    for (i=0; i<NumOfObjects; i++)
    {
        if (!IsObjectValid(&pPerfProps[i]))
            return ApiInvalidDeviceInfo;
    }

    RtlZeroMemory( &IoBuffer, sizeof(PLX_PARAMS) );

    IoBuffer.Key      = pDevice->Key;
    IoBuffer.value[0] = (PLX_UINT_PTR)pPerfProps;
    IoBuffer.value[1] = NumOfObjects;

    PlxIoMessage(
        pDevice,
        PLX_IOCTL_PERFORMANCE_GET_COUNTERS,
        &IoBuffer
        );

    return IoBuffer.ReturnCode;
}




/******************************************************************************
 *
 * Function   :  PlxPci_PerformanceCalcStatistics
 *
 * Description:  Calculate performance statistics for a device
 *
 *****************************************************************************/
PLX_STATUS
PlxPci_PerformanceCalcStatistics(
    PLX_PERF_PROP  *pPerfProp,
    PLX_PERF_STATS *pPerfStats,
    U32             ElapsedTime_ms
    )
{
    S64 TotalBytes;
    S64 MaxLinkRate;
    S64 PayloadAvg;
    S64 Counter_PostedHeader;
    S64 Counter_PostedDW;
    S64 Counter_NonpostedDW;
    S64 Counter_CplHeader;
    S64 Counter_CplDW;
    S64 Counter_Dllp;


    // Verify elapsed time
    if (ElapsedTime_ms == 0)
    {
        RtlZeroMemory( pPerfStats, sizeof(PLX_PERF_STATS) );
        return ApiInvalidData;
    }

    // Determine theoretical max link rate for 1 second
    if (pPerfProp->LinkSpeed == PLX_PCIE_GEN_3_0)
        MaxLinkRate = (S64)(PERF_MAX_BPS_GEN_3_0 * pPerfProp->LinkWidth);
    else
        MaxLinkRate = (S64)(PERF_MAX_BPS_GEN_1_0 * pPerfProp->LinkWidth * pPerfProp->LinkSpeed);

    // Adjust rate for specified elapsed period (ms)
    MaxLinkRate = (MaxLinkRate * ElapsedTime_ms) / 1000;

    //
    // Calculate Ingress actual counters, adjusting for counter wrapping
    //
    Counter_PostedHeader = pPerfProp->IngressPostedHeader;
    Counter_PostedDW     = pPerfProp->IngressPostedDW;
    Counter_NonpostedDW  = pPerfProp->IngressNonpostedDW;
    Counter_CplHeader    = pPerfProp->IngressCplHeader;
    Counter_CplDW        = pPerfProp->IngressCplDW;
    Counter_Dllp         = pPerfProp->IngressDllp;

    // Add 4GB in case counter wrapped
    if (Counter_PostedHeader < pPerfProp->Prev_IngressPostedHeader)
        Counter_PostedHeader += ((S64)1 << 32);

    if (Counter_PostedDW < pPerfProp->Prev_IngressPostedDW)
        Counter_PostedDW += ((S64)1 << 32);

    if (Counter_NonpostedDW < pPerfProp->Prev_IngressNonpostedDW)
        Counter_NonpostedDW += ((S64)1 << 32);

    if (Counter_CplHeader < pPerfProp->Prev_IngressCplHeader)
        Counter_CplHeader += ((S64)1 << 32);

    if (Counter_CplDW < pPerfProp->Prev_IngressCplDW)
        Counter_CplDW += ((S64)1 << 32);

    if (Counter_Dllp < pPerfProp->Prev_IngressDllp)
        Counter_Dllp += ((S64)1 << 32);

    // Determine counter differences
    Counter_PostedHeader = Counter_PostedHeader - pPerfProp->Prev_IngressPostedHeader;
    Counter_PostedDW     = Counter_PostedDW     - pPerfProp->Prev_IngressPostedDW;
    Counter_NonpostedDW  = Counter_NonpostedDW  - pPerfProp->Prev_IngressNonpostedDW;
    Counter_CplHeader    = Counter_CplHeader    - pPerfProp->Prev_IngressCplHeader;
    Counter_CplDW        = Counter_CplDW        - pPerfProp->Prev_IngressCplDW;
    Counter_Dllp         = Counter_Dllp         - pPerfProp->Prev_IngressDllp;


    //
    // Calculate Ingress statistics
    //

    /*************************************************************************
     * Periodically, the chip counters report less TLP Posted DW than expected
     * in comparison to the total number of TLP Posted Headers.  We need an
     * error check for this, otherwise the Posted Payload becomes incorrect.
     * The fix involves changing the posted header count based on the number of
     * posted DW, assuming a 4 byte Payload.
     ************************************************************************/
    if ((Counter_PostedHeader * PERF_TLP_DW) > Counter_PostedDW)
    {
        Counter_PostedHeader = Counter_PostedDW / (PERF_TLP_DW + 1);
    }

    // Posted Payload bytes ((P_DW * size(DW) - (P_TLP * size(P_TLP))
    pPerfStats->IngressPayloadWriteBytes =
        (Counter_PostedDW * sizeof(U32)) -
        (Counter_PostedHeader * PERF_TLP_SIZE);

    // Completion Payload ((CPL_DW * size(DW) - (CPL_TLP * size(TLP))
    pPerfStats->IngressPayloadReadBytes =
        (Counter_CplDW * sizeof(U32)) -
        (Counter_CplHeader * PERF_TLP_SIZE);

    // Total payload
    pPerfStats->IngressPayloadTotalBytes =
        pPerfStats->IngressPayloadWriteBytes + pPerfStats->IngressPayloadReadBytes;

    // Average payload size (Payload / (P_TLP + CPL_TLP))
    PayloadAvg = Counter_PostedHeader + Counter_CplHeader;

    if (PayloadAvg != 0)
        pPerfStats->IngressPayloadAvgPerTlp = (long double)pPerfStats->IngressPayloadTotalBytes / PayloadAvg;
    else
        pPerfStats->IngressPayloadAvgPerTlp = 0;

    // Total number of TLP data ((P_DW + NP_DW + CPL_DW) * size(DW))
    TotalBytes = (Counter_PostedDW    +
                  Counter_NonpostedDW +
                  Counter_CplDW) * sizeof(U32);

    // Add DLLPs to total bytes
    TotalBytes += (Counter_Dllp * PERF_DLLP_SIZE);

    // Total bytes
    pPerfStats->IngressTotalBytes = TotalBytes;

    // Total byte rate
    pPerfStats->IngressTotalByteRate = (long double)((TotalBytes * 1000) / ElapsedTime_ms);

    // Payload rate
    pPerfStats->IngressPayloadByteRate =
        ((long double)pPerfStats->IngressPayloadTotalBytes * 1000) / ElapsedTime_ms;

    // Link Utilization
    if (MaxLinkRate == 0)
    {
        pPerfStats->IngressLinkUtilization = 0;
    }
    else
    {
        pPerfStats->IngressLinkUtilization = ((long double)TotalBytes * 100) / MaxLinkRate;

        // Account for error margin
        if (pPerfStats->IngressLinkUtilization > (double)100)
            pPerfStats->IngressLinkUtilization = 100;
    }


    //
    // Calculate Egress actual counters, adjusting for counter wrapping
    //
    Counter_PostedHeader = pPerfProp->EgressPostedHeader;
    Counter_PostedDW     = pPerfProp->EgressPostedDW;
    Counter_NonpostedDW  = pPerfProp->EgressNonpostedDW;
    Counter_CplHeader    = pPerfProp->EgressCplHeader;
    Counter_CplDW        = pPerfProp->EgressCplDW;
    Counter_Dllp         = pPerfProp->EgressDllp;

    // Add 4GB in case counter wrapped
    if (Counter_PostedHeader < pPerfProp->Prev_EgressPostedHeader)
        Counter_PostedHeader += ((S64)1 << 32);

    if (Counter_PostedDW < pPerfProp->Prev_EgressPostedDW)
        Counter_PostedDW += ((S64)1 << 32);

    if (Counter_NonpostedDW < pPerfProp->Prev_EgressNonpostedDW)
        Counter_NonpostedDW += ((S64)1 << 32);

    if (Counter_CplHeader < pPerfProp->Prev_EgressCplHeader)
        Counter_CplHeader += ((S64)1 << 32);

    if (Counter_CplDW < pPerfProp->Prev_EgressCplDW)
        Counter_CplDW += ((S64)1 << 32);

    if (Counter_Dllp < pPerfProp->Prev_EgressDllp)
        Counter_Dllp += ((S64)1 << 32);

    // Determine counter differences
    Counter_PostedHeader = Counter_PostedHeader - pPerfProp->Prev_EgressPostedHeader;
    Counter_PostedDW     = Counter_PostedDW     - pPerfProp->Prev_EgressPostedDW;
    Counter_NonpostedDW  = Counter_NonpostedDW  - pPerfProp->Prev_EgressNonpostedDW;
    Counter_CplHeader    = Counter_CplHeader    - pPerfProp->Prev_EgressCplHeader;
    Counter_CplDW        = Counter_CplDW        - pPerfProp->Prev_EgressCplDW;
    Counter_Dllp         = Counter_Dllp         - pPerfProp->Prev_EgressDllp;


    //
    // Calculate Egress statistics
    //

    /*************************************************************************
     * Refer to comment above for handling Ingress posted header count issue.
     ************************************************************************/
    if ((Counter_PostedHeader * PERF_TLP_DW) > Counter_PostedDW)
    {
        Counter_PostedHeader = Counter_PostedDW / (PERF_TLP_DW + 1);
    }

    // Posted Payload bytes ((P_DW * size(DW) - (P_TLP * size(P_TLP))
    pPerfStats->EgressPayloadWriteBytes =
        (Counter_PostedDW * sizeof(U32)) -
        (Counter_PostedHeader * PERF_TLP_SIZE);

    // Completion Payload ((CPL_DW * size(DW) - (CPL_TLP * size(CPL_TLP))
    pPerfStats->EgressPayloadReadBytes =
        (Counter_CplDW * sizeof(U32)) -
        (Counter_CplHeader * PERF_TLP_SIZE);

    // Total payload
    pPerfStats->EgressPayloadTotalBytes =
        pPerfStats->EgressPayloadWriteBytes + pPerfStats->EgressPayloadReadBytes;

    // Average payload size (Payload / (P_TLP + CPL_TLP))
    PayloadAvg = Counter_PostedHeader + Counter_CplHeader;

    if (PayloadAvg != 0)
        pPerfStats->EgressPayloadAvgPerTlp = (long double)pPerfStats->EgressPayloadTotalBytes / PayloadAvg;
    else
        pPerfStats->EgressPayloadAvgPerTlp = 0;

    // Total number of TLP data ((P_DW + NP_DW + CPL_DW) * size(DW))
    TotalBytes = (Counter_PostedDW    +
                  Counter_NonpostedDW +
                  Counter_CplDW) * sizeof(U32);

    // Add DLLPs to total bytes
    TotalBytes += (Counter_Dllp * PERF_DLLP_SIZE);

    // Total bytes
    pPerfStats->EgressTotalBytes = TotalBytes;

    // Total byte rate
    pPerfStats->EgressTotalByteRate = ((long double)TotalBytes * 1000) / ElapsedTime_ms;

    // Payload rate
    pPerfStats->EgressPayloadByteRate =
        ((long double)pPerfStats->EgressPayloadTotalBytes * 1000) / ElapsedTime_ms;

    // Link Utilization
    if (MaxLinkRate == 0)
    {
        pPerfStats->EgressLinkUtilization = 0;
    }
    else
    {
        pPerfStats->EgressLinkUtilization = ((long double)TotalBytes * 100) / MaxLinkRate;

        // Account for error margin
        if (pPerfStats->EgressLinkUtilization > (double)100)
            pPerfStats->EgressLinkUtilization = 100;
    }

    return ApiSuccess;
}




/******************************************************************************
 *
 * Function   :  PlxPci_MH_GetProperties
 *
 * Description:  Returns the properties of a Multi-Host switch
 *
 *****************************************************************************/
PLX_STATUS
PlxPci_MH_GetProperties(
    PLX_DEVICE_OBJECT   *pDevice,
    PLX_MULTI_HOST_PROP *pMHProp
    )
{
    PLX_PARAMS IoBuffer;


    if ((pDevice == NULL) || (pMHProp == NULL))
        return ApiNullParam;

    // Verify device object
    if (!IsObjectValid(pDevice))
        return ApiInvalidDeviceInfo;

    RtlZeroMemory( &IoBuffer, sizeof(PLX_PARAMS) );

    IoBuffer.Key = pDevice->Key;

    PlxIoMessage(
        pDevice,
        PLX_IOCTL_MH_GET_PROPERTIES,
        &IoBuffer
        );

    if (IoBuffer.ReturnCode == ApiSuccess)
    {
        // Return properties
        *pMHProp = IoBuffer.u.MH_Prop;
    }
    else
    {
        // Set default value for properties
        RtlZeroMemory( pMHProp, sizeof(PLX_MULTI_HOST_PROP) );
    }

    return IoBuffer.ReturnCode;
}




/******************************************************************************
 *
 * Function   :  PlxPci_MH_MigratePorts
 *
 * Description:  Migrates one or more downstream ports from one VS to another
 *
 *****************************************************************************/
PLX_STATUS
PlxPci_MH_MigratePorts(
    PLX_DEVICE_OBJECT *pDevice,
    U16                VS_Source,
    U16                VS_Dest,
    U32                DsPortMask,
    BOOLEAN            bResetSrc
    )
{
    PLX_PARAMS IoBuffer;


    if (pDevice == NULL)
        return ApiNullParam;

    // Verify device object
    if (!IsObjectValid(pDevice))
        return ApiInvalidDeviceInfo;

    RtlZeroMemory( &IoBuffer, sizeof(PLX_PARAMS) );

    IoBuffer.Key      = pDevice->Key;
    IoBuffer.value[0] = ((U32)VS_Source << 16) | VS_Dest;
    IoBuffer.value[1] = DsPortMask;
    IoBuffer.value[2] = bResetSrc;

    PlxIoMessage(
        pDevice,
        PLX_IOCTL_MH_MIGRATE_DS_PORTS,
        &IoBuffer
        );

    return IoBuffer.ReturnCode;
}




/******************************************************************************
 *
 * Function   :  PlxPci_Nt_ReqIdProbe
 *
 * Description:  Returns the Host PCIe ReqID when accessing PLX NT port
 *
 *****************************************************************************/
PLX_STATUS
PlxPci_Nt_ReqIdProbe(
    PLX_DEVICE_OBJECT *pDevice,
    BOOLEAN            bRead,
    U16               *pReqId
    )
{
    PLX_PARAMS IoBuffer;


    if ((pDevice == NULL) || (pReqId == NULL))
        return ApiNullParam;

    // Verify device object
    if (!IsObjectValid(pDevice))
        return ApiInvalidDeviceInfo;

    RtlZeroMemory( &IoBuffer, sizeof(PLX_PARAMS) );

    IoBuffer.Key      = pDevice->Key;
    IoBuffer.value[0] = bRead;

    PlxIoMessage(
        pDevice,
        PLX_IOCTL_NT_PROBE_REQ_ID,
        &IoBuffer
        );

    // Return ReqID
    if (IoBuffer.ReturnCode == ApiSuccess)
        *pReqId = (U16)IoBuffer.value[1];

    return IoBuffer.ReturnCode;
}




/******************************************************************************
 *
 * Function   :  PlxPci_Nt_LutProperties
 *
 * Description:  Returns the properties for the specified NT LUT entry
 *
 *****************************************************************************/
PLX_STATUS
PlxPci_Nt_LutProperties(
    PLX_DEVICE_OBJECT *pDevice,
    U16                LutIndex,
    U16               *pReqId,
    U32               *pFlags,
    BOOLEAN           *pbEnabled
    )
{
    PLX_PARAMS IoBuffer;


    if (pDevice == NULL)
        return ApiNullParam;

    // Verify device object
    if (!IsObjectValid(pDevice))
        return ApiInvalidDeviceInfo;

    RtlZeroMemory( &IoBuffer, sizeof(PLX_PARAMS) );

    IoBuffer.Key      = pDevice->Key;
    IoBuffer.value[0] = LutIndex;

    PlxIoMessage(
        pDevice,
        PLX_IOCTL_NT_LUT_PROPERTIES,
        &IoBuffer
        );

    // Return LUT entry properties requested
    if (IoBuffer.ReturnCode == ApiSuccess)
    {
        if (pReqId != NULL)
            *pReqId = (U16)IoBuffer.value[0];

        if (pFlags != NULL)
            *pFlags = (U32)IoBuffer.value[1];

        if (pbEnabled != NULL)
            *pbEnabled = (BOOLEAN)IoBuffer.value[2];
    }
    else
    {
        if (pReqId != NULL)
            *pReqId = 0;

        if (pFlags != NULL)
            *pFlags = PLX_NT_LUT_FLAG_NONE;

        if (pbEnabled != NULL)
            *pbEnabled = FALSE;
    }

    return IoBuffer.ReturnCode;
}




/******************************************************************************
 *
 * Function   :  PlxPci_Nt_LutAdd
 *
 * Description:  Adds a Requester ID to the NT LUT
 *
 *****************************************************************************/
PLX_STATUS
PlxPci_Nt_LutAdd(
    PLX_DEVICE_OBJECT *pDevice,
    U16               *pLutIndex,
    U16                ReqId,
    U32                flags
    )
{
    PLX_PARAMS IoBuffer;


    if (pDevice == NULL)
        return ApiNullParam;

    // Verify device object
    if (!IsObjectValid(pDevice))
        return ApiInvalidDeviceInfo;

    RtlZeroMemory( &IoBuffer, sizeof(PLX_PARAMS) );

    IoBuffer.Key      = pDevice->Key;
    IoBuffer.value[1] = ReqId;
    IoBuffer.value[2] = flags;

    if (pLutIndex == NULL)
        IoBuffer.value[0] = (U16)-1;    // Default to auto-select
    else
        IoBuffer.value[0] = *pLutIndex;

    PlxIoMessage(
        pDevice,
        PLX_IOCTL_NT_LUT_ADD,
        &IoBuffer
        );

    // Return assigned LUT index if requested
    if ((IoBuffer.ReturnCode == ApiSuccess) && (pLutIndex != NULL))
        *pLutIndex = (U16)IoBuffer.value[0];

    return IoBuffer.ReturnCode;
}




/******************************************************************************
 *
 * Function   :  PlxPci_Nt_LutDisable
 *
 * Description:  Disables the specified NT LUT entry
 *
 *****************************************************************************/
PLX_STATUS
PlxPci_Nt_LutDisable(
    PLX_DEVICE_OBJECT *pDevice,
    U16                LutIndex
    )
{
    PLX_PARAMS IoBuffer;


    if (pDevice == NULL)
        return ApiNullParam;

    // Verify device object
    if (!IsObjectValid(pDevice))
        return ApiInvalidDeviceInfo;

    RtlZeroMemory( &IoBuffer, sizeof(PLX_PARAMS) );

    IoBuffer.Key      = pDevice->Key;
    IoBuffer.value[0] = LutIndex;

    PlxIoMessage(
        pDevice,
        PLX_IOCTL_NT_LUT_DISABLE,
        &IoBuffer
        );

    return IoBuffer.ReturnCode;
}




/***********************************************************
*
*                  PRIVATE FUNCTIONS
*
***********************************************************/


/******************************************************************************
 *
 * Function   :  Driver_Connect
 *
 * Description:  Attempts to connect a registered driver by link name
 *
 * Returns    :  TRUE   - Driver was found and connected to
 *               FALSE  - Driver not found
 *
 *****************************************************************************/
BOOLEAN
Driver_Connect(
    PLX_DRIVER_HANDLE *pHandle,
    U8                 ApiIndex,
    U16                DeviceNumber
    )
{
    char Extension[10];
    char DriverName[30];


    // Build driver name
    sprintf(
        DriverName,
        "%s%s",
        DRIVER_PATH,
        PlxDrivers[ApiIndex]
        );

#if defined(PLX_MSWINDOWS)
    // For Windows, add SDK version to avoid conflicts with other releases
    sprintf(
        Extension,
        "_v%d%02d",
        PLX_SDK_VERSION_MAJOR,
        PLX_SDK_VERSION_MINOR
        );

    strcat(
        DriverName,
        Extension
        );
#endif

    // Add proper extension to link name
    if (strstr(
            DriverName,
            PLX_SVC_DRIVER_NAME
            ) == NULL)
    {
        // Add device number to link name
        sprintf(
            Extension,
            "-%d",
            DeviceNumber
            );

        strcat(
            DriverName,
            Extension
            );
    }

#if defined(PLX_MSWINDOWS)

    // Open the device
    *pHandle =
        CreateFile(
            DriverName,                          // File name
            GENERIC_READ | GENERIC_WRITE,        // Desired access
            FILE_SHARE_READ | FILE_SHARE_WRITE,  // Shared mode
            NULL,                                // Security attributes
            OPEN_EXISTING,                       // Creation disposition
            FILE_ATTRIBUTE_NORMAL,               // File attributes
            NULL                                 // Template file
            );

    if (*pHandle == INVALID_HANDLE_VALUE)
    {
        return FALSE;
    }

#elif defined(PLX_LINUX)

    // Open the device
    *pHandle =
        open(
            DriverName,
            O_RDWR
            );

    if (*pHandle < 0)
    {
        return FALSE;
    }

#elif defined(PLX_DOS)

    // Default to invalid handle
    *pHandle = INVALID_HANDLE_VALUE;

    // Only support the PLX Service
    if (strstr(
            DriverName,
            PLX_SVC_DRIVER_NAME
            ) == NULL)
    {
        return FALSE;
    }

    // Get a handle to the driver
    if (PlxSvc_DriverEntry( pHandle ) == FALSE)
        return FALSE;

    // Open the device
    if (Dispatch_Create( *pHandle ) == FALSE)
        return FALSE;

#endif

    return TRUE;
}




/******************************************************************************
 *
 * Function   :  PlxIoMessage
 *
 * Description:  Sends a message to the selected PLX driver
 *
 *****************************************************************************/
static S32
PlxIoMessage(
    PLX_DEVICE_OBJECT *pDevice,
    U32                IoControlCode,
    VOID              *pBuffer
    )
{
    S32 status;
#if defined(PLX_MSWINDOWS) && !defined(PLX_DEMO_API)
    U32 BytesReturned;
#endif


    // Send API message depending upon device connection
    if (pDevice->Key.ApiMode == PLX_API_MODE_PCI)
    {
#if defined(PLX_MSWINDOWS)
    #if defined(PLX_DEMO_API)
        status =
            PlxDemo_Dispatch_IoControl(
                pDevice,                // Device
                IoControlCode,          // Control code
                pBuffer,                // Pointer to buffer
                sizeof(PLX_PARAMS)      // Size of buffer
                );
    #else
        status =
            DeviceIoControl(
                pDevice->hDevice,       // Driver handle
                IoControlCode,          // Control code
                pBuffer,                // Pointer to input buffer
                sizeof(PLX_PARAMS),     // Size of input buffer
                pBuffer,                // Pointer to output buffer
                sizeof(PLX_PARAMS),     // Size of output buffer
                &BytesReturned,         // Required when lpOverlapped is NULL
                NULL                    // Pointer to OVERLAPPED struct for asynchronous I/O
                );
    #endif

#elif defined(PLX_LINUX)
        status =
            ioctl(
                (int)pDevice->hDevice,  // Driver handle
                IoControlCode,          // Control code
                pBuffer                 // Pointer to buffer
                );

#elif defined(PLX_DOS)
        status =
            Dispatch_IoControl(
                pDevice->hDevice,       // Driver handle
                IoControlCode,          // Control code
                pBuffer,                // Pointer to buffer
                sizeof(PLX_PARAMS)      // Size of buffer
                );
#endif
    }
    else if (pDevice->Key.ApiMode == PLX_API_MODE_I2C_AARDVARK)
    {
        status =
            PlxI2c_Dispatch_IoControl(
                pDevice,                // Device
                IoControlCode,          // Control code
                pBuffer,                // Pointer to buffer
                sizeof(PLX_PARAMS)      // Size of buffer
                );
    }
    else
    {
        return -1;
    }

    return status;
}




/******************************************************************************
 *
 * Function   :  PlxApi_DebugPrintf
 *
 * Description:  Logs printf() style data to a file for debug purposes
 *
 *****************************************************************************/
void
PlxApi_DebugPrintf(
    const char *format,
    ...
    )
{
    char     pOut[300];
    va_list  pArgs;
 #if defined(PLX_DBG_DEST_FILE)
    FILE    *fp;
 #endif


    // Initialize the optional arguments pointer
    va_start(pArgs, format);

    // Build string to write
    vsprintf(pOut, format, pArgs);

    // Terminate arguments pointer
    va_end(pArgs);

 #if defined(PLX_DBG_DEST_FILE)
    // Open the file for appending output
    fp =
        fopen(
            PLX_LOG_FILE,
            "a+"
            );

    if (fp == NULL)
        return;

    // Write string to file
    fputs( pOut, fp );

    // Close the file
    fclose(fp);

 #else

    _PlxDbgOut( pOut );

 #endif
}
