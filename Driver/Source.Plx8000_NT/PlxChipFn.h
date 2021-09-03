#ifndef __PLX_CHIP_FN_H
#define __PLX_CHIP_FN_H

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
 *      PlxChipFn.h
 *
 * Description:
 *
 *      Header for PLX chip-specific support functions
 *
 * Revision History:
 *
 *      03-01-10 : PLX SDK v6.40
 *
 ******************************************************************************/


#include "DrvDefs.h"




/**********************************************
 *               Functions
 *********************************************/
BOOLEAN
PlxChipSetInterruptRegisterOffsets(
    DEVICE_EXTENSION *pdx
    );

BOOLEAN
PlxChipInterruptsEnable(
    DEVICE_EXTENSION *pdx
    );

BOOLEAN
PlxChipInterruptsDisable(
    DEVICE_EXTENSION *pdx
    );

PLX_STATUS
PlxChipTypeDetect(
    DEVICE_EXTENSION *pdx
    );

BOOLEAN
PlxDetermineNtPortSide(
    DEVICE_EXTENSION *pdx
    );

BOOLEAN
PlxMapRegisterBar(
    DEVICE_EXTENSION *pdx
    );

VOID
PlxErrataWorkAround_NtBarShadow(
    DEVICE_EXTENSION *pdx
    );

VOID
PlxErrataWorkAround_NtCapturedRequesterID(
    DEVICE_EXTENSION *pdx
    );



#endif
