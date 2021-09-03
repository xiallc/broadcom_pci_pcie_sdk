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
 *      Eep_8000.c
 *
 * Description:
 *
 *      This file contains 8000-series EEPROM support functions
 *
 * Revision History:
 *
 *      01-01-18 : PLX SDK v8.00
 *
 ******************************************************************************/


#include "Eep_8000.h"
#include "PlxApi.h"

#if defined(PLX_DOS)
    #include "ChipFunc.h"
    #include "SuppFunc.h"
#else
    #include "PlxApiDebug.h"
    #include "PlxApiDirect.h"
#endif




/******************************************************************************
 *
 * Function   :  Plx8000_EepromPresent
 *
 * Description:  Returns the state of the EEPROM as reported by the PLX device
 *
 ******************************************************************************/
PLX_STATUS
Plx8000_EepromPresent(
    PLX_DEVICE_OBJECT *pdx,
    U8                *pStatus
    )
{
    U16 OffsetCtrl;
    U32 RegValue;


    // Get EEPROM control register offset
    OffsetCtrl = Plx8000_EepromGetCtrlOffset( pdx );

    // Verify access is supported
    if (OffsetCtrl == 0x0)
    {
        *pStatus = PLX_EEPROM_STATUS_NONE;
        return PLX_STATUS_UNSUPPORTED;
    }

    // Get EEPROM Control/Status
    RegValue = PlxPci_PlxMappedRegisterRead( pdx, OffsetCtrl, NULL );

    // Check if an EEPROM is present (bit 16)
    if (RegValue & (1 << 16))
    {
        // Check if there is a CRC error or EEPROM is blank
        if (RegValue & (1 << 17))
        {
            *pStatus = PLX_EEPROM_STATUS_CRC_ERROR;
        }
        else
        {
            *pStatus = PLX_EEPROM_STATUS_VALID;
        }
    }
    else
    {
        *pStatus = PLX_EEPROM_STATUS_NONE;
    }

    return PLX_STATUS_OK;
}




/*******************************************************************************
 *
 * Function   :  Plx8000_EepromGetAddressWidth
 *
 * Description:  Returns the current EEPROM address width
 *
 ******************************************************************************/
PLX_STATUS
Plx8000_EepromGetAddressWidth(
    PLX_DEVICE_OBJECT *pdx,
    U8                *pWidth
    )
{
    U16 OffsetCtrl;
    U32 RegValue;


    // Default to unknown width
    *pWidth = 0;

    // Get EEPROM control register offset
    OffsetCtrl = Plx8000_EepromGetCtrlOffset( pdx );

    // Verify access is supported
    if (OffsetCtrl == 0x0)
    {
        return PLX_STATUS_UNSUPPORTED;
    }

    // Get EEPROM Control/Status
    RegValue = PlxPci_PlxMappedRegisterRead( pdx, OffsetCtrl, NULL );

    *pWidth = (U8)(RegValue >> 22) & 0x3;

    if (*pWidth == 0xFF)
    {
        *pWidth = 0;
    }

    return PLX_STATUS_OK;
}




/*******************************************************************************
 *
 * Function   :  Plx8000_EepromSetAddressWidth
 *
 * Description:  Sets a new EEPROM address width
 *
 ******************************************************************************/
