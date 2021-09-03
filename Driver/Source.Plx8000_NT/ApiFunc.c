/*******************************************************************************
 * Copyright 2013-2018 Avago Technologies
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
 *      ApiFunc.c
 *
 * Description:
 *
 *      Implements the PLX API functions
 *
 * Revision History:
 *
 *      11-01-18 : PLX SDK v8.00
 *
 ******************************************************************************/


#include <linux/uaccess.h>  // For copy_to/from_user()
#include <linux/sched.h>    // For MAX_SCHED_TIMEOUT & TASK_UNINTERRUPTIBLE
#include "ApiFunc.h"
#include "Eep_8000.h"
#include "PciFunc.h"
#include "PciRegs.h"
#include "PlxInterrupt.h"
#include "SuppFunc.h"




/*******************************************************************************
 *
 * Function   :  PlxDeviceFind
 *
 * Description:  Search for a specific device using device key parameters
 *
 ******************************************************************************/
PLX_STATUS
PlxDeviceFind(
    DEVICE_EXTENSION *pdx,
    PLX_DEVICE_KEY   *pKey,
    U16              *pDeviceNumber
    )
{
    U16            DeviceCount;
    BOOLEAN        bMatchId;
    BOOLEAN        bMatchLoc;
    DEVICE_OBJECT *fdo;


    DeviceCount = 0;

    // Get first device instance in list
    fdo = pdx->pDeviceObject->DriverObject->DeviceObject;

    // Compare with items in device list
    while (fdo != NULL)
    {
        // Get the device extension
        pdx = fdo->DeviceExtension;

        // Assume successful match
        bMatchLoc = TRUE;
        bMatchId  = TRUE;

        //
        // Compare device key information
        //

        // Compare Bus, Slot, Fn numbers
        if ( (pKey->bus      != (U8)PCI_FIELD_IGNORE) ||
             (pKey->slot     != (U8)PCI_FIELD_IGNORE) ||
             (pKey->function != (U8)PCI_FIELD_IGNORE) )
        {
            if ( (pKey->bus      != pdx->Key.bus)  ||
                 (pKey->slot     != pdx->Key.slot) ||
                 (pKey->function != pdx->Key.function) )
            {
                bMatchLoc = FALSE;
            }
        }

        //
        // Compare device ID information
        //

        // Compare Vendor ID
        if (pKey->VendorId != (U16)PCI_FIELD_IGNORE)
        {
            if (pKey->VendorId != pdx->Key.VendorId)
            {
                bMatchId = FALSE;
            }
        }

        // Compare Device ID
        if (pKey->DeviceId != (U16)PCI_FIELD_IGNORE)
        {
            if (pKey->DeviceId != pdx->Key.DeviceId)
            {
                bMatchId = FALSE;
            }
        }

        // Compare Subsystem Vendor ID
        if (pKey->SubVendorId != (U16)PCI_FIELD_IGNORE)
        {
            if (pKey->SubVendorId != pdx->Key.SubVendorId)
            {
                bMatchId = FALSE;
            }
        }

        // Compare Subsystem Device ID
        if (pKey->SubDeviceId != (U16)PCI_FIELD_IGNORE)
        {
            if (pKey->SubDeviceId != pdx->Key.SubDeviceId)
            {
                bMatchId = FALSE;
            }
        }

        // Compare Revision
        if (pKey->Revision != (U8)PCI_FIELD_IGNORE)
        {
            if (pKey->Revision != pdx->Key.Revision)
            {
                bMatchId = FALSE;
            }
        }

        // Check if match on location and ID
        if (bMatchLoc && bMatchId)
        {
            // Match found, check if it is the desired device
            if (DeviceCount == *pDeviceNumber)
            {
                // Copy the device information
                *pKey = pdx->Key;

                DebugPrintf((
                    "Criteria matched device %04X_%04X [%02x:%02x.%x]\n",
                    pdx->Key.DeviceId, pdx->Key.VendorId,
                    pdx->Key.bus, pdx->Key.slot, pdx->Key.function
                    ));
                return PLX_STATUS_OK;
            }

            // Increment device count
            DeviceCount++;
        }

        // Jump to next entry
        fdo = fdo->NextDevice;
    }

    // Return number of matched devices
    *pDeviceNumber = DeviceCount;

    DebugPrintf(("Criteria did not match any devices\n"));

    return PLX_STATUS_INVALID_OBJECT;
}




/*******************************************************************************
 *
 * Function   :  PlxChipTypeGet
 *
 * Description:  Returns PLX chip type and revision
 *
 ******************************************************************************/
PLX_STATUS
PlxChipTypeGet(
    DEVICE_EXTENSION *pdx,
    U16              *pChipType,
    U8               *pRevision
    )
{
    *pChipType = pdx->Key.PlxChip;
    *pRevision = pdx->Key.PlxRevision;

    DebugPrintf((
        "PLX chip = %04X rev %02X\n",
        *pChipType, *pRevision
        ));

    return PLX_STATUS_OK;
}




/*******************************************************************************
 *
 * Function   :  PlxChipTypeSet
 *
 * Description:  Sets the PLX chip type dynamically
 *
 ******************************************************************************/
PLX_STATUS
PlxChipTypeSet(
    DEVICE_EXTENSION *pdx,
    U16               ChipType,
    U8                Revision
    )
{
    // Setting the PLX chip type is not supported in this PnP driver
    return PLX_STATUS_UNSUPPORTED;
}




/*******************************************************************************
 *
 * Function   :  PlxGetPortProperties
 *
 * Description:  Returns the current port information and status
 *
 ******************************************************************************/
PLX_STATUS
PlxGetPortProperties(
    DEVICE_EXTENSION *pdx,
    PLX_PORT_PROP    *pPortProp
    )
{
    U16 MaxSize;
    U16 Offset_PcieCap;
    U32 RegValue;


    // Set default properties
    RtlZeroMemory(pPortProp, sizeof(PLX_PORT_PROP));

    // Get the offset of the PCI Express capability
    Offset_PcieCap =
        PlxPciFindCapability(
            pdx,
            PCI_CAP_ID_PCI_EXPRESS,
            FALSE,
            0
            );
    if (Offset_PcieCap == 0)
    {
        DebugPrintf((
            "[D%d %02X:%02X.%X] Does not contain PCIe capability\n",
            pdx->Key.domain, pdx->Key.bus, pdx->Key.slot, pdx->Key.function
            ));

        // Mark device as non-PCIe
        pPortProp->bNonPcieDevice = TRUE;

        // Default to a legacy endpoint
        pPortProp->PortType = PLX_PORT_LEGACY_ENDPOINT;
        return PLX_STATUS_OK;
    }

    // Get PCIe Capability
    PLX_PCI_REG_READ(
        pdx,
        Offset_PcieCap,
        &RegValue
        );

    // Get port type
    pPortProp->PortType = (U8)((RegValue >> 20) & 0xF);

    // Get PCIe Device Capabilities
    PLX_PCI_REG_READ(
        pdx,
        Offset_PcieCap + 0x04,
        &RegValue
        );

    // Get max payload size supported field
    MaxSize = (U8)(RegValue >> 0) & 0x7;

    // Set max payload size (=128 * (2 ^ MaxPaySizeField))
    pPortProp->MaxPayloadSupported = PCIE_MPS_MRR_TO_BYTES( MaxSize );

    // Get PCIe Device Control
    PLX_PCI_REG_READ(
        pdx,
        Offset_PcieCap + 0x08,
        &RegValue
        );

    // Get max payload size field
    MaxSize = (U8)(RegValue >> 5) & 0x7;

    // Set max payload size (=128 * (2 ^ MaxPaySizeField))
    pPortProp->MaxPayloadSize = PCIE_MPS_MRR_TO_BYTES( MaxSize );

    // Get max read request size field
    MaxSize = (U8)(RegValue >> 12) & 0x7;

    // Set max read request size (=128 * (2 ^ MaxReadReqSizeField))
    pPortProp->MaxReadReqSize = PCIE_MPS_MRR_TO_BYTES( MaxSize );

    // Get PCIe Link Capabilities
    PLX_PCI_REG_READ(
        pdx,
        Offset_PcieCap + 0x0C,
        &RegValue
        );

    // Get port number
    pPortProp->PortNumber = (U8)((RegValue >> 24) & 0xFF);

    // Get max link width
    pPortProp->MaxLinkWidth = (U8)((RegValue >> 4) & 0x3F);

    // Get max link speed
    pPortProp->MaxLinkSpeed = (U8)((RegValue >> 0) & 0xF);

    // Get PCIe Link Status/Control
    PLX_PCI_REG_READ(
        pdx,
        Offset_PcieCap + 0x10,
        &RegValue
        );

    // Get link width
    pPortProp->LinkWidth = (U8)((RegValue >> 20) & 0x3F);

    // Get link speed
    pPortProp->LinkSpeed = (U8)((RegValue >> 16) & 0xF);

    DebugPrintf((
        "[D%d %02X:%02X.%X] P=%d T=%d MPS=%d/%d MRR=%d L=G%dx%d/G%dx%d\n",
        pdx->Key.domain, pdx->Key.bus, pdx->Key.slot, pdx->Key.function,
        pPortProp->PortNumber, pPortProp->PortType,
        pPortProp->MaxPayloadSize, pPortProp->MaxPayloadSupported,
        pPortProp->MaxReadReqSize,
        pPortProp->LinkSpeed, pPortProp->LinkWidth,
        pPortProp->MaxLinkSpeed, pPortProp->MaxLinkWidth
        ));

    return PLX_STATUS_OK;
}




