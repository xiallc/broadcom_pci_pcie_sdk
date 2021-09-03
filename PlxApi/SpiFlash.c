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

/*******************************************************************************
 *
 * File Name:
 *
 *      SpiFlash.c
 *
 * Description:
 *
 *      PLX API functions at API level for direct connect interfaces
 *
 * Revision History:
 *
 *      03-01-20 : PCI/PCIe SDK v8.10
 *
 ******************************************************************************/


#include <string.h>     // For memset()/memcpy()
#include <sys/timeb.h>  // For ftime()
#include "SpiFlash.h"
#include "PlxApiDebug.h"




/*******************************************************************************
 *
 * Function   : Spi_FlashPropGet
 *
 * Description: Queries the SPI flash to get its properties
 *
 ******************************************************************************/
PLX_STATUS
Spi_FlashPropGet(
    PLX_DEVICE_OBJECT *PtrDev,
    U8                 ChipSel,
    PEX_SPI_OBJ       *PtrSpi
    )
{
    PLX_STATUS status;


    if (ChipSel != 0)
    {
        return PLX_STATUS_INVALID_ACCESS;
    }

    PtrSpi->ChipSel = ChipSel;
    PtrSpi->IoMode  = PEX_SPI_IO_MODE_SERIAL;
    PtrSpi->Flags   = PEX_SPI_FLAG_DUAL_IO_SUPP |
                      PEX_SPI_FLAG_QUAD_IO_SUPP;

    // Set address to SPI controller regs
    if (PtrDev->Key.ApiMode == PLX_API_MODE_PCI)
    {
        PtrSpi->CtrlBaseAddr = ATLAS_REGS_PCI_PBAM_BASE_ADDR;
    }
    else
    {
        PtrSpi->CtrlBaseAddr = ATLAS_REGS_AXI_PBAM_BASE_ADDR;
    }

    // Properties of chip select 0 device
    if (ChipSel == 0)
    {
        // Set mem-mapped addresses
        if (PtrDev->Key.ApiMode == PLX_API_MODE_PCI)
        {
            PtrSpi->MmapAddr = ATLAS_SPI_CS0_PCI_BASE_ADDR;
        }
        else
        {
            PtrSpi->MmapAddr = ATLAS_SPI_CS0_AXI_BASE_ADDR;
        }

        PtrSpi->PageSize     = 8;   // 256B page size in power of 2
        PtrSpi->SectorsCount = 8;   // 256 sector count in power of 2
        PtrSpi->SectorSize   = 18;  // 256KB sector size in power of 2
    }

    // Set to manual I/O mode
    status =
        PlxDir_PlxMappedRegWrite(
            PtrDev,
            SPI_REG_ADDR( PtrSpi, PEX_REG_SPI_MANUAL_IO_MODE ),
            PtrSpi->IoMode
            );
    if (status != PLX_STATUS_OK)
    {
        ErrorPrintf((
            "SPI: ERROR: Manual I/O mode reg write failed (O:%02Xh V:%04Xh Stat:%Xh)\n",
            PEX_REG_SPI_MANUAL_IO_MODE, PtrSpi->IoMode, status
            ));
        return status;
    }

    // Mark object as valid
    ObjectValidate( PtrSpi );

    return PLX_STATUS_OK;
}




/*******************************************************************************
 *
 * Function   : Spi_Erase
 *
 * Description: Erase the flash, either all or sectors
 *
 ******************************************************************************/
