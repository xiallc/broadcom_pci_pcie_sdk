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
 *      PlxDefCk.h
 *
 * Description:
 *
 *      Verifies definitions required by the PLX API
 *
 * Revision:
 *
 *      05-01-12 : PLX SDK v7.00
 *
 ******************************************************************************/




/**********************************************
*       Automatic selection for Windows
**********************************************/
#if defined(_WIN32) || defined(_WIN64)
    #if !defined(PLX_LITTLE_ENDIAN) && !defined(PLX_BIG_ENDIAN)
        #define PLX_LITTLE_ENDIAN
    #endif

    #if defined(_WIN64)
        #define PLX_CPU_BITS    64
    #else
        #define PLX_CPU_BITS    32
    #endif

    #define PLX_MSWINDOWS
#endif


#if defined(PLX_LINUX)
    #if !defined(PLX_LITTLE_ENDIAN) && !defined(PLX_BIG_ENDIAN)
        #define PLX_LITTLE_ENDIAN
    #endif
#endif



/**********************************************
*               Error Checks
**********************************************/
#if !defined(PLX_LITTLE_ENDIAN) && !defined(PLX_BIG_ENDIAN)
    #error ERROR: Either PLX_LITTLE_ENDIAN or PLX_BIG_ENDIAN must be defined.
#endif