PLX_STATUS
Plx8000_EepromSetAddressWidth(
    PLX_DEVICE_OBJECT *pdx,
    U8                 width
    )
{
    U16 OffsetCtrl;
    U32 RegValue;


    // Get EEPROM control register offset
    OffsetCtrl = Plx8000_EepromGetCtrlOffset( pdx );

    // Verify access is supported
    if (OffsetCtrl == 0x0)
    {
        return PLX_STATUS_UNSUPPORTED;
    }

    // Get EEPROM Control/Status
    RegValue = PlxPci_PlxMappedRegisterRead( pdx, OffsetCtrl, NULL );

    // Clear command field [15:13] to avoid EEPROM cycle
    RegValue &= ~(7 << 13);

    // Set address width override enable
    RegValue |= (1 << 21);

    // Enable override
    PlxPci_PlxMappedRegisterWrite( pdx, OffsetCtrl, RegValue );

    // Verify width is overridable
    RegValue = PlxPci_PlxMappedRegisterRead( pdx, OffsetCtrl, NULL );

    if ((RegValue & (1 << 21)) == 0)
    {
        DebugPrintf(("ERROR - EEPROM width override not supported\n"));
        return PLX_STATUS_UNSUPPORTED;
    }

    // Clear command field [15:13] to avoid EEPROM cycle
    RegValue &= ~(7 << 13);

    // Clear wdith field [23:22]
    RegValue &= ~(3 << 22);

    // Set address width [23:22]
    RegValue |= ((width & 0x3) << 22);

    // Set new address width
    PlxPci_PlxMappedRegisterWrite( pdx, OffsetCtrl, RegValue );

    return PLX_STATUS_OK;
}




/******************************************************************************
 *
 * Function   :  Plx8000_EepromCrcGet
 *
 * Description:  Returns the current value of the CRC and its status
 *
 ******************************************************************************/
PLX_STATUS
Plx8000_EepromCrcGet(
    PLX_DEVICE_OBJECT *pdx,
    U32               *pCrc,
    U8                *pCrcStatus
    )
{
    U16 OffsetCrc;
    U16 Value_16;
    U32 Value_32;


    // Set default return values
    *pCrc       = 0;
    *pCrcStatus = PLX_CRC_UNSUPPORTED;

    // Determine the CRC EEPROM offset
    switch (pdx->Key.PlxFamily)
    {
        case PLX_FAMILY_BRIDGE_PCIE_P2P:
            // 8111/8112 don't support CRC
            if (pdx->Key.PlxChip != 0x8114)
            {
                return PLX_STATUS_UNSUPPORTED;
            }

            if (pdx->Key.PlxRevision >= 0xBA)
            {
                OffsetCrc = 0x3EC;
            }
            else
            {
                OffsetCrc = 0x378;
            }
            break;

        case PLX_FAMILY_VEGA_LITE:
            OffsetCrc = 0x78F * sizeof(U32);
            break;

        case PLX_FAMILY_VEGA:
            OffsetCrc = 0xBE4 * sizeof(U32);
            break;

        case PLX_FAMILY_DRACO_2:
        case PLX_FAMILY_CAPELLA_1:
        case PLX_FAMILY_CAPELLA_2:
            // Determine EEPROM size to get CRC location
            Plx8000_EepromReadByOffset( pdx, 0, &Value_32 );

            // Verify valid size
            if ((Value_32 >> 16) == 0xFFFF)
            {
                DebugPrintf(("ERROR - EEPROM byte count invalid\n"));
                *pCrcStatus = PLX_CRC_INVALID;
                return PLX_STATUS_INVALID_DATA;
            }

            // CRC location after reg addr/data values
            OffsetCrc = (U16)(sizeof(U32) + (Value_32 >> 16));
            break;

        default:
            DebugPrintf(("ERROR - Device (%04X) doesn't support CRC\n", pdx->Key.PlxChip));
            return PLX_STATUS_UNSUPPORTED;
    }

    // Read CRC from EEPROM
    if (((pdx->Key.PlxChip & 0xFF00) == 0x8100) ||
        ((pdx->Key.PlxChip & 0xFF00) == 0x8500))
    {
        Plx8000_EepromReadByOffset( pdx, OffsetCrc, pCrc );
    }
    else
    {
        Plx8000_EepromReadByOffset_16( pdx, OffsetCrc, &Value_16 );
        *pCrc = Value_16;
        Plx8000_EepromReadByOffset_16( pdx, OffsetCrc + sizeof(U16), &Value_16 );
        *pCrc |= (U32)(Value_16) << 16;
    }

    // Get EEPROM and CRC status
    Value_32 = PlxPci_PlxMappedRegisterRead( pdx, 0x260, NULL );

    // Get EEPROM status bits [17:16]
    Value_32 = (Value_32 >> 16) & 0x3;

    if (Value_32 == 1)
    {
        *pCrcStatus = PLX_CRC_VALID;
    }
    else
    {
        *pCrcStatus = PLX_CRC_INVALID;
    }

    DebugPrintf((
        "CRC = %08Xh [%s] (offset=%02Xh)\n",
        (int)*pCrc,
        (*pCrcStatus == PLX_CRC_VALID) ? "Valid" : "Invalid or Undetermined",
        OffsetCrc
        ));

    return PLX_STATUS_OK;
}




