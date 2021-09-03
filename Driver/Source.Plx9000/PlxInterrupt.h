#ifndef __PLX_INTERRUPT_H
#define __PLX_INTERRUPT_H

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
 *      PlxInterrupt.h
 *
 * Description:
 *
 *      Driver interrupt functions
 *
 * Revision History:
 *
 *      12-01-16 : PLX SDK v7.25
 *
 ******************************************************************************/


#include <linux/interrupt.h>
#include "Plx_sysdep.h"




/**********************************************
 *               Definitions
 *********************************************/
#define INTR_TYPE_NONE                  0              // Interrupt identifiers
#define INTR_TYPE_LOCAL_1               (1 << 0)
#define INTR_TYPE_LOCAL_2               (1 << 1)
#define INTR_TYPE_PCI_ABORT             (1 << 2)
#define INTR_TYPE_DOORBELL              (1 << 3)
#define INTR_TYPE_OUTBOUND_POST         (1 << 4)
#define INTR_TYPE_DMA_0                 (1 << 5)
#define INTR_TYPE_DMA_1                 (1 << 6)
#define INTR_TYPE_SOFTWARE              (1 << 7)




/**********************************************
 *               Functions
 *
 * Note: 2.6.19 removed parameter "struct pt_regs *"
 *********************************************/
irqreturn_t
OnInterrupt(
    int   irq,
    void *dev_id
  #if (LINUX_VERSION_CODE < KERNEL_VERSION(2,6,19))
  , struct pt_regs *regs
  #endif
    );

VOID
DpcForIsr(
    PLX_DPC_PARAM *pArg1
    );



#endif
