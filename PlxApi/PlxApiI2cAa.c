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

/*******************************************************************************
 *
 * File Name:
 *
 *      PlxApiI2cAa.c
 *
 * Description:
 *
 *      Implements the PLX API functions over an I2C interface
 *
 * Revision History:
 *
 *      02-01-17 : PLX SDK v7.25
 *
 ******************************************************************************/

/*******************************************************************************
 *
 * I2C API utilization of fields in PLX_DEVICE_KEY
 *
 *  ApiIndex        - I2C USB device number (0,1,etc)
 *  DeviceNumber    - PLX chip I2C slave address
 *  PlxPort         - PLX port # or port type (NTL,NTV,DMA,etc)
 *  PlxPortType     - PLX-specific port type (PLX_PORT_TYPE)
 *  NTPortNum       - NT port numbers if enabled (NT1=[7:4] NT0=[3:0])
 *  DeviceMode      - Current PLX chip mode (Std,NT,VS,Fabric,etc)
 *  ApiInternal[0]  - I2C bus speed in KHz
 *  ApiInternal[1]  - Port-specific base register address from BAR
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
 * Draco 1/Scout
 *              Transp  NTV0 NTV1 NTL0 NTL1 DMA0 DMA1 DMA2 DMA3 DMA_RAM
 *   Mode          0      2    2    1    1    3    3    3    3     3
 *   Stn_Sel      S#      0    0    0    0    0    0    0    0     0
 *   Port_Sel     P#      0    1    0    1    0    1    2    3     4
 *
 * Draco 2
 *              Transp  NTV0 NTV1 NTL0 NTL1 DMA0 DMA1 DMA2 DMA3 DMA_RAM ALUT
 *   Mode          0      2    2    1    1    3    3    3    3     3      3
 *   Stn_Sel      S#      0    0    0    0    0    0    0    0     0      2
 *   Port_Sel     P#      0    1    0    1    0    1    2    3     4     R#
 *
 * Capella 1 / Capella 2 (Standard mode)            | C2 Fabric Mode (*Reg 26C=R[22:18])
 *              Transp  NTV0 NTV1 NTL0 NTL1 ALUT    |
 *   Mode          0      2    2    1    1    3     |        2
 *   Stn_Sel      S#      0    0    0    0    2     |     R[17:15]
 *   Port_Sel     P#      0    1    0    1   R#     |     R[14:12]
 *
 * Mira
 *              Transp  NTV0 NTV1 NTL0 NTL1 DMA0 DMA1 DMA2 DMA3 DMA_RAM PCI2USB USB
 *   Mode         --     --   --   --   --   --   --   --   --    --       --    -
 *   Stn_Sel      --     --   --   --   --   --   --   --   --    --       --    -
 *   Port_Sel     P#     --   --   --   --   --   --   --   --    --        3    4
 *
 ******************************************************************************/

/*******************************************************************************
 *
 * PLX chip I2C port access command
 *
 * Vega-Lite
 *      31   27 26     24 23          19   18   17   15  14 13     10 9          0
 *      --------------------------------------------------------------------------
 *     | Resvd | I2C_Cmd |    Resvd     | Mode | PtSel | R | Byte_En |  DW_Offset |
 *      --------------------------------------------------------------------------
 *
 * Altair/Altair-XL
 *      31   27 26     24 23          19 18         15 14  13     10 9          0
 *      -------------------------------------------------------------------------
 *     | Resvd | I2C_Cmd |    Resvd     |   Port_Sel  | R | Byte_En |  DW_Offset |
 *      -------------------------------------------------------------------------
 *
 * Deneb
 *      31   27 26     24 23  20   19   18    17 16   15 14  13     10 9          0
 *      ---------------------------------------------------------------------------
 *     | Resvd | I2C_Cmd | Rsvd | Mode | StnSel | PtSel | R | Byte_En |  DW_Offset |
 *      ---------------------------------------------------------------------------
 *
 * Sirius
 *      31   27 26     24 23  20   19   18        15 14  13     10 9          0
 *      -----------------------------------------------------------------------
 *     | Resvd | I2C_Cmd | Rsvd | Mode |  Port_Sel  | R | Byte_En |  DW_Offset |
 *      -----------------------------------------------------------------------
 *
 * Cygnus
 *      31   27 26     24 23  21   20   19    17 16   15 14  13     10 9          0
 *      ---------------------------------------------------------------------------
 *     | Resvd | I2C_Cmd | Rsvd | Mode | StnSel | PtSel | R | Byte_En |  DW_Offset |
 *      ---------------------------------------------------------------------------
 *
 * Draco 1 & 2 / Scout
 *      31   27 26     24 23  22 21  20 19    18 17   15 14  13     10 9          0
 *      ---------------------------------------------------------------------------
 *     | Resvd | I2C_Cmd | Rsvd | Mode | StnSel | PtSel | R | Byte_En |  DW_Offset |
 *      ---------------------------------------------------------------------------
 *
 * Capella 1 / Capella 2
 *      31   27 26     24 23  22  21 20    18 17   15 14  13     10 9          0
 *      ------------------------------------------------------------------------
 *     | Resvd | I2C_Cmd | R | Mode | StnSel | PtSel | R | Byte_En |  DW_Offset |
 *      ------------------------------------------------------------------------
 *
 * Mira
 *      31   27 26     24 23  20  19       15 14  13     10 9          0
 *      ----------------------------------------------------------------
 *     | Resvd | I2C_Cmd | Rsvd |  Port_Sel  | R | Byte_En |  DW_Offset |
 *      ----------------------------------------------------------------
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


#include <malloc.h>
#include <math.h>
#include "Eep_8000.h"
#include "PciRegs.h"
#include "PexApi.h"
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
        U8                NTPortNum[PLX_I2C_MAX_NT_PORTS];
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
    {
        return PLX_STATUS_UNSUPPORTED;
    }

    // Find all I2C ports
    PortCount = aa_find_devices( PLX_I2C_MAX_DEVICES, Ports );
    if (PortCount <= 0)
    {
        return PLX_STATUS_NO_DRIVER;
    }

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

    return PLX_STATUS_OK;
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
    if (PlxI2c_Driver_Connect( pDevice, NULL ) == FALSE)
    {
        return PLX_STATUS_INVALID_OBJECT;
    }

    // Mark object as valid
    ObjectValidate( pDevice );

    // Fill in chip version information
    PlxChipTypeDetect( pDevice );

    return PLX_STATUS_OK;
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
    {
        return PLX_STATUS_INVALID_ACCESS;
    }

    // Decrement open count and close device if no longer referenced
    if (InterlockedDecrement(
            &Gbl_PlxI2cProp[pDevice->Key.ApiIndex].OpenCount
            ) == 0)
    {
        // Close the device (aa_close() returns num handles closed)
        if (aa_close( (Aardvark)PLX_PTR_TO_INT( pDevice->hDevice ) ) != 1)
        {
            return PLX_STATUS_INVALID_ACCESS;
        }

        // Mark device is closed
        Gbl_PlxI2cProp[pDevice->Key.ApiIndex].hDevice = 0;

        // Delete the register access lock
        DeleteCriticalSection(
            &(Gbl_PlxI2cProp[pDevice->Key.ApiIndex].Lock_I2cRegAccess)
            );
    }

    return PLX_STATUS_OK;
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
    U16             DeviceNumber,
    PLX_MODE_PROP  *pModeProp
    )
{
    U8                i;
    int               PortCount;
    U16               NumMatched;
    U16               TotalMatches;
    U16               Ports[PLX_I2C_MAX_DEVICES];
    U32               RegValue;
    BOOLEAN           bFound;
    BOOLEAN           bAutoProbe;
    PLX_STATUS        status;
    PLX_DEVICE_OBJECT Device;
    PLX_DEVICE_OBJECT DeviceTemp;


    // Find all I2C ports
    PortCount = aa_find_devices( PLX_I2C_MAX_DEVICES, Ports );
    if (PortCount <= 0)
    {
        return PLX_STATUS_NO_DRIVER;
    }

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
    {
        return PLX_STATUS_INVALID_DATA;
    }

    // Verify port is not in use
    if (Ports[i] & AA_PORT_NOT_FREE)
    {
        return PLX_STATUS_IN_USE;
    }

    // Clear the device object
    RtlZeroMemory( &Device, sizeof(PLX_DEVICE_OBJECT) );

    // Use default clock rate if not specified
    if (pModeProp->I2c.ClockRate == 0)
    {
        pModeProp->I2c.ClockRate = PLX_I2C_DEFAULT_CLOCK_RATE;
    }

    // Open connection to driver
    if (PlxI2c_Driver_Connect( &Device, pModeProp ) == FALSE)
    {
        return PLX_STATUS_INVALID_OBJECT;
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
        Device.Key.DeviceNumber = pModeProp->I2c.SlaveAddr;
    }

    // Must validate object so probe API calls don't fail
    ObjectValidate( &Device );

    // Default to device not found
    bFound = FALSE;

    // Probe devices to check if valid PLX device
    do
    {
        DebugPrintf((" ---- Probe I2C at %02Xh ----\n", Device.Key.DeviceNumber));

        // Reset temporary device object
        RtlCopyMemory( &DeviceTemp, &Device, sizeof(PLX_DEVICE_OBJECT) );

        // Attempt to read Device/Vendor ID
        RegValue =
            PlxI2c_PlxRegisterRead(
                &DeviceTemp,
                0,                  // Port 0 Device/Vendor ID
                &status,
                FALSE,              // Adjust for port?
                FALSE               // Retry on error?
                );

        if (status == PLX_STATUS_OK)
        {
            if ((RegValue & 0xFFFF) == PLX_VENDOR_ID)
            {
                DebugPrintf((
                    "I2C - Detected device %08X at %02Xh\n",
                    RegValue, Device.Key.DeviceNumber
                    ));

                // PLX device found, determine active ports and compare
                status =
                    PlxI2c_ProbeSwitch(
                        &DeviceTemp,
                        pKey,
                        (U16)(DeviceNumber - TotalMatches),
                        &NumMatched
                        );

                if (status == PLX_STATUS_OK)
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
        return PLX_STATUS_OK;
    }

    return PLX_STATUS_INVALID_OBJECT;
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
        {
            return PLX_STATUS_FAILED;
        }
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
    {
        aa_close( hI2cDev );
    }

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

    return PLX_STATUS_OK;
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

    return PLX_STATUS_OK;
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
    PLX_PORT_PROP PortProp;


    // Attempt auto-detection if requested
    if (ChipType == 0)
    {
        PlxChipTypeDetect( pDevice );
        ChipType = pDevice->Key.PlxChip;
    }

    // Verify chip type
    switch (ChipType & 0xFF00)
    {
        case 0:         // Used to clear chip type
        case 0x2300:
        case 0x3300:
        case 0x8500:
        case 0x8600:
        case 0x8700:
        case 0x9700:
            // Get port properties to ensure upstream node
            PlxI2c_GetPortProperties( pDevice, &PortProp );

            if (PortProp.PortType != PLX_PORT_UPSTREAM)
            {
                DebugPrintf(("ERROR - Chip type may only be changed on upstream port\n"));
                return PLX_STATUS_UNSUPPORTED;
            }
            break;

        default:
            DebugPrintf((
                "ERROR - Invalid or unsupported chip type (%04X)\n",
                ChipType
                ));
            return PLX_STATUS_INVALID_DATA;
    }

    // Set the new chip type
    pDevice->Key.PlxChip = ChipType;

    // Check if we should update the revision or use the default
    if ((Revision == (U8)-1) || (Revision == 0))
    {
        // Attempt to detect Revision ID
        PlxChipRevisionDetect( pDevice );
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

    return PLX_STATUS_OK;
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
    RtlZeroMemory( pPortProp, sizeof(PLX_PORT_PROP) );

    // Get the offset of the PCI Express capability
    Offset_PcieCap =
        PlxPciFindCapability(
            pDevice,
            PCI_CAP_ID_PCI_EXPRESS,
            FALSE,
            0
            );
    if (Offset_PcieCap == 0)
    {
        DebugPrintf((
            "[D%d %02X:%02X.%X] Does not contain PCIe capability\n",
            pDevice->Key.domain, pDevice->Key.bus, pDevice->Key.slot, pDevice->Key.function
            ));

        // Mark device as non-PCIe
        pPortProp->bNonPcieDevice = TRUE;
        pPortProp->PortType       = PLX_PORT_UNKNOWN;
        return PLX_STATUS_OK;
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
    pPortProp->MaxPayloadSupported = PCIE_MPS_MRR_TO_BYTES( MaxSize );

    // Get PCIe Device Control
    PLX_PCI_REG_READ(
        pDevice,
        Offset_PcieCap + 0x08,
        &RegValue
        );

    // Get max payload size field
    MaxSize = (U8)(RegValue >> 5) & 0x7;

    // Set max payload size (=128 * (2 ^ MaxPaySizeField))
    pPortProp->MaxPayloadSize = PCIE_MPS_MRR_TO_BYTES( MaxSize );

    // Get max read request size field
    MaxSize = (U8)(RegValue >> 12) & 0x7;

    // Set max read request size (=128 * (2 ^ MaxReadReqSizeField))
    pPortProp->MaxReadReqSize = PCIE_MPS_MRR_TO_BYTES( MaxSize );

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

    /**********************************************************
     * In MIRA 3300 Enhanced mode, link width for the DS port & USB EP
     * is incorrectly reported as x0. Override with Max Width.
     *********************************************************/
    if ((pDevice->Key.PlxFamily == PLX_FAMILY_MIRA) &&
        ((pDevice->Key.PlxChip & 0xFF00) == 0x3300) &&
        (pDevice->Key.DeviceMode == PLX_CHIP_MODE_STANDARD) &&
        (pPortProp->LinkWidth == 0))
    {
        DebugPrintf((
            "NOTE - Override reported link width (x%d) with Max width (x%d)\n",
            pPortProp->LinkWidth, pPortProp->MaxLinkWidth
            ));
        pPortProp->LinkWidth = pPortProp->MaxLinkWidth;
    }

    /**********************************************************
     * If using port bifurication, Draco 2 DS ports may report
     * incorrect port numbers. Override with PLX port number.
     *********************************************************/
    if ((pDevice->Key.PlxFamily == PLX_FAMILY_DRACO_2) &&
        (pPortProp->PortType == PLX_PORT_DOWNSTREAM) &&
        (pPortProp->PortNumber != pDevice->Key.PlxPort))
    {
        DebugPrintf((
            "NOTE - Override reported port num (%d) with probed port num (%d)\n",
            pPortProp->PortNumber, pDevice->Key.PlxPort
            ));
        pPortProp->PortNumber = pDevice->Key.PlxPort;
    }

    DebugPrintf((
        "P=%d T=%d MPS=%d/%d MRR=%d L=G%dx%d/G%dx%d\n",
        pPortProp->PortNumber, pPortProp->PortType,
        pPortProp->MaxPayloadSize, pPortProp->MaxPayloadSupported,
        pPortProp->MaxReadReqSize,
        pPortProp->LinkSpeed, pPortProp->LinkWidth,
        pPortProp->MaxLinkSpeed, pPortProp->MaxLinkWidth
        ));

    return PLX_STATUS_OK;
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
    return PLX_STATUS_UNSUPPORTED;
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
    BOOLEAN            bAdjustForPort,
    U16                bRetryOnError
    )
{
    int Status_AA;
    U8  bSetUpper;
    U16 nBytesRead;
    U16 nBytesWritten;
    U32 Command;
    U32 RegValue;


    // Set max retry count
    if (bRetryOnError)
    {
        bRetryOnError = PLX_I2C_RETRY_MAX_COUNT;
    }
    else
    {
        bRetryOnError = 0;
    }

    // Verify register offset
    if (offset & 0x3)
    {
        DebugPrintf(("ERROR - Invalid register offset (0x%x)\n", offset));
        if (pStatus != NULL)
        {
            *pStatus = PLX_STATUS_INVALID_OFFSET;
        }
        return 0;
    }

    // Default to error
    if (pStatus != NULL)
    {
        *pStatus = PLX_STATUS_FAILED;
    }

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
        {
            *pStatus = PLX_STATUS_OK;
        }
        return 0;
    }

    if (Command == PLX_I2C_CMD_ERROR)
    {
        return 0;
    }

    // Default to not modify upper bits
    bSetUpper = FALSE;

    // For fabric mode, offset[22:18] is taken from 2CC[4:0]
    if ((pDevice->Key.DeviceMode == PLX_CHIP_MODE_FABRIC) &&
        (offset >= ((U32)1 << 18)))
    {
        bSetUpper = TRUE;
        PlxI2c_PlxRegisterWrite(
            pDevice,
            0x2CC,
            ((offset >> 18) & 0x3F),
            FALSE               // Adjust for port?
            );
    }

    // Set default return value
    RegValue = 0;

    do
    {
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

        // Release the register access lock
        LeaveCriticalSection(
            &(Gbl_PlxI2cProp[pDevice->Key.ApiIndex].Lock_I2cRegAccess)
            );

        // Read status in [15:8] and write status in [7:0]
        if (((Status_AA >> 8) == AA_OK) && ((Status_AA & 0xFF) == AA_OK) &&
            (nBytesRead == sizeof(U32)) && (nBytesWritten == sizeof(U32)))
        {
            if (pStatus != NULL)
            {
                *pStatus = PLX_STATUS_OK;
            }

            // Convert to Big Endian format since data is returned in BE
            RegValue = PLX_BE_DATA_32( RegValue );

            if (bRetryOnError)
            {
                // Get retry count
                bRetryOnError = PLX_I2C_RETRY_MAX_COUNT - bRetryOnError;

                // Log a note if had to retry I2C access
                if (bRetryOnError)
                {
                    ErrorPrintf((
                        "NOTE: I2C READ required %d retr%s (%04X@%02X O:%02Xh)\n",
                        bRetryOnError, (bRetryOnError > 1) ? "ies" : "y",
                        pDevice->Key.PlxChip, pDevice->Key.DeviceNumber, offset
                        ));
                }

                // Flag success to halt retry attempts
                bRetryOnError = 0;
            }
        }
        else
        {
            /******************************************************
             * In some cases, the chip's I2C state machine may not
             * respond to commands. Re-issuing the same command
             * eventually returns a successful result, usually
             * within 2 retries or less.
             *****************************************************/
            if (bRetryOnError > 0)
            {
                // Small delay to let chip state machine get ready for next command
                Plx_sleep( PLX_I2C_RETRY_DELAY_MS );

                // Decrement retry count
                bRetryOnError--;

                // Check if reached final one
                if (bRetryOnError == 0)
                {
                    ErrorPrintf((
                        "ERROR: I2C READ failed (St_Rd:%02d %dB St_Wr:%02d %dB O:%02Xh)\n",
                        (Status_AA >> 8) & 0xFF, nBytesRead,
                        (Status_AA & 0xFF), nBytesWritten, offset
                        ));
                }
            }
            RegValue = 0;
        }
    }
    while (bRetryOnError);

    // Clear upper address reg if modified
    if (bSetUpper)
    {
        PlxI2c_PlxRegisterWrite( pDevice, 0x2CC, 0, FALSE );
    }

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
    int        Status_AA;
    U8         bSetUpper;
    U16        bRetryOnError;
    U32        Command;
    U32        DataStream[2];
    PLX_STATUS status;


    // Verify register offset
    if (offset & 0x3)
    {
        DebugPrintf(("ERROR - Invalid register offset (0x%x)\n", offset));
        return PLX_STATUS_INVALID_OFFSET;
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
    {
        return PLX_STATUS_OK;
    }

    if (Command == PLX_I2C_CMD_ERROR)
    {
        return PLX_STATUS_FAILED;
    }

    // Default to not modify upper bits
    bSetUpper = FALSE;

    // For fabric mode, offset[22:18] is taken from 2CC[4:0]
    if ((pDevice->Key.DeviceMode == PLX_CHIP_MODE_FABRIC) &&
        (offset >= ((U32)1 << 18)))
    {
        bSetUpper = TRUE;
        PlxI2c_PlxRegisterWrite(
            pDevice,
            0x2CC,
            ((offset >> 18) & 0x3F),
            FALSE               // Adjust for port?
            );
    }

    // Convert register value to Big Endian format
    value = PLX_BE_DATA_32( value );

    // Copy command and value into data stream
    DataStream[0] = Command;
    DataStream[1] = value;

    // Set max retry count
    bRetryOnError = PLX_I2C_RETRY_MAX_COUNT;

    // Default to success
    status = PLX_STATUS_OK;

    do
    {
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

        if (Status_AA == (2 * sizeof(U32)))
        {
            if (bRetryOnError)
            {
                // Get retry count
                bRetryOnError = PLX_I2C_RETRY_MAX_COUNT - bRetryOnError;

                // Log a note if had to retry I2C access
                if (bRetryOnError)
                {
                    ErrorPrintf((
                        "NOTE: I2C WRITE required %d retr%s (%04X@%02X O:%02Xh V:%08X)\n",
                        bRetryOnError, (bRetryOnError > 1) ? "ies" : "y",
                        pDevice->Key.PlxChip, pDevice->Key.DeviceNumber, offset, value
                        ));
                }

                // Flag success to halt retry attempts
                bRetryOnError = 0;
            }
        }
        else
        {
            /******************************************************
             * In some cases, the chip's I2C state machine may not
             * respond to commands. Re-issuing the same command
             * eventually returns a successful result, usually
             * within 2 retries or less.
             *****************************************************/
            if (bRetryOnError > 0)
            {
                // Small delay to let chip state machine get ready for next command
                Plx_sleep( PLX_I2C_RETRY_DELAY_MS );

                // Decrement retry count
                bRetryOnError--;

                // Check if reached final one
                if (bRetryOnError == 0)
                {
                    ErrorPrintf((
                        "ERROR: I2C WRITE failed (%04X@%02X St:%dB/8B O:%02Xh V:%08X)\n",
                        pDevice->Key.PlxChip, pDevice->Key.DeviceNumber, Status_AA, offset, value
                        ));
                    status = PLX_STATUS_FAILED;
                }
            }
        }
    }
    while (bRetryOnError);

    // Clear upper address reg if modified
    if (bSetUpper)
    {
        PlxI2c_PlxRegisterWrite( pDevice, 0x2CC, 0, FALSE );
    }

    return status;
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
        PCI_REG_HDR_CACHE_LN,
        &PciHeaderType
        );

    PciHeaderType = (U8)((PciHeaderType >> 16) & 0x7F);

    // Verify BAR index
    switch (PciHeaderType)
    {
        case 0:
            if ((BarIndex != 0) && (BarIndex > 5))
            {
                return PLX_STATUS_INVALID_ACCESS;
            }
            break;

        case 1:
            if ((BarIndex != 0) && (BarIndex != 1))
            {
                DebugPrintf(("BAR %d does not exist on PCI type 1 header\n", BarIndex));
                return PLX_STATUS_INVALID_ACCESS;
            }
            break;

        default:
            return PLX_STATUS_INVALID_ACCESS;
    }

    // Get BAR properties if not probed yet
    if (pDevice->PciBar[BarIndex].BarValue == 0)
    {
        // Read PCI BAR
        PLX_PCI_REG_READ(
            pDevice,
            PCI_REG_BAR_0 + (BarIndex * sizeof(U32)),
            &PciBar
            );

        // Query BAR range
        PLX_PCI_REG_WRITE(
            pDevice,
            PCI_REG_BAR_0 + (BarIndex * sizeof(U32)),
            (U32)-1
            );

        // Read size
        PLX_PCI_REG_READ(
            pDevice,
            PCI_REG_BAR_0 + (BarIndex * sizeof(U32)),
            &Size
            );

        // Restore BAR
        PLX_PCI_REG_WRITE(
            pDevice,
            PCI_REG_BAR_0 + (BarIndex * sizeof(U32)),
            PciBar
            );

        // If upper 32-bit of 64-bit BAR, all bits will be set, which is not supported
        if (Size == (U32)-1)
        {
            Size = 0;
        }

        // Store BAR properties
        pDevice->PciBar[BarIndex].BarValue = PciBar;

        // Determine BAR address & size based on space type
        if (PciBar & (1 << 0))
        {
            // Some devices may not report upper 16-bits as FFFF
            Size |= 0xFFFF0000;

            pDevice->PciBar[BarIndex].Physical = PciBar & ~0x3;
            pDevice->PciBar[BarIndex].Size     = (~(Size & ~0x3)) + 1;
            pDevice->PciBar[BarIndex].Flags    = PLX_BAR_FLAG_IO;
        }
        else
        {
            pDevice->PciBar[BarIndex].Physical = PciBar & ~0xF;
            pDevice->PciBar[BarIndex].Size     = (~(Size & ~0xF)) + 1;
            pDevice->PciBar[BarIndex].Flags    = PLX_BAR_FLAG_MEM;
        }

        // Set BAR flags
        if (pDevice->PciBar[BarIndex].Flags & PLX_BAR_FLAG_MEM)
        {
            if ((PciBar & (3 << 1)) == 0)
            {
                pDevice->PciBar[BarIndex].Flags |= PLX_BAR_FLAG_32_BIT;
            }
            else if ((PciBar & (3 << 1)) == 1)
            {
                pDevice->PciBar[BarIndex].Flags |= PLX_BAR_FLAG_BELOW_1MB;
            }
            else if ((PciBar & (3 << 1)) == 2)
            {
                pDevice->PciBar[BarIndex].Flags |= PLX_BAR_FLAG_64_BIT;
            }

            if (PciBar & (1 << 3))
            {
                pDevice->PciBar[BarIndex].Flags |= PLX_BAR_FLAG_PREFETCHABLE;
            }
        }
    }

    // Return BAR properties
    *pBarProperties = pDevice->PciBar[BarIndex];

    // Display BAR properties if enabled
    if (pDevice->PciBar[BarIndex].Size == 0)
    {
        DebugPrintf(("BAR %d is disabled\n", BarIndex));
        return PLX_STATUS_OK;
    }

    DebugPrintf((
        "    PCI BAR %d: %08lX\n",
        BarIndex, (PLX_UINT_PTR)pDevice->PciBar[BarIndex].BarValue
        ));

    DebugPrintf((
        "    Phys Addr: %08lX\n",
        (PLX_UINT_PTR)pDevice->PciBar[BarIndex].Physical
        ));

    DebugPrintf((
        "    Size     : %08lX (%ld %s)\n",
        (PLX_UINT_PTR)pDevice->PciBar[BarIndex].Size,
        (PLX_UINT_PTR)pDevice->PciBar[BarIndex].Size > ((U64)1 << 30) ?
          (PLX_UINT_PTR)pDevice->PciBar[BarIndex].Size >> 30 :
          (PLX_UINT_PTR)pDevice->PciBar[BarIndex].Size > ((U64)1 << 20) ?
          (PLX_UINT_PTR)pDevice->PciBar[BarIndex].Size >> 20 :
          (PLX_UINT_PTR)pDevice->PciBar[BarIndex].Size > ((U64)1 << 10) ?
          (PLX_UINT_PTR)pDevice->PciBar[BarIndex].Size >> 10 :
          (PLX_UINT_PTR)pDevice->PciBar[BarIndex].Size,
        (PLX_UINT_PTR)pDevice->PciBar[BarIndex].Size > ((U64)1 << 30) ? "GB" :
          (PLX_UINT_PTR)pDevice->PciBar[BarIndex].Size > ((U64)1 << 20) ? "MB" :
          (PLX_UINT_PTR)pDevice->PciBar[BarIndex].Size > ((U64)1 << 20) ? "KB" : "B"
        ));

    DebugPrintf((
        "    Property : %sPrefetchable %d-bit\n",
        (pDevice->PciBar[BarIndex].Flags & PLX_BAR_FLAG_PREFETCHABLE) ? "" : "Non-",
        (pDevice->PciBar[BarIndex].Flags & PLX_BAR_FLAG_64_BIT) ? 64 : 32
        ));

    return PLX_STATUS_OK;
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
        case 0x2300:
        case 0x3300:
        case 0x8500:
        case 0x8600:
        case 0x8700:
        case 0x9700:
            return Plx8000_EepromPresent(
                pDevice,
                (U8*)pStatus
                );
    }

    return PLX_STATUS_UNSUPPORTED;
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
    switch (pDevice->Key.PlxChip & 0xFF00)
    {
        case 0x2300:
        case 0x3300:
        case 0x8600:
        case 0x8700:
        case 0x9700:
            TempChip = pDevice->Key.PlxChip & 0xFF00;
            break;

        default:
            TempChip = pDevice->Key.PlxChip;
            break;
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

        case 0x2300:
        case 0x3300:
        case 0x8505:
        case 0x8509:
        case 0x8525:
        case 0x8533:
        case 0x8547:
        case 0x8548:
        case 0x8600:
        case 0x8700:
        case 0x9700:
            OffsetProbe = 0x10;     // No CRC, just use any reasonable address
            break;

        case 0:
        default:
            DebugPrintf((
                "ERROR - Not a supported PLX device (%04X)\n",
                pDevice->Key.PlxChip
                ));
            return PLX_STATUS_UNSUPPORTED;
    }

    DebugPrintf(("Probe EEPROM at offset %02xh\n", OffsetProbe));

    // Get the current value
    status =
        PlxI2c_EepromReadByOffset(
            pDevice,
            OffsetProbe,
            &ValueOriginal
            );

    if (status != PLX_STATUS_OK)
    {
        return status;
    }

    // Prepare inverse value to write
    ValueWrite = ~(ValueOriginal);

    // Write inverse of original value
    status =
        PlxI2c_EepromWriteByOffset(
            pDevice,
            OffsetProbe,
            ValueWrite
            );

    if (status != PLX_STATUS_OK)
    {
        return status;
    }

    // Read updated value
    status =
        PlxI2c_EepromReadByOffset(
            pDevice,
            OffsetProbe,
            &ValueRead
            );

    if (status != PLX_STATUS_OK)
    {
        return status;
    }

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

    return PLX_STATUS_OK;
}




