#ifndef __SPI_FLASH_H
#define __SPI_FLASH_H

/*******************************************************************************
 * Copyright 2013-2020 Broadcom Inc
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
 *     SpiFlash.h
 *
 * Description:
 *
 *     SPI flash access functions
 *
 * Revision:
 *
 *     03-01-20 : PCI/PCIe SDK v8.10
 *
 ******************************************************************************/


#include "PlxApiDirect.h"


#ifdef __cplusplus
extern "C" {
#endif




/******************************************
 *             Definitions
 ******************************************/

#define SPI_REG_ADDR( ptrSpi, offset )      ((ptrSpi)->CtrlBaseAddr + (offset))
#define SPI_MAX_CMD_LEN                     (1 + 4) // 1B cmd + 4B addr

// Max time to wait until controller is ready
#define SPI_MAX_WAIT_CTRL_READY_MS          (1000)

// PBAM SPI registers
#define PEX_REG_SPI_MANUAL_IO_MODE          0x7C
#define PEX_REG_SPI_MANUAL_RD_DATA          0x78
#define PEX_REG_SPI_MANUAL_WR_DATA          0x80
#define PEX_REG_SPI_MANUAL_CTRL_STAT        0x84

// SPI flash command codes
#define SPI_FLASH_CMD_READ_ID_DEPRECATED    0x90
#define SPI_FLASH_CMD_READ_ID_CFI           0x9F
#define SPI_FLASH_CMD_ERASE_ALL             0x60
#define SPI_FLASH_CMD_3B_ERASE_SECTOR       0xD8
#define SPI_FLASH_CMD_4B_ERASE_SECTOR       0xDC
#define SPI_FLASH_CMD_3B_READ               0x03
#define SPI_FLASH_CMD_3B_READ_DUAL_IO       0xBB
#define SPI_FLASH_CMD_3B_READ_QUAD_IO       0xEB
#define SPI_FLASH_CMD_4B_READ               0x13
#define SPI_FLASH_CMD_3B_WRITE              0x02
#define SPI_FLASH_CMD_3B_WRITE_QUAD_IO      0x32
#define SPI_FLASH_CMD_4B_WRITE              0x12
#define SPI_FLASH_CMD_WRITE_ENABLE          0x06
#define SPI_FLASH_CMD_WRITE_DISABLE         0x04
#define SPI_FLASH_CMD_RD_STATUS_REG_1       0x05

// SPI command additional flags
#define SPI_CMD_FLAGS_NONE                  0
#define SPI_CMD_FLAGS_OP_MORE_CMDS          (1 << 0)  // More commands coming for operation
#define SPI_CMD_FLAGS_OP_MORE_DATA          (1 << 1)  // More data coming for operation

// SPI Manual control reg fields
#define SPI_MAN_CTRL_LEN_SHIFT              0
#define SPI_MAN_CTRL_CS_SHIFT               7
#define SPI_MAN_CTRL_LAST_MASK              ((U32)1 << 11)
#define SPI_MAN_CTRL_WRITE_OP_MASK          ((U32)1 << 12)
#define SPI_MAN_CTRL_ATOMIC_OP_MASK         ((U32)1 << 14)
#define SPI_MAN_CTRL_VALID_MASK             ((U32)1 << 16)

// Status register 1
#define SPI_FLASH_REG_SR1_WRITE_IN_PROG     (1 << 0)
#define SPI_FLASH_REG_SR1_WRITE_EN_LATCH    (1 << 1)
#define SPI_FLASH_REG_SR1_BLOCK_PROT_0      (1 << 2)
#define SPI_FLASH_REG_SR1_BLOCK_PROT_1      (1 << 3)
#define SPI_FLASH_REG_SR1_BLOCK_PROT_2      (1 << 4)
#define SPI_FLASH_REG_SR1_ERASE_ERR         (1 << 5)
#define SPI_FLASH_REG_SR1_PROG_ERR          (1 << 6)
#define SPI_FLASH_REG_SR1_SR_WRITE_DIS      (1 << 7)




/******************************************
 *     SPI Flash Functions
 *****************************************/
PLX_STATUS
Spi_FlashPropGet(
    PLX_DEVICE_OBJECT *PtrDev,
    U8                 ChipSel,
    PEX_SPI_OBJ       *PtrSpi
    );

PLX_STATUS
Spi_Erase(
    PLX_DEVICE_OBJECT *PtrDev,
    PEX_SPI_OBJ       *PtrSpi,
    U32                StartOffset,
    U8                 BoolWaitComplete
    );

PLX_STATUS
Spi_ReadBuffer(
    PLX_DEVICE_OBJECT *PtrDev,
    PEX_SPI_OBJ       *PtrSpi,
    U32                StartOffset,
    U8               *PtrRxBuff,
    U32                SizeRx
    );

PLX_STATUS
Spi_WriteBuffer(
    PLX_DEVICE_OBJECT *PtrDev,
    PEX_SPI_OBJ       *PtrSpi,
    U32                StartOffset,
    U8                *PtrTxBuff,
    U32                SizeTx
    );

PLX_STATUS
Spi_GetStatus(
    PLX_DEVICE_OBJECT *PtrDev,
    PEX_SPI_OBJ       *PtrSpi
    );


/******************************************
 *      Support Functions
 *****************************************/
PLX_STATUS
Spi_CmdSendAndReply(
    PLX_DEVICE_OBJECT *PtrDev,
    PEX_SPI_OBJ       *PtrSpi,
    U8                 Flags,
    U8                *PtrDataTx,
    U32                SizeTx,
    U8                *PtrDataRx,
    U32                SizeRx
    );

PLX_STATUS
Spi_WaitControllerReady(
    PLX_DEVICE_OBJECT *PtrDev,
    PEX_SPI_OBJ       *PtrSpi
    );



#ifdef __cplusplus
}
#endif

#endif
