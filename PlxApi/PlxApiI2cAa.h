#ifndef __PLX_API_I2C_AA_H
#define __PLX_API_I2C_AA_H

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
 *     PlxApiI2cAa.h
 *
 * Description:
 *
 *     The PLX API function prototypes for an I2C interface
 *
 * Revision:
 *
 *     04-01-12 : PLX SDK v7.00
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

// NT Legacy or P2P mode
#define PLX_I2C_NT_MODE_LEGACY      0
#define PLX_I2C_NT_MODE_P2P         1

#define PLX_I2C_CMD_ERROR           ((U32)-1)
#define PLX_I2C_CMD_SKIP            ((U32)-2)

#define Plx_pow_int                 pow         // Used to X^Y calculations

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
        #define PLX_PCI_REG_READ(pDevice, offset, pValue)   *(pValue) = PlxI2c_PlxRegisterRead( (pDevice), (offset), NULL, TRUE )
        #define PLX_PCI_REG_WRITE(pDevice, offset, value)   PlxI2c_PlxRegisterWrite( (pDevice), (offset), (value), TRUE )

        #define PLX_8000_REG_READ(pDevice, offset)          PlxI2c_PlxRegisterRead( (pDevice), (offset), NULL, FALSE )
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
    BOOLEAN            bAdjustForPort
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
PlxGetExtendedCapabilityOffset(
    PLX_DEVICE_OBJECT *pDevice,
    U16                CapabilityId
    );

BOOLEAN
PlxChipTypeDetect(
    PLX_DEVICE_OBJECT *pDevice
    );

VOID
PlxChipRevisionDetect(
    PLX_DEVICE_OBJECT *pDevice
    );



#ifdef __cplusplus
}
#endif

#endif