/*******************************************************************************
 *
 * Function   :  PlxI2c_EepromGetAddressWidth
 *
 * Description:  Returns the current EEPROM address width
 *
 ******************************************************************************/
PLX_STATUS
PlxI2c_EepromGetAddressWidth(
    PLX_DEVICE_OBJECT *pDevice,
    U8                *pWidth
    )
{
    PLX_STATUS status;


    switch (pDevice->Key.PlxChip & 0xFF00)
    {
        case 0x2300:
        case 0x3300:
        case 0x8500:
        case 0x8600:
        case 0x8700:
        case 0x9700:
            status =
                Plx8000_EepromGetAddressWidth(
                    pDevice,
                    pWidth
                    );
            break;

        default:
            DebugPrintf((
                "ERROR - Chip (%04X) does not support address width\n",
                pDevice->Key.PlxChip
                ));
            return PLX_STATUS_UNSUPPORTED;
    }

    DebugPrintf(("EEPROM address width = %dB\n", *pWidth));
    return status;
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
            return PLX_STATUS_INVALID_DATA;
    }

    switch (pDevice->Key.PlxChip & 0xFF00)
    {
        case 0x2300:
        case 0x3300:
        case 0x8600:
        case 0x8700:
        case 0x9700:
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
            return PLX_STATUS_UNSUPPORTED;
    }

    if (status == PLX_STATUS_OK)
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
    *pCrc       = 0;
    *pCrcStatus = PLX_CRC_UNSUPPORTED;

    switch (pDevice->Key.PlxChip & 0xFF00)
    {
        case 0x8500:
        case 0x8700:
        case 0x9700:
            return Plx8000_EepromCrcGet(
                pDevice,
                pCrc,
                pCrcStatus
                );
    }

    // CRC not supported
    return PLX_STATUS_UNSUPPORTED;
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

    switch (pDevice->Key.PlxChip & 0xFF00)
    {
        case 0x8500:
        case 0x8700:
        case 0x9700:
            return Plx8000_EepromCrcUpdate(
                pDevice,
                pCrc,
                bUpdateEeprom
                );
    }

    // CRC not supported
    return PLX_STATUS_UNSUPPORTED;
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
    U32                offset,
    U32               *pValue
    )
{
    // Set default return value
    *pValue = 0;

    // Make sure offset is aligned on 32-bit boundary
    if (offset & (3 << 0))
    {
        return PLX_STATUS_INVALID_OFFSET;
    }

    switch (pDevice->Key.PlxChip & 0xFF00)
    {
        case 0x2300:
        case 0x3300:
        case 0x8500:
        case 0x8600:
        case 0x8700:
        case 0x9700:
            return Plx8000_EepromReadByOffset(
                pDevice,
                offset,
                pValue
                );
    }

    return PLX_STATUS_UNSUPPORTED;
}




/*******************************************************************************
 *
 * Function   :  PlxI2c_EepromWriteByOffset
8 *
 * Description:  Write a 32-bit value to the EEPROM at a specified offset
 *
 ******************************************************************************/
