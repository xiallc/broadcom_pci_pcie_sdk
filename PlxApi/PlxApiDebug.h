#ifndef __PLX_API_DEBUG_H
#define __PLX_API_DEBUG_H

/*******************************************************************************
 * Copyright 2013-2018 Avago Technologies
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
 *     PlxApiDebug.h
 *
 * Description:
 *
 *     PLX API debug support functions header
 *
 * Revision:
 *
 *     03-01-18 : PLX SDK v8.00
 *
 ******************************************************************************/


#include <stdio.h>


#ifdef __cplusplus
extern "C" {
#endif



/******************************************
 *             Definitions
 *****************************************/
// Ensure debug is enabled for debug builds
#if defined(DEBUG) || defined(_DEBUG)
    #define PLX_DEBUG
#endif

// Remove any existing definitions of debug macros
#if defined(DebugPrintf)
    #undef DebugPrintf
    #undef DebugPrintf_Cont
    #undef InfoPrintf
    #undef InfoPrintf_Cont
    #undef ErrorPrintf
    #undef ErrorPrintf_Cont
    #undef _PlxDbgFunc
    #undef _PlxDbgOut
    #undef _Debug_Print_Macro
#endif


// Set log data destination
#if defined(WIN32)
    #define PLX_DBG_DEST_DEBUGGER           // Default Win DLL to debugger
#elif defined(PLX_LINUX)
    #define PLX_DBG_DEST_FILE               // Default Linux API to file
#elif defined(PLX_DOS)
    #define PLX_DBG_DEST_FILE               // Default DOS API to file
#else
    #define PLX_DBG_DEST_CONSOLE            // Default to console
#endif


// Debug definitions
#if defined(PLX_DBG_DEST_FILE)
    #define _PlxDbgFunc                     PlxApi_DebugPrintf
    #define PLX_LOG_FILE                    "\\PlxApi.Log"        // Log file for debug output
#elif defined(PLX_DBG_DEST_DEBUGGER)
    #if defined(WIN32)
        #define _PlxDbgFunc                 PlxApi_DebugPrintf
        #define _PlxDbgOut                  OutputDebugStringA
    #endif
#elif defined(PLX_DBG_DEST_CONSOLE)
    #define _PlxDbgFunc                     printf
    #define _PlxDbgOut                      puts
#endif

#if defined(PLX_DEBUG)
    #define DebugPrintf(arg)                _Debug_Print_Macro(arg)
    #define DebugPrintf_Cont(arg)           _PlxDbgFunc arg
#else
    #define DebugPrintf(arg)                do { } while(0)
    #define DebugPrintf_Cont(arg)           do { } while(0)
#endif
#define InfoPrintf(arg)                     _Debug_Print_Macro(arg)
#define InfoPrintf_Cont(arg)                _PlxDbgFunc arg
#define ErrorPrintf(arg)                    _Debug_Print_Macro(arg)
#define ErrorPrintf_Cont(arg)               _PlxDbgFunc arg

#define _Debug_Print_Macro(arg)  \
    do                           \
    {                            \
        _PlxDbgFunc("PlxApi: "); \
        _PlxDbgFunc arg;         \
    }                            \
    while (0)



/******************************************
 *             Functions
 *****************************************/
extern void
PlxApi_DebugPrintf(
    const char *format,
    ...
    );



#ifdef __cplusplus
}
#endif

#endif
