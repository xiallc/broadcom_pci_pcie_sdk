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
 *      PlxChipApi.c
 *
 * Description:
 *
 *      Implements chip-specific API functions
 *
 * Revision History:
 *
 *      12-01-16 : PLX SDK v7.25
 *
 ******************************************************************************/


#include "Eep_9000.h"
#include "PciFunc.h"
#include "PlxChipApi.h"
#include "SuppFunc.h"




/******************************************************************************
 *
 * Function   :  PlxChip_BoardReset
 *
 * Description:  Resets a device using software reset feature of PLX chip
 *
 ******************************************************************************/
PLX_STATUS
PlxChip_BoardReset(
    DEVICE_EXTENSION *pdx
    )
{
    U8  MU_Enabled;
    U8  EepromPresent;
    U32 RegValue;
    U32 DelayLoop;
    U32 RegInterrupt;
    U32 RegHotSwap;
    U32 RegPowerMgmnt;


    // Clear any PCI errors (04[31:27])
    PLX_PCI_REG_READ(
        pdx,
        0x04,
        &RegValue
        );

    if (RegValue & (0xf8 << 24))
    {
        // Write value back to clear aborts
        PLX_PCI_REG_WRITE(
            pdx,
            0x04,
            RegValue
            );
    }

    // Save state of I2O Decode Enable
    RegValue =
        PLX_9000_REG_READ(
            pdx,
            PCI8311_FIFO_CTRL_STAT
            );

    MU_Enabled = (U8)(RegValue & (1 << 0));

    // Determine if an EEPROM is present
    RegValue =
        PLX_9000_REG_READ(
            pdx,
            PCI8311_EEPROM_CTRL_STAT
            );

    // Make sure S/W Reset & EEPROM reload bits are clear
    RegValue &= ~((1 << 30) | (1 << 29));

    // Remember if EEPROM is present
    EepromPresent = (U8)((RegValue >> 28) & (1 << 0));

    // Save interrupt line
    PLX_PCI_REG_READ(
        pdx,
        0x3C,
        &RegInterrupt
        );

    // Save some registers if EEPROM present
    if (EepromPresent)
    {
        PLX_PCI_REG_READ(
            pdx,
            PCI8311_HS_CAP_ID,
            &RegHotSwap
            );

        PLX_PCI_REG_READ(
            pdx,
            PCI8311_PM_CSR,
            &RegPowerMgmnt
            );
    }

    // Issue Software Reset to hold PLX chip in reset
    PLX_9000_REG_WRITE(
        pdx,
        PCI8311_EEPROM_CTRL_STAT,
        RegValue | (1 << 30)
        );

    // Delay for a bit using dummy register reads (1 read = ~1us)
    for (DelayLoop = 0; DelayLoop < (100 * 1000); DelayLoop++)
    {
        PLX_9000_REG_READ( pdx, 0 );
    }

    // Bring chip out of reset
    PLX_9000_REG_WRITE(
        pdx,
        PCI8311_EEPROM_CTRL_STAT,
        RegValue
        );

    // Issue EEPROM reload in case now programmed
    PLX_9000_REG_WRITE(
        pdx,
        PCI8311_EEPROM_CTRL_STAT,
        RegValue | (1 << 29)
        );

    // Delay for a bit using dummy register reads (1 read = ~1us)
    for (DelayLoop = 0; DelayLoop < (10 * 1000); DelayLoop++)
    {
        PLX_9000_REG_READ( pdx, 0 );
    }

    // Clear EEPROM reload
    PLX_9000_REG_WRITE(
        pdx,
        PCI8311_EEPROM_CTRL_STAT,
        RegValue
        );

    // Restore I2O Decode Enable state
    if (MU_Enabled)
    {
        // Save state of I2O Decode Enable
        RegValue =
            PLX_9000_REG_READ(
                pdx,
                PCI8311_FIFO_CTRL_STAT
                );

        PLX_9000_REG_WRITE(
            pdx,
            PCI8311_FIFO_CTRL_STAT,
            RegValue | (1 << 0)
            );
    }

    // Restore interrupt line
    PLX_PCI_REG_WRITE(
        pdx,
        0x3C,
        RegInterrupt
        );

    // If EEPROM was present, restore registers
    if (EepromPresent)
    {
        // Mask out HS bits that can be cleared
        RegHotSwap &= ~((1 << 23) | (1 << 22) | (1 << 17));

        PLX_PCI_REG_WRITE(
            pdx,
            PCI8311_HS_CAP_ID,
            RegHotSwap
            );

        // Mask out PM bits that can be cleared
        RegPowerMgmnt &= ~(1 << 15);

        PLX_PCI_REG_READ(
            pdx,
            PCI8311_PM_CSR,
            &RegPowerMgmnt
            );
    }

    return PLX_STATUS_OK;
}




/*******************************************************************************
 *
 * Function   :  PlxChip_MailboxRead
 *
 * Description:  Reads a PLX mailbox register
 *
 ******************************************************************************/