/******************************************************************************
 *
 * Function   :  Plx8000_EepromCrcUpdate
 *
 * Description:  Calculates and updates the CRC value in the EEPROM
 *
 ******************************************************************************/
PLX_STATUS
Plx8000_EepromCrcUpdate(
    PLX_DEVICE_OBJECT *pdx,
    U32               *pCrc,
    BOOLEAN            bUpdateEeprom
    )
{
    U16 offset;
    U16 OffsetCrc;
    U16 OffsetCalcStart;
    U16 Value_16;
    U32 Crc;
    U32 Value_32;


    // Set starting offset for CRC calculations
    OffsetCalcStart = 0;

    // Determine the CRC EEPROM offset
    switch (pdx->Key.PlxFamily)
    {
        case PLX_FAMILY_BRIDGE_PCIE_P2P:
            // 8111/8112 don't support CRC
            if (pdx->Key.PlxChip != 0x8114)
            {
                return PLX_STATUS_UNSUPPORTED;
            }

            if (pdx->Key.PlxRevision >= 0xBA)
            {
                OffsetCrc = 0x3EC;
            }
            else
            {
                OffsetCrc = 0x378;
            }
            break;

        case PLX_FAMILY_VEGA_LITE:
            OffsetCrc = 0x78F * sizeof(U32);
            break;

        case PLX_FAMILY_VEGA:
            OffsetCrc = 0xBE4 * sizeof(U32);
            break;

        case PLX_FAMILY_DRACO_2:
        case PLX_FAMILY_CAPELLA_1:
        case PLX_FAMILY_CAPELLA_2:
            // Determine EEPROM size to get CRC location
            Plx8000_EepromReadByOffset( pdx, 0, &Value_32 );

            // Verify valid size
            if ((Value_32 >> 16) == 0xFFFF)
            {
                DebugPrintf(("ERROR - EEPROM byte count invalid\n"));
                return PLX_STATUS_INVALID_DATA;
            }

            // CRC location after EEPROM (header + reg addr/data values)
            OffsetCrc = (U16)(sizeof(U32) + (Value_32 >> 16));

            // CRC calculation starts at byte 2
            OffsetCalcStart = 2;
            break;

        default:
            DebugPrintf(("ERROR - Device (%04X) doesn't support CRC\n", pdx->Key.PlxChip));
            return PLX_STATUS_UNSUPPORTED;
    }

    // Initialize CRC
    Crc = (U32)-1;

    // Calculate CRC by reading all values in EEPROM
    for (offset = OffsetCalcStart; offset < OffsetCrc; offset += sizeof(U32))
    {
        // Read next EEPROM value
        if (offset & 0x3)
        {
            // EEPROM offsets are not DWord aligned, must build 16-bits at a time
            Plx8000_EepromReadByOffset_16( pdx, offset, &Value_16 );
            Value_32 = Value_16;

            // If final data not aligned on DWord, pad with 0's
            if ((offset + sizeof(U16)) < OffsetCrc)
            {
                Plx8000_EepromReadByOffset_16( pdx, offset + sizeof(U16), &Value_16 );
            }
            else
            {
                Value_16 = 0;
            }
            Value_32 |= ((U32)Value_16) << 16;
        }
        else
        {
            Plx8000_EepromReadByOffset( pdx, offset, &Value_32 );
        }

        // Update the CRC
        Plx8000_EepromComputeNextCrc( &Crc, Value_32 );
    }

    DebugPrintf(("Calculated CRC = %08Xh (offset=%02Xh)\n", (int)Crc, OffsetCrc));

    // Update CRC in EEPROM if requested
    if (bUpdateEeprom)
    {
        DebugPrintf(("Write new CRC to EEPROM\n"));
        if (OffsetCrc & 0x3)
        {
            Plx8000_EepromWriteByOffset_16( pdx, OffsetCrc, (U16)Crc );
            Plx8000_EepromWriteByOffset_16( pdx, OffsetCrc + sizeof(U16), (U16)(Crc >> 16) );
        }
        else
        {
            Plx8000_EepromWriteByOffset( pdx, OffsetCrc, Crc );
        }
    }
    else
    {
        DebugPrintf(("Skipping CRC update in EEPROM\n"));
    }

    // Return the new CRC
    *pCrc = Crc;

    return PLX_STATUS_OK;
}




