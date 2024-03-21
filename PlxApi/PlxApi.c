/*******************************************************************************
 * Copyright 2013-2022 Avago Technologies
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
 *     PlxApi.c
 *
 * Description:
 *
 *     This file contains all the PLX API functions
 *
 * Revision:
 *
 *     09-01-19: PCI/PCIe SDK v8.10
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
    #define _CRT_NONSTDC_NO_WARNINGS        // Prevents POSIX function warnings
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

#include <stdarg.h>         // For va_start/va_end
#include <math.h>           // For pow()
#include "PciRegs.h"
#include "PlxApi.h"
#include "PlxApiDebug.h"
#include "PlxApiDirect.h"
#include "PlxIoctl.h"
#include "I2cAaUsb.h"
#include "MdioSpliceUsb.h"
#include "SdbComPort.h"




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

    PLX_STATUS MdioSplice_DeviceOpen(PLX_DEVICE_OBJECT *pDev)
    {
        return PLX_STATUS_UNSUPPORTED;
    }

    PLX_STATUS MdioSplice_DeviceClose(PLX_DEVICE_OBJECT *pDev)
    {
        return PLX_STATUS_UNSUPPORTED;
    }

    PLX_STATUS MdioSplice_DeviceFindEx(
        PLX_DEVICE_KEY *pKey,
        U16             DeviceNumber,
        PLX_MODE_PROP  *pModeProp
        )
    {
        return PLX_STATUS_UNSUPPORTED;
    }

    S32 MdioSplice_Dispatch_IoControl(
        PLX_DEVICE_OBJECT *pDevice,
        U32                IoControlCode,
        PLX_PARAMS        *pIoBuffer,
        U32                Size
        )
    {
        return 0;
    }

    PLX_STATUS Sdb_DeviceOpen(PLX_DEVICE_OBJECT *pDev)
    {
        return PLX_STATUS_UNSUPPORTED;
    }

    PLX_STATUS Sdb_DeviceClose(PLX_DEVICE_OBJECT *pDev)
    {
        return PLX_STATUS_UNSUPPORTED;
    }

    PLX_STATUS Sdb_DeviceFindEx(
        PLX_DEVICE_KEY *pKey,
        U16             DeviceNumber,
        PLX_MODE_PROP  *pModeProp
        )
    {
        return PLX_STATUS_UNSUPPORTED;
    }

    S32 Sdb_Dispatch_IoControl(
        PLX_DEVICE_OBJECT *pDevice,
        U32                IoControlCode,
        PLX_PARAMS        *pIoBuffer,
        U32                Size
        )
    {
        return 0;
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
    "Plx_Mgr",
    "Plx_SVF",
    PLX_SVC_DRIVER_NAME,    // PLX PCI service must be last driver
    "0"                     // Must be last item to mark end of list
};




/**********************************************
 *       Private Function Prototypes
 *********************************************/
static PLX_STATUS
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
    PLX_STATUS status;


    if ((pDevice == NULL) || (pKey == NULL))
    {
        return PLX_STATUS_NULL_PARAM;
    }

    // Make sure device object is not already in use
    if (IsObjectValid(pDevice))
    {
        return PLX_STATUS_INVALID_OBJECT;
    }

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
        else if (pKey->ApiMode == PLX_API_MODE_MDIO_SPLICE)
        {
            return MdioSplice_DeviceOpen( pDevice );
        }
        else if (pKey->ApiMode == PLX_API_MODE_SDB)
        {
            return Sdb_DeviceOpen( pDevice );
        }
    }
    else
    {
        // Default to PCI mode & fill in missing key information
        status =
            PlxPci_DeviceFindEx(
                &(pDevice->Key),
                0,                      // First matching device,
                PLX_API_MODE_PCI,
                NULL                    // Mode properties ignored in PCI mode
                );

        if (status != PLX_STATUS_OK)
        {
            return status;
        }
    }

    // Connect to driver
    status =
        Driver_Connect(
            &pDevice->hDevice,
            pDevice->Key.ApiIndex,
            pDevice->Key.DeviceNumber
            );

    if (status != PLX_STATUS_OK)
    {
        return status;
    }

    // Mark object as valid
    ObjectValidate( pDevice );

    return PLX_STATUS_OK;
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
    {
        return PLX_STATUS_NULL_PARAM;
    }

    // Verify device object
    if (!IsObjectValid(pDevice))
    {
        return PLX_STATUS_INVALID_OBJECT;
    }

    // Check for non-PCI mode
    if (pDevice->Key.ApiMode == PLX_API_MODE_I2C_AARDVARK)
    {
        PlxI2c_DeviceClose( pDevice );
    }
    else if (pDevice->Key.ApiMode == PLX_API_MODE_MDIO_SPLICE)
    {
        MdioSplice_DeviceClose( pDevice );
    }
    else if (pDevice->Key.ApiMode == PLX_API_MODE_SDB)
    {
        Sdb_DeviceClose( pDevice );
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

    return PLX_STATUS_OK;
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
    BOOLEAN           bDriverOpened;
    PLX_STATUS        status;
    PLX_PARAMS        IoBuffer;
    PLX_DEVICE_OBJECT Device;


    // Verify parameter
    if (pKey == NULL)
    {
        return PLX_STATUS_NULL_PARAM;
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
        status =
            Driver_Connect(
                &(Device.hDevice),
                i,                  // Driver index
                0                   // Device index in driver
                );

        if (status == PLX_STATUS_OK)
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
            if (IoBuffer.ReturnCode == PLX_STATUS_OK)
            {
                // Copy device key information
                *pKey = IoBuffer.Key;

                // Store driver name index
                pKey->ApiIndex = (U8)i;

                // Validate key
                ObjectValidate( pKey );

                return PLX_STATUS_OK;
            }

            // Add number of matches to total
            TotalMatches += (U16)IoBuffer.value[0];
        }

        // Increment to next driver
        i++;
    }

    if (bDriverOpened == FALSE)
    {
        return PLX_STATUS_NO_DRIVER;
    }

    return PLX_STATUS_INVALID_OBJECT;
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

    // Access through Splice MDIO USB
    if (ApiMode == PLX_API_MODE_MDIO_SPLICE)
    {
        return MdioSplice_DeviceFindEx(
            pKey,
            DeviceNumber,
            pModeProp
            );
    }

    // Access through COM/TTY port to SDB port
    if (ApiMode == PLX_API_MODE_SDB)
    {
        return Sdb_DeviceFindEx(
            pKey,
            DeviceNumber,
            pModeProp
            );
    }

    if (ApiMode == PLX_API_MODE_TCP)
    {
        // Not yet supported
        return PLX_STATUS_UNSUPPORTED;
    }

    return PLX_STATUS_INVALID_ACCESS;
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
    {
        return PLX_STATUS_INVALID_ACCESS;
    }

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
        return PLX_STATUS_NULL_PARAM;
    }

    *pVersionMajor    = PLX_SDK_VERSION_MAJOR;
    *pVersionMinor    = PLX_SDK_VERSION_MINOR / 10;
    *pVersionRevision = PLX_SDK_VERSION_MINOR - (*pVersionMinor * 10);

    return PLX_STATUS_OK;
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
        return PLX_STATUS_NULL_PARAM;
    }

    // Clear version information in case of error
    *pVersionMajor    = 0;
    *pVersionMinor    = 0;
    *pVersionRevision = 0;

    // Verify device object
    if (!IsObjectValid(pDevice))
    {
        return PLX_STATUS_INVALID_OBJECT;
    }

    RtlZeroMemory( &IoBuffer, sizeof(PLX_PARAMS) );

    PlxIoMessage(
        pDevice,
        PLX_IOCTL_DRIVER_VERSION,
        &IoBuffer
        );

    *pVersionMajor    = (U8)((IoBuffer.value[0] >> 16) & 0xFF);
    *pVersionMinor    = (U8)((IoBuffer.value[0] >>  8) & 0xFF) / 10;
    *pVersionRevision = (U8)((IoBuffer.value[0] >>  8) & 0xFF) - (*pVersionMinor * 10);

    return PLX_STATUS_OK;
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
    {
        return PLX_STATUS_NULL_PARAM;
    }

    // Verify device object
    if (!IsObjectValid(pDevice))
    {
        return PLX_STATUS_INVALID_OBJECT;
    }

    RtlZeroMemory( &IoBuffer, sizeof(PLX_PARAMS) );

#if 0
// DBG - Driver message to get properties not supported yet
    PlxIoMessage(
        pDevice,
        PLX_IOCTL_DRIVER_PROPERTIES,
        &IoBuffer
        );

    if (IoBuffer.ReturnCode == PLX_STATUS_OK)
    {
        *pDriverProp = IoBuffer.u.DriverProp;
    }
