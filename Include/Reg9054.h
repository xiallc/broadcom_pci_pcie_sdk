#ifndef __REG9054_H
#define __REG9054_H

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
 *      Reg9054.h
 *
 * Description:
 *
 *      This file defines all the PLX 9054 chip Registers.
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
    #define PCI9054_REG_BASE         0x080
    #define PCI9054_NEW_CAP_BASE     0x140
    #define PCI9054_MAX_REG_OFFSET   0x194
#else
    #define PCI9054_REG_BASE         0x000
    #define PCI9054_NEW_CAP_BASE     0x000
    #define PCI9054_MAX_REG_OFFSET   0x100
#endif


// Additional defintions
#define PCI9054_EEPROM_SIZE          0x058          // EEPROM size (bytes) used by PLX Chip
#define PCI9054_DMA_CHANNELS         2              // Number of DMA channels supported by PLX Chip


// PCI Configuration Registers
#define PCI9054_VENDOR_ID            CFG_VENDOR_ID
#define PCI9054_COMMAND              CFG_COMMAND
#define PCI9054_REV_ID               CFG_REV_ID
#define PCI9054_CACHE_SIZE           CFG_CACHE_SIZE
#define PCI9054_RTR_BASE             CFG_BAR0
#define PCI9054_RTR_IO_BASE          CFG_BAR1
#define PCI9054_LOCAL_BASE0          CFG_BAR2
#define PCI9054_LOCAL_BASE1          CFG_BAR3
#define PCI9054_UNUSED_BASE1         CFG_BAR4
#define PCI9054_UNUSED_BASE2         CFG_BAR5
#define PCI9054_CIS_PTR              CFG_CIS_PTR
#define PCI9054_SUB_ID               CFG_SUB_VENDOR_ID
#define PCI9054_EXP_ROM_BASE         CFG_EXP_ROM_BASE
#define PCI9054_CAP_PTR              CFG_CAP_PTR
#define PCI9054_RESERVED2            CFG_RESERVED1
#define PCI9054_INT_LINE             CFG_INT_LINE
#define PCI9054_PM_CAP_ID            (PCI9054_NEW_CAP_BASE + 0x040)
#define PCI9054_PM_CSR               (PCI9054_NEW_CAP_BASE + 0x044)
#define PCI9054_HS_CAP_ID            (PCI9054_NEW_CAP_BASE + 0x048)
#define PCI9054_VPD_CAP_ID           (PCI9054_NEW_CAP_BASE + 0x04c)
#define PCI9054_VPD_DATA             (PCI9054_NEW_CAP_BASE + 0x050)


// Local Configuration Registers
#define PCI9054_SPACE0_RANGE         (PCI9054_REG_BASE + 0x000)
#define PCI9054_SPACE0_REMAP         (PCI9054_REG_BASE + 0x004)
#define PCI9054_LOCAL_DMA_ARBIT      (PCI9054_REG_BASE + 0x008)
#define PCI9054_ENDIAN_DESC          (PCI9054_REG_BASE + 0x00c)
#define PCI9054_EXP_ROM_RANGE        (PCI9054_REG_BASE + 0x010)
#define PCI9054_EXP_ROM_REMAP        (PCI9054_REG_BASE + 0x014)
#define PCI9054_SPACE0_ROM_DESC      (PCI9054_REG_BASE + 0x018)
#define PCI9054_DM_RANGE             (PCI9054_REG_BASE + 0x01c)
#define PCI9054_DM_MEM_BASE          (PCI9054_REG_BASE + 0x020)
#define PCI9054_DM_IO_BASE           (PCI9054_REG_BASE + 0x024)
#define PCI9054_DM_PCI_MEM_REMAP     (PCI9054_REG_BASE + 0x028)
#define PCI9054_DM_PCI_IO_CONFIG     (PCI9054_REG_BASE + 0x02c)
#define PCI9054_SPACE1_RANGE         (PCI9054_REG_BASE + 0x0f0)
#define PCI9054_SPACE1_REMAP         (PCI9054_REG_BASE + 0x0f4)
#define PCI9054_SPACE1_DESC          (PCI9054_REG_BASE + 0x0f8)
#define PCI9054_DM_DAC               (PCI9054_REG_BASE + 0x0fc)


// Runtime Registers
#if defined(PLX_LOCAL_CODE)
    #define PCI9054_MAILBOX0         0x0c0
    #define PCI9054_MAILBOX1         0x0c4