/******************************************************************************
 *
 * Function   :  Plx8000_EepromReadByOffset
 *
 * Description:  Read a 32-bit value from the EEPROM at a specified offset
 *
 ******************************************************************************/
PLX_STATUS
Plx8000_EepromReadByOffset(
    PLX_DEVICE_OBJECT *pdx,
    U32                offset,
    U32               *pValue
    )
{
    U16 OffsetCtrl;
    U32 EepWidth;
    U32 RegValue;
    U32 RegUpper;


    // Clear return data
    *pValue = 0;

    // Get EEPROM control register offset
    OffsetCtrl = Plx8000_EepromGetCtrlOffset( pdx );

    // Verify access is supported
    if (OffsetCtrl == 0x0)
    {
        return PLX_STATUS_UNSUPPORTED;
    }

    // Wait until EEPROM is ready
    if (Plx8000_EepromWaitIdle( pdx ) == FALSE)
    {
        return PLX_STATUS_TIMEOUT;
    }

    // Get EEPROM control register
    RegValue = PlxPci_PlxMappedRegisterRead( pdx, OffsetCtrl, NULL );

    // Some devices don't report byte addressing
    if ((pdx->Key.PlxChip == 0x8114) ||
        (pdx->Key.PlxFamily == PLX_FAMILY_VEGA) ||
        (pdx->Key.PlxFamily == PLX_FAMILY_VEGA_LITE))
    {
        EepWidth = 2;
    }
    else
    {
        // Determine byte addressing ([23:22])
        EepWidth = (RegValue >> 22) & 0x3;
        if (EepWidth == 0)
        {
           EepWidth = 1;
        }
    }

    // Verify offset doesn't exceed byte addressing
    if (offset >= ((U32)1 << (EepWidth * 8)))
    {
        DebugPrintf((
            "ERROR - Offset (%02Xh) exceeds %dB addressing\n",
            (int)offset, (int)EepWidth
            ));
        return PLX_STATUS_INVALID_OFFSET;
    }

    // For 3-byte addressing, set upper byte
    if (EepWidth == 3)
    {
        RegUpper = PlxPci_PlxMappedRegisterRead( pdx, OffsetCtrl + 0xC, NULL );

        // Set 3rd address byte (26Ch[7:0])
        RegUpper &= ~(0xFF << 0);
        RegUpper |= (offset >> 16) & 0xFF;

        PlxPci_PlxMappedRegisterWrite( pdx, OffsetCtrl + 0xC, RegUpper );
    }

    // Convert offset to an index
    offset = (offset / sizeof(U32));

    // Clear command field [15:13]
    RegValue &= ~(7 << 13);

    // Clear offset field [20,12:0]
    RegValue &= ~((1 << 20) | (0x1FFF << 0));

    // Prepare EEPROM read command
    RegValue |=
        ((offset & 0x1FFF)    <<  0) |     // Bits [12:0] of offset
        (((offset >> 13) & 1) << 20) |     // Bit 13 of offset
        (PLX8000_EE_CMD_READ  << 13);      // EEPROM command

    // Send EEPROM command
    Plx8000_EepromSendCommand( pdx, RegValue );

    // Return EEPROM data (x264h)
    *pValue = PlxPci_PlxMappedRegisterRead( pdx, OffsetCtrl + 0x4, NULL );

    return PLX_STATUS_OK;
}




