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
 *      Eep_8000.c
 *
 * Description:
 *
 *      This file contains 8000-series EEPROM support functions
 *
 * Revision History:
 *
 *      06-01-13 : PLX SDK v7.00
 *
 ******************************************************************************/


#include "ChipFunc.h"
#include "Eep_8000.h"
#include "SuppFunc.h"




/******************************************************************************
 *
 * Function   :  Plx8000_EepromPresent
 *
 * Description:  Returns the state of the EEPROM as reported by the PLX device
 *
 ******************************************************************************/
PLX_STATUS
Plx8000_EepromPresent(
    PLX_DEVICE_NODE *pdx,
    U8              *pStatus
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
        return ApiUnsupportedFunction;
    }

    // Get EEPROM Control/Status
    RegValue =
        PLX_8000_REG_READ(
            pdx,
            OffsetCtrl
            );

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

    return ApiSuccess;
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
    PLX_DEVICE_NODE *pdx,
    U8              *pWidth
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
        return ApiUnsupportedFunction;

    // Get EEPROM Control/Status
    RegValue =
        PLX_8000_REG_READ(
            pdx,
            OffsetCtrl
            );

    *pWidth = (U8)(RegValue >> 22) & 0x3;

    if (*pWidth == 0xFF)
        *pWidth = 0;

    return ApiSuccess;
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
    PLX_DEVICE_NODE *pdx,
    U8               width
    )
{
    U16 OffsetCtrl;
    U32 RegValue;


    // Get EEPROM control register offset
    OffsetCtrl = Plx8000_EepromGetCtrlOffset( pdx );

    // Verify access is supported
    if (OffsetCtrl == 0x0)
        return ApiUnsupportedFunction;

    // Get EEPROM Control/Status
    RegValue =
        PLX_8000_REG_READ(
            pdx,
            OffsetCtrl
            );

    // Clear command field [15:13] to avoid EEPROM cycle
    RegValue &= ~(7 << 13);

    // Set address width override enable
    RegValue |= (1 << 21);

    // Enable override
    PLX_8000_REG_WRITE(
        pdx,
        OffsetCtrl,
        RegValue
        );

    // Verify width is overridable
    RegValue =
        PLX_8000_REG_READ(
            pdx,
            OffsetCtrl
            );

    if ((RegValue & (1 << 21)) == 0)
    {
        DebugPrintf(("ERROR - EEPROM width override not supported\n"));
        return ApiUnsupportedFunction;
    }

    // Clear command field [15:13] to avoid EEPROM cycle
    RegValue &= ~(7 << 13);

    // Clear wdith field [23:22]
    RegValue &= ~(3 << 22);

    // Set address width [23:22]
    RegValue |= ((width & 0x3) << 22);

    // Set new address width
    PLX_8000_REG_WRITE(
        pdx,
        OffsetCtrl,
        RegValue
        );

    return ApiSuccess;
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
    PLX_DEVICE_NODE *pdx,
    U32             *pCrc,
    U8              *pCrcStatus
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
                return ApiUnsupportedFunction;

            if (pdx->Key.PlxRevision >= 0xBA)
                OffsetCrc = 0x3EC;
            else
                OffsetCrc = 0x378;
            break;

        case PLX_FAMILY_VEGA_LITE:
            OffsetCrc = 0x78F * sizeof(U32);
            break;

        case PLX_FAMILY_VEGA:
            OffsetCrc = 0xBE4 * sizeof(U32);
            break;

        case PLX_FAMILY_DRACO_2:
        case PLX_FAMILY_CAPELLA_1:
            // Determine EEPROM size to get CRC location
            Plx8000_EepromReadByOffset( pdx, 0, &Value_32 );

            // Verify valid size
            if ((Value_32 >> 16) == 0xFFFF)
            {
                DebugPrintf(("ERROR - EEPROM byte count invalid\n"));
                *pCrcStatus = PLX_CRC_INVALID;
                return ApiInvalidData;
            }

            // CRC location after reg addr/data values
            OffsetCrc = (U16)(sizeof(U32) + (Value_32 >> 16));
            break;

        default:
            DebugPrintf(("ERROR - Device (%04X) doesn't support CRC\n", pdx->Key.PlxChip));
            return ApiUnsupportedFunction;
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
    Value_32 =
        PLX_8000_REG_READ(
            pdx,
            0x260
            );

    // Get EEPROM status bits [17:16]
    Value_32 = (Value_32 >> 16) & 0x3;

    if (Value_32 == 1)
        *pCrcStatus = PLX_CRC_VALID;
    else
        *pCrcStatus = PLX_CRC_INVALID;

    DebugPrintf((
        "CRC = %08X [%s] (offset=%02X)\n",
        (int)*pCrc,
        (*pCrcStatus == PLX_CRC_VALID) ? "Valid" : "Invalid or Undetermined",
        OffsetCrc
        ));

    return ApiSuccess;
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
    PLX_DEVICE_NODE *pdx,
    U32             *pCrc,
    BOOLEAN          bUpdateEeprom
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
                return ApiUnsupportedFunction;

            if (pdx->Key.PlxRevision >= 0xBA)
                OffsetCrc = 0x3EC;
            else
                OffsetCrc = 0x378;
            break;

        case PLX_FAMILY_VEGA_LITE:
            OffsetCrc = 0x78F * sizeof(U32);
            break;

        case PLX_FAMILY_VEGA:
            OffsetCrc = 0xBE4 * sizeof(U32);
            break;

        case PLX_FAMILY_DRACO_2:
        case PLX_FAMILY_CAPELLA_1:
            // Determine EEPROM size to get CRC location
            Plx8000_EepromReadByOffset( pdx, 0, &Value_32 );

            // Verify valid size
            if ((Value_32 >> 16) == 0xFFFF)
            {
                DebugPrintf(("ERROR - EEPROM byte count invalid\n"));
                return ApiInvalidData;
            }

            // CRC location after EEPROM (header + reg addr/data values)
            OffsetCrc = (U16)(sizeof(U32) + (Value_32 >> 16));

            // CRC calculation starts at byte 2
            OffsetCalcStart = 2;
            break;

        default:
            DebugPrintf(("ERROR - Device (%04X) doesn't support CRC\n", pdx->Key.PlxChip));
            return ApiUnsupportedFunction;
    }

    // Initialize CRC
    Crc = (U32)-1;

    // Calculate CRC by reading all values in EEPROM
    for (offset=OffsetCalcStart; offset < OffsetCrc; offset += sizeof(U32))
    {
        // Read next EEPROM value
        if (offset & 0x3)
        {
            // EEPROM offsets are not DWord aligned, must build 16-bits at a time
            Plx8000_EepromReadByOffset_16( pdx, offset, &Value_16 );
            Value_32 = Value_16;

            // If final data not aligned on DWord, pad with 0's
            if ((offset + sizeof(U16)) < OffsetCrc)
                Plx8000_EepromReadByOffset_16( pdx, offset + sizeof(U16), &Value_16 );
            else
                Value_16 = 0;
            Value_32 |= ((U32)Value_16) << 16;
        }
        else
        {
            Plx8000_EepromReadByOffset( pdx, offset, &Value_32 );
        }

        // Update the CRC
        Plx8000_EepromComputeNextCrc( &Crc, Value_32 );
    }

    DebugPrintf(("Calculated CRC = %08X (offset=%X)\n", (int)Crc, OffsetCrc));

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
            Plx8000_EepromWriteByOffset( pdx, OffsetCrc, Crc );
    }
    else
    {
        DebugPrintf(("Skipping CRC update in EEPROM\n"));
    }

    // Return the new CRC
    *pCrc = Crc;

    return ApiSuccess;
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
    PLX_DEVICE_NODE *pdx,
    U32              offset,
    U32             *pValue
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
        return ApiUnsupportedFunction;

    // Wait until EEPROM is ready
    if (Plx8000_EepromWaitIdle(
            pdx
            ) == FALSE)
    {
        return ApiWaitTimeout;
    }

    // Get EEPROM control register
    RegValue =
        PLX_8000_REG_READ(
            pdx,
            OffsetCtrl
            );

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
           EepWidth = 1;
    }

    // Verify offset doesn't exceed byte addressing
    if (offset >= ((U32)1 << (EepWidth * 8)))
    {
        DebugPrintf(("ERROR - Offset (%X) exceeds %d byte-addresssing\n", (int)offset, (int)EepWidth));
        return ApiInvalidOffset;
    }

    // For 3-byte addressing, set upper byte
    if (EepWidth == 3)
    {
        RegUpper =
            PLX_8000_REG_READ(
                pdx,
                OffsetCtrl + 0xC    // Offset x26Ch
                );

        // Set 3rd address byte (26Ch[7:0])
        RegUpper &= ~(0xFF << 0);
        RegUpper |= (offset >> 16) & 0xFF;

        PLX_8000_REG_WRITE(
            pdx,
            OffsetCtrl + 0xC,   // Offset x26Ch
            RegUpper
            );
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
    Plx8000_EepromSendCommand(
        pdx,
        RegValue
        );

    // Return EEPROM data
    *pValue =
        PLX_8000_REG_READ(
            pdx,
            OffsetCtrl + 0x4    // Offset x264h
            );

    return ApiSuccess;
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
    PLX_DEVICE_NODE *pdx,
    U32              offset,
    U32              value
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
        return ApiUnsupportedFunction;

    // Wait until EEPROM is ready
    if (Plx8000_EepromWaitIdle(
            pdx
            ) == FALSE)
    {
        return ApiWaitTimeout;
    }

    // Get EEPROM control register
    RegValue =
        PLX_8000_REG_READ(
            pdx,
            OffsetCtrl
            );

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
           EepWidth = 1;
    }

    // Verify offset doesn't exceed byte addressing
    if (offset >= ((U32)1 << (EepWidth * 8)))
    {
        DebugPrintf(("ERROR - Offset (%X) exceeds %d byte-addresssing\n", (int)offset, (int)EepWidth));
        return ApiInvalidOffset;
    }

    // For 3-byte addressing, set upper byte
    if (EepWidth == 3)
    {
        RegUpper =
            PLX_8000_REG_READ(
                pdx,
                OffsetCtrl + 0xC    // Offset x26Ch
                );

        // Set 3rd address byte (26Ch[7:0])
        RegUpper &= ~(0xFF << 0);
        RegUpper |= (offset >> 16) & 0xFF;

        PLX_8000_REG_WRITE(
            pdx,
            OffsetCtrl + 0xC,   // Offset x26Ch
            RegUpper
            );
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

    // Prepare EEPROM data
    PLX_8000_REG_WRITE(
        pdx,
        OffsetCtrl + 0x4,   // Offset x264h
        value
        );

    // Prepare EEPROM write command
    RegValue |=
        ((offset & 0x1FFF)    <<  0) |     // Bits [12:0] of offset
        (((offset >> 13) & 1) << 20) |     // Bit 13 of offset
        (PLX8000_EE_CMD_WRITE << 13);      // EEPROM command

    // Send EEPROM write command
    Plx8000_EepromSendCommand(
        pdx,
        RegValue
        );

    return ApiSuccess;
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
    PLX_DEVICE_NODE *pdx,
    U32              offset,
    U16             *pValue
    )
{
    U32        Value_32;
    PLX_STATUS status;


    // Clear return data
    *pValue = 0;

    // Get 32-bit value
    status =
        Plx8000_EepromReadByOffset(
            pdx,
            (offset & ~0x3),
            &Value_32
            );

    if (status != ApiSuccess)
        return status;

    // Return desired 16-bit portion
    if (offset & 0x3)
        *pValue = (U16)(Value_32 >> 16);
    else
        *pValue = (U16)Value_32;

    return ApiSuccess;
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
    PLX_DEVICE_NODE *pdx,
    U32              offset,
    U16              value
    )
{
    U32        Value_32;
    PLX_STATUS status;


    // Get current 32-bit value
    status =
        Plx8000_EepromReadByOffset(
            pdx,
            (offset & ~0x3),
            &Value_32
            );

    if (status != ApiSuccess)
        return status;

    // Insert new 16-bit value in correct location
    if (offset & 0x3)
        Value_32 = ((U32)value << 16) | (Value_32 & 0xFFFF);
    else
        Value_32 = ((U32)value) | (Value_32 & 0xFFFF0000);

    // Update EEPROM
    return Plx8000_EepromWriteByOffset(
        pdx,
        (offset & ~0x3),
        Value_32
        );
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
    PLX_DEVICE_NODE *pdx
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
        return FALSE;

    // Get EEPROM control register
    RegCmd =
        PLX_8000_REG_READ(
            pdx,
            OffsetCtrl
            );

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
        PLX_8000_REG_WRITE( pdx, OffsetCtrl, RegCmd | (PLX8000_EE_CMD_READ << 13) );
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
        Plx8000_EepromSendCommand(
            pdx,
            RegCmd
            );

        // Get EEPROM control register
        RegValue =
            PLX_8000_REG_READ(
                pdx,
                OffsetCtrl
                );

        // Check EEPROM read (bit 24) & write ([30:28]) status bits
        if ( ((RegValue & (1 << 24)) == 0) &&
             ((RegValue & (7 << 28)) == 0))
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
    PLX_DEVICE_NODE *pdx,
    U32              command
    )
{
    U16 OffsetCtrl;
    U32 Timeout;
    U32 RegValue;


    // Get EEPROM control register offset
    OffsetCtrl = Plx8000_EepromGetCtrlOffset( pdx );

    // Verify access is supported
    if (OffsetCtrl == 0x0)
        return FALSE;

    // Send EEPROM command
    PLX_8000_REG_WRITE(
        pdx,
        OffsetCtrl,
        command
        );

    /***************************************************************
     * For Capella-1, ~10us delay is needed after issuing a command
     * to allow the chip's EEPROM output pins to respond. This
     * should only be required for PCI access mode due to speed.
     * The method is to perform multiple register reads (~1-2us each).
     **************************************************************/
    if ((pdx->Key.PlxFamily == PLX_FAMILY_CAPELLA_1) && (pdx->Key.ApiMode == PLX_API_MODE_PCI))
    {
        for (Timeout=0; Timeout < 200; Timeout++)
            RegValue = PLX_8000_REG_READ( pdx, OffsetCtrl );
    }

    // Setup timeout counter
    Timeout = 100000;

    // Wait for command to complete
    do
    {
        // Get EEPROM control register
        RegValue =
            PLX_8000_REG_READ(
                pdx,
                OffsetCtrl
                );

        // EEPROM command is complete if status [19:18] is 0 or 2 (with CRC error)
        if ((RegValue & (1 << 18)) == 0)
            return TRUE;

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
    for( i=0; i<32; ++i )
    {
        // Shift the CRC, XOR'ing in the constant as required
        XorValue = ((*pCrc ^ (NextEepromValue << i)) & (1 << 31));

        if (XorValue)
            XorValue = CONST_CRC_XOR_VALUE;
        else
            XorValue = 0;

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
    PLX_DEVICE_NODE *pdx
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
            OffsetCtrl = 0x1260;
        else
        {
            // In Enhanced mode, EEPROM access through USB EP is not supported
            if (pdx->PciHeaderType == 0)
                OffsetCtrl = 0x0;
        }
    }

    return OffsetCtrl;
}