PLX_STATUS
PlxI2c_EepromWriteByOffset(
    PLX_DEVICE_OBJECT *pDevice,
    U32                offset,
    U32                value
    )
{
    // Make sure offset is aligned on 32-bit boundary
    if (offset & (3 << 0))
        return PLX_STATUS_INVALID_OFFSET;

    switch (pDevice->Key.PlxChip & 0xFF00)
    {
        case 0x2300:
        case 0x3300:
        case 0x8500:
        case 0x8600:
        case 0x8700:
        case 0x9700:
            return Plx8000_EepromWriteByOffset(
                pDevice,
                offset,
                value
                );
    }

    return PLX_STATUS_UNSUPPORTED;
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
    U32                offset,
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
        return PLX_STATUS_INVALID_OFFSET;
    }

    switch (pDevice->Key.PlxChip & 0xFF00)
    {
        case 0x2300:
        case 0x3300:
        case 0x8500:
        case 0x8600:
        case 0x8700:
        case 0x9700:
            /******************************************
             * For devices that do not support 16-bit
             * EEPROM accesses, use 32-bit access
             *****************************************/

            // Get 32-bit value
            status =
                PlxI2c_EepromReadByOffset(
                    pDevice,
                    (offset & ~0x3),
                    &Value_32
                    );

            if (status != PLX_STATUS_OK)
            {
                return status;
            }

            // Return desired 16-bit portion
            if (offset & 0x3)
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
            return PLX_STATUS_UNSUPPORTED;
    }

    return PLX_STATUS_OK;
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
    U32                offset,
    U16                value
    )
{
    U32        Value_32;
    PLX_STATUS status;


    // Make sure offset is aligned on 16-bit boundary
    if (offset & (1 << 0))
        return PLX_STATUS_INVALID_OFFSET;

    switch (pDevice->Key.PlxChip & 0xFF00)
    {
        case 0x2300:
        case 0x3300:
        case 0x8500:
        case 0x8600:
        case 0x8700:
        case 0x9700:
            /******************************************
             * For devices that do not support 16-bit
             * EEPROM accesses, use 32-bit access
             *****************************************/

            // Get current 32-bit value
            status =
                PlxI2c_EepromReadByOffset(
                    pDevice,
                    (offset & ~0x3),
                    &Value_32
                    );

            if (status != PLX_STATUS_OK)
            {
                return status;
            }

            // Insert new 16-bit value in correct location
            if (offset & 0x3)
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
                (offset & ~0x3),
                Value_32
                );

        default:
            DebugPrintf((
                "ERROR - Device (%04X) does not support 16-bit EEPROM access\n",
                pDevice->Key.PlxChip
                ));
            return PLX_STATUS_UNSUPPORTED;
    }

    return PLX_STATUS_OK;
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
    U8            StnPortCount;
    PLX_PORT_PROP PortProp;


    // Clear performance object
    RtlZeroMemory( pPerfProp, sizeof(PLX_PERF_PROP) );

    // Verify supported device & set number of ports per station
    switch (pDevice->Key.PlxFamily)
    {
        case PLX_FAMILY_DENEB:
        case PLX_FAMILY_CYGNUS:
        case PLX_FAMILY_MIRA:
        case PLX_FAMILY_CAPELLA_1:
        case PLX_FAMILY_CAPELLA_2:
            StnPortCount = 4;
            break;

        case PLX_FAMILY_SIRIUS:
            StnPortCount = 16;
            break;

        case PLX_FAMILY_DRACO_1:
        case PLX_FAMILY_DRACO_2:
            StnPortCount = 8;    // Device actually only uses 6 ports out of 8
            break;

        default:
            DebugPrintf(("ERROR - Unsupported PLX chip (%04X)\n", pDevice->Key.PlxChip));
            return PLX_STATUS_UNSUPPORTED;
    }

    // Get port properties
    PlxI2c_GetPortProperties(
        pDevice,
        &PortProp
        );

    if (PortProp.PortNumber >= PERF_MAX_PORTS)
    {
        DebugPrintf(("ERROR - Port number exceeds maximum (%d)\n", (PERF_MAX_PORTS-1)));
        return PLX_STATUS_UNSUPPORTED;
    }

    // Store PLX chip family
    pPerfProp->PlxFamily = pDevice->Key.PlxFamily;

    // Store relevant port properties for later calculations
    pPerfProp->PortNumber = PortProp.PortNumber;
    pPerfProp->LinkWidth  = PortProp.LinkWidth;
    pPerfProp->LinkSpeed  = PortProp.LinkSpeed;

    // Determine station and port number within station
    pPerfProp->Station     = (U8)(PortProp.PortNumber / StnPortCount);
    pPerfProp->StationPort = (U8)(PortProp.PortNumber % StnPortCount);

    return PLX_STATUS_OK;
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
    U8  bStationBased;
    U8  bEgressAllPorts;
    U32 i;
    U32 offset;
    U32 RegValue;
    U32 RegCommand;
    U32 StnCount;
    U32 StnPortCount;
    U32 Offset_Control;


    // Verify supported device
    switch (pDevice->Key.PlxFamily)
    {
        case PLX_FAMILY_DENEB:
        case PLX_FAMILY_SIRIUS:
        case PLX_FAMILY_MIRA:
            Offset_Control = 0x568;

            // Offset changes for MIRA in legacy EP mode
            if ((pDevice->Key.PlxFamily == PLX_FAMILY_MIRA) &&
                (pDevice->Key.DeviceMode == PLX_CHIP_MODE_LEGACY_ADAPTER))
            {
                Offset_Control = 0x1568;
            }
            break;

        case PLX_FAMILY_CYGNUS:
        case PLX_FAMILY_DRACO_1:
        case PLX_FAMILY_DRACO_2:
        case PLX_FAMILY_CAPELLA_1:
        case PLX_FAMILY_CAPELLA_2:
            Offset_Control = 0x3E0;
            break;

        default:
            DebugPrintf(("ERROR - Unsupported PLX chip (%04X)\n", pDevice->Key.PlxChip));
            return PLX_STATUS_UNSUPPORTED;
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
            return PLX_STATUS_INVALID_DATA;
    }

    // Added to avoid compiler warning
    Bit_EgressEn = 0;
    StnCount     = 0;
    StnPortCount = 0;

    // Default to single station access
    bStationBased   = FALSE;
    bEgressAllPorts = FALSE;

    // Set control offset & enable/disable counters in stations
    switch (pDevice->Key.PlxFamily)
    {
        case PLX_FAMILY_MIRA:
            /*************************************************************
             * For MIRA, there are filters available to control counting
             * of different packet types. The PLX API doesn't currently
             * support this filtering so all are enabled.
             *
             * PCIe filters in 664h[29:20] of P0
             *   20: Disable MWr 32 TLP counter
             *   21: Disable MWr 64 TLP counter
             *   22: Disable Msg TLP counter
             *   23: Disable MRd 32 TLP counter
             *   24: Disable MRd 64 TLP counter
             *   25: Disable other NP TLP counter
             *   26: Disable ACK DLLP counting
             *   27: Disable Update-FC P DLLP counter
             *   28: Disable Update-FC NP DLLP counter
             *   29: Disable Update-FC CPL DLLP counter
             ************************************************************/
            // In MIRA legacy EP mode, PCIe registers start at 1000h
            if (pDevice->Key.DeviceMode == PLX_CHIP_MODE_LEGACY_ADAPTER)
            {
                offset = 0x1664;
            }
            else
            {
                offset = 0x664;
            }

            // Clear 664[29:20] to enable all counters
            RegValue = PLX_8000_REG_READ( pDevice, offset );
            PLX_8000_REG_WRITE( pDevice, offset, RegValue & ~(0x3FF << 20) );
            break;

        case PLX_FAMILY_CYGNUS:
        case PLX_FAMILY_DRACO_1:
        case PLX_FAMILY_DRACO_2:
        case PLX_FAMILY_CAPELLA_1:
        case PLX_FAMILY_CAPELLA_2:
            // Set device configuration
            if (pDevice->Key.PlxFamily == PLX_FAMILY_CYGNUS)
            {
                Bit_EgressEn = 7;
                StnCount     = 6;
                StnPortCount = 4;
            }
            else if ((pDevice->Key.PlxFamily == PLX_FAMILY_DRACO_1) ||
                     (pDevice->Key.PlxFamily == PLX_FAMILY_DRACO_2))
            {
                Bit_EgressEn = 6;
                StnCount     = 3;
                StnPortCount = 8;    // Device actually only uses 6 ports out of 8

                // Set 3F0[9:8] to disable probe mode interval timer
                // & avoid RAM pointer corruption
                if (command == PLX_PERF_CMD_START)
                {
                    RegValue = PLX_8000_REG_READ( pDevice, 0x3F0 );
                    PLX_8000_REG_WRITE( pDevice, 0x3F0, RegValue | (3 << 8) );
                }
            }
            else if ((pDevice->Key.PlxFamily == PLX_FAMILY_CAPELLA_1) ||
                     (pDevice->Key.PlxFamily == PLX_FAMILY_CAPELLA_2))
            {
                Bit_EgressEn    = 6;
                StnCount        = 6;
                StnPortCount    = 4;
                bStationBased   = TRUE;
                bEgressAllPorts = TRUE;
            }

            // Enable/Disable Performance Counter in each station (or ports if applicable)
            for (i = 0; i < (StnCount * StnPortCount); i++)
            {
                // Set port base offset
                offset = (i * 0x1000);

                // Ingress ports (in station port 0 only)
                if ((i % StnPortCount) == 0)
                {
                    RegValue = PLX_8000_REG_READ( pDevice, offset + 0x768 );

                    if (command == PLX_PERF_CMD_START)
                    {
                        PLX_8000_REG_WRITE( pDevice, offset + 0x768, RegValue | (1 << 29) );
                    }
                    else
                    {
                        PLX_8000_REG_WRITE( pDevice, offset + 0x768, RegValue & ~(1 << 29) );
                    }
                }

                // Egress ports
                if (bEgressAllPorts || ((i % StnPortCount) == 0))
                {
                    RegValue = PLX_8000_REG_READ( pDevice, offset + 0xF30 );

                    if (command == PLX_PERF_CMD_START)
                    {
                        PLX_8000_REG_WRITE( pDevice, offset + 0xF30, RegValue | (1 << Bit_EgressEn) );
                    }
                    else
                    {
                        PLX_8000_REG_WRITE( pDevice, offset + 0xF30, RegValue & ~(1 << Bit_EgressEn) );
                    }
                }
            }
            break;
    }

    // Update monitor
    for (i = 0; i < StnCount; i++)
    {
        if ((i == 0) || bStationBased)
        {
            PLX_8000_REG_WRITE(
                pDevice,
                Offset_Control + (i * StnPortCount * 0x1000),
                RegCommand
                );
        }
    }

    return PLX_STATUS_OK;
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
    U32 i;
    U32 StnCount;
    U32 StnPortCount;
    U16 Offset_Control;


    // Verify supported device
    switch (pDevice->Key.PlxFamily)
    {
        case PLX_FAMILY_DENEB:
        case PLX_FAMILY_SIRIUS:
        case PLX_FAMILY_MIRA:
            Offset_Control = 0x568;

            // Offset changes for MIRA in legacy EP mode
            if ((pDevice->Key.PlxFamily == PLX_FAMILY_MIRA) &&
                (pDevice->Key.DeviceMode == PLX_CHIP_MODE_LEGACY_ADAPTER))
            {
                Offset_Control = 0x1568;
            }
            break;

        case PLX_FAMILY_CYGNUS:
        case PLX_FAMILY_DRACO_1:
        case PLX_FAMILY_DRACO_2:
        case PLX_FAMILY_CAPELLA_1:
        case PLX_FAMILY_CAPELLA_2:
            Offset_Control = 0x3E0;
            break;

        default:
            DebugPrintf(("ERROR - Unsupported PLX chip (%04X)\n", pDevice->Key.PlxChip));
            return PLX_STATUS_UNSUPPORTED;
    }

    // Set station counts for station-based monitor chips
    if ((pDevice->Key.PlxFamily == PLX_FAMILY_CAPELLA_1) ||
        (pDevice->Key.PlxFamily == PLX_FAMILY_CAPELLA_2))
    {
        StnCount     = 6;
        StnPortCount = 4;
    }
    else
    {
        StnCount     = 1;
        StnPortCount = 0;
    }

    for (i = 0; i < StnCount; i++)
    {
        // Reset (30) & enable monitor (31) & infinite sampling (28) & start (27)
        PLX_8000_REG_WRITE(
            pDevice,
            Offset_Control + (i * StnPortCount * 0x1000),
            (1 << 31) | (1 << 30) | (1 << 28) | (1 << 27)
            );
    }

    return PLX_STATUS_OK;
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
 *   PLD   = USB endpoint payload count
 *   RAW   = USB endpoint raw byte count
 *   PKT   = USB endpoint packet count
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
 *          |-----------------|        |/\/\/\/\/\/\/\/\/|        |/\/\/\/\/\/\/\/\/\|
 *        28| Port 2 IN PH    |        |       :         |        |       :          |
 *          |       :         |        |       :         |        |       :          |
 *          |       :         |        |       :         |        |       :          |
 *        38| Port 2 IN CPLDW |        |       :         |        |       :          |
 *          |-----------------|        |       :         |        |       :          |
 *        3C| Port 3 IN PH    |        |/\/\/\/\/\/\/\/\/|        |/\/\/\/\/\/\/\/\/\|
 *          |       :         |      64| Port 5 IN PH    |     12C| Port 15 IN PH    |
 *          |       :         |        |       :         |        |       :          |
 *        4C| Port 3 IN CPLDW |        |       :         |        |       :          |
 *          |-----------------|      74| Port 5 IN CPLDW |     13C| Port 15 IN CPLDW |
 *        50| Port 0 EG PH    |        |-----------------|        |------------------|
 *        54| Port 0 EG PDW   |      78| Port 0 EG PH    |     140| Port 0 EG PH     |
 *        58| Port 0 EG NPDW  |      7C| Port 0 EG PDW   |     144| Port 0 EG PDW    |
 *        5C| Port 0 EG CPLH  |      80| Port 0 EG NPDW  |     148| Port 0 EG NPDW   |
 *        60| Port 0 EG CPLDW |      84| Port 0 EG CPLH  |     14C| Port 0 EG CPLH   |
 *          |-----------------|      88| Port 0 EG CPLDW |     150| Port 0 EG CPLDW  |
 *        64| Port 1 EG PH    |        |-----------------|        |------------------|
 *          |       :         |      8C| Port 1 EG PH    |     154| Port 1 EG PH     |
 *          |       :         |        |       :         |        |       :          |
 *        74| Port 1 EG CPLDW |        |       :         |        |       :          |
 *          |-----------------|      9C| Port 1 EG CPLDW |     164| Port 1 EG CPLDW  |
 *        78| Port 2 EG PH    |        |/\/\/\/\/\/\/\/\/|        |/\/\/\/\/\/\/\/\/\|
 *          |       :         |        |       :         |        |       :          |
 *          |       :         |        |       :         |        |       :          |
 *        88| Port 2 EG CPLDW |        |       :         |        |       :          |
 *          |-----------------|        |       :         |        |       :          |
 *        8C| Port 3 EG PH    |        |       :         |        |       :          |
 *          |       :         |        |/\/\/\/\/\/\/\/\/|        |/\/\/\/\/\/\/\/\/\|
 *          |       :         |      DC| Port 5 EG PH    |     26C| Port 15 EG PH    |
 *        9C| Port 3 EG CPLDW |        |       :         |        |       :          |
 *          |-----------------|        |       :         |        |       :          |
 *        A0| Port 0 IN DLLP  |      EC| Port 5 EG CPLDW |     27C| Port 15 EG CPLDW |
 *        A4| Port 1 IN DLLP  |        |-----------------|        |------------------|
 *        A8| Port 2 IN DLLP  |      F0| Port 0 IN DLLP  |     280| Port  0 IN DLLP  |
 *        AC| Port 3 IN DLLP  |      F4| Port 1 IN DLLP  |     284| Port  2 IN DLLP  |
 *          |-----------------|      F8| Port 2 IN DLLP  |     288| Port  4 IN DLLP  |
 *        B0| Port 0 EG DLLP  |      FC| Port 3 IN DLLP  |     28C| Port  6 IN DLLP  |
 *        B4| Port 1 EG DLLP  |     100| Port 4 IN DLLP  |     290| Port  8 IN DLLP  |
 *        B8| Port 2 EG DLLP  |     104| Port 5 IN DLLP  |     294| Port 10 IN DLLP  |
 *        BC| Port 3 EG DLLP  |        |-----------------|     298| Port 12 IN DLLP  |
 *          |-----------------|     108| Port 0 EG DLLP  |     29C| Port 14 IN DLLP  |
 *        C0| Port 0 IN PHY   |     10C| Port 1 EG DLLP  |        |------------------|
 *        C4| Port 1 IN PHY   |     110| Port 2 EG DLLP  |     2A0| Port  0 EG DLLP  |
 *        C8| Port 2 IN PHY   |     114| Port 3 EG DLLP  |     2A4| Port  2 EG DLLP  |
 *        CC| Port 3 IN PHY   |     118| Port 4 EG DLLP  |     2A8| Port  4 EG DLLP  |
 *          |-----------------|     11C| Port 5 EG DLLP  |     2AC| Port  6 EG DLLP  |
 *        D0| Port 0 EG PHY   |        |-----------------|     2B0| Port  8 EG DLLP  |
 *        D4| Port 1 EG PHY   |     120| Port 0 IN PHY   |     2B4| Port 10 EG DLLP  |
 *        D8| Port 2 EG PHY   |     124| Port 1 IN PHY   |     2B8| Port 12 EG DLLP  |
 *        DC| Port 3 EG PHY   |     128| Port 2 IN PHY   |     2BC| Port 14 EG DLLP  |
 *           -----------------      12C| Port 3 IN PHY   |        |------------------|
 *                                  130| Port 4 IN PHY   |     2C0| Port  1 IN DLLP  |
 *                                  134| Port 5 IN PHY   |     2C4| Port  3 IN DLLP  |
 *                                     |-----------------|     2C8| Port  5 IN DLLP  |
 *                                  138| Port 0 EG PHY   |     2CC| Port  7 IN DLLP  |
 *                                  13C| Port 1 EG PHY   |     2D0| Port  9 IN DLLP  |
 *                                  140| Port 2 EG PHY   |     2D4| Port 11 IN DLLP  |
 *                                  144| Port 3 EG PHY   |     2D8| Port 13 IN DLLP  |
 *                                  148| Port 4 EG PHY   |     2DC| Port 15 IN DLLP  |
 *                                  14C| Port 5 EG PHY   |        |------------------|
 *                                      -----------------      2E0| Port  1 EG DLLP  |
 *                                                             2E4| Port  3 EG DLLP  |
 *                                                             2E8| Port  5 EG DLLP  |
 *                                                             2EC| Port  7 EG DLLP  |
 *                                                             2F0| Port  9 EG DLLP  |
 *                                                             2F4| Port 11 EG DLLP  |
 *                                                             2F8| Port 13 EG DLLP  |
 *                                                             2FC| Port 15 EG DLLP  |
 *                                                                |------------------|
 *                                                             300| Port 0 PHY       |
 *                                                                |       :          |
 *                                                                |       :          |
 *                                                             33C| Port 15 PHY      |
 *                                                                 ------------------
 *
 *             Mira                        Capella-1/2
 *     --------------------------    -----------------------
 *        14 PCIe counters/port      14 counters/port
 *         4 ports/station            4 pts/stn but 5 in RAM
 *         1 stations (4 ports)       6 stn (24 ports)
 *                                  *SW uses 30 pts to read all
 *        86 counters/station
 *        86 counters (86 * 1)        70 counters/station
 *                                   420 counters (70 * 6)
 *
 *       off     Counter             off     Counter
 *           -----------------           -----------------
 *         0| Port 0 IN PH    |        0| Port 0 IN PH    |
 *         4| Port 0 IN PDW   |        4| Port 0 IN PDW   |
 *         8| Port 0 IN NPDW  |        8| Port 0 IN NPDW  |
 *         C| Port 0 IN CPLH  |        C| Port 0 IN CPLH  |
 *        10| Port 0 IN CPLDW |       10| Port 0 IN CPLDW |
 *          |-----------------|         |-----------------|
 *        14| Port 1 IN PH    |       14| Port 1 IN PH    |
 *          |       :         |         |       :         |
 *          |       :         |         |       :         |
 *        24| Port 1 IN CPLDW |       24| Port 1 IN CPLDW |
 *          |-----------------|         |/\/\/\/\/\/\/\/\/|
 *        28| Port 2 IN PH    |         |       :         |
 *          |       :         |         |       :         |
 *          |       :         |         |       :         |
 *        38| Port 2 IN CPLDW |         |       :         |
 *          |-----------------|         |       :         |
 *        3C| Port 3 IN PH    |         |/\/\/\/\/\/\/\/\/|
 *          |       :         |       50| Port 4 IN PH    |
 *          |       :         |         |       :         |
 *        4C| Port 3 IN CPLDW |         |       :         |
 *          |-----------------|       60| Port 4 IN CPLDW |
 *        50| Port 0 EG PH    |         |-----------------|
 *        54| Port 0 EG PDW   |       64| Port 0 EG PH    |
 *        58| Port 0 EG NPDW  |       68| Port 0 EG PDW   |
 *        5C| Port 0 EG CPLH  |       6c| Port 0 EG NPDW  |
 *        60| Port 0 EG CPLDW |       70| Port 0 EG CPLH  |
 *          |-----------------|       74| Port 0 EG CPLDW |
 *        64| Port 1 EG PH    |         |-----------------|
 *          |       :         |       78| Port 1 EG PH    |
 *          |       :         |         |       :         |
 *        74| Port 1 EG CPLDW |         |       :         |
 *          |-----------------|       88| Port 1 EG CPLDW |
 *        78| Port 2 EG PH    |         |/\/\/\/\/\/\/\/\/|
 *          |       :         |         |       :         |
 *          |       :         |         |       :         |
 *        88| Port 2 EG CPLDW |         |       :         |
 *          |-----------------|         |       :         |
 *        8C| Port 3 EG PH    |         |       :         |
 *          |       :         |         |/\/\/\/\/\/\/\/\/|
 *          |       :         |         |-----------------|
 *        9C| Port 3 EG CPLDW |       B4| Port 4 EG PH    |
 *          |-----------------|         |       :         |
 *        A0| Port 0 IN DLLP  |         |       :         |
 *        A4| Port 1 IN DLLP  |       C4| Port 4 EG CPLDW |
 *        A8| Port 2 IN DLLP  |         |-----------------|
 *        AC| Port 3 IN DLLP  |       C8| Port 0 IN DLLP  |
 *          |-----------------|       CC| Port 1 IN DLLP  |
 *        B0| Port 0 EG DLLP  |       D0| Port 2 IN DLLP  |
 *        B4| Port 1 EG DLLP  |       D4| Port 3 IN DLLP  |
 *        B8| Port 2 EG DLLP  |       D8| Port 4 IN DLLP  |
 *        BC| Port 3 EG DLLP  |         |-----------------|
 *          |-----------------|       DC| -- Invalid --   |
 *        C0| GPEP_0 IN PLD   |       E0| Port 0 EG DLLP  |
 *        C4| GPEP_0 IN RAW   |       E4| Port 1 EG DLLP  |
 *        C8| GPEP_0 IN PKT   |       E8| Port 2 EG DLLP  |
 *          |-----------------|       EC| Port 3 EG DLLP  |*P4 EG DLLP missing
 *        CC| GPEP_1 IN PLD   |         |-----------------|
 *        D0| GPEP_1 IN RAW   |       F0| Port 0 IN PHY   |
 *        D4| GPEP_1 IN PKT   |       F4| Port 1 IN PHY   |
 *          |-----------------|       F8| Port 2 IN PHY   |
 *        D8| GPEP_2 IN PLD   |       FC| Port 3 IN PHY   |
 *        DC| GPEP_2 IN RAW   |      100| Port 4 IN PHY   |
 *        E0| GPEP_2 IN PKT   |         |-----------------|
 *          |-----------------|      104| Port 0 EG PHY   |
 *        E4| GPEP_3 IN PLD   |      108| Port 1 EG PHY   |
 *        E8| GPEP_3 IN RAW   |      10C| Port 2 EG PHY   |
 *        EC| GPEP_3 IN PKT   |      110| Port 3 EG PHY   |
 *          |-----------------|      114| Port 4 EG PHY   |
 *        F0| GPEP_0 OUT PLD  |          -----------------
 *        F4| GPEP_0 OUT RAW  |
 *        F8| GPEP_0 OUT PKT  |
 *          |-----------------|
 *        FC| GPEP_1 OUT PLD  |
 *       100| GPEP_1 OUT RAW  |
 *       104| GPEP_1 OUT PKT  |
 *          |-----------------|
 *       108| GPEP_2 OUT PLD  |
 *       10C| GPEP_2 OUT RAW  |
 *       110| GPEP_2 OUT PKT  |
 *          |-----------------|
 *       114| GPEP_3 OUT PLD  |
 *       118| GPEP_3 OUT RAW  |
 *       11C| GPEP_3 OUT PKT  |
 *          |-----------------|
 *       120| EP_0 IN PLD     |
 *       124| EP_0 IN RAW     |
 *       128| EP_0 IN PKT     |
 *          |-----------------|
 *       12C| EP_0 OUT PLD    |
 *       130| EP_0 OUT RAW    |
 *       134| EP_0 OUT PKT    |
 *          |-----------------|
 *       138| PHY (always 0)  |
 *       13C| PHY (always 0)  |
 *           -----------------
 *
 ******************************************************************************/
