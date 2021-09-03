#ifndef __REG9656_H
#define __REG9656_H

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
 *      Reg9656.h
 *
 * Description:
 *
 *      This file defines all the PLX 9656 chip Registers.
 *
 * Revision:
 *
 *      11-01-06 : PLX SDK v5.00
 *
 ******************************************************************************/


#include "PciRegs.h"


#ifdef __cplusplus
extern "C" {
#endif


#if defined(PLX_LOCAL_CODE)
    #define PCI9656_REG_BASE         0x080
    #define PCI9656_REG_BASE_1       0x0a0
    #define PCI9656_NEW_CAP_BASE     0x140
#else
    #define PCI9656_REG_BASE         0x000
    #define PCI9656_REG_BASE_1       0x000
    #define PCI9656_NEW_CAP_BASE     0x000
#endif


// Additional defintions
#define PCI9656_MAX_REG_OFFSET       (PCI9656_REG_BASE_1 + 0x108)
#define PCI9656_EEPROM_SIZE          0x064          // EEPROM size (bytes) used by PLX Chip
#define PCI9656_DMA_CHANNELS         2              // Number of DMA channels supported by PLX Chip


// PCI Configuration Registers
#define PCI9656_VENDOR_ID            CFG_VENDOR_ID
#define PCI9656_COMMAND              CFG_COMMAND
#define PCI9656_REV_ID               CFG_REV_ID
#define PCI9656_CACHE_SIZE           CFG_CACHE_SIZE
#define PCI9656_RTR_BASE             CFG_BAR0
#define PCI9656_RTR_IO_BASE          CFG_BAR1
#define PCI9656_LOCAL_BASE0          CFG_BAR2
#define PCI9656_LOCAL_BASE1          CFG_BAR3
#define PCI9656_UNUSED_BASE1         CFG_BAR4
#define PCI9656_UNUSED_BASE2         CFG_BAR5
#define PCI9656_CIS_PTR              CFG_CIS_PTR
#define PCI9656_SUB_ID               CFG_SUB_VENDOR_ID
#define PCI9656_EXP_ROM_BASE         CFG_EXP_ROM_BASE
#define PCI9656_CAP_PTR              CFG_CAP_PTR
#define PCI9656_RESERVED2            CFG_RESERVED1
#define PCI9656_INT_LINE             CFG_INT_LINE
#define PCI9656_PM_CAP_ID            (PCI9656_NEW_CAP_BASE + 0x040)
#define PCI9656_PM_CSR               (PCI9656_NEW_CAP_BASE + 0x044)
#define PCI9656_HS_CAP_ID            (PCI9656_NEW_CAP_BASE + 0x048)
#define PCI9656_VPD_CAP_ID           (PCI9656_NEW_CAP_BASE + 0x04c)
#define PCI9656_VPD_DATA             (PCI9656_NEW_CAP_BASE + 0x050)


// Local Configuration Registers
#define PCI9656_SPACE0_RANGE         (PCI9656_REG_BASE   + 0x000)
#define PCI9656_SPACE0_REMAP         (PCI9656_REG_BASE   + 0x004)
#define PCI9656_LOCAL_DMA_ARBIT      (PCI9656_REG_BASE   + 0x008)
#define PCI9656_ENDIAN_DESC          (PCI9656_REG_BASE   + 0x00c)
#define PCI9656_EXP_ROM_RANGE        (PCI9656_REG_BASE   + 0x010)
#define PCI9656_EXP_ROM_REMAP        (PCI9656_REG_BASE   + 0x014)
#define PCI9656_SPACE0_ROM_DESC      (PCI9656_REG_BASE   + 0x018)
#define PCI9656_DM_RANGE             (PCI9656_REG_BASE   + 0x01c)
#define PCI9656_DM_MEM_BASE          (PCI9656_REG_BASE   + 0x020)
#define PCI9656_DM_IO_BASE           (PCI9656_REG_BASE   + 0x024)
#define PCI9656_DM_PCI_MEM_REMAP     (PCI9656_REG_BASE   + 0x028)
#define PCI9656_DM_PCI_IO_CONFIG     (PCI9656_REG_BASE   + 0x02c)
#define PCI9656_SPACE1_RANGE         (PCI9656_REG_BASE   + 0x0f0)
#define PCI9656_SPACE1_REMAP         (PCI9656_REG_BASE   + 0x0f4)
#define PCI9656_SPACE1_DESC          (PCI9656_REG_BASE   + 0x0f8)
#define PCI9656_DM_DAC               (PCI9656_REG_BASE   + 0x0fc)
#define PCI9656_ARBITER_CTRL         (PCI9656_REG_BASE_1 + 0x100)
#define PCI9656_ABORT_ADDRESS        (PCI9656_REG_BASE_1 + 0x104)


// Runtime Registers
#if defined(PLX_LOCAL_CODE)
    #define PCI9656_MAILBOX0         0x0c0
    #define PCI9656_MAILBOX1         0x0c4
#else
    #define PCI9656_MAILBOX0         0x078
    #define PCI9656_MAILBOX1         0x07c
#endif

#define PCI9656_MAILBOX2             (PCI9656_REG_BASE + 0x048)
#define PCI9656_MAILBOX3             (PCI9656_REG_BASE + 0x04c)
#define PCI9656_MAILBOX4             (PCI9656_REG_BASE + 0x050)
#define PCI9656_MAILBOX5             (PCI9656_REG_BASE + 0x054)
#define PCI9656_MAILBOX6             (PCI9656_REG_BASE + 0x058)
#define PCI9656_MAILBOX7             (PCI9656_REG_BASE + 0x05c)
#define PCI9656_LOCAL_DOORBELL       (PCI9656_REG_BASE + 0x060)
#define PCI9656_PCI_DOORBELL         (PCI9656_REG_BASE + 0x064)
#define PCI9656_INT_CTRL_STAT        (PCI9656_REG_BASE + 0x068)
#define PCI9656_EEPROM_CTRL_STAT     (PCI9656_REG_BASE + 0x06c)
#define PCI9656_PERM_VENDOR_ID       (PCI9656_REG_BASE + 0x070)
#define PCI9656_REVISION_ID          (PCI9656_REG_BASE + 0x074)


// DMA Registers
#define PCI9656_DMA0_MODE            (PCI9656_REG_BASE + 0x080)
#define PCI9656_DMA0_PCI_ADDR        (PCI9656_REG_BASE + 0x084)
#define PCI9656_DMA0_LOCAL_ADDR      (PCI9656_REG_BASE + 0x088)
#define PCI9656_DMA0_COUNT           (PCI9656_REG_BASE + 0x08c)
#define PCI9656_DMA0_DESC_PTR        (PCI9656_REG_BASE + 0x090)
#define PCI9656_DMA1_MODE            (PCI9656_REG_BASE + 0x094)
#define PCI9656_DMA1_PCI_ADDR        (PCI9656_REG_BASE + 0x098)
#define PCI9656_DMA1_LOCAL_ADDR      (PCI9656_REG_BASE + 0x09c)
#define PCI9656_DMA1_COUNT           (PCI9656_REG_BASE + 0x0a0)
#define PCI9656_DMA1_DESC_PTR        (PCI9656_REG_BASE + 0x0a4)
#define PCI9656_DMA_COMMAND_STAT     (PCI9656_REG_BASE + 0x0a8)
#define PCI9656_DMA_ARBIT            (PCI9656_REG_BASE + 0x0aC)
#define PCI9656_DMA_THRESHOLD        (PCI9656_REG_BASE + 0x0b0)
#define PCI9656_DMA0_PCI_DAC         (PCI9656_REG_BASE + 0x0b4)
#define PCI9656_DMA1_PCI_DAC         (PCI9656_REG_BASE + 0x0b8)


// Messaging Unit Registers
#define PCI9656_OUTPOST_INT_STAT     (PCI9656_REG_BASE + 0x030)
#define PCI9656_OUTPOST_INT_MASK     (PCI9656_REG_BASE + 0x034)
#define PCI9656_MU_CONFIG            (PCI9656_REG_BASE + 0x0c0)
#define PCI9656_FIFO_BASE_ADDR       (PCI9656_REG_BASE + 0x0c4)
#define PCI9656_INFREE_HEAD_PTR      (PCI9656_REG_BASE + 0x0c8)
#define PCI9656_INFREE_TAIL_PTR      (PCI9656_REG_BASE + 0x0cc)
#define PCI9656_INPOST_HEAD_PTR      (PCI9656_REG_BASE + 0x0d0)
#define PCI9656_INPOST_TAIL_PTR      (PCI9656_REG_BASE + 0x0d4)
#define PCI9656_OUTFREE_HEAD_PTR     (PCI9656_REG_BASE + 0x0d8)
#define PCI9656_OUTFREE_TAIL_PTR     (PCI9656_REG_BASE + 0x0dc)
#define PCI9656_OUTPOST_HEAD_PTR     (PCI9656_REG_BASE + 0x0e0)
#define PCI9656_OUTPOST_TAIL_PTR     (PCI9656_REG_BASE + 0x0e4)
#define PCI9656_FIFO_CTRL_STAT       (PCI9656_REG_BASE + 0x0e8)




#ifdef __cplusplus
}
#endif

#endif
