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

/*******************************************************************************
 *
 * File Name:
 *
 *      PlxApiI2c.c
 *
 * Description:
 *
 *      Implements the PLX API functions over an I2C interface
 *
 * Revision History:
 *
 *      12-01-10 : PLX SDK v6.40
 *
 ******************************************************************************/

/*******************************************************************************
 *
 * I2C API utilization of fields in PLX_DEVICE_KEY
 *
 *  ApiIndex        - I2C USB device number (0,1,etc)
 *  DeviceNumber    - PLX chip I2C slave address
 *  PlxPort         - PLX port # or port type (NTL,NTV,DMA,etc)
 *  NTPortType      - NT port type (PLX_NT_PORT_TYPE)
 *  NTPortNum       - NT port number used if NT Virtual offset accessed
 *  ApiInternal[0]  - I2C bus speed in KHz
 *  ApiInternal[1]  - NT mode - Legacy or P2P
 *
 ******************************************************************************/

/*******************************************************************************
 *
 * PLX chip I2C port access table
 *
 * Vega-Lite
 *              Transp  NTV0 NTV1 NTL0 NTL1 P2P DMA0 DMA1 DMA2 DMA3 DMA_RAM
 *   Mode          0      0   --    1   --   --  --   --   --   --    --
 *   Stn_Sel      --     --   --   --   --   --  --   --   --   --    --
 *   Port_Sel     P#     P#   --    0   --   --  --   --   --   --    --
 *
 * Altair/Altair-XL
 *              Transp  NTV0 NTV1 NTL0 NTL1 P2P DMA0 DMA1 DMA2 DMA3 DMA_RAM
 *   Mode          0     --   --   --   --   --  --   --   --   --    --
 *   Stn_Sel      --     --   --   --   --   --  --   --   --   --    --
 *   Port_Sel     P#     --   --   --   --   --  --   --   --   --    --
 *
 * Deneb
 *              Transp  NTV0 NTV1 NTL0 NTL1 P2P DMA0 DMA1 DMA2 DMA3 DMA_RAM
 *   Mode          0      0   --    1   --   --  --   --   --   --    --
 *   Stn_Sel      --      0   --    0   --   --  --   --   --   --    --
 *   Port_Sel     P#     P#   --    0   --   --  --   --   --   --    --
 *
 * Sirius
 *              Transp  NTV0 NTV1 NTL0 NTL1 P2P DMA0 DMA1 DMA2 DMA3 DMA_RAM
 *   Mode          0      0   --    1   --    1   1   --   --   --     1
 *   Stn_Sel      --     --   --   --   --   --  --   --   --   --    --
 *   Port_Sel     P#     P#   --   10   --   11  12   --   --   --    13
 *
 * Cygnus
 *              Transp  NTV0 NTV1 NTL0 NTL1 P2P DMA0 DMA1 DMA2 DMA3 DMA_RAM
 *   Mode          0      0   --    1   --    0  --   --   --   --    --
 *   Stn_Sel      S#      6   --    0   --   --  --   --   --   --    --
 *   Port_Sel     P#      3   --    0   --   P#  --   --   --   --    --
 *
 * Draco/Scout
 *              Transp  NTV0 NTV1 NTL0 NTL1 P2P DMA0 DMA1 DMA2 DMA3 DMA_RAM
 *   Mode          0      2    2    1    1   --   3    3    3    3     3
 *   Stn_Sel      S#      0    0    0    0   --   0    0    0    0     0
 *   Port_Sel     P#      0    1    0    1   --   0    1    2    3     4
 *
 ******************************************************************************/

/*******************************************************************************
 *
 * PLX chip I2C port access command
 *
 * Vega-Lite
 *      31   27 26     24 23          19   18  17   15  14 13     10 9          0
 *      -------------------------------------------------------------------------
 *     | Resvd | I2C_Cmd |    Resvd     | NTL | PtSel | R | Byte_En |  DW_Offset |
 *      -------------------------------------------------------------------------
 *
 * Altair/Altair-XL
 *      31   27 26     24 23          19 18         15  14 13     10 9          0
 *      -------------------------------------------------------------------------
 *     | Resvd | I2C_Cmd |    Resvd     |   Port_Sel  | R | Byte_En |  DW_Offset |
 *      -------------------------------------------------------------------------
 *
 * Deneb
 *      31   27 26     24 23  20   19  18    17 16   15  14 13     10 9          0
 *      --------------------------------------------------------------------------
 *     | Resvd | I2C_Cmd | Rsvd | NTL | StnSel | PtSel | R | Byte_En |  DW_Offset |
 *      --------------------------------------------------------------------------
 *
 * Sirius
 *      31   27 26     24 23  20    19    18        15  14 13     10 9          0
 *      -------------------------------------------------------------------------
 *     | Resvd | I2C_Cmd | Rsvd | NT/DMA |  Port_Sel  | R | Byte_En |  DW_Offset |
 *      -------------------------------------------------------------------------
 *
 * Cygnus
 *      31   27 26     24 23  21   20   19    17 16   15  14 13     10 9          0
 *      ---------------------------------------------------------------------------
 *     | Resvd | I2C_Cmd | Rsvd | Mode | StnSel | PtSel | R | Byte_En |  DW_Offset |
 *      ---------------------------------------------------------------------------
 *
 * Draco/Scout
 *      31   27 26     24 23  22 21  20 19    18 17   15  14 13     10 9          0
 *      ---------------------------------------------------------------------------
 *     | Resvd | I2C_Cmd | Rsvd | Mode | StnSel | PtSel | R | Byte_En |  DW_Offset |
 *      ---------------------------------------------------------------------------
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


#include <math.h>
#include "Eep_8000.h"
#include "PlxIoctl.h"
#include "PlxApiDebug.h"
#include "PlxApiI2cAa.h"

#if !defined(PLX_DOS)
    #include "Aardvark.h"
#endif

#if defined(PLX_LINUX)
    #include <pthread.h>    // For mutex support
#endif




/**********************************************
 *           Portability Functions
 *********************************************/
#if defined(PLX_LINUX)

    #define CRITICAL_SECTION                        pthread_mutex_t
    #define InitializeCriticalSection(pCS)          pthread_mutex_init   ( (pCS), NULL )
    #define DeleteCriticalSection(pCS)              pthread_mutex_destroy( (pCS) )
    #define EnterCriticalSection(pCS)               pthread_mutex_lock   ( (pCS) )
    #define LeaveCriticalSection(pCS)               pthread_mutex_unlock ( (pCS) )

    #define InterlockedIncrement( pVal )            ++(*(pVal))
    #define InterlockedDecrement( pVal )            --(*(pVal))

#endif




/**********************************************
 *           Global Variables
 *********************************************/
#if !defined(PLX_DOS)

    BOOLEAN Gbl_bInitialized = FALSE;

    static struct _PLX_I2C_API_PROP
    {
        CRITICAL_SECTION  Lock_I2cRegAccess;
        PLX_DRIVER_HANDLE hDevice;
        S32               OpenCount;
        U8                UpstreamBus;
        U8                NTPortNum;
    } Gbl_PlxI2cProp[PLX_I2C_MAX_DEVICES];
#endif




/******************************************************************************
 *
 * Function   :  PlxI2c_I2cGetPorts
 *
 * Description:  Returns status of active I2C ports
 *
 *****************************************************************************/
PLX_STATUS
PlxI2c_I2cGetPorts(
    PLX_API_MODE  ApiMode,
    U32          *pI2cPorts
    )
{
    U8  i;
    U16 Ports[PLX_I2C_MAX_DEVICES];
    int PortCount;


    // Default to no active ports
    *pI2cPorts = 0;

    // Verify supported I2C mode
    if (ApiMode != PLX_API_MODE_I2C_AARDVARK)
        return ApiUnsupportedFunction;

    // Find all I2C ports
    PortCount =
        aa_find_devices(
            PLX_I2C_MAX_DEVICES,
            Ports
            );

    if (PortCount <= 0)
        return ApiNoActiveDriver;

    i = 0;

    // Verify desired port exists and is available
    while (i < PortCount)
    {
        // Check if port is in-use
        if (Ports[i] & AA_PORT_NOT_FREE)
        {
            // Clear port busy flag
            Ports[i] &= ~AA_PORT_NOT_FREE;

            // Flag port is in-use
            *pI2cPorts |= (1 << (Ports[i] + 16));
        }

        // Flag port is active
        *pI2cPorts |= (1 << Ports[i]);

        // Go to next port
        i++;
    }

    return ApiSuccess;
}




/******************************************************************************
 *
 * Function   :  PlxI2c_DeviceOpen
 *
 * Description:  Selects a device
 *
 *****************************************************************************/
PLX_STATUS
PlxI2c_DeviceOpen(
    PLX_DEVICE_OBJECT *pDevice
    )
{
    // Open connection to driver
    if (PlxI2c_Driver_Connect(
            pDevice,
            NULL
            ) == FALSE)
    {
        return ApiInvalidDeviceInfo;
    }

    // Fill in chip version information
    PlxChipTypeDetect( pDevice );

    // Mark object as valid
    ObjectValidate( pDevice );

    return ApiSuccess;
}




/******************************************************************************
 *
 * Function   :  PlxI2c_DeviceClose
 *
 * Description:  Closes a previously opened I2C device
 *
 *****************************************************************************/
PLX_STATUS
PlxI2c_DeviceClose(
    PLX_DEVICE_OBJECT *pDevice
    )
{
    if (Gbl_PlxI2cProp[pDevice->Key.ApiIndex].OpenCount <= 0)
        return ApiInvalidHandle;

    // Decrent open count and close device if no longer referenced
    if (InterlockedDecrement(
            &Gbl_PlxI2cProp[pDevice->Key.ApiIndex].OpenCount
            ) == 0)
    {
        // Close the device (aa_close() returns num handles closed)
        if (aa_close( (Aardvark)PLX_PTR_TO_INT( pDevice->hDevice ) ) != 1)
        {
            return ApiInvalidHandle;
        }

        // Mark device is closed
        Gbl_PlxI2cProp[pDevice->Key.ApiIndex].hDevice = 0;

        // Delete the register access lock
        DeleteCriticalSection(
            &(Gbl_PlxI2cProp[pDevice->Key.ApiIndex].Lock_I2cRegAccess)
            );
    }

    return ApiSuccess;
}




/******************************************************************************
 *
 * Function   :  PlxI2c_DeviceFindEx
 *
 * Description:  Locates a specific device
 *
 *****************************************************************************/
PLX_STATUS
PlxI2c_DeviceFindEx(
    PLX_DEVICE_KEY *pKey,
    U8              DeviceNumber,
    PLX_MODE_PROP  *pModeProp
    )
{
    U8                i;
    int               PortCount;
    U8                NumMatched;
    U8                TotalMatches;
    U16               Ports[PLX_I2C_MAX_DEVICES];
    U32               RegValue;
    BOOLEAN           bFound;
    BOOLEAN           bAutoProbe;
    PLX_STATUS        status;
    PLX_DEVICE_OBJECT Device;


    // Find all I2C ports
    PortCount =
        aa_find_devices(
            PLX_I2C_MAX_DEVICES,
            Ports
            );

    if (PortCount <= 0)
        return ApiNoActiveDriver;

    i = 0;

    // Verify desired port exists and is available
    while (i < PortCount)
    {
        if ((Ports[i] & ~AA_PORT_NOT_FREE) == pModeProp->I2c.I2cPort)
        {
            // Port exists
            break;
        }

        // Go to next port
        i++;
    }

    // Verify port was found
    if (i == PortCount)
        return ApiInvalidIndex;

    // Verify port is not in use
    if (Ports[i] & AA_PORT_NOT_FREE)
        return ApiDeviceInUse;

    // Clear the device object
    RtlZeroMemory( &Device, sizeof(PLX_DEVICE_OBJECT) );

    // Use default clock rate if not specified
    if (pModeProp->I2c.ClockRate == 0)
        pModeProp->I2c.ClockRate = PLX_I2C_DEFAULT_CLOCK_RATE;

    // Open connection to driver
    if (PlxI2c_Driver_Connect(
            &Device,
            pModeProp
            ) == FALSE)
    {
        return ApiInvalidDeviceInfo;
    }

    TotalMatches = 0;

    // Flag whether to auto-probe slave addresses
    if (pModeProp->I2c.SlaveAddr == (U16)-1)
    {
        bAutoProbe = TRUE;

        // Start probe at first I2C address
        Device.Key.DeviceNumber = 0x38;
    }
    else
    {
        bAutoProbe = FALSE;

        // Use requested slave address
        Device.Key.DeviceNumber = (U8)pModeProp->I2c.SlaveAddr;
    }

    // Default to device not found
    bFound = FALSE;

    // Probe devices to check if valid PLX device
    do
    {
        // Attempt to read Device/Vendor ID
        RegValue =
            PlxI2c_PlxRegisterRead(
                &Device,
                0,                  // Port 0 Device/Vendor ID
                &status,
                FALSE               // Don't adjust for port
                );

        if (status == ApiSuccess)
        {
            if ((RegValue & 0xFFFF) == PLX_VENDOR_ID)
            {
                DebugPrintf((
                    "I2C - Detected device %08X at address %02Xh\n",
                    RegValue, Device.Key.DeviceNumber
                    ));

                // PLX device found, determine active ports and compare
                status =
                    PlxI2c_ProbeSwitch(
                        &Device,
                        pKey,
                        (U8)(DeviceNumber - TotalMatches),
                        &NumMatched
                        );

                if (status == ApiSuccess)
                {
                    bFound = TRUE;
                    break;
                }

                // Add number of matched devices
                TotalMatches += NumMatched;
            }
        }

        // Jump to next slave address
        if (bAutoProbe)
        {
            /**************************************************
             * Possible PLX I2C addresses:
             *
             *    38->3F
             *    58->5F
             *    68->6F
             *    70->77
             *    18->1F
             *************************************************/

            switch (Device.Key.DeviceNumber)
            {
                case 0x3F:
                    Device.Key.DeviceNumber = 0x58;
                    break;

                case 0x5F:
                    Device.Key.DeviceNumber = 0x68;
                    break;

                case 0x6F:
                    Device.Key.DeviceNumber = 0x70;
                    break;

                case 0x78:
                    Device.Key.DeviceNumber = 0x18;
                    break;

                case 0x1F:
                    bAutoProbe = FALSE;
                    break;

                default:
                    Device.Key.DeviceNumber++;
                    break;
            }
        }
    }
    while (bAutoProbe && (bFound == FALSE));

    // Close the device
    PlxI2c_DeviceClose( &Device );

    if (bFound)
    {
        // Store API mode
        pKey->ApiMode = PLX_API_MODE_I2C_AARDVARK;

        // Validate key
        ObjectValidate( pKey );
        return ApiSuccess;
    }

    return ApiInvalidDeviceInfo;
}




/******************************************************************************
 *
 * Function   :  PlxPci_I2cVersion
 *
 * Description:  Returns I2C version information
 *
 *****************************************************************************/
PLX_STATUS
PlxI2c_I2cVersion(
    U16          I2cPort,
    PLX_VERSION *pVersion
    )
{
    Aardvark    hI2cDev;
    AardvarkExt I2cProp;


    // Clear structure
    RtlZeroMemory( pVersion, sizeof(PLX_VERSION) );

    // Check if device is already opened by the PLX API
    if (Gbl_PlxI2cProp[I2cPort].hDevice == 0)
    {
        // Select the device
        hI2cDev = aa_open( I2cPort );

        if (hI2cDev <= 0)
            return ApiFailed;
    }
    else
    {
        // Re-use existing handle
        hI2cDev = (Aardvark)PLX_PTR_TO_INT( Gbl_PlxI2cProp[I2cPort].hDevice );
    }

    // Get I2C version information
    aa_version( hI2cDev, &I2cProp.version );

    // Get I2C supported features
    I2cProp.features = aa_features( hI2cDev );

    // Release device only if not already open
    if (Gbl_PlxI2cProp[I2cPort].hDevice == 0)
        aa_close( hI2cDev );

    // Return version information
    pVersion->ApiMode        = PLX_API_MODE_I2C_AARDVARK;
    pVersion->I2c.ApiLibrary = AA_HEADER_VERSION;
    pVersion->I2c.Software   = I2cProp.version.software;
    pVersion->I2c.Firmware   = I2cProp.version.firmware;
    pVersion->I2c.Hardware   = I2cProp.version.hardware;
    pVersion->I2c.SwReqByFw  = I2cProp.version.sw_req_by_fw;
    pVersion->I2c.FwReqBySw  = I2cProp.version.fw_req_by_sw;
    pVersion->I2c.ApiReqBySw = I2cProp.version.api_req_by_sw;
    pVersion->I2c.Features   = I2cProp.features;

    DebugPrintf((
        "I2C version - API:%d.%02d  SW:%d.%02d  FW:%d.%02d  HW:%d.%02d\n",
        (AA_HEADER_VERSION        >> 8), AA_HEADER_VERSION        & 0xFF,
        (I2cProp.version.software >> 8), I2cProp.version.software & 0xFF,
        (I2cProp.version.firmware >> 8), I2cProp.version.firmware & 0xFF,
        (I2cProp.version.hardware >> 8), I2cProp.version.hardware & 0xFF
        ));

    return ApiSuccess;
}




/*******************************************************************************
 *
 * Function   :  PlxI2c_ChipTypeGet
 *
 * Description:  Returns PLX chip type and revision
 *
 ******************************************************************************/
PLX_STATUS
PlxI2c_ChipTypeGet(
    PLX_DEVICE_OBJECT *pDevice,
    U16               *pChipType,
    U8                *pRevision
    )
{
    *pChipType = pDevice->Key.PlxChip;
    *pRevision = pDevice->Key.PlxRevision;

    DebugPrintf((
        "Device %04X_%04X = %04X rev %02X\n",
        pDevice->Key.DeviceId, pDevice->Key.VendorId,
        *pChipType, *pRevision
        ));

    return ApiSuccess;
}




/*******************************************************************************
 *
 * Function   :  PlxI2c_ChipTypeSet
 *
 * Description:  Sets the PLX chip type dynamically
 *
 ******************************************************************************/
