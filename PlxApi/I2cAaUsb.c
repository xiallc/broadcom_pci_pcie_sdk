/*******************************************************************************
 * Copyright 2013-2018 Broadcom Inc
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
 *      I2cAaUsb.c
 *
 * Description:
 *
 *      Implements PLX API support functions over an Aardark I2C interface
 *
 * Revision History:
 *
 *      11-01-18 : PLX SDK v8.00
 *
 ******************************************************************************/

/*******************************************************************************
 *
 * I2C API utilization of fields in PLX_DEVICE_KEY
 *
 *  ApiIndex        - I2C USB device number (0,1,etc)
 *  DeviceNumber    - PLX chip I2C slave address
 *  PlxPort         - PLX port number or port type (NTL,NTV,DMA,etc)
 *  PlxPortType     - PLX-specific port type (PLX_PORT_TYPE)
 *  NTPortNum       - NT port numbers if enabled (NT1=[7:4] NT0=[3:0])
 *  DeviceMode      - Current chip mode (Std,NT,VS,Fabric,etc)
 *  ApiInternal[0]  - I2C bus speed in KHz
 *  ApiInternal[1]  - Port-specific base address for registers
 *
 ******************************************************************************/

/*******************************************************************************
 *
 * PLX chip I2C port access table
 *
 * Mira
 *              Transp  NTV0 NTV1 NTL0 NTL1 DMA0 DMA1 DMA2 DMA3 DMA_RAM PCI2USB USB
 *   Mode         --     --   --   --   --   --   --   --   --    --       --    -
 *   Stn_Sel      --     --   --   --   --   --   --   --   --    --       --    -
 *   Port_Sel     P#     --   --   --   --   --   --   --   --    --        3    4
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
 *   Port_Sel     P#     P#   --   10h  --   11h 12h  --   --   --    13h
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
 * Capella 1 / Capella 2 (Mode 0)                 |
 *              Transp  NTV0 NTV1 NTL0 NTL1 ALUT  | C2 Full address (Mode 2)
 *   Mode          0      2    2    1    1    3   |        2     (*P0 2CCh=R[22:18])
 *   Stn_Sel      S#      0    0    0    0    2   |     R[17:15]
 *   Port_Sel     P#      0    1    0    1   R#   |     R[14:12]
 *
 * Atlas (Mode 0)                       |
 *              Transp                  | Full address (Mode 2)
 *   Mode          0                    |        2     (*P0 2CCh=R[22:18])
 *   Stn_Sel      S#  (Only ports 0-63) |     R[17:16]
 *   Port_Sel     P#                    |     R[15:12]
 *
 ******************************************************************************/

/*******************************************************************************
 *
 * PLX chip I2C port access command
 *
 * Mira
 *      31   27 26     24 23  20  19       15 14  13     10 9          0
 *      ----------------------------------------------------------------
 *     | Resvd | I2C_Cmd | Rsvd |  Port_Sel  | R | Byte_En |  DW_Offset |
 *      ----------------------------------------------------------------
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
 * Atlas
 *      31   27 26     24 23  22  21 20    19 18   15 14  13     10 9          0
 *      ------------------------------------------------------------------------
 *     | Resvd | I2C_Cmd | R | Mode | StnSel | PtSel | R | Byte_En |  DW_Offset |
 *      ------------------------------------------------------------------------
 *
 ******************************************************************************/


#include "Eep_8000.h"
#include "PciRegs.h"
#include "PexApi.h"
#include "PlxApiDebug.h"
#include "PlxApiDirect.h"
#include "I2cAaUsb.h"

#if !defined(PLX_DOS)
    #include "Aardvark.h"
#endif




/**********************************************
 *           Global Variables
 *********************************************/
