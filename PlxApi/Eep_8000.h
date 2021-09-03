#ifndef __EEP_8000_H
#define __EEP_8000_H

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
 *      Eep_8000.h
 *
 * Description:
 *
 *      The include file for 8000-series EEPROM support functions
 *
 * Revision History:
 *
 *      08-01-11 : PLX SDK v6.50
 *
 ******************************************************************************/


#include "PlxTypes.h"


#ifdef __cplusplus
extern "C" {
#endif




/**********************************************
*               Definitions
**********************************************/
#define CONST_CRC_XOR_VALUE             0xDB710641          // Constant used in CRC calculations

// PLX 8000-series EEPROM definitions
#define PLX8000_EE_CMD_READ             3
#define PLX8000_EE_CMD_READ_STATUS      5
#define PLX8000_EE_CMD_WRITE_ENABLE     6
#define PLX8000_EE_CMD_WRITE_DISABLE    4
#define PLX8000_EE_CMD_WRITE            2
#define PLX8000_EE_CMD_WRITE_STATUS     1




/**********************************************
*               Functions
**********************************************/
PLX_STATUS
Plx8000_EepromPresent(
    PLX_DEVICE_OBJECT *pdx,
    U8                *pStatus
    );

PLX_STATUS
Plx8000_EepromGetAddressWidth(
    PLX_DEVICE_OBJECT *pdx,
    U8                *pWidth
    );

PLX_STATUS
Plx8000_EepromSetAddressWidth(
    PLX_DEVICE_OBJECT *pdx,
    U8                 width
    );

PLX_STATUS
Plx8000_EepromCrcGet(
    PLX_DEVICE_OBJECT *pdx,
    U32               *pCrc,
    U8                *pCrcStatus
    );

PLX_STATUS
Plx8000_EepromCrcUpdate(
    PLX_DEVICE_OBJECT *pdx,
    U32               *pCrc,
    BOOLEAN            bUpdateEeprom
    );

PLX_STATUS
Plx8000_EepromReadByOffset(
    PLX_DEVICE_OBJECT *pdx,
    U32                offset,
    U32               *pValue
    );

PLX_STATUS
Plx8000_EepromWriteByOffset(
    PLX_DEVICE_OBJECT *pdx,
    U32                offset,
    U32                value
    );

PLX_STATUS
Plx8000_EepromReadByOffset_16(
    PLX_DEVICE_OBJECT *pdx,
    U32                offset,
    U16               *pValue
    );

PLX_STATUS
Plx8000_EepromWriteByOffset_16(
    PLX_DEVICE_OBJECT *pdx,
    U32                offset,
    U16                value
    );

BOOLEAN
Plx8000_EepromWaitIdle(
    PLX_DEVICE_OBJECT *pdx
    );

BOOLEAN
Plx8000_EepromSendCommand(
    PLX_DEVICE_OBJECT *pdx,
    U32                command
    );

VOID
Plx8000_EepromComputeNextCrc(
    U32 *pCrc,
    U32  NextEepromValue
    );

U16
Plx8000_EepromGetCtrlOffset(
    PLX_DEVICE_OBJECT *pdx
    );



#ifdef __cplusplus
}
#endif

#endif