PLX_STATUS
PlxI2c_ChipTypeSet(
    PLX_DEVICE_OBJECT *pDevice,
    U16                ChipType,
    U8                 Revision
    )
{
    BOOLEAN       bSetUptreamNode;
    PLX_PORT_PROP PortProp;


    // Default to non-upstream node
    bSetUptreamNode = FALSE;

    // Attempt auto-detection if requested
    if (ChipType == 0)
    {
        PlxChipTypeDetect(
            pDevice
            );

        ChipType = pDevice->Key.PlxChip;
    }

    // Verify chip type
    switch (ChipType & 0xFF00)
    {
        case 0:         // Used to clear chip type
        case 0x8400:
        case 0x8500:
        case 0x8600:
        case 0x8700:
            // Get port properties to ensure upstream node
            PlxI2c_GetPortProperties(
                pDevice,
                &PortProp
                );

            if (PortProp.PortType != PLX_PORT_UPSTREAM)
            {
                DebugPrintf(("ERROR - Chip type may only be changed on upstream port\n"));
                return ApiUnsupportedFunction;
            }

            // Flag to update upstream node
            bSetUptreamNode = TRUE;
            break;

        default:
            DebugPrintf((
                "ERROR - Invalid or unsupported chip type (%04X)\n",
                ChipType
                ));
            return ApiInvalidData;
    }

    // Set the new chip type
    pDevice->Key.PlxChip = ChipType;

    // Check if we should update the revision or use the default
    if ((Revision == (U8)-1) || (Revision == 0))
    {
        // Attempt to detect Revision ID
        PlxChipRevisionDetect(
            pDevice
            );
    }
    else
    {
        pDevice->Key.PlxRevision = Revision;
    }

    DebugPrintf((
        "Set device (%04X_%04X) type to %04X rev %02X\n",
        pDevice->Key.DeviceId, pDevice->Key.VendorId,
        pDevice->Key.PlxChip, pDevice->Key.PlxRevision
        ));

    return ApiSuccess;
}




/*******************************************************************************
 *
 * Function   :  PlxI2c_GetPortProperties
 *
 * Description:  Returns the current port information and status
 *
 ******************************************************************************/
PLX_STATUS
PlxI2c_GetPortProperties(
    PLX_DEVICE_OBJECT *pDevice,
    PLX_PORT_PROP     *pPortProp
    )
{
    U16 MaxSize;
    U16 Offset_PcieCap;
    U32 RegValue;


    // Set default properties
    RtlZeroMemory(pPortProp, sizeof(PLX_PORT_PROP));

    // Get the offset of the PCI Express capability 
    Offset_PcieCap =
        PlxGetExtendedCapabilityOffset(
            pDevice,
            0x10       // CAP_ID_PCI_EXPRESS
            );

    if (Offset_PcieCap == 0)
    {
        DebugPrintf(("Device does not support PCI Express Capability\n"));

        // Mark device as non-PCIe
        pPortProp->bNonPcieDevice = TRUE;
        pPortProp->PortType       = PLX_PORT_UNKNOWN;

        return ApiSuccess;
    }

    // Get PCIe Capability
    PLX_PCI_REG_READ(
        pDevice,
        Offset_PcieCap,
        &RegValue
        );

    // Get port type
    pPortProp->PortType = (U8)((RegValue >> 20) & 0xF);

    // Get PCIe Device Capabilities
    PLX_PCI_REG_READ(
        pDevice,
        Offset_PcieCap + 0x04,
        &RegValue
        );

    // Get max payload size supported field
    MaxSize = (U8)(RegValue >> 0) & 0x7;

    // Set max payload size (=128 * (2 ^ MaxPaySizeField))
    if (MaxSize <= 5)
        pPortProp->MaxPayloadSupported = 128 * ((U16)Plx_pow_int(2, MaxSize));

    // Get PCIe Device Control
    PLX_PCI_REG_READ(
        pDevice,
        Offset_PcieCap + 0x08,
        &RegValue
        );

    // Get max payload size field
    MaxSize = (U8)(RegValue >> 5) & 0x7;

    // Set max payload size (=128 * (2 ^ MaxPaySizeField))
    if (MaxSize <= 5)
        pPortProp->MaxPayloadSize = 128 * ((U16)Plx_pow_int(2, MaxSize));

    // Get max read request size field
    MaxSize = (U8)(RegValue >> 12) & 0x7;

    // Set max read request size (=128 * (2 ^ MaxReadReqSizeField))
    if (MaxSize <= 5)
        pPortProp->MaxReadReqSize = 128 * ((U16)Plx_pow_int(2, MaxSize));

    // Get PCIe Link Capabilities
    PLX_PCI_REG_READ(
        pDevice,
        Offset_PcieCap + 0x0C,
        &RegValue
        );

    // Get port number
    pPortProp->PortNumber = (U8)((RegValue >> 24) & 0xFF);

    // Get max link width
    pPortProp->MaxLinkWidth = (U8)((RegValue >> 4) & 0x3F);

    // Get max link speed
    pPortProp->MaxLinkSpeed = (U8)((RegValue >> 0) & 0xF);

    // Get PCIe Link Status/Control
    PLX_PCI_REG_READ(
        pDevice,
        Offset_PcieCap + 0x10,
        &RegValue
        );

    // Get link width
    pPortProp->LinkWidth = (U8)((RegValue >> 20) & 0x3F);

    // Get link speed
    pPortProp->LinkSpeed = (U8)((RegValue >> 16) & 0xF);

    DebugPrintf((
        "Type=%d Num=%d MaxPd=%d/%d MaxRdReq=%d LW=x%d/x%d LS=%d/%d\n",
        pPortProp->PortType, pPortProp->PortNumber,
        pPortProp->MaxPayloadSize, pPortProp->MaxPayloadSupported,
        pPortProp->MaxReadReqSize,
        pPortProp->LinkWidth, pPortProp->MaxLinkWidth,
        pPortProp->LinkSpeed, pPortProp->MaxLinkSpeed
        ));

    return ApiSuccess;
}




/*******************************************************************************
 *
 * Function   :  PlxI2c_DeviceReset
 *
 * Description:  Resets a device using software reset feature of PLX chip
 *
 ******************************************************************************/
PLX_STATUS
PlxI2c_DeviceReset(
    PLX_DEVICE_OBJECT *pDevice
    )
{
    // Device reset not implemented
    return ApiUnsupportedFunction;
}




/*******************************************************************************
 *
 * Function   :  PlxI2c_PlxRegisterRead
 *
 * Description:  Reads a PLX-specific register
 *
 ******************************************************************************/
U32
PlxI2c_PlxRegisterRead(
    PLX_DEVICE_OBJECT *pDevice,
    U32                offset,
    PLX_STATUS        *pStatus,
    BOOLEAN            bAdjustForPort
    )
{
    int Status_AA;
    U16 nBytesRead;
    U16 nBytesWritten;
    U32 Command;
    U32 RegValue;


    // Verify register offset
    if (offset & 0x3)
    {
        DebugPrintf(("ERROR - Invalid register offset (0x%x)\n", offset));

        if (pStatus != NULL)
            *pStatus = ApiInvalidOffset;

        return 0;
    }

    // Default to error
    if (pStatus != NULL)
        *pStatus = ApiFailed;

    // Generate the I2C command
    Command =
        PlxI2c_GenerateCommand(
            pDevice,
            PLX_I2C_CMD_REG_READ,
            offset,
            bAdjustForPort
            );

    // Some I2C commands cannot be sent to chip
    if (Command == PLX_I2C_CMD_SKIP)
    {
        if (pStatus != NULL)
            *pStatus = ApiSuccess;
        return 0;
    }

    if (Command == PLX_I2C_CMD_ERROR)
        return 0;

    // Set default return value
    RegValue = 0;

    // Get the register access lock
    EnterCriticalSection(
        &(Gbl_PlxI2cProp[pDevice->Key.ApiIndex].Lock_I2cRegAccess)
        );

    // Issue read command and get data
    Status_AA =
        aa_i2c_write_read(
            (Aardvark)PLX_PTR_TO_INT( pDevice->hDevice ),
            pDevice->Key.DeviceNumber,
            AA_I2C_NO_FLAGS,
            sizeof(U32),                    // Num write bytes
            (U8*)&Command,                  // Write data
            &nBytesWritten,                 // Bytes written
            sizeof(U32),                    // Num bytes to read
            (U8*)&RegValue,                 // Read data buffer
            &nBytesRead                     // Bytes read
            );

    // Read status in [15:8] and write status in [7:0]
    if (((Status_AA >> 8) == AA_OK) && ((Status_AA & 0xFF) == AA_OK) &&
        (nBytesRead == sizeof(U32)) && (nBytesWritten == sizeof(U32)))
    {
        if (pStatus != NULL)
            *pStatus = ApiSuccess;

        // Convert to Big Endian format since data is returned in BE
        RegValue = PLX_BE_DATA_32( RegValue );
    }
    else
    {
        RegValue = 0;
    }

    // Release the register access lock
    LeaveCriticalSection(
        &(Gbl_PlxI2cProp[pDevice->Key.ApiIndex].Lock_I2cRegAccess)
        );

    return RegValue;
}




/*******************************************************************************
 *
 * Function   :  PlxI2c_PlxRegisterWrite
 *
 * Description:  Writes to a PLX-specific register
 *
 ******************************************************************************/
PLX_STATUS
PlxI2c_PlxRegisterWrite(
    PLX_DEVICE_OBJECT *pDevice,
    U32                offset,
    U32                value,
    BOOLEAN            bAdjustForPort
    )
{
    int Status_AA;
    U32 Command;
    U32 DataStream[2];


    // Verify register offset
    if (offset & 0x3)
    {
        DebugPrintf(("ERROR - Invalid register offset (0x%x)\n", offset));
        return ApiInvalidOffset;
    }

    // Generate the I2C command
    Command =
        PlxI2c_GenerateCommand(
            pDevice,
            PLX_I2C_CMD_REG_WRITE,
            offset,
            bAdjustForPort
            );

    // Some I2C commands cannot be sent to chip
    if (Command == PLX_I2C_CMD_SKIP)
        return ApiSuccess;

    if (Command == PLX_I2C_CMD_ERROR)
        return ApiFailed;

    // Convert register value to Big Endian format
    value = PLX_BE_DATA_32( value );

    // Copy command and value into data stream
    DataStream[0] = Command;
    DataStream[1] = value;

    // Get the register access lock
    EnterCriticalSection(
        &(Gbl_PlxI2cProp[pDevice->Key.ApiIndex].Lock_I2cRegAccess)
        );

    // Issue command & register value
    Status_AA =
        aa_i2c_write(
            (Aardvark)PLX_PTR_TO_INT( pDevice->hDevice ),
            pDevice->Key.DeviceNumber,
            AA_I2C_NO_FLAGS,
            2 * sizeof(U32),
            (U8*)DataStream
            );

    // Release the register access lock
    LeaveCriticalSection(
        &(Gbl_PlxI2cProp[pDevice->Key.ApiIndex].Lock_I2cRegAccess)
        );

    if (Status_AA != (2 * sizeof(U32)))
        return ApiFailed;

    return ApiSuccess;
}




/*******************************************************************************
 *
 * Function   :  PlxI2c_PciBarProperties
 *
 * Description:  Returns the properties of a PCI BAR space
 *
 ******************************************************************************/
PLX_STATUS
PlxI2c_PciBarProperties(
    PLX_DEVICE_OBJECT *pDevice,
    U8                 BarIndex,
    PLX_PCI_BAR_PROP  *pBarProperties
    )
{
    U32 PciBar;
    U32 Size;
    U32 PciHeaderType;


    // Get PCI header type
    PLX_PCI_REG_READ(
        pDevice,
        0xC,            // PCI Header Type
        &PciHeaderType
        );

    PciHeaderType = (U8)((PciHeaderType >> 16) & 0x7F);

    // Verify BAR index
    switch (PciHeaderType)
    {
        case 0:
            if ((BarIndex != 0) && (BarIndex > 5))
                return ApiInvalidIndex;
            break;

        case 1:
            if ((BarIndex != 0) && (BarIndex != 1))
            {
                DebugPrintf(("BAR %d does not exist on PCI type 1 header\n", BarIndex));
                return ApiInvalidIndex;
            }
            break;

        default:
            return ApiInvalidIndex;
    }

    // Get BAR properties if not probed yet
    if (pDevice->PciBar[BarIndex].BarValue == 0)
    {
        // Read PCI BAR
        PLX_PCI_REG_READ(
            pDevice,
            0x10 + (BarIndex * sizeof(U32)),
            &PciBar
            );

        // Query BAR range
        PLX_PCI_REG_WRITE(
            pDevice,
            0x10 + (BarIndex * sizeof(U32)),
            (U32)-1
            );

        // Read size
        PLX_PCI_REG_READ(
            pDevice,
            0x10 + (BarIndex * sizeof(U32)),
            &Size
            );

        // Restore BAR
        PLX_PCI_REG_WRITE(
            pDevice,
            0x10 + (BarIndex * sizeof(U32)),
            PciBar
            );

        // If upper 32-bit of 64-bit BAR, all bits will be set, which is not supported
        if (Size == (U32)-1)
            Size = 0;

        // Store BAR properties
        pDevice->PciBar[BarIndex].BarValue      = PciBar;
        pDevice->PciBar[BarIndex].bIoSpace      = (BOOLEAN)((PciBar >> 0) & (1 << 0));
        pDevice->PciBar[BarIndex].bPrefetchable = (BOOLEAN)((PciBar >> 3) & (1 << 0));
        pDevice->PciBar[BarIndex].b64bit        = (BOOLEAN)((PciBar >> 2) & (1 << 0));

        // Determine BAR address & size based on space type
        if (pDevice->PciBar[BarIndex].bIoSpace)
        {
            pDevice->PciBar[BarIndex].Physical = PciBar & ~0x3;
            pDevice->PciBar[BarIndex].Size     = (~(Size & ~0x3)) + 1;
        }
        else
        {
            pDevice->PciBar[BarIndex].Physical = PciBar & ~0xF;
            pDevice->PciBar[BarIndex].Size     = (~(Size & ~0xF)) + 1;
        }
    }

    // Return BAR properties
    *pBarProperties = pDevice->PciBar[BarIndex];

    // Display BAR properties if enabled
    if (pDevice->PciBar[BarIndex].Size == 0)
    {
        DebugPrintf((
            "PCI BAR %d is disabled\n",
            BarIndex
            ));
    }
    else
    {
        DebugPrintf((
            "    PCI BAR %d: %08lX\n",
            BarIndex, (PLX_UINT_PTR)pDevice->PciBar[BarIndex].BarValue
            ));

        DebugPrintf((
            "    Phys Addr: %08lX\n",
            (PLX_UINT_PTR)pDevice->PciBar[BarIndex].Physical
            ));

        if (pDevice->PciBar[BarIndex].Size >= (1 << 10))
        {
            DebugPrintf((
                "    Size     : %08lx  (%ld Kb)\n",
                (PLX_UINT_PTR)pDevice->PciBar[BarIndex].Size,
                (PLX_UINT_PTR)pDevice->PciBar[BarIndex].Size >> 10
                ));
        }
        else
        {
            DebugPrintf((
                "    Size     : %08lx  (%ld bytes)\n",
                (PLX_UINT_PTR)pDevice->PciBar[BarIndex].Size,
                (PLX_UINT_PTR)pDevice->PciBar[BarIndex].Size
                ));
        }

        DebugPrintf((
            "    Prefetch?: %s\n",
            (pDevice->PciBar[BarIndex].bPrefetchable) ? "Yes" : "No"
            ));
    }

    return ApiSuccess;
}




/*******************************************************************************
 *
 * Function   :  PlxI2c_EepromPresent
 *
 * Description:  Returns the state of the EEPROM as reported by the PLX device
 *
 ******************************************************************************/
PLX_STATUS
PlxI2c_EepromPresent(
    PLX_DEVICE_OBJECT *pDevice,
    PLX_EEPROM_STATUS *pStatus
    )
{
    // Default to no EEPROM present
    *pStatus = PLX_EEPROM_STATUS_NONE;

    switch (pDevice->Key.PlxChip & 0xFF00)
    {
        case 0x8400:
        case 0x8500:
        case 0x8600:
        case 0x8700:
            return Plx8000_EepromPresent(
                pDevice,
                (U8*)pStatus
                );
    }

    return ApiUnsupportedFunction;
}




/*******************************************************************************
 *
 * Function   :  PlxI2c_EepromProbe
 *
 * Description:  Probes for the presence of an EEPROM
 *
 ******************************************************************************/
PLX_STATUS
PlxI2c_EepromProbe(
    PLX_DEVICE_OBJECT *pDevice,
    BOOLEAN           *pFlag
    )
{
    U16        OffsetProbe;
    U32        TempChip;
    U32        ValueRead;
    U32        ValueWrite;
    U32        ValueOriginal;
    PLX_STATUS status;


    // Default to no EEPROM present
    *pFlag = FALSE;

    // Generalize by device type
    TempChip = pDevice->Key.PlxChip & 0xFF00;

    if ((TempChip != 0x8400) && (TempChip != 0x8600) && (TempChip != 0x8700))
    {
        TempChip = pDevice->Key.PlxChip;
    }

    // Determine EEPROM offset to use for probe (e.g. after CRC)
    switch (TempChip)
    {
        case 0x8508:
        case 0x8512:
        case 0x8517:
        case 0x8518:
            OffsetProbe = (0x78F * sizeof(U32)) + sizeof(U32);
            break;

        case 0x8516:
        case 0x8524:
        case 0x8532:
            OffsetProbe = (0xBE4 * sizeof(U32)) + sizeof(U32);
            break;

        case 0x8400:
        case 0x8505:
        case 0x8509:
        case 0x8525:
        case 0x8533:
        case 0x8547:
        case 0x8548:
        case 0x8600:
        case 0x8700:
            OffsetProbe = 0x10;     // No CRC, just use any reasonable address
            break;

        case 0:
        default:
            DebugPrintf((
                "ERROR - Not a supported PLX device (%04X)\n",
                pDevice->Key.PlxChip
                ));
            return ApiUnsupportedFunction;
    }

    DebugPrintf(("Probing EEPROM at offset %02xh\n", OffsetProbe));

    // Get the current value
    status =
        PlxI2c_EepromReadByOffset(
            pDevice,
            OffsetProbe,
            &ValueOriginal
            );

    if (status != ApiSuccess)
        return status;

    // Prepare inverse value to write
    ValueWrite = ~(ValueOriginal);

    // Write inverse of original value
    status =
        PlxI2c_EepromWriteByOffset(
            pDevice,
            OffsetProbe,
            ValueWrite
            );

    if (status != ApiSuccess)
        return status;

    // Read updated value
    status =
        PlxI2c_EepromReadByOffset(
            pDevice,
            OffsetProbe,
            &ValueRead
            );

    if (status != ApiSuccess)
        return status;

    // Check if value was written properly
    if (ValueRead == ValueWrite)
    {
        DebugPrintf(("Probe detected an EEPROM present\n"));

        *pFlag = TRUE;

        // Restore the original value
        PlxI2c_EepromWriteByOffset(
            pDevice,
            OffsetProbe,
            ValueOriginal
            );
    }
    else
    {
        DebugPrintf(("Probe did not detect an EEPROM\n"));
    }

    return ApiSuccess;
}




/*******************************************************************************
 *
 * Function   :  PlxI2c_EepromSetAddressWidth
 *
 * Description:  Sets a new EEPROM address width
 *
 ******************************************************************************/