U32
PlxChip_MailboxRead(
    DEVICE_EXTENSION *pdx,
    U16               mailbox,
    PLX_STATUS       *pStatus
    )
{
    U16 offset;


    // Verify valid mailbox
    if (((S16)mailbox < 0) || ((S16)mailbox > 7))
    {
        if (pStatus != NULL)
            *pStatus = PLX_STATUS_INVALID_DATA;
        return 0;
    }

    // Set mailbox register base
    if ((mailbox == 0) || (mailbox == 1))
        offset = 0x78;
    else
        offset = 0x40;

    // Set status code
    if (pStatus != NULL)
        *pStatus = PLX_STATUS_OK;

    // Calculate mailbox offset
    offset = offset + (mailbox * sizeof(U32));

    // Read mailbox
    return PLX_9000_REG_READ(
        pdx,
        offset
        );
}




/*******************************************************************************
 *
 * Function   :  PlxChip_MailboxWrite
 *
 * Description:  Writes to a PLX mailbox register
 *
 ******************************************************************************/
PLX_STATUS
PlxChip_MailboxWrite(
    DEVICE_EXTENSION *pdx,
    U16               mailbox,
    U32               value
    )
{
    U16 offset;


    // Verify valid mailbox
    if (((S16)mailbox < 0) || ((S16)mailbox > 7))
    {
        return PLX_STATUS_INVALID_DATA;
    }

    // Set mailbox register base
    if ((mailbox == 0) || (mailbox == 1))
        offset = 0x78;
    else
        offset = 0x40;

    // Calculate mailbox offset
    offset = offset + (mailbox * sizeof(U32));

    // Write mailbox
    PLX_9000_REG_WRITE(
        pdx,
        offset,
        value
        );

    return PLX_STATUS_OK;
}




/******************************************************************************
 *
 * Function   :  PlxChip_InterruptEnable
 *
 * Description:  Enables specific interupts of the PLX Chip
 *
 ******************************************************************************/
PLX_STATUS
PlxChip_InterruptEnable(
    DEVICE_EXTENSION *pdx,
    PLX_INTERRUPT    *pPlxIntr
    )
{
    U32          QueueCsr;
    U32          QueueCsr_Original;
    U32          RegValue;
    PLX_REG_DATA RegData;


    // Setup to synchronize access to Interrupt Control/Status Register
    RegData.pdx         = pdx;
    RegData.offset      = PCI8311_INT_CTRL_STAT;
    RegData.BitsToSet   = 0;
    RegData.BitsToClear = 0;

    if (pPlxIntr->PciMain)
        RegData.BitsToSet |= (1 << 8);

    if (pPlxIntr->PciAbort)
        RegData.BitsToSet |= (1 << 10);

    if (pPlxIntr->TargetRetryAbort)
        RegData.BitsToSet |= (1 << 12);

    if (pPlxIntr->LocalToPci & (1 << 0))
        RegData.BitsToSet |= (1 << 11);

    if (pPlxIntr->Doorbell)
        RegData.BitsToSet |= (1 << 9);

    if (pPlxIntr->DmaDone & (1 << 0))
    {
        RegData.BitsToSet |= (1 << 18);

        // Make sure DMA done interrupt is enabled & routed to PCI
        RegValue =
            PLX_9000_REG_READ(
                pdx,
                PCI8311_DMA0_MODE
                );

        PLX_9000_REG_WRITE(
            pdx,
            PCI8311_DMA0_MODE,
            RegValue | (1 << 17) | (1 << 10)
            );
    }

    if (pPlxIntr->DmaDone & (1 << 1))
    {
        RegData.BitsToSet |= (1 << 19);

        // Make sure DMA done interrupt is enabled & routed to PCI
        RegValue =
            PLX_9000_REG_READ(
                pdx,
                PCI8311_DMA1_MODE
                );

        PLX_9000_REG_WRITE(
            pdx,
            PCI8311_DMA1_MODE,
            RegValue | (1 << 17) | (1 << 10)
            );
    }

    // Inbound Post Queue Interrupt Control/Status Register
    QueueCsr_Original =
        PLX_9000_REG_READ(
            pdx,
            PCI8311_FIFO_CTRL_STAT
            );

    QueueCsr = QueueCsr_Original;

    if (pPlxIntr->MuOutboundPost)
    {
        PLX_9000_REG_WRITE(
            pdx,
            PCI8311_OUTPOST_INT_MASK,
            0
            );
    }

    if (pPlxIntr->MuInboundPost)
        QueueCsr &= ~(1 << 4);

    if (pPlxIntr->MuOutboundOverflow)
    {
        RegData.BitsToSet |=  (1 << 1);
        QueueCsr          &= ~(1 << 6);
    }

    // Write register values if they have changed
    if (RegData.BitsToSet != 0)
    {
        // Synchronize write of Interrupt Control/Status Register
        PlxSynchronizedRegisterModify(
            &RegData
            );
    }

    if (QueueCsr != QueueCsr_Original)
    {
        PLX_9000_REG_WRITE(
            pdx,
            PCI8311_FIFO_CTRL_STAT,
            QueueCsr
            );
    }

    return PLX_STATUS_OK;
}




