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
 *      Eep_9000.c
 *
 * Description:
 *
 *      This file contains 9000-series EEPROM support functions
 *
 * Revision History:
 *
 *      12-01-07 : PLX SDK v5.20
 *
 ******************************************************************************/


#include "Eep_9000.h"
#include "SuppFunc.h"




/******************************************************************************
 *
 * Function   :  Plx9000_EepromPresent
 *
 * Description:  Returns the state of the EEPROM as reported by the PLX device
 *
 *****************************************************************************/
PLX_STATUS
Plx9000_EepromPresent(
    DEVICE_EXTENSION *pdx,
    U8               *pStatus
    )
{
    U32 RegValue;


    // Get EEPROM status register
    RegValue =
        PLX_9000_REG_READ(
            pdx,
            REG_EEPROM_CTRL
            );

    if (RegValue & (1 << 28))
    {
        *pStatus = PLX_EEPROM_STATUS_VALID;
    }
    else
    {
        *pStatus = PLX_EEPROM_STATUS_NONE;
    }

    return ApiSuccess;
}




/******************************************************************************
 *
 * Function   :  Plx9000_EepromReadByOffset
 *
 * Description:  Read a 32-bit value from the EEPROM at a specified offset
 *
 *****************************************************************************/
PLX_STATUS
Plx9000_EepromReadByOffset(
    DEVICE_EXTENSION *pdx,
    U16               offset,
    U32              *pValue
    )
{
    S8  BitPos;
    S8  CommandShift;
    S8  CommandLength;
    U16 count;
    U32 RegValue;


    // Setup parameters based on EEPROM type
    switch (PLX_9000_EEPROM_TYPE)
    {
        case Eeprom93CS46:
            CommandShift  = 0;
            CommandLength = PLX9000_EE_CMD_LEN_46;
            break;

        case Eeprom93CS56:
            CommandShift  = 2;
            CommandLength = PLX9000_EE_CMD_LEN_56;
            break;

        default:
            *pValue = 0;
            return ApiUnsupportedFunction;
    }

    // Send EEPROM read command and offset to EEPROM
    Plx9000_EepromSendCommand(
        pdx,
        (PLX9000_EE_CMD_READ << CommandShift) | (offset / 2),
        CommandLength
        );

    /*****************************************************
     * Note: The EEPROM write ouput bit (26) is set here
     *       because it is required before EEPROM read
     *       operations on the 9054.  It does not affect
     *       behavior of non-9054 chips.
     *
     *       The EEDO Input enable bit (31) is required for
     *       some chips.  Since it is a reserved bit in older
     *       chips, there is no harm in setting it for all.
     ****************************************************/

    // Set EEPROM write output bit
    RegValue =
        PLX_9000_REG_READ(
            pdx,
            REG_EEPROM_CTRL
            );

    // Set EEDO Input enable for some PLX chips
    RegValue |= (1 << 31);

    PLX_9000_REG_WRITE(
        pdx,
        REG_EEPROM_CTRL,
        RegValue | (1 << 26)
        );

    // Get 32-bit value from EEPROM - one bit at a time
    for (BitPos = 0; BitPos < 32; BitPos++)
    {
        // Trigger the EEPROM clock
        Plx9000_EepromClock(
            pdx
            );

        /*****************************************************
         * Note: After the EEPROM clock, a delay is sometimes
         *       needed to let the data bit propagate from the
         *       EEPROM to the PLX chip.  If a sleep mechanism
         *       is used, the result is an extremely slow EEPROM
         *       access since the delay resolution is large and
         *       is required for every data bit read.
         *
         *       Rather than using the standard sleep mechanism,
         *       the code, instead, reads the PLX register
         *       multiple times.  This is harmless and provides
         *       enough delay for the EEPROM data to propagate.
         ****************************************************/

        for (count=0; count < 20; count++)
        {
            // Get the result bit
            RegValue =
                PLX_9000_REG_READ(
                    pdx,
                    REG_EEPROM_CTRL
                    );
        }

        // Get bit value and shift into result
        if (RegValue & (1 << 27))
        {
            *pValue = (*pValue << 1) | 1;
        }
        else
        {
            *pValue = (*pValue << 1);
        }
    }

    // Clear EEDO Input enable for some PLX chips
    RegValue &= ~(1 << 31);

    // Clear Chip Select and all other EEPROM bits
    PLX_9000_REG_WRITE(
        pdx,
        REG_EEPROM_CTRL,
        RegValue & ~(0xF << 24)
        );

    return ApiSuccess;
}




/******************************************************************************
 *
 * Function   :  Plx9000_EepromWriteByOffset
 *
 * Description:  Write a 32-bit value to the EEPROM at a specified offset
 *
 *****************************************************************************/