/******************************************************************************
 *
 * Function   :  Plx8000_EepromWriteByOffset
 *
 * Description:  Write a 32-bit value to the EEPROM at a specified offset
 *
 ******************************************************************************/
PLX_STATUS
Plx8000_EepromWriteByOffset(
    PLX_DEVICE_OBJECT *pdx,
    U32                offset,
    U32                value
    )
{
    U16 OffsetCtrl;
    U32 EepWidth;
    U32 RegValue;
    U32 RegUpper;


    // Get EEPROM control register offset
    OffsetCtrl = Plx8000_EepromGetCtrlOffset( pdx );

    // Verify access is supported
    if (OffsetCtrl == 0x0)
    {
        return PLX_STATUS_UNSUPPORTED;
    }

    // Wait until EEPROM is ready
    if (Plx8000_EepromWaitIdle( pdx ) == FALSE)
    {
        return PLX_STATUS_TIMEOUT;
    }

    // Get EEPROM control register
    RegValue = PlxPci_PlxMappedRegisterRead( pdx, OffsetCtrl, NULL );

    // Some devices don't report byte addressing
    if ((pdx->Key.PlxChip == 0x8114) ||
        (pdx->Key.PlxFamily == PLX_FAMILY_VEGA) ||
        (pdx->Key.PlxFamily == PLX_FAMILY_VEGA_LITE))
    {
        EepWidth = 2;
    }
    else
    {
        // Determine byte addressing ([23:22])
        EepWidth = (RegValue >> 22) & 0x3;
        if (EepWidth == 0)
        {
           EepWidth = 1;
        }
    }

    // Verify offset doesn't exceed byte addressing
    if (offset >= ((U32)1 << (EepWidth * 8)))
    {
        DebugPrintf((
            "ERROR - Offset (%02Xh) exceeds %dB addressing\n",
           (int)offset, (int)EepWidth
           ));
        return PLX_STATUS_INVALID_OFFSET;
    }

    // For 3-byte addressing, set upper byte
    if (EepWidth == 3)
    {
        RegUpper = PlxPci_PlxMappedRegisterRead( pdx, OffsetCtrl + 0xC, NULL );

        // Set 3rd address byte (26Ch[7:0])
        RegUpper &= ~(0xFF << 0);
        RegUpper |= (offset >> 16) & 0xFF;

        PlxPci_PlxMappedRegisterWrite( pdx, OffsetCtrl + 0xC, RegUpper );
    }

    // Convert offset to an index
    offset = (offset / sizeof(U32));

    // Clear command field [15:13]
    RegValue &= ~(7 << 13);

    // Clear offset field [20,12:0]
    RegValue &= ~((1 << 20) | (0x1FFF << 0));

    // Send EEPROM write enable command
    Plx8000_EepromSendCommand(
        pdx,
        RegValue | (PLX8000_EE_CMD_WRITE_ENABLE << 13)
        );

    // Prepare EEPROM data (264h)
    PlxPci_PlxMappedRegisterWrite( pdx, OffsetCtrl + 0x4, value );

    // Prepare EEPROM write command
    RegValue |=
        ((offset & 0x1FFF)    <<  0) |     // Bits [12:0] of offset
        (((offset >> 13) & 1) << 20) |     // Bit 13 of offset
        (PLX8000_EE_CMD_WRITE << 13);      // EEPROM command

    // Send EEPROM write command
    Plx8000_EepromSendCommand( pdx, RegValue );

    return PLX_STATUS_OK;
}




