#ifndef __SUPPORT_FN_H
#define __SUPPORT_FN_H

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
 *      SuppFunc.h
 *
 * Description:
 *
 *      Header for additional support functions
 *
 * Revision History:
 *
 *      07-01-12 : PLX SDK v7.00
 *
 ******************************************************************************/


#include "DrvDefs.h"




/**********************************************
 *               Functions
 *********************************************/
VOID
Plx_sleep(
    U32 delay
    );

BOOLEAN
PlxSynchronizedRegisterModify(
    PLX_REG_DATA *pRegData
    );

VOID
PlxSignalNotifications(
    DEVICE_EXTENSION   *pdx,
    PLX_INTERRUPT_DATA *pIntData
    );

U16
PlxGetExtendedCapabilityOffset(
    DEVICE_EXTENSION *pdx,
    U16               CapabilityId
    );

int
PlxPciBarResourceMap(
    DEVICE_EXTENSION *pdx,
    U8                BarIndex
    );

VOID
PlxPciBarResourcesUnmap(
    DEVICE_EXTENSION *pdx
    );

VOID
PlxPciPhysicalMemoryFreeAll_ByOwner(
    DEVICE_EXTENSION *pdx,
    VOID             *pOwner
    );

VOID*
Plx_dma_buffer_alloc(
    DEVICE_EXTENSION    *pdx,
    PLX_PHYS_MEM_OBJECT *pMemObject
    );

VOID
Plx_dma_buffer_free(
    DEVICE_EXTENSION    *pdx,
    PLX_PHYS_MEM_OBJECT *pMemObject
    );

VOID
PlxDmaChannelCleanup(
    DEVICE_EXTENSION *pdx,
    VOID             *pOwner
    );

VOID
PlxSglDmaTransferComplete(
    DEVICE_EXTENSION *pdx,
    U8                channel
    );

PLX_STATUS
PlxLockBufferAndBuildSgl(
    DEVICE_EXTENSION *pdx,
    U8                channel,
    PLX_DMA_PARAMS   *pDma,
    U32              *pSglAddress,
    BOOLEAN          *pbBits64
    );

void
Plx_dev_mem_to_user_8(
    U8            *VaUser,
    U8            *VaDev,
    unsigned long  count
    );

void
Plx_dev_mem_to_user_16(
    U16           *VaUser,
    U16           *VaDev,
    unsigned long  count
    );

void
Plx_dev_mem_to_user_32(
    U32           *VaUser,
    U32           *VaDev,
    unsigned long  count
    );

void
Plx_user_to_dev_mem_8(
    U8            *VaDev,
    U8            *VaUser,
    unsigned long  count
    );

void
Plx_user_to_dev_mem_16(
    U16           *VaDev,
    U16           *VaUser,
    unsigned long  count
    );

void
Plx_user_to_dev_mem_32(
    U32           *VaDev,
    U32           *VaUser,
    unsigned long  count
    );



#endif
