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
 *      Eep_8111.c
 *
 * Description:
 *
 *      This file contains 8111-series EEPROM support functions
 *
 * Revision History:
 *
 *      08-01-11 : PLX SDK v6.50
 *
 ******************************************************************************/


#include "ChipFunc.h"
#include "Eep_8111.h"




/******************************************************************************
 *
 * Function   :  Plx8111_EepromPresent
 *
 * Description:  Returns the state of the EEPROM as reported by the PLX device
 *
 ******************************************************************************/
PLX_STATUS
Plx8111_EepromPresent(
    PLX_DEVICE_NODE *pdx,
    U8              *pStatus
    )
{
    U32 RegValue;


    // Get EEPROM Control/Status
    RegValue =
        PLX_8111_REG_READ(
            pdx,
            0x1004
            );

    // Check if an EEPROM is present
    if (RegValue & (1 << 21))
    {
        // Check if EEPROM contains valid signature (5Ah)
        if (RegValue & (1 << 20))
            *pStatus = PLX_EEPROM_STATUS_VALID;
        else
            *pStatus = PLX_EEPROM_STATUS_INVALID_DATA;
    }
    else
    {
        *pStatus = PLX_EEPROM_STATUS_NONE;
    }

    return ApiSuccess;
}




/******************************************************************************
 *
 * Function   :  Plx8111_EepromGetAddressWidth
 *
 * Description:  Returns the current EEPROM address width
 *
 ******************************************************************************/
PLX_STATUS
Plx8111_EepromGetAddressWidth(
    PLX_DEVICE_NODE *pdx,
    U8              *pWidth
    )
{
    U32 RegValue;


    // Set default return value
    *pWidth = 0;

    // Get EEPROM width (1004h[24:23])
    RegValue =
        PLX_8111_REG_READ(
            pdx,
            0x1004
            );

    *pWidth = (U8)((RegValue >> 23) & 0x3);

    // If valid, update default byte addressing
    if (*pWidth != 0)
        pdx->Default_EepWidth = *pWidth;

    return ApiSuccess;
}




/******************************************************************************
 *
 * Function   :  Plx8111_EepromSetAddressWidth
 *
 * Description:  Sets a new EEPROM address width
 *
 ******************************************************************************/
PLX_STATUS
Plx8111_EepromSetAddressWidth(
    PLX_DEVICE_NODE *pdx,
    U8               width
    )
{
    U32 RegValue;


    // Get EEPROM width (1004h[24:23])
    RegValue =
        PLX_8111_REG_READ(
            pdx,
            0x1004
            );

    if (((RegValue >> 23) & 0x3) != 0)
    {
        DebugPrintf(("Error - EEPROM width already detected in controller\n"));
        return ApiUnsupportedFunction;
    }

    // Update default byte addressing
    pdx->Default_EepWidth = width;

    return ApiSuccess;
}




/******************************************************************************
 *
 * Function   :  Plx8111_EepromReadByOffset_16
 *
 * Description:  Read a 16-bit value from the EEPROM at a specified offset
 *
 ******************************************************************************/
PLX_STATUS
Plx8111_EepromReadByOffset_16(
    PLX_DEVICE_NODE *pdx,
    U32              offset,
    U16             *pValue
    )
{
    U8  i;
    U8  EepData;
    U32 RegValue;


    // Set default return value
    *pValue = 0;

    // Wait until EEPROM is ready
    if (Plx8111_EepromWaitUntilReady(
            pdx
            ) == FALSE)
    {
        return ApiWaitTimeout;
    }

    // Send EEPROM read command
    Plx8111_EepromDataWrite(
        pdx,
        PLX8111_EE_CMD_READ
        );

    // Get EEPROM width
    RegValue =
        PLX_8111_REG_READ(
            pdx,
            0x1004
            );

    RegValue = (RegValue >> 23) & 0x3;

    // For undefined EEPROM, use default byte addressing
    if (RegValue == 0)
        RegValue = pdx->Default_EepWidth;

    // Check for 3-byte addressing
    if (RegValue == 3)
    {
        // Send EEPROM address (byte 3)
        Plx8111_EepromDataWrite(
            pdx,
            0          // 3 byte addressing not supported
            );
    }

    if ((RegValue == 2) || (RegValue == 3))
    {
        // Send EEPROM address (byte 2)
        Plx8111_EepromDataWrite(
            pdx,
            (U8)(offset >> 8)
            );
    }

    // Send EEPROM address (byte 1)
    Plx8111_EepromDataWrite(
        pdx,
        (U8)offset
        );

    // Get data 8-bits at a time
    for (i=0; i < 2; i++)
    {
        // Get EEPROM data
        Plx8111_EepromDataRead(
            pdx,
            &EepData
            );

        // Insert current byte
        if (i == 0)
            *pValue |= EepData;
        else
            *pValue |= (U16)EepData << 8;
    }

    // Disable EEPROM access
    PLX_8111_REG_WRITE(
        pdx,
        0x1004,
        0
        );

    return ApiSuccess;
}




/******************************************************************************
 *
 * Function   :  Plx8111_EepromWriteByOffset_16
 *
 * Description:  Write a 16-bit value to the EEPROM at a specified offset
 *
 ******************************************************************************/
