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
 *      ModuleVersion.c
 *
 * Description:
 *
 *      The source file to contain module version information
 *
 * Revision History:
 *
 *      03-01-06 : PCI SDK v4.40
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




/***********************************************
 *            Module Information
 **********************************************/
MODULE_DESCRIPTION("PLX Linux PCI Express Service Driver");

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