#else
    // Bypass for now
    IoBuffer.ReturnCode = PLX_STATUS_OK;

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
        strcpy( pDriverProp->Name, "I2cAardvark" );
    }
    else if (pDevice->Key.ApiMode == PLX_API_MODE_MDIO_SPLICE)
    {
        // Fill in properties for Splice MDIO
        pDriverProp->bIsServiceDriver = TRUE;
        strcpy( pDriverProp->Name, "MdioSpliceUsb" );
    }
    else if (pDevice->Key.ApiMode == PLX_API_MODE_SDB)
    {
        // Fill in properties for Splice MDIO
        pDriverProp->bIsServiceDriver = TRUE;
        strcpy( pDriverProp->Name, "SdbComPort" );
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
    {
        return PLX_STATUS_INVALID_OBJECT;
    }

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


    if ((pChipType == NULL) || (pRevision == NULL))
    {
        return PLX_STATUS_NULL_PARAM;
    }

    // Verify device object
    if (!IsObjectValid(pDevice))
    {
        return PLX_STATUS_INVALID_OBJECT;
    }

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

    return PLX_STATUS_OK;
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
    {
        return PLX_STATUS_INVALID_OBJECT;
    }

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
    if (IoBuffer.ReturnCode == PLX_STATUS_OK)
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
 * Description:  Returns the chip port mask
 *
 *****************************************************************************/
PLX_STATUS
PlxPci_ChipGetPortMask(
    U32            ChipID,
    U8             Revision,
    PEX_CHIP_FEAT *PtrFeat
    )
{
    if (PtrFeat == NULL)
    {
        return PLX_STATUS_NULL_PARAM;
    }

    // Reset properties
    RtlZeroMemory( PtrFeat, sizeof(PEX_CHIP_FEAT) );

    // Fill in per-switch properties
    switch (ChipID)
    {
        case 0x2380:
        case 0x3380:
        case 0x3382:
            PtrFeat->StnCount    = 1;
            PtrFeat->PortsPerStn = 4;
            PtrFeat->StnMask     = 0x01;    // 0-2,PCEI2USB,USB
            PEX_BITMASK_SET_DW( PtrFeat->PortMask, 0, 0x00000007 );
            PEX_BITMASK_SET( PtrFeat->PortMask, PLX_FLAG_PORT_PCIE_TO_USB );
            PEX_BITMASK_SET( PtrFeat->PortMask, PLX_FLAG_PORT_USB );
            break;

        case 0x8505:
            PtrFeat->StnCount    = 1;
            PtrFeat->PortsPerStn = 5;
            PtrFeat->StnMask     = 0x01;    // 0-4
            PEX_BITMASK_SET_DW( PtrFeat->PortMask, 0, 0x0000001F );
            break;

        case 0x8508:
        case 0x8512:
        case 0x8517:
        case 0x8518:
            PtrFeat->StnCount    = 1;
            PtrFeat->PortsPerStn = 5;
            PtrFeat->StnMask     = 0x01;    // 0-4,NT
            PEX_BITMASK_SET_DW( PtrFeat->PortMask, 0, 0x0000001F );
            PEX_BITMASK_SET( PtrFeat->PortMask, PLX_FLAG_PORT_NT_LINK_0 );
            break;

        case 0x8509:
            PtrFeat->StnCount    = 1;
            PtrFeat->PortsPerStn = 8;
            PtrFeat->StnMask     = 0x01;    // 0-7
            PEX_BITMASK_SET_DW( PtrFeat->PortMask, 0, 0x000000FF );
            break;

        case 0x8516:
            PtrFeat->StnCount    = 1;
            PtrFeat->PortsPerStn = 4;
            PtrFeat->StnMask     = 0x01;    // 0-3,NT
            PEX_BITMASK_SET_DW( PtrFeat->PortMask, 0, 0x0000000F );
            PEX_BITMASK_SET( PtrFeat->PortMask, PLX_FLAG_PORT_NT_LINK_0 );
            break;

        case 0x8524:
            PtrFeat->StnCount    = 2;
            PtrFeat->PortsPerStn = 8;
            PtrFeat->StnMask     = 0x03;    // 0,1,8-11,NT
            PEX_BITMASK_SET_DW( PtrFeat->PortMask, 0, 0x00000F03 );
            PEX_BITMASK_SET( PtrFeat->PortMask, PLX_FLAG_PORT_NT_LINK_0 );
            break;

        case 0x8525:
            PtrFeat->StnCount    = 2;
            PtrFeat->PortsPerStn = 8;
            PtrFeat->StnMask     = 0x03;    // 1,2,8-10
            PEX_BITMASK_SET_DW( PtrFeat->PortMask, 0, 0x00000706 );
            break;

        case 0x8532:
            PtrFeat->StnCount    = 2;
            PtrFeat->PortsPerStn = 8;
            PtrFeat->StnMask     = 0x03;    // 0-3,8-11,NT
            PEX_BITMASK_SET_DW( PtrFeat->PortMask, 0, 0x00000F0F );
            PEX_BITMASK_SET( PtrFeat->PortMask, PLX_FLAG_PORT_NT_LINK_0 );
            break;

        case 0x8533:
            PtrFeat->StnCount    = 2;
            PtrFeat->PortsPerStn = 8;
            PtrFeat->StnMask     = 0x03;    // 0-2,8-10
            PEX_BITMASK_SET_DW( PtrFeat->PortMask, 0, 0x00000707 );
            break;

        case 0x8547:
            PtrFeat->StnCount    = 3;
            PtrFeat->PortsPerStn = 4;
            PtrFeat->StnMask     = 0x07;    // 0,8,12
            PEX_BITMASK_SET_DW( PtrFeat->PortMask, 0, 0x00001101 );
            break;

        case 0x8548:
            PtrFeat->StnCount    = 3;
            PtrFeat->PortsPerStn = 4;
            PtrFeat->StnMask     = 0x07;    // 0-2,8-10,12-14
            PEX_BITMASK_SET_DW( PtrFeat->PortMask, 0, 0x00007707 );
            break;

        case 0x8603:
            PtrFeat->StnCount    = 1;
            PtrFeat->PortsPerStn = 4;
            PtrFeat->StnMask     = 0x01;    // 0-2
            PEX_BITMASK_SET_DW( PtrFeat->PortMask, 0, 0x00000007 );
            break;

        case 0x8604:
            PtrFeat->StnCount    = 1;
            PtrFeat->PortsPerStn = 16;
            PtrFeat->StnMask     = 0x01;    // 0,1,4,5,NT,NTB(BA)
            PEX_BITMASK_SET_DW( PtrFeat->PortMask, 0, 0x00000033 );
            PEX_BITMASK_SET( PtrFeat->PortMask, PLX_FLAG_PORT_NT_LINK_0 );
            if (Revision != 0xAA)
            {
                PEX_BITMASK_SET( PtrFeat->PortMask, PLX_FLAG_PORT_NT_DS_P2P );
            }
            break;

        case 0x8605:
            PtrFeat->StnCount    = 1;
            PtrFeat->PortsPerStn = 4;
            PtrFeat->StnMask     = 0x01;    // 0-3
            PEX_BITMASK_SET_DW( PtrFeat->PortMask, 0, 0x0000000F );
            break;

        case 0x8606:
            PtrFeat->StnCount    = 1;
            PtrFeat->PortsPerStn = 16;
            PtrFeat->StnMask     = 0x01;    // 0,1,4,5,7,9,NT,NTB(BA)
            PEX_BITMASK_SET_DW( PtrFeat->PortMask, 0, 0x000002B3 );
            PEX_BITMASK_SET( PtrFeat->PortMask, PLX_FLAG_PORT_NT_LINK_0 );
            if (Revision != 0xAA)
            {
                PEX_BITMASK_SET( PtrFeat->PortMask, PLX_FLAG_PORT_NT_DS_P2P );
            }
            break;

        case 0x8608:
            PtrFeat->StnCount    = 1;
            PtrFeat->PortsPerStn = 16;
            PtrFeat->StnMask     = 0x01;    // 0,1,4-9,NT,NTB(BA)
            PEX_BITMASK_SET_DW( PtrFeat->PortMask, 0, 0x000003F3 );
            PEX_BITMASK_SET( PtrFeat->PortMask, PLX_FLAG_PORT_NT_LINK_0 );
            if (Revision != 0xAA)
            {
                PEX_BITMASK_SET( PtrFeat->PortMask, PLX_FLAG_PORT_NT_DS_P2P );
            }
            break;

        case 0x8609:
            PtrFeat->StnCount    = 1;
            PtrFeat->PortsPerStn = 16;
            PtrFeat->StnMask     = 0x01;    // 0,1,4-9,DMA,NT,NTB
            PEX_BITMASK_SET_DW( PtrFeat->PortMask, 0, 0x000003F3 );
            PEX_BITMASK_SET( PtrFeat->PortMask, PLX_FLAG_PORT_NT_LINK_0 );
            PEX_BITMASK_SET( PtrFeat->PortMask, PLX_FLAG_PORT_NT_DS_P2P );
            PEX_BITMASK_SET( PtrFeat->PortMask, PLX_FLAG_PORT_DMA_0 );
            break;

        case 0x8612:
            PtrFeat->StnCount    = 2;
            PtrFeat->PortsPerStn = 4;
            PtrFeat->StnMask     = 0x03;    // 0,1,4,5,NT
            PEX_BITMASK_SET_DW( PtrFeat->PortMask, 0, 0x00000033 );
            PEX_BITMASK_SET( PtrFeat->PortMask, PLX_FLAG_PORT_NT_LINK_0 );
            break;

        case 0x8613:
            PtrFeat->StnCount    = 1;
            PtrFeat->PortsPerStn = 16;
            PtrFeat->StnMask     = 0x01;    // 0-2,NT,NTB
            PEX_BITMASK_SET_DW( PtrFeat->PortMask, 0, 0x00000007 );
            PEX_BITMASK_SET( PtrFeat->PortMask, PLX_FLAG_PORT_NT_LINK_0 );
            PEX_BITMASK_SET( PtrFeat->PortMask, PLX_FLAG_PORT_NT_DS_P2P );
            break;

        case 0x8614:
            PtrFeat->StnCount    = 1;
            PtrFeat->PortsPerStn = 16;
            PtrFeat->StnMask     = 0x01;    // 0-2,4-10,12,14,NT,NTB(BA)
            PEX_BITMASK_SET_DW( PtrFeat->PortMask, 0, 0x000057F7 );
            PEX_BITMASK_SET( PtrFeat->PortMask, PLX_FLAG_PORT_NT_LINK_0 );
            if (Revision != 0xAA)
            {
                PEX_BITMASK_SET( PtrFeat->PortMask, PLX_FLAG_PORT_NT_DS_P2P );
            }
            break;

        case 0x8615:
            PtrFeat->StnCount    = 1;
            PtrFeat->PortsPerStn = 16;
            PtrFeat->StnMask     = 0x01;    // 0-2,4-10,12,14,DMA,NT,NTB
            PEX_BITMASK_SET_DW( PtrFeat->PortMask, 0, 0x000057F7 );
            PEX_BITMASK_SET( PtrFeat->PortMask, PLX_FLAG_PORT_NT_LINK_0 );
            PEX_BITMASK_SET( PtrFeat->PortMask, PLX_FLAG_PORT_NT_DS_P2P );
            PEX_BITMASK_SET( PtrFeat->PortMask, PLX_FLAG_PORT_DMA_0 );
            break;

        case 0x8616:
            PtrFeat->StnCount    = 2;
            PtrFeat->PortsPerStn = 4;
            PtrFeat->StnMask     = 0x03;    // 0,1,4-6,NT
            PEX_BITMASK_SET_DW( PtrFeat->PortMask, 0, 0x00000073 );
            PEX_BITMASK_SET( PtrFeat->PortMask, PLX_FLAG_PORT_NT_LINK_0 );
            break;

        case 0x8617:
            PtrFeat->StnCount    = 1;
            PtrFeat->PortsPerStn = 16;
            PtrFeat->StnMask     = 0x01;    // 0-3,NT,NTB
            PEX_BITMASK_SET_DW( PtrFeat->PortMask, 0, 0x0000000F );
            PEX_BITMASK_SET( PtrFeat->PortMask, PLX_FLAG_PORT_NT_LINK_0 );
            PEX_BITMASK_SET( PtrFeat->PortMask, PLX_FLAG_PORT_NT_DS_P2P );
            break;

        case 0x8618:
            PtrFeat->StnCount    = 1;
            PtrFeat->PortsPerStn = 16;
            PtrFeat->StnMask     = 0x01;    // 0-15,NT,NTB(BA)
            PEX_BITMASK_SET_DW( PtrFeat->PortMask, 0, 0x0000FFFF );
            PEX_BITMASK_SET( PtrFeat->PortMask, PLX_FLAG_PORT_NT_LINK_0 );
            if (Revision != 0xAA)
            {
                PEX_BITMASK_SET( PtrFeat->PortMask, PLX_FLAG_PORT_NT_DS_P2P );
            }
            break;

        case 0x8619:
            PtrFeat->StnCount    = 1;
            PtrFeat->PortsPerStn = 16;
            PtrFeat->StnMask     = 0x01;    // 0-15,DMA,NT,NTB
            PEX_BITMASK_SET_DW( PtrFeat->PortMask, 0, 0x0000FFFF );
            PEX_BITMASK_SET( PtrFeat->PortMask, PLX_FLAG_PORT_NT_LINK_0 );
            PEX_BITMASK_SET( PtrFeat->PortMask, PLX_FLAG_PORT_NT_DS_P2P );
            PEX_BITMASK_SET( PtrFeat->PortMask, PLX_FLAG_PORT_DMA_0 );
            break;

        case 0x8624:
            PtrFeat->StnCount    = 3;
            PtrFeat->PortsPerStn = 4;
            PtrFeat->StnMask     = 0x07;    // 0,1,4-6,8,9,NT
            PEX_BITMASK_SET_DW( PtrFeat->PortMask, 0, 0x00000373 );
            PEX_BITMASK_SET( PtrFeat->PortMask, PLX_FLAG_PORT_NT_LINK_0 );
            break;

        case 0x8632:
            PtrFeat->StnCount    = 3;
            PtrFeat->PortsPerStn = 4;
            PtrFeat->StnMask     = 0x07;    // 0-11
            PEX_BITMASK_SET_DW( PtrFeat->PortMask, 0, 0x00000FFF );
            break;

        case 0x8647:
            PtrFeat->StnCount    = 3;
            PtrFeat->PortsPerStn = 4;
            PtrFeat->StnMask     = 0x07;    // 0,4,8
            PEX_BITMASK_SET_DW( PtrFeat->PortMask, 0, 0x00000111 );
            break;

        case 0x8648:
            PtrFeat->StnCount    = 3;
            PtrFeat->PortsPerStn = 4;
            PtrFeat->StnMask     = 0x07;    // 0-11,NT
            PEX_BITMASK_SET_DW( PtrFeat->PortMask, 0, 0x00000FFF );
            PEX_BITMASK_SET( PtrFeat->PortMask, PLX_FLAG_PORT_NT_LINK_0 );
            break;

        case 0x8625:
        case 0x8636:
        case 0x8696:
            PtrFeat->StnCount    = 6;
            PtrFeat->PortsPerStn = 4;
            PtrFeat->StnMask     = 0x3F;    // 0-23,NT,NTB
            PEX_BITMASK_SET_DW( PtrFeat->PortMask, 0, 0x00FFFFFF );
            PEX_BITMASK_SET( PtrFeat->PortMask, PLX_FLAG_PORT_NT_LINK_0 );
            PEX_BITMASK_SET( PtrFeat->PortMask, PLX_FLAG_PORT_NT_VIRTUAL_0 );
            break;

        case 0x8649:
            PtrFeat->StnCount    = 3;
            PtrFeat->PortsPerStn = 4;
            PtrFeat->StnMask     = 0x31;    // 0-3,16-23,NT,NTB
            PEX_BITMASK_SET_DW( PtrFeat->PortMask, 0, 0x00FF000F );
            PEX_BITMASK_SET( PtrFeat->PortMask, PLX_FLAG_PORT_NT_LINK_0 );
            PEX_BITMASK_SET( PtrFeat->PortMask, PLX_FLAG_PORT_NT_VIRTUAL_0 );
            break;

        case 0x8664:
            PtrFeat->StnCount    = 4;
            PtrFeat->PortsPerStn = 4;
            PtrFeat->StnMask     = 0x33;    // 0-7,16-23,NT,NTB
            PEX_BITMASK_SET_DW( PtrFeat->PortMask, 0, 0x00FF00FF );
            PEX_BITMASK_SET( PtrFeat->PortMask, PLX_FLAG_PORT_NT_LINK_0 );
            PEX_BITMASK_SET( PtrFeat->PortMask, PLX_FLAG_PORT_NT_DS_P2P );
            break;

        case 0x8680:
            PtrFeat->StnCount    = 5;
            PtrFeat->PortsPerStn = 4;
            PtrFeat->StnMask     = 0x37;    // 0-11,16-23,NT,NTB
            PEX_BITMASK_SET_DW( PtrFeat->PortMask, 0, 0x00FF0FFF );
            PEX_BITMASK_SET( PtrFeat->PortMask, PLX_FLAG_PORT_NT_LINK_0 );
            PEX_BITMASK_SET( PtrFeat->PortMask, PLX_FLAG_PORT_NT_VIRTUAL_0 );
            break;

        case 0x8700:
        case 0x8712:
        case 0x8716:
            PtrFeat->StnCount    = 1;
            PtrFeat->PortsPerStn = 4;
            PtrFeat->StnMask     = 0x01;    // 0-3,NT
            PEX_BITMASK_SET_DW( PtrFeat->PortMask, 0, 0x0000000F );
            PEX_BITMASK_SET( PtrFeat->PortMask, PLX_FLAG_PORT_NT_LINK_0 );
            PEX_BITMASK_SET( PtrFeat->PortMask, PLX_FLAG_PORT_NT_VIRTUAL_0 );
            break;

        case 0x8713:
            PtrFeat->StnCount    = 2;
            PtrFeat->PortsPerStn = 8;
            PtrFeat->StnMask     = 0x03;    // 0-5,8-13,NT 0/1,DMA 0-4
            PEX_BITMASK_SET_DW( PtrFeat->PortMask, 0, 0x00003F3F );
            PEX_BITMASK_SET( PtrFeat->PortMask, PLX_FLAG_PORT_NT_LINK_0 );
            PEX_BITMASK_SET( PtrFeat->PortMask, PLX_FLAG_PORT_NT_LINK_1 );
            PEX_BITMASK_SET( PtrFeat->PortMask, PLX_FLAG_PORT_NT_VIRTUAL_0 );
            PEX_BITMASK_SET( PtrFeat->PortMask, PLX_FLAG_PORT_NT_VIRTUAL_1 );
            PEX_BITMASK_SET( PtrFeat->PortMask, PLX_FLAG_PORT_DMA_0 );
            PEX_BITMASK_SET( PtrFeat->PortMask, PLX_FLAG_PORT_DMA_1 );
            PEX_BITMASK_SET( PtrFeat->PortMask, PLX_FLAG_PORT_DMA_2 );
            PEX_BITMASK_SET( PtrFeat->PortMask, PLX_FLAG_PORT_DMA_3 );
            PEX_BITMASK_SET( PtrFeat->PortMask, PLX_FLAG_PORT_ALUT_0 );
            PEX_BITMASK_SET( PtrFeat->PortMask, PLX_FLAG_PORT_ALUT_1 );
            PEX_BITMASK_SET( PtrFeat->PortMask, PLX_FLAG_PORT_ALUT_2 );
            PEX_BITMASK_SET( PtrFeat->PortMask, PLX_FLAG_PORT_ALUT_3 );
            break;

        case 0x8714:
        case 0x8718:
            PtrFeat->StnCount    = 1;
            PtrFeat->PortsPerStn = 5;
            PtrFeat->StnMask     = 0x01;    // 0-4,NT,ALUT 0-3
            PEX_BITMASK_SET_DW( PtrFeat->PortMask, 0, 0x0000001F );
            PEX_BITMASK_SET( PtrFeat->PortMask, PLX_FLAG_PORT_NT_LINK_0 );
            PEX_BITMASK_SET( PtrFeat->PortMask, PLX_FLAG_PORT_NT_VIRTUAL_0 );
            PEX_BITMASK_SET( PtrFeat->PortMask, PLX_FLAG_PORT_ALUT_0 );
            PEX_BITMASK_SET( PtrFeat->PortMask, PLX_FLAG_PORT_ALUT_1 );
            PEX_BITMASK_SET( PtrFeat->PortMask, PLX_FLAG_PORT_ALUT_2 );
            PEX_BITMASK_SET( PtrFeat->PortMask, PLX_FLAG_PORT_ALUT_3 );
            break;

        case 0x8717:
            PtrFeat->StnCount    = 2;
            PtrFeat->PortsPerStn = 8;
            PtrFeat->StnMask     = 0x03;    // 0-5,8-13,NT 0/1,DMA 0-4
            PEX_BITMASK_SET_DW( PtrFeat->PortMask, 0, 0x00003F3F );
            PEX_BITMASK_SET( PtrFeat->PortMask, PLX_FLAG_PORT_NT_LINK_0 );
            PEX_BITMASK_SET( PtrFeat->PortMask, PLX_FLAG_PORT_NT_LINK_1 );
            PEX_BITMASK_SET( PtrFeat->PortMask, PLX_FLAG_PORT_NT_VIRTUAL_0 );
            PEX_BITMASK_SET( PtrFeat->PortMask, PLX_FLAG_PORT_NT_VIRTUAL_1 );
            PEX_BITMASK_SET( PtrFeat->PortMask, PLX_FLAG_PORT_DMA_0 );
            PEX_BITMASK_SET( PtrFeat->PortMask, PLX_FLAG_PORT_DMA_1 );
            PEX_BITMASK_SET( PtrFeat->PortMask, PLX_FLAG_PORT_DMA_2 );
            PEX_BITMASK_SET( PtrFeat->PortMask, PLX_FLAG_PORT_DMA_3 );
            PEX_BITMASK_SET( PtrFeat->PortMask, PLX_FLAG_PORT_ALUT_0 );
            PEX_BITMASK_SET( PtrFeat->PortMask, PLX_FLAG_PORT_ALUT_1 );
            PEX_BITMASK_SET( PtrFeat->PortMask, PLX_FLAG_PORT_ALUT_2 );
            PEX_BITMASK_SET( PtrFeat->PortMask, PLX_FLAG_PORT_ALUT_3 );
            break;

        case 0x8723:
            PtrFeat->StnCount    = 2;
            PtrFeat->PortsPerStn = 8;
            PtrFeat->StnMask     = 0x03;    // 0-3,8-10,NT
            PEX_BITMASK_SET_DW( PtrFeat->PortMask, 0, 0x0000070F );
            break;

        case 0x8724:
            PtrFeat->StnCount    = 2;
            PtrFeat->PortsPerStn = 8;
            PtrFeat->StnMask     = 0x03;    // 0-3,8-10,NT
            PEX_BITMASK_SET_DW( PtrFeat->PortMask, 0, 0x0000070F );
            PEX_BITMASK_SET( PtrFeat->PortMask, PLX_FLAG_PORT_NT_LINK_0 );
            PEX_BITMASK_SET( PtrFeat->PortMask, PLX_FLAG_PORT_NT_VIRTUAL_0 );
            break;

        case 0x8725:
            PtrFeat->StnCount    = 2;
            PtrFeat->PortsPerStn = 8;
            PtrFeat->StnMask     = 0x03;    // 0-5,8-13,NT 0/1,DMA 0-4
            PEX_BITMASK_SET_DW( PtrFeat->PortMask, 0, 0x00003F3F );
            PEX_BITMASK_SET( PtrFeat->PortMask, PLX_FLAG_PORT_NT_LINK_0 );
            PEX_BITMASK_SET( PtrFeat->PortMask, PLX_FLAG_PORT_NT_LINK_1 );
            PEX_BITMASK_SET( PtrFeat->PortMask, PLX_FLAG_PORT_NT_VIRTUAL_0 );
            PEX_BITMASK_SET( PtrFeat->PortMask, PLX_FLAG_PORT_NT_VIRTUAL_1 );
            PEX_BITMASK_SET( PtrFeat->PortMask, PLX_FLAG_PORT_DMA_0 );
            PEX_BITMASK_SET( PtrFeat->PortMask, PLX_FLAG_PORT_DMA_1 );
            PEX_BITMASK_SET( PtrFeat->PortMask, PLX_FLAG_PORT_DMA_2 );
            PEX_BITMASK_SET( PtrFeat->PortMask, PLX_FLAG_PORT_DMA_3 );
            PEX_BITMASK_SET( PtrFeat->PortMask, PLX_FLAG_PORT_ALUT_0 );
            PEX_BITMASK_SET( PtrFeat->PortMask, PLX_FLAG_PORT_ALUT_1 );
            PEX_BITMASK_SET( PtrFeat->PortMask, PLX_FLAG_PORT_ALUT_2 );
            PEX_BITMASK_SET( PtrFeat->PortMask, PLX_FLAG_PORT_ALUT_3 );
            break;

        case 0x8732:
            PtrFeat->StnCount    = 2;
            PtrFeat->PortsPerStn = 8;
            PtrFeat->StnMask     = 0x03;    // 0-3,8-11,NT
            PEX_BITMASK_SET_DW( PtrFeat->PortMask, 0, 0x00000F0F );
            PEX_BITMASK_SET( PtrFeat->PortMask, PLX_FLAG_PORT_NT_LINK_0 );
            PEX_BITMASK_SET( PtrFeat->PortMask, PLX_FLAG_PORT_NT_VIRTUAL_0 );
            break;

        case 0x8733:
            PtrFeat->StnCount    = 3;
            PtrFeat->PortsPerStn = 8;
            PtrFeat->StnMask     = 0x07;    // 0-5,8-13,16-21,NT 0/1,DMA 0-4
            PEX_BITMASK_SET_DW( PtrFeat->PortMask, 0, 0x003F3F3F );
            PEX_BITMASK_SET( PtrFeat->PortMask, PLX_FLAG_PORT_NT_LINK_0 );
            PEX_BITMASK_SET( PtrFeat->PortMask, PLX_FLAG_PORT_NT_LINK_1 );
            PEX_BITMASK_SET( PtrFeat->PortMask, PLX_FLAG_PORT_NT_VIRTUAL_0 );
            PEX_BITMASK_SET( PtrFeat->PortMask, PLX_FLAG_PORT_NT_VIRTUAL_1 );
            PEX_BITMASK_SET( PtrFeat->PortMask, PLX_FLAG_PORT_DMA_0 );
            PEX_BITMASK_SET( PtrFeat->PortMask, PLX_FLAG_PORT_DMA_1 );
            PEX_BITMASK_SET( PtrFeat->PortMask, PLX_FLAG_PORT_DMA_2 );
            PEX_BITMASK_SET( PtrFeat->PortMask, PLX_FLAG_PORT_DMA_3 );
            PEX_BITMASK_SET( PtrFeat->PortMask, PLX_FLAG_PORT_ALUT_0 );
            PEX_BITMASK_SET( PtrFeat->PortMask, PLX_FLAG_PORT_ALUT_1 );
            PEX_BITMASK_SET( PtrFeat->PortMask, PLX_FLAG_PORT_ALUT_2 );
            PEX_BITMASK_SET( PtrFeat->PortMask, PLX_FLAG_PORT_ALUT_3 );
            break;

        case 0x8734:
            PtrFeat->StnCount    = 2;
            PtrFeat->PortsPerStn = 4;
            PtrFeat->StnMask     = 0x03;    // 0-7,NT 0/1,ALUT 0-3,VS_S0-1
            PEX_BITMASK_SET_DW( PtrFeat->PortMask, 0, 0x000000FF );
            PEX_BITMASK_SET( PtrFeat->PortMask, PLX_FLAG_PORT_NT_LINK_0 );
            PEX_BITMASK_SET( PtrFeat->PortMask, PLX_FLAG_PORT_NT_LINK_1 );
            PEX_BITMASK_SET( PtrFeat->PortMask, PLX_FLAG_PORT_NT_VIRTUAL_0 );
            PEX_BITMASK_SET( PtrFeat->PortMask, PLX_FLAG_PORT_NT_VIRTUAL_1 );
            PEX_BITMASK_SET( PtrFeat->PortMask, PLX_FLAG_PORT_ALUT_0 );
            PEX_BITMASK_SET( PtrFeat->PortMask, PLX_FLAG_PORT_ALUT_1 );
            PEX_BITMASK_SET( PtrFeat->PortMask, PLX_FLAG_PORT_ALUT_2 );
            PEX_BITMASK_SET( PtrFeat->PortMask, PLX_FLAG_PORT_ALUT_3 );
            PEX_BITMASK_SET( PtrFeat->PortMask, PLX_FLAG_PORT_STN_REGS_S0 );
            PEX_BITMASK_SET( PtrFeat->PortMask, PLX_FLAG_PORT_STN_REGS_S1 );
            break;

        case 0x8747:
            PtrFeat->StnCount    = 3;
            PtrFeat->PortsPerStn = 8;
            PtrFeat->StnMask     = 0x07;    // 0,8,9,16,17
            PEX_BITMASK_SET_DW( PtrFeat->PortMask, 0, 0x00030301 );
            break;

        case 0x8748:
            PtrFeat->StnCount    = 3;
            PtrFeat->PortsPerStn = 8;
            PtrFeat->StnMask     = 0x07;    // 0-3,8-11,16-19,NT
            PEX_BITMASK_SET_DW( PtrFeat->PortMask, 0, 0x000F0F0F );
            PEX_BITMASK_SET( PtrFeat->PortMask, PLX_FLAG_PORT_NT_LINK_0 );
            PEX_BITMASK_SET( PtrFeat->PortMask, PLX_FLAG_PORT_NT_VIRTUAL_0 );
            break;

        case 0x8749:
            PtrFeat->StnCount    = 3;
            PtrFeat->PortsPerStn = 8;
            PtrFeat->StnMask     = 0x07;    // 0-5,8-13,16-21,NT 0/1,DMA 0-4
            PEX_BITMASK_SET_DW( PtrFeat->PortMask, 0, 0x003F3F3F );
            PEX_BITMASK_SET( PtrFeat->PortMask, PLX_FLAG_PORT_NT_LINK_0 );
            PEX_BITMASK_SET( PtrFeat->PortMask, PLX_FLAG_PORT_NT_LINK_1 );
            PEX_BITMASK_SET( PtrFeat->PortMask, PLX_FLAG_PORT_NT_VIRTUAL_0 );
            PEX_BITMASK_SET( PtrFeat->PortMask, PLX_FLAG_PORT_NT_VIRTUAL_1 );
            PEX_BITMASK_SET( PtrFeat->PortMask, PLX_FLAG_PORT_DMA_0 );
            PEX_BITMASK_SET( PtrFeat->PortMask, PLX_FLAG_PORT_DMA_1 );
            PEX_BITMASK_SET( PtrFeat->PortMask, PLX_FLAG_PORT_DMA_2 );
            PEX_BITMASK_SET( PtrFeat->PortMask, PLX_FLAG_PORT_DMA_3 );
            PEX_BITMASK_SET( PtrFeat->PortMask, PLX_FLAG_PORT_ALUT_0 );
            PEX_BITMASK_SET( PtrFeat->PortMask, PLX_FLAG_PORT_ALUT_1 );
            PEX_BITMASK_SET( PtrFeat->PortMask, PLX_FLAG_PORT_ALUT_2 );
            PEX_BITMASK_SET( PtrFeat->PortMask, PLX_FLAG_PORT_ALUT_3 );
            break;

        case 0x8750:
            PtrFeat->StnCount    = 3;
            PtrFeat->PortsPerStn = 4;
            PtrFeat->StnMask     = 0x07;    // 0-11,NT 0/1,ALUT 0-3,VS_S0-2
            PEX_BITMASK_SET_DW( PtrFeat->PortMask, 0, 0x00000FFF );
            PEX_BITMASK_SET( PtrFeat->PortMask, PLX_FLAG_PORT_NT_LINK_0 );
            PEX_BITMASK_SET( PtrFeat->PortMask, PLX_FLAG_PORT_NT_LINK_1 );
            PEX_BITMASK_SET( PtrFeat->PortMask, PLX_FLAG_PORT_NT_VIRTUAL_0 );
            PEX_BITMASK_SET( PtrFeat->PortMask, PLX_FLAG_PORT_NT_VIRTUAL_1 );
            PEX_BITMASK_SET( PtrFeat->PortMask, PLX_FLAG_PORT_ALUT_0 );
            PEX_BITMASK_SET( PtrFeat->PortMask, PLX_FLAG_PORT_ALUT_1 );
            PEX_BITMASK_SET( PtrFeat->PortMask, PLX_FLAG_PORT_ALUT_2 );
            PEX_BITMASK_SET( PtrFeat->PortMask, PLX_FLAG_PORT_ALUT_3 );
            PEX_BITMASK_SET( PtrFeat->PortMask, PLX_FLAG_PORT_STN_REGS_S0 );
            PEX_BITMASK_SET( PtrFeat->PortMask, PLX_FLAG_PORT_STN_REGS_S1 );
            PEX_BITMASK_SET( PtrFeat->PortMask, PLX_FLAG_PORT_STN_REGS_S2 );
            break;

        case 0x8764:
            PtrFeat->StnCount    = 4;
            PtrFeat->PortsPerStn = 4;
            PtrFeat->StnMask     = 0x0F;    // 0-15,NT 0/1,ALUT 0-3,VS_S0-3
            PEX_BITMASK_SET_DW( PtrFeat->PortMask, 0, 0x0000FFFF );
            PEX_BITMASK_SET( PtrFeat->PortMask, PLX_FLAG_PORT_NT_LINK_0 );
            PEX_BITMASK_SET( PtrFeat->PortMask, PLX_FLAG_PORT_NT_LINK_1 );
            PEX_BITMASK_SET( PtrFeat->PortMask, PLX_FLAG_PORT_NT_VIRTUAL_0 );
            PEX_BITMASK_SET( PtrFeat->PortMask, PLX_FLAG_PORT_NT_VIRTUAL_1 );
            PEX_BITMASK_SET( PtrFeat->PortMask, PLX_FLAG_PORT_ALUT_0 );
            PEX_BITMASK_SET( PtrFeat->PortMask, PLX_FLAG_PORT_ALUT_1 );
            PEX_BITMASK_SET( PtrFeat->PortMask, PLX_FLAG_PORT_ALUT_2 );
            PEX_BITMASK_SET( PtrFeat->PortMask, PLX_FLAG_PORT_ALUT_3 );
            PEX_BITMASK_SET( PtrFeat->PortMask, PLX_FLAG_PORT_STN_REGS_S0 );
            PEX_BITMASK_SET( PtrFeat->PortMask, PLX_FLAG_PORT_STN_REGS_S1 );
            PEX_BITMASK_SET( PtrFeat->PortMask, PLX_FLAG_PORT_STN_REGS_S2 );
            PEX_BITMASK_SET( PtrFeat->PortMask, PLX_FLAG_PORT_STN_REGS_S3 );
            break;

        case 0x8780:
            PtrFeat->StnCount    = 5;
            PtrFeat->PortsPerStn = 4;
            PtrFeat->StnMask     = 0x1F;    // 0-19,NT 0/1,ALUT 0-3,VS_S0-4
            PEX_BITMASK_SET_DW( PtrFeat->PortMask, 0,  0x000FFFFF);
            PEX_BITMASK_SET( PtrFeat->PortMask, PLX_FLAG_PORT_NT_LINK_0 );
            PEX_BITMASK_SET( PtrFeat->PortMask, PLX_FLAG_PORT_NT_LINK_1 );
            PEX_BITMASK_SET( PtrFeat->PortMask, PLX_FLAG_PORT_NT_VIRTUAL_0 );
            PEX_BITMASK_SET( PtrFeat->PortMask, PLX_FLAG_PORT_NT_VIRTUAL_1 );
            PEX_BITMASK_SET( PtrFeat->PortMask, PLX_FLAG_PORT_ALUT_0 );
            PEX_BITMASK_SET( PtrFeat->PortMask, PLX_FLAG_PORT_ALUT_1 );
            PEX_BITMASK_SET( PtrFeat->PortMask, PLX_FLAG_PORT_ALUT_2 );
            PEX_BITMASK_SET( PtrFeat->PortMask, PLX_FLAG_PORT_ALUT_3 );
            PEX_BITMASK_SET( PtrFeat->PortMask, PLX_FLAG_PORT_STN_REGS_S0 );
            PEX_BITMASK_SET( PtrFeat->PortMask, PLX_FLAG_PORT_STN_REGS_S1 );
            PEX_BITMASK_SET( PtrFeat->PortMask, PLX_FLAG_PORT_STN_REGS_S2 );
            PEX_BITMASK_SET( PtrFeat->PortMask, PLX_FLAG_PORT_STN_REGS_S3 );
            PEX_BITMASK_SET( PtrFeat->PortMask, PLX_FLAG_PORT_STN_REGS_S4 );
            break;

        case 0x8796:
            PtrFeat->StnCount    = 6;
            PtrFeat->PortsPerStn = 4;
            PtrFeat->StnMask     = 0x3F;    // 0-23,NT 0/1,ALUT 0-3,VS_S0-5
            PEX_BITMASK_SET_DW( PtrFeat->PortMask, 0, 0x00FFFFFF );
            PEX_BITMASK_SET( PtrFeat->PortMask, PLX_FLAG_PORT_NT_LINK_0 );
            PEX_BITMASK_SET( PtrFeat->PortMask, PLX_FLAG_PORT_NT_LINK_1 );
            PEX_BITMASK_SET( PtrFeat->PortMask, PLX_FLAG_PORT_NT_VIRTUAL_0 );
            PEX_BITMASK_SET( PtrFeat->PortMask, PLX_FLAG_PORT_NT_VIRTUAL_1 );
            PEX_BITMASK_SET( PtrFeat->PortMask, PLX_FLAG_PORT_ALUT_0 );
            PEX_BITMASK_SET( PtrFeat->PortMask, PLX_FLAG_PORT_ALUT_1 );
            PEX_BITMASK_SET( PtrFeat->PortMask, PLX_FLAG_PORT_ALUT_2 );
            PEX_BITMASK_SET( PtrFeat->PortMask, PLX_FLAG_PORT_ALUT_3 );
            PEX_BITMASK_SET( PtrFeat->PortMask, PLX_FLAG_PORT_STN_REGS_S0 );
            PEX_BITMASK_SET( PtrFeat->PortMask, PLX_FLAG_PORT_STN_REGS_S1 );
            PEX_BITMASK_SET( PtrFeat->PortMask, PLX_FLAG_PORT_STN_REGS_S2 );
            PEX_BITMASK_SET( PtrFeat->PortMask, PLX_FLAG_PORT_STN_REGS_S3 );
            PEX_BITMASK_SET( PtrFeat->PortMask, PLX_FLAG_PORT_STN_REGS_S4 );
            PEX_BITMASK_SET( PtrFeat->PortMask, PLX_FLAG_PORT_STN_REGS_S5 );
            break;

        case 0x9712:
        case 0x9716:
            PtrFeat->StnCount    = 1;
            PtrFeat->PortsPerStn = 5;
            PtrFeat->StnMask     = 0x01;    // 0-4,NT,ALUT 0-3,S0,GEP,GEP_DS
            PEX_BITMASK_SET_DW( PtrFeat->PortMask, 0, 0x0000001F );
            PEX_BITMASK_SET( PtrFeat->PortMask, PLX_FLAG_PORT_NT_LINK_0 );
            PEX_BITMASK_SET( PtrFeat->PortMask, PLX_FLAG_PORT_NT_LINK_1 );
            PEX_BITMASK_SET( PtrFeat->PortMask, PLX_FLAG_PORT_NT_VIRTUAL_0 );
            PEX_BITMASK_SET( PtrFeat->PortMask, PLX_FLAG_PORT_NT_VIRTUAL_1 );
            PEX_BITMASK_SET( PtrFeat->PortMask, PLX_FLAG_PORT_ALUT_0 );
            PEX_BITMASK_SET( PtrFeat->PortMask, PLX_FLAG_PORT_ALUT_1 );
            PEX_BITMASK_SET( PtrFeat->PortMask, PLX_FLAG_PORT_ALUT_2 );
            PEX_BITMASK_SET( PtrFeat->PortMask, PLX_FLAG_PORT_ALUT_3 );
            PEX_BITMASK_SET( PtrFeat->PortMask, PLX_FLAG_PORT_STN_REGS_S0 );
            PEX_BITMASK_SET( PtrFeat->PortMask, 24 ); // Port 24 for GEP
            PEX_BITMASK_SET( PtrFeat->PortMask, 25 ); // Port 25 for GEP DS
            break;

        case 0x9733:
            PtrFeat->StnCount    = 2;
            PtrFeat->PortsPerStn = 4;
            PtrFeat->StnMask     = 0x03;    // 0-7,NT 0/1,ALUT 0-3,S0-1,GEP,GEP_DS
            PEX_BITMASK_SET_DW( PtrFeat->PortMask, 0, 0x000000FF );
            PEX_BITMASK_SET( PtrFeat->PortMask, PLX_FLAG_PORT_NT_LINK_0 );
            PEX_BITMASK_SET( PtrFeat->PortMask, PLX_FLAG_PORT_NT_LINK_1 );
            PEX_BITMASK_SET( PtrFeat->PortMask, PLX_FLAG_PORT_NT_VIRTUAL_0 );
            PEX_BITMASK_SET( PtrFeat->PortMask, PLX_FLAG_PORT_NT_VIRTUAL_1 );
            PEX_BITMASK_SET( PtrFeat->PortMask, PLX_FLAG_PORT_ALUT_0 );
            PEX_BITMASK_SET( PtrFeat->PortMask, PLX_FLAG_PORT_ALUT_1 );
            PEX_BITMASK_SET( PtrFeat->PortMask, PLX_FLAG_PORT_ALUT_2 );
            PEX_BITMASK_SET( PtrFeat->PortMask, PLX_FLAG_PORT_ALUT_3 );
            PEX_BITMASK_SET( PtrFeat->PortMask, PLX_FLAG_PORT_STN_REGS_S0 );
            PEX_BITMASK_SET( PtrFeat->PortMask, PLX_FLAG_PORT_STN_REGS_S1 );
            PEX_BITMASK_SET( PtrFeat->PortMask, 24 ); // Port 24 for GEP
            PEX_BITMASK_SET( PtrFeat->PortMask, 25 ); // Port 25 for GEP DS
            break;

        case 0x9749:
            PtrFeat->StnCount    = 3;
            PtrFeat->PortsPerStn = 4;
            PtrFeat->StnMask     = 0x07;    // 0-14,NT 0/1,ALUT 0-3,S0-2,GEP,GEP_DS
            PEX_BITMASK_SET_DW( PtrFeat->PortMask, 0, 0x00007FFF );
            PEX_BITMASK_SET( PtrFeat->PortMask, PLX_FLAG_PORT_NT_LINK_0 );
            PEX_BITMASK_SET( PtrFeat->PortMask, PLX_FLAG_PORT_NT_LINK_1 );
            PEX_BITMASK_SET( PtrFeat->PortMask, PLX_FLAG_PORT_NT_VIRTUAL_0 );
            PEX_BITMASK_SET( PtrFeat->PortMask, PLX_FLAG_PORT_NT_VIRTUAL_1 );
            PEX_BITMASK_SET( PtrFeat->PortMask, PLX_FLAG_PORT_ALUT_0 );
            PEX_BITMASK_SET( PtrFeat->PortMask, PLX_FLAG_PORT_ALUT_1 );
            PEX_BITMASK_SET( PtrFeat->PortMask, PLX_FLAG_PORT_ALUT_2 );
            PEX_BITMASK_SET( PtrFeat->PortMask, PLX_FLAG_PORT_ALUT_3 );
            PEX_BITMASK_SET( PtrFeat->PortMask, PLX_FLAG_PORT_STN_REGS_S0 );
            PEX_BITMASK_SET( PtrFeat->PortMask, PLX_FLAG_PORT_STN_REGS_S1 );
            PEX_BITMASK_SET( PtrFeat->PortMask, PLX_FLAG_PORT_STN_REGS_S2 );
            PEX_BITMASK_SET( PtrFeat->PortMask, 24 ); // Port 24 for GEP
            PEX_BITMASK_SET( PtrFeat->PortMask, 25 ); // Port 25 for GEP DS
            break;

        case 0x9765:
            PtrFeat->StnCount    = 4;
            PtrFeat->PortsPerStn = 4;
            PtrFeat->StnMask     = 0x0F;    // 0-15,NT 0/1,ALUT 0-3,S0-3,GEP,GEP_DS
            PEX_BITMASK_SET_DW( PtrFeat->PortMask, 0, 0x0000FFFF );
            PEX_BITMASK_SET( PtrFeat->PortMask, PLX_FLAG_PORT_NT_LINK_0 );
            PEX_BITMASK_SET( PtrFeat->PortMask, PLX_FLAG_PORT_NT_LINK_1 );
            PEX_BITMASK_SET( PtrFeat->PortMask, PLX_FLAG_PORT_NT_VIRTUAL_0 );
            PEX_BITMASK_SET( PtrFeat->PortMask, PLX_FLAG_PORT_NT_VIRTUAL_1 );
            PEX_BITMASK_SET( PtrFeat->PortMask, PLX_FLAG_PORT_ALUT_0 );
            PEX_BITMASK_SET( PtrFeat->PortMask, PLX_FLAG_PORT_ALUT_1 );
            PEX_BITMASK_SET( PtrFeat->PortMask, PLX_FLAG_PORT_ALUT_2 );
            PEX_BITMASK_SET( PtrFeat->PortMask, PLX_FLAG_PORT_ALUT_3 );
            PEX_BITMASK_SET( PtrFeat->PortMask, PLX_FLAG_PORT_STN_REGS_S0 );
            PEX_BITMASK_SET( PtrFeat->PortMask, PLX_FLAG_PORT_STN_REGS_S1 );
            PEX_BITMASK_SET( PtrFeat->PortMask, PLX_FLAG_PORT_STN_REGS_S2 );
            PEX_BITMASK_SET( PtrFeat->PortMask, PLX_FLAG_PORT_STN_REGS_S3 );
            PEX_BITMASK_SET( PtrFeat->PortMask, 24 ); // Port 24 for GEP
            PEX_BITMASK_SET( PtrFeat->PortMask, 25 ); // Port 25 for GEP DS
            break;

        case 0x9781:
            PtrFeat->StnCount    = 5;
            PtrFeat->PortsPerStn = 4;
            PtrFeat->StnMask     = 0x1F;    // 0-19,NT 0/1,ALUT 0-3,S0-4,GEP,GEP_DS
            PEX_BITMASK_SET_DW( PtrFeat->PortMask, 0, 0x000FFFFF );
            PEX_BITMASK_SET( PtrFeat->PortMask, PLX_FLAG_PORT_NT_LINK_0 );
            PEX_BITMASK_SET( PtrFeat->PortMask, PLX_FLAG_PORT_NT_LINK_1 );
            PEX_BITMASK_SET( PtrFeat->PortMask, PLX_FLAG_PORT_NT_VIRTUAL_0 );
            PEX_BITMASK_SET( PtrFeat->PortMask, PLX_FLAG_PORT_NT_VIRTUAL_1 );
            PEX_BITMASK_SET( PtrFeat->PortMask, PLX_FLAG_PORT_ALUT_0 );
            PEX_BITMASK_SET( PtrFeat->PortMask, PLX_FLAG_PORT_ALUT_1 );
            PEX_BITMASK_SET( PtrFeat->PortMask, PLX_FLAG_PORT_ALUT_2 );
            PEX_BITMASK_SET( PtrFeat->PortMask, PLX_FLAG_PORT_ALUT_3 );
            PEX_BITMASK_SET( PtrFeat->PortMask, PLX_FLAG_PORT_STN_REGS_S0 );
            PEX_BITMASK_SET( PtrFeat->PortMask, PLX_FLAG_PORT_STN_REGS_S1 );
            PEX_BITMASK_SET( PtrFeat->PortMask, PLX_FLAG_PORT_STN_REGS_S2 );
            PEX_BITMASK_SET( PtrFeat->PortMask, PLX_FLAG_PORT_STN_REGS_S3 );
            PEX_BITMASK_SET( PtrFeat->PortMask, PLX_FLAG_PORT_STN_REGS_S4 );
            PEX_BITMASK_SET( PtrFeat->PortMask, 24 ); // Port 24 for GEP
            PEX_BITMASK_SET( PtrFeat->PortMask, 25 ); // Port 25 for GEP DS
            break;

        case 0x9797:
            PtrFeat->StnCount    = 6;
            PtrFeat->PortsPerStn = 4;
            PtrFeat->StnMask     = 0x3F;    // 0-25,NT 0/1,ALUT 0-3,S0-5,GEP,GEP_DS
            PEX_BITMASK_SET_DW( PtrFeat->PortMask, 0, 0x00FFFFFF );
            PEX_BITMASK_SET( PtrFeat->PortMask, PLX_FLAG_PORT_NT_LINK_0 );
            PEX_BITMASK_SET( PtrFeat->PortMask, PLX_FLAG_PORT_NT_LINK_1 );
            PEX_BITMASK_SET( PtrFeat->PortMask, PLX_FLAG_PORT_NT_VIRTUAL_0 );
            PEX_BITMASK_SET( PtrFeat->PortMask, PLX_FLAG_PORT_NT_VIRTUAL_1 );
            PEX_BITMASK_SET( PtrFeat->PortMask, PLX_FLAG_PORT_ALUT_0 );
            PEX_BITMASK_SET( PtrFeat->PortMask, PLX_FLAG_PORT_ALUT_1 );
            PEX_BITMASK_SET( PtrFeat->PortMask, PLX_FLAG_PORT_ALUT_2 );
            PEX_BITMASK_SET( PtrFeat->PortMask, PLX_FLAG_PORT_ALUT_3 );
            PEX_BITMASK_SET( PtrFeat->PortMask, PLX_FLAG_PORT_STN_REGS_S0 );
            PEX_BITMASK_SET( PtrFeat->PortMask, PLX_FLAG_PORT_STN_REGS_S1 );
            PEX_BITMASK_SET( PtrFeat->PortMask, PLX_FLAG_PORT_STN_REGS_S2 );
            PEX_BITMASK_SET( PtrFeat->PortMask, PLX_FLAG_PORT_STN_REGS_S3 );
            PEX_BITMASK_SET( PtrFeat->PortMask, PLX_FLAG_PORT_STN_REGS_S4 );
            PEX_BITMASK_SET( PtrFeat->PortMask, PLX_FLAG_PORT_STN_REGS_S5 );
            PEX_BITMASK_SET( PtrFeat->PortMask, 24 ); // Port 24 for GEP
            PEX_BITMASK_SET( PtrFeat->PortMask, 25 ); // Port 25 for GEP DS
            break;

        case 0xA024:
            // Station 0 & 1 (8 upper lanes (24-31)), 2 x2 ports
            PtrFeat->StnCount    = 3;
            PtrFeat->PortsPerStn = 16;
            PtrFeat->StnMask     = 0x83;
            PEX_BITMASK_SET_DW( PtrFeat->PortMask, 0, 0xFF00FFFF );
            PEX_BITMASK_SET_DW( PtrFeat->PortMask, 3, 0x00300000 );
            PEX_BITMASK_SET( PtrFeat->PortMask, PLX_FLAG_PORT_STN_REGS_S0 );
            PEX_BITMASK_SET( PtrFeat->PortMask, PLX_FLAG_PORT_STN_REGS_S1 );

            // Internal ports
            PEX_BITMASK_SET( PtrFeat->PortMask, PLX_FLAG_PORT_INT_MGMT  );
            PEX_BITMASK_SET( PtrFeat->PortMask, PLX_FLAG_PORT_INT_DS_0  );
            PEX_BITMASK_SET( PtrFeat->PortMask, PLX_FLAG_PORT_INT_DS_12 );
            PEX_BITMASK_SET( PtrFeat->PortMask, PLX_FLAG_PORT_INT_UP_0  );
            PEX_BITMASK_SET( PtrFeat->PortMask, PLX_FLAG_PORT_INT_UP_12 );
            break;

        case 0xA032:
            // Station 0 & 1, 2 x2 ports
            PtrFeat->StnCount    = 3;
            PtrFeat->PortsPerStn = 16;
            PtrFeat->StnMask     = 0x83;
            PEX_BITMASK_SET_DW( PtrFeat->PortMask, 0, 0xFFFFFFFF );
            PEX_BITMASK_SET_DW( PtrFeat->PortMask, 3, 0x00300000 );
            PEX_BITMASK_SET( PtrFeat->PortMask, PLX_FLAG_PORT_STN_REGS_S0 );
            PEX_BITMASK_SET( PtrFeat->PortMask, PLX_FLAG_PORT_STN_REGS_S1 );
            PEX_BITMASK_SET( PtrFeat->PortMask, PLX_FLAG_PORT_STN_REGS_S2 );

            // Internal ports
            PEX_BITMASK_SET( PtrFeat->PortMask, PLX_FLAG_PORT_INT_MGMT  );
            PEX_BITMASK_SET( PtrFeat->PortMask, PLX_FLAG_PORT_INT_DS_0  );
            PEX_BITMASK_SET( PtrFeat->PortMask, PLX_FLAG_PORT_INT_DS_12 );
            PEX_BITMASK_SET( PtrFeat->PortMask, PLX_FLAG_PORT_INT_UP_0  );
            PEX_BITMASK_SET( PtrFeat->PortMask, PLX_FLAG_PORT_INT_UP_12 );
            break;

        case 0xA048:
            // Station 0-2 2 x2 ports
            PtrFeat->StnCount    = 4;
            PtrFeat->PortsPerStn = 16;
            PtrFeat->StnMask     = 0x87;
            PEX_BITMASK_SET_DW( PtrFeat->PortMask, 0, 0xFFFFFFFF );
            PEX_BITMASK_SET_DW( PtrFeat->PortMask, 1, 0x0000FFFF );
            PEX_BITMASK_SET_DW( PtrFeat->PortMask, 3, 0x00300000 );
            PEX_BITMASK_SET( PtrFeat->PortMask, PLX_FLAG_PORT_STN_REGS_S0 );
            PEX_BITMASK_SET( PtrFeat->PortMask, PLX_FLAG_PORT_STN_REGS_S1 );
            PEX_BITMASK_SET( PtrFeat->PortMask, PLX_FLAG_PORT_STN_REGS_S2 );

            // Internal ports
            PEX_BITMASK_SET( PtrFeat->PortMask, PLX_FLAG_PORT_INT_MGMT  );
            PEX_BITMASK_SET( PtrFeat->PortMask, PLX_FLAG_PORT_INT_DS_0  );
            PEX_BITMASK_SET( PtrFeat->PortMask, PLX_FLAG_PORT_INT_DS_4  );
            PEX_BITMASK_SET( PtrFeat->PortMask, PLX_FLAG_PORT_INT_DS_12 );
            PEX_BITMASK_SET( PtrFeat->PortMask, PLX_FLAG_PORT_INT_UP_0  );
            PEX_BITMASK_SET( PtrFeat->PortMask, PLX_FLAG_PORT_INT_UP_4  );
            PEX_BITMASK_SET( PtrFeat->PortMask, PLX_FLAG_PORT_INT_UP_12 );
            PEX_BITMASK_SET( PtrFeat->PortMask, PLX_FLAG_PORT_GEP );
            PEX_BITMASK_SET( PtrFeat->PortMask, PLX_FLAG_PORT_GEP_DS );
            PEX_BITMASK_SET( PtrFeat->PortMask, PLX_FLAG_PORT_MPT0 );
            PEX_BITMASK_SET( PtrFeat->PortMask, PLX_FLAG_PORT_MPT1);
            PEX_BITMASK_SET( PtrFeat->PortMask, PLX_FLAG_PORT_MPT2 );
            PEX_BITMASK_SET( PtrFeat->PortMask, PLX_FLAG_PORT_MPT3 );
            break;

        case 0xA064:
            // Station 0, 1, 3, 4, 2 x2 ports
            PtrFeat->StnCount    = 5;
            PtrFeat->PortsPerStn = 16;
            PtrFeat->StnMask     = 0x9B;
            PEX_BITMASK_SET_DW( PtrFeat->PortMask, 0, 0xFFFFFFFF );
            PEX_BITMASK_SET_DW( PtrFeat->PortMask, 1, 0xFFFF0000 );
            PEX_BITMASK_SET_DW( PtrFeat->PortMask, 2, 0x0000FFFF );
            PEX_BITMASK_SET_DW( PtrFeat->PortMask, 3, 0x00300000 );
            PEX_BITMASK_SET( PtrFeat->PortMask, PLX_FLAG_PORT_STN_REGS_S0 );
            PEX_BITMASK_SET( PtrFeat->PortMask, PLX_FLAG_PORT_STN_REGS_S1 );
            PEX_BITMASK_SET( PtrFeat->PortMask, PLX_FLAG_PORT_STN_REGS_S3 );
            PEX_BITMASK_SET( PtrFeat->PortMask, PLX_FLAG_PORT_STN_REGS_S4 );

            // Internal ports
            PEX_BITMASK_SET( PtrFeat->PortMask, PLX_FLAG_PORT_INT_MGMT  );
            PEX_BITMASK_SET( PtrFeat->PortMask, PLX_FLAG_PORT_INT_DS_0  );
            PEX_BITMASK_SET( PtrFeat->PortMask, PLX_FLAG_PORT_INT_DS_4  );
            PEX_BITMASK_SET( PtrFeat->PortMask, PLX_FLAG_PORT_INT_DS_8  );
            PEX_BITMASK_SET( PtrFeat->PortMask, PLX_FLAG_PORT_INT_DS_12 );
            PEX_BITMASK_SET( PtrFeat->PortMask, PLX_FLAG_PORT_INT_UP_0  );
            PEX_BITMASK_SET( PtrFeat->PortMask, PLX_FLAG_PORT_INT_UP_4  );
            PEX_BITMASK_SET( PtrFeat->PortMask, PLX_FLAG_PORT_INT_UP_8  );
            PEX_BITMASK_SET( PtrFeat->PortMask, PLX_FLAG_PORT_INT_UP_12 );
            break;

        case 0xA080:
            // Station 0-4 2 x2 ports
            PtrFeat->StnCount    = 6;
            PtrFeat->PortsPerStn = 16;
            PtrFeat->StnMask     = 0x9F;
            PEX_BITMASK_SET_DW( PtrFeat->PortMask, 0, 0xFFFFFFFF );
            PEX_BITMASK_SET_DW( PtrFeat->PortMask, 1, 0xFFFFFFFF );
            PEX_BITMASK_SET_DW( PtrFeat->PortMask, 2, 0x0000FFFF );
            PEX_BITMASK_SET_DW( PtrFeat->PortMask, 3, 0x00300000 );
            PEX_BITMASK_SET( PtrFeat->PortMask, PLX_FLAG_PORT_STN_REGS_S0 );
            PEX_BITMASK_SET( PtrFeat->PortMask, PLX_FLAG_PORT_STN_REGS_S1 );
            PEX_BITMASK_SET( PtrFeat->PortMask, PLX_FLAG_PORT_STN_REGS_S2 );
            PEX_BITMASK_SET( PtrFeat->PortMask, PLX_FLAG_PORT_STN_REGS_S3 );
            PEX_BITMASK_SET( PtrFeat->PortMask, PLX_FLAG_PORT_STN_REGS_S4 );

            // Internal ports
            PEX_BITMASK_SET( PtrFeat->PortMask, PLX_FLAG_PORT_INT_MGMT  );
            PEX_BITMASK_SET( PtrFeat->PortMask, PLX_FLAG_PORT_INT_DS_0  );
            PEX_BITMASK_SET( PtrFeat->PortMask, PLX_FLAG_PORT_INT_DS_4  );
            PEX_BITMASK_SET( PtrFeat->PortMask, PLX_FLAG_PORT_INT_DS_8  );
            PEX_BITMASK_SET( PtrFeat->PortMask, PLX_FLAG_PORT_INT_DS_12 );
            PEX_BITMASK_SET( PtrFeat->PortMask, PLX_FLAG_PORT_INT_UP_0  );
            PEX_BITMASK_SET( PtrFeat->PortMask, PLX_FLAG_PORT_INT_UP_4  );
            PEX_BITMASK_SET( PtrFeat->PortMask, PLX_FLAG_PORT_INT_UP_8  );
            PEX_BITMASK_SET( PtrFeat->PortMask, PLX_FLAG_PORT_INT_UP_12 );
            break;

        case 0xA096:
            // Station 0-5, 2 x2 ports
            PtrFeat->StnCount    = 7;
            PtrFeat->PortsPerStn = 16;
            PtrFeat->StnMask     = 0xBF;
            PEX_BITMASK_SET_DW( PtrFeat->PortMask, 0, 0xFFFFFFFF );
            PEX_BITMASK_SET_DW( PtrFeat->PortMask, 1, 0xFFFFFFFF );
            PEX_BITMASK_SET_DW( PtrFeat->PortMask, 2, 0xFFFFFFFF );
            PEX_BITMASK_SET_DW( PtrFeat->PortMask, 3, 0x00300000 );
            PEX_BITMASK_SET( PtrFeat->PortMask, PLX_FLAG_PORT_STN_REGS_S0 );
            PEX_BITMASK_SET( PtrFeat->PortMask, PLX_FLAG_PORT_STN_REGS_S1 );
            PEX_BITMASK_SET( PtrFeat->PortMask, PLX_FLAG_PORT_STN_REGS_S2 );
            PEX_BITMASK_SET( PtrFeat->PortMask, PLX_FLAG_PORT_STN_REGS_S3 );
            PEX_BITMASK_SET( PtrFeat->PortMask, PLX_FLAG_PORT_STN_REGS_S4 );
            PEX_BITMASK_SET( PtrFeat->PortMask, PLX_FLAG_PORT_STN_REGS_S5 );

            // Internal ports
            PEX_BITMASK_SET( PtrFeat->PortMask, PLX_FLAG_PORT_INT_MGMT  );
            PEX_BITMASK_SET( PtrFeat->PortMask, PLX_FLAG_PORT_INT_DS_0  );
            PEX_BITMASK_SET( PtrFeat->PortMask, PLX_FLAG_PORT_INT_DS_4  );
            PEX_BITMASK_SET( PtrFeat->PortMask, PLX_FLAG_PORT_INT_DS_8  );
            PEX_BITMASK_SET( PtrFeat->PortMask, PLX_FLAG_PORT_INT_DS_12 );
            PEX_BITMASK_SET( PtrFeat->PortMask, PLX_FLAG_PORT_INT_UP_0  );
            PEX_BITMASK_SET( PtrFeat->PortMask, PLX_FLAG_PORT_INT_UP_4  );
            PEX_BITMASK_SET( PtrFeat->PortMask, PLX_FLAG_PORT_INT_UP_8  );
            PEX_BITMASK_SET( PtrFeat->PortMask, PLX_FLAG_PORT_INT_UP_12 );
            PEX_BITMASK_SET( PtrFeat->PortMask, PLX_FLAG_PORT_GEP );
            PEX_BITMASK_SET( PtrFeat->PortMask, PLX_FLAG_PORT_GEP_DS );
            PEX_BITMASK_SET( PtrFeat->PortMask, PLX_FLAG_PORT_MPT0 );
            PEX_BITMASK_SET( PtrFeat->PortMask, PLX_FLAG_PORT_MPT1);
            PEX_BITMASK_SET( PtrFeat->PortMask, PLX_FLAG_PORT_MPT2 );
            PEX_BITMASK_SET( PtrFeat->PortMask, PLX_FLAG_PORT_MPT3 );
            break;

        case 0x0072:    // Atlas2 72 Lane
        case 0x0048:    // HW errata on Atlas2 A0; 0x0048 signifies 72lane SKU
            // Stations 0-1, 5-6 (each with 8 x2 ports), Station 8 - 4 x2 ports
            PtrFeat->StnCount = 5;
            PtrFeat->PortsPerStn = 16;
            PtrFeat->StnMask = 0x163;
            PEX_BITMASK_SET_DW( PtrFeat->PortMask, 0, 0x55555555 );
            PEX_BITMASK_SET_DW( PtrFeat->PortMask, 2, 0x55550000 );
            PEX_BITMASK_SET_DW( PtrFeat->PortMask, 3, 0x00005555 );
            PEX_BITMASK_SET_DW( PtrFeat->PortMask, 4, 0x00000055 );
            PEX_BITMASK_SET( PtrFeat->PortMask, PLX_FLAG_PORT_STN_REGS_S0 );
            PEX_BITMASK_SET( PtrFeat->PortMask, PLX_FLAG_PORT_STN_REGS_S1 );
            PEX_BITMASK_SET( PtrFeat->PortMask, PLX_FLAG_PORT_STN_REGS_S2 );
            PEX_BITMASK_SET( PtrFeat->PortMask, PLX_FLAG_PORT_STN_REGS_S3 );
            PEX_BITMASK_SET( PtrFeat->PortMask, PLX_FLAG_PORT_STN_REGS_S4 );
            PEX_BITMASK_SET( PtrFeat->PortMask, PLX_FLAG_PORT_STN_REGS_S5 );

            // Internal ports
            PEX_BITMASK_SET( PtrFeat->PortMask, PLX_FLAG_PORT_INT_MGMT );
            PEX_BITMASK_SET( PtrFeat->PortMask, PLX_FLAG_PORT_INT_DS_0 );
            PEX_BITMASK_SET( PtrFeat->PortMask, PLX_FLAG_PORT_INT_DS_4 );
            PEX_BITMASK_SET( PtrFeat->PortMask, PLX_FLAG_PORT_INT_DS_8 );
            PEX_BITMASK_SET( PtrFeat->PortMask, PLX_FLAG_PORT_INT_DS_12 );
            PEX_BITMASK_SET( PtrFeat->PortMask, PLX_FLAG_PORT_INT_DS_16 );
            PEX_BITMASK_SET( PtrFeat->PortMask, PLX_FLAG_PORT_INT_UP_0 );
            PEX_BITMASK_SET( PtrFeat->PortMask, PLX_FLAG_PORT_INT_UP_4 );
            PEX_BITMASK_SET( PtrFeat->PortMask, PLX_FLAG_PORT_INT_UP_8 );
            PEX_BITMASK_SET( PtrFeat->PortMask, PLX_FLAG_PORT_INT_UP_12 );
            PEX_BITMASK_SET( PtrFeat->PortMask, PLX_FLAG_PORT_INT_UP_16 );
            PEX_BITMASK_SET( PtrFeat->PortMask, PLX_FLAG_PORT_GEP );
            PEX_BITMASK_SET( PtrFeat->PortMask, PLX_FLAG_PORT_GEP_DS );
            PEX_BITMASK_SET( PtrFeat->PortMask, PLX_FLAG_PORT_ATLAS2_MPT0 );
            PEX_BITMASK_SET( PtrFeat->PortMask, PLX_FLAG_PORT_ATLAS2_MPT1 );
            PEX_BITMASK_SET( PtrFeat->PortMask, PLX_FLAG_PORT_ATLAS2_MPT2 );
            PEX_BITMASK_SET( PtrFeat->PortMask, PLX_FLAG_PORT_ATLAS2_MPT3 );
            PEX_BITMASK_SET( PtrFeat->PortMask, PLX_FLAG_PORT_ATLAS2_MPT4 );
            PEX_BITMASK_SET( PtrFeat->PortMask, PLX_FLAG_PORT_ATLAS2_MPT5 );
            PEX_BITMASK_SET( PtrFeat->PortMask, PLX_FLAG_PORT_ATLAS2_MPT6 );
            PEX_BITMASK_SET( PtrFeat->PortMask, PLX_FLAG_PORT_ATLAS2_MPT7 );
            break;

        case 0x0088:    // Atlas2 88 Lane
        case 0x0064:    // HW errata on Atlas2 A0; 0x0064 signifies 88lane SKU
            // Stations 0-2, 5-6 (each with 8 x2 ports), Station 8 - 4 x2 ports
            PtrFeat->StnCount = 6;
            PtrFeat->PortsPerStn = 16;
            PtrFeat->StnMask = 0x167;
            PEX_BITMASK_SET_DW( PtrFeat->PortMask, 0, 0x55555555 );
            PEX_BITMASK_SET_DW( PtrFeat->PortMask, 1, 0x00005555 );
            PEX_BITMASK_SET_DW( PtrFeat->PortMask, 2, 0x55550000 );
            PEX_BITMASK_SET_DW( PtrFeat->PortMask, 3, 0x00005555 );
            PEX_BITMASK_SET_DW( PtrFeat->PortMask, 4, 0x00000055 );
            PEX_BITMASK_SET( PtrFeat->PortMask, PLX_FLAG_PORT_STN_REGS_S0 );
            PEX_BITMASK_SET( PtrFeat->PortMask, PLX_FLAG_PORT_STN_REGS_S1 );
            PEX_BITMASK_SET( PtrFeat->PortMask, PLX_FLAG_PORT_STN_REGS_S2 );
            PEX_BITMASK_SET( PtrFeat->PortMask, PLX_FLAG_PORT_STN_REGS_S3 );
            PEX_BITMASK_SET( PtrFeat->PortMask, PLX_FLAG_PORT_STN_REGS_S4 );
            PEX_BITMASK_SET( PtrFeat->PortMask, PLX_FLAG_PORT_STN_REGS_S5 );

            // Internal ports
            PEX_BITMASK_SET( PtrFeat->PortMask, PLX_FLAG_PORT_INT_MGMT );
            PEX_BITMASK_SET( PtrFeat->PortMask, PLX_FLAG_PORT_INT_DS_0 );
            PEX_BITMASK_SET( PtrFeat->PortMask, PLX_FLAG_PORT_INT_DS_4 );
            PEX_BITMASK_SET( PtrFeat->PortMask, PLX_FLAG_PORT_INT_DS_8 );
            PEX_BITMASK_SET( PtrFeat->PortMask, PLX_FLAG_PORT_INT_DS_12 );
            PEX_BITMASK_SET( PtrFeat->PortMask, PLX_FLAG_PORT_INT_DS_16 );
            PEX_BITMASK_SET( PtrFeat->PortMask, PLX_FLAG_PORT_INT_UP_0 );
            PEX_BITMASK_SET( PtrFeat->PortMask, PLX_FLAG_PORT_INT_UP_4 );
            PEX_BITMASK_SET( PtrFeat->PortMask, PLX_FLAG_PORT_INT_UP_8 );
            PEX_BITMASK_SET( PtrFeat->PortMask, PLX_FLAG_PORT_INT_UP_12 );
            PEX_BITMASK_SET( PtrFeat->PortMask, PLX_FLAG_PORT_INT_UP_16 );
            PEX_BITMASK_SET( PtrFeat->PortMask, PLX_FLAG_PORT_GEP );
            PEX_BITMASK_SET( PtrFeat->PortMask, PLX_FLAG_PORT_GEP_DS );
            PEX_BITMASK_SET( PtrFeat->PortMask, PLX_FLAG_PORT_ATLAS2_MPT0 );
            PEX_BITMASK_SET( PtrFeat->PortMask, PLX_FLAG_PORT_ATLAS2_MPT1 );
            PEX_BITMASK_SET( PtrFeat->PortMask, PLX_FLAG_PORT_ATLAS2_MPT2 );
            PEX_BITMASK_SET( PtrFeat->PortMask, PLX_FLAG_PORT_ATLAS2_MPT3 );
            PEX_BITMASK_SET( PtrFeat->PortMask, PLX_FLAG_PORT_ATLAS2_MPT4 );
            PEX_BITMASK_SET( PtrFeat->PortMask, PLX_FLAG_PORT_ATLAS2_MPT5 );
            PEX_BITMASK_SET( PtrFeat->PortMask, PLX_FLAG_PORT_ATLAS2_MPT6 );
            PEX_BITMASK_SET( PtrFeat->PortMask, PLX_FLAG_PORT_ATLAS2_MPT7 );
            break;

        case 0x0104:    // Atlas2 104 Lane
        case 0x0080:    // HW errata on Atlas2 A0; 0x0048 signifies 104lane SKU
            // Stations 0-2, 5-7 (each with 8 x2 ports), Station 8 - 4 x2 ports
            PtrFeat->StnCount = 7;
            PtrFeat->PortsPerStn = 16;
            PtrFeat->StnMask = 0x1E7;
            PEX_BITMASK_SET_DW( PtrFeat->PortMask, 0, 0x55555555 );
            PEX_BITMASK_SET_DW( PtrFeat->PortMask, 1, 0x00005555 );
            PEX_BITMASK_SET_DW( PtrFeat->PortMask, 2, 0x55550000 );
            PEX_BITMASK_SET_DW( PtrFeat->PortMask, 3, 0x55555555 );
            PEX_BITMASK_SET_DW( PtrFeat->PortMask, 4, 0x00000055 );
            PEX_BITMASK_SET( PtrFeat->PortMask, PLX_FLAG_PORT_STN_REGS_S0 );
            PEX_BITMASK_SET( PtrFeat->PortMask, PLX_FLAG_PORT_STN_REGS_S1 );
            PEX_BITMASK_SET( PtrFeat->PortMask, PLX_FLAG_PORT_STN_REGS_S2 );
            PEX_BITMASK_SET( PtrFeat->PortMask, PLX_FLAG_PORT_STN_REGS_S3 );
            PEX_BITMASK_SET( PtrFeat->PortMask, PLX_FLAG_PORT_STN_REGS_S4 );
            PEX_BITMASK_SET( PtrFeat->PortMask, PLX_FLAG_PORT_STN_REGS_S5 );

            // Internal ports
            PEX_BITMASK_SET( PtrFeat->PortMask, PLX_FLAG_PORT_INT_MGMT );
            PEX_BITMASK_SET( PtrFeat->PortMask, PLX_FLAG_PORT_INT_DS_0 );
            PEX_BITMASK_SET( PtrFeat->PortMask, PLX_FLAG_PORT_INT_DS_4 );
            PEX_BITMASK_SET( PtrFeat->PortMask, PLX_FLAG_PORT_INT_DS_8 );
            PEX_BITMASK_SET( PtrFeat->PortMask, PLX_FLAG_PORT_INT_DS_12 );
            PEX_BITMASK_SET( PtrFeat->PortMask, PLX_FLAG_PORT_INT_DS_16 );
            PEX_BITMASK_SET( PtrFeat->PortMask, PLX_FLAG_PORT_INT_UP_0 );
            PEX_BITMASK_SET( PtrFeat->PortMask, PLX_FLAG_PORT_INT_UP_4 );
            PEX_BITMASK_SET( PtrFeat->PortMask, PLX_FLAG_PORT_INT_UP_8 );
            PEX_BITMASK_SET( PtrFeat->PortMask, PLX_FLAG_PORT_INT_UP_12 );
            PEX_BITMASK_SET( PtrFeat->PortMask, PLX_FLAG_PORT_INT_UP_16 );
            PEX_BITMASK_SET( PtrFeat->PortMask, PLX_FLAG_PORT_GEP );
            PEX_BITMASK_SET( PtrFeat->PortMask, PLX_FLAG_PORT_GEP_DS );
            PEX_BITMASK_SET( PtrFeat->PortMask, PLX_FLAG_PORT_ATLAS2_MPT0 );
            PEX_BITMASK_SET( PtrFeat->PortMask, PLX_FLAG_PORT_ATLAS2_MPT1 );
            PEX_BITMASK_SET( PtrFeat->PortMask, PLX_FLAG_PORT_ATLAS2_MPT2 );
            PEX_BITMASK_SET( PtrFeat->PortMask, PLX_FLAG_PORT_ATLAS2_MPT3 );
            PEX_BITMASK_SET( PtrFeat->PortMask, PLX_FLAG_PORT_ATLAS2_MPT4 );
            PEX_BITMASK_SET( PtrFeat->PortMask, PLX_FLAG_PORT_ATLAS2_MPT5 );
            PEX_BITMASK_SET( PtrFeat->PortMask, PLX_FLAG_PORT_ATLAS2_MPT6 );
            PEX_BITMASK_SET( PtrFeat->PortMask, PLX_FLAG_PORT_ATLAS2_MPT7 );
            break;

        case 0x0136:    // Atlas2 136 Lane
        case 0x0096:    // HW errata on Atlas2 A0; 0x0096 signifies 136lane SKU
            // Station 0-7 (each with 8 x2 ports), Station 8 - 4 x2 ports
            PtrFeat->StnCount    = 9;
            PtrFeat->PortsPerStn = 16;
            PtrFeat->StnMask     = 0x1FF;
            PEX_BITMASK_SET_DW( PtrFeat->PortMask, 0, 0x55555555 );
            PEX_BITMASK_SET_DW( PtrFeat->PortMask, 1, 0x55555555 );
            PEX_BITMASK_SET_DW( PtrFeat->PortMask, 2, 0x55555555 );
            PEX_BITMASK_SET_DW( PtrFeat->PortMask, 3, 0x55555555 );
            PEX_BITMASK_SET_DW( PtrFeat->PortMask, 4, 0x00000055 );
            PEX_BITMASK_SET( PtrFeat->PortMask, PLX_FLAG_PORT_STN_REGS_S0 );
            PEX_BITMASK_SET( PtrFeat->PortMask, PLX_FLAG_PORT_STN_REGS_S1 );
            PEX_BITMASK_SET( PtrFeat->PortMask, PLX_FLAG_PORT_STN_REGS_S2 );
            PEX_BITMASK_SET( PtrFeat->PortMask, PLX_FLAG_PORT_STN_REGS_S3 );
            PEX_BITMASK_SET( PtrFeat->PortMask, PLX_FLAG_PORT_STN_REGS_S4 );
            PEX_BITMASK_SET( PtrFeat->PortMask, PLX_FLAG_PORT_STN_REGS_S5 );

            // Internal ports
            PEX_BITMASK_SET( PtrFeat->PortMask, PLX_FLAG_PORT_INT_MGMT );
            PEX_BITMASK_SET( PtrFeat->PortMask, PLX_FLAG_PORT_INT_DS_0 );
            PEX_BITMASK_SET( PtrFeat->PortMask, PLX_FLAG_PORT_INT_DS_4 );
            PEX_BITMASK_SET( PtrFeat->PortMask, PLX_FLAG_PORT_INT_DS_8 );
            PEX_BITMASK_SET( PtrFeat->PortMask, PLX_FLAG_PORT_INT_DS_12 );
            PEX_BITMASK_SET( PtrFeat->PortMask, PLX_FLAG_PORT_INT_DS_16 );
            PEX_BITMASK_SET( PtrFeat->PortMask, PLX_FLAG_PORT_INT_UP_0 );
            PEX_BITMASK_SET( PtrFeat->PortMask, PLX_FLAG_PORT_INT_UP_4 );
            PEX_BITMASK_SET( PtrFeat->PortMask, PLX_FLAG_PORT_INT_UP_8 );
            PEX_BITMASK_SET( PtrFeat->PortMask, PLX_FLAG_PORT_INT_UP_12 );
            PEX_BITMASK_SET( PtrFeat->PortMask, PLX_FLAG_PORT_INT_UP_16 );
            PEX_BITMASK_SET( PtrFeat->PortMask, PLX_FLAG_PORT_GEP );
            PEX_BITMASK_SET( PtrFeat->PortMask, PLX_FLAG_PORT_GEP_DS );
            PEX_BITMASK_SET( PtrFeat->PortMask, PLX_FLAG_PORT_ATLAS2_MPT0 );
            PEX_BITMASK_SET( PtrFeat->PortMask, PLX_FLAG_PORT_ATLAS2_MPT1 );
            PEX_BITMASK_SET( PtrFeat->PortMask, PLX_FLAG_PORT_ATLAS2_MPT2 );
            PEX_BITMASK_SET( PtrFeat->PortMask, PLX_FLAG_PORT_ATLAS2_MPT3 );
            PEX_BITMASK_SET( PtrFeat->PortMask, PLX_FLAG_PORT_ATLAS2_MPT4 );
            PEX_BITMASK_SET( PtrFeat->PortMask, PLX_FLAG_PORT_ATLAS2_MPT5 );
            PEX_BITMASK_SET( PtrFeat->PortMask, PLX_FLAG_PORT_ATLAS2_MPT6 );
            PEX_BITMASK_SET( PtrFeat->PortMask, PLX_FLAG_PORT_ATLAS2_MPT7 );
            break;

        case 0x0144: // Atlas2 144 lanes
        case 0x0128: // HW errata on Atlas2 A0; 0x0128 signifies 144lane SKU
            // Station 0-8 (each with 8 x2 ports), Station 8 - 8 x2 ports
            PtrFeat->StnCount    = 9;
            PtrFeat->PortsPerStn = 16;
            PtrFeat->StnMask     = 0x1FF;
            PEX_BITMASK_SET_DW( PtrFeat->PortMask, 0, 0x55555555 );
            PEX_BITMASK_SET_DW( PtrFeat->PortMask, 1, 0x55555555 );
            PEX_BITMASK_SET_DW( PtrFeat->PortMask, 2, 0x55555555 );
            PEX_BITMASK_SET_DW( PtrFeat->PortMask, 3, 0x55555555 );
            PEX_BITMASK_SET_DW( PtrFeat->PortMask, 4, 0x00005555 );
            PEX_BITMASK_SET( PtrFeat->PortMask, PLX_FLAG_PORT_STN_REGS_S0 );
            PEX_BITMASK_SET( PtrFeat->PortMask, PLX_FLAG_PORT_STN_REGS_S1 );
            PEX_BITMASK_SET( PtrFeat->PortMask, PLX_FLAG_PORT_STN_REGS_S2 );
            PEX_BITMASK_SET( PtrFeat->PortMask, PLX_FLAG_PORT_STN_REGS_S3 );
            PEX_BITMASK_SET( PtrFeat->PortMask, PLX_FLAG_PORT_STN_REGS_S4 );
            PEX_BITMASK_SET( PtrFeat->PortMask, PLX_FLAG_PORT_STN_REGS_S5 );

            // Internal ports
            PEX_BITMASK_SET( PtrFeat->PortMask, PLX_FLAG_PORT_INT_MGMT );
            PEX_BITMASK_SET( PtrFeat->PortMask, PLX_FLAG_PORT_INT_DS_0 );
            PEX_BITMASK_SET( PtrFeat->PortMask, PLX_FLAG_PORT_INT_DS_4 );
            PEX_BITMASK_SET( PtrFeat->PortMask, PLX_FLAG_PORT_INT_DS_8 );
            PEX_BITMASK_SET( PtrFeat->PortMask, PLX_FLAG_PORT_INT_DS_12 );
            PEX_BITMASK_SET( PtrFeat->PortMask, PLX_FLAG_PORT_INT_DS_16 );
            PEX_BITMASK_SET( PtrFeat->PortMask, PLX_FLAG_PORT_INT_UP_0 );
            PEX_BITMASK_SET( PtrFeat->PortMask, PLX_FLAG_PORT_INT_UP_4 );
            PEX_BITMASK_SET( PtrFeat->PortMask, PLX_FLAG_PORT_INT_UP_8 );
            PEX_BITMASK_SET( PtrFeat->PortMask, PLX_FLAG_PORT_INT_UP_12 );
            PEX_BITMASK_SET( PtrFeat->PortMask, PLX_FLAG_PORT_INT_UP_16 );
            PEX_BITMASK_SET( PtrFeat->PortMask, PLX_FLAG_PORT_GEP );
            PEX_BITMASK_SET( PtrFeat->PortMask, PLX_FLAG_PORT_GEP_DS );
            PEX_BITMASK_SET( PtrFeat->PortMask, PLX_FLAG_PORT_ATLAS2_MPT0 );
            PEX_BITMASK_SET( PtrFeat->PortMask, PLX_FLAG_PORT_ATLAS2_MPT1 );
            PEX_BITMASK_SET( PtrFeat->PortMask, PLX_FLAG_PORT_ATLAS2_MPT2 );
            PEX_BITMASK_SET( PtrFeat->PortMask, PLX_FLAG_PORT_ATLAS2_MPT3 );
            PEX_BITMASK_SET( PtrFeat->PortMask, PLX_FLAG_PORT_ATLAS2_MPT4 );
            PEX_BITMASK_SET( PtrFeat->PortMask, PLX_FLAG_PORT_ATLAS2_MPT5 );
            PEX_BITMASK_SET( PtrFeat->PortMask, PLX_FLAG_PORT_ATLAS2_MPT6 );
            PEX_BITMASK_SET( PtrFeat->PortMask, PLX_FLAG_PORT_ATLAS2_MPT7 );
            break;

        default:
            // For unsupported chips, set default
            PEX_BITMASK_SET_DW( PtrFeat->PortMask, 0, 0x00000001 );
            ErrorPrintf(("ERROR: Port mask not set for %04X\n", ChipID));
            return PLX_STATUS_UNSUPPORTED;
    }

    return PLX_STATUS_OK;
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
        return PLX_STATUS_NULL_PARAM;
    }

    // Verify device object
    if (!IsObjectValid(pDevice))
    {
        return PLX_STATUS_INVALID_OBJECT;
    }

    RtlZeroMemory( &IoBuffer, sizeof(PLX_PARAMS) );

    IoBuffer.Key = pDevice->Key;

    PlxIoMessage(
        pDevice,
        PLX_IOCTL_GET_PORT_PROPERTIES,
        &IoBuffer
        );

    if (IoBuffer.ReturnCode == PLX_STATUS_OK)
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
    {
        return PLX_STATUS_INVALID_OBJECT;
    }

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
        {
            *pStatus = PLX_STATUS_INVALID_OBJECT;
        }
        return PCI_CFG_RD_ERR_VAL_32;
    }

    // Verify offset
    if (offset >= PCIE_CONFIG_SPACE_SIZE)
    {
        if (pStatus != NULL)
        {
            *pStatus = PLX_STATUS_INVALID_OFFSET;
        }
        return PCI_CFG_RD_ERR_VAL_32;
    }

    RtlZeroMemory( &IoBuffer, sizeof(PLX_PARAMS) );

    IoBuffer.Key      = pDevice->Key;
    IoBuffer.value[0] = offset;
    IoBuffer.value[1] = PCI_CFG_RD_ERR_VAL_32;

    PlxIoMessage(
        pDevice,
        PLX_IOCTL_PCI_REGISTER_READ,
        &IoBuffer
        );

    if (pStatus != NULL)
    {
        *pStatus = IoBuffer.ReturnCode;
    }

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
    {
        return PLX_STATUS_INVALID_OBJECT;
    }

    // Verify offset
    if (offset >= PCIE_CONFIG_SPACE_SIZE)
    {
        return PLX_STATUS_INVALID_OFFSET;
    }

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
    PLX_STATUS        status;
    PLX_DEVICE_OBJECT Device;


    // Verify offset
    if ( (offset & 0x3) || (offset >= PCIE_CONFIG_SPACE_SIZE) )
    {
        if (pStatus != NULL)
        {
            *pStatus = PLX_STATUS_INVALID_OFFSET;
        }
        return PCI_CFG_RD_ERR_VAL_32;
    }

    // Setup to select any device
    memset( &IoBuffer.Key, PCI_FIELD_IGNORE, sizeof(PLX_DEVICE_KEY) );
    RtlZeroMemory( &Device, sizeof(PLX_DEVICE_OBJECT) );

    status = PlxPci_DeviceOpen( &IoBuffer.Key, &Device );
    if (status != PLX_STATUS_OK)
    {
        if (pStatus != NULL)
        {
            *pStatus = status;
        }
        return PCI_CFG_RD_ERR_VAL_32;
    }

    RtlZeroMemory( &IoBuffer, sizeof(PLX_PARAMS) );

    IoBuffer.Key.bus      = bus;
    IoBuffer.Key.slot     = slot;
    IoBuffer.Key.function = function;
    IoBuffer.value[0]     = offset;
    IoBuffer.value[1]     = PCI_CFG_RD_ERR_VAL_32;

    PlxIoMessage(
        &Device,
        PLX_IOCTL_PCI_REG_READ_BYPASS_OS,
        &IoBuffer
        );

    if (pStatus != NULL)
    {
        *pStatus = IoBuffer.ReturnCode;
    }

    PlxPci_DeviceClose( &Device );

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
    PLX_STATUS        status;
    PLX_DEVICE_OBJECT Device;


    // Verify offset
    if ( (offset & 0x3) || (offset >= PCIE_CONFIG_SPACE_SIZE) )
    {
        return PLX_STATUS_INVALID_OFFSET;
    }

    // Setup to select any device
    memset( &IoBuffer.Key, PCI_FIELD_IGNORE, sizeof(PLX_DEVICE_KEY) );
    RtlZeroMemory( &Device, sizeof(PLX_DEVICE_OBJECT) );

    status = PlxPci_DeviceOpen( &IoBuffer.Key, &Device );
    if (status != PLX_STATUS_OK)
    {
        return status;
    }

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

    PlxPci_DeviceClose( &Device );

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
        {
            *pStatus = PLX_STATUS_INVALID_OBJECT;
        }
        return PCI_CFG_RD_ERR_VAL_32;
    }

    // Verify offset
    if (offset >= PCIE_CONFIG_SPACE_SIZE)
    {
        if (pStatus != NULL)
        {
            *pStatus = PLX_STATUS_INVALID_OFFSET;
        }
        return PCI_CFG_RD_ERR_VAL_32;
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
    {
        *pStatus = IoBuffer.ReturnCode;
    }

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
    {
        return PLX_STATUS_INVALID_OBJECT;
    }

    // Verify offset
    if (offset >= PCIE_CONFIG_SPACE_SIZE)
    {
        return PLX_STATUS_INVALID_OFFSET;
    }

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
    {
        return PLX_STATUS_INVALID_OBJECT;
    }

    RtlZeroMemory( &IoBuffer, sizeof(PLX_PARAMS) );

    IoBuffer.Key      = pDevice->Key;
    IoBuffer.value[0] = offset;

    PlxIoMessage(
        pDevice,
        PLX_IOCTL_MAPPED_REGISTER_READ,
        &IoBuffer
        );

    if (pStatus != NULL)
    {
        *pStatus = IoBuffer.ReturnCode;
    }

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
    {
        return PLX_STATUS_INVALID_OBJECT;
    }

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
        {
            *pStatus = PLX_STATUS_INVALID_OBJECT;
        }
        return PCI_CFG_RD_ERR_VAL_32;
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
    {
        *pStatus = IoBuffer.ReturnCode;
    }

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
    {
        return PLX_STATUS_INVALID_OBJECT;
    }

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
    {
        return PLX_STATUS_NULL_PARAM;
    }

    // Set default value
    RtlZeroMemory( pBarProp, sizeof(PLX_PCI_BAR_PROP) );

    // Verify device object
    if (!IsObjectValid(pDevice))
    {
        return PLX_STATUS_INVALID_OBJECT;
    }

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
            return PLX_STATUS_INVALID_ACCESS;
    }

    RtlZeroMemory( &IoBuffer, sizeof(PLX_PARAMS) );

    IoBuffer.Key      = pDevice->Key;
    IoBuffer.value[0] = BarIndex;

    PlxIoMessage(
        pDevice,
        PLX_IOCTL_PCI_BAR_PROPERTIES,
        &IoBuffer
        );

    if (IoBuffer.ReturnCode == PLX_STATUS_OK)
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
    PLX_STATUS       status;
    PLX_PCI_BAR_PROP BarProp;


    if (pVa == NULL)
    {
        return PLX_STATUS_NULL_PARAM;
    }

    // Set default value
    *(PLX_UINT_PTR*)pVa = 0;

    // Verify device object
    if (!IsObjectValid(pDevice))
    {
        return PLX_STATUS_INVALID_OBJECT;
    }

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
            return PLX_STATUS_INVALID_DATA;
    }

    // Check if mapping has already been performed
    if (pDevice->PciBarVa[BarIndex] != 0)
    {
        *(PLX_UINT_PTR*)pVa = (PLX_UINT_PTR)pDevice->PciBarVa[BarIndex];

        // Increment map count
        pDevice->BarMapRef[BarIndex]++;

        return PLX_STATUS_OK;
    }

    // Get the PCI BAR properties
    status =
        PlxPci_PciBarProperties(
            pDevice,
            BarIndex,
            &BarProp
            );

    if (status != PLX_STATUS_OK)
    {
        return status;
    }

    // Verify BAR exists and is memory type
    if ((BarProp.Physical == 0) || (BarProp.Size == 0) || (BarProp.Flags & PLX_BAR_FLAG_IO))
    {
        return PLX_STATUS_INVALID_ADDR;
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

        if (IoBuffer.ReturnCode != PLX_STATUS_OK)
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
        return PLX_STATUS_INSUFFICIENT_RES;
    }

    // Store BAR properties
    pDevice->PciBar[BarIndex] = BarProp;

    // Add the offset if any
    pDevice->PciBarVa[BarIndex] += BarOffset;

    // Provide virtual address
    *(PLX_UINT_PTR*)pVa = pDevice->PciBarVa[BarIndex];

    // Set map count
    pDevice->BarMapRef[BarIndex] = 1;

    return PLX_STATUS_OK;
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
    {
        return PLX_STATUS_NULL_PARAM;
    }

    if (*(PLX_UINT_PTR*)pVa == 0)
    {
        return PLX_STATUS_INVALID_ADDR;
    }

    // Verify device object
    if (!IsObjectValid(pDevice))
    {
        return PLX_STATUS_INVALID_OBJECT;
    }

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
                    return PLX_STATUS_INVALID_ADDR;
                }

                // Clear internal data
                pDevice->PciBarVa[BarIndex] = 0;
            }

            // Clear address
            *(PLX_UINT_PTR*)pVa = 0;

            return PLX_STATUS_OK;
        }
    }

    return PLX_STATUS_INVALID_ADDR;
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
        {
            *pStatus = PLX_STATUS_INVALID_OBJECT;
        }
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
    {
        *pStatus = IoBuffer.ReturnCode;
    }

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
        {
            *pStatus = PLX_STATUS_INVALID_OBJECT;
        }
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
    {
        *pStatus = IoBuffer.ReturnCode;
    }

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
        {
            *pCrc = 0;
        }
        return PLX_STATUS_INVALID_OBJECT;
    }

    RtlZeroMemory( &IoBuffer, sizeof(PLX_PARAMS) );

    IoBuffer.Key = pDevice->Key;

    PlxIoMessage(
        pDevice,
        PLX_IOCTL_EEPROM_CRC_GET,
        &IoBuffer
        );

    if (pCrc != NULL)
    {
        *pCrc = (U32)IoBuffer.value[0];
    }

    if (pCrcStatus != NULL)
    {
        *pCrcStatus = (U8)IoBuffer.value[1];
    }

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
        {
            *pCrc = 0;
        }
        return PLX_STATUS_INVALID_OBJECT;
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
    {
        *pCrc = (U32)IoBuffer.value[0];
    }

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
    {
        return PLX_STATUS_INVALID_OBJECT;
    }

    if (pWidth == NULL)
    {
        return PLX_STATUS_NULL_PARAM;
    }

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
    {
        return PLX_STATUS_INVALID_OBJECT;
    }

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
    {
        return PLX_STATUS_NULL_PARAM;
    }

    // Verify device object
    if (!IsObjectValid(pDevice))
    {
        return PLX_STATUS_INVALID_OBJECT;
    }

    RtlZeroMemory( &IoBuffer, sizeof(PLX_PARAMS) );

    IoBuffer.Key      = pDevice->Key;
    IoBuffer.value[0] = offset;

    PlxIoMessage(
        pDevice,
        PLX_IOCTL_EEPROM_READ_BY_OFFSET,
        &IoBuffer
        );

    if (IoBuffer.ReturnCode == PLX_STATUS_OK)
    {
        *pValue = (U32)IoBuffer.value[1];
    }
    else
    {
        *pValue = 0;
    }

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
    {
        return PLX_STATUS_INVALID_OBJECT;
    }

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
    {
        return PLX_STATUS_NULL_PARAM;
    }

    // Verify device object
    if (!IsObjectValid(pDevice))
    {
        return PLX_STATUS_INVALID_OBJECT;
    }

    RtlZeroMemory( &IoBuffer, sizeof(PLX_PARAMS) );

    IoBuffer.Key      = pDevice->Key;
    IoBuffer.value[0] = offset;

    PlxIoMessage(
        pDevice,
        PLX_IOCTL_EEPROM_READ_BY_OFFSET_16,
        &IoBuffer
        );

    if (IoBuffer.ReturnCode == PLX_STATUS_OK)
    {
        *pValue = (U16)IoBuffer.value[1];
    }
    else
    {
        *pValue = 0;
    }

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
    {
        return PLX_STATUS_INVALID_OBJECT;
    }

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
 * Function   : PlxPci_SpiFlashPropGet
 *
 * Description: Returns the SPI flash properties at specified chip select
 *
 *****************************************************************************/