/*******************************************************************************
 *
 * Function   :  PlxPciDeviceReset
 *
 * Description:  Resets a device using software reset feature of PLX chip
 *
 ******************************************************************************/
PLX_STATUS
PlxPciDeviceReset(
    DEVICE_EXTENSION *pdx
    )
{
    // Device reset not implemented
    return PLX_STATUS_UNSUPPORTED;
}




/*******************************************************************************
 *
 * Function   :  PlxRegisterRead
 *
 * Description:  Reads a PLX-specific register
 *
 ******************************************************************************/
U32
PlxRegisterRead(
    DEVICE_EXTENSION *pdx,
    U32               offset,
    PLX_STATUS       *pStatus,
    BOOLEAN           bAdjustForPort
    )
{
    // Adjust the offset for correct port
    if (bAdjustForPort)
    {
        offset += pdx->Offset_RegBase;
    }

    // Verify offset
    if ((offset & 0x3) || (offset >= pdx->PciBar[0].Properties.Size))
    {
        DebugPrintf(("ERROR - Invalid register offset (%X)\n", offset));
        if (pStatus)
        {
            *pStatus = PLX_STATUS_INVALID_OFFSET;
        }
        return 0;
    }

    if (pStatus)
    {
        *pStatus = PLX_STATUS_OK;
    }

    // For Draco 1, some register cause problems if accessed
    if (pdx->Key.PlxFamily == PLX_FAMILY_DRACO_1)
    {
        if ((offset == 0x856C)  || (offset == 0x8570) ||
            (offset == 0x1056C) || (offset == 0x10570))
        {
            return 0;
        }
    }

    return PLX_8000_REG_READ( pdx, offset );
}




/*******************************************************************************
 *
 * Function   :  PlxRegisterWrite
 *
 * Description:  Writes to a PLX-specific register
 *
 ******************************************************************************/
PLX_STATUS
PlxRegisterWrite(
    DEVICE_EXTENSION *pdx,
    U32               offset,
    U32               value,
    BOOLEAN           bAdjustForPort
    )
{
    // Adjust the offset for correct port
    if (bAdjustForPort)
    {
        offset += pdx->Offset_RegBase;
    }

    // Verify offset
    if ((offset & 0x3) || (offset >= pdx->PciBar[0].Properties.Size))
    {
        DebugPrintf(("ERROR - Invalid register offset (%X)\n", offset));
        return PLX_STATUS_INVALID_OFFSET;
    }

    // For Draco 1, some register cause problems if accessed
    if (pdx->Key.PlxFamily == PLX_FAMILY_DRACO_1)
    {
        if ((offset == 0x856C)  || (offset == 0x8570) ||
            (offset == 0x1056C) || (offset == 0x10570))
        {
            return PLX_STATUS_OK;
        }
    }

    PLX_8000_REG_WRITE( pdx, offset, value );

    return PLX_STATUS_OK;
}




/*******************************************************************************
 *
 * Function   :  PlxMailboxRead
 *
 * Description:  Reads a PLX mailbox register
 *
 ******************************************************************************/
U32
PlxMailboxRead(
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
    if ((pdx->Key.PlxChip & 0xFF00) == 0x8500)
    {
        offset = 0xB0;
    }
    else
    {
        offset = 0xC6C;
    }

    // Set status code
    if (pStatus)
    {
        *pStatus = PLX_STATUS_OK;
    }

    // Calculate mailbox offset
    offset = offset + (mailbox * sizeof(U32));

    // Read mailbox
    return PlxRegisterRead(
        pdx,
        offset,
        pStatus,
        TRUE        // Adjust for port
        );
}




/*******************************************************************************
 *
 * Function   :  PlxMailboxWrite
 *
 * Description:  Writes to a PLX mailbox register
 *
 ******************************************************************************/
PLX_STATUS
PlxMailboxWrite(
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
    if ((pdx->Key.PlxChip & 0xFF00) == 0x8500)
    {
        offset = 0xB0;
    }
    else
    {
        offset = 0xC6C;
    }

    // Calculate mailbox offset
    offset = offset + (mailbox * sizeof(U32));

    // Write mailbox
    return PlxRegisterWrite(
        pdx,
        offset,
        value,
        TRUE        // Adjust for port
        );
}




/*******************************************************************************
 *
 * Function   :  PlxPciBarProperties
 *
 * Description:  Returns the properties of a PCI BAR space
 *
 ******************************************************************************/
PLX_STATUS
PlxPciBarProperties(
    DEVICE_EXTENSION *pdx,
    U8                BarIndex,
    PLX_PCI_BAR_PROP *pBarProperties
    )
{
    // Verify BAR number
    switch (BarIndex)
    {
        case 0:
        case 1:
        case 2:
        case 3:
        case 4:
        case 5:
            break;

        default:
            return PLX_STATUS_INVALID_ACCESS;
    }

    // Return BAR properties
    *pBarProperties = pdx->PciBar[BarIndex].Properties;

    // Display BAR properties if enabled
    if (pdx->PciBar[BarIndex].Properties.Size == 0)
    {
        DebugPrintf(("BAR %d is disabled\n", BarIndex));
        return PLX_STATUS_OK;
    }

    DebugPrintf((
        "    PCI BAR %d: %08llX\n",
        BarIndex, pdx->PciBar[BarIndex].Properties.BarValue
        ));

    DebugPrintf((
        "    Phys Addr: %08llX\n",
        pdx->PciBar[BarIndex].Properties.Physical
        ));

    DebugPrintf((
        "    Size     : %llXh (%lld%s)\n",
        pdx->PciBar[BarIndex].Properties.Size,
        pdx->PciBar[BarIndex].Properties.Size > ((U64)1 << 30) ?
           (pdx->PciBar[BarIndex].Properties.Size >> 30) :
           pdx->PciBar[BarIndex].Properties.Size > ((U64)1 << 20) ?
           (pdx->PciBar[BarIndex].Properties.Size >> 20) :
           pdx->PciBar[BarIndex].Properties.Size > ((U64)1 << 10) ?
           (pdx->PciBar[BarIndex].Properties.Size >> 10) :
           pdx->PciBar[BarIndex].Properties.Size,
        pdx->PciBar[BarIndex].Properties.Size > ((U64)1 << 30) ? "GB" :
           pdx->PciBar[BarIndex].Properties.Size > ((U64)1 << 20) ? "MB" :
           pdx->PciBar[BarIndex].Properties.Size > ((U64)1 << 10) ? "KB" : "B"
        ));

    DebugPrintf((
        "    Property : %sPrefetchable %d-bit\n",
        (pdx->PciBar[BarIndex].Properties.Flags & PLX_BAR_FLAG_PREFETCHABLE) ? "" : "Non-",
        (pdx->PciBar[BarIndex].Properties.Flags & PLX_BAR_FLAG_64_BIT) ? 64 : 32
        ));

    return PLX_STATUS_OK;
}




/*******************************************************************************
 *
 * Function   :  PlxPciBarMap
 *
 * Description:  Map a PCI BAR Space into User virtual space
 *
 ******************************************************************************/
PLX_STATUS
PlxPciBarMap(
    DEVICE_EXTENSION *pdx,
    U8                BarIndex,
    VOID             *pUserVa,
    VOID             *pOwner
    )
{
    // Handled in Dispatch_mmap() in Linux
    return PLX_STATUS_UNSUPPORTED;
}




/*******************************************************************************
 *
 * Function   :  PlxPciBarUnmap
 *
 * Description:  Unmap a previously mapped PCI BAR Space from User virtual space
 *
 ******************************************************************************/
PLX_STATUS
PlxPciBarUnmap(
    DEVICE_EXTENSION *pdx,
    VOID             *UserVa,
    VOID             *pOwner
    )
{
    // Handled at user API level in Linux
    return PLX_STATUS_UNSUPPORTED;
}




/*******************************************************************************
 *
 * Function   :  PlxEepromPresent
 *
 * Description:  Returns the state of the EEPROM as reported by the PLX device
 *
 ******************************************************************************/
