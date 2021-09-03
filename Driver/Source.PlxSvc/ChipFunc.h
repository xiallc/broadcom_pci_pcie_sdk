#ifndef __CHIP_FUNC_H
#define __CHIP_FUNC_H

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
 *      ChipFunc.h
 *
 * Description:
 *
 *      The header file for the PLX chip functions
 *
 * Revision History:
 *
 *      12-01-16 : PLX SDK v7.25
 *
 ******************************************************************************/


#include "DrvDefs.h"




/**********************************************
*               Functions
**********************************************/
U32
PlxRegisterRead_8111(
    PLX_DEVICE_NODE *pNode,
    U32              offset,
    PLX_STATUS      *pStatus
    );

PLX_STATUS
PlxRegisterWrite_8111(
    PLX_DEVICE_NODE *pNode,
    U32              offset,
    U32              value
    );

U32
PlxRegisterRead_8000(
    PLX_DEVICE_NODE *pNode,
    U32              offset,
    PLX_STATUS      *pStatus,
    BOOLEAN          bAdjustForPort
    );

PLX_STATUS
PlxRegisterWrite_8000(
    PLX_DEVICE_NODE *pNode,
    U32              offset,
    U32              value,
    BOOLEAN          bAdjustForPort
    );

PLX_STATUS
PlxChipTypeDetect(
    PLX_DEVICE_NODE *pdx,
    BOOLEAN          bOnlySetFamily
    );

VOID
PlxChipRevisionDetect(
    PLX_DEVICE_NODE *pdx
    );



#endif