PLX_STATUS
PlxI2c_PerformanceGetCounters(
    PLX_DEVICE_OBJECT *pDevice,
    PLX_PERF_PROP     *pPerfProps,
    U8                 NumOfObjects
    )
{
    U8   NumCounters;
    U8   CurrStation;
    U8   StnCount;
    U8   StnPortCount;
    U8   bStationBased;
    U8   RamStnPortCount;
    U16  i;
    U16  index;
    U16  IndexBase;
    U32  Offset_Fifo;
    U32  Offset_RamCtrl;
    U32  RegValue;
    U32 *pCounters;
    U32 *pCounter;
    U32 *pCounter_Prev;
    U32  Counter_PrevTmp[14];
    S64  TmpValue;


    // Default to single station access
    bStationBased = FALSE;

    // Assume station port count in RAM is identical to station port count
    RamStnPortCount = 0;

    // Setup parameters for reading counters
    switch (pDevice->Key.PlxFamily)
    {
        case PLX_FAMILY_DENEB:
            Offset_RamCtrl = 0x618;
            Offset_Fifo    = 0x628;
            NumCounters    = 14;
            StnCount       = 3;
            StnPortCount   = 4;
            break;

        case PLX_FAMILY_SIRIUS:
            Offset_RamCtrl = 0x618;
            Offset_Fifo    = 0x628;
            NumCounters    = 13;
            StnCount       = 1;
            StnPortCount   = 16;
            break;

        case PLX_FAMILY_CYGNUS:
            Offset_RamCtrl = 0x3F0;
            Offset_Fifo    = 0x3E4;
            NumCounters    = 14;
            StnCount       = 6;
            StnPortCount   = 4;
            break;

        case PLX_FAMILY_MIRA:
            Offset_RamCtrl = 0x618;
            Offset_Fifo    = 0x628;
            NumCounters    = 12;
            StnCount       = 1;
            StnPortCount   = 4;

            // In MIRA legacy EP mode, PCIe registers start at 1000h
            if (pDevice->Key.DeviceMode == PLX_CHIP_MODE_LEGACY_ADAPTER)
            {
                Offset_RamCtrl += 0x1000;
                Offset_Fifo    += 0x1000;
            }
            break;

        case PLX_FAMILY_DRACO_1:
        case PLX_FAMILY_DRACO_2:
            Offset_RamCtrl = 0x3F0;
            Offset_Fifo    = 0x3E4;
            NumCounters    = 14;
            StnCount       = 3;
            StnPortCount   = 6;
            break;

        case PLX_FAMILY_CAPELLA_1:
        case PLX_FAMILY_CAPELLA_2:
            Offset_RamCtrl  = 0x3F0;
            Offset_Fifo     = 0x3E4;
            NumCounters     = 14;
            StnCount        = 6;
            StnPortCount    = 4;
            RamStnPortCount = 5;
            bStationBased   = TRUE;
            break;

        default:
            DebugPrintf(("ERROR - Unsupported PLX chip (%04X)\n", pDevice->Key.PlxChip));
            return PLX_STATUS_UNSUPPORTED;
    }

    // Allocate buffer to contain counter data for all ports (Max = 14 counters/port)
    pCounters = malloc( PERF_MAX_PORTS * PERF_COUNTERS_PER_PORT * sizeof(U32) );
    if (pCounters == NULL)
    {
        return PLX_STATUS_INSUFFICIENT_RES;
    }

    // Set RAM station port count if matches station port count
    if (RamStnPortCount == 0)
    {
        RamStnPortCount = StnPortCount;
    }

    // RAM control
    RegValue = (2 << 4) |   // Capture type ([5:4])
               (1 << 2) |   // Reset read pointer
               (1 << 0);    // Enable RAM

    // Reset RAM read pointer
    for (i = 0; i < StnCount; i++)
    {
        if ((i == 0) || bStationBased)
        {
            PLX_8000_REG_WRITE(
                pDevice,
                Offset_RamCtrl + (i * StnPortCount * 0x1000),
                RegValue
                );
        }
    }

    // Read in all counters
    i           = 0;
    CurrStation = 0;
    while (i < (StnCount * RamStnPortCount * NumCounters))
    {
        // Check if reached station boundary
        if ((i % (NumCounters * RamStnPortCount)) == 0)
        {
            DebugPrintf_Cont(("\n"));
            if (i == 0)
            {
                DebugPrintf(("           Counters\n"));
                DebugPrintf(("----------------------------------\n"));
            }
            else
            {
                // Increment to next station
                CurrStation++;

                // For station based counters use register in station port 0
                if (bStationBased)
                {
                    Offset_Fifo += (StnPortCount * 0x1000);
                }
            }

            DebugPrintf(("Station %d:\n", CurrStation));
            DebugPrintf(("%03X:", (U16)(i * sizeof(U32))));
        }
        else if ((i % 4) == 0)
        {
            DebugPrintf_Cont(("\n"));
            DebugPrintf(("%03X:", (U16)(i * sizeof(U32))));
        }

        // Get next counter
        pCounters[i] = PLX_8000_REG_READ( pDevice, Offset_Fifo );
        DebugPrintf_Cont((" %08X", pCounters[i]));

        // Jump to next counter
        i++;
    }
    DebugPrintf_Cont(("\n"));

    // Assign counter values to enabled ports
    i = 0;
    while (i < NumOfObjects)
    {
        // Make a copy of the previous values before overwriting them
        RtlCopyMemory(
            Counter_PrevTmp,
            &(pPerfProps[i].Prev_IngressPostedHeader),
            PERF_COUNTERS_PER_PORT * sizeof(U32)    // All 14 counters in structure
            );

        // Save current values to previous
        RtlCopyMemory(
            &(pPerfProps[i].Prev_IngressPostedHeader),
            &(pPerfProps[i].IngressPostedHeader),
            PERF_COUNTERS_PER_PORT * sizeof(U32)    // All 14 counters in structure
            );

        // Calculate starting index for counters based on port in station
        IndexBase = pPerfProps[i].Station * (NumCounters * RamStnPortCount);

        // Ingress counters start at index 0 from base
        index = IndexBase + 0 + (pPerfProps[i].StationPort * 5);

        // Get Ingress counters (5 DW/port)
        pPerfProps[i].IngressPostedHeader = pCounters[index + 0];
        pPerfProps[i].IngressPostedDW     = pCounters[index + 1];
        pPerfProps[i].IngressNonpostedDW  = pCounters[index + 2];
        pPerfProps[i].IngressCplHeader    = pCounters[index + 3];
        pPerfProps[i].IngressCplDW        = pCounters[index + 4];

        // Egress counters start after ingress
        index = IndexBase + (5 * RamStnPortCount) + (pPerfProps[i].StationPort * 5);

        // Get Egress counters (5 DW/port)
        pPerfProps[i].EgressPostedHeader = pCounters[index + 0];
        pPerfProps[i].EgressPostedDW     = pCounters[index + 1];
        pPerfProps[i].EgressNonpostedDW  = pCounters[index + 2];
        pPerfProps[i].EgressCplHeader    = pCounters[index + 3];
        pPerfProps[i].EgressCplDW        = pCounters[index + 4];

        // DLLP Ingress counters start after egress
        index = IndexBase + (10 * RamStnPortCount);

        // DLLP counter location depends upon chip
        if (pDevice->Key.PlxFamily == PLX_FAMILY_SIRIUS)
        {
            // Even port number DLLP counters are first
            index += (pPerfProps[i].StationPort / 2);

            // Odd port number DLLP counters follow even ports
            if (pPerfProps[i].StationPort & (1 << 0))
            {
                index += RamStnPortCount;
            }
        }
        else
        {
            index += pPerfProps[i].StationPort;
        }

        // Get DLLP Ingress counters (1 DW/port)
        pPerfProps[i].IngressDllp = pCounters[index];

        // Egress DLLP counters follow Ingress
        if (pDevice->Key.PlxFamily == PLX_FAMILY_SIRIUS)
        {
            // Even ports are grouped together
            index += (RamStnPortCount / 2);
        }
        else
        {
            index += RamStnPortCount;
        }

        // For Capella, Egress DLLP skips one offset & port 4 is lost
        if ((pDevice->Key.PlxFamily == PLX_FAMILY_CAPELLA_1) ||
            (pDevice->Key.PlxFamily == PLX_FAMILY_CAPELLA_2))
        {
            index++;
        }

        // Get DLLP Egress counters (1 DW/port)
        pPerfProps[i].EgressDllp = pCounters[index];

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
            for (index = 0; index < PERF_COUNTERS_PER_PORT; index++)
            {
                if (((*pCounter == 0) && (*pCounter_Prev != 0)) || (*pCounter == 0x4C041301))
                {
                    // Store 64-bit counter in case of wrapping
                    TmpValue = *pCounter_Prev;
                    if (*pCounter_Prev < Counter_PrevTmp[index])
                    {
                        TmpValue += ((S64)1 << 32);
                    }

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

    // Release temporary buffers
    free( pCounters );

    return PLX_STATUS_OK;
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
    pMHProp->SwitchMode = PLX_CHIP_MODE_STANDARD;

    // Verify supported device
    switch (pDevice->Key.PlxFamily)
    {
        case PLX_FAMILY_CYGNUS:
        case PLX_FAMILY_DRACO_1:
        case PLX_FAMILY_DRACO_2:
        case PLX_FAMILY_CAPELLA_1:
        case PLX_FAMILY_CAPELLA_2:
            break;

        default:
            DebugPrintf(("ERROR - Device (%04X) doesn't support VS mode\n", pDevice->Key.PlxChip));
            return PLX_STATUS_UNSUPPORTED;
    }

    // Attempt to read management port configuration
    RegValue = PLX_8000_REG_READ( pDevice, 0x354 );

    // Get active VS mask
    RegVSEnable = PLX_8000_REG_READ( pDevice, 0x358 );

    // Device properties are only available from the management port
    if ((RegValue == 0) && ((RegVSEnable & ~(1 << 0)) == 0))
    {
        // In VS mode, but not management port
        pMHProp->SwitchMode = PLX_CHIP_MODE_VIRT_SW;
        DebugPrintf(("Device is in VS mode, but not management port\n"));
        return PLX_STATUS_OK;
    }

    // Report this is management port regardless of mode
    pMHProp->bIsMgmtPort = TRUE;

    // Store management port info
    pMHProp->MgmtPortNumActive    = (U8)((RegValue >> 0) & 0x1F);
    pMHProp->MgmtPortNumRedundant = (U8)((RegValue >> 8) & 0x1F);

    // Determine which management ports are active
    if (RegValue & (1 << 5))
    {
        pMHProp->bMgmtPortActiveEn = TRUE;
    }

    if (RegValue & (1 << 13))
    {
        pMHProp->bMgmtPortRedundantEn = TRUE;
    }

    // Provide active VS's
    pMHProp->VS_EnabledMask = (U16)RegVSEnable;

    TotalVS = 0;

    // Count number of active virtual switches
    for (i = 0; i < 8; i++)
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
        pMHProp->SwitchMode = PLX_CHIP_MODE_VIRT_SW;

        DebugPrintf((
            "\n"
            "    Mode        : Virtual Switch\n"
            "    Enabled VS  : %04X\n"
            "    Active Mgmt : %d (%s)\n"
            "    Backup Mgmt : %d (%s)\n"
            "    VS UP-DS pts: 0:%02d-%08X 1:%02d-%08X 2:%02d-%08X 3:%02d-%08X\n"
            "                  4:%02d-%08X 5:%02d-%08X 6:%02d-%08X 7:%02d-%08X\n",
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

    return PLX_STATUS_OK;
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
    status = PlxI2c_MH_GetProperties( pDevice, &MHProp );
    if (status != PLX_STATUS_OK)
    {
        return status;
    }

    // Operation only available from VS management port
    if ((MHProp.SwitchMode != PLX_CHIP_MODE_VIRT_SW) || (MHProp.bIsMgmtPort == FALSE))
    {
        return PLX_STATUS_UNSUPPORTED;
    }

    DebugPrintf((
        "Migrate DS ports (%08X) from VS%d ==> VS%d %s\n",
        (int)DsPortMask, VS_Source, VS_Dest,
        (bResetSrc) ? "& reset source port" : ""
        ));

    if ((VS_Source >= 8) || (VS_Dest >= 8))
    {
        DebugPrintf(("ERROR - Source or Dest VS are not valid\n"));
        return PLX_STATUS_INVALID_DATA;
    }

    // Verify source VS is enabled
    if ((MHProp.VS_EnabledMask & (1 << VS_Source)) == 0)
    {
        DebugPrintf(("ERROR - Source VS (%d) not enabled\n", VS_Source));
        return PLX_STATUS_DISABLED;
    }

    // Verify DS ports to move currently owned by source port
    if ((MHProp.VS_DownstreamPorts[VS_Source] & DsPortMask) != DsPortMask)
    {
        DebugPrintf(("ERROR - One or more DS ports not owned by source VS\n"));
        return PLX_STATUS_INVALID_DATA;
    }

    // Migrate DS ports
    for (i = 0; i < PERF_MAX_PORTS; i++)
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
        DebugPrintf(("Enable destination VS%d\n", VS_Dest));
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
        Plx_sleep( 10 );

        // Take VS out of reset
        PLX_8000_REG_WRITE(
            pDevice,
            0x3A0,
            RegValue & ~((U32)1 << VS_Source)
            );
    }

    return PLX_STATUS_OK;
}




/***********************************************************
 *
 *               PRIVATE I2C SUPPORT FUNCTIONS
 *
 **********************************************************/


/******************************************************************
 *
 * Function   :  Plx_delay_us
 *
 * Description:  Delay for specific number of microseconds
 *
 *****************************************************************/
VOID
Plx_delay_us(
    U32 Time_us
    )
{
#if defined(PLX_LINUX) || defined(PLX_DOS)

    usleep( Time_us );

#elif defined(PLX_MSWINDOWS)

    BOOL     status;
    LONGLONG Tick_End;
    LONGLONG Tick_Start;
    LONGLONG TicksToWait;
    LONGLONG TicksPerSecond;


    // Get current counter
    status = QueryPerformanceCounter( (LARGE_INTEGER*)&Tick_Start );

    if (status == FALSE)
    {
        // If system doesn't support HRT, revert to sleep in ms
        Plx_sleep( Time_us / 1000 );
        return;
    }

    // Get the high resolution counter's accuracy
    QueryPerformanceFrequency( (LARGE_INTEGER*)&TicksPerSecond );

    // Calculate number of ticks to wait based on µs delay time
    TicksToWait = ((U64)Time_us * TicksPerSecond) / (1000 * 1000);

    do
    {
        QueryPerformanceCounter( (LARGE_INTEGER*)&Tick_End );
    }
    while ((Tick_End - Tick_Start) < TicksToWait);

#endif
}




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
    U8  bMultiNt;
    U8  Bit_Mode;
    U8  Bit_StnSel;
    U8  Bit_PortSel;
    U8  PortType;
    U8  PortsPerStn;
    U32 StnSel;
    U32 PortSel;
    U32 Mode;
    U32 Command;
    U32 Offset_Base;            // Provided offset port base
    U32 Offset_NTV0Base;        // NT 0 Virtual regs base offset
    U32 Offset_DmaBase;         // DMA regs base offset
    U32 Offset_Stn0Base;        // Station 0 regs base offset


    // If port-based, add port base address to offset
    if (bAdjustForPort)
    {
        offset += pDevice->Key.ApiInternal[1];
    }

    // Determine port base offset
    Offset_Base = offset & ~(U32)0xFFF;

    // Default special port base addresses to an always invalid address
    Offset_NTV0Base = 0xE0000000;
    Offset_DmaBase  = 0xE0000000;
    Offset_Stn0Base = 0xE0000000;

    // Added to avoid compiler warning
    Bit_Mode = 0;

    // Set default bit positions
    Bit_StnSel  = 0;
    Bit_PortSel = 15;

    // Set whether multi-NT is supported
    if ((pDevice->Key.PlxChip & 0xFF00) == 0x8600)
    {
        bMultiNt = FALSE;
    }
    else
    {
        bMultiNt = TRUE;
    }

    // Set bit positions & address traps
    switch (pDevice->Key.PlxFamily)
    {
        case PLX_FAMILY_VEGA_LITE:
            Bit_Mode        = 18;
            Offset_NTV0Base = 0x10000;
            break;

        case PLX_FAMILY_ALTAIR:
        case PLX_FAMILY_ALTAIR_XL:
        case PLX_FAMILY_MIRA:
            // No NT port
            break;

        case PLX_FAMILY_DENEB:
        case PLX_FAMILY_SIRIUS:
            Bit_Mode        = 19;
            Bit_StnSel      = 17;
            Offset_NTV0Base = 0x10000;
            Offset_DmaBase  = 0x20000;
            break;

        case PLX_FAMILY_CYGNUS:
            Bit_Mode        = 20;
            Bit_StnSel      = 17;
            Offset_NTV0Base = 0x3E000;
            break;

        case PLX_FAMILY_SCOUT:
        case PLX_FAMILY_DRACO_1:
        case PLX_FAMILY_DRACO_2:
            Bit_Mode        = 20;
            Bit_StnSel      = 18;
            Offset_NTV0Base = 0x3E000;
            Offset_DmaBase  = 0x20000;
            break;

        case PLX_FAMILY_CAPELLA_1:
        case PLX_FAMILY_CAPELLA_2:
            Bit_Mode        = 21;
            Bit_StnSel      = 18;
            Offset_NTV0Base = 0x3E000;
            Offset_Stn0Base = 0x80000;
            break;

        default:
            // Port will not have family set during initial probe
            Bit_Mode   = 0;
            Bit_StnSel = 0;
            break;
    }

    Mode    = 0;
    StnSel  = 0;
    PortSel = 0;

    // Default to transparent port
    PortType = 0;

    // Trap offsets for special ports
    if (Offset_Base == (Offset_NTV0Base + 0x0))
    {
        PortType = PLX_FLAG_PORT_NT_VIRTUAL_0;          // NTV 0
    }
    else if (Offset_Base == (Offset_NTV0Base + 0x1000))
    {
        PortType = PLX_FLAG_PORT_NT_LINK_0;             // NTL 0
    }
    else if (bMultiNt && (Offset_Base == (Offset_NTV0Base - 0x2000)))
    {
        PortType = PLX_FLAG_PORT_NT_VIRTUAL_1;          // NTV 1
    }
    else if (bMultiNt && (Offset_Base == (Offset_NTV0Base - 0x1000)))
    {
        PortType = PLX_FLAG_PORT_NT_LINK_1;             // NTL 1
    }
    else if (Offset_Base == (Offset_DmaBase + 0x0))
    {
        PortType = PLX_FLAG_PORT_DMA_RAM;               // DMA RAM
    }
    else if (Offset_Base == (Offset_DmaBase + 0x1000))
    {
        PortType = PLX_FLAG_PORT_DMA_0;                 // DMA 0
    }
    else if (Offset_Base == (Offset_DmaBase + 0x2000))
    {
        PortType = PLX_FLAG_PORT_DMA_1;                 // DMA 1
    }
    else if (Offset_Base == (Offset_DmaBase + 0x3000))
    {
        PortType = PLX_FLAG_PORT_DMA_2;                 // DMA 2
    }
    else if (Offset_Base == (Offset_DmaBase + 0x4000))
    {
        PortType = PLX_FLAG_PORT_DMA_3;                 // DMA 3
    }
    else if (Offset_Base == (Offset_Stn0Base + 0x0))
    {
        PortType = PLX_FLAG_PORT_STN_REGS_S0;           // Station 0
    }
    else if (Offset_Base == (Offset_Stn0Base + 0x10000))
    {
        PortType = PLX_FLAG_PORT_STN_REGS_S1;           // Station 1
    }
    else if (Offset_Base == (Offset_Stn0Base + 0x20000))
    {
        PortType = PLX_FLAG_PORT_STN_REGS_S2;           // Station 2
    }
    else if (Offset_Base == (Offset_Stn0Base + 0x30000))
    {
        PortType = PLX_FLAG_PORT_STN_REGS_S3;           // Station 3
    }
    else if (Offset_Base == (Offset_Stn0Base + 0x40000))
    {
        PortType = PLX_FLAG_PORT_STN_REGS_S4;           // Station 4
    }
    else if (Offset_Base == (Offset_Stn0Base + 0x50000))
    {
        PortType = PLX_FLAG_PORT_STN_REGS_S5;           // Station 5
    }
    else if ((offset >> 12) > PLX_FLAG_PORT_MAX)
    {
        // Fabric mode: registers other than standard port use full addressing
        if (pDevice->Key.DeviceMode == PLX_CHIP_MODE_FABRIC)
        {
            PortType = PLX_FLAG_PORT_STN_REGS_S0;
        }
        else
        {
            ErrorPrintf(("ERROR: Port type %d not implemented\n", (offset >> 12)));
        }
    }
    else
    {
        PortSel = (offset >> 12);
    }

    // Handle chips where NT Virtual is accessed as transparent port
    if (PortType == PLX_FLAG_PORT_NT_VIRTUAL_0)
    {
        if ((pDevice->Key.PlxFamily == PLX_FAMILY_VEGA_LITE) ||
            (pDevice->Key.PlxFamily == PLX_FAMILY_DENEB)     ||
            (pDevice->Key.PlxFamily == PLX_FAMILY_SIRIUS)    ||
            (pDevice->Key.PlxFamily == PLX_FAMILY_CYGNUS))
        {
            // For Cygnus, only applies to legacy NT mode
            if ((pDevice->Key.PlxFamily != PLX_FAMILY_CYGNUS) ||
                (pDevice->Key.DeviceMode == PLX_CHIP_MODE_STD_LEGACY_NT))
            {
                // Revert back to standard port
                PortType = 0;

                // Select NT port
                PortSel = pDevice->Key.NTPortNum;
            }
        }
    }

    // Determine values
    switch (pDevice->Key.PlxFamily)
    {
        case PLX_FAMILY_VEGA_LITE:
            // Check if NT-Link port or within NT-Virtual port
            if ((PortType == PLX_FLAG_PORT_NT_LINK_0) ||
                (PortType == PLX_FLAG_PORT_NT_VIRTUAL_0) ||
                (Offset_Base == ((U32)pDevice->Key.NTPortNum * 0x1000)))
            {
                if (PortType == PLX_FLAG_PORT_NT_LINK_0)
                {
                    Mode = 1;
                }

                // Bypass offsets F8h or FCh of NT port to avoid persistent I2C failure
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

        case PLX_FAMILY_MIRA:
            if (PortType == PLX_FLAG_PORT_PCIE_TO_USB)
            {
                /*****************************************
                 * For MIRA special case, if the USB EP is
                 * selected & offset 4xxxh is accessed, the
                 * expectation is to access the USB device
                 * registers which are at offset 4000h.
                 ****************************************/
                if (Offset_Base == 0x4000)
                {
                    PortSel = 4;
                }
                else
                {
                    PortSel = 3;
                }
            }
            else if (PortType == PLX_FLAG_PORT_USB)
            {
                PortSel = 4;
            }
            break;

        case PLX_FAMILY_DENEB:
            if (PortType == PLX_FLAG_PORT_NT_LINK_0)
            {
                Mode = 1;
            }
            break;

        case PLX_FAMILY_SIRIUS:
            // Default to special mode
            Mode = 1;

            if (PortType == PLX_FLAG_PORT_NT_LINK_0)
            {
                PortSel = 0x10;
            }
            else if (PortType == PLX_FLAG_PORT_NT_DS_P2P)
            {
                PortSel = 0x11;
            }
            else if (PortType == PLX_FLAG_PORT_DMA_0)
            {
                PortSel = 0x12;
            }
            else if (PortType == PLX_FLAG_PORT_DMA_RAM)
            {
                PortSel = 0x13;
            }
            else
            {
                Mode = 0;   // Revert to transparent mode
            }
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
        case PLX_FAMILY_CAPELLA_1:
        case PLX_FAMILY_CAPELLA_2:
            // Set ports per station
            if ((pDevice->Key.PlxFamily == PLX_FAMILY_SCOUT)   ||
                (pDevice->Key.PlxFamily == PLX_FAMILY_DRACO_1) ||
                (pDevice->Key.PlxFamily == PLX_FAMILY_DRACO_2))
            {
                PortsPerStn = 8;    // Only 6 ports per stn, but numbered as 8
            }
            else if ((pDevice->Key.PlxFamily == PLX_FAMILY_CAPELLA_1) ||
                     (pDevice->Key.PlxFamily == PLX_FAMILY_CAPELLA_2))
            {
                PortsPerStn = 4;
            }
            else
            {
                ErrorPrintf(("ERRROR: Ports/Station not set for %04X\n", pDevice->Key.PlxChip));
                return PLX_I2C_CMD_ERROR;
            }

            // For Draco 1, some register cause problems if accessed
            if (pDevice->Key.PlxFamily == PLX_FAMILY_DRACO_1)
            {
                if ((offset == 0x856C)  || (offset == 0x8570) ||
                    (offset == 0x1056C) || (offset == 0x10570))
                {
                    return PLX_I2C_CMD_SKIP;
                }
            }

            // For Draco 2, reads of ALUT registers cause problems if accessed
            if ((pDevice->Key.PlxFamily == PLX_FAMILY_DRACO_2) &&
                (I2cOperation == PLX_I2C_CMD_REG_READ))
            {
                if ((PortType == PLX_FLAG_PORT_ALUT_0) ||
                    (PortType == PLX_FLAG_PORT_ALUT_1) ||
                    (PortType == PLX_FLAG_PORT_ALUT_2) ||
                    (PortType == PLX_FLAG_PORT_ALUT_3))
                {
                    DebugPrintf(("I2C - ALUT read causes issues, skipping\n"));
                    return PLX_I2C_CMD_SKIP;
                }
            }

            // GEP will show up as NTV0, so must use full address
            if ((pDevice->Key.DeviceMode == PLX_CHIP_MODE_FABRIC) &&
                (PortType == PLX_FLAG_PORT_NT_VIRTUAL_0))
            {
                offset   = Offset_NTV0Base | (offset & 0xFFF);
                PortType = PLX_FLAG_PORT_STN_REGS_S0;
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
            else if (PortType == PLX_FLAG_PORT_ALUT_0)
            {
                Mode    = 3;    // 11b
                StnSel  = 2;    // 010b
                PortSel = 0;    // ALUT RAM 0
            }
            else if (PortType == PLX_FLAG_PORT_ALUT_1)
            {
                Mode    = 3;    // 11b
                StnSel  = 2;    // 010b
                PortSel = 1;    // ALUT RAM 1
            }
            else if (PortType == PLX_FLAG_PORT_ALUT_2)
            {
                Mode    = 3;    // 11b
                StnSel  = 2;    // 010b
                PortSel = 2;    // ALUT RAM 2
            }
            else if (PortType == PLX_FLAG_PORT_ALUT_3)
            {
                Mode    = 3;    // 11b
                StnSel  = 2;    // 010b
                PortSel = 3;    // ALUT RAM 3
            }
            else if ((PortType == PLX_FLAG_PORT_STN_REGS_S0) ||
                     (PortType == PLX_FLAG_PORT_STN_REGS_S1) ||
                     (PortType == PLX_FLAG_PORT_STN_REGS_S2) ||
                     (PortType == PLX_FLAG_PORT_STN_REGS_S3) ||
                     (PortType == PLX_FLAG_PORT_STN_REGS_S4) ||
                     (PortType == PLX_FLAG_PORT_STN_REGS_S5))
            {
                Mode    = 2;                    // 10b
                StnSel  = (offset >> 15) & 0x7; // offset [17:15]
                PortSel = (offset >> 12) & 0x7; // offset [14:12]
            }
            else
            {
                StnSel  = PortSel / PortsPerStn;    // Station number
                PortSel = PortSel % PortsPerStn;    // Port in station
            }
            break;

        case 0:
            // Family not set yet but still need command during initial probe
            break;

        default:
            ErrorPrintf(("ERRROR: Mode/PortSel/StnSel not set for %04X\n", pDevice->Key.PlxChip));
            return PLX_I2C_CMD_ERROR;
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
    Aardvark    hI2cDev;
    AardvarkExt AaExt;


    // If mode properties supplied, copy into device object
    if (pModeProp != NULL)
    {
        pDevice->Key.ApiMode        = PLX_API_MODE_I2C_AARDVARK;
        pDevice->Key.ApiIndex       = (U8)pModeProp->I2c.I2cPort;
        pDevice->Key.ApiInternal[0] = pModeProp->I2c.ClockRate;
    }

    // Clear handle in case of failure
    pDevice->hDevice = 0;

    // Verify port doesn't exceed limit
    if (pDevice->Key.ApiIndex >= PLX_I2C_MAX_DEVICES)
    {
        return FALSE;
    }

    // Initialize global structure if haven't yet
    if (Gbl_bInitialized == FALSE)
    {
        Gbl_bInitialized = TRUE;
        RtlZeroMemory( Gbl_PlxI2cProp, sizeof(Gbl_PlxI2cProp) );
    }

    // Check if device is already opened by the PLX API
    if (Gbl_PlxI2cProp[pDevice->Key.ApiIndex].hDevice == 0)
    {
        // Open I2C device
        hI2cDev = aa_open_ext( pDevice->Key.ApiIndex, &AaExt );

        // Log version info in case of issues
        DebugPrintf(("Aardvark Versions:\n"));
        DebugPrintf((
            "  Curr: SW=%d.%02d FW=%d.%02d API=%d.%02d HW=%d.%02d\n",
            AaExt.version.software >> 8, AaExt.version.software & 0xFF,
            AaExt.version.firmware >> 8, AaExt.version.firmware & 0xFF,
            AA_HEADER_VERSION      >> 8, AA_HEADER_VERSION      & 0xFF,
            AaExt.version.hardware >> 8, AaExt.version.hardware & 0xFF
            ));
        DebugPrintf((
            "  Req : SW=%d.%02d FW=%d.%02d API=%d.%02d\n",
            AaExt.version.sw_req_by_fw  >> 8, AaExt.version.sw_req_by_fw  & 0xFF,
            AaExt.version.fw_req_by_sw  >> 8, AaExt.version.fw_req_by_sw  & 0xFF,
            AaExt.version.api_req_by_sw >> 8, AaExt.version.api_req_by_sw & 0xFF
            ));

        if (hI2cDev <= 0)
        {
            DebugPrintf((
                "ERROR: Unable to open device %d (status=%d)\n",
                pDevice->Key.ApiIndex, hI2cDev
                ));
            return FALSE;
        }

        pDevice->hDevice = (PLX_DRIVER_HANDLE)(PLX_UINT_PTR)hI2cDev;

        // Setup the Aardvark adapter's power pins
        aa_target_power( (Aardvark)PLX_PTR_TO_INT( pDevice->hDevice ), AA_TARGET_POWER_BOTH );

        // Make sure slave mode is disabled
        aa_i2c_slave_disable( (Aardvark)PLX_PTR_TO_INT( pDevice->hDevice ) );

        // Set I2C bus speed
        aa_i2c_bitrate( (Aardvark)PLX_PTR_TO_INT( pDevice->hDevice ), pDevice->Key.ApiInternal[0]);

        // Store the handle for re-use if necessary
        Gbl_PlxI2cProp[pDevice->Key.ApiIndex].hDevice = pDevice->hDevice;

        // Reset upstream port bus number
        Gbl_PlxI2cProp[pDevice->Key.ApiIndex].UpstreamBus = PCI_FIELD_IGNORE;

        // Reset NT port number
        memset( Gbl_PlxI2cProp[pDevice->Key.ApiIndex].NTPortNum, PCI_FIELD_IGNORE, PLX_I2C_MAX_NT_PORTS );

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
                    TRUE,       // Adjust for port?
                    TRUE        // Retry on error?
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
                    TRUE        // Adjust for port?
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
                    TRUE,       // Adjust for port?
                    TRUE        // Retry on error?
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
                    TRUE        // Adjust for port?
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
                    FALSE,      // Adjust for port?
                    TRUE        // Retry on error?
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
                    FALSE       // Adjust for port?
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

        case PLX_IOCTL_EEPROM_GET_ADDRESS_WIDTH:
            DebugPrintf_Cont(("PLX_IOCTL_EEPROM_FET_ADDRESS_WIDTH\n"));

            pIoBuffer->ReturnCode =
                PlxI2c_EepromGetAddressWidth(
                    pDevice,
                    (U8*)&pIoBuffer->value[0]
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
                    (U32)pIoBuffer->value[0],
                    (U32*)&(pIoBuffer->value[1])
                    );

            DebugPrintf((
                "EEPROM Offset %02X = %08lX\n",
                (U32)pIoBuffer->value[0],
                (U32)pIoBuffer->value[1]
                ));
            break;

        case PLX_IOCTL_EEPROM_WRITE_BY_OFFSET:
            DebugPrintf_Cont(("PLX_IOCTL_EEPROM_WRITE_BY_OFFSET\n"));

            pIoBuffer->ReturnCode =
                PlxI2c_EepromWriteByOffset(
                    pDevice,
                    (U32)pIoBuffer->value[0],
                    (U32)pIoBuffer->value[1]
                    );

            DebugPrintf((
                "Wrote %08lX to EEPROM Offset %02X\n",
                (U32)pIoBuffer->value[1],
                (U32)pIoBuffer->value[0]
                ));
            break;

        case PLX_IOCTL_EEPROM_READ_BY_OFFSET_16:
            DebugPrintf_Cont(("PLX_IOCTL_EEPROM_READ_BY_OFFSET_16\n"));

            pIoBuffer->ReturnCode =
                PlxI2c_EepromReadByOffset_16(
                    pDevice,
                    (U32)pIoBuffer->value[0],
                    (U16*)&(pIoBuffer->value[1])
                    );

            DebugPrintf((
                "EEPROM Offset %02X = %04X\n",
                (U32)pIoBuffer->value[0],
                (U16)pIoBuffer->value[1]
                ));
            break;

        case PLX_IOCTL_EEPROM_WRITE_BY_OFFSET_16:
            DebugPrintf_Cont(("PLX_IOCTL_EEPROM_WRITE_BY_OFFSET_16\n"));

            pIoBuffer->ReturnCode =
                PlxI2c_EepromWriteByOffset_16(
                    pDevice,
                    (U32)pIoBuffer->value[0],
                    (U16)pIoBuffer->value[1]
                    );

            DebugPrintf((
                "Wrote %04X to EEPROM Offset %02X\n",
                (U16)pIoBuffer->value[1],
                (U32)pIoBuffer->value[0]
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

            pIoBuffer->ReturnCode = PLX_STATUS_UNSUPPORTED;
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
    U16                DeviceNumber,
    U16               *pNumMatched
    )
{
    U8            Port;
    U8            Port_Upstream;
    U8            PciHeaderType;
    U8            NtMask;
    U8            StnPort;
    U8            PortsPerStn;
    U16           PlxChip;
    U16           DeviceCount;
    U32           offset;
    U32           Offset_Upstream;
    U32           Offset_NTV0Base;
    U32           Offset_DebugCtrl;
    U32           DevVenID;
    U32           RegValue;
    U32           RegDebugCtrl;
    U32           RegPciHeader;
    U64           PossiblePorts;
    BOOLEAN       bCompareDevice;
    BOOLEAN       bMatchId;
    BOOLEAN       bMatchLoc;
    PLX_STATUS    status;
    PLX_PORT_PROP PortProp;


    DeviceCount   = 0;
    PortsPerStn   = 0;
    RegDebugCtrl  = (U32)-1;
    Port_Upstream = (U8)-1;

    // Reset upstream bus number & NT port numbers in case chips are daisy-chained
    Gbl_PlxI2cProp[pDevice->Key.ApiIndex].UpstreamBus = PCI_FIELD_IGNORE;
    memset( Gbl_PlxI2cProp[pDevice->Key.ApiIndex].NTPortNum, PCI_FIELD_IGNORE, PLX_I2C_MAX_NT_PORTS );

    // Always reset key but save I2C properties
    Port     = pDevice->Key.ApiIndex;
    RegValue = pDevice->Key.DeviceNumber;

    RtlZeroMemory( &pDevice->Key, sizeof(PLX_DEVICE_KEY) );
    pDevice->Key.ApiIndex     = Port;
    pDevice->Key.DeviceNumber = (U16)RegValue;
    pDevice->Key.ApiMode      = PLX_API_MODE_I2C_AARDVARK;

    // Start with port 0
    Port = 0;

    // Start with PLX chip unknown
    PlxChip = 0;

    // Start with empty Device ID
    DevVenID = 0;

    // Default to NT disabled
    NtMask          = 0;
    Offset_NTV0Base = 0x3E000;

    // Initially allow access only to port 0
    PossiblePorts = (1 << 0);

    // Probe all possible ports in the switch
    while (Port < (sizeof(PossiblePorts) * 8))
    {
        // Default to not compare device
        bCompareDevice = FALSE;

        // Default to unknown port type
        pDevice->Key.PlxPortType = PLX_SPEC_PORT_UNKNOWN;

        // Store the port
        pDevice->Key.PlxPort = Port;

        // Set default offset
        offset = 0x0;

        // Verify port and set offset
        if (Port == PLX_FLAG_PORT_NT_VIRTUAL_0)         // NT 0 Virtual port
        {
            pDevice->Key.ApiInternal[1] = Offset_NTV0Base + 0x0;
        }
        else if (Port == PLX_FLAG_PORT_NT_LINK_0)       // NT 0 Link port
        {
            pDevice->Key.ApiInternal[1] = Offset_NTV0Base + 0x1000;
        }
        else if (Port == PLX_FLAG_PORT_NT_VIRTUAL_1)    // NT 1 Virtual port
        {
            pDevice->Key.ApiInternal[1] = Offset_NTV0Base - 0x2000;
        }
        else if (Port == PLX_FLAG_PORT_NT_LINK_1)       // NT 1 Link port
        {
            pDevice->Key.ApiInternal[1] = Offset_NTV0Base - 0x1000;
        }
        else if (Port == PLX_FLAG_PORT_NT_DS_P2P)       // NT Virtual or Downstream Parent port
        {
            pDevice->Key.ApiInternal[1] = (U32)-1;
        }
        else if (Port == PLX_FLAG_PORT_DMA_0)           // DMA functions
        {
            pDevice->Key.ApiInternal[1] = 0x21000;
        }
        else if (Port == PLX_FLAG_PORT_DMA_1)
        {
            pDevice->Key.ApiInternal[1] = 0x22000;
        }
        else if (Port == PLX_FLAG_PORT_DMA_2)
        {
            pDevice->Key.ApiInternal[1] = 0x23000;
        }
        else if (Port == PLX_FLAG_PORT_DMA_3)
        {
            pDevice->Key.ApiInternal[1] = 0x24000;
        }
        else if (Port == PLX_FLAG_PORT_DMA_RAM)
        {
            // Skip probe of DMA RAM since not a device
            PossiblePorts &= ~((U64)1 << Port);
        }
        else if ((Port == PLX_FLAG_PORT_ALUT_0) || (Port == PLX_FLAG_PORT_ALUT_1) ||
                 (Port == PLX_FLAG_PORT_ALUT_2) || (Port == PLX_FLAG_PORT_ALUT_3))
        {
            // Skip probe of ALUT since not a device
            PossiblePorts &= ~((U64)1 << Port);
        }
        else if ((Port == PLX_FLAG_PORT_STN_REGS_S0) || (Port == PLX_FLAG_PORT_STN_REGS_S1) ||
                 (Port == PLX_FLAG_PORT_STN_REGS_S2) || (Port == PLX_FLAG_PORT_STN_REGS_S3) ||
                 (Port == PLX_FLAG_PORT_STN_REGS_S4) || (Port == PLX_FLAG_PORT_STN_REGS_S5))
        {
            // Skip probe of VS/Fabric mode station-specific registers since not a device
            PossiblePorts &= ~((U64)1 << Port);
        }
        else
        {
            // Determine offset
            pDevice->Key.ApiInternal[1] = (U32)Port * 0x1000;
        }

        // Skip probe if port is known not to exist
        if ((PossiblePorts & ((U64)1 << Port)) == 0)
        {
            status = PLX_STATUS_FAILED;
        }
        else
        {
            // Get Device/Vendor ID
            DevVenID =
                PlxI2c_PlxRegisterRead(
                    pDevice,
                    0,
                    &status,
                    TRUE,       // Adjust for port?
                    FALSE       // Retry on error?
                    );
        }

        if ((status != PLX_STATUS_OK) || ((DevVenID & 0xFFFF) != PLX_VENDOR_ID))
        {
            goto _PlxI2c_ProbeSwitch_Next_Port;
        }

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
            {
                PlxChip = pDevice->Key.PlxChip;
            }

            // Get chip port mask
            PlxPci_ChipGetPortMask(
                pDevice->Key.PlxChip,
                pDevice->Key.PlxRevision,
                &PossiblePorts
                );

            // For first port, perform additional steps
            if (Port == 0)
            {
                switch (pDevice->Key.PlxFamily)
                {
                    case PLX_FAMILY_CYGNUS:
                    case PLX_FAMILY_SCOUT:
                    case PLX_FAMILY_DRACO_1:
                    case PLX_FAMILY_DRACO_2:
                    case PLX_FAMILY_CAPELLA_1:
                    case PLX_FAMILY_CAPELLA_2:
                        Offset_DebugCtrl = 0x350;
                        Offset_NTV0Base  = 0x3E000;
                        break;

                    case PLX_FAMILY_MIRA:
                        Offset_DebugCtrl = 0x574;
                        break;

                    case PLX_FAMILY_ALTAIR:
                    case PLX_FAMILY_ALTAIR_XL:
                    case PLX_FAMILY_VEGA:
                    case PLX_FAMILY_VEGA_LITE:
                    case PLX_FAMILY_DENEB:
                    case PLX_FAMILY_SIRIUS:
                        Offset_DebugCtrl = 0x1DC;
                        Offset_NTV0Base  = 0x10000;
                        break;

                    default:
                        ErrorPrintf((
                            "ERROR: Debug Control offset not set for %04X\n",
                            pDevice->Key.PlxChip
                            ));
                        return PLX_STATUS_UNSUPPORTED;
                }

                // Get port 0 debug control register
                RegDebugCtrl =
                    PlxI2c_PlxRegisterRead(
                        pDevice,
                        Offset_DebugCtrl,   // Port 0 debug control
                        &status,
                        FALSE,              // Adjust for port?
                        TRUE                // Retry on error?
                        );

                if ((status != PLX_STATUS_OK) || (RegDebugCtrl == (U32)-1))
                {
                    goto _PlxI2c_ProbeSwitch_Next_Port;
                }

                // Default to standard switch fan-out mode
                pDevice->Key.DeviceMode = PLX_CHIP_MODE_STANDARD;

                // Check if NT is enabled
                switch (pDevice->Key.PlxFamily)
                {
                    case PLX_FAMILY_CYGNUS:
                    case PLX_FAMILY_SCOUT:
                    case PLX_FAMILY_DRACO_1:
                    case PLX_FAMILY_DRACO_2:
                    case PLX_FAMILY_CAPELLA_1:
                    case PLX_FAMILY_CAPELLA_2:
                        // Probe for chip mode
                        RegValue =
                            PlxI2c_PlxRegisterRead(
                                pDevice,
                                0x358,      // VS0 Enable
                                NULL,
                                FALSE,      // Adjust for port?
                                TRUE        // Retry on error?
                                );

                        // 358[7:0]: 01h=Std else VS mode
                        if ((RegValue & 0xFF) != (1 << 0))
                        {
                            pDevice->Key.DeviceMode = PLX_CHIP_MODE_VIRT_SW;
                        }

                        // Check for fabric mode
                        if (pDevice->Key.PlxFamily == PLX_FAMILY_CAPELLA_2)
                        {
                            RegValue =
                                PlxI2c_PlxRegisterRead(
                                    pDevice,
                                    0x46C,      // Strap Configuration
                                    NULL,
                                    FALSE,      // Adjust for port?
                                    TRUE        // Retry on error?
                                    );

                            // Check if fabric mode (46C[15:13]=100b)
                            if (((RegValue >> 13) & 0x7) == 0x4)
                            {
                                pDevice->Key.DeviceMode = PLX_CHIP_MODE_FABRIC;

                                // For fabric mode, determine number of ports per station
                                RegValue =
                                    PlxI2c_PlxRegisterRead(
                                        pDevice,
                                        0x300,      // Port configuration
                                        NULL,
                                        FALSE,      // Adjust for port?
                                        TRUE        // Retry on error?
                                        );

                                // If station 0 is 5 ports, then only single station
                                if ((RegValue & 0x7) == 0x5)      // [2:0] = 5
                                {
                                    PortsPerStn = 5;              // 5 port x8x2x2x2x2x2 config
                                }
                                else
                                {
                                    PortsPerStn = 4;              // Other configs are 4 ports/station
                                }

                                DebugPrintf((
                                    "I2C - Chip config: %d station%s, %d ports/stn\n",
                                    (PortsPerStn == 5) ? 1 : 6,
                                    (PortsPerStn == 5) ? "" : "s",
                                    PortsPerStn
                                    ));
                            }
                        }

                        RegValue =
                            PlxI2c_PlxRegisterRead(
                                pDevice,
                                0x360,      // VS0 Upstream
                                NULL,
                                FALSE,      // Adjust for port?
                                TRUE        // Retry on error?
                                );

                        // Store upstream port number
                        Port_Upstream = (U8)((RegValue >> 0) & 0x1F);

                        // NT enable setting is not valid in fabric mode
                        if (pDevice->Key.DeviceMode != PLX_CHIP_MODE_FABRIC)
                        {
                            // Determine if NT0 is enabled & store port number
                            if (RegValue & (1 << 13))
                            {
                                Gbl_PlxI2cProp[pDevice->Key.ApiIndex].NTPortNum[0] = (U8)((RegValue >> 8) & 0xF);
                                NtMask |= (1 << 0);
                            }

                            // Determine if NT1 is enabled & store port number
                            if (RegValue & (1 << 21))
                            {
                                Gbl_PlxI2cProp[pDevice->Key.ApiIndex].NTPortNum[1] = (U8)((RegValue >> 16) & 0xF);
                                NtMask |= (1 << 1);
                            }

                            // Update chip mode
                            if (NtMask)
                            {
                                pDevice->Key.DeviceMode = PLX_CHIP_MODE_STD_NT_DS_P2P;

                                // Verify Cygnus in NT P2P mode (360[14]=1)
                                if ((pDevice->Key.PlxFamily == PLX_FAMILY_CYGNUS) &&
                                   ((RegDebugCtrl & (1 << 14)) == 0))
                                {
                                    pDevice->Key.DeviceMode = PLX_CHIP_MODE_STD_LEGACY_NT;
                                }
                            }
                        }
                        break;

                    case PLX_FAMILY_MIRA:
                        // Upstream port always 0 & no NT
                        Port_Upstream = 0;
                        NtMask        = 0;

                        // Check port 0 header type
                        RegPciHeader =
                            PlxI2c_PlxRegisterRead(
                                pDevice,
                                0xC,        // PCI Header Type
                                NULL,
                                FALSE,      // Adjust for port?
                                TRUE        // Retry on error?
                                );

                        // If port 0 is EP, device is in Legacy mode & skip remaining ports
                        if (((RegPciHeader >> 16) & 0x7F) == 0)
                        {
                            // Only enable port 0
                            PossiblePorts = (1 << 0);
                            pDevice->Key.DeviceMode = PLX_CHIP_MODE_LEGACY_ADAPTER;
                        }
                        break;

                    case PLX_FAMILY_ALTAIR:
                    case PLX_FAMILY_ALTAIR_XL:
                    case PLX_FAMILY_VEGA:
                    case PLX_FAMILY_VEGA_LITE:
                    case PLX_FAMILY_DENEB:
                    case PLX_FAMILY_SIRIUS:
                        // Store upstream port number ([11:8])
                        Port_Upstream = (U8)((RegDebugCtrl >> 8) & 0xF);

                        // Determine if NT is enabled & store port number
                        if (pDevice->Key.PlxFamily == PLX_FAMILY_VEGA_LITE)
                        {
                            // NT is enabled if NT dual-host or intelligent adapter mode
                            if ((((RegDebugCtrl >> 18) & 0x3) == 1) ||  // NT intelligent mode
                                (((RegDebugCtrl >> 18) & 0x3) == 2))    // NT Dual-host mode
                            {
                                NtMask = (1 << 0);
                            }
                        }
                        else if ((pDevice->Key.PlxFamily == PLX_FAMILY_ALTAIR) ||
                                 (pDevice->Key.PlxFamily == PLX_FAMILY_ALTAIR_XL))
                        {
                            // Altair doesn't support NT
                        }
                        else if (RegDebugCtrl & (1 << 18))
                        {
                            NtMask = (1 << 0);
                        }

                        // Update chip mode
                        if (NtMask)
                        {
                            // Store NT port number
                            Gbl_PlxI2cProp[pDevice->Key.ApiIndex].NTPortNum[0] = (U8)((RegDebugCtrl >> 24) & 0xF);

                            // Default to legacy NT
                            pDevice->Key.DeviceMode = PLX_CHIP_MODE_STD_LEGACY_NT;

                            // Check if Sirius in NT P2P mode (1DC[6]=1)
                            if (pDevice->Key.PlxFamily == PLX_FAMILY_SIRIUS)
                            {
                                // NT P2P mode (1DC[6])
                                if (RegDebugCtrl & (1 << 6))
                                {
                                    pDevice->Key.DeviceMode = PLX_CHIP_MODE_STD_NT_DS_P2P;
                                }

                                // For some chips, I2C bus timeout (1DC[14]) must be enabled or
                                //  accesses to a disabled port prevents further I2C accesses.
                                if ((RegDebugCtrl & (1 << 14)) == 0)
                                {
                                    RegDebugCtrl |= (1 << 14);

                                    PlxI2c_PlxRegisterWrite(
                                        pDevice,
                                        0x1DC,
                                        RegDebugCtrl,
                                        FALSE               // Adjust for port?
                                        );
                                }
                            }
                        }
                        break;

                    default:
                        ErrorPrintf((
                            "ERROR: Chip mode & UP/NT detection not implemented for %04X\n",
                            pDevice->Key.PlxChip
                            ));
                        return PLX_STATUS_UNSUPPORTED;
                }

                DebugPrintf((
                    "I2C - ChipMode=%s  UP=%d  NT0=%02Xh  NT1=%02Xh\n",
                    (pDevice->Key.DeviceMode == PLX_CHIP_MODE_STANDARD) ? "STD" :
                      (pDevice->Key.DeviceMode == PLX_CHIP_MODE_STD_LEGACY_NT) ? "STD NT" :
                      (pDevice->Key.DeviceMode == PLX_CHIP_MODE_STD_NT_DS_P2P) ? "NT P2P" :
                      (pDevice->Key.DeviceMode == PLX_CHIP_MODE_VIRT_SW) ? "VS" :
                      (pDevice->Key.DeviceMode == PLX_CHIP_MODE_FABRIC) ? "FABRIC" :
                      (pDevice->Key.DeviceMode == PLX_CHIP_MODE_LEGACY_ADAPTER) ? "LEGACY" :
                      "UNKOWN",
                    Port_Upstream,
                    NtMask & (1 << 0) ? Gbl_PlxI2cProp[pDevice->Key.ApiIndex].NTPortNum[0] : 0xFF,
                    NtMask & (1 << 1) ? Gbl_PlxI2cProp[pDevice->Key.ApiIndex].NTPortNum[1] : 0xFF
                    ));
            }

            // Remove disabled ports to avoid probing them
            // NOTE: Must be called after Key.DeviceMode is set since may refer to it
            PlxChipFilterDisabledPorts(
                pDevice,
                &PossiblePorts
                );

            // If not NT P2P mode, do not probe for NT P2P port
            if (pDevice->Key.DeviceMode != PLX_CHIP_MODE_STD_NT_DS_P2P)
            {
                PossiblePorts &= ~((U64)1 << PLX_FLAG_PORT_NT_DS_P2P);
            }

            // If NT not enabled, don't probe NT ports
            if (NtMask == 0)
            {
                PossiblePorts &=
                    ~(((U64)1 << PLX_FLAG_PORT_NT_LINK_0) |
                      ((U64)1 << PLX_FLAG_PORT_NT_LINK_1) |
                      ((U64)1 << PLX_FLAG_PORT_NT_VIRTUAL_0) |
                      ((U64)1 << PLX_FLAG_PORT_NT_VIRTUAL_1) |
                      ((U64)1 << PLX_FLAG_PORT_NT_DS_P2P));
            }

            // Fabric mode specific settings
            if (pDevice->Key.DeviceMode == PLX_CHIP_MODE_FABRIC)
            {
                // Initialize upper I2C address to 0
                PlxI2c_PlxRegisterWrite( pDevice, 0x2CC, 0, FALSE );

                // Always enable NTV0 which is GEP
                PossiblePorts |= ((U64)1 << PLX_FLAG_PORT_NT_VIRTUAL_0);
            }
        }

        // Get PCI header
        RegPciHeader =
            PlxI2c_PlxRegisterRead(
                pDevice,
                0xC,        // PCI Header Type
                &status,
                TRUE,       // Adjust for port?
                TRUE        // Retry on error?
                );
        if (status != PLX_STATUS_OK)
        {
            goto _PlxI2c_ProbeSwitch_Next_Port;
        }

        // Verify port exists
        if (RegPciHeader == (U32)-1)
        {
            // Some chips return FFFF_FFFF when accessing 0Ch of disabled port
            DebugPrintf(("I2C - Port %d is disabled, PCI[0Ch]=FFFF_FFFF\n", Port));
        }
        else if ((RegPciHeader & 0xFFFF) == PLX_VENDOR_ID)
        {
            // Some chips return value of last register (Dev/Ven ID)
            // read when accessing 0Ch of disabled port
            DebugPrintf(("I2C - Port %d is disabled, PCI[0Ch]=%08X (Dev/Ven ID)\n", Port, RegPciHeader));
        }
        else
        {
            DebugPrintf(("I2C - Port %d is enabled, get additional properties\n", Port));
            bCompareDevice = TRUE;
        }

        if (bCompareDevice)
        {
            // Fill in device key
            pDevice->Key.DeviceId = (U16)(DevVenID >> 16);
            pDevice->Key.VendorId = (U16)DevVenID;

            // Set header type field
            PciHeaderType = (U8)((RegPciHeader >> 16) & 0x7F);

            // Store NT port numbers if exist
            if (Gbl_PlxI2cProp[pDevice->Key.ApiIndex].NTPortNum[0] != (U8)PCI_FIELD_IGNORE)
            {
                pDevice->Key.NTPortNum |= Gbl_PlxI2cProp[pDevice->Key.ApiIndex].NTPortNum[0];
            }

            if (Gbl_PlxI2cProp[pDevice->Key.ApiIndex].NTPortNum[1] != (U8)PCI_FIELD_IGNORE)
            {
                pDevice->Key.NTPortNum |= (Gbl_PlxI2cProp[pDevice->Key.ApiIndex].NTPortNum[1] << 4);
            }

            // Get port properties
            PlxI2c_GetPortProperties(
                pDevice,
                &PortProp
                );

            // Set PLX-specific port type
            if (PortProp.PortType == PLX_PORT_UPSTREAM)
            {
                // In fabric mode, upstream ports other than chip UP are host ports
                if ((pDevice->Key.DeviceMode == PLX_CHIP_MODE_FABRIC) &&
                    (Port_Upstream != Port))
                {
                    pDevice->Key.PlxPortType = PLX_SPEC_PORT_HOST;
                }
                else
                {
                    pDevice->Key.PlxPortType = PLX_SPEC_PORT_UPSTREAM;
                }
            }
            else if (PortProp.PortType == PLX_PORT_DOWNSTREAM)
            {
                // Default to DS port
                pDevice->Key.PlxPortType = PLX_SPEC_PORT_DOWNSTREAM;

                // Could be a fabric port in fabric mode
                if ((Port < 24) && (pDevice->Key.DeviceMode == PLX_CHIP_MODE_FABRIC))
                {
                    // Calculate offset for port's configuration
                    StnPort = Port % PortsPerStn;
                    offset  = 0x80000 + ((Port / PortsPerStn) * 0x10000);
                    offset += ((StnPort / 4) * sizeof(U32));

                    RegValue =
                        PlxI2c_PlxRegisterRead(
                            pDevice,
                            offset,     // Port configuration
                            NULL,
                            FALSE,      // Adjust for port?
                            TRUE        // Retry on error?
                            );

                    // Get port type in [1:0] (01 = Fabric)
                    RegValue = (U8)((RegValue >> ((StnPort % 4) * 8)) & 0x3);
                    if (RegValue == 1)
                    {
                        pDevice->Key.PlxPortType = PLX_SPEC_PORT_FABRIC;
                    }
                }
            }
            else if (PortProp.PortType == PLX_PORT_ENDPOINT)
            {
                // NT link port
                if ((Port == PLX_FLAG_PORT_NT_LINK_0) || (Port == PLX_FLAG_PORT_NT_LINK_1))
                {
                    pDevice->Key.PlxPortType = PLX_SPEC_PORT_NT_LINK;
                }

                // NT virtual port (Also check for older chips where NTV accessed by port #)
                if ((Port == PLX_FLAG_PORT_NT_VIRTUAL_0) || (Port == PLX_FLAG_PORT_NT_VIRTUAL_1) ||
                    (Port == Gbl_PlxI2cProp[pDevice->Key.ApiIndex].NTPortNum[0]))
                {
                    pDevice->Key.PlxPortType = PLX_SPEC_PORT_NT_VIRTUAL;
                }

                // DMA function
                if ((Port == PLX_FLAG_PORT_DMA_0) || (Port == PLX_FLAG_PORT_DMA_1) ||
                    (Port == PLX_FLAG_PORT_DMA_2) || (Port == PLX_FLAG_PORT_DMA_3))
                {
                    pDevice->Key.PlxPortType = PLX_SPEC_PORT_DMA;
                }

                // GEP
                if (pDevice->Key.DeviceId == 0x1009)
                {
                    pDevice->Key.PlxPortType = PLX_SPEC_PORT_GEP;
                }
            }

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
                            FALSE,                  // Adjust for port?
                            TRUE                    // Retry on error?
                            );

                    if ((status == PLX_STATUS_OK) && (RegValue != (U32)-1))
                    {
                        // Set the upstream bus number
                        Gbl_PlxI2cProp[pDevice->Key.ApiIndex].UpstreamBus = (U8)(RegValue >> 0);
                    }
                }
            }

            // Reset bus
            pDevice->Key.bus = 0;

            // Get bus number
            if (PciHeaderType == 1)
            {
                RegValue =
                    PlxI2c_PlxRegisterRead(
                        pDevice,
                        0x18,       // Primary/Secondary bus numbers
                        NULL,
                        TRUE,       // Adjust for port?
                        TRUE        // Retry on error?
                        );

                // Bus is primary bus number field (18h[7:0])
                pDevice->Key.bus = (U8)(RegValue >> 0);

                // Store the upstream port bus number
                if (pDevice->Key.PlxPortType == PLX_SPEC_PORT_UPSTREAM)
                {
                    Gbl_PlxI2cProp[pDevice->Key.ApiIndex].UpstreamBus = pDevice->Key.bus;
                    DebugPrintf(("I2C - UP bus=%02X\n", pDevice->Key.bus));
                }

                // For host/fabric ports, use bus number assigned by manager in [15:8]
                if ((pDevice->Key.PlxPortType == PLX_SPEC_PORT_HOST) ||
                    (pDevice->Key.PlxPortType == PLX_SPEC_PORT_FABRIC))
                {
                    pDevice->Key.bus = (U8)(RegValue >> 8);
                }
            }
            else
            {
                // Some chips have captured bus number register
                if ((pDevice->Key.PlxFamily == PLX_FAMILY_CYGNUS)  ||
                    (pDevice->Key.PlxFamily == PLX_FAMILY_SCOUT)   ||
                    (pDevice->Key.PlxFamily == PLX_FAMILY_DRACO_1) ||
                    (pDevice->Key.PlxFamily == PLX_FAMILY_DRACO_2) ||
                    (pDevice->Key.PlxFamily == PLX_FAMILY_CAPELLA_1) ||
                    (pDevice->Key.PlxFamily == PLX_FAMILY_CAPELLA_2))
                {
                    // GEP doesn't report correct captured bus, so get from parent P2P
                    if (pDevice->Key.PlxPortType == PLX_SPEC_PORT_GEP)
                    {
                        // Get Primary/Secondary bus register (18h)
                        RegValue =
                            PlxI2c_PlxRegisterRead(
                                pDevice,
                                (PortProp.PortNumber * 0x1000) + 0x18,
                                NULL,
                                FALSE,      // Adjust for port?
                                TRUE        // Retry on error?
                                );

                        pDevice->Key.bus = (U8)(RegValue >> 8);
                    }

                    if (pDevice->Key.bus == 0)
                    {
                        // Get Captured bus number
                        RegValue =
                            PlxI2c_PlxRegisterRead(
                                pDevice,
                                0x1DC,      // Captured bus number
                                NULL,
                                TRUE,       // Adjust for port?
                                TRUE        // Retry on error?
                                );

                        pDevice->Key.bus = (U8)RegValue;
                    }

                    // Cygnus NT Virtual port 1DCh not corrrect over I2C
                    if ((pDevice->Key.PlxFamily == PLX_FAMILY_CYGNUS) &&
                        (pDevice->Key.PlxPortType == PLX_SPEC_PORT_NT_VIRTUAL))
                    {
                        pDevice->Key.bus = 0;
                    }
                }
            }

            /***************************************************************
             * If devices aren't enumerated, DS ports & other device bus
             * numbers will not be assigned and will always end up on
             * bus 0. If that is the case, auto-generate bus numbers
             **************************************************************/
            if ((pDevice->Key.bus == 0) && (pDevice->Key.PlxPortType != PLX_SPEC_PORT_UPSTREAM))
            {
                // Default to upstream bus if known
                if (Gbl_PlxI2cProp[pDevice->Key.ApiIndex].UpstreamBus != (U8)PCI_FIELD_IGNORE)
                {
                    pDevice->Key.bus = Gbl_PlxI2cProp[pDevice->Key.ApiIndex].UpstreamBus;
                }

                if (pDevice->Key.PlxPortType == PLX_SPEC_PORT_DOWNSTREAM)
                {
                    // DS ports are behind upstream bus
                    pDevice->Key.bus += 1;
                }
                else if (pDevice->Key.PlxPortType == PLX_SPEC_PORT_NT_LINK)
                {
                    // NT Link port
                    pDevice->Key.bus += 4;

                    // Add a bus for 2nd NT port
                    if (pDevice->Key.PlxPort == PLX_FLAG_PORT_NT_LINK_1)
                    {
                        pDevice->Key.bus += 1;
                    }
                }
                else if ((PortProp.PortType == PLX_PORT_LEGACY_ENDPOINT) ||
                         (pDevice->Key.PlxPortType == PLX_SPEC_PORT_NT_VIRTUAL))
                {
                    // USB controller or NT-Virtual port
                    pDevice->Key.bus += 2;

                    // For P2P mode, NT Virtual has parent DS port
                    if (pDevice->Key.DeviceMode == PLX_CHIP_MODE_STD_NT_DS_P2P)
                    {
                        pDevice->Key.bus += 1;
                    }

                    // Add a bus for 2nd NT port
                    if (pDevice->Key.PlxPort == PLX_FLAG_PORT_NT_VIRTUAL_1)
                    {
                        pDevice->Key.bus += 1;
                    }
                }
                else if (pDevice->Key.PlxPortType == PLX_SPEC_PORT_DMA)
                {
                    // DMA controller is on same bus as upstream port
                }
                else if (pDevice->Key.PlxPortType == PLX_SPEC_PORT_GEP)
                {
                    // GEP
                    pDevice->Key.bus += 0x20;
                }
                else if ((pDevice->Key.PlxPortType == PLX_SPEC_PORT_HOST) ||
                         (pDevice->Key.PlxPortType == PLX_SPEC_PORT_FABRIC))
                {
                    // Host & fabric ports - Pick high bus number
                    pDevice->Key.bus = 0xE0 + pDevice->Key.PlxPortType;
                }
                else
                {
                    ErrorPrintf((
                        "ERROR: Bus # auto-gen not implemented for port type %d\n",
                        pDevice->Key.PlxPortType
                        ));
                    pDevice->Key.bus = 0xFF;
                }
            }

            // Default to slot 0
            pDevice->Key.slot = 0;

            // Set slot number
            if (PortProp.PortType == PLX_PORT_ENDPOINT)
            {
                // UP & EPs are always slot 0 except NTV in older chips
                if ((Port != PLX_FLAG_PORT_NT_VIRTUAL_0) &&
                    (pDevice->Key.PlxPortType == PLX_SPEC_PORT_NT_VIRTUAL))
                {
                    pDevice->Key.slot = (U8)Port;  // Slot matches port number
                }
            }
            else if (pDevice->Key.PlxPort == PLX_FLAG_PORT_NT_DS_P2P)
            {
                // NT P2P port uses NT port number as slot
                pDevice->Key.slot = pDevice->Key.NTPortNum;
            }
            else
            {
                pDevice->Key.slot = (U8)Port;  // Slot matches port number
            }

            // Function number is non-zero for DMA & 0 for other ports
            if (pDevice->Key.PlxPort == PLX_FLAG_PORT_DMA_0)
            {
                pDevice->Key.function = 1;
            }
            else if (pDevice->Key.PlxPort == PLX_FLAG_PORT_DMA_1)
            {
                pDevice->Key.function = 2;
            }
            else if (pDevice->Key.PlxPort == PLX_FLAG_PORT_DMA_2)
            {
                pDevice->Key.function = 3;
            }
            else if (pDevice->Key.PlxPort == PLX_FLAG_PORT_DMA_3)
            {
                pDevice->Key.function = 4;
            }
            else
            {
                pDevice->Key.function = 0;
            }

            // Get PCI Revision
            RegValue =
                PlxI2c_PlxRegisterRead(
                    pDevice,
                    0x08,
                    &status,
                    TRUE,       // Adjust for port?
                    TRUE        // Retry on error?
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
                        TRUE,       // Adjust for port?
                        TRUE        // Retry on error?
                        );
            }
            else
            {
                // Get subsytem ID from capability if supported
                offset =
                    PlxPciFindCapability(
                        pDevice,
                        PCI_CAP_ID_BRIDGE_SUB_ID,
                        FALSE,
                        0
                        );
                if (offset == 0)
                {
                    RegValue = 0;
                }
                else
                {
                    RegValue =
                        PlxI2c_PlxRegisterRead(
                            pDevice,
                            offset + 0x04,
                            &status,
                            TRUE,       // Adjust for port?
                            TRUE        // Retry on error?
                            );
                }
            }

            pDevice->Key.SubDeviceId = (U16)(RegValue >> 16);
            pDevice->Key.SubVendorId = (U16)RegValue;

            // Display MIRA mode for debug
            if (pDevice->Key.PlxFamily == PLX_FAMILY_MIRA)
            {
                DebugPrintf((
                    "MIRA Device @ [b:%02x s:%02x] in %s mode\n",
                    pDevice->Key.bus, pDevice->Key.slot,
                    (pDevice->Key.DeviceMode == PLX_CHIP_MODE_STANDARD) ? "Enhanced" : "Legacy"
                    ));
            }

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

            // Compare Subsystem ID only if valid in chip
            if (pDevice->Key.SubVendorId != 0)
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
                        "Criteria matched device %04X %04X [%02X:%02X.%d]\n",
                        pDevice->Key.DeviceId, pDevice->Key.VendorId,
                        pDevice->Key.bus, pDevice->Key.slot, pDevice->Key.function
                        ));

                    // Copy the device information
                    *pKey = pDevice->Key;
                    return PLX_STATUS_OK;
                }

                // Increment device count
                DeviceCount++;
            }
        }

_PlxI2c_ProbeSwitch_Next_Port:
        // Go to next port
        Port++;
    }

    // Return number of matched devices
    *pNumMatched = DeviceCount;

    DebugPrintf(("Criteria did not match any devices\n"));

    return PLX_STATUS_INVALID_OBJECT;
}