PLX_STATUS
PlxEepromPresent(
    DEVICE_EXTENSION *pdx,
    U8               *pStatus
    )
{
    return Plx8000_EepromPresent(
               pdx,
               pStatus
               );
}




/*******************************************************************************
 *
 * Function   :  PlxEepromProbe
 *
 * Description:  Probes for the presence of an EEPROM
 *
 ******************************************************************************/
PLX_STATUS
PlxEepromProbe(
    DEVICE_EXTENSION *pdx,
    U8               *pFlag
    )
{
    U16        TempChip;
    U16        OffsetProbe;
    U32        ValueRead;
    U32        ValueWrite;
    U32        ValueOriginal;
    PLX_STATUS status;


    // Default to no EEPROM present
    *pFlag = FALSE;

    // Generalize by device type
    switch (pdx->Key.PlxChip & 0xFF00)
    {
        case 0x8600:
        case 0x8700:
        case 0x9700:
            TempChip = pdx->Key.PlxChip & 0xFF00;
            break;

        default:
            TempChip = pdx->Key.PlxChip;
            break;
    }

    // Determine EEPROM offset to use for probe (e.g. after CRC)
    switch (TempChip)
    {
        case 0x8508:
        case 0x8512:
        case 0x8517:
        case 0x8518:
            OffsetProbe = (0x78F * sizeof(U32)) + sizeof(U32);
            break;

        case 0x8516:
        case 0x8524:
        case 0x8532:
            OffsetProbe = (0xBE4 * sizeof(U32)) + sizeof(U32);
            break;

        case 0x8600:
        case 0x8700:
        case 0x9700:
            OffsetProbe = 0x10;     // No CRC, just use any reasonable address
            break;

        case 0:
        default:
            DebugPrintf((
                "ERROR - Not a supported PLX device (%04X)\n",
                pdx->Key.PlxChip
                ));
            return PLX_STATUS_UNSUPPORTED;
    }

    DebugPrintf(("Probe EEPROM at offset %02xh\n", OffsetProbe));

    // Get the current value
    status =
        PlxEepromReadByOffset(
            pdx,
            OffsetProbe,
            &ValueOriginal
            );

    if (status != PLX_STATUS_OK)
    {
        return status;
    }

    // Prepare inverse value to write
    ValueWrite = ~(ValueOriginal);

    // Write inverse of original value
    status =
        PlxEepromWriteByOffset(
            pdx,
            OffsetProbe,
            ValueWrite
            );

    if (status != PLX_STATUS_OK)
    {
        return status;
    }

    // Read updated value
    status =
        PlxEepromReadByOffset(
            pdx,
            OffsetProbe,
            &ValueRead
            );

    if (status != PLX_STATUS_OK)
    {
        return status;
    }

    // Check if value was written properly
    if (ValueRead == ValueWrite)
    {
        DebugPrintf(("Probe detected an EEPROM present\n"));

        *pFlag = TRUE;

        // Restore the original value
        PlxEepromWriteByOffset(
            pdx,
            OffsetProbe,
            ValueOriginal
            );
    }
    else
    {
        DebugPrintf(("Probe did not detect an EEPROM\n"));
    }

    return PLX_STATUS_OK;
}




/*******************************************************************************
 *
 * Function   :  PlxEepromGetAddressWidth
 *
 * Description:  Returns the current EEPROM address width
 *
 ******************************************************************************/
PLX_STATUS
PlxEepromGetAddressWidth(
    DEVICE_EXTENSION *pdx,
    U8               *pWidth
    )
{
    PLX_STATUS status;


    status =
        Plx8000_EepromGetAddressWidth(
            pdx,
            pWidth
            );

    DebugPrintf(("EEPROM address width = %dB\n", *pWidth));
    return status;
}




/*******************************************************************************
 *
 * Function   :  PlxEepromSetAddressWidth
 *
 * Description:  Sets a new EEPROM address width
 *
 ******************************************************************************/
PLX_STATUS
PlxEepromSetAddressWidth(
    DEVICE_EXTENSION *pdx,
    U8                width
    )
{
    PLX_STATUS status;


    // Verify the width
    switch (width)
    {
        case 1:
        case 2:
        case 3:
            break;

        default:
            DebugPrintf(("ERROR - Invalid address width (%d)\n", width));
            return PLX_STATUS_INVALID_DATA;
    }

    status =
        Plx8000_EepromSetAddressWidth(
            pdx,
            width
            );

    DebugPrintf((
       "%s EEPROM address width to %dB\n",
       (status == PLX_STATUS_OK) ? "Set" : "ERROR - Unable to set",
       width
       ));

    return status;
}




/*******************************************************************************
 *
 * Function   :  PlxEepromCrcGet
 *
 * Description:  Returns the EEPROM CRC and its status
 *
 ******************************************************************************/
PLX_STATUS
PlxEepromCrcGet(
    DEVICE_EXTENSION *pdx,
    U32              *pCrc,
    U8               *pCrcStatus
    )
{
    // Clear return value
    *pCrc       = 0;
    *pCrcStatus = PLX_CRC_UNSUPPORTED;

    switch (pdx->Key.PlxChip & 0xFF00)
    {
        case 0x8500:
        case 0x8700:
        case 0x9700:
            return Plx8000_EepromCrcGet(
                pdx,
                pCrc,
                pCrcStatus
                );
    }

    // CRC not supported
    return PLX_STATUS_UNSUPPORTED;
}




/*******************************************************************************
 *
 * Function   :  PlxEepromCrcUpdate
 *
 * Description:  Updates the EEPROM CRC
 *
 ******************************************************************************/
PLX_STATUS
PlxEepromCrcUpdate(
    DEVICE_EXTENSION *pdx,
    U32              *pCrc,
    BOOLEAN           bUpdateEeprom
    )
{
    // Clear return value
    *pCrc = 0;

    switch (pdx->Key.PlxChip & 0xFF00)
    {
        case 0x8500:
        case 0x8700:
        case 0x9700:
            return Plx8000_EepromCrcUpdate(
                pdx,
                pCrc,
                bUpdateEeprom
                );
    }

    // CRC not supported
    return PLX_STATUS_UNSUPPORTED;
}




/*******************************************************************************
 *
 * Function   :  PlxEepromReadByOffset
 *
 * Description:  Read a 32-bit value from the EEPROM at a specified offset
 *
 ******************************************************************************/
PLX_STATUS
PlxEepromReadByOffset(
    DEVICE_EXTENSION *pdx,
    U32               offset,
    U32              *pValue
    )
{
    // Set default return value
    *pValue = 0;

    // Make sure offset is aligned on 32-bit boundary
    if (offset & (3 << 0))
    {
        return PLX_STATUS_INVALID_OFFSET;
    }

    return Plx8000_EepromReadByOffset(
        pdx,
        offset,
        pValue
        );
}




/*******************************************************************************
 *
 * Function   :  PlxEepromWriteByOffset
 *
 * Description:  Write a 32-bit value to the EEPROM at a specified offset
 *
 ******************************************************************************/
PLX_STATUS
PlxEepromWriteByOffset(
    DEVICE_EXTENSION *pdx,
    U32               offset,
    U32               value
    )
{
    // Make sure offset is aligned on 32-bit boundary
    if (offset & (3 << 0))
    {
        return PLX_STATUS_INVALID_OFFSET;
    }

    return Plx8000_EepromWriteByOffset(
        pdx,
        offset,
        value
        );
}




/*******************************************************************************
 *
 * Function   :  PlxEepromReadByOffset_16
 *
 * Description:  Read a 16-bit value from the EEPROM at a specified offset
 *
 ******************************************************************************/
PLX_STATUS
PlxEepromReadByOffset_16(
    DEVICE_EXTENSION *pdx,
    U32               offset,
    U16              *pValue
    )
{
    U32        Value_32;
    PLX_STATUS status;


    // Set default return value
    *pValue = 0;

    // Make sure offset is aligned on 16-bit boundary
    if (offset & (1 << 0))
    {
        return PLX_STATUS_INVALID_OFFSET;
    }

    /******************************************
     * For devices that do not support 16-bit
     * EEPROM accesses, use 32-bit access
     *****************************************/

    // Get 32-bit value
    status =
        PlxEepromReadByOffset(
            pdx,
            (offset & ~0x3),
            &Value_32
            );

    if (status != PLX_STATUS_OK)
    {
        return status;
    }

    // Return desired 16-bit portion
    if (offset & 0x3)
    {
        *pValue = (U16)(Value_32 >> 16);
    }
    else
    {
        *pValue = (U16)Value_32;
    }

    return PLX_STATUS_OK;
}




/*******************************************************************************
 *
 * Function   :  PlxEepromWriteByOffset_16
 *
 * Description:  Write a 16-bit value to the EEPROM at a specified offset
 *
 ******************************************************************************/