/******************************************************************************
 *
 * Function   :  PlxChip_InterruptDisable
 *
 * Description:  Disables specific interrupts of the PLX Chip
 *
 ******************************************************************************/
PLX_STATUS
PlxChip_InterruptDisable(
    DEVICE_EXTENSION *pdx,
    PLX_INTERRUPT    *pPlxIntr
    )
{
    U32          QueueCsr;
    U32          QueueCsr_Original;
    U32          RegValue;
    PLX_REG_DATA RegData;


    // Setup to synchronize access to Interrupt Control/Status Register
    RegData.pdx         = pdx;
    RegData.offset      = PCI8311_INT_CTRL_STAT;
    RegData.BitsToSet   = 0;
    RegData.BitsToClear = 0;

    if (pPlxIntr->PciMain)
        RegData.BitsToClear |= (1 << 8);

    if (pPlxIntr->PciAbort)
        RegData.BitsToClear |= (1 << 10);

    if (pPlxIntr->TargetRetryAbort)
        RegData.BitsToClear |= (1 << 12);

    if (pPlxIntr->LocalToPci & (1 << 0))
        RegData.BitsToClear |= (1 << 11);

    if (pPlxIntr->Doorbell)
        RegData.BitsToClear |= (1 << 9);

    if (pPlxIntr->DmaDone & (1 << 0))
    {
        // Check if DMA interrupt is routed to PCI
        RegValue =
            PLX_9000_REG_READ(
                pdx,
                PCI8311_DMA0_MODE
                );

        if (RegValue & (1 << 17))
        {
            RegData.BitsToClear |= (1 << 18);

            // Disable DMA interrupt enable
            PLX_9000_REG_WRITE(
                pdx,
                PCI8311_DMA0_MODE,
                RegValue & ~(1 << 10)
                );
        }
    }

    if (pPlxIntr->DmaDone & (1 << 1))
    {
        // Check if DMA interrupt is routed to PCI
        RegValue =
            PLX_9000_REG_READ(
                pdx,
                PCI8311_DMA1_MODE
                );

        if (RegValue & (1 << 17))
        {
            RegData.BitsToClear |= (1 << 19);

            // Disable DMA interrupt enable
            PLX_9000_REG_WRITE(
                pdx,
                PCI8311_DMA1_MODE,
                RegValue & ~(1 << 10)
                );
        }
    }

    // Inbound Post Queue Interrupt Control/Status Register
    QueueCsr_Original =
        PLX_9000_REG_READ(
            pdx,
            PCI8311_FIFO_CTRL_STAT
            );

    QueueCsr = QueueCsr_Original;

    if (pPlxIntr->MuOutboundPost)
    {
        PLX_9000_REG_WRITE(
            pdx,
            PCI8311_OUTPOST_INT_MASK,
            (1 << 3)
            );
    }

    if (pPlxIntr->MuInboundPost)
        QueueCsr |= (1 << 4);

    if (pPlxIntr->MuOutboundOverflow)
        QueueCsr |= (1 << 6);

    // Write register values if they have changed
    if (RegData.BitsToClear != 0)
    {
        // Synchronize write of Interrupt Control/Status Register
        PlxSynchronizedRegisterModify(
            &RegData
            );
    }

    if (QueueCsr != QueueCsr_Original)
    {
        PLX_9000_REG_WRITE(
            pdx,
            PCI8311_FIFO_CTRL_STAT,
            QueueCsr
            );
    }

    return PLX_STATUS_OK;
}




/******************************************************************************
 *
 * Function   :  PlxChip_EepromReadByOffset
 *
 * Description:  Read a 32-bit value from the EEPROM at a specified offset
 *
 ******************************************************************************/
PLX_STATUS
PlxChip_EepromReadByOffset(
    DEVICE_EXTENSION *pdx,
    U32               offset,
    U32              *pValue
    )
{
    // Verify the offset
    if ((offset & 0x3) || (offset > 0x200))
    {
        DebugPrintf(("ERROR - Invalid EEPROM offset\n"));
        return PLX_STATUS_INVALID_OFFSET;
    }

    // Read EEPROM
    Plx9000_EepromReadByOffset(
        pdx,
        offset,
        pValue
        );

    return PLX_STATUS_OK;
}




/******************************************************************************
 *
 * Function   :  PlxChip_EepromWriteByOffset
 *
 * Description:  Write a 32-bit value to the EEPROM at a specified offset
 *
 ******************************************************************************/
