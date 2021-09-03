#ifndef __EEP_8111_H
#define __EEP_8111_H

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
