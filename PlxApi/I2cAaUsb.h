#ifndef __I2C_AA_USB_H
#define __I2C_AA_USB_H

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
 *     I2cAaUsb.h
 *
 * Description:
 *
 *     The PLX API support function prototypes for Aardark I2C interface
 *
 * Revision:
 *
 *     01-01-18 : PLX SDK v8.00
 *
 ******************************************************************************/


#include "PlxIoctl.h"


#ifdef __cplusplus
extern "C" {
#endif




/******************************************
 *             Definitions
 ******************************************/
#define I2C_MAX_DEVICES             10          // Max number of I2C USB devices supported
#define I2C_MAX_NT_PORTS            2           // Max number of NT ports in single switch
#define I2C_DEFAULT_CLOCK_RATE      100         // I2C default clock rate in Khz
#define I2C_CMD_REG_READ            0x04        // I2C read command code
#define I2C_CMD_REG_WRITE           0x03        // I2C write command code
#define I2C_CMD_ERROR               ((U32)-1)   // Reserved command value to denote error
#define I2C_CMD_SKIP                ((U32)-2)   // Reserved command value to denote skip operation
#define I2C_RETRY_MAX_COUNT         3           // On error, max retry count
#define I2C_RETRY_DELAY_MS          300         // Delay in ms to wait before command retry
#define I2C_HIGH_ADDR_OFFSET        0x2CC       // Register to contain high address bits
#define I2C_HIGH_ADDR_INIT          0xFF

// Various addressing modes, which vary between chips
#define I2C_ADDR_MODE_STD           0           // Standard (ports only)
#define I2C_ADDR_MODE_NON_STD       1           // Non-standard
#define I2C_ADDR_MODE_FULL          2           // Full
#define I2C_ADDR_MODE_NTV           2           // NT-Virtual
#define I2C_ADDR_MODE_NTL           1           // NT-Link
#define I2C_ADDR_MODE_NT_P2P        1           // NT parent P2P
#define I2C_ADDR_MODE_DMA           3           // DMA & DMA RAM
#define I2C_ADDR_MODE_ALUT          3           // ALUT

#define I2C_PEX_BASE_ADDR_MASK      0xFF800000  // PEX region base address mask
#define I2C_PEX_MAX_OFFSET_MASK     0x007FFFFF  // Max I2C addressing (23 bits)




/******************************************
 *      PLX Device Selection Functions
 *****************************************/
PLX_STATUS
PlxI2c_I2cGetPorts(
    PLX_API_MODE  ApiMode,
    U32          *pI2cPorts
    );

PLX_STATUS
PlxI2c_DeviceOpen(
    PLX_DEVICE_OBJECT *pDevice
    );

PLX_STATUS
PlxI2c_DeviceClose(
    PLX_DEVICE_OBJECT *pDevice
    );

PLX_STATUS
PlxI2c_DeviceFindEx(
    PLX_DEVICE_KEY *pKey,
    U16             DeviceNumber,
    PLX_MODE_PROP  *pModeProp
    );


/******************************************
 *    Query for Information Functions
 *****************************************/
PLX_STATUS
PlxI2c_I2cVersion(
    U16          I2cPort,
    PLX_VERSION *pVersion
    );


/******************************************
 * Device-specific Register Access Functions
 *****************************************/
U32
PlxI2c_PlxRegisterRead(
    PLX_DEVICE_OBJECT *pDevice,
    U32                offset,
    PLX_STATUS        *pStatus,
    BOOLEAN            bAdjustForPort,
    U16                bRetryOnError
    );

PLX_STATUS
PlxI2c_PlxRegisterWrite(
    PLX_DEVICE_OBJECT *pDevice,
    U32                offset,
    U32                value,
    BOOLEAN            bAdjustForPort
    );


/******************************************
 *     Serial EEPROM Access Functions
 *****************************************/
PLX_STATUS
PlxI2c_EepromPresent(
    PLX_DEVICE_OBJECT *pDevice,
    PLX_EEPROM_STATUS *pStatus
    );

PLX_STATUS
PlxI2c_EepromProbe(
    PLX_DEVICE_OBJECT *pDevice,
    BOOLEAN           *pFlag
    );

PLX_STATUS
PlxI2c_EepromGetAddressWidth(
    PLX_DEVICE_OBJECT *pDevice,
    U8                *pWidth
    );

PLX_STATUS
PlxI2c_EepromSetAddressWidth(
    PLX_DEVICE_OBJECT *pDevice,
    U8                 width
    );

PLX_STATUS
PlxI2c_EepromCrcUpdate(
    PLX_DEVICE_OBJECT *pDevice,
    U32               *pCrc,
    BOOLEAN            bUpdateEeprom
    );

PLX_STATUS
PlxI2c_EepromCrcGet(
    PLX_DEVICE_OBJECT *pDevice,
    U32               *pCrc,
    U8                *pCrcStatus
    );

PLX_STATUS
PlxI2c_EepromReadByOffset(
    PLX_DEVICE_OBJECT *pDevice,
    U32                offset,
    U32               *pValue
    );

PLX_STATUS
PlxI2c_EepromWriteByOffset(
    PLX_DEVICE_OBJECT *pDevice,
    U32                offset,
    U32                value
    );

PLX_STATUS
PlxI2c_EepromReadByOffset_16(
    PLX_DEVICE_OBJECT *pDevice,
    U32                offset,
    U16               *pValue
    );

PLX_STATUS
PlxI2c_EepromWriteByOffset_16(
    PLX_DEVICE_OBJECT *pDevice,
    U32                offset,
    U16                value
    );


/******************************************
 *          Multi-VS Functions
 *****************************************/
PLX_STATUS
PlxI2c_MH_GetProperties(
    PLX_DEVICE_OBJECT   *pDevice,
    PLX_MULTI_HOST_PROP *pMHProp
    );

PLX_STATUS
PlxI2c_MH_MigrateDsPorts(
    PLX_DEVICE_OBJECT *pDevice,
    U16                VS_Source,
    U16                VS_Dest,
    U32                DsPortMask,
    BOOLEAN            bResetSrc
    );


/******************************************
 *      Private Support Functions
 *****************************************/
U32
PlxI2c_GenerateCommand(
    PLX_DEVICE_OBJECT *pDevice,
    U8                 I2cOperation,
    U32                Address,
    BOOLEAN            bAdjustForPort
    );

BOOLEAN
PlxI2c_Driver_Connect(
    PLX_DEVICE_OBJECT *pDevice,
    PLX_MODE_PROP     *pModeProp
    );

S32
PlxI2c_Dispatch_IoControl(
    PLX_DEVICE_OBJECT *pDevice,
    U32                IoControlCode,
    PLX_PARAMS        *pIoBuffer,
    U32                Size
    );

PLX_STATUS
PlxI2c_ProbeSwitch(
    PLX_DEVICE_OBJECT *pDevice,
    PLX_DEVICE_KEY    *pKey,
    U16                DeviceNumber,
    U16               *pNumMatched
    );



#ifdef __cplusplus
}
#endif

#endif