PLX_STATUS
PlxI2c_EepromSetAddressWidth(
    PLX_DEVICE_OBJECT *pDevice,
    U8                 width
    )
{
    PLX_STATUS status;


    // Verify the width
    switch (width)
    {
        case 1:
        case 2:
        case 3:
            break;

        default:
            DebugPrintf((
                "ERROR - Invalid address width (%d)\n",
                width
                ));
            return ApiInvalidData;
    }

    switch (pDevice->Key.PlxChip & 0xFF00)
    {
        case 0x8400:
        case 0x8500:
        case 0x8600:
        case 0x8700:
            status =
                Plx8000_EepromSetAddressWidth(
                    pDevice,
                    width
                    );
            break;

        default:
            DebugPrintf((
                "ERROR - Chip (%04X) does not support address width override\n",
                pDevice->Key.PlxChip
                ));
            return ApiUnsupportedFunction;
    }

    if (status == ApiSuccess)
    {
        DebugPrintf((
            "Set EEPROM address width to %d bytes\n",
            width
            ));
    }

    return status;
}




/*******************************************************************************
 *
 * Function   :  PlxI2c_EepromCrcGet
 *
 * Description:  Returns the EEPROM CRC and its status
 *
 ******************************************************************************/
PLX_STATUS
PlxI2c_EepromCrcGet(
    PLX_DEVICE_OBJECT *pDevice,
    U32               *pCrc,
    U8                *pCrcStatus
    )
{
    // Clear return value
    *pCrc = 0;

    switch (pDevice->Key.PlxChip)
    {
        case 0x8114:
        case 0x8508:
        case 0x8512:
        case 0x8516:
        case 0x8517:
        case 0x8518:
        case 0x8524:
        case 0x8532:
            return Plx8000_EepromCrcGet(
                pDevice,
                pCrc,
                pCrcStatus
                );

        default:
            // Devices don't support CRC
            break;
    }

    return ApiUnsupportedFunction;
}




/*******************************************************************************
 *
 * Function   :  PlxI2c_EepromCrcUpdate
 *
 * Description:  Updates the EEPROM CRC
 *
 ******************************************************************************/
PLX_STATUS
PlxI2c_EepromCrcUpdate(
    PLX_DEVICE_OBJECT *pDevice,
    U32               *pCrc,
    BOOLEAN            bUpdateEeprom
    )
{
    // Clear return value
    *pCrc = 0;

    switch (pDevice->Key.PlxChip)
    {
        case 0x8114:
        case 0x8508:
        case 0x8512:
        case 0x8516:
        case 0x8517:
        case 0x8518:
        case 0x8524:
        case 0x8532:
            return Plx8000_EepromCrcUpdate(
                pDevice,
                pCrc,
                bUpdateEeprom
                );

        default:
            // Devices don't support CRC
            break;
    }

    return ApiUnsupportedFunction;
}




/*******************************************************************************
 *
 * Function   :  PlxI2c_EepromReadByOffset
 *
 * Description:  Read a 32-bit value from the EEPROM at a specified offset
 *
 ******************************************************************************/
PLX_STATUS
PlxI2c_EepromReadByOffset(
    PLX_DEVICE_OBJECT *pDevice,
    U16                offset,
    U32               *pValue
    )
{
    // Set default return value
    *pValue = 0;

    // Make sure offset is aligned on 32-bit boundary
    if (offset & (3 << 0))
    {
        return ApiInvalidOffset;
    }

    switch (pDevice->Key.PlxChip & 0xFF00)
    {
        case 0x8400:
        case 0x8500:
        case 0x8600:
        case 0x8700:
            return Plx8000_EepromReadByOffset(
                pDevice,
                offset,
                pValue
                );
    }

    return ApiUnsupportedFunction;
}




/*******************************************************************************
 *
 * Function   :  PlxI2c_EepromWriteByOffset
 *
 * Description:  Write a 32-bit value to the EEPROM at a specified offset
 *
 ******************************************************************************/
PLX_STATUS
PlxI2c_EepromWriteByOffset(
    PLX_DEVICE_OBJECT *pDevice,
    U16                offset,
    U32                value
    )
{
    // Make sure offset is aligned on 32-bit boundary
    if (offset & (3 << 0))
    {
        return ApiInvalidOffset;
    }

    switch (pDevice->Key.PlxChip & 0xFF00)
    {
        case 0x8400:
        case 0x8500:
        case 0x8600:
        case 0x8700:
            return Plx8000_EepromWriteByOffset(
                pDevice,
                offset,
                value
                );
    }

    return ApiUnsupportedFunction;
}




/*******************************************************************************
 *
 * Function   :  PlxI2c_EepromReadByOffset_16
 *
 * Description:  Read a 16-bit value from the EEPROM at a specified offset
 *
 ******************************************************************************/
PLX_STATUS
PlxI2c_EepromReadByOffset_16(
    PLX_DEVICE_OBJECT *pDevice,
    U16                offset,
    U16               *pValue
    )
{
    U32        Value_32;
    PLX_STATUS status;


    // Set default return value
    *pValue = 0;

    // Make sure offset is aligned on 16-bit boundary
    if (offset & (1 << 0))
    {
        return ApiInvalidOffset;
    }

    switch (pDevice->Key.PlxChip & 0xFF00)
    {
        case 0x8400:
        case 0x8500:
        case 0x8600:
        case 0x8700:
            /******************************************
             * For devices that do not support 16-bit
             * EEPROM accesses, use 32-bit access
             *****************************************/

            // Get current 32-bit value
            status =
                PlxI2c_EepromReadByOffset(
                    pDevice,
                    (U16)(offset & ~0x3),
                    &Value_32
                    );

            if (status != ApiSuccess)
                return status;

            // Return desired 16-bit portion
            if ((offset & 0x3) == 2)
            {
                *pValue = (U16)(Value_32 >> 16);
            }
            else
            {
                *pValue = (U16)(Value_32);
            }
            break;

        default:
            DebugPrintf((
                "ERROR - Device (%04X) does not support 16-bit EEPROM access\n",
                pDevice->Key.PlxChip
                ));
            return ApiUnsupportedFunction;
    }

    return ApiSuccess;
}




/*******************************************************************************
 *
 * Function   :  PlxI2c_EepromWriteByOffset_16
 *
 * Description:  Write a 16-bit value to the EEPROM at a specified offset
 *
 ******************************************************************************/
PLX_STATUS
PlxI2c_EepromWriteByOffset_16(
    PLX_DEVICE_OBJECT *pDevice,
    U16                offset,
    U16                value
    )
{
    U32        Value_32;
    PLX_STATUS status;


    // Make sure offset is aligned on 16-bit boundary
    if (offset & (1 << 0))
    {
        return ApiInvalidOffset;
    }

    switch (pDevice->Key.PlxChip & 0xFF00)
    {
        case 0x8400:
        case 0x8500:
        case 0x8600:
        case 0x8700:
            /******************************************
             * For devices that do not support 16-bit
             * EEPROM accesses, use 32-bit access
             *****************************************/

            // Get current 32-bit value
            status =
                PlxI2c_EepromReadByOffset(
                    pDevice,
                    (U16)(offset & ~0x3),
                    &Value_32
                    );

            if (status != ApiSuccess)
                return status;

            // Insert new 16-bit value in correct location
            if ((offset & 0x3) == 2)
            {
                Value_32 = ((U32)value << 16) | (Value_32 & 0xFFFF);
            }
            else
            {
                Value_32 = ((U32)value) | (Value_32 & 0xFFFF0000);
            }

            // Update EEPROM
            return PlxI2c_EepromWriteByOffset(
                pDevice,
                (U16)(offset & ~0x3),
                Value_32
                );

        default:
            DebugPrintf((
                "ERROR - Device (%04X) does not support 16-bit EEPROM access\n",
                pDevice->Key.PlxChip
                ));
            return ApiUnsupportedFunction;
    }

    return ApiSuccess;
}




/*******************************************************************************
 *
 * Function   :  PlxI2c_PerformanceInitializeProperties
 *
 * Description:  Initializes the performance properties for a device
 *
 ******************************************************************************/
PLX_STATUS
PlxI2c_PerformanceInitializeProperties(
    PLX_DEVICE_OBJECT *pDevice,
    PLX_PERF_PROP     *pPerfProp
    )
{
    U8            PortsPerStation;
    PLX_PORT_PROP PortProp;


    // Clear performance object
    RtlZeroMemory( pPerfProp, sizeof(PLX_PERF_PROP) );

    // Verify supported device & set number of ports per station
    switch (pDevice->Key.PlxFamily)
    {
        case PLX_FAMILY_DENEB:
        case PLX_FAMILY_CYGNUS:
            PortsPerStation = 4;
            break;

        case PLX_FAMILY_SIRIUS:
            PortsPerStation = 16;
            break;

        case PLX_FAMILY_DRACO_1:
        case PLX_FAMILY_DRACO_2:
            PortsPerStation = 8;    // Device actually only uses 6 ports out of 8
            break;

        default:
            DebugPrintf(("Error: Unsupported PLX chip (%04X)\n", pDevice->Key.PlxChip));
            return ApiUnsupportedFunction;
    }

    // Get port properties
    PlxI2c_GetPortProperties(
        pDevice,
        &PortProp
        );

    if (PortProp.PortNumber >= 24)
    {
        DebugPrintf(("ERROR - Port number exceeds maximum (%d)\n", (24-1)));
        return ApiUnsupportedFunction;
    }

    // Store relevant port properties for later calculations
    pPerfProp->PortNumber = PortProp.PortNumber;
    pPerfProp->LinkWidth  = PortProp.LinkWidth;
    pPerfProp->LinkSpeed  = PortProp.LinkSpeed;

    // Determine station and port number within station
    pPerfProp->Station     = (U8)(PortProp.PortNumber / PortsPerStation);
    pPerfProp->StationPort = (U8)(PortProp.PortNumber % PortsPerStation);

    return ApiSuccess;
}




/*******************************************************************************
 *
 * Function   :  PlxI2c_PerformanceMonitorControl
 *
 * Description:  Controls PLX Performance Monitor
 *
 ******************************************************************************/
PLX_STATUS
PlxI2c_PerformanceMonitorControl(
    PLX_DEVICE_OBJECT *pDevice,
    PLX_PERF_CMD       command
    )
{
    U8  Bit_EgressEn;
    U16 Offset_Control;
    U32 offset;
    U32 RegValue;
    U32 RegCommand;
    U32 NumStations;
    U32 PortsPerStation;


    // Verify supported device
    switch (pDevice->Key.PlxFamily)
    {
        case PLX_FAMILY_DENEB:
        case PLX_FAMILY_SIRIUS:
            Offset_Control = 0x568;
            break;

        case PLX_FAMILY_CYGNUS:
        case PLX_FAMILY_DRACO_1:
        case PLX_FAMILY_DRACO_2:
            Offset_Control = 0x3E0;
            break;

        default:
            DebugPrintf(("Error: Unsupported PLX chip (%04X)\n", pDevice->Key.PlxChip));
            return ApiUnsupportedFunction;
    }

    switch (command)
    {
        case PLX_PERF_CMD_START:
            DebugPrintf(("Reset & enable monitor with infinite sampling\n"));
            RegCommand = (1 << 31) | (1 << 30) | (1 << 28) | (1 << 27);
            break;

        case PLX_PERF_CMD_STOP:
            DebugPrintf(("Reset & disable monitor\n"));
            RegCommand = (1 << 30);
            break;

        default:
            return ApiInvalidData;
    }

    // Added to avoid compiler warning
    Bit_EgressEn    = 0;
    NumStations     = 0;
    PortsPerStation = 0;

    // Set control offset & enable/disable counters in stations
    switch (pDevice->Key.PlxFamily)
    {
        case PLX_FAMILY_CYGNUS:
        case PLX_FAMILY_DRACO_1:
        case PLX_FAMILY_DRACO_2:
            // Set device configuration
            if (pDevice->Key.PlxFamily == PLX_FAMILY_CYGNUS)
            {
                Bit_EgressEn    = 7;
                NumStations     = 6;
                PortsPerStation = 4;
            }
            else if ((pDevice->Key.PlxFamily == PLX_FAMILY_DRACO_1) ||
                     (pDevice->Key.PlxFamily == PLX_FAMILY_DRACO_2))
            {
                Bit_EgressEn    = 6;
                NumStations     = 3;
                PortsPerStation = 8;    // Device actually only uses 6 ports out of 8

                // Set 3F0[9:8] to disable probe mode interval timer
                // & avoid RAM pointer corruption
                if (command == PLX_PERF_CMD_START)
                {
                    RegValue = PLX_8000_REG_READ( pDevice, 0x3F0 );
                    PLX_8000_REG_WRITE( pDevice, 0x3F0, RegValue | (3 << 8) );
                }
            }

            // Enable/Disable Performance Counter in each station
            for (offset = 0; offset < (NumStations * (PortsPerStation * 0x1000)); offset += (PortsPerStation * 0x1000))
            {
                // Ingress ports
                RegValue = PLX_8000_REG_READ( pDevice, offset + 0x768 );

                if (command == PLX_PERF_CMD_START)
                    PLX_8000_REG_WRITE( pDevice, offset + 0x768, RegValue | (1 << 29) );
                else
                    PLX_8000_REG_WRITE( pDevice, offset + 0x768, RegValue & ~(1 << 29) );

                // Egress ports
                RegValue = PLX_8000_REG_READ( pDevice, offset + 0xF30 );

                if (command == PLX_PERF_CMD_START)
                    PLX_8000_REG_WRITE( pDevice, offset + 0xF30, RegValue | (1 << Bit_EgressEn) );
                else
                    PLX_8000_REG_WRITE( pDevice, offset + 0xF30, RegValue & ~(1 << Bit_EgressEn) );
            }
            break;
    }

    // Update monitor
    PLX_8000_REG_WRITE(
        pDevice,
        Offset_Control,
        RegCommand
        );

    return ApiSuccess;
}




/*******************************************************************************
 *
 * Function   :  PlxI2c_PerformanceResetCounters
 *
 * Description:  Resets the internal performance counters
 *
 ******************************************************************************/
PLX_STATUS
PlxI2c_PerformanceResetCounters(
    PLX_DEVICE_OBJECT *pDevice
    )
{
    U16 Offset_Control;


    // Verify supported device
    switch (pDevice->Key.PlxFamily)
    {
        case PLX_FAMILY_DENEB:
        case PLX_FAMILY_SIRIUS:
            Offset_Control = 0x568;
            break;

        case PLX_FAMILY_CYGNUS:
        case PLX_FAMILY_DRACO_1:
        case PLX_FAMILY_DRACO_2:
            Offset_Control = 0x3E0;
            break;

        default:
            DebugPrintf(("Error: Unsupported PLX chip (%04X)\n", pDevice->Key.PlxChip));
            return ApiUnsupportedFunction;
    }

    // Reset (30) & enable monitor (31) & infinite sampling (28) & start (27)
    PLX_8000_REG_WRITE(
        pDevice,
        Offset_Control,
        (1 << 31) | (1 << 30) | (1 << 28) | (1 << 27)
        );

    return ApiSuccess;
}




