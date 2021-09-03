#ifndef _REGISTER_DEFS_H
#define _REGISTER_DEFS_H

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
 *      RegDefs.h
 *
 * Description:
 *
 *      Definitions for the register sets
 *
 ******************************************************************************/


#include "PlxTypes.h"




/*************************************
 *          Definitions
 ************************************/
#define REGS_LCR            1
#define REGS_RTR            2
#define REGS_DMA            3
#define REGS_MQR            4
#define REGS_MCR            5


typedef struct _REGISTER_SET
{
    U16  Offset;
    char Description[80];
} REGISTER_SET;




/*************************************
 *            Globals
 ************************************/
extern REGISTER_SET Pci_Type_0[];
extern REGISTER_SET Pci_Type_1[];
extern REGISTER_SET Pci_Type_2[];

extern REGISTER_SET Lcr9050[];
extern REGISTER_SET Eep9050[];

extern REGISTER_SET Lcr9030[];
extern REGISTER_SET Eep9030[];

extern REGISTER_SET Lcr9080[];
extern REGISTER_SET Rtr9080[];
extern REGISTER_SET Dma9080[];
extern REGISTER_SET Mqr9080[];
extern REGISTER_SET Eep9080[];

extern REGISTER_SET Pci9054[];
extern REGISTER_SET Lcr9054[];
extern REGISTER_SET Dma9054[];
extern REGISTER_SET Eep9054[];

extern REGISTER_SET Lcr9656[];
extern REGISTER_SET Eep9656[];

extern REGISTER_SET Pci6540[];
extern REGISTER_SET Eep6540[];

extern REGISTER_SET Eep6254[];

extern REGISTER_SET Pci8111[];
extern REGISTER_SET Lcr8111[];
extern REGISTER_SET Eep8111[];

extern REGISTER_SET Pci8500[];
extern REGISTER_SET Lcr8500[];
extern REGISTER_SET Eep8500[];

extern REGISTER_SET LcrGeneric[];




/*************************************
 *            Functions
 ************************************/
char*
RegSet_DescrGetByIndex(
    REGISTER_SET *pSet,
    U8            index
    );

char*
RegSet_DescrGetByOffset(
    REGISTER_SET *pSet,
    U32           offset
    );



#endif
