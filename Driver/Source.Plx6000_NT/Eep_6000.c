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
 *      Eep_6000.c
 *
 * Description:
 *
 *      This file contains 6000-series EEPROM support functions
 *
 * Revision History:
 *
 *      08-01-11 : PLX SDK v6.50
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
    DEVICE_EXTENSION *pdx,
    U32               offset,
    U16              *pValue
    )
{
    U16 Offset_EepromCtrl;
    U32 RegSave;
    U32 RegValue;


    // Save the chip control register
    PLX_PCI_REG_READ(
        pdx,
        0xD8,
        &RegSave
        );

    // Enable shadow register access
    PLX_PCI_REG_WRITE(
        pdx,
        0xD8,
        RegSave | (1 << 6)
        );

    // Set EEPROM control register offset
    Offset_EepromCtrl = 0x54;

    // Offset can only be 8-bits
    offset = offset & 0xFF;

    // Prepare EEPROM read command
    RegValue = ((U32)offset << 8) | (1 << 0);

    // Write EEPROM command
    PLX_PCI_REG_WRITE(
        pdx,
        Offset_EepromCtrl,
        RegValue
        );

    // Insert a small delay
    Plx_sleep(50);

    // Read data
    PLX_PCI_REG_READ(
        pdx,
        Offset_EepromCtrl,
        &RegValue
        );

    // Store data
    *pValue = (U16)(RegValue >> 16);

    // Restore chip control register
    PLX_PCI_REG_WRITE(
        pdx,
        0xD8,
        RegSave
        );

    return ApiSuccess;
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
    DEVICE_EXTENSION *pdx,
    U32               offset,
    U16               value
    )
{
    U16 Offset_EepromCtrl;
    U32 RegSave;
    U32 RegValue;


    // Save the chip control register
    PLX_PCI_REG_READ(
        pdx,
        0xD8,
        &RegSave
        );

    // Enable shadow register access
    PLX_PCI_REG_WRITE(
        pdx,
        0xD8,
        RegSave | (1 << 6)
        );

    // Set EEPROM control register offset
    Offset_EepromCtrl = 0x54;

    // Offset can only be 8-bits
    offset = offset & 0xFF;

    // Prepare EEPROM write command & data
    RegValue = (value << 16) | ((U32)offset << 8) | (1 << 1) | (1 << 0);

    // Write EEPROM command
    PLX_PCI_REG_WRITE(
        pdx,
        Offset_EepromCtrl,
        RegValue
        );

    // Insert a small delay
    Plx_sleep(100);

    // Restore chip control register
    PLX_PCI_REG_WRITE(
        pdx,
        0xD8,
        RegSave
        );

    return ApiSuccess;
}