PLX_STATUS
Spi_Erase(
    PLX_DEVICE_OBJECT *PtrDev,
    PEX_SPI_OBJ       *PtrSpi,
    U32                StartOffset,
    U8                 BoolWaitComplete
    )
{
    U8         cmd[SPI_MAX_CMD_LEN];
    U8         cmdLen;
    PLX_STATUS status;


    // Send WRITE ENABLE
    cmd[0] = SPI_FLASH_CMD_WRITE_ENABLE;
    status =
        Spi_CmdSendAndReply(
            PtrDev,
            PtrSpi,
            SPI_CMD_FLAGS_OP_MORE_CMDS, // ERASE command to follow
            cmd,
            sizeof(U8),
            NULL,       // No reply
            0
            );
    if (status != PLX_STATUS_OK)
    {
        return status;
    }

    // Check for bullk erase
    if (StartOffset == SPI_FLASH_ERASE_ALL)
    {
        // Set command to bulk erase
        cmd[0] = SPI_FLASH_CMD_ERASE_ALL;
        cmdLen = sizeof(U8);
    }
    else
    {
        if (StartOffset & 0xFF000000)
        {
            cmd[0] = SPI_FLASH_CMD_4B_ERASE_SECTOR;

            // 4B addressing not currently supported
            return PLX_STATUS_UNSUPPORTED;
        }
        else
        {
            cmd[0] = SPI_FLASH_CMD_3B_ERASE_SECTOR;

            // Add address & specify length
            cmd[1] = (U8)(StartOffset >> 16);
            cmd[2] = (U8)(StartOffset >>  8);
            cmd[3] = (U8)(StartOffset >>  0);
            cmdLen = sizeof(U8) + (3 * sizeof(U8));
        }
    }

    // Send ERASE command + address, if sector
    status =
        Spi_CmdSendAndReply(
            PtrDev,
            PtrSpi,
            SPI_CMD_FLAGS_NONE,
            cmd,
            cmdLen,
            NULL,       // No reply
            0
            );
    if (status != PLX_STATUS_OK)
    {
        return status;
    }

    // If requested, poll status until completion
    if (BoolWaitComplete)
    {
        while (Spi_GetStatus( PtrDev, PtrSpi ) == PLX_STATUS_IN_PROGRESS)
        {
            Plx_sleep( 200 );
        }
    }

    return PLX_STATUS_OK;
}




/*******************************************************************************
 *
 * Function   : Spi_ReadBuffer
 *
 * Description: Reads data from the SPI flash at specified offset
 *
 ******************************************************************************/
PLX_STATUS
Spi_ReadBuffer(
    PLX_DEVICE_OBJECT *PtrDev,
    PEX_SPI_OBJ       *PtrSpi,
    U32                StartOffset,
    U8                *PtrRxBuff,
    U32                SizeRx
    )
{
    U8         cmdLen;
    U8         cmd[SPI_MAX_CMD_LEN];
    U32        addr;
    U32        offset;
    U32        rxBytes;
    U32        pageBytes;
    PLX_STATUS status;


    // Make sure offset is 32b aligned
    if (StartOffset & 0x3)
    {
        return PLX_STATUS_INVALID_OFFSET;
    }

    offset = 0;

    // Get page size
    pageBytes = (1 << PtrSpi->PageSize);

    while (SizeRx)
    {
        // Set next base address
        addr = StartOffset + offset;

        // Use mem-mapped access read if enabled
        if ( (PtrSpi->Flags & PEX_SPI_FLAG_USE_MM_RD) &&
             (PtrSpi->MmapAddr != 0xFFFFFFF) &&
             (PtrSpi->MmapAddr != 0) )
        {
            // Read 32-bits at a time
            *(U32*)&PtrRxBuff[offset] =
                PlxDir_PlxMappedRegRead( PtrDev, PtrSpi->MmapAddr + addr, &status );
            if (status != PLX_STATUS_OK)
            {
                ErrorPrintf((
                    "SPI: ERROR: Mem-mapped read failed (A:%08Xh Stat:%Xh)\n",
                    PtrSpi->MmapAddr + addr, status
                    ));
                return status;
            }
            rxBytes = sizeof(U32);
        }
        else
        {
            // Prepare READ command
            if (addr & 0xFF000000)
            {
                cmd[0] = SPI_FLASH_CMD_4B_READ;
                // 4B addressing not currently supported
                return PLX_STATUS_UNSUPPORTED;
            }
            else
            {
                // Set command based on mode
                if (PtrSpi->IoMode == PEX_SPI_IO_MODE_QUAD_IO)
                {
                    cmd[0] = SPI_FLASH_CMD_3B_READ_QUAD_IO;
                }
                else if (PtrSpi->IoMode == PEX_SPI_IO_MODE_DUAL_IO)
                {
                    cmd[0] = SPI_FLASH_CMD_3B_READ_DUAL_IO;
                }
                else
                {
                    cmd[0] = SPI_FLASH_CMD_3B_READ;
                }

                // Add address & specify length
                cmd[1] = (U8)(addr >> 16);
                cmd[2] = (U8)(addr >>  8);
                cmd[3] = (U8)(addr >>  0);
                cmdLen = sizeof(U8) + (3 * sizeof(U8));
            }

            // Set max transfer size, ensuring not to cross page boundary
            rxBytes = pageBytes - (addr & (pageBytes - 1));

            // Determine size of next transfer
            rxBytes = PEX_MIN( SizeRx, rxBytes );

            // Send READ command + address
            status =
                Spi_CmdSendAndReply(
                    PtrDev,
                    PtrSpi,
                    SPI_CMD_FLAGS_NONE,
                    cmd,
                    cmdLen,
                    &PtrRxBuff[offset],
                    rxBytes
                    );
            if (status != PLX_STATUS_OK)
            {
                return status;
            }
        }

        // Adjust for next transfer
        SizeRx -= rxBytes;
        offset += rxBytes;
    }

    return PLX_STATUS_OK;
}




