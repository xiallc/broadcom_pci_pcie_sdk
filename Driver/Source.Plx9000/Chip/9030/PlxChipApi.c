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
 *      PlxChipApi.c
 *
 * Description:
 *
 *      Implements chip-specific API functions
 *
 * Revision History:
 *
 *      05-01-13 : PLX SDK v7.10
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
    U32 RegInterrupt;
    U32 RegHotSwap;
    U32 RegPowerMgmnt;
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
            PCI9030_EEPROM_CTRL
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
                PCI9030_INT_CTRL_STAT
                );

        PLX_PCI_REG_READ(
            pdx,
            PCI9030_HS_CAP_ID,
            &RegHotSwap
            );

        PLX_PCI_REG_READ(
            pdx,
            PCI9030_PM_CSR,
            &RegPowerMgmnt
            );
    }

    // Issue Software Reset to hold PLX chip in reset
    PLX_9000_REG_WRITE(
        pdx,
        PCI9030_EEPROM_CTRL,
        RegValue | (1 << 30)
        );

    // Delay for a bit
    Plx_sleep(100);

    // Bring chip out of reset
    PLX_9000_REG_WRITE(
        pdx,
        PCI9030_EEPROM_CTRL,
        RegValue
        );

    // Issue EEPROM reload in case now programmed
    PLX_9000_REG_WRITE(
        pdx,
        PCI9030_EEPROM_CTRL,
        RegValue | (1 << 29)
        );

    // Delay for a bit
    Plx_sleep(10);

    // Clear EEPROM reload
    PLX_9000_REG_WRITE(
        pdx,
        PCI9030_EEPROM_CTRL,
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
            PCI9030_INT_CTRL_STAT,
            RegIntCtrlStatus
            );

        // Mask out HS bits that can be cleared
        RegHotSwap &= ~((1 << 23) | (1 << 22) | (1 << 17));

        PLX_PCI_REG_WRITE(
            pdx,
            PCI9030_HS_CAP_ID,
            RegHotSwap
            );

        // Mask out PM bits that can be cleared
        RegPowerMgmnt &= ~(1 << 15);

        PLX_PCI_REG_READ(
            pdx,
            PCI9030_PM_CSR,
            &RegPowerMgmnt
            );
    }

    return ApiSuccess;
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
        *pStatus = ApiUnsupportedFunction;

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
    return ApiUnsupportedFunction;
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
    RegData.offset      = PCI9030_INT_CTRL_STAT;
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

    return ApiSuccess;
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
    RegData.offset      = PCI9030_INT_CTRL_STAT;
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

    return ApiSuccess;
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
    U16               offset,
    U32              *pValue
    )
{
    // Verify the offset
    if ((offset & 0x3) || (offset > 0x200))
    {
        DebugPrintf(("ERROR - Invalid EEPROM offset\n"));
        return ApiInvalidOffset;
    }

    // Read EEPROM
    Plx9000_EepromReadByOffset(
        pdx,
        offset,
        pValue
        );

    return ApiSuccess;
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
    U16               offset,
    U32               value
    )
{
    U32 RegisterSave;


    // Verify the offset
    if ((offset & 0x3) || (offset > 0x200))
    {
        DebugPrintf(("ERROR - Invalid EEPROM offset\n"));
        return ApiInvalidOffset;
    }

    // Unprotect the EEPROM for write access
    RegisterSave =
        PLX_9000_REG_READ(
            pdx,
            PCI9030_INT_CTRL_STAT
            );

    PLX_9000_REG_WRITE(
        pdx,
        PCI9030_INT_CTRL_STAT,
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
        PCI9030_INT_CTRL_STAT,
        RegisterSave
        );

    return ApiSuccess;
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
    return ApiUnsupportedFunction;
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
    return ApiUnsupportedFunction;
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
    return ApiUnsupportedFunction;
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
    return ApiUnsupportedFunction;
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
    return ApiUnsupportedFunction;
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
    return ApiUnsupportedFunction;
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
    return ApiUnsupportedFunction;
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
    return ApiUnsupportedFunction;
}