PLX_STATUS
PlxEepromWriteByOffset_16(
    DEVICE_EXTENSION *pdx,
    U32               offset,
    U16               value
    )
{
    U32        Value_32;
    PLX_STATUS status;


    // Make sure offset is aligned on 16-bit boundary
    if (offset & (1 << 0))
    {
        return PLX_STATUS_INVALID_OFFSET;
    }

    /******************************************
     * For devices that do not support 16-bit
     * EEPROM accesses, use 32-bit access
     *****************************************/

    // Get current 32-bit value
    status =
        PlxEepromReadByOffset(
            pdx,
            (offset & ~0x3),
            &Value_32
            );

    if (status != PLX_STATUS_OK)
    {
        return status;
    }

    // Insert new 16-bit value in correct location
    if (offset & 0x3)
    {
        Value_32 = ((U32)value << 16) | (Value_32 & 0xFFFF);
    }
    else
    {
        Value_32 = ((U32)value) | (Value_32 & 0xFFFF0000);
    }

    // Update EEPROM
    return PlxEepromWriteByOffset(
        pdx,
        (offset & ~0x3),
        Value_32
        );
}




/*******************************************************************************
 *
 * Function   :  PlxPciIoPortTransfer
 *
 * Description:  Read or Write from/to an I/O port
 *
 ******************************************************************************/
PLX_STATUS
PlxPciIoPortTransfer(
    U64              IoPort,
    VOID            *pBuffer,
    U32              SizeInBytes,
    PLX_ACCESS_TYPE  AccessType,
    BOOLEAN          bReadOperation
    )
{
    U8  AlignMask;
    U8  AccessSize;
    U32 Value;


    if (pBuffer == NULL)
    {
        return PLX_STATUS_NULL_PARAM;
    }

    // Verify size & type
    switch (AccessType)
    {
        case BitSize8:
            AlignMask  = 0;
            AccessSize = sizeof(U8);
            break;

        case BitSize16:
            AlignMask  = (1 << 0);
            AccessSize = sizeof(U16);
            break;

        case BitSize32:
            AlignMask  = (3 << 0);
            AccessSize = sizeof(U32);
            break;

        default:
            return PLX_STATUS_INVALID_ACCESS;
    }

    // Verify alignments
    if (IoPort & AlignMask)
    {
        DebugPrintf(("ERROR - I/O port not %d-bit aligned\n", (AccessSize * 8)));
        return PLX_STATUS_INVALID_ADDR;
    }

    if (SizeInBytes & AlignMask)
    {
        DebugPrintf(("ERROR - Byte count not %d-bit aligned\n", (AccessSize * 8)));
        return PLX_STATUS_INVALID_SIZE;
    }

    // Perform operation
    while (SizeInBytes)
    {
        if (bReadOperation)
        {
            switch (AccessType)
            {
                case BitSize8:
                    Value = IO_PORT_READ_8( IoPort );
                    break;

                case BitSize16:
                    Value = IO_PORT_READ_16( IoPort );
                    break;

                case BitSize32:
                    Value = IO_PORT_READ_32( IoPort );
                    break;

                default:
                    // Added to avoid compiler warnings
                    break;
            }

            // Copy value to user buffer
            if (copy_to_user( pBuffer, &Value, AccessSize ) != 0)
            {
                return PLX_STATUS_INVALID_ACCESS;
            }
        }
        else
        {
            // Copy next value from user buffer
            if (copy_from_user( &Value, pBuffer, AccessSize ) != 0)
            {
                return PLX_STATUS_INVALID_ACCESS;
            }

            switch (AccessType)
            {
                case BitSize8:
                    IO_PORT_WRITE_8( IoPort, (U8)Value );
                    break;

                case BitSize16:
                    IO_PORT_WRITE_16( IoPort, (U16)Value );
                    break;

                case BitSize32:
                    IO_PORT_WRITE_32( IoPort, (U32)Value );
                    break;

                default:
                    // Added to avoid compiler warnings
                    break;
            }
        }

        // Adjust pointer & byte count
        pBuffer      = (U8*)pBuffer + AccessSize;
        SizeInBytes -= AccessSize;
    }

    return PLX_STATUS_OK;
}




/*******************************************************************************
 *
 * Function   :  PlxPciPhysicalMemoryAllocate
 *
 * Description:  Allocate physically contiguous page-locked memory
 *
 ******************************************************************************/
PLX_STATUS
PlxPciPhysicalMemoryAllocate(
    DEVICE_EXTENSION *pdx,
    PLX_PHYSICAL_MEM *pPciMem,
    BOOLEAN           bSmallerOk,
    VOID             *pOwner
    )
{
    U32                  DecrementAmount;
    PLX_PHYS_MEM_OBJECT *pMemObject;


    // Initialize buffer information
    pPciMem->UserAddr     = 0;
    pPciMem->PhysicalAddr = 0;
    pPciMem->CpuPhysical  = 0;

    /*******************************************************
     * Verify size
     *
     * A size of 0 is valid because this function may
     * be called to allocate a common buffer of size 0;
     * therefore, the information is reset & return sucess.
     ******************************************************/
    if (pPciMem->Size == 0)
    {
        return PLX_STATUS_OK;
    }

    // Allocate memory for new list object
    pMemObject =
        kmalloc(
            sizeof(PLX_PHYS_MEM_OBJECT),
            GFP_KERNEL
            );
    if (pMemObject == NULL)
    {
        DebugPrintf(("ERROR - Memory allocation for list object failed\n"));
        return PLX_STATUS_INSUFFICIENT_RES;
    }

    // Clear object
    RtlZeroMemory( pMemObject, sizeof(PLX_PHYS_MEM_OBJECT) );

    // Set buffer request size
    pMemObject->Size = pPciMem->Size;

    // Setup amount to reduce on failure
    DecrementAmount = (pPciMem->Size / 10);

    DebugPrintf((
        "Attempt to allocate physical memory (%dKB)\n",
        (pPciMem->Size >> 10)
        ));

    do
    {
        // Attempt to allocate the buffer
        pMemObject->pKernelVa =
            Plx_dma_buffer_alloc(
                pdx,
                pMemObject
                );

        if (pMemObject->pKernelVa == NULL)
        {
            // Reduce memory request size if requested
            if (bSmallerOk && (pMemObject->Size > PAGE_SIZE))
            {
                pMemObject->Size -= DecrementAmount;
            }
            else
            {
                ErrorPrintf(("ERROR - Physical memory allocation failed\n"));
                kfree( pMemObject );
                pPciMem->Size = 0;
                return PLX_STATUS_INSUFFICIENT_RES;
            }
        }
    }
    while (pMemObject->pKernelVa == NULL);

    // Record buffer owner
    pMemObject->pOwner = pOwner;

    // Assign buffer to device if provided
    if (pOwner != pGbl_DriverObject)
    {
        // Return buffer information
        pPciMem->Size         = pMemObject->Size;
        pPciMem->PhysicalAddr = pMemObject->BusPhysical;
        pPciMem->CpuPhysical  = pMemObject->CpuPhysical;

        // Add buffer object to list
        spin_lock(
            &(pdx->Lock_PhysicalMemList)
            );

        list_add_tail(
            &(pMemObject->ListEntry),
            &(pdx->List_PhysicalMem)
            );

        spin_unlock(
            &(pdx->Lock_PhysicalMemList)
            );
    }
    else
    {
        // Store common buffer information
        pGbl_DriverObject->CommonBuffer = *pMemObject;

        // Release the list object
        kfree( pMemObject );
    }

    return PLX_STATUS_OK;
}




/*******************************************************************************
 *
 * Function   :  PlxPciPhysicalMemoryFree
 *
 * Description:  Free previously allocated physically contiguous page-locked memory
 *
 ******************************************************************************/
PLX_STATUS
PlxPciPhysicalMemoryFree(
    DEVICE_EXTENSION *pdx,
    PLX_PHYSICAL_MEM *pPciMem
    )
{
    struct list_head    *pEntry;
    PLX_PHYS_MEM_OBJECT *pMemObject;


    spin_lock( &(pdx->Lock_PhysicalMemList) );

    pEntry = pdx->List_PhysicalMem.next;

    // Traverse list to find the desired list object
    while (pEntry != &(pdx->List_PhysicalMem))
    {
        // Get the object
        pMemObject =
            list_entry(
                pEntry,
                PLX_PHYS_MEM_OBJECT,
                ListEntry
                );

        // Check if the physical addresses matches
        if (pMemObject->BusPhysical == pPciMem->PhysicalAddr)
        {
            // Remove the object from the list
            list_del( pEntry );

            spin_unlock( &(pdx->Lock_PhysicalMemList) );

            // Release the buffer
            Plx_dma_buffer_free( pdx, pMemObject );

            // Release the list object
            kfree( pMemObject );

            return PLX_STATUS_OK;
        }

        // Jump to next item in the list
        pEntry = pEntry->next;
    }

    spin_unlock( &(pdx->Lock_PhysicalMemList) );

    DebugPrintf(("ERROR - buffer object not found in list\n"));

    return PLX_STATUS_INVALID_DATA;
}




