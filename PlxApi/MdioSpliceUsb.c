/*******************************************************************************
 * Copyright 2013-2019 Broadcom Inc
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
 *      MdioSpliceUsb.c
 *
 * Description:
 *
 *      Implements the PLX API functions over a Splice MDIO USB interface
 *
 * Revision History:
 *
 *      01-01-19 : PCI/PCIe SDK v8.00
 *
 ******************************************************************************/

/*******************************************************************************
 *
 * Splice MDIO API utilization of fields in PLX_DEVICE_KEY
 *
 *  ApiIndex        - Splice USB device number (0,1,etc)
 *  DeviceMode      - Current chip mode (Std/Base, Fabric/Smart Switch, etc)
 *  PlxPort         - Port number
 *  PlxPortType     - PLX-specific port type (PLX_PORT_TYPE)
 *  ApiInternal[0]  - MDIO clock speed in KHz
 *  ApiInternal[1]  - Last accessed device & upper 16-bits of address
 *
 ******************************************************************************/


#include "PciRegs.h"
#include "PexApi.h"
#include "PlxApiDebug.h"
#include "PlxApiDirect.h"
#include "MdioSpliceUsb.h"




/**********************************************
 *           Global Variables
 *********************************************/
// Global handle to Splice dynamic library
static HINSTANCE Gbl_hSpliceLib = NULL;

// Global count of number of current open MDIO devices
static U8 Gbl_OpenCount = 0;

// MDIO functions dynamically loaded from the library
static Fn_UsbConnect      Gbl_Fn_UsbConnect     = NULL;
static Fn_UsbDisConnect   Gbl_Fn_UsbDisConnect  = NULL;
static Fn_UsbReadMdio     Gbl_Fn_UsbReadMdio    = NULL;
static Fn_UsbWriteMdio    Gbl_Fn_UsbWriteMdio   = NULL;
static Fn_UsbReadCfgReg   Gbl_Fn_UsbReadCfgReg  = NULL;
static Fn_UsbWriteCfgReg  Gbl_Fn_UsbWriteCfgReg = NULL;


// Per MDIO controller properties
static struct _MDIO_SPLICE_USB_PROP
{
    CRITICAL_SECTION  Lock_RegAccess;
    PLX_DRIVER_HANDLE hDevice;
    S32               OpenCount;
} Gbl_MdioProp[MDIO_MAX_DEVICES];