PLX_STATUS
PlxChip_EepromWriteByOffset(
    DEVICE_EXTENSION *pdx,
    U32               offset,
    U32               value
    )
{
    U32 RegisterSave;


    // Verify the offset
    if ((offset & 0x3) || (offset > 0x200))
    {
        DebugPrintf(("ERROR - Invalid EEPROM offset\n"));
        return PLX_STATUS_INVALID_OFFSET;
    }

    // Unprotect the EEPROM for write access
    RegisterSave =
        PLX_9000_REG_READ(
            pdx,
            PCI8311_ENDIAN_DESC
            );

    PLX_9000_REG_WRITE(
        pdx,
        PCI8311_ENDIAN_DESC,
        RegisterSave & ~(0xFF << 16)
        );

    // Write to EEPROM
    Plx9000_EepromWriteByOffset(
        pdx,
        offset,
        value
        );

    // Restore EEPROM Write-Protected Address Boundary
    PLX_9000_REG_WRITE(
        pdx,
        PCI8311_ENDIAN_DESC,
        RegisterSave
        );

    return PLX_STATUS_OK;
}




/******************************************************************************
 *
 * Function   :  PlxChip_DmaChannelOpen
 *
 * Description:  Open a DMA channel
 *
 ******************************************************************************/
PLX_STATUS
PlxChip_DmaChannelOpen(
    DEVICE_EXTENSION *pdx,
    U8                channel,
    VOID             *pOwner
    )
{
    PLX_REG_DATA RegData;


    // Verify valid DMA channel
    switch (channel)
    {
        case 0:
        case 1:
            break;

        default:
            DebugPrintf(("ERROR - Invalid DMA channel\n"));
            return PLX_STATUS_INVALID_ACCESS;
    }

    spin_lock(
        &(pdx->Lock_Dma[channel])
        );

    // Verify that we can open the channel
    if (pdx->DmaInfo[channel].bOpen)
    {
        DebugPrintf(("ERROR - DMA channel already opened\n"));
        spin_unlock( &(pdx->Lock_Dma[channel]) );
        return PLX_STATUS_INVALID_ACCESS;
    }

    // Open the channel
    pdx->DmaInfo[channel].bOpen = TRUE;

    // Record the Owner
    pdx->DmaInfo[channel].pOwner = pOwner;

    // No SGL DMA is pending
    pdx->DmaInfo[channel].bSglPending = FALSE;

    spin_unlock(
        &(pdx->Lock_Dma[channel])
        );

    // Setup for synchronized access to Interrupt register
    RegData.pdx         = pdx;
    RegData.offset      = PCI8311_INT_CTRL_STAT;
    RegData.BitsToClear = 0;

    // Enable DMA channel interrupt
    if (channel == 0)
        RegData.BitsToSet = (1 << 18);
    else
        RegData.BitsToSet = (1 << 19);

    // Update interrupt register
    PlxSynchronizedRegisterModify(
        &RegData
        );

    DebugPrintf((
        "Opened DMA channel %d\n",
        channel
        ));

    return PLX_STATUS_OK;
}




/******************************************************************************
 *
 * Function   :  PlxChip_DmaGetProperties
 *
 * Description:  Gets the current DMA properties
 *
 ******************************************************************************/
PLX_STATUS
PlxChip_DmaGetProperties(
    DEVICE_EXTENSION *pdx,
    U8                channel,
    PLX_DMA_PROP     *pProp
    )
{
    U16 OffsetMode;
    U32 RegValue;


    // Verify valid DMA channel
    switch (channel)
    {
        case 0:
            OffsetMode = PCI8311_DMA0_MODE;
            break;

        case 1:
            OffsetMode = PCI8311_DMA1_MODE;
            break;

        default:
            DebugPrintf(("ERROR - Invalid DMA channel\n"));
            return PLX_STATUS_INVALID_ACCESS;
    }

    // Get DMA mode
    RegValue =
        PLX_9000_REG_READ(
            pdx,
            OffsetMode
            );

    // Clear properties
    RtlZeroMemory( pProp, sizeof(PLX_DMA_PROP) );

    // Set DMA properties
    pProp->LocalBusWidth     = (U8)(RegValue >>  0) & 0x3;
    pProp->WaitStates        = (U8)(RegValue >>  2) & 0xF;
    pProp->ReadyInput        = (U8)(RegValue >>  6) & 0x1;
    pProp->BurstInfinite     = (U8)(RegValue >>  7) & 0x1;
    pProp->Burst             = (U8)(RegValue >>  8) & 0x1;
    pProp->SglMode           = (U8)(RegValue >>  9) & 0x1;
    pProp->DoneInterrupt     = (U8)(RegValue >> 10) & 0x1;
    pProp->ConstAddrLocal    = (U8)(RegValue >> 11) & 0x1;
    pProp->DemandMode        = (U8)(RegValue >> 12) & 0x1;
    pProp->WriteInvalidMode  = (U8)(RegValue >> 13) & 0x1;
    pProp->EnableEOT         = (U8)(RegValue >> 14) & 0x1;
    pProp->FastTerminateMode = (U8)(RegValue >> 15) & 0x1;
    pProp->ClearCountMode    = (U8)(RegValue >> 16) & 0x1;
    pProp->RouteIntToPci     = (U8)(RegValue >> 17) & 0x1;
    pProp->DualAddressMode   = (U8)(RegValue >> 18) & 0x1;
    pProp->EOTEndLink        = (U8)(RegValue >> 19) & 0x1;
    pProp->ValidMode         = (U8)(RegValue >> 20) & 0x1;
    pProp->ValidStopControl  = (U8)(RegValue >> 21) & 0x1;

    return PLX_STATUS_OK;
}




