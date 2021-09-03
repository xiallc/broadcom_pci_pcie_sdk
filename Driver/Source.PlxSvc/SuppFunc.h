#ifndef __SUPPORT_FUNC_H
#define __SUPPORT_FUNC_H

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
 *      03-01-09 : PLX SDK v6.10
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

U32
Plx_pow_int(
    U32 x,
    U32 y
    );

U16
PlxGetExtendedCapabilityOffset(
    PLX_DEVICE_NODE *pNode,
    U16              CapabilityId
    );

int
PlxPciBarResourceMap(
    PLX_DEVICE_NODE *pNode,
    U8               BarIndex
    );

int
PlxPciBarResourcesUnmap(
    PLX_DEVICE_NODE *pNode,
    U8               BarIndex
    );

int
PlxResourcesReleaseAll(
    DEVICE_EXTENSION *pdx
    );

VOID
PlxUserMappingRequestFreeAll_ByNode(
    DEVICE_EXTENSION *pdx,
    PLX_DEVICE_NODE  *pNode
    );

U8
PlxDeviceListBuild(
    DRIVER_OBJECT *pDriverObject
    );

PLX_DEVICE_NODE *
PlxAssignParentDevice(
    DEVICE_EXTENSION *pdx,
    PLX_DEVICE_NODE  *pNode
    );

BOOLEAN
PlxDetermineNtPortSide(
    PLX_DEVICE_NODE *pNode
    );

PLX_DEVICE_NODE *
GetDeviceNodeFromKey(
    DEVICE_EXTENSION *pdx,
    PLX_DEVICE_KEY   *pKey
    );

BOOLEAN
PlxChipTypeDetect(
    PLX_DEVICE_NODE *pNode
    );

VOID
PlxChipRevisionDetect(
    PLX_DEVICE_NODE *pNode
    );



#endif
