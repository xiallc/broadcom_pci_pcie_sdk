#ifndef __PLX_API_I2C_AA_H
#define __PLX_API_I2C_AA_H

/*******************************************************************************
 * Copyright 2013-2016 Avago Technologies
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
 *     PlxApiI2cAa.h
 *
 * Description:
 *
 *     The PLX API function prototypes for an I2C interface
 *
 * Revision:
 *
 *     09-01-16 : PLX SDK v7.25
 *
 ******************************************************************************/


#include "PlxIoctl.h"
#if defined(PLX_DEMO_API)
    #include "PexApi.h"
#endif


#ifdef __cplusplus
extern "C" {
#endif




/******************************************
 *             Definitions
 ******************************************/
#define PLX_I2C_MAX_DEVICES         10          // Max number of I2C USB devices supported
#define PLX_I2C_MAX_NT_PORTS        2           // Max number of NT ports in single switch
#define PLX_I2C_DEFAULT_CLOCK_RATE  100         // I2C default clock rate in Khz
#define PLX_I2C_CMD_REG_READ        0x04        // I2C read command code
#define PLX_I2C_CMD_REG_WRITE       0x03        // I2C write command code
#define PLX_I2C_RETRY_MAX_COUNT     3           // On error, max retry count
#define PLX_I2C_RETRY_DELAY_MS      300         // Delay in ms to wait before command retry
#define PLX_I2C_CMD_ERROR           ((U32)-1)
#define PLX_I2C_CMD_SKIP            ((U32)-2)

#if defined(PLX_MSWINDOWS)
    #define Plx_sleep               Sleep
#elif defined(PLX_LINUX)
    #define Plx_sleep(arg)          usleep((arg) * 1000)
#endif

#if !defined(PLX_DOS)
    // Macros for PLX chip register access
    #if defined(PLX_DEMO_API)
        #define PLX_PCI_REG_READ(pDevice, offset, pValue)   *(pValue) = PlxPci_PciRegisterReadFast( (pDevice), (U16)(offset), NULL )
        #define PLX_PCI_REG_WRITE(pDevice, offset, value)   PlxPci_PciRegisterWriteFast( (pDevice), (U16)(offset), (value) )

        #define PLX_8000_REG_READ(pDevice, offset)          PlxPci_PlxRegisterRead( (pDevice), (offset), NULL )
        #define PLX_8000_REG_WRITE(pDevice, offset, value)  PlxPci_PlxRegisterWrite( (pDevice), (offset), (value) )
    #else
        #define PLX_PCI_REG_READ(pDevice, offset, pValue)   *(pValue) = PlxI2c_PlxRegisterRead( (pDevice), (offset), NULL, TRUE, TRUE )
        #define PLX_PCI_REG_WRITE(pDevice, offset, value)   PlxI2c_PlxRegisterWrite( (pDevice), (offset), (value), TRUE )

        #define PLX_8000_REG_READ(pDevice, offset)          PlxI2c_PlxRegisterRead( (pDevice), (offset), NULL, FALSE, TRUE )
        #define PLX_8000_REG_WRITE(pDevice, offset, value)  PlxI2c_PlxRegisterWrite( (pDevice), (offset), (value), FALSE )
    #endif
#endif




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

PLX_STATUS
PlxI2c_ChipTypeGet(
    PLX_DEVICE_OBJECT *pDevice,
    U16               *pChipType,
    U8                *pRevision
    );

PLX_STATUS
PlxI2c_ChipTypeSet(
    PLX_DEVICE_OBJECT *pDevice,
    U16                ChipType,
    U8                 Revision
    );

PLX_STATUS
PlxI2c_GetPortProperties(
    PLX_DEVICE_OBJECT *pDevice,
    PLX_PORT_PROP     *pPortProp
    );


/******************************************
 *        Device Control Functions
 *****************************************/
PLX_STATUS
PlxI2c_DeviceReset(
    PLX_DEVICE_OBJECT *pDevice
    );


/******************************************
 * PLX-specific Register Access Functions
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
 *           PCI BAR Functions
 *****************************************/
PLX_STATUS
PlxI2c_PciBarProperties(
    PLX_DEVICE_OBJECT *pDevice,
    U8                 BarIndex,
    PLX_PCI_BAR_PROP  *pBarProperties
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
 *   Performance Monitoring Functions
 *****************************************/
PLX_STATUS
PlxI2c_PerformanceInitializeProperties(
    PLX_DEVICE_OBJECT *pDevice,
    PLX_PERF_PROP     *pPerfObject
    );

PLX_STATUS
PlxI2c_PerformanceMonitorControl(
    PLX_DEVICE_OBJECT *pDevice,
    PLX_PERF_CMD       command
    );

PLX_STATUS
PlxI2c_PerformanceResetCounters(
    PLX_DEVICE_OBJECT *pDevice
    );

PLX_STATUS
PlxI2c_PerformanceGetCounters(
    PLX_DEVICE_OBJECT *pDevice,
    PLX_PERF_PROP     *pPerfProps,
    U8                 NumOfObjects
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
 *      I2C Private Support Functions
 *****************************************/
VOID
Plx_delay_us(
    U32 Time_us
    );

U32
PlxI2c_GenerateCommand(
    PLX_DEVICE_OBJECT *pDevice,
    U8                 I2cOperation,
    U32                offset,
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

U16
PlxPciFindCapability(
    PLX_DEVICE_OBJECT *pDevice,
    U16                CapID,
    U8                 bPCIeCap,
    U8                 InstanceNum
    );

BOOLEAN
PlxChipTypeDetect(
    PLX_DEVICE_OBJECT *pDevice
    );

VOID
PlxChipRevisionDetect(
    PLX_DEVICE_OBJECT *pDevice
    );

PLX_STATUS
PlxChipFilterDisabledPorts(
    PLX_DEVICE_OBJECT *pDevice,
    U64               *pPortMask
    );



#ifdef __cplusplus
}
#endif

#endif
