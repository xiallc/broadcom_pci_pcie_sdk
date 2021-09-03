#ifndef __DISPATCH_H
#define __DISPATCH_H

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
 *      Dispatch.h
 *
 * Description:
 *
 *      The Driver Dispatch functions
 *
 * Revision History:
 *
 *      10-01-08 : PLX SDK v6.10
 *
 ******************************************************************************/


#include <linux/fs.h>




/**********************************************
 *               Functions
 *********************************************/
int
Dispatch_open(
    struct inode *inode,
    struct file  *filp
    );

int
Dispatch_release(
    struct inode *inode,
    struct file  *filp
    );

int
Dispatch_mmap(
    struct file           *filp,
    struct vm_area_struct *vma
    );

int 
Dispatch_IoControl(
    struct inode  *inode,
    struct file   *filp,
    unsigned int   cmd,
    unsigned long  args
    );




#endif
