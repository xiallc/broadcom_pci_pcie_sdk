#ifndef __PLX_CHIP_FN_H
#define __PLX_CHIP_FN_H

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
 *      PlxChipFn.h
 *
 * Description:
 *
 *      Header for PLX chip-specific support functions
 *
 * Revision History:
 *
 *      12-01-16 : PLX SDK v7.25
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

VOID
PlxDetermineNtMode(
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
