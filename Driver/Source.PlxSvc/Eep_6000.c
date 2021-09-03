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
 *      Eep_6000.c
 *
 * Description:
 *
 *      This file contains 6000-series EEPROM support functions
 *
 * Revision History:
 *
 *      05-01-18 : PLX SDK v8.00
 *
 ******************************************************************************/


#include "Eep_6000.h"
#include "PciFunc.h"
#include "SuppFunc.h"




/******************************************************************************
 *
 * Function   :  Plx6000_EepromReadByOffset_16
 *
 * Description:  Read a 16-bit value from the EEPROM at a specified offset
 *
 ******************************************************************************/
PLX_STATUS
Plx6000_EepromReadByOffset_16(
    PLX_DEVICE_NODE *pdx,
    U32              offset,
    U16             *pValue
    )
{
    U16 Offset_EepromCtrl;
    U32 RegSave;
    U32 RegValue;


    switch (pdx->Key.PlxChip)
    {
        case 0x6152:
        case 0x6156:
            Offset_EepromCtrl = 0xC8;
            break;

        case 0x6150:
        case 0x6154:
        case 0x6254:
        case 0x6350:
        case 0x6520:
        case 0x6540:
            Offset_EepromCtrl = 0x54;
            break;

        case 0x6140:    // No EEPROM support
        default:
            DebugPrintf((
                "ERROR - Unsupported PLX chip type (%04X)\n",
                pdx->Key.PlxChip
                ));
            return PLX_STATUS_UNSUPPORTED;
    }

    // For Non-transparent mode chips, enable shadow registers
    if ((pdx->Key.PlxChip == 0x6254) ||
        (pdx->Key.PlxChip == 0x6540))
    {
        // Save the chip control register
        PLX_PCI_REG_READ( pdx, 0xD8, &RegSave );

        // Enable shadow register access
        PLX_PCI_REG_WRITE( pdx, 0xD8, RegSave | (1 << 6) );
    }

    // Offset can only be 8-bits
    offset = offset & 0xFF;

    // Prepare EEPROM read command
    RegValue = ((U32)offset << 8) | (1 << 0);

    // Write EEPROM command
    PLX_PCI_REG_WRITE( pdx, Offset_EepromCtrl, RegValue );

    // Insert a small delay
    Plx_sleep(50);

    // Read data
    PLX_PCI_REG_READ( pdx, Offset_EepromCtrl, &RegValue );

    // Store data
    *pValue = (U16)(RegValue >> 16);

    if ((pdx->Key.PlxChip == 0x6254) ||
        (pdx->Key.PlxChip == 0x6540))
    {
        // Restore chip control register
        PLX_PCI_REG_WRITE( pdx, 0xD8, RegSave );
    }

    return PLX_STATUS_OK;
}




/******************************************************************************
 *
 * Function   :  Plx6000_EepromWriteByOffset_16
 *
 * Description:  Write a 16-bit value to the EEPROM at a specified offset
 *
 ******************************************************************************/
PLX_STATUS
Plx6000_EepromWriteByOffset_16(
    PLX_DEVICE_NODE *pdx,
    U32              offset,
    U16              value
    )
{
    U16 Offset_EepromCtrl;
    U32 RegSave;
    U32 RegValue;


    switch (pdx->Key.PlxChip)
    {
        case 0x6152:
        case 0x6156:
            Offset_EepromCtrl = 0xC8;
            break;

        case 0x6150:
        case 0x6154:
        case 0x6254:
        case 0x6350:
        case 0x6520:
        case 0x6540:
            Offset_EepromCtrl = 0x54;
            break;

        case 0x6140:    // No EEPROM support
        default:
            DebugPrintf((
                "ERROR - Unsupported PLX chip type (%04X)\n",
                pdx->Key.PlxChip
                ));
            return PLX_STATUS_UNSUPPORTED;
    }

    // For Non-transparent mode chips, enable shadow registers
    if ((pdx->Key.PlxChip == 0x6254) ||
        (pdx->Key.PlxChip == 0x6540))
    {
        // Save the chip control register
        PLX_PCI_REG_READ( pdx, 0xD8, &RegSave );

        // Enable shadow register access
        PLX_PCI_REG_WRITE( pdx, 0xD8, RegSave | (1 << 6) );
    }

    // Offset can only be 8-bits
    offset = offset & 0xFF;

    // Prepare EEPROM write command & data
    RegValue = (value << 16) | ((U32)offset << 8) | (1 << 1) | (1 << 0);

    // Write EEPROM command
    PLX_PCI_REG_WRITE( pdx, Offset_EepromCtrl, RegValue );

    // Insert a small delay
    Plx_sleep(100);

    if ((pdx->Key.PlxChip == 0x6254) ||
        (pdx->Key.PlxChip == 0x6540))
    {
        // Restore chip control register
        PLX_PCI_REG_WRITE( pdx, 0xD8, RegSave );
    }

    return PLX_STATUS_OK;
}