// MDIO access table to convert address -> MDIO command
static struct
{
    unsigned long  BaseAddr;
    unsigned long  EndAddr;
    unsigned char  PhyAddr;
    unsigned char  DevAddr;
    unsigned short SlaveAddr;
} MdioAccessTable[] =
{
   // Base Addr   End Addr    PHY   DEV   SLAV
    { 0x2A000000, 0x2A00FFFF, 0x04, 0x02, 0x00 },  // PBAM - Config & Aladin
    { 0x2A010000, 0x2A01FFFF, 0x04, 0x02, 0x01 },  // PBAM - TRNG
    { 0x2A020000, 0x2A027FFF, 0x04, 0x02, 0x02 },  // PBAM - APSHA
    { 0x2A028000, 0x2A02FFFF, 0x04, 0x02, 0x02 },  // PBAM - SRK
    { 0x2A030000, 0x2A03FFFF, 0x04, 0x02, 0x03 },  // PBAM - PWM
    { 0x2A040000, 0x2A04FFFF, 0x04, 0x02, 0x04 },  // PBAM - SGPIO
    { 0x2A050000, 0x2A05FFFF, 0x04, 0x02, 0x05 },  // PBAM - UART
    { 0x2A060000, 0x2A06FFFF, 0x04, 0x02, 0x06 },  // PBAM - I2C
    { 0x2A070000, 0x2A07FFFF, 0x04, 0x02, 0x07 },  // PBAM - LED
    { 0x2A080000, 0x2A09FFFF, 0x04, 0x02, 0x08 },  // PBAM - GPIO
    { 0x2A0A0000, 0x2A0BFFFF, 0x04, 0x02, 0x0A },  // PBAM - GPT
    { 0x2A0C0000, 0x2A0DFFFF, 0x04, 0x02, 0x0C },  // PBAM - SPI
// ??    { 0x2A0E0000, 0x2A0FFFFF, 0x0, 0x0, 0x0 },  // PBAM - Axi2Apb

    { 0x60000000, 0x60000FFF, 0x05, 0x04, 0x00 },  // PSB - Fusion
    { 0x60001000, 0x600047FF, 0x05, 0x04, 0x00 },  // PSB - Core
    { 0x60004800, 0x60004BFF, 0x05, 0x04, 0x00 },  // PSB - PBAPLB
    { 0x60004C00, 0x60007FFF, 0x05, 0x04, 0x00 },  // PSB - Generic
    { 0x60008000, 0x60008FFF, 0x05, 0x04, 0x00 },  // PSB - DCR
    { 0x60400000, 0x604003FF, 0x05, 0x02, 0x00 },  // PSB - Shell2Acelite
    { 0x60400400, 0x604007FF, 0x05, 0x02, 0x01 },  // PSB - Axi2Shell
    { 0x60400800, 0x60400BFF, 0x05, 0x02, 0x02 },  // PSB - Axi2ai_Base
    { 0x60400C00, 0x60400FFF, 0x05, 0x02, 0x06 },  // PSB - Axi2Apb
    { 0x60401000, 0x60401FFF, 0x05, 0x02, 0x04 },  // PSB - Axi2ai_PEPP
    { 0x60410000, 0x6041FFFF, 0x05, 0x06, 0x00 },  // PSB - PEPP
    { 0x60800000, 0x609FFFFF, 0x05, 0x06, 0x01 },  // PSB - PEX ports, GEP, etc

    { 0x64900000, 0x6490003F, 0x06, 0x02, 0x00 },  // OCM Bank 1 registers
    { 0x64940000, 0x6494003F, 0x06, 0x02, 0x00 },  // OCM Bank 2 registers

// ??  No slave addr    { 0x70000000, 0x7003FFFF, 0x07, 0x10, 0x0 },  // PSW0 - SERDES (lanes 0-15)
    { 0x70040000, 0x70043FFF, 0x07, 0x02, 0x04 },  // PSW0 - PIPE (lanes 0-3)
    { 0x70044000, 0x70047FFF, 0x07, 0x02, 0x05 },  // PSW0 - PIPE (lanes 4-7)
    { 0x70048000, 0x7004BFFF, 0x07, 0x02, 0x06 },  // PSW0 - PIPE (lanes 8-11)
    { 0x7004c000, 0x7004FFFF, 0x07, 0x02, 0x07 },  // PSW0 - PIPE (lanes 12-15)
    { 0x70050000, 0x70057FFF, 0x07, 0x02, 0x08 },  // PSW0 - CTRL_REG_LOWER
    { 0x70058000, 0x7005FFFF, 0x07, 0x02, 0x09 },  // PSW0 - CTRL_REG_UPPER

// ??  No slave addr    { 0x70100000, 0x7013FFFF, 0x08, 0x10, 0x0 },  // PSW1 - SERDES (lanes 0-15)
    { 0x70140000, 0x70143FFF, 0x08, 0x02, 0x04 },  // PSW1 - PIPE (lanes 0-3)
    { 0x70144000, 0x70147FFF, 0x08, 0x02, 0x05 },  // PSW1 - PIPE (lanes 4-7)
    { 0x70148000, 0x7014BFFF, 0x08, 0x02, 0x06 },  // PSW1 - PIPE (lanes 8-11)
    { 0x7014c000, 0x7014FFFF, 0x08, 0x02, 0x07 },  // PSW1 - PIPE (lanes 12-15)
    { 0x70150000, 0x70157FFF, 0x08, 0x02, 0x08 },  // PSW1 - CTRL_REG_LOWER
    { 0x70158000, 0x7015FFFF, 0x08, 0x02, 0x09 },  // PSW1 - CTRL_REG_UPPER

// ??  No slave addr    { 0x70200000, 0x7023FFFF, 0x09, 0x10, 0x0 },  // PSW2 - SERDES (lanes 0-15)
    { 0x70240000, 0x70243FFF, 0x09, 0x02, 0x04 },  // PSW2 - PIPE (lanes 0-3)
    { 0x70244000, 0x70247FFF, 0x09, 0x02, 0x05 },  // PSW2 - PIPE (lanes 4-7)
    { 0x70248000, 0x7024BFFF, 0x09, 0x02, 0x06 },  // PSW2 - PIPE (lanes 8-11)
    { 0x7024c000, 0x7024FFFF, 0x09, 0x02, 0x07 },  // PSW2 - PIPE (lanes 12-15)
    { 0x70250000, 0x70257FFF, 0x09, 0x02, 0x08 },  // PSW2 - CTRL_REG_LOWER
    { 0x70258000, 0x7025FFFF, 0x09, 0x02, 0x09 },  // PSW2 - CTRL_REG_UPPER

// ??  No slave addr    { 0x70300000, 0x7033FFFF, 0x0A, 0x10, 0x0 },  // PSW3 - SERDES (lanes 0-15)
    { 0x70340000, 0x70343FFF, 0x0A, 0x02, 0x04 },  // PSW3 - PIPE (lanes 0-3)
    { 0x70344000, 0x70347FFF, 0x0A, 0x02, 0x05 },  // PSW3 - PIPE (lanes 4-7)
    { 0x70348000, 0x7034BFFF, 0x0A, 0x02, 0x06 },  // PSW3 - PIPE (lanes 8-11)
    { 0x7034c000, 0x7034FFFF, 0x0A, 0x02, 0x07 },  // PSW3 - PIPE (lanes 12-15)
    { 0x70350000, 0x70357FFF, 0x0A, 0x02, 0x08 },  // PSW3 - CTRL_REG_LOWER
    { 0x70358000, 0x7035FFFF, 0x0A, 0x02, 0x09 },  // PSW3 - CTRL_REG_UPPER

// ??  No slave addr    { 0x70400000, 0x7043FFFF, 0x0B, 0x10, 0x0 },  // PSW4 - SERDES (lanes 0-15)
    { 0x70440000, 0x70443FFF, 0x0B, 0x02, 0x04 },  // PSW4 - PIPE (lanes 0-3)
    { 0x70444000, 0x70447FFF, 0x0B, 0x02, 0x05 },  // PSW4 - PIPE (lanes 4-7)
    { 0x70448000, 0x7044BFFF, 0x0B, 0x02, 0x06 },  // PSW4 - PIPE (lanes 8-11)
    { 0x7044c000, 0x7044FFFF, 0x0B, 0x02, 0x07 },  // PSW4 - PIPE (lanes 12-15)
    { 0x70450000, 0x70457FFF, 0x0B, 0x02, 0x08 },  // PSW4 - CTRL_REG_LOWER
    { 0x70458000, 0x7045FFFF, 0x0B, 0x02, 0x09 },  // PSW4 - CTRL_REG_UPPER

// ??  No slave addr    { 0x70500000, 0x7053FFFF, 0x0C, 0x10, 0x0 },  // PSW5 - SERDES (lanes 0-15)
    { 0x70540000, 0x70543FFF, 0x0C, 0x02, 0x04 },  // PSW5 - PIPE (lanes 0-3)
    { 0x70544000, 0x70547FFF, 0x0C, 0x02, 0x05 },  // PSW5 - PIPE (lanes 4-7)
    { 0x70548000, 0x7054BFFF, 0x0C, 0x02, 0x06 },  // PSW5 - PIPE (lanes 8-11)
    { 0x7054c000, 0x7054FFFF, 0x0C, 0x02, 0x07 },  // PSW5 - PIPE (lanes 12-15)
    { 0x70550000, 0x70557FFF, 0x0C, 0x02, 0x08 },  // PSW5 - CTRL_REG_LOWER
    { 0x70558000, 0x7055FFFF, 0x0C, 0x02, 0x09 },  // PSW5 - CTRL_REG_UPPER

// ?? No AXI addr    { 0x, 0x, 0x0D, 0x02, 0x0 },  // PSWX2: Axi2Apb0
// ?? No AXI addr    { 0x, 0x, 0x0D, 0x04, 0x0 },  // PSWX2: Axi2Apb1
// ?? No Slave addr  { 0x70F00000, 0x70F3FFFF, 0x0D, 0x10, 0x0 },  // PSWX2 - SERDES (lanes 0-15)

// ?? No Slave addr  { 0x29C00000, 0x29CFFFFF, 0x0F, 0x02, 0x0 },  // Secure Boot ROM

    { 0xFFE78000, 0xFFE79FFF, 0x0E, 0x1E, 0x00 },  // MXSSubsystemClk(psb_stn_top)
    { 0xFFE7A000, 0xFFE7BFFF, 0x02, 0x1E, 0x00 },  // Mxsbias0 Ax2Apb
    { 0xFFE7C000, 0xFFE7DFFF, 0x03, 0x1E, 0x00 },  // Mxsbias1 Ax2Apb
    { 0xFFE7E000, 0xFFE7FFFF, 0x01, 0x02, 0x03 },  // CCR - Efuse
    { 0xFFE80000, 0xFFEBFFFF, 0x01, 0x02, 0x02 },  // CCR - Watchdog
    { 0xFFEC0000, 0xFFEDFFFF, 0x01, 0x02, 0x01 },  // CCR - Aladin
    { 0xFFEE0000, 0xFFEFFFFF, 0x01, 0x02, 0x00 },  // CCR - Axi2apb
    { 0xFFF00000, 0xFFFEFFFF, 0x01, 0x02, 0x00 },  // CCR

    // Entry below must be final one
    { 0xFFFFFFFF, 0x0000000, 0, 0, 0 }
};




