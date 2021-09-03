#ifndef __EEP_9000_H
#define __EEP_9000_H

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
 *      Eep_9000.h
 *
 * Description:
 *
 *      The include file for 9000-series EEPROM support functions
 *
 * Revision History:
 *
 *      07-01-12 : PLX SDK v7.00
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
    U16               offset,
    U32              *pValue
    );

PLX_STATUS
Plx9000_EepromWriteByOffset(
    DEVICE_EXTENSION *pdx,
    U16               offset,
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
