#ifndef _PLX_EEP_H
#define _PLX_EEP_H

/*******************************************************************************
 * Copyright 2013-2017 Avago Technologies
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
 *      PlxEep.h
 *
 * Description:
 *
 *      Definitions for the the EEPROM application
 *
 ******************************************************************************/


#include "PlxTypes.h"




/*************************************
 *          Definitions
 ************************************/
#define APP_VERSION_MAJOR           2       // Version information
#define APP_VERSION_MINOR           9
#define APP_VERSION_REVISION        0

#define BOOLEAN_UNKNOWN             4
#define MAX_DEVICES_TO_LIST         50

#define EXIT_CODE_SUCCESS           0
#define EXIT_CODE_NON_DOS           1
#define EXIT_CODE_CMD_LINE_ERR      2
#define EXIT_CODE_INVALID_DEV       3
#define EXIT_CODE_DEV_OPEN_ERR      4
#define EXIT_CODE_EEP_FAIL          5
#define EXIT_CODE_EEP_NOT_EXIST     6
#define EXIT_CODE_INVALID_PLX       7
#define EXIT_CODE_EEP_WIDTH_ERR     8


#if defined(PLX_DOS)
    #define PlxPreventInWindows()   (getenv("OS") != NULL)
#else
    #define PlxPreventInWindows()   FALSE
#endif


typedef struct _EEP_OPTIONS
{
    BOOLEAN bVerbose;
    BOOLEAN bLoadFile;
    BOOLEAN bIgnoreWarnings;
    char    FileName[255];
    S8      DeviceNumber;
    U8      EepWidthSet;
    U16     LimitPlxChip;
    U8      LimitPlxRevision;
    U16     ExtraBytes;
} EEP_OPTIONS;




/*************************************
 *            Functions
 ************************************/
S8
ProcessCommandLine(
    int          argc,
    char        *argv[],
    EEP_OPTIONS *pOptions
    );

VOID
DisplayHelp(
    VOID
    );

S16
SelectDevice(
    PLX_DEVICE_KEY *pKey,
    EEP_OPTIONS    *pOptions
    );

S8
EepFile(
    PLX_DEVICE_OBJECT *pDevice,
    EEP_OPTIONS       *pOptions
    );

S8
Plx_EepromFileLoad(
    PLX_DEVICE_OBJECT *pDevice,
    EEP_OPTIONS       *pOptions,
    U32                EepSize,
    U8                 EepPortSize,
    BOOLEAN            bCrc,
    BOOLEAN            bEndianSwap
    );

S8
Plx_EepromFileSave(
    PLX_DEVICE_OBJECT *pDevice,
    EEP_OPTIONS       *pOptions,
    U32                EepSize,
    U8                 EepPortSize,
    BOOLEAN            bCrc,
    BOOLEAN            bEndianSwap
    );

S8
Plx8000_EepromFileLoad(
    PLX_DEVICE_OBJECT *pDevice,
    EEP_OPTIONS       *pOptions,
    BOOLEAN            bCrc
    );

S8
Plx8000_EepromFileSave(
    PLX_DEVICE_OBJECT *pDevice,
    EEP_OPTIONS       *pOptions,
    U32                ByteCount,
    BOOLEAN            bCrc
    );



#endif