PLX_STATUS
PlxPci_SpiFlashPropGet(
    PLX_DEVICE_OBJECT *PtrDev,
    U8                 ChipSel,
    PEX_SPI_OBJ       *PtrSpi
    )
{
    // Verify device object
    if (!IsObjectValid(PtrDev))
    {
        return PLX_STATUS_INVALID_OBJECT;
    }

    if (PtrSpi == NULL)
    {
        return PLX_STATUS_NULL_PARAM;
    }

    return PlxDir_SpiFlashPropGet( PtrDev, ChipSel, PtrSpi );
}




/******************************************************************************
 *
 * Function   : PlxPci_SpiFlashErase
 *
 * Description: Erase all or a portion of flash
 *
 *****************************************************************************/
PLX_STATUS
PlxPci_SpiFlashErase(
    PLX_DEVICE_OBJECT *PtrDev,
    PEX_SPI_OBJ       *PtrSpi,
    U32                StartOffset,
    U8                 BoolWaitComplete
    )
{
    return PlxDir_SpiFlashErase( PtrDev, PtrSpi, StartOffset, BoolWaitComplete );
}




/******************************************************************************
 *
 * Function   : PlxPci_SpiFlashReadBuffer
 *
 * Description: Reads data from flash
 *
 *****************************************************************************/
