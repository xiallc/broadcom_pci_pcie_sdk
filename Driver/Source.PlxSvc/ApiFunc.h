#ifndef __API_FUNCTIONS_H
#define __API_FUNCTIONS_H

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
 *      ApiFunc.h
 *
 * Description:
 *
 *      The header file for the PLX API functions
 *
 * Revision History:
 *
 *      02-01-13 : PLX SDK v7.00
 *
 ******************************************************************************/


#include "DrvDefs.h"




/**********************************************
 *                Functions
 *********************************************/
PLX_STATUS
PlxDeviceFind(
    DEVICE_EXTENSION *pdx,
    PLX_DEVICE_KEY   *pKey,
    U16              *pDeviceNumber
    );

PLX_STATUS
PlxChipTypeGet(
    PLX_DEVICE_NODE *pdx,
    U16             *pChipType,
    U8              *pRevision
    );

PLX_STATUS
PlxChipTypeSet(
    PLX_DEVICE_NODE *pdx,
    U16              ChipType,
    U8              *pRevision,
    U8              *pFamily
    );

PLX_STATUS
PlxGetPortProperties(
    PLX_DEVICE_NODE *pdx,
    PLX_PORT_PROP   *pPortProp
    );

PLX_STATUS
PlxPciDeviceReset(
    PLX_DEVICE_NODE *pdx
    );

U32
PlxRegisterRead(
    PLX_DEVICE_NODE *pdx,
    U32              offset,
    PLX_STATUS      *pStatus,
    BOOLEAN          bAdjustForPort
    );

PLX_STATUS
PlxRegisterWrite(
    PLX_DEVICE_NODE *pdx,
    U32              offset,
    U32              value,
    BOOLEAN          bAdjustForPort
    );

PLX_STATUS
PlxPciBarProperties(
    PLX_DEVICE_NODE  *pdx,
    U8                BarIndex,
    PLX_PCI_BAR_PROP *pBarProperties
    );

PLX_STATUS
PlxPciBarMap(
    PLX_DEVICE_NODE *pdx,
    U8               BarIndex,
    VOID            *pUserVa
    );

PLX_STATUS
PlxPciBarUnmap(
    PLX_DEVICE_NODE *pdx,
    VOID            *UserVa
    );

PLX_STATUS
PlxEepromPresent(
    PLX_DEVICE_NODE *pdx,
    U8              *pStatus
    );

PLX_STATUS
PlxEepromProbe(
    PLX_DEVICE_NODE *pdx,
    U8              *pFlag
    );

PLX_STATUS
PlxEepromGetAddressWidth(
    PLX_DEVICE_NODE *pdx,
    U8              *pWidth
    );

PLX_STATUS
PlxEepromSetAddressWidth(
    PLX_DEVICE_NODE *pdx,
    U8               width
    );

PLX_STATUS
PlxEepromCrcGet(
    PLX_DEVICE_NODE *pdx,
    U32             *pCrc,
    U8              *pCrcStatus
    );

PLX_STATUS
PlxEepromCrcUpdate(
    PLX_DEVICE_NODE *pdx,
    U32             *pCrc,
    BOOLEAN          bUpdateEeprom
    );

PLX_STATUS
PlxEepromReadByOffset(
    PLX_DEVICE_NODE *pdx,
    U32              offset,
    U32             *pValue
    );

PLX_STATUS
PlxEepromWriteByOffset(
    PLX_DEVICE_NODE *pdx,
    U32              offset,
    U32              value
    );

PLX_STATUS
PlxEepromReadByOffset_16(
    PLX_DEVICE_NODE *pdx,
    U32              offset,
    U16             *pValue
    );

PLX_STATUS
PlxEepromWriteByOffset_16(
    PLX_DEVICE_NODE *pdx,
    U32              offset,
    U16              value
    );

PLX_STATUS
PlxPciIoPortTransfer(
    U64              IoPort,
    VOID            *pBuffer,
    U32              SizeInBytes,
    PLX_ACCESS_TYPE  AccessType,
    BOOLEAN          bReadOperation
    );

PLX_STATUS
PlxPciPhysicalMemoryAllocate(
    PLX_DEVICE_NODE  *pdx,
    PLX_PHYSICAL_MEM *pPciMem,
    BOOLEAN           bSmallerOk
    );

PLX_STATUS
PlxPciPhysicalMemoryFree(
    PLX_DEVICE_NODE  *pdx,
    PLX_PHYSICAL_MEM *pPciMem
    );

PLX_STATUS
PlxPciPhysicalMemoryMap(
    PLX_DEVICE_NODE  *pdx,
    PLX_PHYSICAL_MEM *pPciMem
    );

PLX_STATUS
PlxPciPhysicalMemoryUnmap(
    PLX_DEVICE_NODE  *pdx,
    PLX_PHYSICAL_MEM *pPciMem
    );

PLX_STATUS
PlxPciPerformanceInitializeProperties(
    PLX_DEVICE_NODE *pdx,
    PLX_PERF_PROP   *pPerfProp
    );

PLX_STATUS
PlxPciPerformanceMonitorControl(
    PLX_DEVICE_NODE *pdx,
    PLX_PERF_CMD     command
    );

PLX_STATUS
PlxPciPerformanceResetCounters(
    PLX_DEVICE_NODE *pdx
    );

PLX_STATUS
PlxPciPerformanceGetCounters(
    PLX_DEVICE_NODE *pdx,
    PLX_PERF_PROP   *pPerfProps,
    U8               NumOfObjects
    );

PLX_STATUS
PlxMH_GetProperties(
    PLX_DEVICE_NODE     *pdx,
    PLX_MULTI_HOST_PROP *pMHProp
    );

PLX_STATUS
PlxMH_MigrateDsPorts(
    PLX_DEVICE_NODE *pdx,
    U16              VS_Source,
    U16              VS_Dest,
    U32              DsPortMask,
    BOOLEAN          bResetSrc
    );



#endif
