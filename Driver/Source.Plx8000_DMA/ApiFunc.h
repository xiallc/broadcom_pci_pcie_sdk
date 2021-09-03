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
    DEVICE_EXTENSION *pdx,
    U16              *pChipType,
    U8               *pRevision
    );

PLX_STATUS
PlxChipTypeSet(
    DEVICE_EXTENSION *pdx,
    U16               ChipType,
    U8                Revision
    );

PLX_STATUS
PlxGetPortProperties(
    DEVICE_EXTENSION *pdx,
    PLX_PORT_PROP    *pPortProp
    );

PLX_STATUS
PlxPciDeviceReset(
    DEVICE_EXTENSION *pdx
    );

U32
PlxRegisterRead(
    DEVICE_EXTENSION *pdx,
    U32               offset,
    PLX_STATUS       *pStatus,
    BOOLEAN           bAdjustForPort
    );

PLX_STATUS
PlxRegisterWrite(
    DEVICE_EXTENSION *pdx,
    U32               offset,
    U32               value,
    BOOLEAN           bAdjustForPort
    );

PLX_STATUS
PlxPciBarProperties(
    DEVICE_EXTENSION *pdx,
    U8                BarIndex,
    PLX_PCI_BAR_PROP *pBarProperties
    );

PLX_STATUS
PlxPciBarMap(
    DEVICE_EXTENSION *pdx,
    U8                BarIndex,
    VOID             *pUserVa,
    VOID             *pOwner
    );

PLX_STATUS
PlxPciBarUnmap(
    DEVICE_EXTENSION *pdx,
    VOID             *UserVa,
    VOID             *pOwner
    );

PLX_STATUS
PlxEepromPresent(
    DEVICE_EXTENSION *pdx,
    U8               *pStatus
    );

PLX_STATUS
PlxEepromProbe(
    DEVICE_EXTENSION *pdx,
    U8               *pFlag
    );

PLX_STATUS
PlxEepromGetAddressWidth(
    DEVICE_EXTENSION *pdx,
    U8               *pWidth
    );

PLX_STATUS
PlxEepromSetAddressWidth(
    DEVICE_EXTENSION *pdx,
    U8                width
    );

PLX_STATUS
PlxEepromCrcGet(
    DEVICE_EXTENSION *pdx,
    U32              *pCrc,
    U8               *pCrcStatus
    );

PLX_STATUS
PlxEepromCrcUpdate(
    DEVICE_EXTENSION *pdx,
    U32              *pCrc,
    BOOLEAN           bUpdateEeprom
    );

PLX_STATUS
PlxEepromReadByOffset(
    DEVICE_EXTENSION *pdx,
    U32               offset,
    U32              *pValue
    );

PLX_STATUS
PlxEepromWriteByOffset(
    DEVICE_EXTENSION *pdx,
    U32               offset,
    U32               value
    );

PLX_STATUS
PlxEepromReadByOffset_16(
    DEVICE_EXTENSION *pdx,
    U32               offset,
    U16              *pValue
    );

PLX_STATUS
PlxEepromWriteByOffset_16(
    DEVICE_EXTENSION *pdx,
    U32               offset,
    U16               value
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
    DEVICE_EXTENSION *pdx,
    PLX_PHYSICAL_MEM *pPciMem,
    BOOLEAN           bSmallerOk,
    VOID             *pOwner
    );

PLX_STATUS
PlxPciPhysicalMemoryFree(
    DEVICE_EXTENSION *pdx,
    PLX_PHYSICAL_MEM *pPciMem
    );

PLX_STATUS
PlxPciPhysicalMemoryMap(
    DEVICE_EXTENSION *pdx,
    PLX_PHYSICAL_MEM *pPciMem,
    VOID             *pOwner
    );

PLX_STATUS
PlxPciPhysicalMemoryUnmap(
    DEVICE_EXTENSION *pdx,
    PLX_PHYSICAL_MEM *pPciMem,
    VOID             *pOwner
    );

PLX_STATUS
PlxInterruptEnable(
    DEVICE_EXTENSION *pdx,
    PLX_INTERRUPT    *pPlxIntr
    );

PLX_STATUS
PlxInterruptDisable(
    DEVICE_EXTENSION *pdx,
    PLX_INTERRUPT    *pPlxIntr
    );

PLX_STATUS
PlxNotificationRegisterFor(
    DEVICE_EXTENSION  *pdx,
    PLX_INTERRUPT     *pPlxIntr,
    VOID             **pUserWaitObject,
    VOID              *pOwner
    );

PLX_STATUS
PlxNotificationWait(
    DEVICE_EXTENSION *pdx,
    VOID             *pUserWaitObject,
    PLX_UINT_PTR      Timeout_ms
    );

PLX_STATUS
PlxNotificationStatus(
    DEVICE_EXTENSION *pdx,
    VOID             *pUserWaitObject,
    PLX_INTERRUPT    *pPlxIntr
    );

PLX_STATUS
PlxNotificationCancel(
    DEVICE_EXTENSION *pdx,
    VOID             *pUserWaitObject,
    VOID             *pOwner
    );

PLX_STATUS
PlxDmaChannelOpen(
    DEVICE_EXTENSION *pdx,
    U8                channel,
    VOID             *pOwner
    );

PLX_STATUS
PlxDmaGetProperties(
    DEVICE_EXTENSION *pdx,
    U8                channel,
    PLX_DMA_PROP     *pProp
    );

PLX_STATUS
PlxDmaSetProperties(
    DEVICE_EXTENSION *pdx,
    U8                channel,
    PLX_DMA_PROP     *pProp,
    VOID             *pOwner
    );

PLX_STATUS
PlxDmaControl(
    DEVICE_EXTENSION *pdx,
    U8                channel,
    PLX_DMA_COMMAND   command,
    VOID             *pOwner
    );

PLX_STATUS
PlxDmaStatus(
    DEVICE_EXTENSION *pdx,
    U8                channel,
    VOID             *pOwner
    );

PLX_STATUS
PlxDmaTransferBlock(
    DEVICE_EXTENSION *pdx,
    U8                channel,
    PLX_DMA_PARAMS   *pParams,
    VOID             *pOwner
    );

PLX_STATUS
PlxDmaTransferUserBuffer(
    DEVICE_EXTENSION *pdx,
    U8                channel,
    PLX_DMA_PARAMS   *pParams,
    VOID             *pOwner
    );

PLX_STATUS
PlxDmaChannelClose(
    DEVICE_EXTENSION *pdx,
    U8                channel,
    BOOLEAN           bCheckInProgress,
    VOID             *pOwner
    );



#endif