PLX_STATUS
PlxPci_SpiFlashReadBuffer(
    PLX_DEVICE_OBJECT *PtrDev,
    PEX_SPI_OBJ       *PtrSpi,
    U32                StartOffset,
    U8                *PtrRxBuff,
    U32                SizeRx
    )
{
    // Verify objects
    if ( !IsObjectValid(PtrDev) || !IsObjectValid(PtrSpi) )
    {
        return PLX_STATUS_INVALID_OBJECT;
    }

    return PlxDir_SpiFlashReadBuffer( PtrDev, PtrSpi, StartOffset, PtrRxBuff, SizeRx );
}




/******************************************************************************
 *
 * Function   : PlxPci_SpiFlashReadByOffset
 *
 * Description: Reads a 32b value from flash at the specified offset
 *
 *****************************************************************************/
U32
PlxPci_SpiFlashReadByOffset(
    PLX_DEVICE_OBJECT *PtrDev,
    PEX_SPI_OBJ       *PtrSpi,
    U32                Offset,
    PLX_STATUS        *PtrStatus
    )
{
    U32        regVal;
    PLX_STATUS status;


    status =
        PlxPci_SpiFlashReadBuffer(
            PtrDev,
            PtrSpi,
            Offset,
            (U8*)&regVal,
            sizeof(U32)
            );

    if (PtrStatus)
    {
        *PtrStatus = status;
    }

    return regVal;
}