/******************************************************************************
 *
 * Function   :  PlxChip_DmaSetProperties
 *
 * Description:  Sets the current DMA properties
 *
 ******************************************************************************/
PLX_STATUS
PlxChip_DmaSetProperties(
    DEVICE_EXTENSION *pdx,
    U8                channel,
    PLX_DMA_PROP     *pProp,
    VOID             *pOwner
    )
{
    U16 OffsetMode;
    U32 RegValue;


    // Verify valid DMA channel
    switch (channel)
    {
        case 0:
            OffsetMode = PCI8311_DMA0_MODE;
            break;

        case 1:
            OffsetMode = PCI8311_DMA1_MODE;
            break;

        default:
            DebugPrintf(("ERROR - Invalid DMA channel\n"));
            return PLX_STATUS_INVALID_ACCESS;
    }

    // Verify owner
    if ((pdx->DmaInfo[channel].bOpen) && (pdx->DmaInfo[channel].pOwner != pOwner))
    {
        DebugPrintf(("ERROR - DMA owned by different process\n"));
        return PLX_STATUS_IN_USE;
    }

    // Verify DMA not in progress
    if (PlxChip_DmaStatus(
            pdx,
            channel,
            pOwner
            ) != PLX_STATUS_COMPLETE)
    {
        DebugPrintf(("ERROR - DMA transfer in progress\n"));
        return PLX_STATUS_IN_PROGRESS;
    }

    // Set DMA properties
    RegValue =
        (pProp->LocalBusWidth     <<  0) |
        (pProp->WaitStates        <<  2) |
        (pProp->ReadyInput        <<  6) |
        (pProp->BurstInfinite     <<  7) |
        (pProp->Burst             <<  8) |
        (pProp->SglMode           <<  9) |
        (pProp->DoneInterrupt     << 10) |
        (pProp->ConstAddrLocal    << 11) |
        (pProp->DemandMode        << 12) |
        (pProp->WriteInvalidMode  << 13) |
        (pProp->EnableEOT         << 14) |
        (pProp->FastTerminateMode << 15) |
        (pProp->ClearCountMode    << 16) |
        (pProp->RouteIntToPci     << 17) |
        (pProp->DualAddressMode   << 18) |
        (pProp->EOTEndLink        << 19) |
        (pProp->ValidMode         << 20) |
        (pProp->ValidStopControl  << 21);

    // Update properties
    PLX_9000_REG_WRITE(
        pdx,
        OffsetMode,
        RegValue
        );

    return PLX_STATUS_OK;
}




/******************************************************************************
 *
 * Function   :  PlxChip_DmaControl
 *
 * Description:  Control the DMA engine
 *
 ******************************************************************************/
PLX_STATUS
PlxChip_DmaControl(
    DEVICE_EXTENSION *pdx,
    U8                channel,
    PLX_DMA_COMMAND   command,
    VOID             *pOwner
    )
{
    U8  shift;
    U32 RegValue;


    // Verify valid DMA channel
    switch (channel)
    {
        case 0:
        case 1:
            break;

        default:
            DebugPrintf(("ERROR - Invalid DMA channel\n"));
            return PLX_STATUS_INVALID_ACCESS;
    }

    // Verify owner
    if ((pdx->DmaInfo[channel].bOpen) && (pdx->DmaInfo[channel].pOwner != pOwner))
    {
        DebugPrintf(("ERROR - DMA owned by different process\n"));
        return PLX_STATUS_IN_USE;
    }

    // Set shift for status register
    shift = (channel * 8);

    switch (command)
    {
        case DmaPause:
            // Pause the DMA Channel
            RegValue =
                PLX_9000_REG_READ(
                    pdx,
                    PCI8311_DMA_COMMAND_STAT
                    );

            PLX_9000_REG_WRITE(
                pdx,
                PCI8311_DMA_COMMAND_STAT,
                RegValue & ~((1 << 0) << shift)
                );

            // Check if the transfer has completed
            RegValue =
                PLX_9000_REG_READ(
                    pdx,
                    PCI8311_DMA_COMMAND_STAT
                    );

            if (RegValue & ((1 << 4) << shift))
                return PLX_STATUS_COMPLETE;
            break;

        case DmaResume:
            // Verify that the DMA Channel is paused
            RegValue =
                PLX_9000_REG_READ(
                    pdx,
                    PCI8311_DMA_COMMAND_STAT
                    );

            if ((RegValue & (((1 << 4) | (1 << 0)) << shift)) == 0)
            {
                PLX_9000_REG_WRITE(
                    pdx,
                    PCI8311_DMA_COMMAND_STAT,
                    RegValue | ((1 << 0) << shift)
                    );
            }
            else
            {
                return PLX_STATUS_IN_PROGRESS;
            }
            break;

        case DmaAbort:
            // Pause the DMA Channel
            RegValue =
                PLX_9000_REG_READ(
                    pdx,
                    PCI8311_DMA_COMMAND_STAT
                    );

            PLX_9000_REG_WRITE(
                pdx,
                PCI8311_DMA_COMMAND_STAT,
                RegValue & ~((1 << 0) << shift)
                );

            // Check if the transfer has completed
            RegValue =
                PLX_9000_REG_READ(
                    pdx,
                    PCI8311_DMA_COMMAND_STAT
                    );

            if (RegValue & ((1 << 4) << shift))
                return PLX_STATUS_COMPLETE;

            // Abort the transfer (should cause an interrupt)
            PLX_9000_REG_WRITE(
                pdx,
                PCI8311_DMA_COMMAND_STAT,
                RegValue | ((1 << 2) << shift)
                );
            break;

        default:
            return PLX_STATUS_INVALID_DATA;
    }

    return PLX_STATUS_OK;
}