/******************************************************************************
 *
 * Function   :  MdioSplice_DeviceOpen
 *
 * Description:  Selects a device
 *
 *****************************************************************************/
PLX_STATUS
MdioSplice_DeviceOpen(
    PLX_DEVICE_OBJECT *pDevice
    )
{
    // Open connection to driver
    if (MdioSplice_Driver_Connect( pDevice, NULL ) == FALSE)
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
 * Function   :  MdioSplice_DeviceClose
 *
 * Description:  Closes a previously opened device
 *
 *****************************************************************************/
PLX_STATUS
MdioSplice_DeviceClose(
    PLX_DEVICE_OBJECT *pDevice
    )
{
    if (Gbl_MdioProp[pDevice->Key.ApiIndex].OpenCount <= 0)
    {
        return PLX_STATUS_INVALID_ACCESS;
    }

    if (Gbl_Fn_UsbDisConnect == NULL)
    {
        return PLX_STATUS_NOT_FOUND;
    }

    // Decrement open count and close device if no longer referenced
    if (InterlockedDecrement(
            &Gbl_MdioProp[pDevice->Key.ApiIndex].OpenCount
            ) == 0)
    {
        // Disconnect from device
        Gbl_Fn_UsbDisConnect( PLX_INT_TO_PTR( pDevice->hDevice ) );

        // Mark device is closed
        Gbl_MdioProp[pDevice->Key.ApiIndex].hDevice = 0;

        // Reset previous access parameters
        pDevice->Key.ApiInternal[1] =
            MDIO_ACCESS_ID( MDIO_ADDR_TABLE_IDX_INVALID, 0xFFFF );

        // Delete the register access lock
        DeleteCriticalSection(
            &(Gbl_MdioProp[pDevice->Key.ApiIndex].Lock_RegAccess)
            );

        // Decrement global open count
        Gbl_OpenCount--;

        // If no more MDIO devices are in use, unload dynamic library
        if (Gbl_OpenCount == 0)
        {
            // Clear all dynamic functions
            Gbl_Fn_UsbConnect     = NULL;
            Gbl_Fn_UsbDisConnect  = NULL;
            Gbl_Fn_UsbReadMdio    = NULL;
            Gbl_Fn_UsbWriteMdio   = NULL;
            Gbl_Fn_UsbReadCfgReg  = NULL;
            Gbl_Fn_UsbWriteCfgReg = NULL;

            // Release library
            FreeLibrary( Gbl_hSpliceLib );
            Gbl_hSpliceLib = NULL;
        }
    }

    return PLX_STATUS_OK;
}




/******************************************************************************
 *
 * Function   :  MdioSplice_DeviceFindEx
 *
 * Description:  Locates a specific device
 *
 *****************************************************************************/
PLX_STATUS
MdioSplice_DeviceFindEx(
    PLX_DEVICE_KEY *pKey,
    U16             DeviceNumber,
    PLX_MODE_PROP  *pModeProp
    )
{
    U16               numMatched;
    U16               totalMatches;
    U32               regVal;
    BOOLEAN           bFound;
    PLX_STATUS        status;
    PLX_DEVICE_OBJECT devObj;
    PLX_DEVICE_OBJECT devObjTemp;


    // Clear the device object
    RtlZeroMemory( &devObj, sizeof(PLX_DEVICE_OBJECT) );

    // Use default clock rate if not specified
    if (pModeProp->Mdio.ClockRate == 0)
    {
        pModeProp->Mdio.ClockRate = MDIO_DEFAULT_CLOCK_RATE;
    }

    // Open connection to driver
    if (MdioSplice_Driver_Connect( &devObj, pModeProp ) == FALSE)
    {
        return PLX_STATUS_INVALID_OBJECT;
    }

    totalMatches = 0;

    // Must validate object so probe API calls don't fail
    ObjectValidate( &devObj );

    // Default to device not found
    bFound = FALSE;

    // Probe devices to check if valid device
    DebugPrintf((" ---- Probe MDIO via USB %d ----\n", pModeProp->Mdio.Port));

    // Reset temporary device object
    RtlCopyMemory( &devObjTemp, &devObj, sizeof(PLX_DEVICE_OBJECT) );

    // Attempt to read Device/Vendor ID
    regVal =
        MdioSplice_PlxRegisterRead(
            &devObjTemp,
            PEX_REG_CCR_DEV_ID,     // Port 0 Device/Vendor ID
            &status,
            FALSE,                  // Adjust for port?
            FALSE                   // Retry on error?
            );

    if (status == PLX_STATUS_OK)
    {
        // Check if supported chip ID (eg C010_1000h)
        if ( (regVal & 0xFF00FFFF) == (0xC0000000 | PLX_PCI_VENDOR_ID_LSI) )
        {
            DebugPrintf(("MDIO: Detected device %08X\n", regVal));

            // Device found, determine active ports and compare
            status =
                PlxDir_ProbeSwitch(
                    &devObjTemp,
                    pKey,
                    (U16)(DeviceNumber - totalMatches),
                    &numMatched
                    );

            if (status == PLX_STATUS_OK)
            {
                bFound = TRUE;
            }
            else
            {
                // Add number of matched devices
                totalMatches += numMatched;
            }
        }
    }

    // Close the device
    MdioSplice_DeviceClose( &devObj );

    if (bFound)
    {
        // Store API mode
        pKey->ApiMode = PLX_API_MODE_MDIO_SPLICE;

        // Validate key
        ObjectValidate( pKey );
        return PLX_STATUS_OK;
    }

    return PLX_STATUS_INVALID_OBJECT;
}




/*******************************************************************************
 *
 * Function   :  MdioSplice_PlxRegisterRead
 *
 * Description:  Reads a PLX-specific register
 *
 ******************************************************************************/
U32
MdioSplice_PlxRegisterRead(
    PLX_DEVICE_OBJECT *pDevice,
    U32                offset,
    PLX_STATUS        *pStatus,
    BOOLEAN            bAdjustForPort,
    U16                bRetryOnError
    )
{
    U8  bStatusOk;
    U16 idx;
    U32 regVal;
    U32 mdioCmd;
    U32 mdioData;


    // Verify register offset
    if (offset & 0x3)
    {
        DebugPrintf(("ERROR - Invalid register offset (0x%x)\n", offset));
        if (pStatus != NULL)
        {
            *pStatus = PLX_STATUS_INVALID_OFFSET;
        }
        return MDIO_REG_READ_U32_FAIL_VAL;
    }

    // Default to error
    if (pStatus != NULL)
    {
        *pStatus = PLX_STATUS_FAILED;
    }

    // Verify DLL is loaded & required dynamic functions are loaded
    if ( (Gbl_hSpliceLib == NULL) || (pDevice->hDevice == 0) ||
         (Gbl_Fn_UsbReadMdio == NULL) || (Gbl_Fn_UsbWriteMdio == NULL) )
    {
        return MDIO_REG_READ_U32_FAIL_VAL;
    }

    // Adjust offset for port if requested
    if (bAdjustForPort)
    {
        // Adjust offset to port-specific register region
        offset += ATLAS_REGS_AXI_BASE_ADDR + (pDevice->Key.PlxPort * 0x1000);
    }

    // Get access parameters
    idx = MdioGetAccessTableIndex( offset );
    if (idx == MDIO_ADDR_TABLE_IDX_INVALID)
    {
        ErrorPrintf(("ERROR: No access defined for address %08X\n", offset));
        return MDIO_REG_READ_U32_FAIL_VAL;
    }

    // Assume successful access
    bStatusOk = TRUE;

    // Get the register access lock
    EnterCriticalSection(
        &(Gbl_MdioProp[pDevice->Key.ApiIndex].Lock_RegAccess)
        );

    //
    // Step 1: Setup the desired address
    //

    // For optimization, if the previous transaction had the same address phase, can skip
    if ( MDIO_ACCESS_ID( idx, (offset >> 16) ) != pDevice->Key.ApiInternal[1] )
    {
        // Build MDIO command for address phase
        mdioCmd =
            MDIO_CMD_BUILD(
                MdioAccessTable[idx].PhyAddr,
                MdioAccessTable[idx].DevAddr | (1 << 0),
                0
                );

        // MDIO data: [31:16]=Slave address  [15:0]=AXI address upper 16-bits
        mdioData = ((U32)MdioAccessTable[idx].SlaveAddr << 16) |
                   ((offset >> 16) & 0xFFFF);

        // Issue 32-bit address phase
        if (Gbl_Fn_UsbWriteMdio(
                PLX_INT_TO_PTR( pDevice->hDevice ),
                &mdioCmd,
                &mdioData,
                1,
                TRUE    // 32-bit MDIO transaction
                ) != MDIO_SPLICE_STATUS_OK)
        {
            bStatusOk = FALSE;
        }
        else
        {
            // Update address phase parameters for next transaction
            pDevice->Key.ApiInternal[1] = MDIO_ACCESS_ID( idx, (offset >> 16) );
        }
    }


    //
    // Step 2: Read 32-bit register
    //

    if (bStatusOk == TRUE)
    {
        // Build MDIO command for data phase ([15:0]=AXI address lower 16-bits)
        mdioCmd =
            MDIO_CMD_BUILD(
                MdioAccessTable[idx].PhyAddr,
                MdioAccessTable[idx].DevAddr,
                ((offset >> 0) & 0xFFFF)
                );

        // Issue 32-bit data phase
        if (Gbl_Fn_UsbReadMdio(
                PLX_INT_TO_PTR( pDevice->hDevice ),
                &mdioCmd,
                &regVal,
                1,
                TRUE
                ) != MDIO_SPLICE_STATUS_OK)
        {
            bStatusOk = FALSE;
        }
    }

    // Release the register access lock
    LeaveCriticalSection(
        &(Gbl_MdioProp[pDevice->Key.ApiIndex].Lock_RegAccess)
        );

    // Set return values
    if (bStatusOk)
    {
        if (pStatus != NULL)
        {
            *pStatus = PLX_STATUS_OK;
        }
    }
    else
    {
        if (pStatus != NULL)
        {
            *pStatus = PLX_STATUS_FAILED;
        }
        regVal = MDIO_REG_READ_U32_FAIL_VAL;
    }

    return regVal;
}




/*******************************************************************************
 *
 * Function   :  MdioSplice_PlxRegisterWrite
 *
 * Description:  Writes to a PLX-specific register
 *
 ******************************************************************************/
PLX_STATUS
MdioSplice_PlxRegisterWrite(
    PLX_DEVICE_OBJECT *pDevice,
    U32                offset,
    U32                value,
    BOOLEAN            bAdjustForPort
    )
{
    U16        idx;
    U32        mdioCmd;
    U32        mdioData;
    PLX_STATUS status;


    // Verify register offset
    if (offset & 0x3)
    {
        DebugPrintf(("ERROR - Invalid register offset (0x%x)\n", offset));
        return PLX_STATUS_INVALID_OFFSET;
    }

    // Verify DLL is loaded & required dynamic functions are loaded
    if ( (Gbl_hSpliceLib == NULL) || (pDevice->hDevice == 0) ||
         (Gbl_Fn_UsbReadMdio == NULL) || (Gbl_Fn_UsbWriteMdio == NULL) )
    {
        return MDIO_REG_READ_U32_FAIL_VAL;
    }

    // Adjust offset for port if requested
    if (bAdjustForPort)
    {
        // Adjust offset to port-specific register region
        offset += ATLAS_REGS_AXI_BASE_ADDR + (pDevice->Key.PlxPort * 0x1000);
    }

    // Get access parameters
    idx = MdioGetAccessTableIndex( offset );
    if (idx == MDIO_ADDR_TABLE_IDX_INVALID)
    {
        ErrorPrintf(("ERROR: No access defined for address %08X\n", offset));
        return MDIO_REG_READ_U32_FAIL_VAL;
    }

    // Assume successful access
    status = PLX_STATUS_OK;

    // Get the register access lock
    EnterCriticalSection(
        &(Gbl_MdioProp[pDevice->Key.ApiIndex].Lock_RegAccess)
        );

    //
    // Step 1: Setup the desired address
    //


    // For optimization, if the previous transaction had the same address phase, can skip
    if ( MDIO_ACCESS_ID( idx, (offset >> 16) ) != pDevice->Key.ApiInternal[1] )
    {
        // Build MDIO command for address phase
        mdioCmd =
            MDIO_CMD_BUILD(
                MdioAccessTable[idx].PhyAddr,
                MdioAccessTable[idx].DevAddr | (1 << 0),
                0
                );

        // MDIO data: [31:16]=Slave address  [15:0]=AXI address upper 16-bits
        mdioData = ((U32)MdioAccessTable[idx].SlaveAddr << 16) |
                   ((offset >> 16) & 0xFFFF);

        // Issue 32-bit address phase
        if (Gbl_Fn_UsbWriteMdio(
                PLX_INT_TO_PTR( pDevice->hDevice ),
                &mdioCmd,
                &mdioData,
                1,
                TRUE    // 32-bit MDIO transaction
                ) != MDIO_SPLICE_STATUS_OK)
        {
            status = PLX_STATUS_FAILED;
        }
        else
        {
            // Update address phase parameters for next transaction
            pDevice->Key.ApiInternal[1] = MDIO_ACCESS_ID( idx, (offset >> 16) );
        }
    }

    //
    // Step 2: Write 32-bit register
    //

    if (status == PLX_STATUS_OK)
    {
        // Build MDIO command for data phase ([15:0]=AXI address lower 16-bits)
        mdioCmd =
            MDIO_CMD_BUILD(
                MdioAccessTable[idx].PhyAddr,
                MdioAccessTable[idx].DevAddr,
                ((offset >> 0) & 0xFFFF)
                );

        // Issue 32-bit data phase
        if (Gbl_Fn_UsbWriteMdio(
                PLX_INT_TO_PTR( pDevice->hDevice ),
                &mdioCmd,
                &value,
                1,
                TRUE
                ) != MDIO_SPLICE_STATUS_OK)
        {
            status = PLX_STATUS_FAILED;
        }
    }

    // Release the register access lock
    LeaveCriticalSection(
        &(Gbl_MdioProp[pDevice->Key.ApiIndex].Lock_RegAccess)
        );

    return status;
}




/***********************************************************
 *
 *               PRIVATE SUPPORT FUNCTIONS
 *
 **********************************************************/


/******************************************************************************
 *
 * Function   :  MdioSplice_Driver_Connect
 *
 * Description:  Attempts to connect to the Splice MDIO driver
 *
 * Returns    :  TRUE   - Driver was found and connected to
 *               FALSE  - Driver not found
 *
 *****************************************************************************/
BOOLEAN
MdioSplice_Driver_Connect(
    PLX_DEVICE_OBJECT *pDevice,
    PLX_MODE_PROP     *pModeProp
    )
{
    U8    bDllLoadOk;
    char *strSplicePath;
    void *ptrHdlMdio;


    // Clear handle in case of failure
    pDevice->hDevice = 0;

    // Verify port doesn't exceed limit
    if (pDevice->Key.ApiIndex >= MDIO_MAX_DEVICES)
    {
        return FALSE;
    }

    // Load USB DLL library & functions if haven't yet
    if (Gbl_hSpliceLib == NULL)
    {
        // Reset global properties
        memset( Gbl_MdioProp, 0, sizeof(Gbl_MdioProp) );

        // Assume DLL loaded ok
        bDllLoadOk = TRUE;

        // Load the library
        Gbl_hSpliceLib = LoadLibrary( TEXT( MDIO_SPLICE_LIB_NAME ) );
        if (Gbl_hSpliceLib == NULL)
        {
            return FALSE;
        }

        // Get UsbConnect()
        Gbl_Fn_UsbConnect = (Fn_UsbConnect)GetProcAddress( Gbl_hSpliceLib, "UsbConnect" );
        if (Gbl_Fn_UsbConnect == NULL)
        {
            bDllLoadOk = FALSE;
        }

        // Get UsbDisConnect()
        if (bDllLoadOk)
        {
            Gbl_Fn_UsbDisConnect =
                (Fn_UsbDisConnect)GetProcAddress( Gbl_hSpliceLib, "UsbDisConnect" );
            if (Gbl_Fn_UsbDisConnect == NULL)
            {
                bDllLoadOk = FALSE;
            }
        }

        // Get UsbReadMdio()
        if (bDllLoadOk)
        {
            Gbl_Fn_UsbReadMdio =
                (Fn_UsbReadMdio)GetProcAddress( Gbl_hSpliceLib, "UsbReadMdio" );
            if (Gbl_Fn_UsbReadMdio == NULL)
            {
                bDllLoadOk = FALSE;
            }
        }

        // Get UsbWriteMdio()
        if (bDllLoadOk)
        {
            Gbl_Fn_UsbWriteMdio =
                (Fn_UsbWriteMdio)GetProcAddress( Gbl_hSpliceLib, "UsbWriteMdio" );
            if (Gbl_Fn_UsbWriteMdio == NULL)
            {
                bDllLoadOk = FALSE;
            }
        }

        // Get UsbReadCfgReg()
        if (bDllLoadOk)
        {
            Gbl_Fn_UsbReadCfgReg =
                (Fn_UsbReadCfgReg)GetProcAddress( Gbl_hSpliceLib, "UsbReadCfgReg" );
            if (Gbl_Fn_UsbReadCfgReg == NULL)
            {
                bDllLoadOk = FALSE;
            }
        }

        // Get UsbWriteCfgReg()
        if (bDllLoadOk)
        {
            Gbl_Fn_UsbWriteCfgReg =
                (Fn_UsbWriteCfgReg)GetProcAddress( Gbl_hSpliceLib, "UsbWriteCfgReg" );
            if (Gbl_Fn_UsbWriteCfgReg == NULL)
            {
                bDllLoadOk = FALSE;
            }
        }

        // Release library on error
        if (bDllLoadOk == FALSE)
        {
            Gbl_Fn_UsbConnect     = NULL;
            Gbl_Fn_UsbDisConnect  = NULL;
            Gbl_Fn_UsbReadMdio    = NULL;
            Gbl_Fn_UsbWriteMdio   = NULL;
            Gbl_Fn_UsbReadCfgReg  = NULL;
            Gbl_Fn_UsbWriteCfgReg = NULL;
            FreeLibrary( Gbl_hSpliceLib );
            Gbl_hSpliceLib = NULL;
            ErrorPrintf(("ERROR: Unable to load MDIO dynamic library\n"));
            return FALSE;
        }
    }

    // If mode properties supplied, copy into device object
    if (pModeProp != NULL)
    {
        pDevice->Key.ApiMode        = PLX_API_MODE_MDIO_SPLICE;
        pDevice->Key.ApiIndex       = (U8)pModeProp->Mdio.Port;
        pDevice->Key.ApiInternal[0] = pModeProp->Mdio.ClockRate;
    }

    // Check if device is already opened by the PLX API
    if (Gbl_MdioProp[pDevice->Key.ApiIndex].hDevice == 0)
    {
        // Use supplied path to Splice package, if provided
        if ( (pModeProp != NULL) && (pModeProp->Mdio.StrPath != NULL) )
        {
            strSplicePath = pModeProp->Mdio.StrPath;
        }
        else
        {
            strSplicePath = MDIO_SPLICE_ROOT_PATH_DEFAULT;
        }

        DebugPrintf_Cont(("\n"));
        DebugPrintf((
            "MDIO: Attempt connect via device %d (Path=%s)\n",
            pDevice->Key.ApiIndex, strSplicePath
            ));

        // Attempt connection to device
        if (Gbl_Fn_UsbConnect(
                &ptrHdlMdio,
                TEXT( strSplicePath ),
                "D6S",
                pDevice->Key.ApiIndex
                ) == MDIO_SPLICE_STATUS_ERROR)
        {
            ErrorPrintf(("ERROR: Unable to open MDIO device %d\n", pDevice->Key.ApiIndex));
            return FALSE;
        }

        // Store handle
        pDevice->hDevice = (PLX_DRIVER_HANDLE) PLX_PTR_TO_INT( ptrHdlMdio );

        // Store the handle for re-use if necessary
        Gbl_MdioProp[pDevice->Key.ApiIndex].hDevice = pDevice->hDevice;

        // Reset previous access parameters
        pDevice->Key.ApiInternal[1] =
            MDIO_ACCESS_ID( MDIO_ADDR_TABLE_IDX_INVALID, 0xFFFF );

        // Initialize the register access lock
        InitializeCriticalSection(
            &(Gbl_MdioProp[pDevice->Key.ApiIndex].Lock_RegAccess)
            );

        // Increment global open count
        Gbl_OpenCount++;
    }
    else
    {
        // Re-use existing handle
        pDevice->hDevice = Gbl_MdioProp[pDevice->Key.ApiIndex].hDevice;
    }

    // Increment open count
    InterlockedIncrement( &Gbl_MdioProp[pDevice->Key.ApiIndex].OpenCount );

    return TRUE;
}




/******************************************************************************
 *
 * Function   :  MdioSplice_Dispatch_IoControl
 *
 * Description:  Processes the IOCTL messages
 *
 ******************************************************************************/
S32
MdioSplice_Dispatch_IoControl(
    PLX_DEVICE_OBJECT *pDevice,
    U32                IoControlCode,
    PLX_PARAMS        *pIoBuffer,
    U32                Size
    )
{
    DebugPrintf_Cont(("\n"));
    DebugPrintf(("Received PLX Splice MDIO message ===> "));

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
                PlxDir_ChipTypeGet(
                    pDevice,
                    (U16*)&(pIoBuffer->value[0]),
                    (U8*)&(pIoBuffer->value[1])
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
                MdioSplice_PlxRegisterRead(
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
                MdioSplice_PlxRegisterWrite(
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
                MdioSplice_PlxRegisterRead(
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
                MdioSplice_PlxRegisterWrite(
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
                MdioSplice_PlxRegisterRead(
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
                MdioSplice_PlxRegisterWrite(
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




/*******************************************************************************
 *
 * Function   : MdioGetAccessTableIndex
 *
 * Description:
 *
 ******************************************************************************/
U16
MdioGetAccessTableIndex(
    U32 Address
    )
{
    int idx;


    idx = 0;
    while ( MdioAccessTable[idx].BaseAddr != (U32)-1 )
    {
        // Check if address falls within range
        if ( (Address >= MdioAccessTable[idx].BaseAddr) &&
             (Address <= MdioAccessTable[idx].EndAddr) )
        {
            return idx;
        }

        // Next entry
        idx++;
    }

    return MDIO_ADDR_TABLE_IDX_INVALID;
}