/*******************************************************************************
 *
 * Function   :  PlxI2c_PerformanceGetCounters
 *
 * Description:  Retrieves a snapshot of the Performance Counters for selected ports
 *
 * Notes      :  The counters are retrieved from the PLX chip as a preset structure.
 *               Each register read returns the next value from the sequence.  The
 *               structure varies between some PLX chips.  Below is a diagram of each.
 *
 *   IN    = Ingress port
 *   EG    = Egress port
 *   PH    = Number of Posted Headers (Write TLPs)
 *   PDW   = Number of Posted DWords
 *   NPDW  = Non-Posted DWords (Read TLP Dwords)
 *   CPLH  = Number of Completion Headers (CPL TLPs)
 *   CPLDW = Number of Completion DWords
 *   DLLP  = Number of DLLPs
 *   PHY   = PHY Layer (always 0)
 *
 *          Deneb & Cygnus                  Draco                     Sirius
 *     --------------------------   -----------------------   -------------------------
 *         14 counters/port           14 counters/port          13 counters/port
 *          4 ports/station            6 ports/station          16 ports/station
 *   Deneb: 3 stations (12 ports)      3 stations (18 ports)     1 station (16 ports)
 *  Cygnus: 6 stations (24 ports)
 *         56 counters/station        84 counters/station      208 counters/station
 * Deneb: 168 counters (56 * 3)      252 counters (84 * 3)     208 counters (208 * 1)
 *Cygnus: 336 counters (56 * 6)
 *
 *       off     Counter            off     Counter            off      Counter
 *           -----------------          -----------------          ------------------
 *         0| Port 0 IN PH    |       0| Port 0 IN PH    |       0| Port 0 IN PH     |
 *         4| Port 0 IN PDW   |       4| Port 0 IN PDW   |       4| Port 0 IN PDW    |
 *         8| Port 0 IN NPDW  |       8| Port 0 IN NPDW  |       8| Port 0 IN NPDW   |
 *         C| Port 0 IN CPLH  |       C| Port 0 IN CPLH  |       C| Port 0 IN CPLH   |
 *        10| Port 0 IN CPLDW |      10| Port 0 IN CPLDW |      10| Port 0 IN CPLDW  |
 *          |-----------------|        |-----------------|        |------------------|
 *        14| Port 1 IN PH    |      14| Port 1 IN PH    |      14| Port 1 IN PH     |
 *          |       :         |        |       :         |        |       :          |
 *          |       :         |        |       :         |        |       :          |
 *        24| Port 1 IN CPLDW |      24| Port 1 IN CPLDW |      24| Port 1 IN CPLDW  |
 *          |-----------------|        |-----------------|        |/\/\/\/\/\/\/\/\/\|
 *        28| Port 2 IN PH    |      28| Port 2 IN PH    |        |       :          |
 *          |       :         |        |       :         |        |       :          |
 *          |       :         |        |       :         |        |       :          |
 *        38| Port 2 IN CPLDW |      38| Port 2 IN CPLDW |        |       :          |
 *          |-----------------|        |-----------------|        |       :          |
 *        3C| Port 3 IN PH    |      3C| Port 3 IN PH    |        |/\/\/\/\/\/\/\/\/\|
 *          |       :         |        |       :         |     12C| Port 15 IN PH    |
 *          |       :         |        |       :         |        |       :          |
 *        4C| Port 3 IN CPLDW |      4C| Port 3 IN CPLDW |        |       :          |
 *          |-----------------|        |-----------------|     13C| Port 15 IN CPLDW |
 *        50| Port 0 EG PH    |      50| Port 4 IN PH    |        |------------------|
 *        54| Port 0 EG PDW   |        |       :         |     140| Port 0 EG PH     |
 *        58| Port 0 EG NPDW  |        |       :         |     144| Port 0 EG PDW    |
 *        5C| Port 0 EG CPLH  |      60| Port 4 IN CPLDW |     148| Port 0 EG NPDW   |
 *        60| Port 0 EG CPLDW |        |-----------------|     14C| Port 0 EG CPLH   |
 *          |-----------------|      64| Port 5 IN PH    |     150| Port 0 EG CPLDW  |
 *        64| Port 1 EG PH    |        |       :         |        |------------------|
 *          |       :         |        |       :         |     154| Port 1 EG PH     |
 *          |       :         |      74| Port 5 IN CPLDW |        |       :          |
 *        74| Port 1 EG CPLDW |        |-----------------|        |       :          |
 *          |-----------------|      78| Port 0 EG PH    |     164| Port 1 EG CPLDW  |
 *        78| Port 2 EG PH    |      7C| Port 0 EG PDW   |        |/\/\/\/\/\/\/\/\/\|
 *          |       :         |      80| Port 0 EG NPDW  |        |       :          |
 *          |       :         |      84| Port 0 EG CPLH  |        |       :          |
 *        88| Port 2 EG CPLDW |      88| Port 0 EG CPLDW |        |       :          |
 *          |-----------------|        |-----------------|        |       :          |
 *        8C| Port 3 EG PH    |      8C| Port 1 EG PH    |        |       :          |
 *          |       :         |        |       :         |        |/\/\/\/\/\/\/\/\/\|
 *          |       :         |        |       :         |     26C| Port 15 EG PH    |
 *        9C| Port 3 EG CPLDW |      9C| Port 1 EG CPLDW |        |       :          |
 *          |-----------------|        |-----------------|        |       :          |
 *        A0| Port 0 IN DLLP  |      A0| Port 2 EG PH    |     27C| Port 15 EG CPLDW |
 *        A4| Port 1 IN DLLP  |        |       :         |        |------------------|
 *        A8| Port 2 IN DLLP  |        |       :         |     280| Port  0 IN DLLP  |
 *        AC| Port 3 IN DLLP  |      B0| Port 2 EG CPLDW |     284| Port  2 IN DLLP  |
 *          |-----------------|        |-----------------|     288| Port  4 IN DLLP  |
 *        B0| Port 0 EG DLLP  |      B4| Port 3 EG PH    |     28C| Port  6 IN DLLP  |
 *        B4| Port 1 EG DLLP  |        |       :         |     290| Port  8 IN DLLP  |
 *        B8| Port 2 EG DLLP  |        |       :         |     294| Port 10 IN DLLP  |
 *        BC| Port 3 EG DLLP  |      C4| Port 3 EG CPLDW |     298| Port 12 IN DLLP  |
 *          |-----------------|        |-----------------|     29C| Port 14 IN DLLP  |
 *        C0| Port 0 IN PHY   |      C8| Port 4 EG PH    |        |------------------|
 *        C4| Port 1 IN PHY   |        |       :         |     2A0| Port  0 EG DLLP  |
 *        C8| Port 2 IN PHY   |        |       :         |     2A4| Port  2 EG DLLP  |
 *        CC| Port 3 IN PHY   |      D8| Port 4 EG CPLDW |     2A8| Port  4 EG DLLP  |
 *          |-----------------|        |-----------------|     2AC| Port  6 EG DLLP  |
 *        D0| Port 0 EG PHY   |      DC| Port 5 EG PH    |     2B0| Port  8 EG DLLP  |
 *        D4| Port 1 EG PHY   |        |       :         |     2B4| Port 10 EG DLLP  |
 *        D8| Port 2 EG PHY   |        |       :         |     2B8| Port 12 EG DLLP  |
 *        DC| Port 3 EG PHY   |      EC| Port 5 EG CPLDW |     2BC| Port 14 EG DLLP  |
 *           -----------------         |-----------------|        |------------------|
 *                                   F0| Port 0 IN DLLP  |     2C0| Port  1 IN DLLP  |
 *                                   F4| Port 1 IN DLLP  |     2C4| Port  3 IN DLLP  |
 *                                   F8| Port 2 IN DLLP  |     2C8| Port  5 IN DLLP  |
 *                                   FC| Port 3 IN DLLP  |     2CC| Port  7 IN DLLP  |
 *                                  100| Port 4 IN DLLP  |     2D0| Port  9 IN DLLP  |
 *                                  104| Port 5 IN DLLP  |     2D4| Port 11 IN DLLP  |
 *                                     |-----------------|     2D8| Port 13 IN DLLP  |
 *                                  108| Port 0 EG DLLP  |     2DC| Port 15 IN DLLP  |
 *                                  10C| Port 1 EG DLLP  |        |------------------|
 *                                  110| Port 2 EG DLLP  |     2E0| Port  1 EG DLLP  |
 *                                  114| Port 3 EG DLLP  |     2E4| Port  3 EG DLLP  |
 *                                  118| Port 4 EG DLLP  |     2E8| Port  5 EG DLLP  |
 *                                  11C| Port 5 EG DLLP  |     2EC| Port  7 EG DLLP  |
 *                                     |-----------------|     2F0| Port  9 EG DLLP  |
 *                                  120| Port 0 IN PHY   |     2F4| Port 11 EG DLLP  |
 *                                  124| Port 1 IN PHY   |     2F8| Port 13 EG DLLP  |
 *                                  128| Port 2 IN PHY   |     2FC| Port 15 EG DLLP  |
 *                                  12C| Port 3 IN PHY   |        |------------------|
 *                                  130| Port 4 IN PHY   |     300| Port 0 PHY       |
 *                                  134| Port 5 IN PHY   |        |       :          |
 *                                     |-----------------|        |       :          |
 *                                  138| Port 0 EG PHY   |        |       :          |
 *                                  13C| Port 1 EG PHY   |     33C| Port 15 PHY      |
 *                                  140| Port 2 EG PHY   |         ------------------
 *                                  144| Port 3 EG PHY   |
 *                                  148| Port 4 EG PHY   |
 *                                  14C| Port 5 EG PHY   |
 *                                      ----------------- 
 ******************************************************************************/
PLX_STATUS
PlxI2c_PerformanceGetCounters(
    PLX_DEVICE_OBJECT *pDevice,
    PLX_PERF_PROP     *pPerfProps,
    U8                 NumOfObjects
    )
{
    U8          NumPorts;
    U8          NumCounters;
    U8          PortsPerStation;
    U16         i;
    U16         index;
    U16         IndexBase;
    U16         Offset_Fifo;
    U16         Offset_RamCtrl;
    U32         RegValue;
    U32        *pCounter;
    U32        *pCounter_Prev;
    U32         Counter_PrevTmp[14];
    S64         TmpValue;
    static U32  Counters[336];     // Cygnus currently max (24 ports * 14 counters/port)


    // Setup parameters for reading counters
    switch (pDevice->Key.PlxFamily)
    {
        case PLX_FAMILY_DENEB:
            Offset_RamCtrl  = 0x618;
            Offset_Fifo     = 0x628;
            NumPorts        = 12;
            NumCounters     = 14;
            PortsPerStation = 4;
            break;

        case PLX_FAMILY_SIRIUS:
            Offset_RamCtrl  = 0x618;
            Offset_Fifo     = 0x628;
            NumPorts        = 16;
            NumCounters     = 13;
            PortsPerStation = 16;
            break;

        case PLX_FAMILY_CYGNUS:
            Offset_RamCtrl  = 0x3F0;
            Offset_Fifo     = 0x3E4;
            NumPorts        = 24;
            NumCounters     = 14;
            PortsPerStation = 4;
            break;

        case PLX_FAMILY_DRACO_1:
        case PLX_FAMILY_DRACO_2:
            Offset_RamCtrl  = 0x3F0;
            Offset_Fifo     = 0x3E4;
            NumPorts        = 18;
            NumCounters     = 14;
            PortsPerStation = 6;
            break;

        default:
            DebugPrintf(("Error: Unsupported PLX chip (%04X)\n", pDevice->Key.PlxChip));
            return ApiUnsupportedFunction;
    }

    // RAM control default - Set capture type [5:4] to 2 & enable RAM (bit 0)
    RegValue = (2 << 4) | (1 << 0);

    // Reset RAM read pointer (bit 2)
    PLX_8000_REG_WRITE(
        pDevice,
        Offset_RamCtrl,
        RegValue | (1 << 2)
        );

    // Read in all counters
    i = 0;
    while (i < (NumPorts * NumCounters))
    {
        // Get next counter
        Counters[i] =
            PLX_8000_REG_READ(
                pDevice,
                Offset_Fifo
                );

        // Jump to next counter
        i++;
    }

    // Assign counter values to enabled ports
    i = 0;
    while (i < NumOfObjects)
    {
        // Make a copy of the previous values before overwriting them
        RtlCopyMemory(
            Counter_PrevTmp,
            &(pPerfProps[i].Prev_IngressPostedHeader),
            14 * sizeof(U32)    // All 14 counters in structure
            );

        // Save current values to previous
        RtlCopyMemory(
            &(pPerfProps[i].Prev_IngressPostedHeader),
            &(pPerfProps[i].IngressPostedHeader),
            14 * sizeof(U32)    // All 14 counters in structure
            );

        // Calculate starting index for counters based on port in station
        IndexBase = pPerfProps[i].Station * (NumCounters * PortsPerStation);

        // Ingress counters start at index 0 from base
        index = IndexBase + 0 + (pPerfProps[i].StationPort * 5);

        // Get Ingress counters (5 DW/port)
        pPerfProps[i].IngressPostedHeader = Counters[index + 0];
        pPerfProps[i].IngressPostedDW     = Counters[index + 1];
        pPerfProps[i].IngressNonpostedDW  = Counters[index + 2];
        pPerfProps[i].IngressCplHeader    = Counters[index + 3];
        pPerfProps[i].IngressCplDW        = Counters[index + 4];

        // Egress counters start after ingress
        index = IndexBase + (5 * PortsPerStation) + (pPerfProps[i].StationPort * 5);

        // Get Egress counters (5 DW/port)
        pPerfProps[i].EgressPostedHeader = Counters[index + 0];
        pPerfProps[i].EgressPostedDW     = Counters[index + 1];
        pPerfProps[i].EgressNonpostedDW  = Counters[index + 2];
        pPerfProps[i].EgressCplHeader    = Counters[index + 3];
        pPerfProps[i].EgressCplDW        = Counters[index + 4];

        // DLLP Ingress counters start after egress
        index = IndexBase + (10 * PortsPerStation);

        // DLLP counter location depends upon chip
        if (pDevice->Key.PlxFamily == PLX_FAMILY_SIRIUS)
        {
            // Even port number DLLP counters are first
            index += (pPerfProps[i].StationPort / 2);

            // Odd port number DLLP counters follow even ports
            if (pPerfProps[i].StationPort & (1 << 0))
                index += PortsPerStation;
        }
        else
        {
            index += pPerfProps[i].StationPort;
        }

        // Get DLLP Ingress counters (1 DW/port)
        pPerfProps[i].IngressDllp = Counters[index];

        // Egress DLLP counters follow Ingress
        if (pDevice->Key.PlxFamily == PLX_FAMILY_SIRIUS)
        {
            // Even ports are grouped together
            index += (PortsPerStation / 2);
        }
        else
        {
            index += PortsPerStation;
        }

        // Get DLLP Egress counters (1 DW/port)
        pPerfProps[i].EgressDllp = Counters[index];

        // Any PHY counters are always 0, so ignore any values
        pPerfProps[i].IngressPhy = 0;
        pPerfProps[i].EgressPhy  = 0;

        /**********************************************************
         * In some cases on Draco 1 chips, device may incorrectly
         * report a counter as 0. The following code checks the
         * current & previous counters to detect this case. If the
         * issue is present, the previous value is used instead to
         * minimize data reporting errors.
         *********************************************************/
        if ((pDevice->Key.PlxFamily == PLX_FAMILY_DRACO_1) &&
            (pPerfProps[i].LinkWidth != 0))
        {
            // Setup initial pointers to stored counters
            pCounter      = &pPerfProps[i].IngressPostedHeader;
            pCounter_Prev = &pPerfProps[i].Prev_IngressPostedHeader;

            // Verify each counter & use previous on error
            for (index = 0; index < 14; index++)
            {
                if ((*pCounter == 0) && (*pCounter_Prev != 0))
                {
                    // Store 64-bit counter in case of wrapping
                    TmpValue = *pCounter_Prev;
                    if (*pCounter_Prev < Counter_PrevTmp[index])
                        TmpValue += ((S64)1 << 32);

                    // Re-use difference between previous 2 counter values
                    *pCounter = *pCounter_Prev + (U32)(TmpValue - Counter_PrevTmp[index]);
                }

                // Increment to next counter
                pCounter++;
                pCounter_Prev++;
            }
        }

        // Go to next performance object
        i++;
    }

    return ApiSuccess;
}




/*******************************************************************************
 *
 * Function   :  PlxI2c_MH_GetProperties
 *
 * Description:  Controls PLX Performance Monitor
 *
 ******************************************************************************/
PLX_STATUS
PlxI2c_MH_GetProperties(
    PLX_DEVICE_OBJECT   *pDevice,
    PLX_MULTI_HOST_PROP *pMHProp
    )
{
    U8  i;
    U8  TotalVS;
    U32 RegValue;
    U32 RegVSEnable;


    // Clear properties
    RtlZeroMemory( pMHProp, sizeof(PLX_MULTI_HOST_PROP) );

    // Default to standard mode
    pMHProp->SwitchMode = PLX_SWITCH_MODE_STANDARD;

    // Verify supported device
    switch (pDevice->Key.PlxFamily)
    {
        case PLX_FAMILY_CYGNUS:
        case PLX_FAMILY_DRACO_1:
        case PLX_FAMILY_DRACO_2:
            break;

        default:
            DebugPrintf(("ERROR - Device (%04X) doesn't support multi-host\n", pDevice->Key.PlxChip));
            return ApiUnsupportedFunction;
    }

    // Attempt to read management port configuration
    RegValue =
        PLX_8000_REG_READ(
            pDevice,
            0x354
            );

    // Device properties are only available from the management port
    if (RegValue == 0)
    {
        // In Multi-Host mode, but not management port
        pMHProp->SwitchMode = PLX_SWITCH_MODE_MULTI_HOST;
        DebugPrintf(("Device is in multi-host mode, but not management port\n"));
        return ApiSuccess;
    }

    // Report this is management port regardless of mode
    pMHProp->bIsMgmtPort = TRUE;

    // Store management port info
    pMHProp->MgmtPortNumActive    = (U8)((RegValue >> 0) & 0x1F);
    pMHProp->MgmtPortNumRedundant = (U8)((RegValue >> 8) & 0x1F);

    if (RegValue & (1 << 5))
        pMHProp->bMgmtPortActiveEn = TRUE;

    if (RegValue & (1 << 13))
        pMHProp->bMgmtPortRedundantEn = TRUE;

    // Get active VS mask
    RegVSEnable =
        PLX_8000_REG_READ(
            pDevice,
            0x358
            );

    // Provide active VS's
    pMHProp->VS_EnabledMask = (U16)RegVSEnable;

    TotalVS = 0;

    // Count number of active virtual switches
    for (i=0; i<8; i++)
    {
        // Check if VS is active
        if (RegVSEnable & (1 << i))
        {
            // Increment count
            TotalVS++;

            // Get VS upstream port ([4:0])
            RegValue =
                PLX_8000_REG_READ(
                    pDevice,
                    0x360 + (i * sizeof(U32))
                    );

            pMHProp->VS_UpstreamPortNum[i] = (U8)((RegValue >> 0) & 0x1F);

            // Get VS downstream port vector ([23:0])
            RegValue =
                PLX_8000_REG_READ(
                    pDevice,
                    0x380 + (i * sizeof(U32))
                    );

            pMHProp->VS_DownstreamPorts[i] = RegValue & 0x00FFFFFF;

            // Remove upstream port from downstream vectors
            pMHProp->VS_DownstreamPorts[i] &= ~(1 << pMHProp->VS_UpstreamPortNum[i]);
        }
    }

    // If more than one VS is active, then multi-host mode
    if (TotalVS == 1)
    {
        DebugPrintf(("Device is in standard mode\n"));
    }
    else
    {
        pMHProp->SwitchMode = PLX_SWITCH_MODE_MULTI_HOST;

        DebugPrintf((
            "Mode        : Mult-Host\n"
            "Enabled VS  : %04X\n"
            "Active Mgmt : %d (%s)\n"
            "Backup Mgmt : %d (%s)\n"
            "VS UP-DS pts: 0:%02d-%08X 1:%02d-%08X 2:%02d-%08X 3:%02d-%08X\n"
            "              4:%02d-%08X 5:%02d-%08X 6:%02d-%08X 7:%02d-%08X\n",
            pMHProp->VS_EnabledMask,
            pMHProp->MgmtPortNumActive,
            (pMHProp->bMgmtPortActiveEn) ? "enabled" : "disabled",
            pMHProp->MgmtPortNumRedundant,
            (pMHProp->bMgmtPortRedundantEn) ? "enabled" : "disabled",
            pMHProp->VS_UpstreamPortNum[0], (int)pMHProp->VS_DownstreamPorts[0],
            pMHProp->VS_UpstreamPortNum[1], (int)pMHProp->VS_DownstreamPorts[1],
            pMHProp->VS_UpstreamPortNum[2], (int)pMHProp->VS_DownstreamPorts[2],
            pMHProp->VS_UpstreamPortNum[3], (int)pMHProp->VS_DownstreamPorts[3],
            pMHProp->VS_UpstreamPortNum[4], (int)pMHProp->VS_DownstreamPorts[4],
            pMHProp->VS_UpstreamPortNum[5], (int)pMHProp->VS_DownstreamPorts[5],
            pMHProp->VS_UpstreamPortNum[6], (int)pMHProp->VS_DownstreamPorts[6],
            pMHProp->VS_UpstreamPortNum[7], (int)pMHProp->VS_DownstreamPorts[7]
            ));
    }

    return ApiSuccess;
}




/*******************************************************************************
 *
 * Function   :  PlxI2c_MH_MigrateDsPorts
 *
 * Description:  Migrates one or more downstream ports from one VS to another
 *
 ******************************************************************************/