#else
    #define PCI9054_MAILBOX0         0x078
    #define PCI9054_MAILBOX1         0x07c
#endif

#define PCI9054_MAILBOX2             (PCI9054_REG_BASE + 0x048)
#define PCI9054_MAILBOX3             (PCI9054_REG_BASE + 0x04c)
#define PCI9054_MAILBOX4             (PCI9054_REG_BASE + 0x050)
#define PCI9054_MAILBOX5             (PCI9054_REG_BASE + 0x054)
#define PCI9054_MAILBOX6             (PCI9054_REG_BASE + 0x058)
#define PCI9054_MAILBOX7             (PCI9054_REG_BASE + 0x05c)
#define PCI9054_LOCAL_DOORBELL       (PCI9054_REG_BASE + 0x060)
#define PCI9054_PCI_DOORBELL         (PCI9054_REG_BASE + 0x064)
#define PCI9054_INT_CTRL_STAT        (PCI9054_REG_BASE + 0x068)
#define PCI9054_EEPROM_CTRL_STAT     (PCI9054_REG_BASE + 0x06c)
#define PCI9054_PERM_VENDOR_ID       (PCI9054_REG_BASE + 0x070)
#define PCI9054_REVISION_ID          (PCI9054_REG_BASE + 0x074)


// DMA Registers
#define PCI9054_DMA0_MODE            (PCI9054_REG_BASE + 0x080)
#define PCI9054_DMA0_PCI_ADDR        (PCI9054_REG_BASE + 0x084)
#define PCI9054_DMA0_LOCAL_ADDR      (PCI9054_REG_BASE + 0x088)
#define PCI9054_DMA0_COUNT           (PCI9054_REG_BASE + 0x08c)
#define PCI9054_DMA0_DESC_PTR        (PCI9054_REG_BASE + 0x090)
#define PCI9054_DMA1_MODE            (PCI9054_REG_BASE + 0x094)
#define PCI9054_DMA1_PCI_ADDR        (PCI9054_REG_BASE + 0x098)
#define PCI9054_DMA1_LOCAL_ADDR      (PCI9054_REG_BASE + 0x09c)
#define PCI9054_DMA1_COUNT           (PCI9054_REG_BASE + 0x0a0)
#define PCI9054_DMA1_DESC_PTR        (PCI9054_REG_BASE + 0x0a4)
#define PCI9054_DMA_COMMAND_STAT     (PCI9054_REG_BASE + 0x0a8)
#define PCI9054_DMA_ARBIT            (PCI9054_REG_BASE + 0x0ac)
#define PCI9054_DMA_THRESHOLD        (PCI9054_REG_BASE + 0x0b0)
#define PCI9054_DMA0_PCI_DAC         (PCI9054_REG_BASE + 0x0b4)
#define PCI9054_DMA1_PCI_DAC         (PCI9054_REG_BASE + 0x0b8)


// Messaging Unit Registers
#define PCI9054_OUTPOST_INT_STAT     (PCI9054_REG_BASE + 0x030)
#define PCI9054_OUTPOST_INT_MASK     (PCI9054_REG_BASE + 0x034)
#define PCI9054_MU_CONFIG            (PCI9054_REG_BASE + 0x0c0)
#define PCI9054_FIFO_BASE_ADDR       (PCI9054_REG_BASE + 0x0c4)
#define PCI9054_INFREE_HEAD_PTR      (PCI9054_REG_BASE + 0x0c8)
#define PCI9054_INFREE_TAIL_PTR      (PCI9054_REG_BASE + 0x0cc)
#define PCI9054_INPOST_HEAD_PTR      (PCI9054_REG_BASE + 0x0d0)
#define PCI9054_INPOST_TAIL_PTR      (PCI9054_REG_BASE + 0x0d4)
#define PCI9054_OUTFREE_HEAD_PTR     (PCI9054_REG_BASE + 0x0d8)
#define PCI9054_OUTFREE_TAIL_PTR     (PCI9054_REG_BASE + 0x0dc)
#define PCI9054_OUTPOST_HEAD_PTR     (PCI9054_REG_BASE + 0x0e0)
#define PCI9054_OUTPOST_TAIL_PTR     (PCI9054_REG_BASE + 0x0e4)
#define PCI9054_FIFO_CTRL_STAT       (PCI9054_REG_BASE + 0x0e8)




#ifdef __cplusplus
}
#endif

#endif
