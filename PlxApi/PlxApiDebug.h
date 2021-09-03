#ifndef __PLX_API_DEBUG_H
#define __PLX_API_DEBUG_H

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
 *     PlxApiDebug.h
 *
 * Description:
 *
 *     PLX API debug support functions header
 *
 * Revision:
 *
 *     09-01-10 : PLX SDK v6.40
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