PLX_STATUS
PlxI2c_MH_MigrateDsPorts(
    PLX_DEVICE_OBJECT *pDevice,
    U16                VS_Source,
    U16                VS_Dest,
    U32                DsPortMask,
    BOOLEAN            bResetSrc
    )
{
    U8                  i;
    U32                 RegValue;
    PLX_STATUS          status;
    PLX_MULTI_HOST_PROP MHProp;


    // Get current MH properties
    status =
        PlxI2c_MH_GetProperties(
            pDevice,
            &MHProp
            );

    if (status != ApiSuccess)
        return status;

    // Operation only available from Multi-Host management port
    if ((MHProp.SwitchMode != PLX_SWITCH_MODE_MULTI_HOST) || (MHProp.bIsMgmtPort == FALSE))
        return ApiUnsupportedFunction;

    DebugPrintf((
        "Migrating DS ports (%08X) from VS%d ==> VS%d %s\n",
        (int)DsPortMask, VS_Source, VS_Dest,
        (bResetSrc) ? "& reset source port" : ""
        ));

    if ((VS_Source >= 8) || (VS_Dest >= 8))
    {
        DebugPrintf(("ERROR - Source or Dest VS are not valid\n"));
        return ApiInvalidIndex;
    }

    // Verify source VS is enabled
    if ((MHProp.VS_EnabledMask & (1 << VS_Source)) == 0)
    {
        DebugPrintf(("ERROR - Source VS (%d) not enabled\n", VS_Source));
        return ApiDeviceDisabled;
    }

    // Verify DS ports to move currently owned by source port
    if ((MHProp.VS_DownstreamPorts[VS_Source] & DsPortMask) != DsPortMask)
    {
        DebugPrintf(("ERROR - One or more DS ports not owned by source VS\n"));
        return ApiInvalidData;
    }

    // Migrate DS ports
    for (i=0; i<24; i++)
    {
        // Migrate port from source to destination if requested
        if (DsPortMask & (1 << i))
        {
            // Remove port from source
            MHProp.VS_DownstreamPorts[VS_Source] &= ~(1 << i);

            // Add port to destination
            MHProp.VS_DownstreamPorts[VS_Dest] |= (1 << i);
        }
    }

    // Update source & destination ports
    PLX_8000_REG_WRITE(
        pDevice,
        0x380 + (VS_Source * sizeof(U32)),
        MHProp.VS_DownstreamPorts[VS_Source]
        );

    PLX_8000_REG_WRITE(
        pDevice,
        0x380 + (VS_Dest * sizeof(U32)),
        MHProp.VS_DownstreamPorts[VS_Dest]
        );

    // Make sure destination VS is enabled
    if ((MHProp.VS_EnabledMask & (1 << VS_Dest)) == 0)
    {
        DebugPrintf(("Enabling destination VS%d\n", VS_Dest));

        PLX_8000_REG_WRITE(
            pDevice,
            0x358,
            MHProp.VS_EnabledMask | (1 << VS_Dest)
            );
    }

    // Reset source port if requested
    if (bResetSrc)
    {
        RegValue =
            PLX_8000_REG_READ(
                pDevice,
                0x3A0
                );

        // Put VS into reset
        PLX_8000_REG_WRITE(
            pDevice,
            0x3A0,
            RegValue | (1 << VS_Source)
            );

        // Keep in reset for a short time
// DBG - Plx_sleep not yet defined in API
//        Plx_sleep( 10 );

        // Take VS out of reset
        PLX_8000_REG_WRITE(
            pDevice,
            0x3A0,
            RegValue & ~((U32)1 << VS_Source)
            );
    }

    return ApiSuccess;
}




/***********************************************************
 *
 *               PRIVATE I2C SUPPORT FUNCTIONS
 *
 **********************************************************/


/******************************************************************************
 *
 * Function   :  PlxI2c_GenerateCommand
 *
 * Description:  Prepares the I2C command for access
 *
 *****************************************************************************/
U32
PlxI2c_GenerateCommand(
    PLX_DEVICE_OBJECT *pDevice,
    U8                 I2cOperation,
    U32                offset,
    BOOLEAN            bAdjustForPort
    )
{
    U8  Bit_Mode;
    U8  Bit_StnSel;
    U8  Bit_PortSel;
    U8  PortType;
    U32 StnSel;
    U32 PortSel;
    U32 Mode;
    U32 Command;
    U32 Offset_NTVirt[2];       // NT Virtual port offsets
    U32 Offset_NTLink[2];       // NT Link port offsets


    // Reset NT base offsets
    memset( Offset_NTVirt, 0xFF, 2 * sizeof(U32) );
    memset( Offset_NTLink, 0xFF, 2 * sizeof(U32) );

    // Added to avoid compiler warning
    Bit_Mode = 0;

    // Set bit positions
    Bit_StnSel  = 0;
    Bit_PortSel = 15;

    switch (pDevice->Key.PlxFamily)
    {
        case PLX_FAMILY_VEGA_LITE:
            Bit_Mode         = 18;
            Offset_NTVirt[0] = 0x10000;
            Offset_NTLink[0] = 0x11000;
            break;

        case PLX_FAMILY_ALTAIR:
        case PLX_FAMILY_ALTAIR_XL:
            // No NT port
            break;

        case PLX_FAMILY_DENEB:
        case PLX_FAMILY_SIRIUS:
            Bit_Mode         = 19;
            Bit_StnSel       = 17;
            Offset_NTVirt[0] = 0x10000;
            Offset_NTLink[0] = 0x11000;
            break;

        case PLX_FAMILY_CYGNUS:
            Bit_Mode         = 20;
            Bit_StnSel       = 17;
            Offset_NTVirt[0] = 0x3E000;
            Offset_NTLink[0] = 0x3F000;
            break;

        case PLX_FAMILY_SCOUT:
        case PLX_FAMILY_DRACO_1:
        case PLX_FAMILY_DRACO_2:
        default:
            Bit_Mode         = 20;
            Bit_StnSel       = 18;
            Offset_NTVirt[0] = 0x3E000;
            Offset_NTLink[0] = 0x3F000;
            Offset_NTVirt[1] = 0x3C000;
            Offset_NTLink[1] = 0x3D000;
            break;
    }

    // Default to transparent port
    PortType = 0;

    // Determine if special port is being accessed
    if ((pDevice->Key.PlxPort == PLX_FLAG_PORT_NT_VIRTUAL_0) ||
        (pDevice->Key.PlxPort == PLX_FLAG_PORT_NT_VIRTUAL_1) ||
        (pDevice->Key.PlxPort == PLX_FLAG_PORT_NT_LINK_0)    ||
        (pDevice->Key.PlxPort == PLX_FLAG_PORT_NT_LINK_1)    ||
        (pDevice->Key.PlxPort == PLX_FLAG_PORT_NT_DS_P2P)    ||
        (pDevice->Key.PlxPort == PLX_FLAG_PORT_DMA_0)        ||
        (pDevice->Key.PlxPort == PLX_FLAG_PORT_DMA_1)        ||
        (pDevice->Key.PlxPort == PLX_FLAG_PORT_DMA_2)        ||
        (pDevice->Key.PlxPort == PLX_FLAG_PORT_DMA_3))
    {
        PortType = pDevice->Key.PlxPort;
    }

    Mode    = 0;
    StnSel  = 0;
    PortSel = 0;

    // Adjust offset if needed & check for NT port
    if (PortType == 0)
    {
        // Adjust offset to port if requested
        if (bAdjustForPort)
        {
            PortSel = pDevice->Key.PlxPort;
            offset += (PortSel * 0x1000);
        }

        // Compare with NT offsets
        if ((offset & 0xFF000) == Offset_NTVirt[0])
            PortType = PLX_FLAG_PORT_NT_VIRTUAL_0;
        else if ((offset & 0xFF000) == Offset_NTVirt[1])
            PortType = PLX_FLAG_PORT_NT_VIRTUAL_1;
        else if ((offset & 0xFF000) == Offset_NTLink[0])
            PortType = PLX_FLAG_PORT_NT_LINK_0;
        else if ((offset & 0xFF000) == Offset_NTLink[1])
            PortType = PLX_FLAG_PORT_NT_LINK_1;
        else
            PortSel = (offset >> 12);

        // Handle large offsets that target NT Virtual port
        if (PortType == PLX_FLAG_PORT_NT_VIRTUAL_0)
        {
            if ((pDevice->Key.PlxFamily == PLX_FAMILY_VEGA_LITE) ||
                (pDevice->Key.PlxFamily == PLX_FAMILY_DENEB) ||
                (pDevice->Key.PlxFamily == PLX_FAMILY_SIRIUS) ||
                (pDevice->Key.PlxFamily == PLX_FAMILY_CYGNUS))
            {
                // For Cygnus, only applies to legacy P2P mode
                if ((pDevice->Key.PlxFamily != PLX_FAMILY_CYGNUS) ||
                    (pDevice->Key.ApiInternal[1] == PLX_I2C_NT_MODE_LEGACY))
                {
                    // Revert back to standard port
                    PortType = 0;

                    // Select NT port
                    PortSel = pDevice->Key.NTPortNum;
                }
            }
        }
    }

    // Determine values
    switch (pDevice->Key.PlxFamily)
    {
        case PLX_FAMILY_VEGA_LITE:
            if (PortType == PLX_FLAG_PORT_NT_LINK_0)
            {
                Mode = 1;

                // Accessing offsets F8h or FCh of NT-Link port causes I2C failure, so bypass them
                if (((offset & 0xFFF) == 0x0F8) || ((offset & 0xFFF) == 0x0FC))
                {
                    return PLX_I2C_CMD_SKIP;
                }
            }
            break;

        case PLX_FAMILY_ALTAIR:
        case PLX_FAMILY_ALTAIR_XL:
            // No NT port
            break;

        case PLX_FAMILY_DENEB:
            if (PortType == PLX_FLAG_PORT_NT_LINK_0)
                Mode = 1;
            break;

        case PLX_FAMILY_SIRIUS:
            // Default to special mode
            Mode = 1;

            if (PortType == PLX_FLAG_PORT_NT_LINK_0)
                PortSel = 0x10;
            else if (PortType == PLX_FLAG_PORT_NT_DS_P2P)
                PortSel = 0x11;
            else if (PortType == PLX_FLAG_PORT_DMA_0)
                PortSel = 0x12;
            else if (PortType == PLX_FLAG_PORT_DMA_RAM)
                PortSel = 0x13;
            else
                Mode = 0;   // Revert to transparent mode
            break;

        case PLX_FAMILY_CYGNUS:
            if (PortType == PLX_FLAG_PORT_NT_VIRTUAL_0)
            {
                StnSel  = 6;    // 110b
                PortSel = 3;    // 11b
            }
            else if (PortType == PLX_FLAG_PORT_NT_LINK_0)
            {
                Mode    = 1;
                PortSel = 0;    // 00b
            }
            else
            {
                StnSel  = PortSel / 4;  // 6 stations (0-5), 4 ports each
                PortSel = PortSel % 4;  // Port in station (0-3)
            }
            break;

        case PLX_FAMILY_SCOUT:
        case PLX_FAMILY_DRACO_1:
        case PLX_FAMILY_DRACO_2:
            // For Draco 1, some register cause problems if accessed
            if (pDevice->Key.PlxFamily == PLX_FAMILY_DRACO_1)
            {
                if ((offset == 0x856C)  || (offset == 0x8570) ||
                    (offset == 0x1056C) || (offset == 0x10570))
                {
                    return PLX_I2C_CMD_SKIP;
                }
            }

            if (PortType == PLX_FLAG_PORT_NT_VIRTUAL_0)
            {
                Mode    = 2;    // 10b
                PortSel = 0;    // NT 0
            }
            else if (PortType == PLX_FLAG_PORT_NT_VIRTUAL_1)
            {
                Mode    = 2;    // 10b
                PortSel = 1;    // NT 1
            }
            else if (PortType == PLX_FLAG_PORT_NT_LINK_0)
            {
                Mode    = 1;    // 01b
                PortSel = 0;    // NT 0
            }
            else if (PortType == PLX_FLAG_PORT_NT_LINK_1)
            {
                Mode    = 1;    // 01b
                PortSel = 1;    // NT 1
            }
            else if (PortType == PLX_FLAG_PORT_DMA_0)
            {
                Mode    = 3;    // 11b
                PortSel = 0;    // DMA 0
            }
            else if (PortType == PLX_FLAG_PORT_DMA_1)
            {
                Mode    = 3;    // 11b
                PortSel = 1;    // DMA 1
            }
            else if (PortType == PLX_FLAG_PORT_DMA_2)
            {
                Mode    = 3;    // 11b
                PortSel = 2;    // DMA 2
            }
            else if (PortType == PLX_FLAG_PORT_DMA_3)
            {
                Mode    = 3;    // 11b
                PortSel = 3;    // DMA 3
            }
            else if (PortType == PLX_FLAG_PORT_DMA_RAM)
            {
                Mode    = 3;    // 11b
                PortSel = 4;    // DMA RAM
            }
            else
            {
                StnSel  = PortSel / 8;  // 3 stations (0-2), 6 ports ea, but numbered as 8
                PortSel = PortSel % 8;  // Port in station (0-5)
            }
            break;
    }

    // Get offset within 4K port
    offset = offset & 0xFFF;

    // Convert offset to DWORD index
    offset = offset >> 2;

    // Build I2C command
    Command =
        (0            <<          27) |  // Reserved [31:27]
        (I2cOperation <<          24) |  // Register operation [26:24]
        (Mode         <<    Bit_Mode) |  // Mode to select transp, NTV/NTL, DMA, etc
        (StnSel       <<  Bit_StnSel) |  // Station number if needed
        (PortSel      << Bit_PortSel) |  // Port # or DMA channel or NT 0/1
        (0            <<          14) |  // Reserved [14]
        (0xF          <<          10) |  // Byte enables [13:10]
        (offset       <<           0);   // Register DWORD address [9:0]

    // 32-bit command must be in BE format since byte 0 is MSB
    Command = PLX_BE_DATA_32( Command );

    return Command;
}




/******************************************************************************
 *
 * Function   :  PlxI2c_Driver_Connect
 *
 * Description:  Attempts to connect to the I2C driver
 *
 * Returns    :  TRUE   - Driver was found and connected to
 *               FALSE  - Driver not found
 *
 *****************************************************************************/
BOOLEAN
PlxI2c_Driver_Connect(
    PLX_DEVICE_OBJECT *pDevice,
    PLX_MODE_PROP     *pModeProp
    )
{
    // If mode properties supplied, copy into device object
    if (pModeProp != NULL)
    {
        pDevice->Key.ApiIndex       = (U8)pModeProp->I2c.I2cPort;
        pDevice->Key.ApiInternal[0] = pModeProp->I2c.ClockRate;
    }

    // Clear handle in case of failure
    pDevice->hDevice = 0;

    // Verify port doesn't exceed limit
    if (pDevice->Key.ApiIndex >= PLX_I2C_MAX_DEVICES)
        return FALSE;

    // Initialize global structure if haven't yet
    if (Gbl_bInitialized == FALSE)
    {
        Gbl_bInitialized = TRUE;
        memset( Gbl_PlxI2cProp, 0, sizeof(Gbl_PlxI2cProp) );
    }

    // Check if device is already opened by the PLX API
    if (Gbl_PlxI2cProp[pDevice->Key.ApiIndex].hDevice == 0)
    {
        // Open I2C device
        pDevice->hDevice = (PLX_DRIVER_HANDLE)(PLX_UINT_PTR)aa_open( pDevice->Key.ApiIndex );
        if (PLX_PTR_TO_INT( pDevice->hDevice ) <= 0)
            return FALSE;

        // Disable the Aardvark adapter's power pins.
        aa_target_power( (Aardvark)PLX_PTR_TO_INT( pDevice->hDevice ), AA_TARGET_POWER_NONE );

        // Make sure slave mode is disabled
        aa_i2c_slave_disable( (Aardvark)PLX_PTR_TO_INT( pDevice->hDevice ) );

        // Set I2C bus speed
        aa_i2c_bitrate( (Aardvark)PLX_PTR_TO_INT( pDevice->hDevice ), pDevice->Key.ApiInternal[0]);

        // Store the handle for re-use if necessary
        Gbl_PlxI2cProp[pDevice->Key.ApiIndex].hDevice = pDevice->hDevice;

        // Reset upstream port bus number
        Gbl_PlxI2cProp[pDevice->Key.ApiIndex].UpstreamBus = PCI_FIELD_IGNORE;

        // Reset NT port number
        Gbl_PlxI2cProp[pDevice->Key.ApiIndex].NTPortNum = PCI_FIELD_IGNORE;

        // Initialize the register access lock
        InitializeCriticalSection(
            &(Gbl_PlxI2cProp[pDevice->Key.ApiIndex].Lock_I2cRegAccess)
            );
    }
    else
    {
        // Re-use existing handle
        pDevice->hDevice = Gbl_PlxI2cProp[pDevice->Key.ApiIndex].hDevice;
    }

    // Increment open count
    InterlockedIncrement( &Gbl_PlxI2cProp[pDevice->Key.ApiIndex].OpenCount );

    return TRUE;
}




/******************************************************************************
 *
 * Function   :  PlxI2c_Dispatch_IoControl
 *
 * Description:  Processes the IOCTL messages
 *
 ******************************************************************************/