/******************************************************************************
 *
 * Function   :  Plx8000_EepromReadByOffset_16
 *
 * Description:  Read a 16-bit value from the EEPROM at a specified offset
 *
 ******************************************************************************/
PLX_STATUS
Plx8000_EepromReadByOffset_16(
    PLX_DEVICE_OBJECT *pdx,
    U32                offset,
    U16               *pValue
    )
{
    U32        Value_32;
    PLX_STATUS status;


    // Clear return data
    *pValue = 0;

    // Get 32-bit value
    status = Plx8000_EepromReadByOffset( pdx, (offset & ~0x3), &Value_32 );
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
        *pValue = (U16)Value_32;
    }

    return PLX_STATUS_OK;
}




/******************************************************************************
 *
 * Function   :  Plx8000_EepromWriteByOffset_16
 *
 * Description:  Write a 16-bit value to the EEPROM at a specified offset
 *
 ******************************************************************************/
PLX_STATUS
Plx8000_EepromWriteByOffset_16(
    PLX_DEVICE_OBJECT *pdx,
    U32                offset,
    U16                value
    )
{
    U32        Value_32;
    PLX_STATUS status;


    // Get current 32-bit value
    status = Plx8000_EepromReadByOffset( pdx, (offset & ~0x3), &Value_32 );
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
    return Plx8000_EepromWriteByOffset( pdx, (offset & ~0x3), Value_32 );
}




/******************************************************************************
 *
 * Function   :  Plx8000_EepromWaitIdle
 *
 * Description:  Wait until the EEPROM access is idle
 *
 ******************************************************************************/
BOOLEAN
Plx8000_EepromWaitIdle(
    PLX_DEVICE_OBJECT *pdx
    )
{
    U16 OffsetCtrl;
    U32 Timeout;
    U32 RegValue;
    U32 RegCmd;


    // Get EEPROM control register offset
    OffsetCtrl = Plx8000_EepromGetCtrlOffset( pdx );

    // Verify access is supported
    if (OffsetCtrl == 0x0)
    {
        return FALSE;
    }

    // Get EEPROM control register
    RegCmd = PlxPci_PlxMappedRegisterRead( pdx, OffsetCtrl, NULL );

    // Clear command field [15:13]
    RegCmd &= ~(7 << 13);

    // Clear the EepRdy bit [24]
    RegCmd &= ~(1 << 24);

    // Clear the EepWrStatus bits[30:28]
    RegCmd &= ~(7 << 28);

    // Check if there is initial CRC error reported ([19])
    if (RegCmd & (1 << 19))
    {
        /**********************************************************
         * In event of CRC error on power up, the error must be
         * cleared by writing '1'. In addition, a dummy EEPROM read
         * command might be needed to "unblock" the EEPROM controller
         * (mainly over I2C), along with a small delay to let it
         * complete. Without this dummy command, the 1st EEPROM read
         * command will fail.
         *********************************************************/
        PlxPci_PlxMappedRegisterWrite( pdx, OffsetCtrl, RegCmd | (PLX8000_EE_CMD_READ << 13) );
        RegCmd &= ~(1 << 19);
        Plx_sleep( 50 );
    }

    // Prepare EEPROM write command
    RegCmd |= (PLX8000_EE_CMD_READ_STATUS << 13);

    // Set timeout
    Timeout = 10000;

    // Query EEPROM status until it's ready
    do
    {
        // Send command to get EEPROM status in bits [31:24]
        Plx8000_EepromSendCommand( pdx, RegCmd  );

        // Get EEPROM control register
        RegValue = PlxPci_PlxMappedRegisterRead( pdx, OffsetCtrl, NULL );

        // Check EEPROM read (bit 24) & write ([30:28]) status bits
        if ( ((RegValue & (1 << 24)) == 0) &&
             ((RegValue & (7 << 28)) == 0) )
        {
            return TRUE;
        }

        // Decrement timeout
        Timeout--;
    }
    while (Timeout);

    return FALSE;
}