/*******************************************************************************
 *
 * Function   : Spi_WriteBuffer
 *
 * Description: Writes data to the SPI flash at specified offset
 *
 ******************************************************************************/
PLX_STATUS
Spi_WriteBuffer(
    PLX_DEVICE_OBJECT *PtrDev,
    PEX_SPI_OBJ       *PtrSpi,
    U32                StartOffset,
    U8                *PtrTxBuff,
    U32                SizeTx
    )
{
    U8         cmdLen;
    U8         cmd[SPI_MAX_CMD_LEN];
    U32        addr;
    U32        offset;
    U32        txBytes;
    U32        pageBytes;
    PLX_STATUS status;


    // Make sure offset is 32b aligned
    if (StartOffset & 0x3)
    {
        return PLX_STATUS_INVALID_OFFSET;
    }

    offset = 0;

    // Get page size
    pageBytes = (1 << PtrSpi->PageSize);

    while (SizeTx)
    {
        // Send WRITE ENABLE command
        cmd[0] = SPI_FLASH_CMD_WRITE_ENABLE;
        status =
            Spi_CmdSendAndReply(
                PtrDev,
                PtrSpi,
                SPI_CMD_FLAGS_OP_MORE_CMDS, // WRITE command to follow
                cmd,
                sizeof(U8),
                NULL,       // No reply
                0
                );
        if (status != PLX_STATUS_OK)
        {
            return status;
        }

        // Set next base address
        addr = StartOffset + offset;

        // Prepare WRITE command
        if (addr & 0xFF000000)
        {
            // 4B addressing not currently supported
            return PLX_STATUS_UNSUPPORTED;
        }
        else
        {
            // Set command based on mode
            if (PtrSpi->IoMode == PEX_SPI_IO_MODE_QUAD_IO)
            {
                cmd[0] = SPI_FLASH_CMD_3B_WRITE_QUAD_IO;
            }
            else
            {
                cmd[0] = SPI_FLASH_CMD_3B_WRITE;
            }

            // Add address & specify length
            cmd[1] = (U8)(addr >> 16);
            cmd[2] = (U8)(addr >>  8);
            cmd[3] = (U8)(addr >>  0);
            cmdLen = sizeof(U8) + (3 * sizeof(U8));
        }

        // Send WRITE command + address
        status =
            Spi_CmdSendAndReply(
                PtrDev,
                PtrSpi,
                SPI_CMD_FLAGS_OP_MORE_DATA,
                cmd,
                cmdLen,
                NULL,       // No reply
                0
                );
        if (status != PLX_STATUS_OK)
        {
            return status;
        }

        // Set max transfer size, ensuring not to cross page boundary
        txBytes = pageBytes - (addr & (pageBytes - 1));

        // Determine size of next transfer
        txBytes = PEX_MIN( SizeTx, txBytes );

        // Send data
        status =
            Spi_CmdSendAndReply(
                PtrDev,
                PtrSpi,
                SPI_CMD_FLAGS_NONE,
                &PtrTxBuff[offset],
                txBytes,
                NULL,       // No reply
                0
                );
        if (status != PLX_STATUS_OK)
        {
            return status;
        }

        // Poll status register to ensure completion
        do
        {
            status = Spi_GetStatus( PtrDev, PtrSpi );
            if ( (status != PLX_STATUS_OK) &&
                 (status != PLX_STATUS_IN_PROGRESS) )
            {
                return status;
            }
        }
        while (status == PLX_STATUS_IN_PROGRESS);

        // Adjust for next transfer
        SizeTx -= txBytes;
        offset += txBytes;
    }

    return PLX_STATUS_OK;
}