S32
PlxI2c_Dispatch_IoControl(
    PLX_DEVICE_OBJECT *pDevice,
    U32                IoControlCode,
    PLX_PARAMS        *pIoBuffer,
    U32                Size
    )
{
    DebugPrintf_Cont(("\n"));
    DebugPrintf(("Received PLX I2C message ===> "));

    // Handle the PLX specific message
    switch (IoControlCode)
    {
        /******************************************
         * Driver Query Functions
         *****************************************/
        case PLX_IOCTL_DRIVER_VERSION:
            DebugPrintf_Cont(("PLX_IOCTL_DRIVER_VERSION\n"));

            pIoBuffer->value[0] =
                (PLX_SDK_VERSION_MAJOR << 16) |
                (PLX_SDK_VERSION_MINOR <<  8) |
                (0                     <<  0);
            break;

        case PLX_IOCTL_CHIP_TYPE_GET:
            DebugPrintf_Cont(("PLX_IOCTL_CHIP_TYPE_GET\n"));

            pIoBuffer->ReturnCode =
                PlxI2c_ChipTypeGet(
                    pDevice,
                    (U16*)&(pIoBuffer->value[0]),
                    (U8*)&(pIoBuffer->value[1])
                    );
            break;

        case PLX_IOCTL_CHIP_TYPE_SET:
            DebugPrintf_Cont(("PLX_IOCTL_CHIP_TYPE_SET\n"));

            pIoBuffer->ReturnCode =
                PlxI2c_ChipTypeSet(
                    pDevice,
                    (U16)pIoBuffer->value[0],
                    (U8)pIoBuffer->value[1]
                    );
            break;

        case PLX_IOCTL_GET_PORT_PROPERTIES:
            DebugPrintf_Cont(("PLX_IOCTL_GET_PORT_PROPERTIES\n"));

            pIoBuffer->ReturnCode =
                PlxI2c_GetPortProperties(
                    pDevice,
                    &(pIoBuffer->u.PortProp)
                    );
            break;


        /******************************************
         * PCI Register Access Functions
         *****************************************/
        case PLX_IOCTL_PCI_REGISTER_READ:
            DebugPrintf_Cont(("PLX_IOCTL_PCI_REGISTER_READ\n"));

            pIoBuffer->value[1] =
                PlxI2c_PlxRegisterRead(
                    pDevice,
                    (U32)pIoBuffer->value[0],
                    &(pIoBuffer->ReturnCode),
                    TRUE        // Adjust offset based on port
                    );

            DebugPrintf((
                "PCI Reg %03X = %08lX\n",
                (U16)pIoBuffer->value[0],
                (U32)pIoBuffer->value[1]
                ));
            break;

        case PLX_IOCTL_PCI_REGISTER_WRITE:
            DebugPrintf_Cont(("PLX_IOCTL_PCI_REGISTER_WRITE\n"));

            pIoBuffer->ReturnCode =
                PlxI2c_PlxRegisterWrite(
                    pDevice,
                    (U32)pIoBuffer->value[0],
                    (U32)pIoBuffer->value[1],
                    TRUE        // Adjust offset based on port
                    );

            DebugPrintf((
                "Wrote %08lX to PCI Reg %03X\n",
                (U32)pIoBuffer->value[1],
                (U16)pIoBuffer->value[0]
                ));
            break;


        /******************************************
         * PLX-specific Register Access Functions
         *****************************************/
        case PLX_IOCTL_REGISTER_READ:
            DebugPrintf_Cont(("PLX_IOCTL_REGISTER_READ\n"));

            pIoBuffer->value[1] =
                PlxI2c_PlxRegisterRead(
                    pDevice,
                    (U32)pIoBuffer->value[0],
                    &(pIoBuffer->ReturnCode),
                    TRUE        // Adjust offset based on port
                    );

            DebugPrintf((
                "PLX Reg %03lX = %08lX\n",
                (U32)pIoBuffer->value[0],
                (U32)pIoBuffer->value[1]
                ));
            break;

        case PLX_IOCTL_REGISTER_WRITE:
            DebugPrintf_Cont(("PLX_IOCTL_REGISTER_WRITE\n"));

            pIoBuffer->ReturnCode =
                PlxI2c_PlxRegisterWrite(
                    pDevice,
                    (U32)pIoBuffer->value[0],
                    (U32)pIoBuffer->value[1],
                    TRUE        // Adjust offset based on port
                    );

            DebugPrintf((
                "Wrote %08lX to PLX Reg %03lX\n",
                (U32)pIoBuffer->value[1],
                (U32)pIoBuffer->value[0]
                ));
            break;

        case PLX_IOCTL_MAPPED_REGISTER_READ:
            DebugPrintf_Cont(("PLX_IOCTL_MAPPED_REGISTER_READ\n"));

            pIoBuffer->value[1] =
                PlxI2c_PlxRegisterRead(
                    pDevice,
                    (U32)pIoBuffer->value[0],
                    &(pIoBuffer->ReturnCode),
                    FALSE       // Don't adjust offset based on port
                    );

            DebugPrintf((
                "PLX Mapped Reg %03lX = %08lX\n",
                (U32)pIoBuffer->value[0],
                (U32)pIoBuffer->value[1]
                ));
            break;

        case PLX_IOCTL_MAPPED_REGISTER_WRITE:
            DebugPrintf_Cont(("PLX_IOCTL_MAPPED_REGISTER_WRITE\n"));

            pIoBuffer->ReturnCode =
                PlxI2c_PlxRegisterWrite(
                    pDevice,
                    (U32)pIoBuffer->value[0],
                    (U32)pIoBuffer->value[1],
                    FALSE       // Don't adjust offset based on port
                    );

            DebugPrintf((
                "Wrote %08lX to PLX Mapped Reg %03lX\n",
                (U32)pIoBuffer->value[1],
                (U32)pIoBuffer->value[0]
                ));
            break;


        /******************************************
         * PCI BAR Functions
         *****************************************/
        case PLX_IOCTL_PCI_BAR_PROPERTIES:
            DebugPrintf_Cont(("PLX_IOCTL_PCI_BAR_PROPERTIES\n"));

            pIoBuffer->ReturnCode =
                PlxI2c_PciBarProperties(
                    pDevice,
                    (U8)(pIoBuffer->value[0]),
                    &(pIoBuffer->u.BarProp)
                    );
            break;


        /******************************************
         * Serial EEPROM Access Functions
         *****************************************/
        case PLX_IOCTL_EEPROM_PRESENT:
            DebugPrintf_Cont(("PLX_IOCTL_EEPROM_PRESENT\n"));

            pIoBuffer->ReturnCode =
                PlxI2c_EepromPresent(
                    pDevice,
                    (PLX_EEPROM_STATUS*)&(pIoBuffer->value[0])
                    );
            break;

        case PLX_IOCTL_EEPROM_PROBE:
            DebugPrintf_Cont(("PLX_IOCTL_EEPROM_PROBE\n"));

            pIoBuffer->ReturnCode =
                PlxI2c_EepromProbe(
                    pDevice,
                    (BOOLEAN*)&(pIoBuffer->value[0])
                    );
            break;

        case PLX_IOCTL_EEPROM_SET_ADDRESS_WIDTH:
            DebugPrintf_Cont(("PLX_IOCTL_EEPROM_SET_ADDRESS_WIDTH\n"));

            pIoBuffer->ReturnCode =
                PlxI2c_EepromSetAddressWidth(
                    pDevice,
                    (U8)pIoBuffer->value[0]
                    );
            break;

        case PLX_IOCTL_EEPROM_CRC_GET:
            DebugPrintf_Cont(("PLX_IOCTL_EEPROM_CRC_GET\n"));

            pIoBuffer->ReturnCode =
                PlxI2c_EepromCrcGet(
                    pDevice,
                    (U32*)&(pIoBuffer->value[0]),
                    (U8*)&(pIoBuffer->value[1])
                    );
            break;

        case PLX_IOCTL_EEPROM_CRC_UPDATE:
            DebugPrintf_Cont(("PLX_IOCTL_EEPROM_CRC_UPDATE\n"));

            pIoBuffer->ReturnCode =
                PlxI2c_EepromCrcUpdate(
                    pDevice,
                    (U32*)&(pIoBuffer->value[0]),
                    (BOOLEAN)pIoBuffer->value[1]
                    );
            break;

        case PLX_IOCTL_EEPROM_READ_BY_OFFSET:
            DebugPrintf_Cont(("PLX_IOCTL_EEPROM_READ_BY_OFFSET\n"));

            pIoBuffer->ReturnCode =
                PlxI2c_EepromReadByOffset(
                    pDevice,
                    (U16)pIoBuffer->value[0],
                    (U32*)&(pIoBuffer->value[1])
                    );

            DebugPrintf((
                "EEPROM Offset %02X = %08lX\n",
                (U16)pIoBuffer->value[0],
                (U32)pIoBuffer->value[1]
                ));
            break;

        case PLX_IOCTL_EEPROM_WRITE_BY_OFFSET:
            DebugPrintf_Cont(("PLX_IOCTL_EEPROM_WRITE_BY_OFFSET\n"));

            pIoBuffer->ReturnCode =
                PlxI2c_EepromWriteByOffset(
                    pDevice,
                    (U16)pIoBuffer->value[0],
                    (U32)pIoBuffer->value[1]
                    );

            DebugPrintf((
                "Wrote %08lX to EEPROM Offset %02X\n",
                (U32)pIoBuffer->value[1],
                (U16)pIoBuffer->value[0]
                ));
            break;

        case PLX_IOCTL_EEPROM_READ_BY_OFFSET_16:
            DebugPrintf_Cont(("PLX_IOCTL_EEPROM_READ_BY_OFFSET_16\n"));

            pIoBuffer->ReturnCode =
                PlxI2c_EepromReadByOffset_16(
                    pDevice,
                    (U16)pIoBuffer->value[0],
                    (U16*)&(pIoBuffer->value[1])
                    );

            DebugPrintf((
                "EEPROM Offset %02X = %04X\n",
                (U16)pIoBuffer->value[0],
                (U16)pIoBuffer->value[1]
                ));
            break;

        case PLX_IOCTL_EEPROM_WRITE_BY_OFFSET_16:
            DebugPrintf_Cont(("PLX_IOCTL_EEPROM_WRITE_BY_OFFSET_16\n"));

            pIoBuffer->ReturnCode =
                PlxI2c_EepromWriteByOffset_16(
                    pDevice,
                    (U16)pIoBuffer->value[0],
                    (U16)pIoBuffer->value[1]
                    );

            DebugPrintf((
                "Wrote %04X to EEPROM Offset %02X\n",
                (U16)pIoBuffer->value[1],
                (U16)pIoBuffer->value[0]
                ));
            break;


        /******************************************
         * PLX Performance Object Functions
         *****************************************/
        case PLX_IOCTL_PERFORMANCE_INIT_PROPERTIES:
            DebugPrintf_Cont(("PLX_IOCTL_PERFORMANCE_INIT_PROPERTIES\n"));

            pIoBuffer->ReturnCode =
                PlxI2c_PerformanceInitializeProperties(
                    pDevice,
                    PLX_INT_TO_PTR(pIoBuffer->value[0])
                    );
            break;

        case PLX_IOCTL_PERFORMANCE_MONITOR_CTRL:
            DebugPrintf_Cont(("PLX_IOCTL_PERFORMANCE_MONITOR_CTRL\n"));

            pIoBuffer->ReturnCode =
                PlxI2c_PerformanceMonitorControl(
                    pDevice,
                    (PLX_PERF_CMD)pIoBuffer->value[0]
                    );
            break;

        case PLX_IOCTL_PERFORMANCE_RESET_COUNTERS:
            DebugPrintf_Cont(("PLX_IOCTL_PERFORMANCE_RESET_COUNTERS\n"));

            pIoBuffer->ReturnCode =
                PlxI2c_PerformanceResetCounters(
                    pDevice
                    );
            break;

        case PLX_IOCTL_PERFORMANCE_GET_COUNTERS:
            DebugPrintf_Cont(("PLX_IOCTL_PERFORMANCE_GET_COUNTERS\n"));

            pIoBuffer->ReturnCode =
                PlxI2c_PerformanceGetCounters(
                    pDevice,
                    PLX_INT_TO_PTR(pIoBuffer->value[0]),
                    (U8)pIoBuffer->value[1]
                    );
            break;


        /******************************************
         * PLX Multi-Host Functions
         *****************************************/
        case PLX_IOCTL_MH_GET_PROPERTIES:
            DebugPrintf_Cont(("PLX_IOCTL_MH_GET_PROPERTIES\n"));

            pIoBuffer->ReturnCode =
                PlxI2c_MH_GetProperties(
                    pDevice,
                    &pIoBuffer->u.MH_Prop
                    );
            break;

        case PLX_IOCTL_MH_MIGRATE_DS_PORTS:
            DebugPrintf_Cont(("PLX_IOCTL_MH_MIGRATE_DS_PORTS\n"));

            pIoBuffer->ReturnCode =
                PlxI2c_MH_MigrateDsPorts(
                    pDevice,
                    (U16)(pIoBuffer->value[0] >> 16),
                    (U16)(pIoBuffer->value[0] & 0xFFFF),
                    (U32)pIoBuffer->value[1],
                    (BOOLEAN)pIoBuffer->value[2]
                    );
            break;


        /******************************************
         * Unsupported Messages
         *****************************************/
        default:
            DebugPrintf_Cont((
                "Unsupported PLX_IOCTL_Xxx (%d)\n",
                IoControlCode
                ));

            pIoBuffer->ReturnCode = ApiUnsupportedFunction;
            break;
    }

    DebugPrintf(("...Completed message\n"));

    return 0;
}




/******************************************************************************
 *
 * Function   :  PlxI2c_ProbeSwitch
 *
 * Description:  Probes a switch over Aardark I2C to find active ports
 *
 ******************************************************************************/