/******************************************************************************
 *
 * Function   :  Plx8000_EepromSendCommand
 *
 * Description:  Send a command to the EEPROM and wait until it has completed
 *
 ******************************************************************************/
BOOLEAN
Plx8000_EepromSendCommand(
    PLX_DEVICE_OBJECT *pdx,
    U32                command
    )
{
    U16 OffsetCtrl;
    U32 Timeout;
    U32 RegValue;


    // Get EEPROM control register offset
    OffsetCtrl = Plx8000_EepromGetCtrlOffset( pdx );

    // Verify access is supported
    if (OffsetCtrl == 0x0)
    {
        return FALSE;
    }

    // Send EEPROM command
    PlxPci_PlxMappedRegisterWrite( pdx, OffsetCtrl, command );

    /***************************************************************
     * For Capella-1, ~10us delay is needed after issuing a command
     * to allow the chip's EEPROM output pins to respond. This
     * should only be required for PCI access mode due to speed.
     * The method is to perform multiple register reads (~1-2us each).
     **************************************************************/
    if ( (pdx->Key.PlxFamily == PLX_FAMILY_CAPELLA_1) &&
         (pdx->Key.ApiMode == PLX_API_MODE_PCI) )
    {
        for (Timeout = 0; Timeout < 200; Timeout++)
        {
            RegValue = PlxPci_PlxMappedRegisterRead( pdx, OffsetCtrl, NULL );
        }
    }

    // Setup timeout counter
    Timeout = 100000;

    // Wait for command to complete
    do
    {
        // Get EEPROM control register
        RegValue = PlxPci_PlxMappedRegisterRead( pdx, OffsetCtrl, NULL );

        // EEPROM command is complete if status [19:18] is 0 or 2 (with CRC error)
        if ((RegValue & (1 << 18)) == 0)
        {
            return TRUE;
        }

        // Decrement timeout
        Timeout--;
    }
    while (Timeout);

    return FALSE;
}




/******************************************************************************
 *
 * Function   :  Plx8000_EepromComputeNextCrc
 *
 * Description:  Updates the CRC based on the next EEPROM value
 *
 ******************************************************************************/
VOID
Plx8000_EepromComputeNextCrc(
    U32 *pCrc,
    U32  NextEepromValue
    )
{
    U16 i;
    U32 XorValue;


    // Step through each bit of the CRC
    for( i = 0; i < 32; ++i )
    {
        // Shift the CRC, XOR'ing in the constant as required
        XorValue = ((*pCrc ^ (NextEepromValue << i)) & ((U32)1 << 31));

        if (XorValue)
        {
            XorValue = CONST_CRC_XOR_VALUE;
        }
        else
        {
            XorValue = 0;
        }

        // XOR to update the CRC
        *pCrc = (*pCrc << 1) ^ XorValue;
    }
}




/******************************************************************************
 *
 * Function   :  Plx8000_EepromGetCtrlOffset
 *
 * Description:  Returns the EEPROM control register absolute offset (0=error)
 *
 ******************************************************************************/
U16
Plx8000_EepromGetCtrlOffset(
    PLX_DEVICE_OBJECT *pdx
    )
{
    U16 OffsetCtrl;


    // Most PLX chips use 260h in port 0
    OffsetCtrl = 0x260;

    // Mira requires special cases
    if (pdx->Key.PlxFamily == PLX_FAMILY_MIRA)
    {
        // In Legacy mode (EP only), EEPROM offset moves
        if (pdx->Key.DeviceMode == PLX_PORT_LEGACY_ENDPOINT)
        {
            OffsetCtrl = 0x1260;
        }
        else
        {
            // In Enhanced mode, EEPROM access through USB EP is not supported
            if (pdx->Key.PlxPort == PLX_FLAG_PORT_PCIE_TO_USB)
            {
                OffsetCtrl = 0x0;
            }
        }
    }

    return OffsetCtrl;
}
