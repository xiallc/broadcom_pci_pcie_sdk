#ifndef __PLX_API_DIRECT_H
#define __PLX_API_DIRECT_H

/*******************************************************************************
 * Copyright 2013-2019 Broadcom Inc
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
 *     PlxApiDirect.h
 *
 * Description:
 *
 *     PLX API function prototypes at API level for direct connect interfaces
 *
 * Revision:
 *
 *     01-01-19 : PCI/PCIe SDK v8.00
 *
 ******************************************************************************/


#if defined(PLX_LINUX)
    #include <pthread.h>    // For mutex support
    #include <unistd.h>     // For usleep()
#endif


#ifdef __cplusplus
extern "C" {
#endif




/******************************************
 *             Definitions
 ******************************************/

// For displaying connection mode in debug statements
#define DbgGetApiModeStr( pDev ) \
        ( ((pDev)->Key.ApiMode == PLX_API_MODE_SDB) ? "SDB" : \
          ((pDev)->Key.ApiMode == PLX_API_MODE_I2C_AARDVARK) ? "I2C" : \
          ((pDev)->Key.ApiMode == PLX_API_MODE_MDIO_SPLICE) ? "MDIO" : \
          "??" )

// AXI addresses of some register locations used for probe & register access
#define ATLAS_REGS_AXI_CCR_BASE_ADDR        0xFFF00000
#define ATLAS_REGS_AXI_BASE_ADDR            0x60800000
#define _REG_CCR(offset)                    (ATLAS_REGS_AXI_CCR_BASE_ADDR + (offset))

// Atlas PEX registers start offset
#if !defined(ATLAS_PEX_REGS_BASE_OFFSET)
    #define ATLAS_PEX_REGS_BASE_OFFSET      ATLAS_REGS_AXI_BASE_ADDR
#endif

#define PEX_MAX_PORT                        96
#define PEX_PORT_REGS_SIZE                  (4 * 1024)

// CCR registers
#define PEX_REG_CCR_DEV_ID                  _REG_CCR( 0x0 )
#define PMG_REG_CCR_PCIE_SW_MODE            _REG_CCR( 0xB0 )
#define PMG_REG_CCR_PORT_TYPE0              _REG_CCR( 0x120 )
#define PMG_REG_CCR_PCIE_CONFIG             _REG_CCR( 0x170 )

// Chip registers
#define PEX_REG_PORT_CLOCK_EN_0             0x30C    // Port clock enable for 0-31
#define PEX_REG_PORT_CLOCK_EN_1             0x310    // Port clock enable for 32-63
#define PEX_REG_PORT_CLOCK_EN_2             0x314    // Port clock enable for 64-95
#define PEX_REG_VS0_UPSTREAM                0x360    // Port upstream setting
#define PEX_REG_IDX_AXI_ADDR                0x1F0100 // Index access AXI address
#define PEX_REG_IDX_AXI_DATA                0x1F0104 // Index access data
#define PEX_REG_IDX_AXI_CTRL                0x1F0108 // Index access control

// Per port registers
#define PEX_OFF_PORT_CAP_BUS                0x97C   // Port captured bus

// CSR index method control
#define PEX_IDX_CTRL_CMD_READ               (1 << 1) // READ command
#define PEX_IDX_CTRL_CMD_WRITE              (1 << 0) // WRITE command
#define PEX_IDX_CTRL_READ_VALID             (1 << 3) // Indicates READ completed
#define PEX_IDX_CTRL_BUSY                   (1 << 2) // Indicates operation pending




/**********************************************
 *           Portability Functions
 *********************************************/
#if defined(PLX_MSWINDOWS)

    #define Plx_sleep                       Sleep

#elif defined(PLX_LINUX)

    #define Plx_sleep(arg)                  usleep((arg) * 1000)

    #define CRITICAL_SECTION                pthread_mutex_t
    #define InitializeCriticalSection(pCS)  pthread_mutex_init   ( (pCS), NULL )
    #define DeleteCriticalSection(pCS)      pthread_mutex_destroy( (pCS) )
    #define EnterCriticalSection(pCS)       pthread_mutex_lock   ( (pCS) )
    #define LeaveCriticalSection(pCS)       pthread_mutex_unlock ( (pCS) )

    #define InterlockedIncrement( pVal )    ++(*(pVal))
    #define InterlockedDecrement( pVal )    --(*(pVal))

#endif

#if !defined(PLX_DOS)
    // Macros for PLX chip register access
    #define PLX_PCI_REG_READ(pDevice, offset, pValue)   *(pValue) = PlxDir_PlxRegRead( (pDevice), (U16)(offset), NULL )
    #define PLX_PCI_REG_WRITE(pDevice, offset, value)   PlxDir_PlxRegWrite( (pDevice), (U16)(offset), (value) )
    #define PLX_8000_REG_READ(pDevice, offset)          PlxDir_PlxMappedRegRead( (pDevice), (offset), NULL )
    #define PLX_8000_REG_WRITE(pDevice, offset, value)  PlxDir_PlxMappedRegWrite( (pDevice), (offset), (value) )
#endif




/******************************************
 *    Query for Information Functions
 *****************************************/
PLX_STATUS
PlxDir_ChipTypeGet(
    PLX_DEVICE_OBJECT *pDevice,
    U16               *pChipType,
    U8                *pRevision
    );

PLX_STATUS
PlxDir_ChipTypeSet(
    PLX_DEVICE_OBJECT *pDevice,
    U16                ChipType,
    U8                 Revision
    );

PLX_STATUS
PlxDir_GetPortProperties(
    PLX_DEVICE_OBJECT *pDevice,
    PLX_PORT_PROP     *pPortProp
    );


/******************************************
 *           PCI BAR Functions
 *****************************************/
PLX_STATUS
PlxDir_PciBarProperties(
    PLX_DEVICE_OBJECT *pDevice,
    U8                 BarIndex,
    PLX_PCI_BAR_PROP  *pBarProperties
    );


/******************************************
 *   Performance Monitoring Functions
 *****************************************/
PLX_STATUS
PlxDir_PerformanceInitializeProperties(
    PLX_DEVICE_OBJECT *pDevice,
    PLX_PERF_PROP     *pPerfObject
    );

PLX_STATUS
PlxDir_PerformanceMonitorControl(
    PLX_DEVICE_OBJECT *pDevice,
    PLX_PERF_CMD       command
    );

PLX_STATUS
PlxDir_PerformanceResetCounters(
    PLX_DEVICE_OBJECT *pDevice
    );

PLX_STATUS
PlxDir_PerformanceGetCounters(
    PLX_DEVICE_OBJECT *pDevice,
    PLX_PERF_PROP     *pPerfProps,
    U8                 NumOfObjects
    );


/******************************************
 *      Private Support Functions
 *****************************************/
U16
PlxDir_PciFindCapability(
    PLX_DEVICE_OBJECT *pDevice,
    U16                CapID,
    U8                 bPCIeCap,
    U8                 InstanceNum
    );

BOOLEAN
PlxDir_ChipTypeDetect(
    PLX_DEVICE_OBJECT *pDevice
    );

VOID
PlxDir_ChipRevisionDetect(
    PLX_DEVICE_OBJECT *pDevice
    );

PLX_STATUS
PlxDir_ChipFilterDisabledPorts(
    PLX_DEVICE_OBJECT *pDevice,
    U64               *pPortMask
    );

PLX_STATUS
PlxDir_ProbeSwitch(
    PLX_DEVICE_OBJECT *pDevice,
    PLX_DEVICE_KEY    *pKey,
    U16                DeviceNumber,
    U16               *pNumMatched
    );


/******************************************
 *  Private Register Dispatch Functions
 *****************************************/
U32
PlxDir_PlxRegRead(
    PLX_DEVICE_OBJECT *pDevice,
    U16                offset,
    PLX_STATUS        *pStatus
    );

PLX_STATUS
PlxDir_PlxRegWrite(
    PLX_DEVICE_OBJECT *pDevice,
    U16                offset,
    U32                value
    );

U32
PlxDir_PlxMappedRegRead(
    PLX_DEVICE_OBJECT *pDevice,
    U32                offset,
    PLX_STATUS        *pStatus
    );

PLX_STATUS
PlxDir_PlxMappedRegWrite(
    PLX_DEVICE_OBJECT *pDevice,
    U32                offset,
    U32                value
    );



#ifdef __cplusplus
}
#endif

#endif