PLX_STATUS
PlxI2c_ProbeSwitch(
    PLX_DEVICE_OBJECT *pDevice,
    PLX_DEVICE_KEY    *pKey,
    U8                 DeviceNumber,
    U8                *pNumMatched
    )
{
    U8            Port;
    U8            Port_Upstream;
    U8            DeviceCount;
    U8            PciHeaderType;
    U16           PlxChip;
    U32           offset;
    U32           Offset_Upstream;
    U32           Offset_DebugCtrl;
    U32           DevVenID;
    U32           RegValue;
    U32           RegDebugCtrl;
    U32           RegPciHeader;
    U64           PossiblePorts;
    BOOLEAN       bAutoGenBus;
    BOOLEAN       bCompareDevice;
    BOOLEAN       bMatchId;
    BOOLEAN       bMatchLoc;
    BOOLEAN       bNtEnabled;
    PLX_STATUS    status;
    PLX_PORT_PROP PortProp;


    DeviceCount   = 0;
    RegDebugCtrl  = (U32)-1;
    Port_Upstream = (U8)-1;

    // Start with port 0
    Port = 0;

    // Start with PLX chip unknown
    PlxChip = 0;

    // Start with empty Device ID
    DevVenID = 0;

    // Default to NT enabled
    bNtEnabled = TRUE;

    // Initially allow access only to port 0
    PossiblePorts = (1 << 0);

    // Probe all possible ports in the switch
    while (Port < (sizeof(PossiblePorts) * 8))
    {
        // Default to not compare device
        bCompareDevice = FALSE;

        // Default to non-NT Link port
        pDevice->Key.NTPortType = PLX_NT_PORT_NONE;

        // Store the port
        pDevice->Key.PlxPort = Port;

        // Set default offset
        offset = 0x0;

        // Verify port and set offset
        if ((Port == PLX_FLAG_PORT_NT_LINK_0) || (Port == PLX_FLAG_PORT_NT_LINK_1))
        {
            // NT link port
            pDevice->Key.NTPortType = PLX_NT_PORT_LINK;
        }
        else if ((Port == PLX_FLAG_PORT_NT_VIRTUAL_0) || (Port == PLX_FLAG_PORT_NT_VIRTUAL_1))
        {
            // NT virtual port
            pDevice->Key.NTPortType = PLX_NT_PORT_VIRTUAL;
        }
        else if (Port == PLX_FLAG_PORT_NT_DS_P2P)
        {
            // NT Virtual or Downstream Parent port
        }
        else if ((Port == PLX_FLAG_PORT_DMA_0) || (Port == PLX_FLAG_PORT_DMA_1) ||
                 (Port == PLX_FLAG_PORT_DMA_2) || (Port == PLX_FLAG_PORT_DMA_3))
        {
            // DMA function
        }
        else
        {
            // Determine offset
            offset = (U32)Port * 0x1000;
        }

        // Skip probe if port is known not to exist
        if ((PossiblePorts & ((U64)1 << Port)) == 0)
        {
            status = ApiFailed;
        }
        else
        {
            // Get Device/Vendor ID
            DevVenID =
                PlxI2c_PlxRegisterRead(
                    pDevice,
                    offset,
                    &status,
                    FALSE           // Do not adjust for port
                    );
        }

        if ((status == ApiSuccess) && ((DevVenID & 0xFFFF) == PLX_VENDOR_ID))
        {
            DebugPrintf(("I2C - Port %d detected (ID=%08X)\n", Port, DevVenID));

            // Update possible ports
            if (PlxChip == 0)
            {
                // Set PLX chip type & revision
                if (pDevice->Key.PlxChip == 0)
                {
                    // Set device ID in case needed by chip detection algorithm
                    pDevice->Key.DeviceId = (U16)(DevVenID >> 16);

                    // Probe to determine chip type
                    PlxChipTypeDetect( pDevice );
                }

                // Store connected chip
                if (pDevice->Key.PlxChip != 0)
                    PlxChip = pDevice->Key.PlxChip;

                switch (PlxChip)
                {
                    case 0x8505:
                        PossiblePorts  = 0x0000001F;  // 0-4
                        break;

                    case 0x8509:
                        PossiblePorts  = 0x000000FF;  // 0-7
                        break;

                    case 0x8508:
                    case 0x8512:
                    case 0x8517:
                    case 0x8518:
                        PossiblePorts  = 0x0000001F;  // 0-4,NT
                        PossiblePorts |= ((U64)1 << PLX_FLAG_PORT_NT_LINK_0);
                        break;

                    case 0x8516:
                        PossiblePorts  = 0x0000000F;  // 0-3,NT
                        PossiblePorts |= ((U64)1 << PLX_FLAG_PORT_NT_LINK_0);
                        break;

                    case 0x8524:
                        PossiblePorts  = 0x00000F03;  // 0,1,8-11,NT
                        PossiblePorts |= ((U64)1 << PLX_FLAG_PORT_NT_LINK_0);
                        break;

                    case 0x8525:
                        PossiblePorts  = 0x00000706;  // 1,2,8-10
                        break;

                    case 0x8532:
                        PossiblePorts  = 0x00000F0F;  // 0-3,8-11,NT
                        PossiblePorts |= ((U64)1 << PLX_FLAG_PORT_NT_LINK_0);
                        break;

                    case 0x8533:
                        PossiblePorts  = 0x00000707;  // 0-2,8-10
                        break;

                    case 0x8547:
                        PossiblePorts  = 0x00001101;  // 0,8,12
                        break;

                    case 0x8548:
                        PossiblePorts  = 0x00007707;  // 0-2,8-10,12-14
                        break;

                    case 0x8604:
                        PossiblePorts  = 0x00000033;  // 0,1,4,5,NT,NTB(BA)
                        PossiblePorts |= ((U64)1 << PLX_FLAG_PORT_NT_LINK_0);
                        if (pDevice->Key.PlxRevision != 0xAA)
                            PossiblePorts |= ((U64)1 << PLX_FLAG_PORT_NT_DS_P2P);
                        break;

                    case 0x8606:
                        PossiblePorts  = 0x000002B3;  // 0,1,4,5,7,9,NT,NTB(BA)
                        PossiblePorts |= ((U64)1 << PLX_FLAG_PORT_NT_LINK_0);
                        if (pDevice->Key.PlxRevision != 0xAA)
                            PossiblePorts |= ((U64)1 << PLX_FLAG_PORT_NT_DS_P2P);
                        break;

                    case 0x8608:
                        PossiblePorts  = 0x000003F3;  // 0,1,4-9,NT,NTB(BA)
                        PossiblePorts |= ((U64)1 << PLX_FLAG_PORT_NT_LINK_0);
                        if (pDevice->Key.PlxRevision != 0xAA)
                            PossiblePorts |= ((U64)1 << PLX_FLAG_PORT_NT_DS_P2P);
                        break;

                    case 0x8609:
                        PossiblePorts  = 0x000003F3;  // 0,1,4-9,DMA,NT,NTB
                        PossiblePorts |= ((U64)1 << PLX_FLAG_PORT_NT_LINK_0) |
                                         ((U64)1 << PLX_FLAG_PORT_NT_DS_P2P) |
                                         ((U64)1 << PLX_FLAG_PORT_DMA_0);
                        break;

                    case 0x8612:
                        PossiblePorts  = 0x00000033;  // 0,1,4,5,NT
                        PossiblePorts |= ((U64)1 << PLX_FLAG_PORT_NT_LINK_0);
                        break;

                    case 0x8613:
                        PossiblePorts  = 0x00000007;  // 0-2,NT,NTB
                        PossiblePorts |= ((U64)1 << PLX_FLAG_PORT_NT_LINK_0) |
                                         ((U64)1 << PLX_FLAG_PORT_NT_DS_P2P);
                        break;

                    case 0x8614:
                        PossiblePorts  = 0x000057F7;  // 0-2,4-10,12,14,NT,NTB(BA)
                        PossiblePorts |= ((U64)1 << PLX_FLAG_PORT_NT_LINK_0);
                        if (pDevice->Key.PlxRevision != 0xAA)
                            PossiblePorts |= ((U64)1 << PLX_FLAG_PORT_NT_DS_P2P);
                        break;

                    case 0x8615:
                        PossiblePorts  = 0x000057F7;  // 0-2,4-10,12,14,DMA,NT,NTB
                        PossiblePorts |= ((U64)1 << PLX_FLAG_PORT_NT_LINK_0) |
                                         ((U64)1 << PLX_FLAG_PORT_NT_DS_P2P) |
                                         ((U64)1 << PLX_FLAG_PORT_DMA_0);
                        break;

                    case 0x8616:
                        PossiblePorts  = 0x00000073;  // 0,1,4-6,NT
                        PossiblePorts |= ((U64)1 << PLX_FLAG_PORT_NT_LINK_0);
                        break;

                    case 0x8617:
                        PossiblePorts  = 0x0000000F;  // 0-3,NT,NTB
                        PossiblePorts |= ((U64)1 << PLX_FLAG_PORT_NT_LINK_0) |
                                         ((U64)1 << PLX_FLAG_PORT_NT_DS_P2P);
                        break;

                    case 0x8618:
                        PossiblePorts  = 0x0000FFFF;  // 0-15,NT,NTB(BA)
                        PossiblePorts |= ((U64)1 << PLX_FLAG_PORT_NT_LINK_0);
                        if (pDevice->Key.PlxRevision != 0xAA)
                            PossiblePorts |= ((U64)1 << PLX_FLAG_PORT_NT_DS_P2P);
                        break;

                    case 0x8619:
                        PossiblePorts  = 0x0000FFFF;  // 0-15,DMA,NT,NTB
                        PossiblePorts |= ((U64)1 << PLX_FLAG_PORT_NT_LINK_0) |
                                         ((U64)1 << PLX_FLAG_PORT_NT_DS_P2P) |
                                         ((U64)1 << PLX_FLAG_PORT_DMA_0);
                        break;

                    case 0x8624:
                        PossiblePorts  = 0x00000373;  // 0,1,4-6,8,9,NT
                        PossiblePorts |= ((U64)1 << PLX_FLAG_PORT_NT_LINK_0);
                        break;

                    case 0x8632:
                        PossiblePorts  = 0x00000FFF;  // 0-11
                        break;

                    case 0x8647:
                        PossiblePorts  = 0x00000111;  // 0,4,8
                        break;

                    case 0x8648:
                        PossiblePorts  = 0x00000FFF;  // 0-11,NT
                        PossiblePorts |= ((U64)1 << PLX_FLAG_PORT_NT_LINK_0);
                        break;

                    case 0x8649:
                        PossiblePorts  = 0x00FF000F;  // 0-3,16-23,NT,NTB
                        PossiblePorts |= ((U64)1 << PLX_FLAG_PORT_NT_LINK_0) |
                                         ((U64)1 << PLX_FLAG_PORT_NT_VIRTUAL_0);
                        break;

                    case 0x8664:
                        PossiblePorts  = 0x00FF00FF;  // 0-7,16-23,NT,NTB
                        PossiblePorts |= ((U64)1 << PLX_FLAG_PORT_NT_LINK_0) |
                                         ((U64)1 << PLX_FLAG_PORT_NT_DS_P2P);
                        break;

                    case 0x8680:
                        PossiblePorts  = 0x00FF0FFF;  // 0-11,16-23,NT,NTB
                        PossiblePorts |= ((U64)1 << PLX_FLAG_PORT_NT_LINK_0) |
                                         ((U64)1 << PLX_FLAG_PORT_NT_VIRTUAL_0);
                        break;

                    case 0x8625:
                    case 0x8636:
                    case 0x8696:
                        PossiblePorts  = 0x00FFFFFF;  // 0-23,NT,NTB
                        PossiblePorts |= ((U64)1 << PLX_FLAG_PORT_NT_LINK_0) |
                                         ((U64)1 << PLX_FLAG_PORT_NT_VIRTUAL_0);
                        break;

                    case 0x8700:
                        PossiblePorts  = 0x0000000F;  // 0-3
                        PossiblePorts |= ((U64)1 << PLX_FLAG_PORT_NT_LINK_0) |
                                         ((U64)1 << PLX_FLAG_PORT_NT_VIRTUAL_0);
                        break;

                    case 0x8712:
                        PossiblePorts  = 0x00003C3F;  // 0-5,10-13,NT 0/1
                        PossiblePorts |= ((U64)1 << PLX_FLAG_PORT_NT_LINK_0) |
                                         ((U64)1 << PLX_FLAG_PORT_NT_LINK_1) |
                                         ((U64)1 << PLX_FLAG_PORT_NT_VIRTUAL_0) |
                                         ((U64)1 << PLX_FLAG_PORT_NT_VIRTUAL_1);
                        break;

                    case 0x8713:
                        PossiblePorts  = 0x00003C3F;  // 0-5,10-13,NT 0/1,DMA 0-4
                        PossiblePorts |= ((U64)1 << PLX_FLAG_PORT_NT_LINK_0) |
                                         ((U64)1 << PLX_FLAG_PORT_NT_LINK_1) |
                                         ((U64)1 << PLX_FLAG_PORT_NT_VIRTUAL_0) |
                                         ((U64)1 << PLX_FLAG_PORT_NT_VIRTUAL_1) |
                                         ((U64)1 << PLX_FLAG_PORT_DMA_0) |
                                         ((U64)1 << PLX_FLAG_PORT_DMA_1) |
                                         ((U64)1 << PLX_FLAG_PORT_DMA_2) |
                                         ((U64)1 << PLX_FLAG_PORT_DMA_3);
                        break;

                    case 0x8716:
                        PossiblePorts  = 0x00003C3F;  // 0-5,10-13,NT 0/1
                        PossiblePorts |= ((U64)1 << PLX_FLAG_PORT_NT_LINK_0) |
                                         ((U64)1 << PLX_FLAG_PORT_NT_LINK_1) |
                                         ((U64)1 << PLX_FLAG_PORT_NT_VIRTUAL_0) |
                                         ((U64)1 << PLX_FLAG_PORT_NT_VIRTUAL_1);
                        break;

                    case 0x8717:
                        PossiblePorts  = 0x00003C3F;  // 0-5,10-13,NT 0/1,DMA 0-4
                        PossiblePorts |= ((U64)1 << PLX_FLAG_PORT_NT_LINK_0) |
                                         ((U64)1 << PLX_FLAG_PORT_NT_LINK_1) |
                                         ((U64)1 << PLX_FLAG_PORT_NT_VIRTUAL_0) |
                                         ((U64)1 << PLX_FLAG_PORT_NT_VIRTUAL_1) |
                                         ((U64)1 << PLX_FLAG_PORT_DMA_0) |
                                         ((U64)1 << PLX_FLAG_PORT_DMA_1) |
                                         ((U64)1 << PLX_FLAG_PORT_DMA_2) |
                                         ((U64)1 << PLX_FLAG_PORT_DMA_3);
                        break;

                    case 0x8724:
                        PossiblePorts  = 0x00003F3F;  // 0-5,8-13,NT 0/1
                        PossiblePorts |= ((U64)1 << PLX_FLAG_PORT_NT_LINK_0) |
                                         ((U64)1 << PLX_FLAG_PORT_NT_LINK_1) |
                                         ((U64)1 << PLX_FLAG_PORT_NT_VIRTUAL_0) |
                                         ((U64)1 << PLX_FLAG_PORT_NT_VIRTUAL_1);
                        break;

                    case 0x8725:
                        PossiblePorts  = 0x00003E3F;  // 0-5,9-13,NT 0/1,DMA 0-4
                        PossiblePorts |= ((U64)1 << PLX_FLAG_PORT_NT_LINK_0) |
                                         ((U64)1 << PLX_FLAG_PORT_NT_LINK_1) |
                                         ((U64)1 << PLX_FLAG_PORT_NT_VIRTUAL_0) |
                                         ((U64)1 << PLX_FLAG_PORT_NT_VIRTUAL_1) |
                                         ((U64)1 << PLX_FLAG_PORT_DMA_0) |
                                         ((U64)1 << PLX_FLAG_PORT_DMA_1) |
                                         ((U64)1 << PLX_FLAG_PORT_DMA_2) |
                                         ((U64)1 << PLX_FLAG_PORT_DMA_3);
                        break;

                    case 0x8732:
                        PossiblePorts  = 0x003F3F3F;  // 0-5,8-13,16-21,NT 0/1
                        PossiblePorts |= ((U64)1 << PLX_FLAG_PORT_NT_LINK_0) |
                                         ((U64)1 << PLX_FLAG_PORT_NT_LINK_1) |
                                         ((U64)1 << PLX_FLAG_PORT_NT_VIRTUAL_0) |
                                         ((U64)1 << PLX_FLAG_PORT_NT_VIRTUAL_1);
                        break;

                    case 0x8733:
                        PossiblePorts  = 0x003F3F3F;  // 0-5,8-13,16-21,NT 0/1,DMA 0-4
                        PossiblePorts |= ((U64)1 << PLX_FLAG_PORT_NT_LINK_0) |
                                         ((U64)1 << PLX_FLAG_PORT_NT_LINK_1) |
                                         ((U64)1 << PLX_FLAG_PORT_NT_VIRTUAL_0) |
                                         ((U64)1 << PLX_FLAG_PORT_NT_VIRTUAL_1) |
                                         ((U64)1 << PLX_FLAG_PORT_DMA_0) |
                                         ((U64)1 << PLX_FLAG_PORT_DMA_1) |
                                         ((U64)1 << PLX_FLAG_PORT_DMA_2) |
                                         ((U64)1 << PLX_FLAG_PORT_DMA_3);
                        break;

                    case 0x8747:
                        PossiblePorts  = 0x00030303;  // 0,1,8,9,16,17
                        break;

                    case 0x8748:
                        PossiblePorts  = 0x003F3F3F;  // 0-5,8-13,16-21,NT 0/1
                        PossiblePorts |= ((U64)1 << PLX_FLAG_PORT_NT_LINK_0) |
                                         ((U64)1 << PLX_FLAG_PORT_NT_LINK_1) |
                                         ((U64)1 << PLX_FLAG_PORT_NT_VIRTUAL_0) |
                                         ((U64)1 << PLX_FLAG_PORT_NT_VIRTUAL_1);
                        break;

                    case 0x8749:
                        PossiblePorts  = 0x003F3F3F;  // 0-5,8-13,16-21,NT 0/1,DMA 0-4
                        PossiblePorts |= ((U64)1 << PLX_FLAG_PORT_NT_LINK_0) |
                                         ((U64)1 << PLX_FLAG_PORT_NT_LINK_1) |
                                         ((U64)1 << PLX_FLAG_PORT_NT_VIRTUAL_0) |
                                         ((U64)1 << PLX_FLAG_PORT_NT_VIRTUAL_1) |
                                         ((U64)1 << PLX_FLAG_PORT_DMA_0) |
                                         ((U64)1 << PLX_FLAG_PORT_DMA_1) |
                                         ((U64)1 << PLX_FLAG_PORT_DMA_2) |
                                         ((U64)1 << PLX_FLAG_PORT_DMA_3);
                        break;

                    default:
                        // For unsupported chips, set default
                        PossiblePorts = 0x0000000F;  // 0-3
                        break;
                }

                // For first port, perform additional steps
                if (Port == 0)
                {
                    if ((pDevice->Key.PlxFamily == PLX_FAMILY_CYGNUS)  ||
                        (pDevice->Key.PlxFamily == PLX_FAMILY_SCOUT)   ||
                        (pDevice->Key.PlxFamily == PLX_FAMILY_DRACO_1) ||
                        (pDevice->Key.PlxFamily == PLX_FAMILY_DRACO_2))
                        Offset_DebugCtrl = 0x350;
                    else
                        Offset_DebugCtrl = 0x1DC;

                    // Get port 0 debug control register
                    RegDebugCtrl =
                        PlxI2c_PlxRegisterRead(
                            pDevice,
                            Offset_DebugCtrl,   // Port 0 debug control
                            &status,
                            FALSE               // Do not adjust for port
                            );

                    if (status != ApiSuccess)
                        RegDebugCtrl = (U32)-1;

                    // Default to NT legacy mode
                    pDevice->Key.ApiInternal[1] = PLX_I2C_NT_MODE_LEGACY;

                    // Set NT mode
                    if (RegDebugCtrl != (U32)-1)
                    {
                        // Check if NT is enabled
                        if ((pDevice->Key.PlxFamily == PLX_FAMILY_CYGNUS)  ||
                            (pDevice->Key.PlxFamily == PLX_FAMILY_SCOUT)   ||
                            (pDevice->Key.PlxFamily == PLX_FAMILY_DRACO_1) ||
                            (pDevice->Key.PlxFamily == PLX_FAMILY_DRACO_2))
                        {
                            RegValue =
                                PlxI2c_PlxRegisterRead(
                                    pDevice,
                                    0x360,              // VS0 Upstream
                                    NULL,
                                    FALSE               // Do not adjust for port
                                    );

                            // Store upstream port number
                            Port_Upstream = (U8)((RegValue >> 0) & 0xF);

                            // Determine if NT is enabled & store port number
                            if (RegValue & (1 << 13))
                                Gbl_PlxI2cProp[pDevice->Key.ApiIndex].NTPortNum = (U8)((RegValue >> 8) & 0xF);
                            else
                                bNtEnabled = FALSE;
                        }
                        else
                        {
                            // Store upstream port number
                            Port_Upstream = (U8)((RegDebugCtrl >> 8) & 0xF);

                            // Determine if NT is enabled & store port number
                            if (pDevice->Key.PlxFamily == PLX_FAMILY_VEGA_LITE)
                            {
                                // NT is enabled if NT dual-host or intelligent adapter mode
                                if ((((RegDebugCtrl >> 18) & 0x3) == 0) ||
                                    (((RegDebugCtrl >> 18) & 0x3) == 3))
                                {
                                    bNtEnabled = FALSE;
                                }
                            }
                            else if ((RegDebugCtrl & (1 << 18)) == 0)
                                bNtEnabled = FALSE;

                            // Store NT port number if enabled
                            if (bNtEnabled)
                                Gbl_PlxI2cProp[pDevice->Key.ApiIndex].NTPortNum = (U8)((RegDebugCtrl >> 24) & 0xF);
                        }

                        DebugPrintf((
                            "I2C - Upstream port=%d  NT=%s (port=%02X)\n",
                            Port_Upstream,
                            bNtEnabled ? "Enabled" : "Disabled",
                            bNtEnabled ? Gbl_PlxI2cProp[pDevice->Key.ApiIndex].NTPortNum : 0xFF
                            ));

                        // Check for NT P2P mode
                        if ((pDevice->Key.PlxChip & 0xFF00) == 0x8700)
                        {
                            // Always NT P2P mode
                            pDevice->Key.ApiInternal[1] = PLX_I2C_NT_MODE_P2P;
                        }
                        else if (pDevice->Key.PlxFamily == PLX_FAMILY_CYGNUS)
                        {
                            // NT P2P mode (360[14])
                            if (RegDebugCtrl & (1 << 14))
                                pDevice->Key.ApiInternal[1] = PLX_I2C_NT_MODE_P2P;
                        }
                        else if (pDevice->Key.PlxFamily == PLX_FAMILY_SIRIUS)
                        {
                            // NT P2P mode (1DC[6])
                            if (RegDebugCtrl & (1 << 6))
                                pDevice->Key.ApiInternal[1] = PLX_I2C_NT_MODE_P2P;

                            // For some chips, I2C bus timeout (1DC[14]) must be enabled or
                            //  accesses to a disabled port prevents further I2C accesses.
                            if ((RegDebugCtrl & (1 << 14)) == 0)
                            {
                                RegDebugCtrl |= (1 << 14);

                                PlxI2c_PlxRegisterWrite(
                                    pDevice,
                                    0x1DC,
                                    RegDebugCtrl,
                                    FALSE               // Do not adjust for port
                                    );
                            }
                        }
                    }

                    // If legacy mode, do not probe for NT P2P port
                    if (pDevice->Key.ApiInternal[1] == PLX_I2C_NT_MODE_LEGACY)
                        PossiblePorts &= ~((U64)1 << PLX_FLAG_PORT_NT_DS_P2P);

                    // If NT not enabled, don't probe NT ports
                    if (bNtEnabled == FALSE)
                    {
                        PossiblePorts &=
                            ~(((U64)1 << PLX_FLAG_PORT_NT_LINK_0) |
                              ((U64)1 << PLX_FLAG_PORT_NT_LINK_1) |
                              ((U64)1 << PLX_FLAG_PORT_NT_VIRTUAL_0) |
                              ((U64)1 << PLX_FLAG_PORT_NT_VIRTUAL_1) |
                              ((U64)1 << PLX_FLAG_PORT_NT_DS_P2P));
                    }

                }
            }

            // Get PCI header
            RegPciHeader =
                PlxI2c_PlxRegisterRead(
                    pDevice,
                    0xC,            // PCI Header Type
                    &status,
                    TRUE            // Adjust for port
                    );

            if (status == ApiSuccess)
            {
                // Verify port exists
                if (RegPciHeader == (U32)-1)
                {
                    // Some chips return FFFF_FFFF when accessing 0Ch of disabled port
                    DebugPrintf(("I2C - Port %d is disabled, (PCI Header 0Ch = FFFF_FFFF)\n", Port));
                }
                else if ((RegPciHeader & 0xFFFF) == PLX_VENDOR_ID)
                {
                    // Some chips return value of last register (Dev/Ven ID)
                    // read when accessing 0Ch of disabled port
                    DebugPrintf(("I2C - Port %d is disabled, (PCI Header 0Ch returned Dev/Ven ID)\n", Port));
                }
                else
                {
                    DebugPrintf(("I2C - Port %d is enabled, getting additional properties\n", Port));
                    bCompareDevice = TRUE;
                }
            }
        }

        if (bCompareDevice)
        {
            // Fill in device key
            pDevice->Key.DeviceId = (U16)(DevVenID >> 16);
            pDevice->Key.VendorId = (U16)DevVenID;

            // Set header type field
            PciHeaderType = (U8)((RegPciHeader >> 16) & 0x7F);

            // Store NT port number if exists
            if (Gbl_PlxI2cProp[pDevice->Key.ApiIndex].NTPortNum != (U8)PCI_FIELD_IGNORE)
                pDevice->Key.NTPortNum = Gbl_PlxI2cProp[pDevice->Key.ApiIndex].NTPortNum;

            // Get port properties
            PlxI2c_GetPortProperties(
                pDevice,
                &PortProp
                );

            /******************************************************************
             * When port 0 is not the upstream port, additional work is needed
             * to probe the upstream port first and determine its bus number.
             *****************************************************************/
            if ((Gbl_PlxI2cProp[pDevice->Key.ApiIndex].UpstreamBus == (U8)PCI_FIELD_IGNORE) &&
                (Port == 0) &&
                (PortProp.PortType != PLX_PORT_UPSTREAM))
            {
                if (Port_Upstream != (U8)-1)
                {
                    // Set upstream port base offset
                    Offset_Upstream = Port_Upstream * 0x1000;

                    RegValue =
                        PlxI2c_PlxRegisterRead(
                            pDevice,
                            Offset_Upstream + 0x18, // Primary/Secondary bus numbers
                            &status,
                            FALSE                   // Do not adjust for port
                            );

                    if ((status == ApiSuccess) && (RegValue != (U32)-1))
                    {
                        // Set the upstream bus number
                        Gbl_PlxI2cProp[pDevice->Key.ApiIndex].UpstreamBus = (U8)(RegValue >> 0);
                    }
                }
            }

            // Get bus number
            if (PciHeaderType == 1)
            {
                RegValue =
                    PlxI2c_PlxRegisterRead(
                        pDevice,
                        0x18,           // Primary/Secondary bus numbers
                        NULL,
                        TRUE            // Adjust for port
                        );

                pDevice->Key.bus = (U8)(RegValue >> 0);

                // Store the upstream port bus number
                if (PortProp.PortType == PLX_PORT_UPSTREAM)
                {
                    Gbl_PlxI2cProp[pDevice->Key.ApiIndex].UpstreamBus = pDevice->Key.bus;
                }

                /***************************************************************
                 * If devices aren't enumerated (e.g. loopback mode), downstream
                 * bus numbers will not be assigned and will always end up on
                 * bus 0. If that is the case, default to upstream bus + 1.
                 **************************************************************/
                if ((PortProp.PortType == PLX_PORT_DOWNSTREAM) && (pDevice->Key.bus == 0))
                {
                    if (Gbl_PlxI2cProp[pDevice->Key.ApiIndex].UpstreamBus != (U8)PCI_FIELD_IGNORE)
                        pDevice->Key.bus = Gbl_PlxI2cProp[pDevice->Key.ApiIndex].UpstreamBus;

                    pDevice->Key.bus += 1;
                }
            }
            else
            {
                // Reset bus
                pDevice->Key.bus = 0;

                bAutoGenBus = TRUE;

                // Some chips have captured bus number register
                if ((pDevice->Key.PlxFamily == PLX_FAMILY_CYGNUS)  ||
                    (pDevice->Key.PlxFamily == PLX_FAMILY_SCOUT)   ||
                    (pDevice->Key.PlxFamily == PLX_FAMILY_DRACO_1) ||
                    (pDevice->Key.PlxFamily == PLX_FAMILY_DRACO_2))
                {
                    // Get Captured bus number
                    RegValue =
                        PlxI2c_PlxRegisterRead(
                            pDevice,
                            0x1DC,          // Captured bus number
                            &status,
                            TRUE            // Adjust for port
                            );

                    pDevice->Key.bus = (U8)RegValue;

                    // Cygnus NT Virtual port 1DCh not corrrect over I2C
                    if ((pDevice->Key.PlxFamily == PLX_FAMILY_CYGNUS) &&
                       ((pDevice->Key.PlxPort == PLX_FLAG_PORT_NT_VIRTUAL_0) ||
                        (pDevice->Key.PlxPort == PLX_FLAG_PORT_NT_VIRTUAL_1)))
                    {
                        pDevice->Key.bus = 0;
                    }

                    if (pDevice->Key.bus != 0)
                        bAutoGenBus = FALSE;
                }

                // Use Upstream bus if bus not determined
                if (pDevice->Key.bus == 0)
                {
                    if (Gbl_PlxI2cProp[pDevice->Key.ApiIndex].UpstreamBus != (U8)PCI_FIELD_IGNORE)
                        pDevice->Key.bus = Gbl_PlxI2cProp[pDevice->Key.ApiIndex].UpstreamBus;
                }

                // Auto-generate bus number if not yet determined
                if (bAutoGenBus)
                {
                    // Handle NT ports and DMA controller
                    if (pDevice->Key.NTPortType == PLX_NT_PORT_LINK)
                    {
                        pDevice->Key.bus += 4;

                        // Add a bus for 2nd NT port
                        if (pDevice->Key.PlxPort == PLX_FLAG_PORT_NT_LINK_1)
                            pDevice->Key.bus += 1;
                    }
                    else if ((pDevice->Key.PlxPort == PLX_FLAG_PORT_DMA_0) ||
                             (pDevice->Key.PlxPort == PLX_FLAG_PORT_DMA_1) ||
                             (pDevice->Key.PlxPort == PLX_FLAG_PORT_DMA_2) ||
                             (pDevice->Key.PlxPort == PLX_FLAG_PORT_DMA_3))
                    {
                        // DMA controller is on same bus as upstream port
                    }
                    else
                    {
                        pDevice->Key.bus += 1;

                        // Flag this is virtual port
                        pDevice->Key.NTPortType = PLX_NT_PORT_VIRTUAL;

                        // Flag an NT port detected
                        bNtEnabled = TRUE;

                        // For P2P mode, NT Virtual has parent DS port
                        if (pDevice->Key.ApiInternal[1] == PLX_I2C_NT_MODE_P2P)
                            pDevice->Key.bus += 1;

                        // Add a bus for 2nd NT port
                        if (pDevice->Key.PlxPort == PLX_FLAG_PORT_NT_VIRTUAL_1)
                            pDevice->Key.bus += 1;
                    }
                }
            }

            // Set port slot number
            if ((PortProp.PortType    == PLX_PORT_UPSTREAM)       ||
                (pDevice->Key.PlxPort == PLX_FLAG_PORT_NT_LINK_0) ||
                (pDevice->Key.PlxPort == PLX_FLAG_PORT_NT_LINK_1) ||
                (pDevice->Key.PlxPort == PLX_FLAG_PORT_DMA_0)     ||
                (pDevice->Key.PlxPort == PLX_FLAG_PORT_DMA_1)     ||
                (pDevice->Key.PlxPort == PLX_FLAG_PORT_DMA_2)     ||
                (pDevice->Key.PlxPort == PLX_FLAG_PORT_DMA_3))
            {
                // Upstream, NT Link, & DMA are always slot 0
                pDevice->Key.slot = 0;
            }
            else if (pDevice->Key.PlxPort == PLX_FLAG_PORT_NT_DS_P2P)
            {
                // NT P2P port uses NT port number as slot
                pDevice->Key.slot = pDevice->Key.NTPortNum;
            }
            else
                pDevice->Key.slot = (U8)(offset / 0x1000);  // Slot matches port number

            // Function number is 1 for DMA and always 0 for other ports
            if (pDevice->Key.PlxPort == PLX_FLAG_PORT_DMA_0)
                pDevice->Key.function = 1;
            else if (pDevice->Key.PlxPort == PLX_FLAG_PORT_DMA_1)
                pDevice->Key.function = 2;
            else if (pDevice->Key.PlxPort == PLX_FLAG_PORT_DMA_2)
                pDevice->Key.function = 3;
            else if (pDevice->Key.PlxPort == PLX_FLAG_PORT_DMA_3)
                pDevice->Key.function = 4;
            else
                pDevice->Key.function = 0;

            // Get PCI Revision
            RegValue =
                PlxI2c_PlxRegisterRead(
                    pDevice,
                    0x08,
                    &status,
                    TRUE            // Adjust for port
                    );

            pDevice->Key.Revision = (U8)RegValue;

            // Get subsystem device ID
            if (PciHeaderType == 0)
            {
                RegValue =
                    PlxI2c_PlxRegisterRead(
                        pDevice,
                        0x2C,
                        &status,
                        TRUE            // Adjust for port
                        );
            }
            else
            {
                RegValue = 0;
            }

            pDevice->Key.SubDeviceId = (U16)(RegValue >> 16);
            pDevice->Key.SubVendorId = (U16)RegValue;

            // Assume successful match
            bMatchLoc = TRUE;
            bMatchId  = TRUE;

            //
            // Compare device key information
            //

            // Compare Bus number
            if (pKey->bus != (U8)PCI_FIELD_IGNORE)
            {
                if (pKey->bus != pDevice->Key.bus)
                {
                    bMatchLoc = FALSE;
                }
            }

            // Compare Slot number
            if (pKey->slot != (U8)PCI_FIELD_IGNORE)
            {
                if (pKey->slot != pDevice->Key.slot)
                {
                    bMatchLoc = FALSE;
                }
            }

            // Compare Function number
            if (pKey->function != (U8)PCI_FIELD_IGNORE)
            {
                if (pKey->function != pDevice->Key.function)
                {
                    bMatchLoc = FALSE;
                }
            }

            //
            // Compare device ID information
            //

            // Compare Vendor ID
            if (pKey->VendorId != (U16)PCI_FIELD_IGNORE)
            {
                if (pKey->VendorId != pDevice->Key.VendorId)
                {
                    bMatchId = FALSE;
                }
            }

            // Compare Device ID
            if (pKey->DeviceId != (U16)PCI_FIELD_IGNORE)
            {
                if (pKey->DeviceId != pDevice->Key.DeviceId)
                {
                    bMatchId = FALSE;
                }
            }

            // Subsystem ID only valid in PCI header 0 type
            if (PciHeaderType == 0)
            {
                // Compare Subsystem Vendor ID
                if (pKey->SubVendorId != (U16)PCI_FIELD_IGNORE)
                {
                    if (pKey->SubVendorId != pDevice->Key.SubVendorId)
                    {
                        bMatchId = FALSE;
                    }
                }

                // Compare Subsystem Device ID
                if (pKey->SubDeviceId != (U16)PCI_FIELD_IGNORE)
                {
                    if (pKey->SubDeviceId != pDevice->Key.SubDeviceId)
                    {
                        bMatchId = FALSE;
                    }
                }
            }

            // Compare Revision
            if (pKey->Revision != (U8)PCI_FIELD_IGNORE)
            {
                if (pKey->Revision != pDevice->Key.Revision)
                {
                    bMatchId = FALSE;
                }
            }

            // Check if match on location and ID
            if (bMatchLoc && bMatchId)
            {
                // Match found, check if it is the desired device
                if (DeviceCount == DeviceNumber)
                {
                    DebugPrintf((
                        "Criteria matched with device %04X %04X [b:%02X s:%02X f:%d]\n",
                        pDevice->Key.DeviceId, pDevice->Key.VendorId,
                        pDevice->Key.bus, pDevice->Key.slot, pDevice->Key.function
                        ));

                    // Copy the device information
                    *pKey = pDevice->Key;

                    return ApiSuccess;
                }

                // Increment device count
                DeviceCount++;
            }
        }

        // Go to next port
        Port++;
    }

    // Return number of matched devices
    *pNumMatched = DeviceCount;

    DebugPrintf(("Criteria did not match any devices\n"));

    return ApiInvalidDeviceInfo;
}