/*******************************************************************************
 *
 * Function   : Spi_GetStatus
 *
 * Description: Queries SPI and return current whether operation is in-progress
 *
 ******************************************************************************/
PLX_STATUS
Spi_GetStatus(
    PLX_DEVICE_OBJECT *PtrDev,
    PEX_SPI_OBJ       *PtrSpi
    )
{
    U8         cmd;
    U8         regVal;
    PLX_STATUS status;


    // Get status register 1
    cmd = SPI_FLASH_CMD_RD_STATUS_REG_1;
    status =
        Spi_CmdSendAndReply(
            PtrDev,
            PtrSpi,
            SPI_CMD_FLAGS_NONE,
            &cmd,
            sizeof(U8),
            &regVal,
            sizeof(U8)
            );
    if (status != PLX_STATUS_OK)
    {
        return status;
    }

    // Set status
    if (regVal & SPI_FLASH_REG_SR1_WRITE_IN_PROG)
    {
        status = PLX_STATUS_IN_PROGRESS;
    }
    else
    {
        status = PLX_STATUS_OK;
    }

    return status;
}




/*******************************************************************************
 *
 * Function   : Spi_CmdSendAndReply
 *
 * Description: Sends data to SPI and retrieves read data after, if requested
 *
 ******************************************************************************/
PLX_STATUS
Spi_CmdSendAndReply(
    PLX_DEVICE_OBJECT *PtrDev,
    PEX_SPI_OBJ       *PtrSpi,
    U8                 Flags,
    U8                *PtrDataTx,
    U32                SizeTx,
    U8                *PtrDataRx,
    U32                SizeRx
    )
{
    U32        idx;
    U32        regVal;
    U32        byteCount;
    PLX_STATUS status;


    idx = 0;

    while (SizeTx)
    {
        // Next transfer size
        byteCount = PEX_MIN( SizeTx, sizeof(U32) );

        // Adjust bytes remaining
        SizeTx -= byteCount;

        // Prepare register value
        regVal = 0;
        memcpy( &regVal, &PtrDataTx[idx], byteCount );

        // For 32-bit value, must endian swap
        if (byteCount == sizeof(U32))
        {
            regVal = EndianSwap32( regVal );
        }

        // Write value in data register
        status =
            PlxDir_PlxMappedRegWrite(
                PtrDev,
                SPI_REG_ADDR( PtrSpi, PEX_REG_SPI_MANUAL_WR_DATA ),
                regVal
                );
        if (status != PLX_STATUS_OK)
        {
            ErrorPrintf((
                "SPI: ERROR: Data value reg write failed (O:%02Xh V:%08Xh Stat:%Xh)\n",
                PEX_REG_SPI_MANUAL_WR_DATA, regVal, status
                ));
            return status;
        }

        // Prepare SPI command
        regVal =
            PLX_BYTE_TO_BIT_COUNT( byteCount ) << SPI_MAN_CTRL_LEN_SHIFT |
            PtrSpi->ChipSel << SPI_MAN_CTRL_CS_SHIFT |
            SPI_MAN_CTRL_VALID_MASK |
              SPI_MAN_CTRL_WRITE_OP_MASK;

        // If additional commands coming, put controller into atomic mode
        if (Flags & SPI_CMD_FLAGS_OP_MORE_CMDS)
        {
            regVal |= SPI_MAN_CTRL_ATOMIC_OP_MASK;
        }

        // Set as last opertion if last write data & no reply requested
        if ( ((Flags & SPI_CMD_FLAGS_OP_MORE_DATA) == 0) &&
             (SizeTx == 0) &&
             ((PtrDataRx == NULL) || (SizeRx == 0)) )
        {
            regVal |= SPI_MAN_CTRL_LAST_MASK;
        }

        // Send data
        status =
            PlxDir_PlxMappedRegWrite(
                PtrDev,
                SPI_REG_ADDR( PtrSpi, PEX_REG_SPI_MANUAL_CTRL_STAT ),
                regVal
                );
        if (status != PLX_STATUS_OK)
        {
            ErrorPrintf((
                "SPI: ERROR: Control/stat reg write failed (O:%02Xh V:%08Xh Stat:%Xh)\n",
                PEX_REG_SPI_MANUAL_CTRL_STAT, regVal, status
                ));
            return status;
        }

        // Wait for SPI controller completion
        status = Spi_WaitControllerReady( PtrDev, PtrSpi );
        if (status != PLX_STATUS_OK)
        {
            return status;
        }

        // Prepare for next transfer
        idx += byteCount;
    }

    // Halt if no reply requested
    if ( (PtrDataRx == NULL) || (SizeRx == 0) )
    {
        return PLX_STATUS_OK;
    }

    // Read requested reply
    idx = 0;

    while (SizeRx)
    {
        // Next transfer size
        byteCount = PEX_MIN( SizeRx, sizeof(U32) );

        // Adjust bytes remaining
        SizeRx -= byteCount;

        // Prepare SPI command
        regVal =
            PLX_BYTE_TO_BIT_COUNT( byteCount ) << SPI_MAN_CTRL_LEN_SHIFT |
            PtrSpi->ChipSel << SPI_MAN_CTRL_CS_SHIFT |
            SPI_MAN_CTRL_VALID_MASK;

        // Set as last opertion if last read data
        if (SizeRx == 0)
        {
            regVal |= SPI_MAN_CTRL_LAST_MASK;
        }

        // Issue read to populate data register
        status =
            PlxDir_PlxMappedRegWrite(
                PtrDev,
                SPI_REG_ADDR( PtrSpi, PEX_REG_SPI_MANUAL_CTRL_STAT ),
                regVal
                );
        if (status != PLX_STATUS_OK)
        {
            ErrorPrintf((
                "SPI: ERROR: Control/stat reg write failed (O:%02Xh V:%08Xh Stat:%Xh)\n",
                PEX_REG_SPI_MANUAL_CTRL_STAT, regVal, status
                ));
            return status;
        }

        // Wait for SPI controller completion
        status = Spi_WaitControllerReady( PtrDev, PtrSpi );
        if (status != PLX_STATUS_OK)
        {
            return status;
        }

        // Read data
        regVal =
            PlxDir_PlxMappedRegRead(
                PtrDev,
                SPI_REG_ADDR( PtrSpi, PEX_REG_SPI_MANUAL_RD_DATA ),
                &status
                );
        if (status != PLX_STATUS_OK)
        {
            ErrorPrintf((
                "SPI: ERROR: Data reg read failed (O:%02Xh Stat:%Xh)\n",
                PEX_REG_SPI_MANUAL_RD_DATA, status
                ));
            return status;
        }

        // If 32-bit. must endian swap
        if (byteCount == sizeof(U32))
        {
            regVal = EndianSwap32( regVal );
        }

        // Copy to reply buffer
        memcpy( &PtrDataRx[idx], &regVal, byteCount );

        // Prepare for next transfer
        idx += byteCount;
    }

    return PLX_STATUS_OK;
}





