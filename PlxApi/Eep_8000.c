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
 *      03-01-09 : PLX SDK v6.10
 *
 ******************************************************************************/


#include "Eep_8000.h"

#if defined(PLX_DOS)
    #include "ChipFunc.h"
    #include "SuppFunc.h"
#else
    #include "PlxApiDebug.h"
    #include "PlxApiI2cAa.h"
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
    PLX_DEVICE_OBJECT *pDevice,
    U8                *pStatus
    )
{
    U32 RegValue;


    // Get EEPROM Control/Status
    RegValue =
        PLX_8000_REG_READ(
            pDevice,
            0x260
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
 * Function   :  Plx8000_EepromSetAddressWidth
 *
 * Description:  Sets a new EEPROM address width
 *
 ******************************************************************************/
PLX_STATUS
Plx8000_EepromSetAddressWidth(
    PLX_DEVICE_OBJECT *pDevice,
    U8                 width
    )
{
    U32 RegValue;


    // Get EEPROM Control/Status
    RegValue =
        PLX_8000_REG_READ(
            pDevice,
            0x260
            );

    // Clear command field [15:13] to avoid EEPROM cycle
    RegValue &= ~(7 << 13);

    // Set address width override enable
    RegValue |= (1 << 21);

    // Enable override
    PLX_8000_REG_WRITE(
        pDevice,
        0x260,
        RegValue
        );

    // Verify width is overridable
    RegValue =
        PLX_8000_REG_READ(
            pDevice,
            0x260
            );

    if ((RegValue & (1 << 21)) == 0)
        return ApiUnsupportedFunction;

    // Set address width [23:22]
    RegValue |= ((width & 0x3) << 22);

    // Set new address width
    PLX_8000_REG_WRITE(
        pDevice,
        0x260,
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
    PLX_DEVICE_OBJECT *pDevice,
    U32               *pCrc,
    U8                *pCrcStatus
    )
{
    U16 OffsetCrc;
    U32 RegValue;


    // Determine the CRC EEPROM offset
    switch (pDevice->Key.PlxChip)
    {
        case 0x8114:
            if (pDevice->Key.PlxRevision >= 0xBA)
                OffsetCrc = 0x3EC;
            else
                OffsetCrc = 0x378;
            break;

        case 0x8508:
        case 0x8512:
        case 0x8517:
        case 0x8518:
            OffsetCrc = 0x78F * sizeof(U32);
            break;

        case 0x8516:
        case 0x8524:
        case 0x8532:
            OffsetCrc = 0xBE4 * sizeof(U32);
            break;

        default:
            return ApiUnsupportedFunction;
    }

    // Read CRC from EEPROM
    Plx8000_EepromReadByOffset(
        pDevice,
        OffsetCrc,
        pCrc
        );

    DebugPrintf((
        "CRC = %08x (offset=%02x)\n",
        (int)*pCrc, OffsetCrc
        ));

    // Get EEPROM and CRC status
    RegValue =
        PLX_8000_REG_READ(
            pDevice,
            0x260
            );

    // Get EEPROM status bits [17:16]
    RegValue = (RegValue >> 16) & 0x3;

    if (RegValue == 1)
        *pCrcStatus = PLX_CRC_VALID;
    else
        *pCrcStatus = PLX_CRC_INVALID;

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
    PLX_DEVICE_OBJECT *pDevice,
    U32               *pCrc,
    BOOLEAN            bUpdateEeprom
    )
{
    U16 offset;
    U16 OffsetCrc;
    U32 Crc;
    U32 EepromValue;


    // Determine the CRC EEPROM offset
    switch (pDevice->Key.PlxChip)
    {
        case 0x8114:
            if (pDevice->Key.PlxRevision >= 0xBA)
                OffsetCrc = 0x3EC;
            else
                OffsetCrc = 0x378;
            break;

        case 0x8508:
        case 0x8512:
        case 0x8517:
        case 0x8518:
            OffsetCrc = 0x78F * sizeof(U32);
            break;

        case 0x8516:
        case 0x8524:
        case 0x8532:
            OffsetCrc = 0xBE4 * sizeof(U32);
            break;

        default:
            return ApiUnsupportedFunction;
    }

    // Initialize CRC
    Crc = (U32)-1;

    // Calculate CRC by reading all values in EEPROM
    for (offset=0; offset < OffsetCrc; offset += sizeof(U32))
    {
        // Read next EEPROM value
        Plx8000_EepromReadByOffset(
            pDevice,
            offset,
            &EepromValue
            );

        // Update the CRC
        Plx8000_EepromComputeNextCrc(
            &Crc,
            EepromValue
            );
    }

    DebugPrintf((
        "Calculated CRC = %08x\n",
        (int)Crc
        ));

    // Write new value to EEPROM if requested
    if (bUpdateEeprom)
    {
        DebugPrintf(("Writing new CRC to EEPROM...\n"));

        // Update CRC in EEPROM
        Plx8000_EepromWriteByOffset(
            pDevice,
            OffsetCrc,
            Crc
            );
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
    PLX_DEVICE_OBJECT *pDevice,
    U16                offset,
    U32               *pValue
    )
{
    U32 RegValue;


    // Verify offset (14 bits max [13:0])
    if (offset >= (1 << 14))
        return ApiInvalidOffset;

    // Wait until EEPROM is ready
    if (Plx8000_EepromWaitIdle(
            pDevice
            ) == FALSE)
    {
        return ApiWaitTimeout;
    }

    // Clear return data
    *pValue = 0;

    // Convert offset to an index
    offset = (offset / sizeof(U32));

    // Get EEPROM control register
    RegValue =
        PLX_8000_REG_READ(
            pDevice,
            0x260
            );

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
        pDevice,
        RegValue
        );

    // Return EEPROM data
    *pValue =
        PLX_8000_REG_READ(
            pDevice,
            0x264
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
    PLX_DEVICE_OBJECT *pDevice,
    U16                offset,
    U32                value
    )
{
    U32 RegValue;


    // Verify offset (14 bits max [13:0])
    if (offset >= (1 << 14))
        return ApiInvalidOffset;

    // Wait until EEPROM is ready
    if (Plx8000_EepromWaitIdle(
            pDevice
            ) == FALSE)
    {
        return ApiWaitTimeout;
    }

    // Convert offset to an index
    offset = (offset / sizeof(U32));

    // Get EEPROM control register
    RegValue =
        PLX_8000_REG_READ(
            pDevice,
            0x260
            );

    // Clear command field [15:13]
    RegValue &= ~(7 << 13);

    // Clear offset field [20,12:0]
    RegValue &= ~((1 << 20) | (0x1FFF << 0));

    // Send EEPROM write enable command
    Plx8000_EepromSendCommand(
        pDevice,
        RegValue | (PLX8000_EE_CMD_WRITE_ENABLE << 13)
        );

    // Prepare EEPROM data
    PLX_8000_REG_WRITE(
        pDevice,
        0x264,
        value
        );

    // Prepare EEPROM write command
    RegValue |=
        ((offset & 0x1FFF)    <<  0) |     // Bits [12:0] of offset
        (((offset >> 13) & 1) << 20) |     // Bit 13 of offset
        (PLX8000_EE_CMD_WRITE << 13);      // EEPROM command

    // Send EEPROM write command
    Plx8000_EepromSendCommand(
        pDevice,
        RegValue
        );

    return ApiSuccess;
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
    PLX_DEVICE_OBJECT *pDevice
    )
{
    U32 Timeout;
    U32 RegValue;
    U32 RegCmd;


    // Set timeout
    Timeout = 10000;

    // Get EEPROM control register
    RegCmd =
        PLX_8000_REG_READ(
            pDevice,
            0x260
            );
	
    // Clear command field [15:13]
    RegCmd &= ~(7 << 13);

    // Clear the EepRdy bit [24]
    RegCmd &= ~(1 << 24);

    // Clear the EepWrStatus bits[30:28]
    RegCmd &= ~(7 << 28);
	
    // Prepare EEPROM write command
    RegCmd |= (PLX8000_EE_CMD_READ_STATUS << 13);

    // Query EEPROM status until it's ready
    do
    {
        // Send command to get EEPROM status in bits [31:24]
        Plx8000_EepromSendCommand(
            pDevice,
            RegCmd
            );

        // Get EEPROM control register
        RegValue =
            PLX_8000_REG_READ(
                pDevice,
                0x260
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
    PLX_DEVICE_OBJECT *pDevice,
    U32                command
    )
{
    U32 Timeout;
    U32 RegValue;


    // Send EEPROM command
    PLX_8000_REG_WRITE(
        pDevice,
        0x260,
        command
        );

    // Setup timeout counter
    Timeout = 100000;

    // Wait for command to complete
    do
    {
        // Get EEPROM control register
        RegValue =
            PLX_8000_REG_READ(
                pDevice,
                0x260
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
