#ifndef _MONITOR_H
#define _MONITOR_H

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
 *      Monitor.h
 *
 * Description:
 *
 *      Definitions for the the monitor application
 *
 ******************************************************************************/


#include "PciDev.h"
#include "PlxTypes.h"
#include "CmdLine.h"

#if defined(PLX_MSWINDOWS)
    #include "..\\Shared\\ConsFunc.h"
#endif

#if defined(PLX_LINUX) || defined(PLX_DOS)
    #include "ConsFunc.h"
#endif




/*************************************
 *          Definitions
 ************************************/
#define MONITOR_VERSION_MAJOR       3       // Version information
#define MONITOR_VERSION_MINOR       0
#define MONITOR_VERSION_REVISION    0

#define MONITOR_PROMPT              ">"     // The monitor prompt string


#if defined(PLX_DOS)
    #define PlxPreventInWindows()   (getenv("OS") != NULL)
#else
    #define PlxPreventInWindows()   FALSE
#endif




/*************************************
 *            Globals
 ************************************/
extern PLX_LIST_ENTRY Gbl_ListCmds;
extern U64            Gbl_LastRetVal;




/*************************************
 *            Functions
 ************************************/
S8
ProcessMonitorParams(
    int   argc,
    char *argv[]
    );

VOID
Monitor(
    VOID
    );

PLXCM_COMMAND*
ProcessCommand(
    PLX_DEVICE_OBJECT *pDevice,
    char              *buffer
    );

DEVICE_NODE*
DeviceSelectByIndex(
    U16 index
    );

VOID
PciSpacesMap(
    PLX_DEVICE_OBJECT *pDevice
    );

VOID
PciSpacesUnmap(
    PLX_DEVICE_OBJECT *pDevice
    );

VOID
CommonBufferMap(
    PLX_DEVICE_OBJECT *pDevice
    );

VOID
CommonBufferUnmap(
    PLX_DEVICE_OBJECT *pDevice
    );

VOID
Mon_PostCommand(
    PLXCM_COMMAND *pCmd
    );


#endif