/******************************************************************************
 *
 * Function   :  PlxChip_DmaStatus
 *
 * Description:  Get status of a DMA channel
 *
 ******************************************************************************/
PLX_STATUS
PlxChip_DmaStatus(
    DEVICE_EXTENSION *pdx,
    U8                channel,
    VOID             *pOwner
    )
{
    U32 RegValue;


    // Verify valid DMA channel
    switch (channel)
    {
        case 0:
        case 1:
            break;

        default:
            DebugPrintf(("ERROR - Invalid DMA channel\n"));
            return PLX_STATUS_INVALID_ACCESS;
    }

    // Check if DMA owned by another caller
    if ((pdx->DmaInfo[channel].bOpen) && (pdx->DmaInfo[channel].pOwner != pOwner))
        return PLX_STATUS_IN_USE;

    // Return the current DMA status
    RegValue =
        PLX_9000_REG_READ(
            pdx,
            PCI8311_DMA_COMMAND_STAT
            );

    // Shift status for channel 1
    if (channel == 1)
        RegValue = RegValue >> 8;

    if ((RegValue & ((1 << 4) | (1 << 0))) == 0)
        return PLX_STATUS_PAUSED;

    if (RegValue & (1 << 4))
        return PLX_STATUS_COMPLETE;

    return PLX_STATUS_IN_PROGRESS;
}




/******************************************************************************
 *
 * Function   :  PlxChip_DmaTransferBlock
 *
 * Description:  Performs DMA block transfer
 *
 ******************************************************************************/
PLX_STATUS
PlxChip_DmaTransferBlock(
    DEVICE_EXTENSION *pdx,
    U8                channel,
    PLX_DMA_PARAMS   *pParams,
    VOID             *pOwner
    )
{
    U8  shift;
    U16 OffsetMode;
    U32 RegValue;


    // Verify DMA channel & setup register offsets
    switch (channel)
    {
        case 0:
            OffsetMode = PCI8311_DMA0_MODE;
            break;

        case 1:
            OffsetMode = PCI8311_DMA1_MODE;
            break;

        default:
            DebugPrintf(("ERROR - Invalid DMA channel\n"));
            return PLX_STATUS_INVALID_ACCESS;
    }

    // Verify owner
    if (pdx->DmaInfo[channel].pOwner != pOwner)
    {
        DebugPrintf(("ERROR - DMA owned by different process\n"));
        return PLX_STATUS_IN_USE;
    }

    // Set shift for status register
    shift = (channel * 8);

    // Verify that DMA is not in progress
    RegValue =
        PLX_9000_REG_READ(
            pdx,
            PCI8311_DMA_COMMAND_STAT
            );

    if ((RegValue & ((1 << 4) << shift)) == 0)
    {
        DebugPrintf(("ERROR - DMA channel is currently active\n"));
        return PLX_STATUS_IN_PROGRESS;
    }

    spin_lock(
        &(pdx->Lock_Dma[channel])
        );

    // Verify DMA channel was opened
    if (pdx->DmaInfo[channel].bOpen == FALSE)
    {
        DebugPrintf(("ERROR - DMA channel has not been opened\n"));
        spin_unlock( &(pdx->Lock_Dma[channel]) );
        return PLX_STATUS_INVALID_ACCESS;
    }

    // Get DMA mode
    RegValue =
        PLX_9000_REG_READ(
            pdx,
            OffsetMode
            );

    // Disable DMA chaining & SGL dual-addressing
    RegValue &= ~((1 << 9) | (1 << 18));

    // Route interrupt to PCI
    RegValue |= (1 << 17);

    // Ignore interrupt if requested
    if (pParams->bIgnoreBlockInt)
        RegValue &= ~(1 << 10);
    else
        RegValue |= (1 << 10);

    PLX_9000_REG_WRITE(
        pdx,
        OffsetMode,
        RegValue
        );

    // Write PCI Address
    PLX_9000_REG_WRITE(
        pdx,
        OffsetMode + 0x4,
        PLX_64_LOW_32(pParams->PciAddr)
        );

    // Write Local Address
    PLX_9000_REG_WRITE(
        pdx,
        OffsetMode + 0x8,
        pParams->LocalAddr
        );

    // Write Transfer Count
    PLX_9000_REG_WRITE(
        pdx,
        OffsetMode + 0xc,
        pParams->ByteCount
        );

    // Write Descriptor Pointer
    if (pParams->Direction == PLX_DMA_LOC_TO_PCI)
        RegValue = (1 << 3);
    else
        RegValue = 0;

    PLX_9000_REG_WRITE(
        pdx,
        OffsetMode + 0x10,
        RegValue
        );

    // Write Dual Address cycle with upper 32-bit PCI address
    PLX_9000_REG_WRITE(
        pdx,
        PCI8311_DMA0_PCI_DAC + (channel * sizeof(U32)),
        PLX_64_HIGH_32(pParams->PciAddr)
        );

    // Enable DMA channel
    RegValue =
        PLX_9000_REG_READ(
            pdx,
            PCI8311_DMA_COMMAND_STAT
            );

    PLX_9000_REG_WRITE(
        pdx,
        PCI8311_DMA_COMMAND_STAT,
        RegValue | ((1 << 0) << shift)
        );

    spin_unlock(
        &(pdx->Lock_Dma[channel])
        );

    DebugPrintf(("Starting DMA transfer...\n"));

    // Start DMA
    PLX_9000_REG_WRITE(
        pdx,
        PCI8311_DMA_COMMAND_STAT,
        RegValue | (((1 << 0) | (1 << 1)) << shift)
        );

    return PLX_STATUS_OK;
}




