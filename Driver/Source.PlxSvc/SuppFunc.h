#ifndef __SUPPORT_FN_H
#define __SUPPORT_FN_H

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
 *      SuppFunc.h
 *
 * Description:
 *
 *      Header for additional support functions
 *
 * Revision History:
 *
 *      01-01-18 : PLX SDK v8.00
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

U16
PlxPciFindCapability(
    PLX_DEVICE_NODE *pDevice,
    U16              CapID,
    U8               bPCIeCap,
    U8               InstanceNum
    );

int
PlxPciBarResourceMap(
    PLX_DEVICE_NODE *pdx,
    U8               BarIndex
    );

int
PlxPciBarResourcesUnmap(
    PLX_DEVICE_NODE *pdx,
    U8               BarIndex
    );

int
PlxResourcesReleaseAll(
    DEVICE_EXTENSION *pdx
    );

VOID
PlxUserMappingRequestFreeAll_ByNode(
    DEVICE_EXTENSION *pdx,
    PLX_DEVICE_NODE  *pDevice
    );

U16
PlxDeviceListBuild(
    DRIVER_OBJECT *pDriverObject
    );

PLX_DEVICE_NODE *
PlxAssignParentDevice(
    DEVICE_EXTENSION *pdx,
    PLX_DEVICE_NODE  *pDevice
    );

VOID
PlxProbePciBarSpaces(
    PLX_DEVICE_NODE *pdx
    );

BOOLEAN
PlxDeterminePlxPortType(
    PLX_DEVICE_NODE *pdx
    );

BOOLEAN
PlxSetupRegisterAccess(
    PLX_DEVICE_NODE *pdx
    );

PLX_DEVICE_NODE *
GetDeviceNodeFromKey(
    DEVICE_EXTENSION *pdx,
    PLX_DEVICE_KEY   *pKey
    );



#endif
