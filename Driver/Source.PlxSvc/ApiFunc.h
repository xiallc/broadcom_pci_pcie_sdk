#ifndef __API_FUNCTIONS_H
#define __API_FUNCTIONS_H

/*******************************************************************************
 * Copyright 2013-2015 Avago Technologies
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