/*******************************************************************************
 *
 * Function   :  PlxPciPhysicalMemoryMap
 *
 * Description:  Maps physical memory to User virtual address space
 *
 ******************************************************************************/
PLX_STATUS
PlxPciPhysicalMemoryMap(
    DEVICE_EXTENSION *pdx,
    PLX_PHYSICAL_MEM *pPciMem,
    VOID             *pOwner
    )
{
    // Handled in Dispatch_mmap() in Linux
    return PLX_STATUS_UNSUPPORTED;
}




/*******************************************************************************
 *
 * Function   :  PlxPciPhysicalMemoryUnmap
 *
 * Description:  Unmap physical memory from User virtual address space
 *
 ******************************************************************************/
PLX_STATUS
PlxPciPhysicalMemoryUnmap(
    DEVICE_EXTENSION *pdx,
    PLX_PHYSICAL_MEM *pPciMem,
    VOID             *pOwner
    )
{
    // Handled by user-level API in Linux
    return PLX_STATUS_UNSUPPORTED;
}




/*******************************************************************************
 *
 * Function   :  PlxInterruptEnable
 *
 * Description:  Enables specific interupts of the PLX Chip
 *
 ******************************************************************************/
PLX_STATUS
PlxInterruptEnable(
    DEVICE_EXTENSION *pdx,
    PLX_INTERRUPT    *pPlxIntr
    )
{
    PLX_REG_DATA RegData;


    // Setup to synchronize access to interrupt register
    RegData.pdx         = pdx;
    RegData.BitsToSet   = 0;
    RegData.BitsToClear = 0;

    // NT Virtual Link-side Error status interrupts
    if (((pdx->Key.PlxChip & 0xFF00) != 0x8500) &&
         (pdx->Key.PlxPortType == PLX_SPEC_PORT_NT_VIRTUAL))
    {
        if (pPlxIntr->NTV_LE_Correctable)
        {
            RegData.BitsToClear |= (1 << 0);
        }

        if (pPlxIntr->NTV_LE_Uncorrectable)
        {
            RegData.BitsToClear |= (1 << 1);
        }

        if (pPlxIntr->NTV_LE_LinkStateChange)
        {
            RegData.BitsToClear |= (1 << 2);
        }

        if (pPlxIntr->NTV_LE_UncorrErrorMsg)
        {
            RegData.BitsToClear |= (1 << 3);
        }

        if (RegData.BitsToClear != 0)
        {
            // Setup for Link Error interrupt mask register
            RegData.offset = pdx->Offset_LE_IntMask;

            // Synchronize write to interrupt register
            PlxSynchronizedRegisterModify(
                &RegData
                );
        }
    }

    // Only 16 doorbell interrupts are supported
    pPlxIntr->Doorbell &= 0xFFFF;

    // Enable doorbell interrupts
    if (pPlxIntr->Doorbell)
    {
        // Setup for doorbell interrupt register
        RegData.offset      = pdx->Offset_DB_IntMaskClear;
        RegData.BitsToSet   = pPlxIntr->Doorbell;
        RegData.BitsToClear = 0;

        // Synchronize write to interrupt register
        PlxSynchronizedRegisterModify(
            &RegData
            );
    }

    return PLX_STATUS_OK;
}




/*******************************************************************************
 *
 * Function   :  PlxInterruptDisable
 *
 * Description:  Disables specific interrupts of the PLX Chip
 *
 ******************************************************************************/
PLX_STATUS
PlxInterruptDisable(
    DEVICE_EXTENSION *pdx,
    PLX_INTERRUPT    *pPlxIntr
    )
{
    PLX_REG_DATA RegData;


    // Setup to synchronize access to interrupt register
    RegData.pdx         = pdx;
    RegData.BitsToSet   = 0;
    RegData.BitsToClear = 0;

    // NT Virtual Link-side Error status interrupts
    if (((pdx->Key.PlxChip & 0xFF00) != 0x8500) &&
         (pdx->Key.PlxPortType == PLX_SPEC_PORT_NT_VIRTUAL))
    {
        if (pPlxIntr->NTV_LE_Correctable)
        {
            RegData.BitsToSet |= (1 << 0);
        }

        if (pPlxIntr->NTV_LE_Uncorrectable)
        {
            RegData.BitsToSet |= (1 << 1);
        }

        if (pPlxIntr->NTV_LE_LinkStateChange)
        {
            RegData.BitsToSet |= (1 << 2);
        }

        if (pPlxIntr->NTV_LE_UncorrErrorMsg)
        {
            RegData.BitsToSet |= (1 << 3);
        }

        if (RegData.BitsToSet != 0)
        {
            // Setup for Link Error interrupt mask register
            RegData.offset = pdx->Offset_LE_IntMask;

            // Synchronize write to interrupt register
            PlxSynchronizedRegisterModify(
                &RegData
                );
        }
    }

    // Only 16 doorbell interrupts are supported
    pPlxIntr->Doorbell &= 0xFFFF;

    // Disable doorbell interrupts
    if (pPlxIntr->Doorbell)
    {
        // Setup for doorbell interrupt register
        RegData.offset      = pdx->Offset_DB_IntMaskSet;
        RegData.BitsToSet   = pPlxIntr->Doorbell;
        RegData.BitsToClear = 0;

        // Synchronize write to interrupt register
        PlxSynchronizedRegisterModify(
            &RegData
            );
    }

    return PLX_STATUS_OK;
}




/*******************************************************************************
 *
 * Function   :  PlxNotificationRegisterFor
 *
 * Description:  Registers a wait object for notification on interrupt(s)
 *
 ******************************************************************************/
PLX_STATUS
PlxNotificationRegisterFor(
    DEVICE_EXTENSION  *pdx,
    PLX_INTERRUPT     *pPlxIntr,
    VOID             **pUserWaitObject,
    VOID              *pOwner
    )
{
    unsigned long    flags;
    PLX_WAIT_OBJECT *pWaitObject;


    // Allocate a new wait object
    pWaitObject =
        kmalloc(
            sizeof(PLX_WAIT_OBJECT),
            GFP_KERNEL
            );

    if (pWaitObject == NULL)
    {
        DebugPrintf(("ERROR - Allocation for interrupt wait object failed\n"));
        return PLX_STATUS_INSUFFICIENT_RES;
    }

    // Provide the wait object to the user app
    *pUserWaitObject = pWaitObject;

    // Record the owner
    pWaitObject->pOwner = pOwner;

    // Mark the object as waiting
    pWaitObject->state = PLX_STATE_WAITING;

    // Clear number of sleeping threads
    atomic_set( &pWaitObject->SleepCount, 0 );

    // Initialize wait queue
    init_waitqueue_head( &(pWaitObject->WaitQueue) );

    // Clear interrupt source
    pWaitObject->Source_Ints     = INTR_TYPE_NONE;
    pWaitObject->Source_Doorbell = 0;

    // Set interrupt notification flags
    pWaitObject->Notify_Flags    = INTR_TYPE_NONE;
    pWaitObject->Notify_Doorbell = pPlxIntr->Doorbell;

    if (pPlxIntr->NTV_LE_Correctable)
    {
        pWaitObject->Notify_Flags |= INTR_TYPE_LE_CORRECTABLE;
    }

    if (pPlxIntr->NTV_LE_Uncorrectable)
    {
        pWaitObject->Notify_Flags |= INTR_TYPE_LE_UNCORRECTABLE;
    }

    if (pPlxIntr->NTV_LE_LinkStateChange)
    {
        pWaitObject->Notify_Flags |= INTR_TYPE_LE_LINK_STATE_CHANGE;
    }

    if (pPlxIntr->NTV_LE_UncorrErrorMsg)
    {
        pWaitObject->Notify_Flags |= INTR_TYPE_LE_UNCORR_ERR_MSG;
    }

    // Add to list of waiting objects
    spin_lock_irqsave(
        &(pdx->Lock_WaitObjectsList),
        flags
        );

    list_add_tail(
        &(pWaitObject->ListEntry),
        &(pdx->List_WaitObjects)
        );

    spin_unlock_irqrestore(
        &(pdx->Lock_WaitObjectsList),
        flags
        );

    DebugPrintf(("Registered interrupt wait object (%p)\n", pWaitObject));

    return PLX_STATUS_OK;
}




/*******************************************************************************
 *
 * Function   :  PlxNotificationWait
 *
 * Description:  Put the process to sleep until wake-up event occurs or timeout
 *
 ******************************************************************************/