/*******************************************************************************
 *
 * Function   : Spi_WaitControllerReady
 *
 * Description: Polls SPI controller until it's ready for next command
 *
 ******************************************************************************/
PLX_STATUS
Spi_WaitControllerReady(
    PLX_DEVICE_OBJECT *PtrDev,
    PEX_SPI_OBJ       *PtrSpi
    )
{
    U32          regVal;
    U32          elapsedTimeMs;
    PLX_STATUS   status;
    struct timeb endTime;
    struct timeb startTime;


    // Note start time
    ftime( &startTime );

    // Wait until command valid is clear
    do
    {
        regVal =
            PlxDir_PlxMappedRegRead(
                PtrDev,
                SPI_REG_ADDR( PtrSpi, PEX_REG_SPI_MANUAL_CTRL_STAT ),
                &status
                );
        if (status != PLX_STATUS_OK)
        {
            ErrorPrintf((
                "SPI: ERROR: Control/stat reg read failed (O:%02Xh Stat:%Xh)\n",
                PEX_REG_SPI_MANUAL_CTRL_STAT, status
                ));
            return status;
        }

        // Verify we don't exceed poll time
        ftime( &endTime );
        elapsedTimeMs = (U32)(PLX_DIFF_TIMEB( endTime, startTime ) * 1000);
        if (elapsedTimeMs >= SPI_MAX_WAIT_CTRL_READY_MS)
        {
            ErrorPrintf((
                "SPI: ERROR: Timeout (%dms) waiting for controller ready\n",
                SPI_MAX_WAIT_CTRL_READY_MS
                ));
            return PLX_STATUS_TIMEOUT;
        }
    }
    while (regVal & SPI_MAN_CTRL_VALID_MASK);

    return PLX_STATUS_OK;
}