#if !defined(PLX_DOS)
    // Global count of number of current open I2C devices
    static U8 Gbl_OpenCount = 0;

    static struct _I2C_API_PROP
    {
        CRITICAL_SECTION  Lock_RegAccess;
        PLX_DRIVER_HANDLE hDevice;
        S32               OpenCount;
        U8                UpstreamBus;
        U8                NTPortNum[I2C_MAX_NT_PORTS];
        U8                LastHighBits;
        U32               IdxLastAddr;
    } Gbl_I2cProp[I2C_MAX_DEVICES];

    // Macros for store/retrieval of I2C high address bits
    #define I2C_KEY_HIGH_ADDR_GET(pDev)         Gbl_I2cProp[(pDev)->Key.ApiIndex].LastHighBits
    #define I2C_KEY_HIGH_ADDR_SAVE(pDev,addr)   Gbl_I2cProp[(pDev)->Key.ApiIndex].LastHighBits = (addr)
    #define I2C_KEY_HIGH_ADDR_RESET(pDev)       I2C_KEY_HIGH_ADDR_SAVE( (pDev), I2C_HIGH_ADDR_INIT )
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
    U16 Ports[I2C_MAX_DEVICES];
    int PortCount;


    // Default to no active ports
    *pI2cPorts = 0;

    // Verify supported I2C mode
    if (ApiMode != PLX_API_MODE_I2C_AARDVARK)
    {
        return PLX_STATUS_UNSUPPORTED;
    }

    // Find all I2C ports
    PortCount = aa_find_devices( I2C_MAX_DEVICES, Ports );
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
    PlxDir_ChipTypeDetect( pDevice );

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
    if (Gbl_I2cProp[pDevice->Key.ApiIndex].OpenCount <= 0)
    {
        return PLX_STATUS_INVALID_ACCESS;
    }

    // Decrement open count and close device if no longer referenced
    if (InterlockedDecrement(
            &Gbl_I2cProp[pDevice->Key.ApiIndex].OpenCount
            ) == 0)
    {
        // Close the device (aa_close() returns num handles closed)
        if (aa_close( (Aardvark)PLX_PTR_TO_INT( pDevice->hDevice ) ) != 1)
        {
            return PLX_STATUS_INVALID_ACCESS;
        }

        // Mark device is closed
        Gbl_I2cProp[pDevice->Key.ApiIndex].hDevice = 0;

        // Reset saved values
        I2C_KEY_HIGH_ADDR_RESET( pDevice );
        Gbl_I2cProp[pDevice->Key.ApiIndex].IdxLastAddr = (U32)-1;

        // Delete the register access lock
        DeleteCriticalSection(
            &(Gbl_I2cProp[pDevice->Key.ApiIndex].Lock_RegAccess)
            );

        // Decrement global open count
        Gbl_OpenCount--;
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
    U16               Ports[I2C_MAX_DEVICES];
    U32               RegValue;
    BOOLEAN           bFound;
    BOOLEAN           bAutoProbe;
    PLX_STATUS        status;
    PLX_DEVICE_OBJECT Device;
    PLX_DEVICE_OBJECT DeviceTemp;


    // Find all I2C ports
    PortCount = aa_find_devices( I2C_MAX_DEVICES, Ports );
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
        pModeProp->I2c.ClockRate = I2C_DEFAULT_CLOCK_RATE;
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
                PCI_REG_DEV_VEN_ID, // Port 0 Device/Vendor ID
                &status,
                FALSE,              // Adjust for port?
                FALSE               // Retry on error?
                );

        if (status == PLX_STATUS_OK)
        {
            if ( ((RegValue & 0xFFFF) == PLX_PCI_VENDOR_ID_PLX) ||
                 ((RegValue & 0xFFFF) == PLX_PCI_VENDOR_ID_LSI) )
            {
                DebugPrintf((
                    "I2C: Detected device %08X at %02Xh\n",
                    RegValue, Device.Key.DeviceNumber
                    ));

                if ((RegValue & 0xFFFF) == PLX_PCI_VENDOR_ID_LSI)
                {
                    // Device found, probe for active ports and compare
                    status =
                        PlxDir_ProbeSwitch(
                            &DeviceTemp,
                            pKey,
                            (U16)(DeviceNumber - TotalMatches),
                            &NumMatched
                            );
                }
                else
                {
                    // Legacy device found, determine active ports and compare
                    status =
                        PlxI2c_ProbeSwitch(
                            &DeviceTemp,
                            pKey,
                            (U16)(DeviceNumber - TotalMatches),
                            &NumMatched
                            );
                }

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
    if (Gbl_I2cProp[I2cPort].hDevice == 0)
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
        hI2cDev = (Aardvark)PLX_PTR_TO_INT( Gbl_I2cProp[I2cPort].hDevice );
    }

    // Get I2C version information
    aa_version( hI2cDev, &I2cProp.version );

    // Get I2C supported features
    I2cProp.features = aa_features( hI2cDev );

    // Release device only if not already open
    if (Gbl_I2cProp[I2cPort].hDevice == 0)
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
    int status_AA;
    U16 nBytesRead;
    U16 nBytesWritten;
    U32 command;
    U32 regVal;


    // Set max retry count
    if (bRetryOnError)
    {
        bRetryOnError = I2C_RETRY_MAX_COUNT;
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

    // If outside PEX region (60800000-60FFFFFF), must use indexing method
    if ( (pDevice->Key.PlxFamily == PLX_FAMILY_ATLAS) &&
         (bAdjustForPort == FALSE) &&
         ((offset & I2C_PEX_BASE_ADDR_MASK) != ATLAS_REGS_AXI_BASE_ADDR) )
    {
        // Set AXI address to read (1F0100h) if has changed
        if (offset != Gbl_I2cProp[pDevice->Key.ApiIndex].IdxLastAddr)
        {
            PlxI2c_PlxRegisterWrite(
                pDevice,
                ATLAS_REGS_AXI_BASE_ADDR + PEX_REG_IDX_AXI_ADDR,
                offset,
                FALSE        // Adjust for port?
                );

            // Store address to avoid future write if no change
            Gbl_I2cProp[pDevice->Key.ApiIndex].IdxLastAddr = offset;
        }

        // Issue read command (1F0108h[1]=1)
        PlxI2c_PlxRegisterWrite(
            pDevice,
            ATLAS_REGS_AXI_BASE_ADDR + PEX_REG_IDX_AXI_CTRL,
            PEX_IDX_CTRL_CMD_READ,
            FALSE        // Adjust for port?
            );

        // Get the data (1F0104h)
        regVal =
            PlxI2c_PlxRegisterRead(
                pDevice,
                ATLAS_REGS_AXI_BASE_ADDR + PEX_REG_IDX_AXI_DATA,
                pStatus,
                FALSE,                    // Adjust for port?
                (U8)(bRetryOnError != 0)  // Retry on error?
                );

        return regVal;
    }

    // Generate the I2C command
    command =
        PlxI2c_GenerateCommand(
            pDevice,
            I2C_CMD_REG_READ,
            offset,
            bAdjustForPort
            );

    // Some I2C commands cannot be sent to chip
    if (command == I2C_CMD_SKIP)
    {
        if (pStatus != NULL)
        {
            *pStatus = PLX_STATUS_OK;
        }
        return 0;
    }

    if (command == I2C_CMD_ERROR)
    {
        return 0;
    }

    // Set default return value
    regVal = 0;

    do
    {
        // Get the register access lock
        EnterCriticalSection(
            &(Gbl_I2cProp[pDevice->Key.ApiIndex].Lock_RegAccess)
            );

        // Issue read command and get data
        status_AA =
            aa_i2c_write_read(
                (Aardvark)PLX_PTR_TO_INT( pDevice->hDevice ),
                pDevice->Key.DeviceNumber,
                AA_I2C_NO_FLAGS,
                sizeof(U32),                    // Num write bytes
                (U8*)&command,                  // Write data
                &nBytesWritten,                 // Bytes written
                sizeof(U32),                    // Num bytes to read
                (U8*)&regVal,                 // Read data buffer
                &nBytesRead                     // Bytes read
                );

        // Release the register access lock
        LeaveCriticalSection(
            &(Gbl_I2cProp[pDevice->Key.ApiIndex].Lock_RegAccess)
            );

        // Read status in [15:8] and write status in [7:0]
        if (((status_AA >> 8) == AA_OK) && ((status_AA & 0xFF) == AA_OK) &&
            (nBytesRead == sizeof(U32)) && (nBytesWritten == sizeof(U32)))
        {
            if (pStatus != NULL)
            {
                *pStatus = PLX_STATUS_OK;
            }

            // Convert to Big Endian format since data is returned in BE
            regVal = PLX_BE_DATA_32( regVal );

            if (bRetryOnError)
            {
                // Get retry count
                bRetryOnError = I2C_RETRY_MAX_COUNT - bRetryOnError;

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
                Plx_sleep( I2C_RETRY_DELAY_MS );

                // Decrement retry count
                bRetryOnError--;

                // Check if reached final one
                if (bRetryOnError == 0)
                {
                    ErrorPrintf((
                        "ERROR: I2C READ failed (St_Rd:%02d %dB St_Wr:%02d %dB O:%02Xh)\n",
                        (status_AA >> 8) & 0xFF, nBytesRead,
                        (status_AA & 0xFF), nBytesWritten, offset
                        ));
                }
            }
            regVal = 0;
        }
    }
    while (bRetryOnError);

    return regVal;
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
    int        status_AA;
    U16        bRetryOnError;
    U32        command;
    U32        dataStream[2];
    PLX_STATUS status;


    // Verify register offset
    if (offset & 0x3)
    {
        DebugPrintf(("ERROR - Invalid register offset (0x%x)\n", offset));
        return PLX_STATUS_INVALID_OFFSET;
    }

    // If outside PEX region (60800000-60FFFFFF), must use indexing method
    if ( (pDevice->Key.PlxFamily == PLX_FAMILY_ATLAS) &&
         (bAdjustForPort == FALSE) &&
         ((offset & I2C_PEX_BASE_ADDR_MASK) != ATLAS_REGS_AXI_BASE_ADDR) )
    {
        // Set AXI address to write (1F0100h) if has changed
        if (offset != Gbl_I2cProp[pDevice->Key.ApiIndex].IdxLastAddr)
        {
            status =
                PlxI2c_PlxRegisterWrite(
                    pDevice,
                    ATLAS_REGS_AXI_BASE_ADDR + PEX_REG_IDX_AXI_ADDR,
                    offset,
                    FALSE        // Adjust for port?
                    );
        }
        else
        {
            status = PLX_STATUS_OK;
        }

        // Set data to write the data (1F0104h)
        if (status == PLX_STATUS_OK)
        {
            // Store address to avoid future write if no change
            Gbl_I2cProp[pDevice->Key.ApiIndex].IdxLastAddr = offset;

            status =
                PlxI2c_PlxRegisterWrite(
                    pDevice,
                    ATLAS_REGS_AXI_BASE_ADDR + PEX_REG_IDX_AXI_DATA,
                    value,
                    FALSE        // Adjust for port?
                    );
        }

        // Issue read command (1F0108h[1]=1)
        if (status == PLX_STATUS_OK)
        {
            status =
                PlxI2c_PlxRegisterWrite(
                    pDevice,
                    ATLAS_REGS_AXI_BASE_ADDR + PEX_REG_IDX_AXI_CTRL,
                    PEX_IDX_CTRL_CMD_WRITE,
                    FALSE        // Adjust for port?
                    );
        }

        return status;
    }

    // Generate the I2C command
    command =
        PlxI2c_GenerateCommand(
            pDevice,
            I2C_CMD_REG_WRITE,
            offset,
            bAdjustForPort
            );

    // Some I2C commands cannot be sent to chip
    if (command == I2C_CMD_SKIP)
    {
        return PLX_STATUS_OK;
    }

    if (command == I2C_CMD_ERROR)
    {
        return PLX_STATUS_FAILED;
    }

    // Convert register value to Big Endian format
    value = PLX_BE_DATA_32( value );

    // Copy command and value into data stream
    dataStream[0] = command;
    dataStream[1] = value;

    // Set max retry count
    bRetryOnError = I2C_RETRY_MAX_COUNT;

    // Default to success
    status = PLX_STATUS_OK;

    do
    {
        // Get the register access lock
        EnterCriticalSection(
            &(Gbl_I2cProp[pDevice->Key.ApiIndex].Lock_RegAccess)
            );

        // Issue command & register value
        status_AA =
            aa_i2c_write(
                (Aardvark)PLX_PTR_TO_INT( pDevice->hDevice ),
                pDevice->Key.DeviceNumber,
                AA_I2C_NO_FLAGS,
                2 * sizeof(U32),
                (U8*)dataStream
                );

        // Release the register access lock
        LeaveCriticalSection(
            &(Gbl_I2cProp[pDevice->Key.ApiIndex].Lock_RegAccess)
            );

        if (status_AA == (2 * sizeof(U32)))
        {
            if (bRetryOnError)
            {
                // Get retry count
                bRetryOnError = I2C_RETRY_MAX_COUNT - bRetryOnError;

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
                Plx_sleep( I2C_RETRY_DELAY_MS );

                // Decrement retry count
                bRetryOnError--;

                // Check if reached final one
                if (bRetryOnError == 0)
                {
                    ErrorPrintf((
                        "ERROR: I2C WRITE failed (%04X@%02X St:%dB/8B O:%02Xh V:%08X)\n",
                        pDevice->Key.PlxChip, pDevice->Key.DeviceNumber, status_AA, offset, value
                        ));
                    status = PLX_STATUS_FAILED;
                }
            }
        }
    }
    while (bRetryOnError);

    return status;
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
    RegValue = PlxPci_PlxMappedRegisterRead( pDevice, 0x354, NULL );

    // Get active VS mask
    RegVSEnable = PlxPci_PlxMappedRegisterRead( pDevice, 0x358, NULL );

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
                PlxPci_PlxMappedRegisterRead(
                    pDevice,
                    0x360 + (i * sizeof(U32)),
                    NULL
                    );

            pMHProp->VS_UpstreamPortNum[i] = (U8)((RegValue >> 0) & 0x1F);

            // Get VS downstream port vector ([23:0])
            RegValue =
                PlxPci_PlxMappedRegisterRead(
                    pDevice,
                    0x380 + (i * sizeof(U32)),
                    NULL
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
    PlxPci_PlxMappedRegisterWrite(
        pDevice,
        0x380 + (VS_Source * sizeof(U32)),
        MHProp.VS_DownstreamPorts[VS_Source]
        );

    PlxPci_PlxMappedRegisterWrite(
        pDevice,
        0x380 + (VS_Dest * sizeof(U32)),
        MHProp.VS_DownstreamPorts[VS_Dest]
        );

    // Make sure destination VS is enabled
    if ((MHProp.VS_EnabledMask & (1 << VS_Dest)) == 0)
    {
        DebugPrintf(("Enable destination VS%d\n", VS_Dest));
        PlxPci_PlxMappedRegisterWrite(
            pDevice,
            0x358,
            MHProp.VS_EnabledMask | (1 << VS_Dest)
            );
    }

    // Reset source port if requested
    if (bResetSrc)
    {
        RegValue =
            PlxPci_PlxMappedRegisterRead(
                pDevice,
                0x3A0,
                NULL
                );

        // Put VS into reset
        PlxPci_PlxMappedRegisterWrite(
            pDevice,
            0x3A0,
            RegValue | (1 << VS_Source)
            );

        // Keep in reset for a short time
        Plx_sleep( 10 );

        // Take VS out of reset
        PlxPci_PlxMappedRegisterWrite(
            pDevice,
            0x3A0,
            RegValue & ~((U32)1 << VS_Source)
            );
    }

    return PLX_STATUS_OK;
}




/***********************************************************
 *
 *               PRIVATE SUPPORT FUNCTIONS
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
    U32                Address,
    BOOLEAN            bAdjustForPort
    )
{
    U8  portType;
    U8  bMultiNt;
    U8  bitPosMode;
    U8  bitPosStnSel;
    U8  bitPosPtSel;
    U8  portsPerStn;
    U8  highAddr;
    U32 mode;
    U32 stnSel;
    U32 portSel;
    U32 command;
    U32 regOffset;
    U32 highAddrOffset;
    U32 addr_Base;            // Provided offset port base
    U32 addr_NTV0Base;        // NT 0 Virtual regs base offset
    U32 addr_DmaBase;         // DMA regs base offset
    U32 addr_Stn0Base;        // Station 0 regs base offset


    // If port-based, add port base address to offset
    if (bAdjustForPort)
    {
        Address += pDevice->Key.ApiInternal[1];
    }

    // If within PEX region (60800000-60FFFFFF), remove base address
    if ( (pDevice->Key.PlxFamily == PLX_FAMILY_ATLAS) &&
         (bAdjustForPort == FALSE) &&
         ((Address & I2C_PEX_BASE_ADDR_MASK) == ATLAS_REGS_AXI_BASE_ADDR) )
    {
        Address &= I2C_PEX_MAX_OFFSET_MASK;
    }

    // Determine port base address
    addr_Base = Address & ~(U32)0xFFF;

    // Default special port base addresses to an always invalid address
    addr_NTV0Base = 0xFFFFFFFF;
    addr_DmaBase  = 0xFFFFFFFF;
    addr_Stn0Base = 0xFFFFFFFF;

    // Added to avoid compiler warning
    bitPosMode = 0;

    // Set default bit positions
    bitPosStnSel = 0;
    bitPosPtSel  = 15;

    // Set whether multi-NT is supported
    if ( ((pDevice->Key.PlxChip & 0xFF00) == 0x8700) ||
         ((pDevice->Key.PlxChip & 0xFF00) == 0x9700) )
    {
        bMultiNt = TRUE;
    }
    else
    {
        bMultiNt = FALSE;
    }

    // Set bit positions & address traps
    switch (pDevice->Key.PlxFamily)
    {
        case PLX_FAMILY_VEGA_LITE:
            bitPosMode    = 18;
            addr_NTV0Base = 0x10000;
            break;

        case PLX_FAMILY_ALTAIR:
        case PLX_FAMILY_ALTAIR_XL:
        case PLX_FAMILY_MIRA:
            // No NT port
            break;

        case PLX_FAMILY_DENEB:
        case PLX_FAMILY_SIRIUS:
            bitPosMode    = 19;
            bitPosStnSel  = 17;
            addr_NTV0Base = 0x10000;
            addr_DmaBase  = 0x20000;
            break;

        case PLX_FAMILY_CYGNUS:
            bitPosMode    = 20;
            bitPosStnSel  = 17;
            addr_NTV0Base = 0x3E000;
            break;

        case PLX_FAMILY_SCOUT:
        case PLX_FAMILY_DRACO_1:
        case PLX_FAMILY_DRACO_2:
            bitPosMode    = 20;
            bitPosStnSel  = 18;
            addr_NTV0Base = 0x3E000;
            addr_DmaBase  = 0x20000;
            break;

        case PLX_FAMILY_CAPELLA_1:
        case PLX_FAMILY_CAPELLA_2:
            bitPosMode    = 21;
            bitPosStnSel  = 18;
            addr_NTV0Base = 0x3E000;
            addr_Stn0Base = 0x80000;
            break;

        case PLX_FAMILY_ATLAS:
            bitPosMode   = 21;
            bitPosStnSel = 19;
            break;

        default:
            // Port will not have family set during initial probe
            bitPosMode   = 0;
            bitPosStnSel = 0;
            break;
    }

    mode    = I2C_ADDR_MODE_STD;
    stnSel  = 0;
    portSel = 0;

    // Default to transparent port
    portType = 0;

    // Trap offsets for special ports
    if (addr_Base == (addr_NTV0Base + 0x0))
    {
        portType = PLX_FLAG_PORT_NT_VIRTUAL_0;          // NTV 0
    }
    else if (addr_Base == (addr_NTV0Base + 0x1000))
    {
        portType = PLX_FLAG_PORT_NT_LINK_0;             // NTL 0
    }
    else if (bMultiNt && (addr_Base == (addr_NTV0Base - 0x2000)))
    {
        portType = PLX_FLAG_PORT_NT_VIRTUAL_1;          // NTV 1
    }
    else if (bMultiNt && (addr_Base == (addr_NTV0Base - 0x1000)))
    {
        portType = PLX_FLAG_PORT_NT_LINK_1;             // NTL 1
    }
    else if (addr_Base == (addr_DmaBase + 0x0))
    {
        portType = PLX_FLAG_PORT_DMA_RAM;               // DMA RAM
    }
    else if (addr_Base == (addr_DmaBase + 0x1000))
    {
        portType = PLX_FLAG_PORT_DMA_0;                 // DMA 0
    }
    else if (addr_Base == (addr_DmaBase + 0x2000))
    {
        portType = PLX_FLAG_PORT_DMA_1;                 // DMA 1
    }
    else if (addr_Base == (addr_DmaBase + 0x3000))
    {
        portType = PLX_FLAG_PORT_DMA_2;                 // DMA 2
    }
    else if (addr_Base == (addr_DmaBase + 0x4000))
    {
        portType = PLX_FLAG_PORT_DMA_3;                 // DMA 3
    }
    else if (addr_Base == (addr_Stn0Base + 0x0))
    {
        portType = PLX_FLAG_PORT_STN_REGS_S0;           // Station 0
    }
    else if (addr_Base == (addr_Stn0Base + 0x10000))
    {
        portType = PLX_FLAG_PORT_STN_REGS_S1;           // Station 1
    }
    else if (addr_Base == (addr_Stn0Base + 0x20000))
    {
        portType = PLX_FLAG_PORT_STN_REGS_S2;           // Station 2
    }
    else if (addr_Base == (addr_Stn0Base + 0x30000))
    {
        portType = PLX_FLAG_PORT_STN_REGS_S3;           // Station 3
    }
    else if (addr_Base == (addr_Stn0Base + 0x40000))
    {
        portType = PLX_FLAG_PORT_STN_REGS_S4;           // Station 4
    }
    else if (addr_Base == (addr_Stn0Base + 0x50000))
    {
        portType = PLX_FLAG_PORT_STN_REGS_S5;           // Station 5
    }
    else if ((Address & ~(U32)I2C_PEX_MAX_OFFSET_MASK) != 0)
    {
        // Address outside max I2C of 23-bits, default to station type
        portType = PLX_FLAG_PORT_STN_REGS_S0;
    }
    else
    {
        portSel = (Address >> 12);
    }

    // Handle chips where NT Virtual is accessed as transparent port
    if (portType == PLX_FLAG_PORT_NT_VIRTUAL_0)
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
                portType = 0;

                // Select NT port
                portSel = pDevice->Key.NTPortNum;
            }
        }
    }

    // Determine values
    switch (pDevice->Key.PlxFamily)
    {
        case PLX_FAMILY_VEGA_LITE:
            // Check if NT-Link port or within NT-Virtual port
            if ((portType == PLX_FLAG_PORT_NT_LINK_0) ||
                (portType == PLX_FLAG_PORT_NT_VIRTUAL_0) ||
                (addr_Base == ((U32)pDevice->Key.NTPortNum * 0x1000)))
            {
                if (portType == PLX_FLAG_PORT_NT_LINK_0)
                {
                    mode = I2C_ADDR_MODE_NTL;
                }

                // Bypass offsets F8h or FCh of NT port to avoid persistent I2C failure
                if (((Address & 0xFFF) == 0x0F8) || ((Address & 0xFFF) == 0x0FC))
                {
                    return I2C_CMD_SKIP;
                }
            }
            break;

        case PLX_FAMILY_ALTAIR:
        case PLX_FAMILY_ALTAIR_XL:
            // No NT port
            break;

        case PLX_FAMILY_MIRA:
            if (portType == PLX_FLAG_PORT_PCIE_TO_USB)
            {
                /*****************************************
                 * For MIRA special case, if the USB EP is
                 * selected & offset 4xxxh is accessed, the
                 * expectation is to access the USB device
                 * registers which are at offset 4000h.
                 ****************************************/
                if (addr_Base == 0x4000)
                {
                    portSel = 4;
                }
                else
                {
                    portSel = 3;
                }
            }
            else if (portType == PLX_FLAG_PORT_USB)
            {
                portSel = 4;
            }
            break;

        case PLX_FAMILY_DENEB:
            if (portType == PLX_FLAG_PORT_NT_LINK_0)
            {
                mode = I2C_ADDR_MODE_NTL;
            }
            break;

        case PLX_FAMILY_SIRIUS:
            // Default to special mode
            mode = I2C_ADDR_MODE_NON_STD;

            if (portType == PLX_FLAG_PORT_NT_LINK_0)
            {
                portSel = 0x10;
            }
            else if (portType == PLX_FLAG_PORT_NT_DS_P2P)
            {
                portSel = 0x11;
            }
            else if (portType == PLX_FLAG_PORT_DMA_0)
            {
                portSel = 0x12;
            }
            else if (portType == PLX_FLAG_PORT_DMA_RAM)
            {
                portSel = 0x13;
            }
            else
            {
                mode = I2C_ADDR_MODE_STD;   // Revert to transparent mode
            }
            break;

        case PLX_FAMILY_CYGNUS:
            if (portType == PLX_FLAG_PORT_NT_VIRTUAL_0)
            {
                stnSel  = 6;    // 110b
                portSel = 3;    // 11b
            }
            else if (portType == PLX_FLAG_PORT_NT_LINK_0)
            {
                mode    = I2C_ADDR_MODE_NTL;
                portSel = 0;    // 00b
            }
            else
            {
                stnSel  = portSel / 4;  // 6 stations (0-5), 4 ports each
                portSel = portSel % 4;  // Port in station (0-3)
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
                portsPerStn = 8;    // Only 6 ports per stn, but numbered as 8
            }
            else if ((pDevice->Key.PlxFamily == PLX_FAMILY_CAPELLA_1) ||
                     (pDevice->Key.PlxFamily == PLX_FAMILY_CAPELLA_2))
            {
                portsPerStn = 4;
            }
            else
            {
                ErrorPrintf(("ERRROR: Ports/Station not set for %04X\n", pDevice->Key.PlxChip));
                return I2C_CMD_ERROR;
            }

            // For Draco 1, some register cause problems if accessed
            if (pDevice->Key.PlxFamily == PLX_FAMILY_DRACO_1)
            {
                if ((Address == 0x856C)  || (Address == 0x8570) ||
                    (Address == 0x1056C) || (Address == 0x10570))
                {
                    return I2C_CMD_SKIP;
                }
            }

            // For Draco 2, reads of ALUT registers cause problems if accessed
            if ((pDevice->Key.PlxFamily == PLX_FAMILY_DRACO_2) &&
                (I2cOperation == I2C_CMD_REG_READ))
            {
                if ((portType == PLX_FLAG_PORT_ALUT_0) ||
                    (portType == PLX_FLAG_PORT_ALUT_1) ||
                    (portType == PLX_FLAG_PORT_ALUT_2) ||
                    (portType == PLX_FLAG_PORT_ALUT_3))
                {
                    DebugPrintf(("I2C: ALUT read causes issues, skipping\n"));
                    return I2C_CMD_SKIP;
                }
            }

            // GEP will show up as NTV0, so must use full address
            if ((pDevice->Key.DeviceMode == PLX_CHIP_MODE_FABRIC) &&
                (portType == PLX_FLAG_PORT_NT_VIRTUAL_0))
            {
                Address  = addr_NTV0Base | (Address & 0xFFF);
                portType = PLX_FLAG_PORT_STN_REGS_S0;
            }

            if (portType == PLX_FLAG_PORT_NT_VIRTUAL_0)
            {
                mode    = I2C_ADDR_MODE_NTV;
                portSel = 0;    // NT 0
            }
            else if (portType == PLX_FLAG_PORT_NT_VIRTUAL_1)
            {
                mode    = I2C_ADDR_MODE_NTV;
                portSel = 1;    // NT 1
            }
            else if (portType == PLX_FLAG_PORT_NT_LINK_0)
            {
                mode    = I2C_ADDR_MODE_NTL;
                portSel = 0;    // NT 0
            }
            else if (portType == PLX_FLAG_PORT_NT_LINK_1)
            {
                mode    = I2C_ADDR_MODE_NTL;
                portSel = 1;    // NT 1
            }
            else if (portType == PLX_FLAG_PORT_DMA_0)
            {
                mode    = I2C_ADDR_MODE_DMA;
                portSel = 0;    // DMA 0
            }
            else if (portType == PLX_FLAG_PORT_DMA_1)
            {
                mode    = I2C_ADDR_MODE_DMA;
                portSel = 1;    // DMA 1
            }
            else if (portType == PLX_FLAG_PORT_DMA_2)
            {
                mode    = I2C_ADDR_MODE_DMA;
                portSel = 2;    // DMA 2
            }
            else if (portType == PLX_FLAG_PORT_DMA_3)
            {
                mode    = I2C_ADDR_MODE_DMA;
                portSel = 3;    // DMA 3
            }
            else if (portType == PLX_FLAG_PORT_DMA_RAM)
            {
                mode    = I2C_ADDR_MODE_DMA;
                portSel = 4;    // DMA RAM
            }
            else if (portType == PLX_FLAG_PORT_ALUT_0)
            {
                mode    = I2C_ADDR_MODE_ALUT;
                stnSel  = 2;    // 010b
                portSel = 0;    // ALUT RAM 0
            }
            else if (portType == PLX_FLAG_PORT_ALUT_1)
            {
                mode    = I2C_ADDR_MODE_ALUT;
                stnSel  = 2;    // 010b
                portSel = 1;    // ALUT RAM 1
            }
            else if (portType == PLX_FLAG_PORT_ALUT_2)
            {
                mode    = I2C_ADDR_MODE_ALUT;
                stnSel  = 2;    // 010b
                portSel = 2;    // ALUT RAM 2
            }
            else if (portType == PLX_FLAG_PORT_ALUT_3)
            {
                mode    = I2C_ADDR_MODE_ALUT;
                stnSel  = 2;    // 010b
                portSel = 3;    // ALUT RAM 3
            }
            else if ((portType == PLX_FLAG_PORT_STN_REGS_S0) ||
                     (portType == PLX_FLAG_PORT_STN_REGS_S1) ||
                     (portType == PLX_FLAG_PORT_STN_REGS_S2) ||
                     (portType == PLX_FLAG_PORT_STN_REGS_S3) ||
                     (portType == PLX_FLAG_PORT_STN_REGS_S4) ||
                     (portType == PLX_FLAG_PORT_STN_REGS_S5))
            {
                mode    = I2C_ADDR_MODE_FULL;
                stnSel  = (Address >> 15) & 0x7; // Addr [17:15]
                portSel = (Address >> 12) & 0x7; // Addr [14:12]
            }
            else
            {
                stnSel  = portSel / portsPerStn; // Station number
                portSel = portSel % portsPerStn; // Port in station
            }
            break;

        case PLX_FAMILY_ATLAS:
            // Addressing PEX area (6080_0000h -> 60FF_FFFFh)
            if (addr_Base < (64 * PEX_PORT_REGS_SIZE))
            {
                // Accessing ports 0-63 uses standard addressing
                stnSel  = portSel / 16;    // Station number
                portSel = portSel % 16;    // Port in station
            }
            else
            {
                // Use full addressing mode
                mode = I2C_ADDR_MODE_FULL;

                // I2C commands max addressing is 23 bits for PEX region
                stnSel  = (addr_Base >> 16) & 0x3; // Addr [17:16]
                portSel = (addr_Base >> 12) & 0xF; // Addr [15:12]
            }
            break;

        case 0:
            // Family not set yet but still need command during initial probe
            break;

        default:
            ErrorPrintf(("ERRROR: Mode/PortSel/StnSel not set for %04X\n", pDevice->Key.PlxChip));
            return I2C_CMD_ERROR;
    }

    // Check if need to update high address reg in full address mode
    if (mode == I2C_ADDR_MODE_FULL)
    {
        // High address bits [22:18]
        highAddr = (U8)((addr_Base >> 18) & 0x1F);
        if (highAddr != I2C_KEY_HIGH_ADDR_GET( pDevice ))
        {
            // Set high address bits register (2CCh)
            highAddrOffset = I2C_HIGH_ADDR_OFFSET;
            if (pDevice->Key.PlxFamily == PLX_FAMILY_ATLAS)
            {
                // For Atlas, must specify PEX region (608x_xxxx)
                highAddrOffset += ATLAS_REGS_AXI_BASE_ADDR;
            }

            PlxI2c_PlxRegisterWrite(
                pDevice,
                highAddrOffset,
                highAddr,
                FALSE               // Adjust for port?
                );

            // Store high bits to avoid future write if no change
            I2C_KEY_HIGH_ADDR_SAVE( pDevice, highAddr );
        }
    }

    // Get offset within 4K port
    regOffset = Address & 0xFFF;

    // Convert offset to DWORD index
    regOffset = regOffset >> 2;

    // Build I2C command
    command =
        (0            <<           27) |  // Reserved [31:27]
        (I2cOperation <<           24) |  // Register operation [26:24]
        (mode         <<   bitPosMode) |  // Mode to select transp, NTV/NTL, DMA, etc
        (stnSel       << bitPosStnSel) |  // Station number if needed
        (portSel      <<  bitPosPtSel) |  // Port # or DMA channel or NT 0/1
        (0            <<           14) |  // Reserved [14]
        (0xF          <<           10) |  // Byte enables [13:10]
        (regOffset    <<            0);   // Register DWORD address [9:0]

    // 32-bit command must be in BE format since byte 0 is MSB
    command = PLX_BE_DATA_32( command );

    return command;
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
        I2C_KEY_HIGH_ADDR_RESET( pDevice );
    }

    // Clear handle in case of failure
    pDevice->hDevice = 0;

    // Verify port doesn't exceed limit
    if (pDevice->Key.ApiIndex >= I2C_MAX_DEVICES)
    {
        return FALSE;
    }

    // Reset global properties if first time
    if (Gbl_OpenCount == 0)
    {
        memset( Gbl_I2cProp, 0, sizeof(Gbl_I2cProp) );
    }

    // Check if device is already opened by the PLX API
    if (Gbl_I2cProp[pDevice->Key.ApiIndex].hDevice == 0)
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
        aa_i2c_bitrate(
            (Aardvark)PLX_PTR_TO_INT( pDevice->hDevice ),
            pDevice->Key.ApiInternal[0]
            );

        // Store the handle for re-use if necessary
        Gbl_I2cProp[pDevice->Key.ApiIndex].hDevice = pDevice->hDevice;

        // Reset upstream port bus number
        Gbl_I2cProp[pDevice->Key.ApiIndex].UpstreamBus = PCI_FIELD_IGNORE;

        // Reset NT port numbers
        memset( Gbl_I2cProp[pDevice->Key.ApiIndex].NTPortNum, PCI_FIELD_IGNORE, I2C_MAX_NT_PORTS );

        // Reset saved addresses
        I2C_KEY_HIGH_ADDR_RESET( pDevice );
        Gbl_I2cProp[pDevice->Key.ApiIndex].IdxLastAddr = (U32)-1;

        // Initialize the register access lock
        InitializeCriticalSection(
            &(Gbl_I2cProp[pDevice->Key.ApiIndex].Lock_RegAccess)
            );

        // Increment global open count
        Gbl_OpenCount++;
    }
    else
    {
        // Re-use existing handle
        pDevice->hDevice = Gbl_I2cProp[pDevice->Key.ApiIndex].hDevice;
    }

    // Increment open count
    InterlockedIncrement( &Gbl_I2cProp[pDevice->Key.ApiIndex].OpenCount );

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
                PlxDir_ChipTypeSet(
                    pDevice,
                    (U16)pIoBuffer->value[0],
                    (U8)pIoBuffer->value[1]
                    );
            break;

        case PLX_IOCTL_GET_PORT_PROPERTIES:
            DebugPrintf_Cont(("PLX_IOCTL_GET_PORT_PROPERTIES\n"));

            pIoBuffer->ReturnCode =
                PlxDir_GetPortProperties(
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
                PlxDir_PciBarProperties(
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
                PlxDir_PerformanceInitializeProperties(
                    pDevice,
                    PLX_INT_TO_PTR(pIoBuffer->value[0])
                    );
            break;

        case PLX_IOCTL_PERFORMANCE_MONITOR_CTRL:
            DebugPrintf_Cont(("PLX_IOCTL_PERFORMANCE_MONITOR_CTRL\n"));

            pIoBuffer->ReturnCode =
                PlxDir_PerformanceMonitorControl(
                    pDevice,
                    (PLX_PERF_CMD)pIoBuffer->value[0]
                    );
            break;

        case PLX_IOCTL_PERFORMANCE_RESET_COUNTERS:
            DebugPrintf_Cont(("PLX_IOCTL_PERFORMANCE_RESET_COUNTERS\n"));

            pIoBuffer->ReturnCode =
                PlxDir_PerformanceResetCounters(
                    pDevice
                    );
            break;

        case PLX_IOCTL_PERFORMANCE_GET_COUNTERS:
            DebugPrintf_Cont(("PLX_IOCTL_PERFORMANCE_GET_COUNTERS\n"));

            pIoBuffer->ReturnCode =
                PlxDir_PerformanceGetCounters(
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
                "Unsupported PLX_IOCTL_Xxx (%08Xh)\n",
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
    Gbl_I2cProp[pDevice->Key.ApiIndex].UpstreamBus = PCI_FIELD_IGNORE;
    memset( Gbl_I2cProp[pDevice->Key.ApiIndex].NTPortNum, PCI_FIELD_IGNORE, I2C_MAX_NT_PORTS );

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
                    PCI_REG_DEV_VEN_ID,
                    &status,
                    TRUE,       // Adjust for port?
                    FALSE       // Retry on error?
                    );
        }

        if ((status != PLX_STATUS_OK) || ((DevVenID & 0xFFFF) != PLX_PCI_VENDOR_ID_PLX))
        {
            goto _PlxI2c_ProbeSwitch_Next_Port;
        }

        DebugPrintf(("I2C: Port %d detected (ID=%08X)\n", Port, DevVenID));

        // Update possible ports
        if (PlxChip == 0)
        {
            // Set PLX chip type & revision
            if (pDevice->Key.PlxChip == 0)
            {
                // Set device ID in case needed by chip detection algorithm
                pDevice->Key.DeviceId = (U16)(DevVenID >> 16);

                // Probe to determine chip type
                PlxDir_ChipTypeDetect( pDevice );
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
                                    "I2C: Chip config: %d station%s, %d ports/stn\n",
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
                                Gbl_I2cProp[pDevice->Key.ApiIndex].NTPortNum[0] = (U8)((RegValue >> 8) & 0xF);
                                NtMask |= (1 << 0);
                            }

                            // Determine if NT1 is enabled & store port number
                            if (RegValue & (1 << 21))
                            {
                                Gbl_I2cProp[pDevice->Key.ApiIndex].NTPortNum[1] = (U8)((RegValue >> 16) & 0xF);
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
                                PCI_REG_HDR_CACHE_LN,
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
                            Gbl_I2cProp[pDevice->Key.ApiIndex].NTPortNum[0] = (U8)((RegDebugCtrl >> 24) & 0xF);

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
                    "I2C: ChipMode=%s  UP=%d  NT0=%02Xh  NT1=%02Xh\n",
                    (pDevice->Key.DeviceMode == PLX_CHIP_MODE_STANDARD) ? "STD" :
                      (pDevice->Key.DeviceMode == PLX_CHIP_MODE_STD_LEGACY_NT) ? "STD NT" :
                      (pDevice->Key.DeviceMode == PLX_CHIP_MODE_STD_NT_DS_P2P) ? "NT P2P" :
                      (pDevice->Key.DeviceMode == PLX_CHIP_MODE_VIRT_SW) ? "VS" :
                      (pDevice->Key.DeviceMode == PLX_CHIP_MODE_FABRIC) ? "FABRIC" :
                      (pDevice->Key.DeviceMode == PLX_CHIP_MODE_LEGACY_ADAPTER) ? "LEGACY" :
                      "UNKOWN",
                    Port_Upstream,
                    NtMask & (1 << 0) ? Gbl_I2cProp[pDevice->Key.ApiIndex].NTPortNum[0] : 0xFF,
                    NtMask & (1 << 1) ? Gbl_I2cProp[pDevice->Key.ApiIndex].NTPortNum[1] : 0xFF
                    ));
            }

            // Remove disabled ports to avoid probing them
            // NOTE: Must be called after Key.DeviceMode is set since may refer to it
            PlxDir_ChipFilterDisabledPorts(
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
                // Always enable NTV0 which is GEP
                PossiblePorts |= ((U64)1 << PLX_FLAG_PORT_NT_VIRTUAL_0);
            }
        }

        // Get PCI header
        RegPciHeader =
            PlxI2c_PlxRegisterRead(
                pDevice,
                PCI_REG_HDR_CACHE_LN,
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
            DebugPrintf(("I2C: Port %d is disabled, PCI[0Ch]=FFFF_FFFF\n", Port));
        }
        else if ((RegPciHeader & 0xFFFF) == PLX_PCI_VENDOR_ID_PLX)
        {
            // Some chips return value of last register (Dev/Ven ID)
            // read when accessing 0Ch of disabled port
            DebugPrintf(("I2C: Port %d is disabled, PCI[0Ch]=%08X (Dev/Ven ID)\n", Port, RegPciHeader));
        }
        else
        {
            DebugPrintf(("I2C: Port %d is enabled, get additional properties\n", Port));
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
            if (Gbl_I2cProp[pDevice->Key.ApiIndex].NTPortNum[0] != (U8)PCI_FIELD_IGNORE)
            {
                pDevice->Key.NTPortNum |= Gbl_I2cProp[pDevice->Key.ApiIndex].NTPortNum[0];
            }

            if (Gbl_I2cProp[pDevice->Key.ApiIndex].NTPortNum[1] != (U8)PCI_FIELD_IGNORE)
            {
                pDevice->Key.NTPortNum |= (Gbl_I2cProp[pDevice->Key.ApiIndex].NTPortNum[1] << 4);
            }

            // Get port properties
            PlxDir_GetPortProperties( pDevice, &PortProp );

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
                    (Port == Gbl_I2cProp[pDevice->Key.ApiIndex].NTPortNum[0]))
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
            if ((Gbl_I2cProp[pDevice->Key.ApiIndex].UpstreamBus == (U8)PCI_FIELD_IGNORE) &&
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
                            Offset_Upstream + PCI_REG_T1_PRIM_SEC_BUS,
                            &status,
                            FALSE,                  // Adjust for port?
                            TRUE                    // Retry on error?
                            );

                    if ((status == PLX_STATUS_OK) && (RegValue != (U32)-1))
                    {
                        // Set the upstream bus number
                        Gbl_I2cProp[pDevice->Key.ApiIndex].UpstreamBus = (U8)(RegValue >> 0);
                    }
                }
            }

            // Reset bus
            pDevice->Key.bus = 0;

            // Get bus number
            if (PciHeaderType == PCI_HDR_TYPE_1)
            {
                RegValue =
                    PlxI2c_PlxRegisterRead(
                        pDevice,
                        PCI_REG_T1_PRIM_SEC_BUS,
                        NULL,
                        TRUE,       // Adjust for port?
                        TRUE        // Retry on error?
                        );

                // Bus is primary bus number field (18h[7:0])
                pDevice->Key.bus = (U8)(RegValue >> 0);

                // Store the upstream port bus number
                if (pDevice->Key.PlxPortType == PLX_SPEC_PORT_UPSTREAM)
                {
                    Gbl_I2cProp[pDevice->Key.ApiIndex].UpstreamBus = pDevice->Key.bus;
                    DebugPrintf(("I2C: UP bus=%02X\n", pDevice->Key.bus));
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
                                (PortProp.PortNumber * 0x1000) + PCI_REG_T1_PRIM_SEC_BUS,
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
                if (Gbl_I2cProp[pDevice->Key.ApiIndex].UpstreamBus != (U8)PCI_FIELD_IGNORE)
                {
                    pDevice->Key.bus = Gbl_I2cProp[pDevice->Key.ApiIndex].UpstreamBus;
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
                    PCI_REG_CLASS_REV,
                    &status,
                    TRUE,       // Adjust for port?
                    TRUE        // Retry on error?
                    );

            pDevice->Key.Revision = (U8)RegValue;

            // Get subsystem device ID
            if (PciHeaderType == PCI_HDR_TYPE_0)
            {
                RegValue =
                    PlxI2c_PlxRegisterRead(
                        pDevice,
                        PCI_REG_TO_SUBSYS_ID,
                        &status,
                        TRUE,       // Adjust for port?
                        TRUE        // Retry on error?
                        );
            }
            else
            {
                // Get subsytem ID from capability if supported
                offset =
                    PlxDir_PciFindCapability(
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

            // Compare Bus, Slot, Fn numbers
            if ( (pKey->bus      != (U8)PCI_FIELD_IGNORE) ||
                 (pKey->slot     != (U8)PCI_FIELD_IGNORE) ||
                 (pKey->function != (U8)PCI_FIELD_IGNORE) )
            {
                if ( (pKey->bus      != pDevice->Key.bus)  ||
                     (pKey->slot     != pDevice->Key.slot) ||
                     (pKey->function != pDevice->Key.function) )
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