/*******************************************************************************
 *
 * Function   :  PlxPciFindCapability
 *
 * Description:  Scans a device's capability list to find the base offset of the
 *               specified PCI or PCIe extended capability. If the capability ID
 *               is PCI or PCIe vendor-specific (VSEC), an optional instance
 *               number can be provided since a device could have multiple VSEC
 *               capabilities. A value of 0 returns the 1st matching instance but
 *               the parameter is ignored for non-VSEC capability search.
 *
 ******************************************************************************/
U16
PlxPciFindCapability(
    PLX_DEVICE_OBJECT *pDevice,
    U16                CapID,
    U8                 bPCIeCap,
    U8                 InstanceNum
    )
{
    U8  matchCount;
    U16 offset;
    U16 currID;
    U32 regVal;


    // Get PCI command register (04h)
    PLX_PCI_REG_READ( pDevice, PCI_REG_CMD_STAT, &regVal );

    // Verify device responds to PCI accesses (in case link down)
    if (regVal == (U32)-1)
    {
        return 0;
    }

    // Verify device supports extended capabilities (04h[20])
    if ((regVal & ((U32)1 << 20)) == 0)
    {
        return 0;
    }

    // Set capability pointer offset
    if (bPCIeCap)
    {
        // PCIe capabilities must start at 100h
        offset = 0x100;

        // Ignore instance number for non-VSEC capabilities
        if (CapID != PCIE_CAP_ID_VENDOR_SPECIFIC)
        {
            InstanceNum = 0;
        }
    }
    else
    {
        // Get offset of first capability from capability pointer (34h[7:0])
        PLX_PCI_REG_READ( pDevice, PCI_REG_CAP_PTR, &regVal );

        // Set first capability offset
        offset = (U8)regVal;

        // Ignore instance number for non-VSEC capabilities
        if (CapID != PCI_CAP_ID_VENDOR_SPECIFIC)
        {
            InstanceNum = 0;
        }
    }

    // Start with 1st match
    matchCount = 0;

    // Traverse capability list searching for desired ID
    while ((offset != 0) && (regVal != (U32)-1))
    {
        // Get next capability
        PLX_PCI_REG_READ( pDevice, offset, &regVal );

        // Verify capability is valid
        if ((regVal == 0) || (regVal == (U32)-1))
        {
            return 0;
        }

        // Extract the capability ID
        if (bPCIeCap)
        {
            // PCIe ID in [15:0]
            currID = (U16)((regVal >> 0) & 0xFFFF);
        }
        else
        {
            // PCI ID in [7:0]
            currID = (U16)((regVal >> 0) & 0xFF);
        }

        // Compare with desired capability
        if (currID == CapID)
        {
            // Verify correct instance
            if (InstanceNum == matchCount)
            {
                // Capability found, return base offset
                return offset;
            }

            // Increment count of matches
            matchCount++;
        }

        // Jump to next capability
        if (bPCIeCap)
        {
            // PCIe next cap offset in [31:20]
            offset = (U16)((regVal >> 20) & 0xFFF);
        }
        else
        {
            // PCI next cap offset in [15:8]
            offset = (U8)((regVal >> 8) & 0xFF);
        }
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
    U16 offset[] = {0xB78,0x958,0xE0,0x0};
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

            // PLX revision should be in next register
            PLX_PCI_REG_READ(
                pDevice,
                offset[i] + sizeof(U32),
                &RegValue
                );

            pDevice->Key.PlxRevision = (U8)(RegValue & 0xFF);

            // Some chips have not updated hard-coded revision ID of AA
            if ((pDevice->Key.PlxRevision == 0xAA) &&
                ((pDevice->Key.PlxChip == 0x8612) ||
                 (pDevice->Key.PlxChip == 0x8616) ||
                 (pDevice->Key.PlxChip == 0x8624) ||
                 (pDevice->Key.PlxChip == 0x8632) ||
                 (pDevice->Key.PlxChip == 0x8647) ||
                 (pDevice->Key.PlxChip == 0x8648)))
            {
                // Override hard-coded revision
                PlxChipRevisionDetect( pDevice );
            }

            // Skip to assigning family
            goto _PlxChipAssignFamily;
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
            PCI_REG_TO_SUBSYS_ID,
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
    PlxChipRevisionDetect( pDevice );

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

        case 0x8712:
        case 0x8716:
        case 0x8723:
        case 0x8724:
        case 0x8732:
        case 0x8747:
        case 0x8748:
            if (pDevice->Key.PlxRevision == 0xAA)
                pDevice->Key.PlxFamily = PLX_FAMILY_DRACO_1;
            else
                pDevice->Key.PlxFamily = PLX_FAMILY_DRACO_2;
            break;

        case 0x8713:
        case 0x8717:
        case 0x8725:
        case 0x8733:
        case 0x8749:
            pDevice->Key.PlxFamily = PLX_FAMILY_DRACO_2;
            break;

        case 0x2380:
        case 0x3380:
        case 0x3382:
        case 0x8603:
        case 0x8605:
            pDevice->Key.PlxFamily = PLX_FAMILY_MIRA;
            break;

        case 0x8714:
        case 0x8718:
        case 0x8734:
        case 0x8750:
        case 0x8764:
        case 0x8780:
        case 0x8796:
            pDevice->Key.PlxFamily = PLX_FAMILY_CAPELLA_1;
            break;

        case 0x9712:
        case 0x9713:
        case 0x9716:
        case 0x9717:
        case 0x9733:
        case 0x9734:
        case 0x9749:
        case 0x9750:
        case 0x9765:
        case 0x9766:
        case 0x9781:
        case 0x9782:
        case 0x9797:
        case 0x9798:
            pDevice->Key.PlxFamily = PLX_FAMILY_CAPELLA_2;
            break;

        case 0:
            pDevice->Key.PlxFamily = PLX_FAMILY_NONE;
            break;

        default:
            ErrorPrintf(("ERROR - PLX Family not set for %04X\n", pDevice->Key.PlxChip));
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
    PLX_PCI_REG_READ( pDevice, PCI_REG_CLASS_REV, &RegValue );

    // Default revision to value in chip
    pDevice->Key.PlxRevision = (U8)(RegValue & 0xFF);
}




/******************************************************************************
 *
 * Function   :  PlxChipFilterDisabledPorts
 *
 * Description:  Probes a chip for disabled ports & removes them from port mask
 *
 ******************************************************************************/
PLX_STATUS
PlxChipFilterDisabledPorts(
    PLX_DEVICE_OBJECT *pDevice,
    U64               *pPortMask
    )
{
    U8         MaxPorts;
    U32        Offset;
    U32        RegValue;
    U64        ChipMask;
    PLX_STATUS status;


    // Set default max number of ports
    MaxPorts = 24;

    // Set register to use for port enabled status
    switch (pDevice->Key.PlxFamily)
    {
        case PLX_FAMILY_ALTAIR:
        case PLX_FAMILY_ALTAIR_XL:
        case PLX_FAMILY_VEGA:
        case PLX_FAMILY_VEGA_LITE:
        case PLX_FAMILY_DENEB:
        case PLX_FAMILY_SIRIUS:
            Offset = 0x668;
            break;

        case PLX_FAMILY_MIRA:
            Offset = 0x1D8;
            break;

        case PLX_FAMILY_CYGNUS:
        case PLX_FAMILY_SCOUT:
        case PLX_FAMILY_DRACO_1:
        case PLX_FAMILY_DRACO_2:
            Offset = 0x314;
            break;

        case PLX_FAMILY_CAPELLA_1:
        case PLX_FAMILY_CAPELLA_2:
            if (pDevice->Key.DeviceMode == PLX_CHIP_MODE_STANDARD)
            {
                Offset = 0xF4C;
            }
            else
            {
                Offset = 0x30C;
            }
            break;

        default:
            ErrorPrintf(("ERROR - Disabled port filter not implemented (%04X)\n", pDevice->Key.PlxChip));
            return PLX_STATUS_UNSUPPORTED;
    }

    // Get port enable status
    RegValue = PlxI2c_PlxRegisterRead( pDevice, Offset, &status, FALSE, TRUE );
    if (status != PLX_STATUS_OK)
    {
        return status;
    }

    // Set port only mask
    ChipMask = (((U64)1 << MaxPorts) - 1);

    // Extract only valid ports from register
    RegValue &= ChipMask;

    // Filter ports that don't exist in this chip
    RegValue &= PLX_64_LOW_32( *pPortMask );

    // Clear port numbers in mask
    *pPortMask &= ~(U64)ChipMask;

    // Insert valid ports
    *pPortMask |= RegValue;

    return PLX_STATUS_OK;
}