/*******************************************************************************
 *
 * Function   :  PlxGetExtendedCapabilityOffset
 *
 * Description:  Scans the capability list to search for a specific capability
 *
 ******************************************************************************/
U16
PlxGetExtendedCapabilityOffset(
    PLX_DEVICE_OBJECT *pDevice,
    U16                CapabilityId
    )
{
    U16 Offset_Cap;
    U32 RegValue;


    // Get offset of first capability
    PLX_PCI_REG_READ(
        pDevice,
        0x34,           // PCI capabilities pointer
        &RegValue
        );

    // If link is down, PCI reg accesses will fail
    if (RegValue == (U32)-1)
        return 0;

    // Set first capability
    Offset_Cap = (U16)RegValue;

    // Traverse capability list searching for desired ID
    while ((Offset_Cap != 0) && (Offset_Cap  != (U8)-1))
    {
        // Get next capability
        PLX_PCI_REG_READ(
            pDevice,
            Offset_Cap,
            &RegValue
            );

        if ((U8)RegValue == (U8)CapabilityId)
        {
            // Capability found, return base offset
            return Offset_Cap;
        }

        // Jump to next capability
        Offset_Cap = (U16)((RegValue >> 8) & 0xFF);
    }

    // Capability not found
    return 0;
}




/******************************************************************************
 *
 * Function   :  PlxChipTypeDetect
 *
 * Description:  Attempts to determine PLX chip type and revision
 *
 ******************************************************************************/
BOOLEAN
PlxChipTypeDetect(
    PLX_DEVICE_OBJECT *pDevice
    )
{
    U8  i;
    U16 offset[] = {0xE0,0x958,0xB78,0x0};
    U32 RegValue;


    // Default revision to PCI revision
    pDevice->Key.PlxRevision = pDevice->Key.Revision;

    i = 0;

    while (offset[i] != 0)
    {
        // Check for hard-coded ID
        PLX_PCI_REG_READ(
            pDevice,
            offset[i],
            &RegValue
            );

        if ((RegValue & 0xFFFF) == PLX_VENDOR_ID)
        {
            pDevice->Key.PlxChip = (U16)(RegValue >> 16);

            // Some PLX chips did not update hard-coded ID in revisions
            if ((pDevice->Key.PlxChip != 0x8612) &&
                (pDevice->Key.PlxChip != 0x8616) &&
                (pDevice->Key.PlxChip != 0x8624) &&
                (pDevice->Key.PlxChip != 0x8632) &&
                (pDevice->Key.PlxChip != 0x8647) &&
                (pDevice->Key.PlxChip != 0x8648))
            {
                // PLX revision should be in next register
                PLX_PCI_REG_READ(
                    pDevice,
                    offset[i] + sizeof(U32),
                    &RegValue
                    );

                pDevice->Key.PlxRevision = (U8)(RegValue & 0xFF);
                goto _PlxChipAssignFamily;
            }
        }

        // Go to next offset
        i++;
    }

    // Attempt to use Subsytem ID for DMA devices that don't have hard-coded ID
    if ((pDevice->Key.DeviceId == 0x87D0) || (pDevice->Key.DeviceId == 0x87E0))
    {
        // Get PCI Subsystem ID
        PLX_PCI_REG_READ(
            pDevice,
            0x2C,
            &RegValue
            );

        if ((RegValue & 0xFF00FFFF) == 0x870010B5)
        {
            pDevice->Key.PlxChip = (U16)(RegValue >> 16);
            goto _PlxChipAssignFamily;
        }
    }

    // Hard-coded ID doesn't exist, so use Device/Vendor ID
    pDevice->Key.PlxChip = pDevice->Key.DeviceId;

    // Detect the PLX chip revision
    PlxChipRevisionDetect(
        pDevice
        );

_PlxChipAssignFamily:

    switch (pDevice->Key.PlxChip)
    {
        case 0x9050:        // 9000 series
        case 0x9030:
        case 0x9080:
        case 0x9054:
        case 0x9056:
        case 0x9656:
        case 0x8311:
            pDevice->Key.PlxFamily = PLX_FAMILY_BRIDGE_P2L;
            break;

        case 0x6140:        // 6000 series
        case 0x6150:
        case 0x6152:
        case 0x6154:
        case 0x6156:
        case 0x6254:
        case 0x6350:
        case 0x6520:
        case 0x6540:
        case 0x6466:
            pDevice->Key.PlxFamily = PLX_FAMILY_BRIDGE_PCI_P2P;
            break;

        case 0x8111:
        case 0x8112:
        case 0x8114:
            pDevice->Key.PlxFamily = PLX_FAMILY_BRIDGE_PCIE_P2P;
            break;

        case 0x8525:
        case 0x8533:
        case 0x8547:
        case 0x8548:
            pDevice->Key.PlxFamily = PLX_FAMILY_ALTAIR;
            break;

        case 0x8505:
        case 0x8509:
            pDevice->Key.PlxFamily = PLX_FAMILY_ALTAIR_XL;
            break;

        case 0x8516:
        case 0x8524:
        case 0x8532:
            pDevice->Key.PlxFamily = PLX_FAMILY_VEGA;
            break;

        case 0x8508:
        case 0x8512:
        case 0x8517:
        case 0x8518:
            pDevice->Key.PlxFamily = PLX_FAMILY_VEGA_LITE;
            break;

        case 0x8612:
        case 0x8616:
        case 0x8624:
        case 0x8632:
        case 0x8647:
        case 0x8648:
            pDevice->Key.PlxFamily = PLX_FAMILY_DENEB;
            break;

        case 0x8604:
        case 0x8606:
        case 0x8608:
        case 0x8609:
        case 0x8613:
        case 0x8614:
        case 0x8615:
        case 0x8617:
        case 0x8618:
        case 0x8619:
            pDevice->Key.PlxFamily = PLX_FAMILY_SIRIUS;
            break;

        case 0x8625:
        case 0x8636:
        case 0x8649:
        case 0x8664:
        case 0x8680:
        case 0x8696:
            pDevice->Key.PlxFamily = PLX_FAMILY_CYGNUS;
            break;

        case 0x8700:
            // DMA devices that don't have hard-coded ID
            if ((pDevice->Key.DeviceId == 0x87D0) || (pDevice->Key.DeviceId == 0x87E0))
                pDevice->Key.PlxFamily = PLX_FAMILY_DRACO_1;
            else
                pDevice->Key.PlxFamily = PLX_FAMILY_SCOUT;
            break;

        case 0x8408:
        case 0x8416:
        case 0x8712:
        case 0x8716:
        case 0x8724:
        case 0x8732:
        case 0x8747:
        case 0x8748:
            pDevice->Key.PlxFamily = PLX_FAMILY_DRACO_1;
            break;

        case 0x8713:
        case 0x8717:
        case 0x8725:
        case 0x8733:
        case 0x8749:
            pDevice->Key.PlxFamily = PLX_FAMILY_DRACO_2;
            break;

        case 0:
            pDevice->Key.PlxFamily = PLX_FAMILY_NONE;
            break;

        default:
            pDevice->Key.PlxFamily = PLX_FAMILY_UNKNOWN;
            break;
    }

    return TRUE;
}




/******************************************************************************
 *
 * Function   :  PlxChipRevisionDetect
 *
 * Description:  Attempts to detect the PLX chip revision
 *
 ******************************************************************************/
VOID
PlxChipRevisionDetect(
    PLX_DEVICE_OBJECT *pDevice
    )
{
    U32 RegValue;


    // Get PCI Class code/Revision ID register
    PLX_PCI_REG_READ(
        pDevice,
        0x08,           // PCI Revision ID
        &RegValue
        );

    // Default revision to value in chip
    pDevice->Key.PlxRevision = (U8)(RegValue & 0xFF);
}
