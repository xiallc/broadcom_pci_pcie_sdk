#ifndef __EEP_9000_H
#define __EEP_9000_H

/*******************************************************************************
 * Copyright 2013-2016 Avago Technologies
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
 *      Eep_9000.h
 *
 * Description:
 *
 *      The include file for 9000-series EEPROM support functions
 *
 * Revision History:
 *
 *      12-01-16 : PLX SDK v7.25
 *
 ******************************************************************************/


#include "DrvDefs.h"


#ifdef __cplusplus
extern "C" {
#endif




/**********************************************
 *               Definitions
 *********************************************/
// PLX 9000-series EEPROM definitions
#define PLX9000_EE_CMD_LEN_46           9       // Bits in instructions
#define PLX9000_EE_CMD_LEN_56           11      // Bits in instructions
#define PLX9000_EE_CMD_LEN_66           11      // Bits in instructions
#define PLX9000_EE_CMD_READ             0x0180  // 01 1000 0000
#define PLX9000_EE_CMD_WRITE            0x0140  // 01 0100 0000
#define PLX9000_EE_CMD_WRITE_ENABLE     0x0130  // 01 0011 0000
#define PLX9000_EE_CMD_WRITE_DISABLE    0x0100  // 01 0000 0000


// EEPROM Control register offset
#if ((PLX_CHIP == 9080) || (PLX_CHIP == 9054) || \
     (PLX_CHIP == 9056) || (PLX_CHIP == 9656) || (PLX_CHIP == 8311))
    #define REG_EEPROM_CTRL     0x6C
#elif ((PLX_CHIP == 9050) || (PLX_CHIP == 9030))
    #define REG_EEPROM_CTRL     0x50
#else
    #error ERROR: 'REG_EEPROM_CTRL' not defined
#endif


// EEPROM type
#if ((PLX_CHIP == 9030) || (PLX_CHIP == 9054) || \
     (PLX_CHIP == 9056) || (PLX_CHIP == 9656) || (PLX_CHIP == 8311))
    #define PLX_9000_EEPROM_TYPE   Eeprom93CS56
#elif ((PLX_CHIP == 9050) || (PLX_CHIP == 9080))
    #define PLX_9000_EEPROM_TYPE   Eeprom93CS46
#else
    #error ERROR: 'PLX_9000_EEPROM_TYPE' not defined
#endif


// EEPROM Types
typedef enum _PLX_EEPROM_TYPE
{
    Eeprom93CS46,
    Eeprom93CS56
} PLX_EEPROM_TYPE;




/**********************************************
 *                Functions
 *********************************************/
PLX_STATUS
Plx9000_EepromPresent(
    DEVICE_EXTENSION *pdx,
    U8               *pStatus
    );

PLX_STATUS
Plx9000_EepromReadByOffset(
    DEVICE_EXTENSION *pdx,
    U32               offset,
    U32              *pValue
    );

PLX_STATUS
Plx9000_EepromWriteByOffset(
    DEVICE_EXTENSION *pdx,
    U32               offset,
    U32               value
    );

VOID
Plx9000_EepromSendCommand(
    DEVICE_EXTENSION *pdx,
    U32               EepromCommand,
    U8                DataLengthInBits
    );

VOID
Plx9000_EepromClock(
    DEVICE_EXTENSION *pdx
    );



#ifdef __cplusplus
}
#endif

#endif
