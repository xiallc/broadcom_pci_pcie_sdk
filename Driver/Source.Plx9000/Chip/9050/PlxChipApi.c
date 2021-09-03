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
    U8  EepromPresent;
    U32 RegValue;
    U32 DelayLoop;
    U32 RegInterrupt;
    U32 RegIntCtrlStatus;


    // Added to avoid compiler warnings
    RegIntCtrlStatus = 0;

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

    // Determine if an EEPROM is present
    RegValue =
        PLX_9000_REG_READ(
            pdx,
            PCI9050_EEPROM_CTRL
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
        RegIntCtrlStatus =
            PLX_9000_REG_READ(
                pdx,
                PCI9050_INT_CTRL_STAT
                );
    }

    // Issue Software Reset to hold PLX chip in reset
    PLX_9000_REG_WRITE(
        pdx,
        PCI9050_EEPROM_CTRL,
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
        PCI9050_EEPROM_CTRL,
        RegValue
        );

    // Issue EEPROM reload in case now programmed
    PLX_9000_REG_WRITE(
        pdx,
        PCI9050_EEPROM_CTRL,
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
        PCI9050_EEPROM_CTRL,
        RegValue
        );

    // Restore interrupt line
    PLX_PCI_REG_WRITE(
        pdx,
        0x3C,
        RegInterrupt
        );

    // If EEPROM was present, restore registers
    if (EepromPresent)
    {
        // Mask interrupt clear bits
        RegIntCtrlStatus &= ~((1 << 11) | (1 << 10));

        PLX_9000_REG_WRITE(
            pdx,
            PCI9050_INT_CTRL_STAT,
            RegIntCtrlStatus
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
    // Chip does not contain mailbox registers
    if (pStatus != NULL)
        *pStatus = PLX_STATUS_UNSUPPORTED;

    return 0;
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
    // Chip does not contain mailbox registers
    return PLX_STATUS_UNSUPPORTED;
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
    PLX_REG_DATA RegData;


    // Setup to synchronize access to Interrupt Control/Status Register
    RegData.pdx         = pdx;
    RegData.offset      = PCI9050_INT_CTRL_STAT;
    RegData.BitsToSet   = 0;
    RegData.BitsToClear = 0;

    if (pPlxIntr->PciMain)
        RegData.BitsToSet |= (1 << 6);

    if (pPlxIntr->LocalToPci & (1 << 0))
        RegData.BitsToSet |= (1 << 0);

    if (pPlxIntr->LocalToPci & (1 << 1))
        RegData.BitsToSet |= (1 << 3);

    // Write register values if they have changed
    if (RegData.BitsToSet != 0)
    {
        // Synchronize write of Interrupt Control/Status Register
        PlxSynchronizedRegisterModify(
            &RegData
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
    PLX_REG_DATA RegData;


    // Setup to synchronize access to Interrupt Control/Status Register
    RegData.pdx         = pdx;
    RegData.offset      = PCI9050_INT_CTRL_STAT;
    RegData.BitsToSet   = 0;
    RegData.BitsToClear = 0;

    if (pPlxIntr->PciMain)
        RegData.BitsToClear |= (1 << 6);

    if (pPlxIntr->LocalToPci & (1 << 0))
        RegData.BitsToClear |= (1 << 0);

    if (pPlxIntr->LocalToPci & (1 << 1))
        RegData.BitsToClear |= (1 << 3);

    // Write register values if they have changed
    if (RegData.BitsToClear != 0)
    {
        // Synchronize write of Interrupt Control/Status Register
        PlxSynchronizedRegisterModify(
            &RegData
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
    // Verify the offset
    if ((offset & 0x3) || (offset > 0x200))
    {
        DebugPrintf(("ERROR - Invalid EEPROM offset\n"));
        return PLX_STATUS_INVALID_OFFSET;
    }

    // 9050 does not protect the EEPROM from write access, do nothing

    // Write to EEPROM
    Plx9000_EepromWriteByOffset(
        pdx,
        offset,
        value
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
    return PLX_STATUS_UNSUPPORTED;
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
    return PLX_STATUS_UNSUPPORTED;
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
    return PLX_STATUS_UNSUPPORTED;
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
    return PLX_STATUS_UNSUPPORTED;
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
    return PLX_STATUS_UNSUPPORTED;
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
    return PLX_STATUS_UNSUPPORTED;
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
    return PLX_STATUS_UNSUPPORTED;
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
    return PLX_STATUS_UNSUPPORTED;
}
