#ifndef __PLX_INTERRUPT_H
#define __PLX_INTERRUPT_H

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
 *      PlxInterrupt.h
 *
 * Description:
 *
 *      Driver interrupt functions
 *
 * Revision History:
 *
 *      11-01-11 : PLX SDK v7.00
 *
 ******************************************************************************/


#include <linux/interrupt.h>
#include "Plx_sysdep.h"




/**********************************************
*               Definitions
**********************************************/
#define INTR_TYPE_NONE                  0              // Interrupt identifiers
#define INTR_TYPE_RSTIN                 (1 << 0)
#define INTR_TYPE_PME                   (1 << 1)
#define INTR_TYPE_GPIO_4_5              (1 << 2)
#define INTR_TYPE_GPIO_14_15            (1 << 3)
#define INTR_TYPE_MESSAGE_0             (1 << 4)
#define INTR_TYPE_MESSAGE_1             (1 << 5)
#define INTR_TYPE_MESSAGE_2             (1 << 6)
#define INTR_TYPE_MESSAGE_3             (1 << 7)




/**********************************************
*               Functions
**********************************************/
// In kernel 2.6.19, the "struct pt_regs *" parameter was removed
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