/******************************************************************************
 *
 * Function   :  PlxChip_DmaTransferUserBuffer
 *
 * Description:  Transfers a user-mode buffer using SGL DMA
 *
 ******************************************************************************/
PLX_STATUS
PlxChip_DmaTransferUserBuffer(
    DEVICE_EXTENSION *pdx,
    U8                channel,
    PLX_DMA_PARAMS   *pParams,
    VOID             *pOwner
    )
{
    U8         shift;
    U16        OffsetMode;
    U32        RegValue;
    U32        SglPciAddress;
    BOOLEAN    bBits64;
    PLX_STATUS rc;


    // Verify DMA channel & setup register offsets
    switch (channel)
    {
        case 0:
            OffsetMode = PCI8311_DMA0_MODE;
            break;

        case 1:
            OffsetMode = PCI8311_DMA1_MODE;
            break;

        default:
            DebugPrintf(("ERROR - Invalid DMA channel\n"));
            return PLX_STATUS_INVALID_ACCESS;
    }

    // Verify owner
    if (pdx->DmaInfo[channel].pOwner != pOwner)
    {
        DebugPrintf(("ERROR - DMA owned by different process\n"));
        return PLX_STATUS_IN_USE;
    }

    // Set shift for status register
    shift = (channel * 8);

    // Verify that DMA is not in progress
    RegValue =
        PLX_9000_REG_READ(
            pdx,
            PCI8311_DMA_COMMAND_STAT
            );

    if ((RegValue & ((1 << 4) << shift)) == 0)
    {
        DebugPrintf(("ERROR - DMA channel is currently active\n"));
        return PLX_STATUS_IN_PROGRESS;
    }

    spin_lock(
        &(pdx->Lock_Dma[channel])
        );

    // Verify DMA channel was opened
    if (pdx->DmaInfo[channel].bOpen == FALSE)
    {
        DebugPrintf(("ERROR - DMA channel has not been opened\n"));
        spin_unlock( &(pdx->Lock_Dma[channel]) );
        return PLX_STATUS_INVALID_ACCESS;
    }

    // Verify an SGL DMA transfer is not pending
    if (pdx->DmaInfo[channel].bSglPending)
    {
        DebugPrintf(("ERROR - An SGL DMA transfer is currently pending\n"));
        spin_unlock( &(pdx->Lock_Dma[channel]) );
        return PLX_STATUS_IN_PROGRESS;
    }

    // Set the SGL DMA pending flag
    pdx->DmaInfo[channel].bSglPending = TRUE;

    spin_unlock(
        &(pdx->Lock_Dma[channel])
        );

    // Get DMA mode
    RegValue =
        PLX_9000_REG_READ(
            pdx,
            OffsetMode
            );

    // Keep track if local address should remain constant
    if (RegValue & (1 << 11))
        pdx->DmaInfo[channel].bConstAddrLocal = TRUE;
    else
        pdx->DmaInfo[channel].bConstAddrLocal = FALSE;

    // Page-lock user buffer & build SGL
    rc =
        PlxLockBufferAndBuildSgl(
            pdx,
            channel,
            pParams,
            &SglPciAddress,
            &bBits64
            );

    if (rc != PLX_STATUS_OK)
    {
        DebugPrintf(("ERROR - Unable to lock buffer and build SGL list\n"));
        pdx->DmaInfo[channel].bSglPending = FALSE;
        return rc;
    }

    spin_lock(
        &(pdx->Lock_Dma[channel])
        );

    // Disable valid mode
    RegValue &= ~(1 << 20);

    // Enable DMA chaining, interrupt, & route interrupt to PCI
    RegValue |= (1 << 9) | (1 << 10) | (1 << 17);

    // Enable dual-addressing DMA if 64-bit DMA is required
    if (bBits64)
        RegValue |= (1 << 18);
    else
        RegValue &= ~(1 << 18);

    PLX_9000_REG_WRITE(
        pdx,
        OffsetMode,
        RegValue
        );

    // Clear DAC upper 32-bit PCI address in case it contains non-zero value
    PLX_9000_REG_WRITE(
        pdx,
        PCI8311_DMA0_PCI_DAC + (channel * sizeof(U32)),
        0
        );

    // Write SGL physical address & set descriptors in PCI space
    PLX_9000_REG_WRITE(
        pdx,
        OffsetMode + 0x10,
        SglPciAddress | (1 << 0)
        );

    // Enable DMA channel
    RegValue =
        PLX_9000_REG_READ(
            pdx,
            PCI8311_DMA_COMMAND_STAT
            );

    PLX_9000_REG_WRITE(
        pdx,
        PCI8311_DMA_COMMAND_STAT,
        RegValue | ((1 << 0) << shift)
        );

    spin_unlock(
        &(pdx->Lock_Dma[channel])
        );

    DebugPrintf(("Starting DMA transfer...\n"));

    // Start DMA
    PLX_9000_REG_WRITE(
        pdx,
        PCI8311_DMA_COMMAND_STAT,
        RegValue | (((1 << 0) | (1 << 1)) << shift)
        );

    return PLX_STATUS_OK;
}




