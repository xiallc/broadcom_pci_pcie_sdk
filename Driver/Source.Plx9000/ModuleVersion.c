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
 *      ModuleVersion.c
 *
 * Description:
 *
 *      The source file to contain module version information
 *
 * Revision History:
 *
 *      05-01-15 : PCI SDK v7.30
 *
 ******************************************************************************/


/*********************************************************
 * __NO_VERSION__ should be defined for all source files
 * that include <module.h>.  In order to get kernel
 * version information into the driver, __NO_VERSION__
 * must be undefined before <module.h> is included in
 * one and only one file.  Otherwise, the linker will
 * complain about multiple definitions of module version
 * information.
 ********************************************************/
#if defined(__NO_VERSION__)
    #undef __NO_VERSION__
#endif

#include <linux/module.h>
#include "Plx.h"




/***********************************************
 *            Module Information
 **********************************************/
MODULE_DESCRIPTION("PLX SDK " __stringify(PLX_CHIP) " Linux driver");

#if defined(PLX_DEBUG)
    #define _DRV_CFG      "Debug"
#else
    #define _DRV_CFG      "Release"
#endif

#if defined(PLX_64BIT)
    #define _DRV_BIT      "64"
#else
    #define _DRV_BIT      "32"
#endif

MODULE_VERSION("v" PLX_SDK_VERSION_STRING " [" _DRV_BIT "-bit " _DRV_CFG " build]");


/*********************************************************
 * In later releases of Linux kernel 2.4, the concept of
 * module licensing was introduced.  This is used when
 * the module is loaded to determine if the kernel is
 * tainted due to loading of the module.  Each module
 * declares its license MODULE_LICENSE().  Refer to
 * http://www.tux.org/lkml/#export-tainted for more info.
 *
 * From "module.h", the possible values for license are:
 *
 *   "GPL"                        [GNU Public License v2 or later]
 *   "GPL and additional rights"  [GNU Public License v2 rights and more]
 *   "Dual BSD/GPL"               [GNU Public License v2 or BSD license choice]
 *   "Dual MPL/GPL"               [GNU Public License v2 or Mozilla license choice]
 *   "Proprietary"                [Non free products]
 *
 * Since PLX drivers are provided only to customers who
 * have purchased the PLX SDK, PLX modules are usually marked
 * as "Proprietary"; however, this causes some issues
 * on newer 2.6 kernels, so the license is set to "GPL".
 ********************************************************/

#if defined(MODULE_LICENSE)
    MODULE_LICENSE("GPL");
#endif