PLX_STATUS
PlxNotificationWait(
    DEVICE_EXTENSION *pdx,
    VOID             *pUserWaitObject,
    PLX_UINT_PTR      Timeout_ms
    )
{
    long              Wait_rc;
    PLX_STATUS        rc;
    PLX_UINT_PTR      Timeout_sec;
    unsigned long     flags;
    struct list_head *pEntry;
    PLX_WAIT_OBJECT  *pWaitObject;


    // Find the wait object in the list
    spin_lock_irqsave(
        &(pdx->Lock_WaitObjectsList),
        flags
        );

    pEntry = pdx->List_WaitObjects.next;

    // Find the wait object and wait for wake-up event
    while (pEntry != &(pdx->List_WaitObjects))
    {
        // Get the wait object
        pWaitObject =
            list_entry(
                pEntry,
                PLX_WAIT_OBJECT,
                ListEntry
                );

        // Check if the object address matches the Tag
        if (pWaitObject == pUserWaitObject)
        {
            spin_unlock_irqrestore(
                &(pdx->Lock_WaitObjectsList),
                flags
                );

            DebugPrintf((
                "Wait for Interrupt wait object (%p) to wake-up\n",
                pWaitObject
                ));

            /*********************************************************
             * Convert milliseconds to jiffies.  The following
             * formula is used:
             *
             *                      ms * HZ
             *           jiffies = ---------
             *                       1,000
             *
             *
             *  where:  HZ      = System-defined clock ticks per second
             *          ms      = Timeout in milliseconds
             *          jiffies = Number of HZ's per second
             *
             *  Note: Since the timeout is stored as a "long" integer,
             *        the conversion to jiffies is split into two operations.
             *        The first is on number of seconds and the second on
             *        the remaining millisecond precision.  This mimimizes
             *        overflow when the specified timeout is large and also
             *        keeps millisecond precision.
             ********************************************************/

            // Perform conversion if not infinite wait
            if (Timeout_ms != PLX_TIMEOUT_INFINITE)
            {
                // Get number of seconds
                Timeout_sec = Timeout_ms / 1000;

                // Store milliseconds precision
                Timeout_ms = Timeout_ms - (Timeout_sec * 1000);

                // Convert to jiffies
                Timeout_sec = Timeout_sec * HZ;
                Timeout_ms  = (Timeout_ms * HZ) / 1000;

                // Compute total jiffies
                Timeout_ms = Timeout_sec + Timeout_ms;
            }

            // Timeout parameter is signed and can't be negative
            if ((signed long)Timeout_ms < 0)
            {
                // Shift out negative bit
                Timeout_ms = Timeout_ms >> 1;
            }

            // Increment number of sleeping threads
            atomic_inc( &pWaitObject->SleepCount );

            do
            {
                // Wait for interrupt event
                Wait_rc =
                    wait_event_interruptible_timeout(
                        pWaitObject->WaitQueue,
                        (pWaitObject->state != PLX_STATE_WAITING),
                        Timeout_ms
                        );
            }
            while ((Wait_rc == 0) && (Timeout_ms == PLX_TIMEOUT_INFINITE));

            if (Wait_rc > 0)
            {
                // Condition met or interrupt occurred
                DebugPrintf(("Interrupt wait object awakened\n"));
                rc = PLX_STATUS_OK;
            }
            else if (Wait_rc == 0)
            {
                // Timeout reached
                DebugPrintf(("Timeout waiting for interrupt\n"));
                rc = PLX_STATUS_TIMEOUT;
            }
            else
            {
                // Interrupted by a signal
                DebugPrintf(("Interrupt wait object interrupted by signal or error\n"));
                rc = PLX_STATUS_CANCELED;
            }

            // If object is in triggered state, rest to waiting state
            if (pWaitObject->state == PLX_STATE_TRIGGERED)
            {
                pWaitObject->state = PLX_STATE_WAITING;
            }

            // Decrement number of sleeping threads
            atomic_dec( &pWaitObject->SleepCount );

            return rc;
        }

        // Jump to next item in the list
        pEntry = pEntry->next;
    }

    spin_unlock_irqrestore(
        &(pdx->Lock_WaitObjectsList),
        flags
        );

    DebugPrintf((
        "Interrupt wait object (%p) not found or previously canceled\n",
        pUserWaitObject
        ));

    // Object not found at this point
    return PLX_STATUS_FAILED;
}




/******************************************************************************
 *
 * Function   :  PlxNotificationStatus
 *
 * Description:  Returns the interrupt(s) that have caused notification events
 *
 *****************************************************************************/
PLX_STATUS
PlxNotificationStatus(
    DEVICE_EXTENSION *pdx,
    VOID             *pUserWaitObject,
    PLX_INTERRUPT    *pPlxIntr
    )
{
    unsigned long       flags;
    struct list_head   *pEntry;
    PLX_WAIT_OBJECT    *pWaitObject;
    PLX_INTERRUPT_DATA  IntData;


    spin_lock_irqsave( &(pdx->Lock_WaitObjectsList), flags );

    pEntry = pdx->List_WaitObjects.next;

    // Traverse list to find the desired list object
    while (pEntry != &(pdx->List_WaitObjects))
    {
        // Get the object
        pWaitObject =
            list_entry(
                pEntry,
                PLX_WAIT_OBJECT,
                ListEntry
                );

        // Check if desired object
        if (pWaitObject == pUserWaitObject)
        {
            // Copy the interrupt sources
            IntData.Source_Ints     = pWaitObject->Source_Ints;
            IntData.Source_Doorbell = pWaitObject->Source_Doorbell;

            // Reset interrupt sources
            pWaitObject->Source_Ints     = INTR_TYPE_NONE;
            pWaitObject->Source_Doorbell = 0;

            spin_unlock_irqrestore( &(pdx->Lock_WaitObjectsList), flags );

            DebugPrintf((
                "Return status for interrupt wait object (%p)...\n",
                pWaitObject
                ));

            // Set triggered interrupts
            RtlZeroMemory( pPlxIntr, sizeof(PLX_INTERRUPT) );

            pPlxIntr->Doorbell = IntData.Source_Doorbell;

            if (IntData.Source_Ints & INTR_TYPE_LE_CORRECTABLE)
            {
                pPlxIntr->NTV_LE_Correctable = 1;
            }

            if (IntData.Source_Ints & INTR_TYPE_LE_UNCORRECTABLE)
            {
                pPlxIntr->NTV_LE_Uncorrectable = 1;
            }

            if (IntData.Source_Ints & INTR_TYPE_LE_LINK_STATE_CHANGE)
            {
                pPlxIntr->NTV_LE_LinkStateChange = 1;
            }

            if (IntData.Source_Ints & INTR_TYPE_LE_UNCORR_ERR_MSG)
            {
                pPlxIntr->NTV_LE_UncorrErrorMsg = 1;
            }

            return PLX_STATUS_OK;
        }

        // Jump to next item in the list
        pEntry = pEntry->next;
    }

    spin_unlock_irqrestore( &(pdx->Lock_WaitObjectsList), flags );

    return PLX_STATUS_FAILED;
}




/*******************************************************************************
 *
 * Function   :  PlxNotificationCancel
 *
 * Description:  Cancels a registered notification event
 *
 ******************************************************************************/
PLX_STATUS
PlxNotificationCancel(
    DEVICE_EXTENSION *pdx,
    VOID             *pUserWaitObject,
    VOID             *pOwner
    )
{
    U32               LoopCount;
    BOOLEAN           bRemove;
    unsigned long     flags;
    struct list_head *pEntry;
    PLX_WAIT_OBJECT  *pWaitObject;


    spin_lock_irqsave(
        &(pdx->Lock_WaitObjectsList),
        flags
        );

    pEntry = pdx->List_WaitObjects.next;

    // Find the object and remove it
    while (pEntry != &(pdx->List_WaitObjects))
    {
        // Get the object
        pWaitObject =
            list_entry(
                pEntry,
                PLX_WAIT_OBJECT,
                ListEntry
                );

        // Default to not remove
        bRemove = FALSE;

        // Determine if object should be removed
        if (pOwner == pWaitObject->pOwner)
        {
            if (pUserWaitObject == NULL)
            {
                bRemove = TRUE;
            }
            else if (pWaitObject == pUserWaitObject)
            {
                bRemove = TRUE;
            }
        }

        // Remove object
        if (bRemove)
        {
            DebugPrintf((
                "Remove interrupt wait object (%p)...\n",
                pWaitObject
                ));

            // Remove the object from the list
            list_del( pEntry );

            spin_unlock_irqrestore(
                &(pdx->Lock_WaitObjectsList),
                flags
                );

            // Set loop count
            LoopCount = 20;

            // Wake-up processes if wait object is pending
            if (atomic_read(&pWaitObject->SleepCount) != 0)
            {
                DebugPrintf(("Wait object pending in another thread, forcing wake up\n"));

                // Mark object for deletion
                pWaitObject->state = PLX_STATE_MARKED_FOR_DELETE;

                // Wake-up any process waiting on the object
                wake_up_interruptible( &(pWaitObject->WaitQueue) );

                do
                {
                    // Set current task as uninterruptible
                    set_current_state(TASK_UNINTERRUPTIBLE);

                    // Relieve timeslice to allow pending thread to wake up
                    schedule_timeout( Plx_ms_to_jiffies( 10 ) );

                    // Decrement counter
                    LoopCount--;
                }
                while (LoopCount && (atomic_read(&pWaitObject->SleepCount) != 0));
            }

            if (LoopCount == 0)
            {
                DebugPrintf(("ERROR: Timeout waiting for pending thread, unable to free wait object\n"));
            }
            else
            {
                // Release the list object
                kfree( pWaitObject );
            }

            // Return if removing only a specific object
            if (pUserWaitObject != NULL)
            {
                return PLX_STATUS_OK;
            }

            // Reset to beginning of list
            spin_lock_irqsave(
                &(pdx->Lock_WaitObjectsList),
                flags
                );

            pEntry = pdx->List_WaitObjects.next;
        }
        else
        {
            // Jump to next item in the list
            pEntry = pEntry->next;
        }
    }

    spin_unlock_irqrestore(
        &(pdx->Lock_WaitObjectsList),
        flags
        );

    return PLX_STATUS_FAILED;
}