PLX_STATUS
Plx9000_EepromWriteByOffset(
    DEVICE_EXTENSION *pdx,
    U16               offset,
    U32               value
    )
{
    S8  i;
    S8  BitPos;
    S8  CommandShift;
    S8  CommandLength;
    U16 EepromValue;
    S32 Timeout;
    U32 RegValue;


    // Setup parameters based on EEPROM type
    switch (PLX_9000_EEPROM_TYPE)
    {
        case Eeprom93CS46:
            CommandShift  = 0;
            CommandLength = PLX9000_EE_CMD_LEN_46;
            break;

        case Eeprom93CS56:
            CommandShift  = 2;
            CommandLength = PLX9000_EE_CMD_LEN_56;
            break;

        default:
            return ApiUnsupportedFunction;
    }

    // Write EEPROM 16-bits at a time
    for (i=0; i<2; i++)
    {
        // Set 16-bit value to write
        if (i == 0)
        {
            EepromValue = (U16)(value >> 16);
        }
        else
        {
            EepromValue = (U16)value;

            // Update offset
            offset = offset + sizeof(U16);
        }

        // Send Write_Enable command to EEPROM
        Plx9000_EepromSendCommand(
            pdx,
            (PLX9000_EE_CMD_WRITE_ENABLE << CommandShift),
            CommandLength
            );

        // Send EEPROM Write command and offset to EEPROM
        Plx9000_EepromSendCommand(
            pdx,
            (PLX9000_EE_CMD_WRITE << CommandShift) | (offset / 2),
            CommandLength
            );

        RegValue =
            PLX_9000_REG_READ(
                pdx,
                REG_EEPROM_CTRL
                );

        // Clear all EEPROM bits
        RegValue &= ~(0xF << 24);

        // Make sure EEDO Input is disabled for some PLX chips
        RegValue &= ~(1 << 31);

        // Enable EEPROM Chip Select
        RegValue |= (1 << 25);

        // Write 16-bit value to EEPROM - one bit at a time
        for (BitPos = 15; BitPos >= 0; BitPos--)
        {
            // Get bit value and shift into result
            if (EepromValue & (1 << BitPos))
            {
                PLX_9000_REG_WRITE(
                    pdx,
                    REG_EEPROM_CTRL,
                    RegValue | (1 << 26)
                    );
            }
            else
            {
                PLX_9000_REG_WRITE(
                    pdx,
                    REG_EEPROM_CTRL,
                    RegValue
                    );
            }

            // Trigger the EEPROM clock
            Plx9000_EepromClock(
                pdx
                );
        }

        // Deselect Chip
        PLX_9000_REG_WRITE(
            pdx,
            REG_EEPROM_CTRL,
            RegValue & ~(1 << 25)
            );

        // Re-select Chip
        PLX_9000_REG_WRITE(
            pdx,
            REG_EEPROM_CTRL,
            RegValue | (1 << 25)
            );

        /*****************************************************
         * Note: After the clocking in the last data bit, a
         *       delay is needed to let the EEPROM internally
         *       complete the write operation.  If a sleep
         *       mechanism is used, the result is an extremely
         *       slow EEPROM access since the delay resolution
         *       is too large.
         *
         *       Rather than using the standard sleep mechanism,
         *       the code, instead, reads the PLX register
         *       multiple times.  This is harmless and provides
         *       enough delay for the EEPROM write to complete.
         ****************************************************/

        // A small delay is needed to let EEPROM complete
        Timeout = 0;
        do
        {
            RegValue =
                PLX_9000_REG_READ(
                    pdx,
                    REG_EEPROM_CTRL
                    );

            Timeout++;
        }
        while (((RegValue & (1 << 27)) == 0) && (Timeout < 20000));

        // Send Write_Disable command to EEPROM
        Plx9000_EepromSendCommand(
            pdx,
            PLX9000_EE_CMD_WRITE_DISABLE << CommandShift,
            CommandLength
            );

        // Clear Chip Select and all other EEPROM bits
        PLX_9000_REG_WRITE(
            pdx,
            REG_EEPROM_CTRL,
            RegValue & ~(0xF << 24)
            );
    }

    return ApiSuccess;
}




/******************************************************************************
 *
 * Function   :  Plx9000_EepromSendCommand
 *
 * Description:  Sends a Command to the EEPROM
 *
 *****************************************************************************/
VOID
Plx9000_EepromSendCommand(
    DEVICE_EXTENSION *pdx,
    U32               EepromCommand,
    U8                DataLengthInBits
    )
{
    S8  BitPos;
    U32 RegValue;


    RegValue =
        PLX_9000_REG_READ(
            pdx,
            REG_EEPROM_CTRL
            );

    // Clear all EEPROM bits
    RegValue &= ~(0xF << 24);

    // Toggle EEPROM's Chip select to get it out of Shift Register Mode
    PLX_9000_REG_WRITE(
        pdx,
        REG_EEPROM_CTRL,
        RegValue
        );

    // Enable EEPROM Chip Select
    RegValue |= (1 << 25);

    PLX_9000_REG_WRITE(
        pdx,
        REG_EEPROM_CTRL,
        RegValue
        );

    // Send EEPROM command - one bit at a time
    for (BitPos = (S8)(DataLengthInBits-1); BitPos >= 0; BitPos--)
    {
        // Check if current bit is 0 or 1
        if (EepromCommand & (1 << BitPos))
        {
            PLX_9000_REG_WRITE(
                pdx,
                REG_EEPROM_CTRL,
                RegValue | (1 << 26)
                );
        }
        else
        {
            PLX_9000_REG_WRITE(
                pdx,
                REG_EEPROM_CTRL,
                RegValue
                );
        }

        Plx9000_EepromClock(
            pdx
            );
    }
}




/******************************************************************************
 *
 * Function   :  Plx9000_EepromClock
 *
 * Description:  Sends the clocking sequence to the EEPROM
 *
 *****************************************************************************/
VOID
Plx9000_EepromClock(
    DEVICE_EXTENSION *pdx
    )
{
    S8  i;
    U32 RegValue;


    RegValue =
        PLX_9000_REG_READ(
            pdx,
            REG_EEPROM_CTRL
            );

    // Set EEPROM clock High
    PLX_9000_REG_WRITE(
        pdx,
        REG_EEPROM_CTRL,
        RegValue | (1 << 24)
        );

    // Need a small delay, perform dummy register reads
    for (i=0; i<20; i++)
    {
        RegValue =
            PLX_9000_REG_READ(
                pdx,
                REG_EEPROM_CTRL
                );
    }

    // Set EEPROM clock Low
    PLX_9000_REG_WRITE(
        pdx,
        REG_EEPROM_CTRL,
        RegValue & ~(1 << 24)
        );
}
