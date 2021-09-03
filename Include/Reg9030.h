#ifndef __REG9030_H
#define __REG9030_H

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
 *      Reg9030.h
 *
 * Description:
 *
 *      This file defines all the PLX 9030 chip Registers.
 *
 * Revision:
 *
 *      05-01-03 : PCI SDK v4.10
 *
 ******************************************************************************/


#include "PciRegs.h"


#ifdef __cplusplus
extern "C" {
#endif


// Additional defintions
#define PCI9030_MAX_REG_OFFSET       0x078
#define PCI9030_EEPROM_SIZE          0x88          // EEPROM size (bytes) used by PLX Chip


// PCI Configuration Registers
#define PCI9030_VENDOR_ID            CFG_VENDOR_ID
#define PCI9030_COMMAND              CFG_COMMAND
#define PCI9030_REV_ID               CFG_REV_ID
#define PCI9030_CACHE_SIZE           CFG_CACHE_SIZE
#define PCI9030_PCI_BASE_0           CFG_BAR0
#define PCI9030_PCI_BASE_1           CFG_BAR1
#define PCI9030_PCI_BASE_2           CFG_BAR2
#define PCI9030_PCI_BASE_3           CFG_BAR3
#define PCI9030_PCI_BASE_4           CFG_BAR4
#define PCI9030_PCI_BASE_5           CFG_BAR5
#define PCI9030_CIS_PTR              CFG_CIS_PTR
#define PCI9030_SUB_ID               CFG_SUB_VENDOR_ID
#define PCI9030_PCI_BASE_EXP_ROM     CFG_EXP_ROM_BASE
#define PCI9030_CAP_PTR              CFG_CAP_PTR
#define PCI9030_PCI_RESERVED         CFG_RESERVED1
#define PCI9030_INT_LINE             CFG_INT_LINE
#define PCI9030_PM_CAP_ID            0x040
#define PCI9030_PM_CSR               0x044
#define PCI9030_HS_CAP_ID            0x048
#define PCI9030_VPD_CAP_ID           0x04c
#define PCI9030_VPD_DATA             0x050


// Local Configuration Registers
#define PCI9030_RANGE_SPACE0         0x000
#define PCI9030_RANGE_SPACE1         0x004
#define PCI9030_RANGE_SPACE2         0x008
#define PCI9030_RANGE_SPACE3         0x00c
#define PCI9030_RANGE_EXP_ROM        0x010
#define PCI9030_REMAP_SPACE0         0x014
#define PCI9030_REMAP_SPACE1         0x018
#define PCI9030_REMAP_SPACE2         0x01c
#define PCI9030_REMAP_SPACE3         0x020
#define PCI9030_REMAP_EXP_ROM        0x024
#define PCI9030_DESC_SPACE0          0x028
#define PCI9030_DESC_SPACE1          0x02c
#define PCI9030_DESC_SPACE2          0x030
#define PCI9030_DESC_SPACE3          0x034
#define PCI9030_DESC_EXP_ROM         0x038
#define PCI9030_BASE_CS0             0x03c
#define PCI9030_BASE_CS1             0x040
#define PCI9030_BASE_CS2             0x044
#define PCI9030_BASE_CS3             0x048
#define PCI9030_INT_CTRL_STAT        0x04c
#define PCI9030_EEPROM_CTRL          0x050
#define PCI9030_GP_IO_CTRL           0x054
#define PCI9030_PM_DATA_SELECT       0x070
#define PCI9030_PM_DATA_SCALE        0x074




#ifdef __cplusplus
}
#endif

#endif