/******************************************************************************
 *
 * Function   :  PlxChip_DmaChannelClose
 *
 * Description:  Close a previously opened channel
 *
 ******************************************************************************/
PLX_STATUS
PlxChip_DmaChannelClose(
    DEVICE_EXTENSION *pdx,
    U8                channel,
    BOOLEAN           bCheckInProgress,
    VOID             *pOwner
    )
{
    PLX_STATUS status;


    DebugPrintf(("Closing DMA channel %d...\n", channel));

    // Verify valid DMA channel
    switch (channel)
    {
        case 0:
        case 1:
            break;

        default:
            DebugPrintf(("ERROR - Invalid DMA channel\n"));
            return PLX_STATUS_INVALID_ACCESS;
    }

    // Verify DMA channel was opened
    if (pdx->DmaInfo[channel].bOpen == FALSE)
    {
        DebugPrintf(("ERROR - DMA channel has not been opened\n"));
        return PLX_STATUS_INVALID_ACCESS;
    }

    // Verify owner
    if (pdx->DmaInfo[channel].pOwner != pOwner)
    {
        DebugPrintf(("ERROR - DMA owned by different process\n"));
        return PLX_STATUS_IN_USE;
    }

    // Check DMA status
    status =
        PlxChip_DmaStatus(
            pdx,
            channel,
            pOwner
            );

    // Verify DMA is not in progress
    if (status != PLX_STATUS_COMPLETE)
    {
        // DMA is still in progress
        if (bCheckInProgress)
            return status;

        DebugPrintf(("DMA in progress, aborting...\n"));

        // Force DMA abort, which may generate a DMA done interrupt
        PlxChip_DmaControl(
            pdx,
            channel,
            DmaAbort,
            pOwner
            );

        // Small delay to let driver cleanup if DMA interrupts
        Plx_sleep( 100 );
    }

    spin_lock(
        &(pdx->Lock_Dma[channel])
        );

    // Close the channel
    pdx->DmaInfo[channel].bOpen = FALSE;

    // Clear owner information
    pdx->DmaInfo[channel].pOwner = NULL;

    spin_unlock(
        &(pdx->Lock_Dma[channel])
        );

    // If DMA is hung, an SGL transfer could be pending, so release user buffer
    if (pdx->DmaInfo[channel].bSglPending)
    {
        PlxSglDmaTransferComplete(
            pdx,
            channel
            );
    }

    // Release memory previously used for SGL descriptors
    if (pdx->DmaInfo[channel].SglBuffer.pKernelVa != NULL)
    {
        DebugPrintf(("Releasing memory used for SGL descriptors...\n"));

        Plx_dma_buffer_free(
            pdx,
            &pdx->DmaInfo[channel].SglBuffer
            );
    }

    return PLX_STATUS_OK;
}