/******************************************************************************
 *
 * Function   : PlxPci_SpiFlashWriteBuffer
 *
 * Description: Write data to flash
 *
 *****************************************************************************/
PLX_STATUS
PlxPci_SpiFlashWriteBuffer(
    PLX_DEVICE_OBJECT *PtrDev,
    PEX_SPI_OBJ       *PtrSpi,
    U32                StartOffset,
    U8                *PtrTxBuff,
    U32                SizeTx
    )
{
    // Verify objects
    if ( !IsObjectValid(PtrDev) || !IsObjectValid(PtrSpi) )
    {
        return PLX_STATUS_INVALID_OBJECT;
    }

    return PlxDir_SpiFlashWriteBuffer( PtrDev, PtrSpi, StartOffset, PtrTxBuff, SizeTx );
}




/******************************************************************************
 *
 * Function   : PlxPci_SpiFlashWriteByOffset
 *
 * Description: Writes a 32b value to flash at the specified offset
 *
 *****************************************************************************/
PLX_STATUS
PlxPci_SpiFlashWriteByOffset(
    PLX_DEVICE_OBJECT *PtrDev,
    PEX_SPI_OBJ       *PtrSpi,
    U32                Offset,
    U32                Data
    )
{
    return PlxPci_SpiFlashWriteBuffer(
               PtrDev,
               PtrSpi,
               Offset,
               (U8*)&Data,
               sizeof(U32)
               );
}




