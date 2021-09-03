#ifndef __EEP_8111_H
#define __EEP_8111_H

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
 *      Eep_8111.h
 *
 * Description:
 *
 *      The include file for 8111-series EEPROM support functions
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
*               Definitions
**********************************************/
// PLX 8111-series EEPROM definitions
#define PLX8111_EE_CMD_READ             3
#define PLX8111_EE_CMD_READ_STATUS      5
#define PLX8111_EE_CMD_WRITE_ENABLE     6
#define PLX8111_EE_CMD_WRITE            2
#define PLX8111_EE_CMD_WRITE_STATUS     1




/**********************************************
*               Functions
**********************************************/
PLX_STATUS
Plx8111_EepromPresent(
    PLX_DEVICE_NODE *pdx,
    U8              *pStatus
    );

PLX_STATUS
Plx8111_EepromGetAddressWidth(
    PLX_DEVICE_NODE *pdx,
    U8              *pWidth
    );

PLX_STATUS
Plx8111_EepromSetAddressWidth(
    PLX_DEVICE_NODE *pdx,
    U8               width
    );

PLX_STATUS
Plx8111_EepromReadByOffset_16(
    PLX_DEVICE_NODE *pdx,
    U32              offset,
    U16             *pValue
    );

PLX_STATUS
Plx8111_EepromWriteByOffset_16(
    PLX_DEVICE_NODE *pdx,
    U32              offset,
    U16              value
    );

BOOLEAN
Plx8111_EepromWaitIdle(
    PLX_DEVICE_NODE *pdx
    );

BOOLEAN
Plx8111_EepromWaitUntilReady(
    PLX_DEVICE_NODE *pdx
    );

BOOLEAN
Plx8111_EepromDataRead(
    PLX_DEVICE_NODE *pdx,
    U8              *pData
    );

BOOLEAN
Plx8111_EepromDataWrite(
    PLX_DEVICE_NODE *pdx,
    U8               data
    );



#ifdef __cplusplus
}
#endif

#endif