PLX_STATUS
Plx8111_EepromWriteByOffset_16(
    PLX_DEVICE_NODE *pdx,
    U32              offset,
    U16              value
    )
{
    U8  i;
    U32 RegValue;


    // Wait until EEPROM is ready
    if (Plx8111_EepromWaitUntilReady(
            pdx
            ) == FALSE)
    {
        return ApiWaitTimeout;
    }

    // Send EEPROM write-enable command
    Plx8111_EepromDataWrite(
        pdx,
        PLX8111_EE_CMD_WRITE_ENABLE
        );

    // Disable EEPROM access
    PLX_8111_REG_WRITE(
        pdx,
        0x1004,
        0
        );

    // Send EEPROM write command
    Plx8111_EepromDataWrite(
        pdx,
        PLX8111_EE_CMD_WRITE
        );

    // Get EEPROM width
    RegValue =
        PLX_8111_REG_READ(
            pdx,
            0x1004
            );

    RegValue = (RegValue >> 23) & 0x3;

    // For undefined EEPROM, use default byte addressing
    if (RegValue == 0)
        RegValue = pdx->Default_EepWidth;

    // Check for 3-byte addressing
    if (RegValue == 3)
    {
        // Send EEPROM address (byte 3)
        Plx8111_EepromDataWrite(
            pdx,
            0          // 3 byte addressing not supported
            );
    }

    // Check for 2 or 3-byte addressing
    if ((RegValue == 2) || (RegValue == 3))
    {
        // Send EEPROM address (byte 2)
        Plx8111_EepromDataWrite(
            pdx,
            (U8)(offset >> 8)
            );
    }

    // Send EEPROM address (byte 1)
    Plx8111_EepromDataWrite(
        pdx,
        (U8)offset
        );

    // Write data 8-bits at a time
    for (i=0; i < 2; i++)
    {
        // Write current byte
        Plx8111_EepromDataWrite(
            pdx,
            (U8)value
            );

        // Shift for next byte
        value >>= 8;
    }

    // Disable EEPROM access
    PLX_8111_REG_WRITE(
        pdx,
        0x1004,
        0
        );

    return ApiSuccess;
}




/******************************************************************************
 *
 * Function   :  Plx8111_EepromWaitIdle
 *
 * Description:  Wait until the EEPROM access is idle
 *
 ******************************************************************************/
BOOLEAN
Plx8111_EepromWaitIdle(
    PLX_DEVICE_NODE *pdx
    )
{
    U32 Timeout;
    U32 RegValue;


    // Wait until EEPROM is idle
    Timeout = 100000;

    do
    {
        // Get EEPROM control register
        RegValue =
            PLX_8111_REG_READ(
                pdx,
                0x1004
                );

        // Decrement timeout
        Timeout--;
    }
    while (Timeout && (RegValue & (1 << 19)));

    if (Timeout == 0)
        return FALSE;

    return TRUE;
}




/******************************************************************************
 *
 * Function   :  Plx8111_EepromWaitUntilReady
 *
 * Description:  Wait until the EEPROM status is not busy
 *
 ******************************************************************************/
BOOLEAN
Plx8111_EepromWaitUntilReady(
    PLX_DEVICE_NODE *pdx
    )
{
    U8  status;
    U16 Timeout;


    // Set initial timeout
    Timeout = 200;

    // Check EEPROM status to make sure it is not busy
    do
    {
        // Send EEPROM query status command
        Plx8111_EepromDataWrite(
            pdx,
            PLX8111_EE_CMD_READ_STATUS
            );

        // Get status
        Plx8111_EepromDataRead(
            pdx,
            &status
            );

        // Disable EEPROM access
        PLX_8111_REG_WRITE(
            pdx,
            0x1004,
            0
            );

        // Check if EEPROM is ready
        if ((status & (1 << 0)) == 0)
            return TRUE;

        // Decrement count
        Timeout--;
    }
    while (Timeout);

    // Timeout error
    return FALSE;
}




/******************************************************************************
 *
 * Function   :  Plx8111_EepromDataRead
 *
 * Description:  Reads a data byte from the EEPROM
 *
 ******************************************************************************/
BOOLEAN
Plx8111_EepromDataRead(
    PLX_DEVICE_NODE *pdx,
    U8              *pData
    )
{
    U32     RegValue;
    BOOLEAN rc;


    // Wait until EEPROM is idle
    if (Plx8111_EepromWaitIdle(
            pdx
            ) == FALSE)
    {
        return FALSE;
    }

    // Enable EEPROM access
    RegValue =
        (1 << 18) |     // EEPROM chip select
        (1 << 17);      // EEPROM read start

    PLX_8111_REG_WRITE(
        pdx,
        0x1004,
        RegValue
        );

    // Wait until EEPROM is idle
    rc =
        Plx8111_EepromWaitIdle(
            pdx
            );

    if (rc == TRUE)
    {
        // Get EEPROM data
        RegValue =
            PLX_8111_REG_READ(
                pdx,
                0x1004
                );

        *pData = (U8)(RegValue >> 8);
    }

    return rc;
}




/******************************************************************************
 *
 * Function   :  Plx8111_EepromDataWrite
 *
 * Description:  Writes a data byte to the EEPROM
 *
 ******************************************************************************/
BOOLEAN
Plx8111_EepromDataWrite(
    PLX_DEVICE_NODE *pdx,
    U8               data
    )
{
    U32 RegValue;


    // Wait until EEPROM is idle
    if (Plx8111_EepromWaitIdle(
            pdx
            ) == FALSE)
    {
        return FALSE;
    }

    // Enable EEPROM access
    RegValue =
        (1 << 18) |     // EEPROM chip select
        (1 << 16) |     // EEPROM write start
        data;           // EEPROM data

    PLX_8111_REG_WRITE(
        pdx,
        0x1004,
        RegValue
        );

    // Wait until EEPROM is idle
    return Plx8111_EepromWaitIdle(
        pdx
        );
}