/******************************************************************************
 *
 * Function   :  PlxNtReqIdProbe
 *
 * Description:  Implements a procedure to determine the Requester ID for NT accesses
 *
 *****************************************************************************/
PLX_STATUS
PlxNtReqIdProbe(
    DEVICE_EXTENSION *pdx,
    BOOLEAN           bReadTlp,
    U16              *pReqId
    )
{
    U8   i;
    U16  Offset_CapPcie;
    U16  Offset_CapAer;
    U32 *pBarVa;
    U32  RegValue;
    U32  RegAerMask;
    U32  RegPciCommand;
    U32  RegPcieCapCsr;
    U32  RegAerSeverity;
    U32  WriteValue;


    /*********************************************************
     * This function attempts to detect the PCIe Requester ID
     * of the Host CPU.  The ReqID must be added to the NT
     * ReqID LUT so that it accepts TLPs from that requester.
     *
     * On most systems, the ReqID for both reads & writes is
     * the same. On many newer chipsets, the ReqID will be
     * different between read & write TLPs.  In general, the
     * ReqID will be the Root Complex (0,0,0) and/or the
     * upper-most parent root of the PLX switch, which will
     * be a PCIe Root Port type device.
     *
     * The basic algorithm is listed below.  The premise is to
     * disable access to the PLX chip and issue a TLP, which
     * should trigger an uncorrectable error and log the TLP.
     *
     *   - Clear any PCIe & AER errors
     *   - Setup PLX device to generate AER errors (but mask PCIe msg)
     *   - Disable accesses to the PLX device
     *   - Issue a dummy memory read or write to a PLX register
     *   - Restore settings
     *   - Capture ReqID from the AER TLP header log
     ********************************************************/

    // Default to Req ID of 0
    *pReqId = 0;

    // For newer PLX chips, use the built-in feature for Read ReqID
    if (bReadTlp &&
        ((pdx->Key.PlxFamily == PLX_FAMILY_DRACO_2) ||
         (pdx->Key.PlxFamily == PLX_FAMILY_CAPELLA_1) ||
         (pdx->Key.PlxFamily == PLX_FAMILY_CAPELLA_2)))
    {
        RegValue =
            PlxRegisterRead(
                pdx,
                0xC90,
                NULL,
                TRUE        // Adjust for port
                );

        // ReqID in bits [15:0]
        *pReqId = (U16)RegValue;
        goto _Exit_PlxNtReqIdProbe;
    }

    // 8500 series NT-Virtual side
    if (((pdx->Key.PlxChip & 0xFF00) == 0x8500) &&
         (pdx->Key.PlxPortType == PLX_SPEC_PORT_NT_VIRTUAL))
    {
        // Search for a valid BAR space to access
        i = 2;
        do
        {
            pBarVa = (U32*)pdx->PciBar[i].pVa;
            i++;
        }
        while ((i < 6) && (pBarVa == NULL));
    }
    else
    {
        // Always use BAR 0 for register access
        pBarVa = (U32*)pdx->PciBar[0].pVa;
    }

    if (pBarVa == NULL)
    {
        DebugPrintf(("ERROR - No valid BAR space available to use (8500 NT-Virtual)\n"));
        return PLX_STATUS_UNSUPPORTED;
    }

    // Set default base offsets for PCIe & AER capabilities
    Offset_CapPcie = 0x68;
    Offset_CapAer  = 0xFB4;

    // Get PCIe Device Status/Control & disable error reporting to host ([3:0])
    PLX_PCI_REG_READ( pdx, (U16)(Offset_CapPcie + 0x8), &RegPcieCapCsr );
    RegPcieCapCsr |= (0xF << 16);       // Clear any error status bits [19:16]
    PLX_PCI_REG_WRITE( pdx, (U16)(Offset_CapPcie + 0x8), RegPcieCapCsr & ~(0xF << 0) );

    // Clear any pending AER errors ([20:16,12,5,4]) to ensure error TLP logging
    WriteValue = (0x1F << 16) | (1 << 12) | (1 << 5) | (1 << 4);
    PLX_PCI_REG_WRITE( pdx, (U16)(Offset_CapAer + 0x4), WriteValue );

    // Mask all AER errors except for UR [20]
    WriteValue = (0xF << 16) | (1 << 12) | (1 << 5) | (1 << 4);
    PLX_PCI_REG_READ( pdx, (U16)(Offset_CapAer + 0x8), &RegAerMask );
    PLX_PCI_REG_WRITE( pdx, (U16)(Offset_CapAer + 0x8), WriteValue );

    // Set severity for UR errors ([20]). Must be Fatal for read TLPs or no AER logging
    PLX_PCI_REG_READ( pdx, (U16)(Offset_CapAer + 0xC), &RegAerSeverity );
    if (bReadTlp)
    {
        PLX_PCI_REG_WRITE( pdx, (U16)(Offset_CapAer + 0xC), RegAerSeverity | (1 << 20) );
    }
    else
    {
        PLX_PCI_REG_WRITE( pdx, (U16)(Offset_CapAer + 0xC), RegAerSeverity & ~(1 << 20) );
    }

    // Disable PCI accesses [2:0] & SERR enable [8] in PCI command/status
    PLX_PCI_REG_READ( pdx, 0x04, &RegPciCommand );
    RegPciCommand |= (0x1F << 27);      // Clear any PCI error status ([31:27])
    PLX_PCI_REG_WRITE( pdx, 0x04, (RegPciCommand & ~0x107) );

    /*********************************************************
     * Issue a TLP read or write to a BAR space
     ********************************************************/
    WriteValue = 0x12345678;
    if (bReadTlp)
    {
        PHYS_MEM_READ_32( pBarVa );
    }
    else
    {
        PHYS_MEM_WRITE_32( pBarVa, WriteValue );
    }

    // Restore modified registers
    PLX_PCI_REG_WRITE( pdx, 0x04, RegPciCommand );
    PLX_PCI_REG_WRITE( pdx, (U16)(Offset_CapAer + 0x8), RegAerMask );
    PLX_PCI_REG_WRITE( pdx, (U16)(Offset_CapAer + 0xC), RegAerSeverity );

    // Verify an AER UR error was flagged
    PLX_PCI_REG_READ( pdx, (U16)(Offset_CapAer + 0x4), &RegValue );
    if ((RegValue & (1 << 20)) == 0)
    {
        DebugPrintf(("ERROR - ReqID probe failed, unable to capture error TLP\n"));
        return PLX_STATUS_FAILED;
    }

    // Check if AER header is valid from First Error Pointer ([4:0])
    PLX_PCI_REG_READ( pdx, (U16)(Offset_CapAer + 0x18), &RegValue );
    if ((RegValue & 0x1F) != 20)
    {
        DebugPrintf((
            "WARNING - Error Pointer reports log is for bit %d, not 20 (UR). Log may be invalid\n",
            (RegValue & 0x1F)
            ));
    }

    // Get the AER logged TLP DWord 1, which contains Req ID
    PLX_PCI_REG_READ( pdx, (U16)(Offset_CapAer + 0x20), &RegValue );

    // Clear UR status
    PLX_PCI_REG_WRITE( pdx, (U16)(Offset_CapAer + 0x4), (1 << 20) );

    // Get Requester ID (DW1[31:16])
    *pReqId = (U16)(RegValue >> 16);

_Exit_PlxNtReqIdProbe:

    DebugPrintf((
        "Probed %s ReqID = %04X [%02x:%02x.%x]\n",
        (bReadTlp) ? "Read" : "Write", *pReqId,
        (*pReqId >> 8), (*pReqId >> 3) & 0x1F, (*pReqId & 0x7)
        ));

    return PLX_STATUS_OK;
}




