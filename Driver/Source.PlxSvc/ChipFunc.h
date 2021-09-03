#ifndef __CHIP_FUNC_H
#define __CHIP_FUNC_H

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
 *      ChipFunc.h
 *
 * Description:
 *
 *      The header file for the PLX chip functions
 *
 * Revision History:
 *
 *      12-01-07 : PLX SDK v5.20
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



#endif