/******************************************************************************
 *
 * Function   : PlxPci_SpiFlashGetStatus
 *
 * Description: Returns whether a flash write/erase operation is still in-progress
 *
 *****************************************************************************/
PLX_STATUS
PlxPci_SpiFlashGetStatus(
    PLX_DEVICE_OBJECT *PtrDev,
    PEX_SPI_OBJ       *PtrSpi
    )
{
    return PlxDir_SpiFlashGetStatus( PtrDev, PtrSpi );
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
    {
        return PLX_STATUS_NULL_PARAM;
    }

    // Verify device object
    if (!IsObjectValid(pDevice))
    {
        return PLX_STATUS_INVALID_OBJECT;
    }

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
    {
        return PLX_STATUS_NULL_PARAM;
    }

    // Verify device object
    if (!IsObjectValid(pDevice))
    {
        return PLX_STATUS_INVALID_OBJECT;
    }

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
    {
        return PLX_STATUS_NULL_PARAM;
    }

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
            return PLX_STATUS_INVALID_DATA;
    }

    // Verify device object
    if (!IsObjectValid(pDevice))
    {
        return PLX_STATUS_INVALID_OBJECT;
    }

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
    {
        return PLX_STATUS_NULL_PARAM;
    }

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
            return PLX_STATUS_INVALID_DATA;
    }

    // Verify device object
    if (!IsObjectValid(pDevice))
    {
        return PLX_STATUS_INVALID_OBJECT;
    }

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
    {
        return PLX_STATUS_NULL_PARAM;
    }

    // Verify device object
    if (!IsObjectValid(pDevice))
    {
        return PLX_STATUS_INVALID_OBJECT;
    }

    // Verify size
    if (pMemoryInfo->Size == 0)
    {
        return PLX_STATUS_INVALID_SIZE;
    }

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
    {
        return PLX_STATUS_NULL_PARAM;
    }

    // Verify device object
    if (!IsObjectValid(pDevice))
    {
        return PLX_STATUS_INVALID_OBJECT;
    }

    // Unmap the buffer if it was previously mapped to user space
    if (pMemoryInfo->UserAddr != 0)
    {
        PlxPci_PhysicalMemoryUnmap( pDevice, pMemoryInfo );
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
    {
        return PLX_STATUS_NULL_PARAM;
    }

    // Set default return value
    pMemoryInfo->UserAddr = 0;

    // Verify device object
    if (!IsObjectValid(pDevice))
    {
        return PLX_STATUS_INVALID_OBJECT;
    }

    // Verify buffer object
    if ((pMemoryInfo->CpuPhysical == 0) ||
        (pMemoryInfo->Size        == 0))
    {
        return PLX_STATUS_INVALID_DATA;
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
        return PLX_STATUS_INSUFFICIENT_RES;
    }

    return PLX_STATUS_OK;
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
    {
        return PLX_STATUS_NULL_PARAM;
    }

    // Verify device object
    if (!IsObjectValid(pDevice))
    {
        return PLX_STATUS_INVALID_OBJECT;
    }

    // Verify buffer object
    if ((pMemoryInfo->CpuPhysical == 0) ||
        (pMemoryInfo->Size        == 0))
    {
        return PLX_STATUS_INVALID_DATA;
    }

    // Verify virtual address
    if (pMemoryInfo->UserAddr == 0)
    {
        return PLX_STATUS_INVALID_ADDR;
    }

    // Unmap buffer from virtual space
    rc =
        munmap(
            PLX_INT_TO_PTR(pMemoryInfo->UserAddr),
            pMemoryInfo->Size
            );

    if (rc != 0)
    {
        return PLX_STATUS_INVALID_ADDR;
    }

    // Clear buffer address
    pMemoryInfo->UserAddr = 0;

    return PLX_STATUS_OK;
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
    {
        return PLX_STATUS_NULL_PARAM;
    }

    // Verify device object
    if (!IsObjectValid(pDevice))
    {
        return PLX_STATUS_INVALID_OBJECT;
    }

    // If we already have the information, just copy it
    if (pDevice->CommonBuffer.PhysicalAddr != 0)
    {
        *pMemoryInfo = pDevice->CommonBuffer;
        return PLX_STATUS_OK;
    }

    // Clear properties in case not supported in driver
    RtlZeroMemory( pMemoryInfo, sizeof(PLX_PHYSICAL_MEM) );

    RtlZeroMemory( &IoBuffer, sizeof(PLX_PARAMS) );

    PlxIoMessage(
        pDevice,
        PLX_IOCTL_COMMON_BUFFER_PROPERTIES,
        &IoBuffer
        );

    if (IoBuffer.ReturnCode != PLX_STATUS_OK)
    {
        return IoBuffer.ReturnCode;
    }

    // Copy buffer properties
    *pMemoryInfo = IoBuffer.u.PciMemory;

    // Make sure virtual address is clear
    pMemoryInfo->UserAddr = 0;

    // Save the data for any future calls
    pDevice->CommonBuffer = *pMemoryInfo;

    return PLX_STATUS_OK;
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
    PLX_STATUS       status;
    PLX_PHYSICAL_MEM MemInfo;


    if (pVa == NULL)
    {
        return PLX_STATUS_NULL_PARAM;
    }

    // Set default return value
    *(PLX_UINT_PTR*)pVa = 0;

    // Verify device object
    if (!IsObjectValid(pDevice))
    {
        return PLX_STATUS_INVALID_OBJECT;
    }

    // If buffer was previously mapped, just copy it
    if (pDevice->CommonBuffer.UserAddr != 0)
    {
        *(PLX_UINT_PTR*)pVa = (PLX_UINT_PTR)pDevice->CommonBuffer.UserAddr;
        return PLX_STATUS_OK;
    }

    // Check for valid common buffer info
    if (pDevice->CommonBuffer.PhysicalAddr == 0)
    {
        // Get buffer properties
        status =
            PlxPci_CommonBufferProperties(
                pDevice,
                &MemInfo
                );

        if (status != PLX_STATUS_OK)
        {
            return status;
        }
    }
    else
    {
        // Copy buffer properties
        MemInfo = pDevice->CommonBuffer;
    }

    status = PlxPci_PhysicalMemoryMap( pDevice, &MemInfo );
    if (status != PLX_STATUS_OK)
    {
        *(PLX_UINT_PTR*)pVa = 0;
        return status;
    }

    // Pass virtual address
    *(PLX_UINT_PTR*)pVa = (PLX_UINT_PTR)MemInfo.UserAddr;

    // Save the data for any future calls
    pDevice->CommonBuffer.UserAddr = MemInfo.UserAddr;

    return PLX_STATUS_OK;
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
    PLX_STATUS       status;
    PLX_PHYSICAL_MEM MemInfo;


    if (pVa == NULL)
    {
        return PLX_STATUS_NULL_PARAM;
    }

    if (*(PLX_UINT_PTR*)pVa == 0)
    {
        return PLX_STATUS_INVALID_ADDR;
    }

    // Verify device object
    if (!IsObjectValid(pDevice))
    {
        return PLX_STATUS_INVALID_OBJECT;
    }

    // Verify virtual address
    if (pDevice->CommonBuffer.UserAddr != *(PLX_UINT_PTR*)pVa)
    {
        return PLX_STATUS_INVALID_ADDR;
    }

    // Copy buffer properties
    MemInfo = pDevice->CommonBuffer;

    // Unmap the buffer
    status = PlxPci_PhysicalMemoryUnmap( pDevice, &MemInfo );
    if (status == PLX_STATUS_OK)
    {
        // Clear internal data
        pDevice->CommonBuffer.UserAddr = 0;

        // Clear buffer address
        *(PLX_UINT_PTR*)pVa = 0;
    }

    return status;
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
    {
        return PLX_STATUS_NULL_PARAM;
    }

    // Verify device object
    if (!IsObjectValid(pDevice))
    {
        return PLX_STATUS_INVALID_OBJECT;
    }

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
    {
        return PLX_STATUS_NULL_PARAM;
    }

    // Verify device object
    if (!IsObjectValid(pDevice))
    {
        return PLX_STATUS_INVALID_OBJECT;
    }

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
    return PLX_STATUS_UNSUPPORTED;

#elif defined(PLX_LINUX)

    PLX_PARAMS IoBuffer;


    // Check for null pointers
    if ((pPlxIntr == NULL) || (pEvent == NULL))
    {
        return PLX_STATUS_NULL_PARAM;
    }

    // Verify device object
    if (!IsObjectValid(pDevice))
    {
        return PLX_STATUS_INVALID_OBJECT;
    }

    RtlZeroMemory( &IoBuffer, sizeof(PLX_PARAMS) );

    IoBuffer.u.PlxIntr = *pPlxIntr;

    PlxIoMessage(
        pDevice,
        PLX_IOCTL_NOTIFICATION_REGISTER_FOR,
        &IoBuffer
        );

    if (IoBuffer.ReturnCode == PLX_STATUS_OK)
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
    return PLX_STATUS_UNSUPPORTED;

#elif defined(PLX_LINUX)

    PLX_PARAMS IoBuffer;


    // Verify notify object
    if (pEvent == NULL)
    {
        return PLX_STATUS_NULL_PARAM;
    }

    // Verify device object
    if (!IsObjectValid(pDevice))
    {
        return PLX_STATUS_INVALID_OBJECT;
    }

    // Verify event object
    if (!IsObjectValid(pEvent))
    {
        return PLX_STATUS_FAILED;
    }

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
    {
        return PLX_STATUS_NULL_PARAM;
    }

    // Verify device object
    if (!IsObjectValid(pDevice))
    {
        return PLX_STATUS_INVALID_OBJECT;
    }

    // Verify event object
    if (!IsObjectValid(pEvent))
    {
        return PLX_STATUS_FAILED;
    }

    RtlZeroMemory( &IoBuffer, sizeof(PLX_PARAMS) );

    IoBuffer.value[0] = pEvent->pWaitObject;

    PlxIoMessage(
        pDevice,
        PLX_IOCTL_NOTIFICATION_STATUS,
        &IoBuffer
        );

    if (IoBuffer.ReturnCode == PLX_STATUS_OK)
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
    {
        return PLX_STATUS_NULL_PARAM;
    }

    // Verify device object
    if (!IsObjectValid(pDevice))
    {
        return PLX_STATUS_INVALID_OBJECT;
    }

    // Verify event object
    if (!IsObjectValid(pEvent))
    {
        return PLX_STATUS_FAILED;
    }

    RtlZeroMemory( &IoBuffer, sizeof(PLX_PARAMS) );

    IoBuffer.value[0] = pEvent->pWaitObject;

    PlxIoMessage(
        pDevice,
        PLX_IOCTL_NOTIFICATION_CANCEL,
        &IoBuffer
        );

    if (IoBuffer.ReturnCode == PLX_STATUS_OK)
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
        {
            *pStatus = PLX_STATUS_INVALID_OBJECT;
        }
        return PCI_CFG_RD_ERR_VAL_32;
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
    {
        *pStatus = IoBuffer.ReturnCode;
    }

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
    {
        return PLX_STATUS_INVALID_OBJECT;
    }

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
    {
        return PLX_STATUS_INVALID_OBJECT;
    }

    RtlZeroMemory( &IoBuffer, sizeof(PLX_PARAMS) );

    IoBuffer.value[0] = channel;

    PlxIoMessage(
        pDevice,
        PLX_IOCTL_DMA_CHANNEL_OPEN,
        &IoBuffer
        );

    // If channel opened, update properties if provided
    if (IoBuffer.ReturnCode == PLX_STATUS_OK)
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
    {
        return PLX_STATUS_NULL_PARAM;
    }

    // Verify device object
    if (!IsObjectValid(pDevice))
    {
        return PLX_STATUS_INVALID_OBJECT;
    }

    RtlZeroMemory( &IoBuffer, sizeof(PLX_PARAMS) );

    IoBuffer.value[0] = channel;

    PlxIoMessage(
        pDevice,
        PLX_IOCTL_DMA_GET_PROPERTIES,
        &IoBuffer
        );

    // Return properties on success
    if (IoBuffer.ReturnCode == PLX_STATUS_OK)
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
    {
        return PLX_STATUS_NULL_PARAM;
    }

    // Verify device object
    if (!IsObjectValid(pDevice))
    {
        return PLX_STATUS_INVALID_OBJECT;
    }

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
    {
        return PLX_STATUS_INVALID_OBJECT;
    }

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
    {
        return PLX_STATUS_INVALID_OBJECT;
    }

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
    PLX_STATUS        status;
    PLX_INTERRUPT     PlxIntr;
    PLX_NOTIFY_OBJECT Event;


    if (pDmaParams == NULL)
    {
        return PLX_STATUS_NULL_PARAM;
    }

    // Verify device object
    if (!IsObjectValid(pDevice))
    {
        return PLX_STATUS_INVALID_OBJECT;
    }

    // Added to avoid compiler warning
    bIgnoreInt = FALSE;

    // Setup to wait for interrupt if requested
    if (Timeout_ms != 0)
    {
        // Clear interrupt fields
        RtlZeroMemory( &PlxIntr, sizeof(PLX_INTERRUPT) );

        // Setup for DMA done interrupt
        if (((S8)channel >= 0) && ((S8)channel < 4))
        {
            PlxIntr.DmaDone = (1 << channel);
        }
        else
        {
            return PLX_STATUS_INVALID_ADDR;
        }

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

    status = IoBuffer.ReturnCode;

    // Don't wait for completion if requested not to
    if (Timeout_ms == 0)
    {
        return status;
    }

    // Restore original value of interrupt ignore
    pDmaParams->bIgnoreBlockInt = bIgnoreInt;

    // Wait for completion if requested
    if (status == PLX_STATUS_OK)
    {
        status =
            PlxPci_NotificationWait(
                pDevice,
                &Event,
                Timeout_ms
                );

        switch (status)
        {
            case PLX_STATUS_OK:
                // DMA transfer completed
                break;

            case PLX_STATUS_TIMEOUT:
                status = PLX_STATUS_TIMEOUT;
                break;

            case PLX_STATUS_CANCELED:
                status = PLX_STATUS_FAILED;
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

    return status;
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
    PLX_STATUS        status;
    PLX_INTERRUPT     PlxIntr;
    PLX_NOTIFY_OBJECT Event;


    if (pDmaParams == NULL)
    {
        return PLX_STATUS_NULL_PARAM;
    }

    // Verify device object
    if (!IsObjectValid(pDevice))
    {
        return PLX_STATUS_INVALID_OBJECT;
    }

    // Setup to wait for interrupt if requested
    if (Timeout_ms != 0)
    {
        // Clear interrupt fields
        RtlZeroMemory( &PlxIntr, sizeof(PLX_INTERRUPT) );

        // Setup for DMA done interrupt
        if (((S8)channel >= 0) && ((S8)channel < 4))
        {
            PlxIntr.DmaDone = (1 << channel);
        }
        else
        {
            return PLX_STATUS_INVALID_ADDR;
        }

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

    status = IoBuffer.ReturnCode;

    // Don't wait for completion if requested not to
    if (Timeout_ms == 0)
    {
        return status;
    }

    // Wait for completion if requested
    if (status == PLX_STATUS_OK)
    {
        status =
            PlxPci_NotificationWait(
                pDevice,
                &Event,
                Timeout_ms
                );

        switch (status)
        {
            case PLX_STATUS_OK:
                // DMA transfer completed
                break;

            case PLX_STATUS_TIMEOUT:
                status = PLX_STATUS_TIMEOUT;
                break;

            case PLX_STATUS_CANCELED:
                status = PLX_STATUS_FAILED;
                break;

            default:
                // Added to avoid compiler warning
                break;
        }
    }

    // Cancel event notification
    PlxPci_NotificationCancel( pDevice, &Event );

    return status;
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
    {
        return PLX_STATUS_INVALID_OBJECT;
    }

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
    {
        return PLX_STATUS_NULL_PARAM;
    }

    // Verify device object
    if (!IsObjectValid(pDevice))
    {
        return PLX_STATUS_INVALID_OBJECT;
    }

    // Make sure Performance object is not already in use
    if (IsObjectValid(pPerfProp))
    {
        return PLX_STATUS_INVALID_OBJECT;
    }

    RtlZeroMemory( &IoBuffer, sizeof(PLX_PARAMS) );

    IoBuffer.Key      = pDevice->Key;
    IoBuffer.value[0] = (PLX_UINT_PTR)pPerfProp;

    PlxIoMessage(
        pDevice,
        PLX_IOCTL_PERFORMANCE_INIT_PROPERTIES,
        &IoBuffer
        );

    if (IoBuffer.ReturnCode != PLX_STATUS_OK)
    {
        return IoBuffer.ReturnCode;
    }

    // Mark object as valid
    ObjectValidate( pPerfProp );

    return PLX_STATUS_OK;
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
    {
        return PLX_STATUS_NULL_PARAM;
    }

    // Verify device object
    if (!IsObjectValid(pDevice))
    {
        return PLX_STATUS_INVALID_OBJECT;
    }

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
    {
        return PLX_STATUS_NULL_PARAM;
    }

    // Verify device object
    if (!IsObjectValid(pDevice))
    {
        return PLX_STATUS_INVALID_OBJECT;
    }

    // Make sure each Performance object is valid
    for (i = 0; i < NumOfObjects; i++)
    {
        if (!IsObjectValid(&pPerfProps[i]))
        {
            return PLX_STATUS_INVALID_OBJECT;
        }

        // Clear all current and previous counter values (14 counters)
        RtlZeroMemory(
            &pPerfProps[i].IngressPostedHeader,
            2 * (PERF_COUNTERS_PER_PORT * sizeof(U32))
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
    {
        return PLX_STATUS_NULL_PARAM;
    }

    // Verify device object
    if (!IsObjectValid(pDevice))
    {
        return PLX_STATUS_INVALID_OBJECT;
    }

    // Make sure each Performance object is valid
    for (i = 0; i < NumOfObjects; i++)
    {
        if (!IsObjectValid(&pPerfProps[i]))
        {
            return PLX_STATUS_INVALID_OBJECT;
        }
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


    // Verify elapsed time and link is up
    if ( (ElapsedTime_ms == 0) || (pPerfProp->LinkWidth == 0) )
    {
        RtlZeroMemory( pPerfStats, sizeof(PLX_PERF_STATS) );
        return PLX_STATUS_INVALID_DATA;
    }

    // Determine theoretical max link rate for 1 second (Gen1 bps * link_width * 2^(link_speed - 1) )
    MaxLinkRate = PERF_MAX_BPS_GEN_1_0 * pPerfProp->LinkWidth * (S64)pow( 2, (pPerfProp->LinkSpeed - 1) );

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
    {
        Counter_PostedHeader += ((S64)1 << 32);
    }

    if (Counter_PostedDW < pPerfProp->Prev_IngressPostedDW)
    {
        Counter_PostedDW += ((S64)1 << 32);
    }

    if (Counter_NonpostedDW < pPerfProp->Prev_IngressNonpostedDW)
    {
        Counter_NonpostedDW += ((S64)1 << 32);
    }

    if (Counter_CplHeader < pPerfProp->Prev_IngressCplHeader)
    {
        Counter_CplHeader += ((S64)1 << 32);
    }

    if (Counter_CplDW < pPerfProp->Prev_IngressCplDW)
    {
        Counter_CplDW += ((S64)1 << 32);
    }

    if (Counter_Dllp < pPerfProp->Prev_IngressDllp)
    {
        Counter_Dllp += ((S64)1 << 32);
    }

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
    {
        pPerfStats->IngressPayloadAvgPerTlp =
            (long double)pPerfStats->IngressPayloadTotalBytes / PayloadAvg;
    }
    else
    {
        pPerfStats->IngressPayloadAvgPerTlp = 0;
    }

    // Total number of TLP data ((P_DW + NP_DW + CPL_DW) * size(DW))
    TotalBytes = (Counter_PostedDW    +
                  Counter_NonpostedDW +
                  Counter_CplDW) * sizeof(U32);

    // Add DLLPs to total bytes
    TotalBytes += (Counter_Dllp * PERF_DLLP_SIZE);

    // Total bytes
    pPerfStats->IngressTotalBytes = TotalBytes;

    // Total byte rate
    pPerfStats->IngressTotalByteRate =
        (long double)((TotalBytes * 1000) / ElapsedTime_ms);

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
        pPerfStats->IngressLinkUtilization =
            ((long double)TotalBytes * 100) / MaxLinkRate;

        // Account for error margin
        if (pPerfStats->IngressLinkUtilization > (double)100)
        {
            pPerfStats->IngressLinkUtilization = 100;
        }
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
    {
        Counter_PostedHeader += ((S64)1 << 32);
    }

    if (Counter_PostedDW < pPerfProp->Prev_EgressPostedDW)
    {
        Counter_PostedDW += ((S64)1 << 32);
    }

    if (Counter_NonpostedDW < pPerfProp->Prev_EgressNonpostedDW)
    {
        Counter_NonpostedDW += ((S64)1 << 32);
    }

    if (Counter_CplHeader < pPerfProp->Prev_EgressCplHeader)
    {
        Counter_CplHeader += ((S64)1 << 32);
    }

    if (Counter_CplDW < pPerfProp->Prev_EgressCplDW)
    {
        Counter_CplDW += ((S64)1 << 32);
    }

    if (Counter_Dllp < pPerfProp->Prev_EgressDllp)
    {
        Counter_Dllp += ((S64)1 << 32);
    }

    // Determine counter differences
    Counter_PostedHeader = Counter_PostedHeader - pPerfProp->Prev_EgressPostedHeader;
    Counter_PostedDW     = Counter_PostedDW     - pPerfProp->Prev_EgressPostedDW;
    Counter_NonpostedDW  = Counter_NonpostedDW  - pPerfProp->Prev_EgressNonpostedDW;
    Counter_CplHeader    = Counter_CplHeader    - pPerfProp->Prev_EgressCplHeader;
    Counter_CplDW        = Counter_CplDW        - pPerfProp->Prev_EgressCplDW;
    Counter_Dllp         = Counter_Dllp         - pPerfProp->Prev_EgressDllp;

    /*************************************************************************
     * Capella-2 does not count the 2DW overhead for egress DW. The DW counts
     * are adjusted by adding 2DW per TLP to account for the overhead.
     ************************************************************************/
    if (pPerfProp->PlxFamily == PLX_FAMILY_CAPELLA_2)
    {
        Counter_PostedDW += (Counter_PostedHeader * PERF_TLP_OH_DW);
        Counter_CplDW    += (Counter_CplHeader * PERF_TLP_OH_DW);

        // No TLP count is provided for non-posted, unable to adjust
        Counter_NonpostedDW += 0;
    }

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
    {
        pPerfStats->EgressPayloadAvgPerTlp =
            (long double)pPerfStats->EgressPayloadTotalBytes / PayloadAvg;
    }
    else
    {
        pPerfStats->EgressPayloadAvgPerTlp = 0;
    }

    // Total number of TLP data ((P_DW + NP_DW + CPL_DW) * size(DW))
    TotalBytes = (Counter_PostedDW    +
                  Counter_NonpostedDW +
                  Counter_CplDW) * sizeof(U32);

    // Add DLLPs to total bytes
    TotalBytes += (Counter_Dllp * PERF_DLLP_SIZE);

    // Total bytes
    pPerfStats->EgressTotalBytes = TotalBytes;

    // Total byte rate
    pPerfStats->EgressTotalByteRate =
        ((long double)TotalBytes * 1000) / ElapsedTime_ms;

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
        pPerfStats->EgressLinkUtilization =
            ((long double)TotalBytes * 100) / MaxLinkRate;

        // Account for error margin
        if (pPerfStats->EgressLinkUtilization > (double)100)
        {
            pPerfStats->EgressLinkUtilization = 100;
        }
    }

    return PLX_STATUS_OK;
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
    {
        return PLX_STATUS_NULL_PARAM;
    }

    // Verify device object
    if (!IsObjectValid(pDevice))
    {
        return PLX_STATUS_INVALID_OBJECT;
    }

    RtlZeroMemory( &IoBuffer, sizeof(PLX_PARAMS) );

    IoBuffer.Key = pDevice->Key;

    PlxIoMessage(
        pDevice,
        PLX_IOCTL_MH_GET_PROPERTIES,
        &IoBuffer
        );

    if (IoBuffer.ReturnCode == PLX_STATUS_OK)
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
    {
        return PLX_STATUS_NULL_PARAM;
    }

    // Verify device object
    if (!IsObjectValid(pDevice))
    {
        return PLX_STATUS_INVALID_OBJECT;
    }

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
    {
        return PLX_STATUS_NULL_PARAM;
    }

    // Verify device object
    if (!IsObjectValid(pDevice))
    {
        return PLX_STATUS_INVALID_OBJECT;
    }

    RtlZeroMemory( &IoBuffer, sizeof(PLX_PARAMS) );

    IoBuffer.Key      = pDevice->Key;
    IoBuffer.value[0] = bRead;

    PlxIoMessage(
        pDevice,
        PLX_IOCTL_NT_PROBE_REQ_ID,
        &IoBuffer
        );

    // Return ReqID
    if (IoBuffer.ReturnCode == PLX_STATUS_OK)
    {
        *pReqId = (U16)IoBuffer.value[1];
    }

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
    {
        return PLX_STATUS_NULL_PARAM;
    }

    // Verify device object
    if (!IsObjectValid(pDevice))
    {
        return PLX_STATUS_INVALID_OBJECT;
    }

    RtlZeroMemory( &IoBuffer, sizeof(PLX_PARAMS) );

    IoBuffer.Key      = pDevice->Key;
    IoBuffer.value[0] = LutIndex;

    PlxIoMessage(
        pDevice,
        PLX_IOCTL_NT_LUT_PROPERTIES,
        &IoBuffer
        );

    // Return LUT entry properties requested
    if (IoBuffer.ReturnCode == PLX_STATUS_OK)
    {
        if (pReqId != NULL)
        {
            *pReqId = (U16)IoBuffer.value[0];
        }

        if (pFlags != NULL)
        {
            *pFlags = (U32)IoBuffer.value[1];
        }

        if (pbEnabled != NULL)
        {
            *pbEnabled = (BOOLEAN)IoBuffer.value[2];
        }
    }
    else
    {
        if (pReqId != NULL)
        {
            *pReqId = 0;
        }

        if (pFlags != NULL)
        {
            *pFlags = PLX_NT_LUT_FLAG_NONE;
        }

        if (pbEnabled != NULL)
        {
            *pbEnabled = FALSE;
        }
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
    {
        return PLX_STATUS_NULL_PARAM;
    }

    // Verify device object
    if (!IsObjectValid(pDevice))
    {
        return PLX_STATUS_INVALID_OBJECT;
    }

    RtlZeroMemory( &IoBuffer, sizeof(PLX_PARAMS) );

    IoBuffer.Key      = pDevice->Key;
    IoBuffer.value[1] = ReqId;
    IoBuffer.value[2] = flags;

    if (pLutIndex == NULL)
    {
        IoBuffer.value[0] = (U16)-1;    // Default to auto-select
    }
    else
    {
        IoBuffer.value[0] = *pLutIndex;
    }

    PlxIoMessage(
        pDevice,
        PLX_IOCTL_NT_LUT_ADD,
        &IoBuffer
        );

    // Return assigned LUT index if requested
    if ((IoBuffer.ReturnCode == PLX_STATUS_OK) && (pLutIndex != NULL))
    {
        *pLutIndex = (U16)IoBuffer.value[0];
    }

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
    {
        return PLX_STATUS_NULL_PARAM;
    }

    // Verify device object
    if (!IsObjectValid(pDevice))
    {
        return PLX_STATUS_INVALID_OBJECT;
    }

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
 *****************************************************************************/
PLX_STATUS
Driver_Connect(
    PLX_DRIVER_HANDLE *pHandle,
    U8                 ApiIndex,
    U16                DeviceNumber
    )
{
    char Extension[10];
    char DriverName[30];
#if defined(PLX_LINUX)
    PLX_PARAMS IoBuffer;        // Driver version verification
#endif


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

    strcat( DriverName, Extension );
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

        strcat( DriverName, Extension );
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
        return PLX_STATUS_NO_DRIVER;
    }

#elif defined(PLX_LINUX)

    // Open the device
    *pHandle = open( DriverName, O_RDWR );
    if (*pHandle < 0)
    {
        *pHandle = INVALID_HANDLE_VALUE;
        return PLX_STATUS_NO_DRIVER;
    }

    /******************************************
     * For Linux, query driver version & verify
     * match with API since driver link names
     * don't include version information.
     *****************************************/
    if (ioctl(
            *pHandle,                   // Driver handle
            PLX_IOCTL_DRIVER_VERSION,   // Control code
            &IoBuffer                   // Pointer to buffer
            ) < 0)
    {
        *pHandle = INVALID_HANDLE_VALUE;
        close( *pHandle );
        return PLX_STATUS_UNSUPPORTED;
    }

    // Verify matching version
    if (((U8)(IoBuffer.value[0] >> 16) != PLX_SDK_VERSION_MAJOR) ||
        ((U8)(IoBuffer.value[0] >>  8) != PLX_SDK_VERSION_MINOR))
    {
        close( *pHandle );
        *pHandle = INVALID_HANDLE_VALUE;
        return PLX_STATUS_VER_MISMATCH;
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
        return PLX_STATUS_FAILED;
    }

    // Get a handle to the driver
    if (PlxSvc_DriverEntry( pHandle ) == FALSE)
    {
        return PLX_STATUS_NO_DRIVER;
    }

    // Open the device
    if (Dispatch_Create( *pHandle ) == FALSE)
    {
        return PLX_STATUS_INVALID_OBJECT;
    }

#endif

    return PLX_STATUS_OK;
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
    else if (pDevice->Key.ApiMode == PLX_API_MODE_MDIO_SPLICE)
    {
        status =
            MdioSplice_Dispatch_IoControl(
                pDevice,                // Device
                IoControlCode,          // Control code
                pBuffer,                // Pointer to buffer
                sizeof(PLX_PARAMS)      // Size of buffer
                );
    }
    else if (pDevice->Key.ApiMode == PLX_API_MODE_SDB)
    {
        status =
            Sdb_Dispatch_IoControl(
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
    FILE    *pFile;
 #endif


    // Initialize the optional arguments pointer
    va_start(pArgs, format);

    // Build string to write
    vsprintf(pOut, format, pArgs);

    // Terminate arguments pointer
    va_end(pArgs);

 #if defined(PLX_DBG_DEST_FILE)
    // Open the file for appending output
    pFile = fopen( PLX_LOG_FILE, "a+" );
    if (pFile == NULL)
    {
        return;
    }

    // Write string to file
    fputs( pOut, pFile );

    // Close the file
    fclose( pFile );

 #else

    _PlxDbgOut( pOut );

 #endif
}