/******************************************************************************
 *
 * Function   :  PlxNtLutProperties
 *
 * Description:  Returns the properties of an NT LUT entry
 *
 *****************************************************************************/
PLX_STATUS
PlxNtLutProperties(
    DEVICE_EXTENSION *pdx,
    U16               LutIndex,
    U16              *pReqId,
    U32              *pFlags,
    BOOLEAN          *pbEnabled
    )
{
    // Not yet implemented
    *pReqId    = 0;
    *pFlags    = PLX_NT_LUT_FLAG_NONE;
    *pbEnabled = FALSE;

    return PLX_STATUS_UNSUPPORTED;
}




/******************************************************************************
 *
 * Function   :  PlxNtLutAdd
 *
 * Description:  Adds a new entry to the NT LUT with specified requester ID & flags
 *
 *****************************************************************************/
PLX_STATUS
PlxNtLutAdd(
    DEVICE_EXTENSION *pdx,
    U16              *pLutIndex,
    U16               ReqId,
    U32               flags,
    VOID             *pOwner
    )
{
    U8  LutShift;
    U8  LutWidth;
    U8  ReqIdShift;
    U8  bExists;
    U8  bLegacyLUT;
    U16 index;
    U16 IndexToUse;
    U16 offset;
    U16 LutSize;
    U16 BaseOffset;
    U32 LutTemp;
    U32 LutValue;
    U32 LutEnMask;
    U32 LutNsMask;
    U32 ReqIdMask;


    // Flag entry is not already in LUT
    bExists = FALSE;

    // Set the starting offset for the LUT
    if ( (pdx->Key.PlxFamily == PLX_FAMILY_CAPELLA_1) ||
         (pdx->Key.PlxFamily == PLX_FAMILY_CAPELLA_2) )
    {
        bLegacyLUT = FALSE;             // Indexed LUT access method
        LutWidth   = sizeof(U32);       // 32-bit LUT entry size
        LutSize    = 256;               // Max of 256 entries
        LutEnMask  = ((U32)1 << 0);     // Enable is bit 0
        LutNsMask  = ((U32)1 << 1);     // No Snoop flag is bit 1
        ReqIdShift = 4;                 // ReqID in [19:4]
        ReqIdMask  = 0xFFFF;            // Full 16-bit ReqID stored
        BaseOffset = 0xC98;
    }
    else
    {
        // Set defaults
        bLegacyLUT = TRUE;              // Legacy LUT access
        LutWidth   = sizeof(U16);       // 16-bit LUT entry size
        LutSize    = 32;                // Max of 32 entries
        LutEnMask  = ((U32)1 << 0);     // Enable is bit 0
        LutNsMask  = ((U32)1 << 1);     // No Snoop flag is bit 1
        ReqIdShift = 0;                 // ReqID in [15:0]
        ReqIdMask  = 0xFFFF;            // Full 16-bit ReqID stored

        if (pdx->Key.PlxPortType == PLX_SPEC_PORT_NT_LINK)
        {
            BaseOffset = 0xDB4;
        }
        else
        {
            BaseOffset = 0xD94;

            // For NT Virtual side on older devices, LUT is 32-bit wide
            if (((pdx->Key.PlxChip & 0xFF00) == 0x8500) ||
                ((pdx->Key.PlxChip & 0xFF00) == 0x8600))
            {
                LutWidth  = sizeof(U32);    // 32-bit LUT entry size
                LutSize   = 8;              // Max of 8 entries
                LutEnMask = ((U32)1 << 31); // Enable is bit 31
                LutNsMask = ((U32)1 << 30); // No Snoop flag is bit 1
                ReqIdMask  = 0xFFFC;        // ReqID [1:0] are LUT flags
            }
        }
    }

    // Verify index
    if (*pLutIndex != (U16)-1)
    {
        if (*pLutIndex >= LutSize)
        {
            return PLX_STATUS_INVALID_DATA;
        }
    }

    // Set initial index to use
    IndexToUse = *pLutIndex;

    // If requested, find first available entry
    if (IndexToUse == (U16)-1)
    {
        for (index=0; index < LutSize; index++)
        {
            // Set next offset to read
            offset = BaseOffset;
            if (bLegacyLUT)
            {
                // Get 32-bit register offset, accounting for 16-bit entries
                offset += ( (IndexToUse * LutWidth) & ~(U32)0x3 );
            }

            // For newer LUT, set desired LUT index to read (C9Ch[7:0])
            if (bLegacyLUT == FALSE)
            {
                PlxRegisterWrite( pdx, 0xC9C, index, TRUE );
            }

            // Get LUT
            LutValue = PlxRegisterRead( pdx, offset, NULL, TRUE );

            // Determine if LUT is in use or matches new ReqID
            if (LutWidth == sizeof(U16))
            {
                // Get current 16-bit entry (odd entries in [31:16])
                LutValue >>= ( 16 * (index & (1 << 0)) );
                LutValue  &= 0xFFFF;
            }

            // If enabled, check for a match
            if (LutValue & LutEnMask)
            {
                // Compare ReqID with ID in LUT
                if ( (ReqId & ReqIdMask) ==
                           ((LutValue >> ReqIdShift) & ReqIdMask) )
                {
                    IndexToUse = index;
                    bExists    = TRUE;
                }
            }
            else
            {
                // Entry is available so use it if not found yet
                if (IndexToUse == (U16)-1)
                {
                    IndexToUse = index;
                }
            }
        }
    }

    // Error if no index available
    if (IndexToUse == (U16)-1)
    {
        return PLX_STATUS_INSUFFICIENT_RES;
    }

    // Return index used
    *pLutIndex = IndexToUse;

    if (bExists)
    {
        DebugPrintf((
            "Req ID (%04X) already exists in LUT #%d, skipping update\n",
            ReqId, IndexToUse
            ));
        return PLX_STATUS_OK;
    }

    // Set offset to desired LUT entry
    offset = BaseOffset;
    if (bLegacyLUT)
    {
        // Get 32-bit register offset, accounting for 16-bit entries
        offset += ( (IndexToUse * LutWidth) & ~(U32)0x3 );
    }

    // Build LUT entry
    LutValue =
        (LutEnMask |                            // LUT entry enable
        ((ReqId & ReqIdMask) << ReqIdShift) );  // Requester ID

    // Enable No_Snoop for entry if requested
    if ( (flags & PLX_NT_LUT_FLAG_NO_SNOOP) &&
         ((pdx->Key.PlxChip & 0xFF00) != 0x8500) )
    {
        LutValue |= LutNsMask;
    }

    // For newer LUT, set entry number in [31:24]
    if (bLegacyLUT == FALSE)
    {
        LutValue |= ((U32)IndexToUse << 24);
    }

    // For 16-bit, build full 32-bit value, preserving other entry
    if (LutWidth == sizeof(U16))
    {
        // Set entry position in 32-bit register
        LutShift = ( 16 * (IndexToUse & (1 << 0)) );

        // Determine mask for entry to use
        LutEnMask = ( (U32)0xFFFF << LutShift );

        // Get current LUT entries
        LutTemp = PlxRegisterRead( pdx, offset, NULL, TRUE );

        // Clear current entry ([31:16] if odd entry or [15:0] if even)
        LutTemp &= ~LutEnMask;

        // Place entry in correct portion
        LutValue <<= LutShift;

        // Restore saved entry
        LutValue |= LutTemp;
    }

    // Update the LUT entry
    PlxRegisterWrite(
        pdx,
        offset,
        LutValue,
        TRUE
        );

    // For 8500 series, LUT is globally set at 660h[24] in Port 0
    if ((pdx->Key.PlxChip & 0xFF00) == 0x8500)
    {
        LutValue = PlxRegisterRead( pdx, 0x660, NULL, FALSE );

        // Set or clear No_Snoop to match current LUT entry
        if (flags & PLX_NT_LUT_FLAG_NO_SNOOP)
        {
            PlxRegisterWrite( pdx, 0x660, LutValue | (1 << 24), FALSE );
        }
        else
        {
            PlxRegisterWrite( pdx, 0x660, LutValue & ~(1 << 24), FALSE );
        }
    }

    DebugPrintf((
        "Added Req ID (%04X) to LUT #%d (No_Snoop=%s)\n",
        ReqId, IndexToUse,
        (flags & PLX_NT_LUT_FLAG_NO_SNOOP) ? "ON" : "OFF"
        ));

    return PLX_STATUS_OK;
}




/******************************************************************************
 *
 * Function   :  PlxNtLutDisable
 *
 * Description:  Disables the specified NT LUT entry
 *
 *****************************************************************************/
PLX_STATUS
PlxNtLutDisable(
    DEVICE_EXTENSION *pdx,
    U16               LutIndex,
    VOID             *pOwner
    )
{
    // Not yet implemented
    return PLX_STATUS_UNSUPPORTED;
}
