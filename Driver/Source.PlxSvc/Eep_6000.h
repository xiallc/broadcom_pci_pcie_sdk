#ifndef __EEP_6000_H
#define __EEP_6000_H

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
 *      Eep_6000.h
 *
 * Description:
 *
 *      The include file for 6000-series EEPROM support functions
 *
 * Revision History:
 *
 *      08-01-11 : PLX SDK v6.50
 *
 ******************************************************************************/


#include "DrvDefs.h"


#ifdef __cplusplus
extern "C" {
#endif




/**********************************************
*               Functions
**********************************************/
PLX_STATUS
Plx6000_EepromReadByOffset_16(
    PLX_DEVICE_NODE *pNode,
    U32              offset,
    U16             *pValue
    );

PLX_STATUS
Plx6000_EepromWriteByOffset_16(
    PLX_DEVICE_NODE *pNode,
    U32              offset,
    U16              value
    );



#ifdef __cplusplus
}
#endif

#endif
