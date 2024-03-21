/*******************************************************************************
 * Copyright 2013-2022 Broadcom Inc
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
 *     PlxApiDirect.c
 *
 * Description:
 *
 *     PLX API functions at API level for direct connect interfaces
 *
 * Revision History:
 *
 *     11-01-19: PCI/PCIe SDK v8.10
 *
 ******************************************************************************/


#include <stdlib.h>     // For malloc()/free()
#include <string.h>     // For memset()/memcpy()
#include "PexApi.h"
#include "PciRegs.h"
#include "PlxApiDebug.h"
#include "PlxApiDirect.h"
#include "I2cAaUsb.h"
#include "MdioSpliceUsb.h"
#include "SdbComPort.h"
#include "SpiFlash.h"




/*******************************************************************************
 *
 * Function   : PlxDir_ChipTypeGet
 *
 * Description: Returns PLX chip type and revision
 *
 ******************************************************************************/
PLX_STATUS
PlxDir_ChipTypeGet(
    PLX_DEVICE_OBJECT *pDevice,
    U16               *pChipType,
    U8                *pRevision
    )
{
    *pChipType = pDevice->Key.PlxChip;
    *pRevision = pDevice->Key.PlxRevision;

    DebugPrintf((
        "Device %04X_%04X = %04X rev %02X\n",
        pDevice->Key.DeviceId, pDevice->Key.VendorId,
        *pChipType, *pRevision
        ));

    return PLX_STATUS_OK;
}




/*******************************************************************************
 *
 * Function   : PlxDir_ChipTypeSet
 *
 * Description: Sets the PLX chip type dynamically
 *
 ******************************************************************************/
PLX_STATUS
PlxDir_ChipTypeSet(
    PLX_DEVICE_OBJECT *pDevice,
    U16                ChipType,
    U8                 Revision
    )
{
    PLX_PORT_PROP portProp;


    // Attempt auto-detection if requested
    if (ChipType == 0)
    {
        PlxDir_ChipTypeDetect( pDevice );
        ChipType = pDevice->Key.PlxChip;
    }

    // Verify chip type
    switch (ChipType & 0xFF00)
    {
        case 0:         // Used to clear chip type
        case 0x2300:
        case 0x3300:
        case 0x8500:
        case 0x8600:
        case 0x8700:
        case 0x9700:
            // Get port properties to ensure upstream node
            PlxDir_GetPortProperties( pDevice, &portProp );

            if (portProp.PortType != PLX_PORT_UPSTREAM)
            {
                DebugPrintf(("ERROR - Chip type may only be changed on upstream port\n"));
                return PLX_STATUS_UNSUPPORTED;
            }
            break;

        default:
            DebugPrintf((
                "ERROR - Invalid or unsupported chip type (%04X)\n",
                ChipType
                ));
            return PLX_STATUS_INVALID_DATA;
    }

    // Set the new chip type
    pDevice->Key.PlxChip = ChipType;

    // Check if we should update the revision or use the default
    if ((Revision == (U8)-1) || (Revision == 0))
    {
        // Attempt to detect Revision ID
        PlxDir_ChipRevisionDetect( pDevice );
    }
    else
    {
        pDevice->Key.PlxRevision = Revision;
    }

    DebugPrintf((
        "Set device (%04X_%04X) type to %04X rev %02X\n",
        pDevice->Key.DeviceId, pDevice->Key.VendorId,
        pDevice->Key.PlxChip, pDevice->Key.PlxRevision
        ));

    return PLX_STATUS_OK;
}




/*******************************************************************************
 *
 * Function   : PlxDir_GetPortProperties
 *
 * Description: Returns the current port information and status
 *
 ******************************************************************************/
PLX_STATUS
PlxDir_GetPortProperties(
    PLX_DEVICE_OBJECT *pDevice,
    PLX_PORT_PROP     *pPortProp
    )
{
    U16 MaxSize;
    U16 Offset_PcieCap;
    U32 regVal;


    // Set default properties
    RtlZeroMemory( pPortProp, sizeof(PLX_PORT_PROP) );

    /**********************************************************
     * In Atlas iSSW mode, management port does not report PCIe
     * capability, so fill in reasonable values
     *********************************************************/
    if ( ((pDevice->Key.PlxFamily == PLX_FAMILY_ATLAS) ||
          (pDevice->Key.PlxFamily == PLX_FAMILY_ATLAS_2) ||
          (pDevice->Key.PlxFamily == PLX_FAMILY_ATLAS2_LLC)) &&
         (pDevice->Key.PlxPort == PLX_FLAG_PORT_INT_MGMT) )
    {
        pPortProp->PortType            = PLX_PORT_UPSTREAM;
        pPortProp->PortNumber          = PLX_FLAG_PORT_INT_MGMT;
        pPortProp->MaxReadReqSize      = 512;
        pPortProp->MaxPayloadSize      = 256;
        pPortProp->MaxPayloadSupported = 256;
        pPortProp->LinkSpeed           = 4;
        pPortProp->MaxLinkSpeed        = 4;
        pPortProp->LinkWidth           = 16;
        pPortProp->MaxLinkWidth        = 16;
        return PLX_STATUS_OK;
    }

    // Get the offset of the PCI Express capability
    Offset_PcieCap =
        PlxDir_PciFindCapability(
            pDevice,
            PCI_CAP_ID_PCI_EXPRESS,
            FALSE,
            0
            );
    if (Offset_PcieCap == 0)
    {
        DebugPrintf((
            "[D%d %02X:%02X.%X] Does not contain PCIe capability\n",
            pDevice->Key.domain, pDevice->Key.bus, pDevice->Key.slot, pDevice->Key.function
            ));

        // Mark device as non-PCIe
        pPortProp->bNonPcieDevice = TRUE;
        pPortProp->PortType       = PLX_PORT_UNKNOWN;
        return PLX_STATUS_OK;
    }

    // Get PCIe Capability
    PLX_PCI_REG_READ(
        pDevice,
        Offset_PcieCap,
        &regVal
        );

    // Get port type
    pPortProp->PortType = (U8)((regVal >> 20) & 0xF);

    // Get PCIe Device Capabilities
    PLX_PCI_REG_READ(
        pDevice,
        Offset_PcieCap + 0x04,
        &regVal
        );

    // Get max payload size supported field
    MaxSize = (U8)(regVal >> 0) & 0x7;

    // Set max payload size (=128 * (2 ^ MaxPaySizeField))
    pPortProp->MaxPayloadSupported = PCIE_MPS_MRR_TO_BYTES( MaxSize );

    // Get PCIe Device Control
    PLX_PCI_REG_READ(
        pDevice,
        Offset_PcieCap + 0x08,
        &regVal
        );

    // Get max payload size field
    MaxSize = (U8)(regVal >> 5) & 0x7;

    // Set max payload size (=128 * (2 ^ MaxPaySizeField))
    pPortProp->MaxPayloadSize = PCIE_MPS_MRR_TO_BYTES( MaxSize );

    // Get max read request size field
    MaxSize = (U8)(regVal >> 12) & 0x7;

    // Set max read request size (=128 * (2 ^ MaxReadReqSizeField))
    pPortProp->MaxReadReqSize = PCIE_MPS_MRR_TO_BYTES( MaxSize );

    // Get PCIe Link Capabilities
    PLX_PCI_REG_READ(
        pDevice,
        Offset_PcieCap + 0x0C,
        &regVal
        );

    // Get port number
    pPortProp->PortNumber = (U8)((regVal >> 24) & 0xFF);

    // Get max link width
    pPortProp->MaxLinkWidth = (U8)((regVal >> 4) & 0x3F);

    // Get max link speed
    pPortProp->MaxLinkSpeed = (U8)((regVal >> 0) & 0xF);

    // Get PCIe Link Status/Control
    PLX_PCI_REG_READ(
        pDevice,
        Offset_PcieCap + 0x10,
        &regVal
        );

    // Get link width
    pPortProp->LinkWidth = (U8)((regVal >> 20) & 0x3F);

    // Get link speed
    pPortProp->LinkSpeed = (U8)((regVal >> 16) & 0xF);

    /**********************************************************
     * In MIRA 3300 Enhanced mode, link width for the DS port & USB EP
     * is incorrectly reported as x0. Override with Max Width.
     *********************************************************/
    if ((pDevice->Key.PlxFamily == PLX_FAMILY_MIRA) &&
        ((pDevice->Key.PlxChip & 0xFF00) == 0x3300) &&
        (pDevice->Key.DeviceMode == PLX_CHIP_MODE_STANDARD) &&
        (pPortProp->LinkWidth == 0))
    {
        DebugPrintf((
            "NOTE - Override reported link width (x%d) with Max width (x%d)\n",
            pPortProp->LinkWidth, pPortProp->MaxLinkWidth
            ));
        pPortProp->LinkWidth = pPortProp->MaxLinkWidth;
    }

    /**********************************************************
     * If using port bifurication, Draco 2 DS ports may report
     * incorrect port numbers. Override with PLX port number.
     *********************************************************/
    if ((pDevice->Key.PlxFamily == PLX_FAMILY_DRACO_2) &&
        (pPortProp->PortType == PLX_PORT_DOWNSTREAM) &&
        (pPortProp->PortNumber != pDevice->Key.PlxPort))
    {
        DebugPrintf((
            "NOTE - Override reported port num (%d) with probed port num (%d)\n",
            pPortProp->PortNumber, pDevice->Key.PlxPort
            ));
        pPortProp->PortNumber = pDevice->Key.PlxPort;
    }

    DebugPrintf((
        "P=%d T=%d MPS=%d/%d MRR=%d L=G%dx%d/G%dx%d\n",
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
 * Function   : PlxDir_PciBarProperties
 *
 * Description: Returns the properties of a PCI BAR space
 *
 ******************************************************************************/
PLX_STATUS
PlxDir_PciBarProperties(
    PLX_DEVICE_OBJECT *pDevice,
    U8                 BarIndex,
    PLX_PCI_BAR_PROP  *pBarProperties
    )
{
    U32 PciBar;
    U32 Size;
    U32 PciHeaderType;


    // Get PCI header type
    PLX_PCI_REG_READ(
        pDevice,
        PCI_REG_HDR_CACHE_LN,
        &PciHeaderType
        );

    PciHeaderType = (U8)((PciHeaderType >> 16) & 0x7F);

    // Verify BAR index
    switch (PciHeaderType)
    {
        case PCI_HDR_TYPE_0:
            if ((BarIndex != 0) && (BarIndex > 5))
            {
                return PLX_STATUS_INVALID_ACCESS;
            }
            break;

        case PCI_HDR_TYPE_1:
            if ((BarIndex != 0) && (BarIndex != 1))
            {
                DebugPrintf(("BAR %d does not exist on PCI type 1 header\n", BarIndex));
                return PLX_STATUS_INVALID_ACCESS;
            }
            break;

        default:
            return PLX_STATUS_INVALID_ACCESS;
    }

    // Get BAR properties if not probed yet
    if (pDevice->PciBar[BarIndex].BarValue == 0)
    {
        // Read PCI BAR
        PLX_PCI_REG_READ(
            pDevice,
            PCI_REG_BAR_0 + (BarIndex * sizeof(U32)),
            &PciBar
            );

        // Query BAR range
        PLX_PCI_REG_WRITE(
            pDevice,
            PCI_REG_BAR_0 + (BarIndex * sizeof(U32)),
            (U32)-1
            );

        // Read size
        PLX_PCI_REG_READ(
            pDevice,
            PCI_REG_BAR_0 + (BarIndex * sizeof(U32)),
            &Size
            );

        // Restore BAR
        PLX_PCI_REG_WRITE(
            pDevice,
            PCI_REG_BAR_0 + (BarIndex * sizeof(U32)),
            PciBar
            );

        // If upper 32-bit of 64-bit BAR, all bits will be set, which is not supported
        if (Size == (U32)-1)
        {
            Size = 0;
        }

        // Store BAR properties
        pDevice->PciBar[BarIndex].BarValue = PciBar;

        // Determine BAR address & size based on space type
        if (PciBar & (1 << 0))
        {
            // Some devices may not report upper 16-bits as FFFF
            Size |= 0xFFFF0000;

            pDevice->PciBar[BarIndex].Physical = PciBar & ~0x3;
            pDevice->PciBar[BarIndex].Size     = (~(Size & ~0x3)) + 1;
            pDevice->PciBar[BarIndex].Flags    = PLX_BAR_FLAG_IO;
        }
        else
        {
            pDevice->PciBar[BarIndex].Physical = PciBar & ~0xF;
            pDevice->PciBar[BarIndex].Size     = (~(Size & ~0xF)) + 1;
            pDevice->PciBar[BarIndex].Flags    = PLX_BAR_FLAG_MEM;
        }

        // Set BAR flags
        if (pDevice->PciBar[BarIndex].Flags & PLX_BAR_FLAG_MEM)
        {
            if (((PciBar >> 1) & 0x3) == 0)
            {
                pDevice->PciBar[BarIndex].Flags |= PLX_BAR_FLAG_32_BIT;
            }
            else if (((PciBar >> 1) & 0x3) == 1)
            {
                pDevice->PciBar[BarIndex].Flags |= PLX_BAR_FLAG_BELOW_1MB;
            }
            else if (((PciBar >> 1) & 0x3) == 2)
            {
                pDevice->PciBar[BarIndex].Flags |= PLX_BAR_FLAG_64_BIT;
            }

            if (PciBar & (1 << 3))
            {
                pDevice->PciBar[BarIndex].Flags |= PLX_BAR_FLAG_PREFETCHABLE;
            }
        }
    }

    // Return BAR properties
    *pBarProperties = pDevice->PciBar[BarIndex];

    // Display BAR properties if enabled
    if (pDevice->PciBar[BarIndex].Size == 0)
    {
        DebugPrintf(("BAR %d is disabled\n", BarIndex));
        return PLX_STATUS_OK;
    }

    DebugPrintf((
        "    PCI BAR %d: %08lX\n",
        BarIndex, (PLX_UINT_PTR)pDevice->PciBar[BarIndex].BarValue
        ));

    DebugPrintf((
        "    Phys Addr: %08lX\n",
        (PLX_UINT_PTR)pDevice->PciBar[BarIndex].Physical
        ));

    DebugPrintf((
        "    Size     : %08lX (%ld %s)\n",
        (PLX_UINT_PTR)pDevice->PciBar[BarIndex].Size,
        (PLX_UINT_PTR)pDevice->PciBar[BarIndex].Size > ((U64)1 << 30) ?
          (PLX_UINT_PTR)pDevice->PciBar[BarIndex].Size >> 30 :
          (PLX_UINT_PTR)pDevice->PciBar[BarIndex].Size > ((U64)1 << 20) ?
          (PLX_UINT_PTR)pDevice->PciBar[BarIndex].Size >> 20 :
          (PLX_UINT_PTR)pDevice->PciBar[BarIndex].Size > ((U64)1 << 10) ?
          (PLX_UINT_PTR)pDevice->PciBar[BarIndex].Size >> 10 :
          (PLX_UINT_PTR)pDevice->PciBar[BarIndex].Size,
        (PLX_UINT_PTR)pDevice->PciBar[BarIndex].Size > ((U64)1 << 30) ? "GB" :
          (PLX_UINT_PTR)pDevice->PciBar[BarIndex].Size > ((U64)1 << 20) ? "MB" :
          (PLX_UINT_PTR)pDevice->PciBar[BarIndex].Size > ((U64)1 << 20) ? "KB" : "B"
        ));

    DebugPrintf((
        "    Property : %sPrefetchable %d-bit\n",
        (pDevice->PciBar[BarIndex].Flags & PLX_BAR_FLAG_PREFETCHABLE) ? "" : "Non-",
        (pDevice->PciBar[BarIndex].Flags & PLX_BAR_FLAG_64_BIT) ? 64 : 32
        ));

    return PLX_STATUS_OK;
}




/*******************************************************************************
 *
 * Function   : PlxDir_SpiFlashPropGet
 *
 * Description: Queries the SPI flash to get its properties
 *
 ******************************************************************************/
PLX_STATUS
PlxDir_SpiFlashPropGet(
    PLX_DEVICE_OBJECT *PtrDev,
    U8                 ChipSel,
    PEX_SPI_OBJ       *PtrSpi
    )
{
    return Spi_FlashPropGet( PtrDev, ChipSel, PtrSpi );
}




/*******************************************************************************
 *
 * Function   : PlxDir_SpiFlashErase
 *
 * Description: Erase the flash, either all or sectors
 *
 ******************************************************************************/
PLX_STATUS
PlxDir_SpiFlashErase(
    PLX_DEVICE_OBJECT *PtrDev,
    PEX_SPI_OBJ       *PtrSpi,
    U32                StartOffset,
    U8                 BoolWaitComplete
    )
{
    return Spi_Erase( PtrDev, PtrSpi, StartOffset, BoolWaitComplete );
}




/*******************************************************************************
 *
 * Function   : PlxDir_SpiFlashReadBuffer
 *
 * Description: Reads data from the SPI flash at specified offset
 *
 ******************************************************************************/
PLX_STATUS
PlxDir_SpiFlashReadBuffer(
    PLX_DEVICE_OBJECT *PtrDev,
    PEX_SPI_OBJ       *PtrSpi,
    U32                StartOffset,
    U8                *PtrRxBuff,
    U32                SizeRx
    )
{
    return Spi_ReadBuffer( PtrDev, PtrSpi, StartOffset, PtrRxBuff, SizeRx );
}




/*******************************************************************************
 *
 * Function   : PlxDir_SpiFlashWriteBuffer
 *
 * Description: Writes data to the SPI flash at specified offset
 *
 ******************************************************************************/
PLX_STATUS
PlxDir_SpiFlashWriteBuffer(
    PLX_DEVICE_OBJECT *PtrDev,
    PEX_SPI_OBJ       *PtrSpi,
    U32                StartOffset,
    U8                *PtrTxBuff,
    U32                SizeTx
    )
{
    return Spi_WriteBuffer( PtrDev, PtrSpi, StartOffset, PtrTxBuff, SizeTx );
}




/*******************************************************************************
 *
 * Function   : PlxDir_SpiFlashGetStatus
 *
 * Description:
 *
 ******************************************************************************/
PLX_STATUS
PlxDir_SpiFlashGetStatus(
    PLX_DEVICE_OBJECT *PtrDev,
    PEX_SPI_OBJ       *PtrSpi
    )
{
    return Spi_GetStatus( PtrDev, PtrSpi );
}




/*******************************************************************************
 *
 * Function   : PlxDir_PerformanceInitializeProperties
 *
 * Description: Initializes the performance properties for a device
 *
 ******************************************************************************/
PLX_STATUS
PlxDir_PerformanceInitializeProperties(
    PLX_DEVICE_OBJECT *pDevice,
    PLX_PERF_PROP     *pPerfProp
    )
{
    U8            StnPortCount;
    PLX_PORT_PROP portProp;


    // Clear performance object
    RtlZeroMemory( pPerfProp, sizeof(PLX_PERF_PROP) );

    // Verify supported device & set number of ports per station
    switch (pDevice->Key.PlxFamily)
    {
        case PLX_FAMILY_DENEB:
        case PLX_FAMILY_CYGNUS:
        case PLX_FAMILY_MIRA:
        case PLX_FAMILY_CAPELLA_1:
        case PLX_FAMILY_CAPELLA_2:
            StnPortCount = 4;
            break;

        case PLX_FAMILY_SIRIUS:
        case PLX_FAMILY_ATLAS:
		case PLX_FAMILY_ATLAS_2:
		case PLX_FAMILY_ATLAS2_LLC:
            StnPortCount = 16;
            break;

        case PLX_FAMILY_DRACO_1:
        case PLX_FAMILY_DRACO_2:
            StnPortCount = 8;    // Device actually only uses 6 ports out of 8
            break;

        default:
            DebugPrintf(("ERROR - Unsupported PLX chip (%04X)\n", pDevice->Key.PlxChip));
            return PLX_STATUS_UNSUPPORTED;
    }

    // Get port properties
    PlxDir_GetPortProperties( pDevice, &portProp );

    if (portProp.PortNumber >= PERF_MAX_PORTS)
    {
        DebugPrintf(("ERROR - Port number exceeds maximum (%d)\n", (PERF_MAX_PORTS-1)));
        return PLX_STATUS_UNSUPPORTED;
    }

    // Store PLX chip family
    pPerfProp->PlxFamily = pDevice->Key.PlxFamily;

    // Store relevant port properties for later calculations
    pPerfProp->PortNumber = portProp.PortNumber;
    pPerfProp->LinkWidth  = portProp.LinkWidth;
    pPerfProp->LinkSpeed  = portProp.LinkSpeed;

    // Determine station and port number within station
    pPerfProp->Station     = (U8)(portProp.PortNumber / StnPortCount);
    pPerfProp->StationPort = (U8)(portProp.PortNumber % StnPortCount);

    return PLX_STATUS_OK;
}




/*******************************************************************************
 *
 * Function   : PlxDir_PerformanceMonitorControl
 *
 * Description: Controls PLX Performance Monitor
 *
 ******************************************************************************/
PLX_STATUS
PlxDir_PerformanceMonitorControl(
    PLX_DEVICE_OBJECT *pDevice,
    PLX_PERF_CMD       command
    )
{
    U8  Bit_EgressEn;
    U8  bStationBased;
    U8  bEgressAllPorts;
    U32 i;
    U32 offset;
    U32 pexBase;
    U32 regVal;
    U32 RegCommand;
    U32 StnCount;
    U32 StnPortCount;
    U32 Offset_Control;


    // Set default base offset to PCIe port registers
    pexBase = 0;

    // Verify supported device
    switch (pDevice->Key.PlxFamily)
    {
        case PLX_FAMILY_DENEB:
        case PLX_FAMILY_SIRIUS:
        case PLX_FAMILY_MIRA:
            Offset_Control = 0x568;

            // Offset changes for MIRA in legacy EP mode
            if ((pDevice->Key.PlxFamily == PLX_FAMILY_MIRA) &&
                (pDevice->Key.DeviceMode == PLX_CHIP_MODE_LEGACY_ADAPTER))
            {
                Offset_Control = 0x1568;
            }
            break;

        case PLX_FAMILY_CYGNUS:
        case PLX_FAMILY_DRACO_1:
        case PLX_FAMILY_DRACO_2:
        case PLX_FAMILY_CAPELLA_1:
        case PLX_FAMILY_CAPELLA_2:
            Offset_Control = 0x3E0;
            break;

        case PLX_FAMILY_ATLAS:
        case PLX_FAMILY_ATLAS_2:
        case PLX_FAMILY_ATLAS2_LLC:
            pexBase        = ATLAS_PEX_REGS_BASE_OFFSET;
            Offset_Control = 0x3E0;
            break;

        default:
            DebugPrintf(("ERROR - Unsupported PLX chip (%04X)\n", pDevice->Key.PlxChip));
            return PLX_STATUS_UNSUPPORTED;
    }

    switch (command)
    {
        case PLX_PERF_CMD_START:
            DebugPrintf(("Reset & enable monitor with infinite sampling\n"));
            RegCommand = ((U32)1 << 31) | ((U32)1 << 30) | ((U32)1 << 28) | ((U32)1 << 27);
            break;

        case PLX_PERF_CMD_STOP:
            DebugPrintf(("Reset & disable monitor\n"));
            RegCommand = ((U32)1 << 30);
            break;

        default:
            return PLX_STATUS_INVALID_DATA;
    }

    // Added to avoid compiler warning
    Bit_EgressEn = 0;
    StnCount     = 0;
    StnPortCount = 0;

    // Default to single station access
    bStationBased   = FALSE;
    bEgressAllPorts = FALSE;

    // Set control offset & enable/disable counters in stations
    switch (pDevice->Key.PlxFamily)
    {
        case PLX_FAMILY_MIRA:
            /*************************************************************
             * For MIRA, there are filters available to control counting
             * of different packet types. The PLX API doesn't currently
             * support this filtering so all are enabled.
             *
             * PCIe filters in 664h[29:20] of P0
             *   20: Disable MWr 32 TLP counter
             *   21: Disable MWr 64 TLP counter
             *   22: Disable Msg TLP counter
             *   23: Disable MRd 32 TLP counter
             *   24: Disable MRd 64 TLP counter
             *   25: Disable other NP TLP counter
             *   26: Disable ACK DLLP counting
             *   27: Disable Update-FC P DLLP counter
             *   28: Disable Update-FC NP DLLP counter
             *   29: Disable Update-FC CPL DLLP counter
             ************************************************************/
            // In MIRA legacy EP mode, PCIe registers start at 1000h
            if (pDevice->Key.DeviceMode == PLX_CHIP_MODE_LEGACY_ADAPTER)
            {
                offset = 0x1664;
            }
            else
            {
                offset = 0x664;
            }

            // Clear 664[29:20] to enable all counters
            regVal = PLX_8000_REG_READ( pDevice, offset );
            PLX_8000_REG_WRITE( pDevice, offset, regVal & ~(0x3FF << 20) );
            break;

        case PLX_FAMILY_CYGNUS:
        case PLX_FAMILY_DRACO_1:
        case PLX_FAMILY_DRACO_2:
        case PLX_FAMILY_CAPELLA_1:
        case PLX_FAMILY_CAPELLA_2:
        case PLX_FAMILY_ATLAS:
        case PLX_FAMILY_ATLAS_2:
        case PLX_FAMILY_ATLAS2_LLC:
            // Set device configuration
            if (pDevice->Key.PlxFamily == PLX_FAMILY_CYGNUS)
            {
                Bit_EgressEn = 7;
                StnCount     = 6;
                StnPortCount = 4;
            }
            else if ((pDevice->Key.PlxFamily == PLX_FAMILY_DRACO_1) ||
                     (pDevice->Key.PlxFamily == PLX_FAMILY_DRACO_2))
            {
                Bit_EgressEn = 6;
                StnCount     = 3;
                StnPortCount = 8;    // Device actually only uses 6 ports out of 8
            }
            else if ((pDevice->Key.PlxFamily == PLX_FAMILY_CAPELLA_1) ||
                     (pDevice->Key.PlxFamily == PLX_FAMILY_CAPELLA_2) ||
                     (pDevice->Key.PlxFamily == PLX_FAMILY_ATLAS) ||
                     (pDevice->Key.PlxFamily == PLX_FAMILY_ATLAS_2) ||
                     (pDevice->Key.PlxFamily == PLX_FAMILY_ATLAS2_LLC))
            {
                Bit_EgressEn    = 6;
                StnCount        = 6;
                StnPortCount    = 4;
                bStationBased   = TRUE;
                bEgressAllPorts = TRUE;

                // Override station port count for Atlas
                if (pDevice->Key.PlxFamily == PLX_FAMILY_ATLAS)
                {
                    StnPortCount = 16;
                }

                // Override station and station port count for Atlas2
                if (pDevice->Key.PlxFamily == PLX_FAMILY_ATLAS_2)
                {
                    StnCount = 9;
                    StnPortCount = 16;
                }

                // Override station and station port count for Atlas2LLC
                if (pDevice->Key.PlxFamily == PLX_FAMILY_ATLAS2_LLC)
                {
                    StnCount = 3;
                    StnPortCount = 16;
                }
                // Disable probe mode (350h[8]=0)
                if (command == PLX_PERF_CMD_START)
                {
                    pDevice->Key.ApiInternal[2] = PLX_8000_REG_READ(pDevice, pexBase + Offset_Control);
                    if ((pDevice->Key.ApiInternal[2] & (1 << 31)) != 0)
                    {
                        return PLX_STATUS_OK;
                    }			
                    regVal = PLX_8000_REG_READ( pDevice, pexBase + 0x350 );
                    PLX_8000_REG_WRITE( pDevice, pexBase + 0x350, regVal & ~((U32)1 << 8) );
                }
                else
                {
                    if ((pDevice->Key.ApiInternal[2] & (1 << 31)) != 0)
                    {
                        return PLX_STATUS_OK;
                    }
                }
		
            }

            // For certain chips, set 3F0h[9:8]=3 to disable probe mode interval
            // timer & avoid RAM pointer corruption
            if ( (command == PLX_PERF_CMD_START) &&
                 ((pDevice->Key.PlxFamily == PLX_FAMILY_DRACO_1)   ||
                  (pDevice->Key.PlxFamily == PLX_FAMILY_DRACO_2)   ||
                  (pDevice->Key.PlxFamily == PLX_FAMILY_CAPELLA_1) ||
                  (pDevice->Key.PlxFamily == PLX_FAMILY_CAPELLA_2) ||
                  (pDevice->Key.PlxFamily == PLX_FAMILY_ATLAS)) )
            {
                regVal = PLX_8000_REG_READ( pDevice, pexBase + 0x3F0 );
                PLX_8000_REG_WRITE( pDevice, pexBase + 0x3F0, regVal | (3 << 8) );
            }

	    if ((command == PLX_PERF_CMD_START) &&
                ((pDevice->Key.PlxFamily == PLX_FAMILY_ATLAS_2) ||
                (pDevice->Key.PlxFamily == PLX_FAMILY_ATLAS2_LLC)))
            {
                regVal = PLX_8000_REG_READ( pDevice, pexBase + 0x3F0 );
                PLX_8000_REG_WRITE( pDevice, pexBase + 0x3F0, regVal | (7 << 0) );
            }		    

            // Enable/Disable Performance Counter in each station (or ports if applicable)
            for (i = 0; i < (StnCount * StnPortCount); i++)
            {
                // Set port base offset
                offset = pexBase + (i * 0x1000);

                // Ingress ports (in station port 0 only)
                if ((i % StnPortCount) == 0)
                {
                    regVal = PLX_8000_REG_READ( pDevice, offset + 0x768 );

                    if (command == PLX_PERF_CMD_START)
                    {
                        PLX_8000_REG_WRITE( pDevice, offset + 0x768, regVal | ((U32)1 << 29) );
                    }
                    else
                    {
                        PLX_8000_REG_WRITE( pDevice, offset + 0x768, regVal & ~((U32)1 << 29) );
                    }
                }

                // Egress ports
                if (bEgressAllPorts || ((i % StnPortCount) == 0))
                {
                    regVal = PLX_8000_REG_READ( pDevice, offset + 0xF30 );

                    // On Atlas, F30h[21] is egress credit enable but always reads 0, so ensure remains set
                    if ((pDevice->Key.PlxFamily == PLX_FAMILY_ATLAS) ||
                        (pDevice->Key.PlxFamily == PLX_FAMILY_ATLAS_2) ||
                        (pDevice->Key.PlxFamily == PLX_FAMILY_ATLAS2_LLC)) // TODO: Check if this is applicable to Atlas2 as well?
                    {
                        regVal |= ((U32)1 << 21);
                    }

                    if (command == PLX_PERF_CMD_START)
                    {
                        PLX_8000_REG_WRITE( pDevice, offset + 0xF30, regVal | ((U32)1 << Bit_EgressEn) );
                    }
                    else
                    {
                        PLX_8000_REG_WRITE( pDevice, offset + 0xF30, regVal & ~((U32)1 << Bit_EgressEn) );
                    }
                }
            }
            break;
    }

    // Update monitor
    for (i = 0; i < StnCount; i++)
    {
        if ((i == 0) || bStationBased)
        {
            PLX_8000_REG_WRITE(
                pDevice,
                pexBase + Offset_Control + (i * StnPortCount * 0x1000),
                RegCommand
                );
        }
    }

    return PLX_STATUS_OK;
}




/*******************************************************************************
 *
 * Function   : PlxDir_PerformanceResetCounters
 *
 * Description: Resets the internal performance counters
 *
 ******************************************************************************/
PLX_STATUS
PlxDir_PerformanceResetCounters(
    PLX_DEVICE_OBJECT *pDevice
    )
{
    U32 i;
    U32 StnCount;
    U32 StnPortCount;
    U32 Offset_Control;


    // Verify supported device
    switch (pDevice->Key.PlxFamily)
    {
        case PLX_FAMILY_DENEB:
        case PLX_FAMILY_SIRIUS:
        case PLX_FAMILY_MIRA:
            Offset_Control = 0x568;

            // Offset changes for MIRA in legacy EP mode
            if ((pDevice->Key.PlxFamily == PLX_FAMILY_MIRA) &&
                (pDevice->Key.DeviceMode == PLX_CHIP_MODE_LEGACY_ADAPTER))
            {
                Offset_Control = 0x1568;
            }
            break;

        case PLX_FAMILY_CYGNUS:
        case PLX_FAMILY_DRACO_1:
        case PLX_FAMILY_DRACO_2:
        case PLX_FAMILY_CAPELLA_1:
        case PLX_FAMILY_CAPELLA_2:
            Offset_Control = 0x3E0;
            break;

        case PLX_FAMILY_ATLAS:
        case PLX_FAMILY_ATLAS_2:
        case PLX_FAMILY_ATLAS2_LLC:
            Offset_Control = ATLAS_PEX_REGS_BASE_OFFSET + 0x3E0;
            break;

        default:
            DebugPrintf(("ERROR - Unsupported PLX chip (%04X)\n", pDevice->Key.PlxChip));
            return PLX_STATUS_UNSUPPORTED;
    }

    // Set station counts for station-based monitor chips
    if ((pDevice->Key.PlxFamily == PLX_FAMILY_CAPELLA_1) ||
        (pDevice->Key.PlxFamily == PLX_FAMILY_CAPELLA_2))
    {
        StnCount     = 6;
        StnPortCount = 4;
    }
    else if (pDevice->Key.PlxFamily == PLX_FAMILY_ATLAS)
    {
        StnCount     = 6;
        StnPortCount = 16;
    }
    else if (pDevice->Key.PlxFamily == PLX_FAMILY_ATLAS_2)
    {
        StnCount     = 9;
        StnPortCount = 16;
    }
    else if (pDevice->Key.PlxFamily == PLX_FAMILY_ATLAS2_LLC)
    {
        StnCount     = 3;
        StnPortCount = 16;
    }
    else
    {
        StnCount     = 1;
        StnPortCount = 0;
    }

    // Enable monitor in each station
    for (i = 0; i < StnCount; i++)
    {
        // Reset (30) & enable monitor (31) & infinite sampling (28) & start (27)
        PLX_8000_REG_WRITE(
            pDevice,
            Offset_Control + (i * StnPortCount * 0x1000),
            ((U32)1 << 31) | ((U32)1 << 30) | ((U32)1 << 28) | ((U32)1 << 27)
            );
    }

    return PLX_STATUS_OK;
}




/*******************************************************************************
 *
 * Function   : PlxDir_PerformanceGetCounters
 *
 * Description: Retrieves a snapshot of the Performance Counters for selected ports
 *
 * Notes      :  The counters are retrieved from the PLX chip as a preset structure.
 *               Each register read returns the next value from the sequence.  The
 *               structure varies between some PLX chips.  Below is a diagram of each.
 *
 *   IN    = Ingress port
 *   EG    = Egress port
 *   PH    = Number of Posted Headers (Write TLPs)
 *   PDW   = Number of Posted DWords
 *   NPH   = Number of Non-Posted Headers
 *   NPDW  = Non-Posted DWords (Read TLP Dwords)
 *   CPLH  = Number of Completion Headers (CPL TLPs)
 *   CPLDW = Number of Completion DWords
 *   DLLP  = Number of DLLPs
 *   PHY   = PHY Layer (always 0)
 *   PLD   = USB endpoint payload count
 *   RAW   = USB endpoint raw byte count
 *   PKT   = USB endpoint packet count
 *
 *           Deneb & Cygnus                  Draco                     Sirius
 *      --------------------------   ----------------------    ----------------------
 *          14 counters/port          14 counters/port          13 counters/port
 *           4 ports/station           6 ports/station          16 ports/station
 *    Deneb: 3 stations (12 ports)     3 stations (18 ports)     1 station (16 ports)
 *   Cygnus: 6 stations (24 ports)
 *          56 counters/station       84 counters/station      208 counters/station
 *  Deneb: 168 counters (56 * 3)     252 counters (84 * 3)     208 counters (208 * 1)
 * Cygnus: 336 counters (56 * 6)
 *
 *       off     Counter            off     Counter            off      Counter
 *           -----------------          -----------------          ------------------
 *         0| Port 0 IN PH    |       0| Port 0 IN PH    |       0| Port 0 IN PH     |
 *         4| Port 0 IN PDW   |       4| Port 0 IN PDW   |       4| Port 0 IN PDW    |
 *         8| Port 0 IN NPDW  |       8| Port 0 IN NPDW  |       8| Port 0 IN NPDW   |
 *         C| Port 0 IN CPLH  |       C| Port 0 IN CPLH  |       C| Port 0 IN CPLH   |
 *        10| Port 0 IN CPLDW |      10| Port 0 IN CPLDW |      10| Port 0 IN CPLDW  |
 *          |-----------------|        |-----------------|        |------------------|
 *        14| Port 1 IN PH    |      14| Port 1 IN PH    |      14| Port 1 IN PH     |
 *          |       :         |        |       :         |        |       :          |
 *          |       :         |        |       :         |        |       :          |
 *        24| Port 1 IN CPLDW |      24| Port 1 IN CPLDW |      24| Port 1 IN CPLDW  |
 *          |-----------------|        |/\/\/\/\/\/\/\/\/|        |/\/\/\/\/\/\/\/\/\|
 *        28| Port 2 IN PH    |        |       :         |        |       :          |
 *          |       :         |        |       :         |        |       :          |
 *          |       :         |        |       :         |        |       :          |
 *        38| Port 2 IN CPLDW |        |       :         |        |       :          |
 *          |-----------------|        |       :         |        |       :          |
 *        3C| Port 3 IN PH    |        |/\/\/\/\/\/\/\/\/|        |/\/\/\/\/\/\/\/\/\|
 *          |       :         |      64| Port 5 IN PH    |     12C| Port 15 IN PH    |
 *          |       :         |        |       :         |        |       :          |
 *        4C| Port 3 IN CPLDW |        |       :         |        |       :          |
 *          |-----------------|      74| Port 5 IN CPLDW |     13C| Port 15 IN CPLDW |
 *        50| Port 0 EG PH    |        |-----------------|        |------------------|
 *        54| Port 0 EG PDW   |      78| Port 0 EG PH    |     140| Port 0 EG PH     |
 *        58| Port 0 EG NPDW  |      7C| Port 0 EG PDW   |     144| Port 0 EG PDW    |
 *        5C| Port 0 EG CPLH  |      80| Port 0 EG NPDW  |     148| Port 0 EG NPDW   |
 *        60| Port 0 EG CPLDW |      84| Port 0 EG CPLH  |     14C| Port 0 EG CPLH   |
 *          |-----------------|      88| Port 0 EG CPLDW |     150| Port 0 EG CPLDW  |
 *        64| Port 1 EG PH    |        |-----------------|        |------------------|
 *          |       :         |      8C| Port 1 EG PH    |     154| Port 1 EG PH     |
 *          |       :         |        |       :         |        |       :          |
 *        74| Port 1 EG CPLDW |        |       :         |        |       :          |
 *          |-----------------|      9C| Port 1 EG CPLDW |     164| Port 1 EG CPLDW  |
 *        78| Port 2 EG PH    |        |/\/\/\/\/\/\/\/\/|        |/\/\/\/\/\/\/\/\/\|
 *          |       :         |        |       :         |        |       :          |
 *          |       :         |        |       :         |        |       :          |
 *        88| Port 2 EG CPLDW |        |       :         |        |       :          |
 *          |-----------------|        |       :         |        |       :          |
 *        8C| Port 3 EG PH    |        |       :         |        |       :          |
 *          |       :         |        |/\/\/\/\/\/\/\/\/|        |/\/\/\/\/\/\/\/\/\|
 *          |       :         |      DC| Port 5 EG PH    |     26C| Port 15 EG PH    |
 *        9C| Port 3 EG CPLDW |        |       :         |        |       :          |
 *          |-----------------|        |       :         |        |       :          |
 *        A0| Port 0 IN DLLP  |      EC| Port 5 EG CPLDW |     27C| Port 15 EG CPLDW |
 *        A4| Port 1 IN DLLP  |        |-----------------|        |------------------|
 *        A8| Port 2 IN DLLP  |      F0| Port 0 IN DLLP  |     280| Port  0 IN DLLP  |
 *        AC| Port 3 IN DLLP  |      F4| Port 1 IN DLLP  |     284| Port  2 IN DLLP  |
 *          |-----------------|      F8| Port 2 IN DLLP  |     288| Port  4 IN DLLP  |
 *        B0| Port 0 EG DLLP  |      FC| Port 3 IN DLLP  |     28C| Port  6 IN DLLP  |
 *        B4| Port 1 EG DLLP  |     100| Port 4 IN DLLP  |     290| Port  8 IN DLLP  |
 *        B8| Port 2 EG DLLP  |     104| Port 5 IN DLLP  |     294| Port 10 IN DLLP  |
 *        BC| Port 3 EG DLLP  |        |-----------------|     298| Port 12 IN DLLP  |
 *          |-----------------|     108| Port 0 EG DLLP  |     29C| Port 14 IN DLLP  |
 *        C0| Port 0 IN PHY   |     10C| Port 1 EG DLLP  |        |------------------|
 *        C4| Port 1 IN PHY   |     110| Port 2 EG DLLP  |     2A0| Port  0 EG DLLP  |
 *        C8| Port 2 IN PHY   |     114| Port 3 EG DLLP  |     2A4| Port  2 EG DLLP  |
 *        CC| Port 3 IN PHY   |     118| Port 4 EG DLLP  |     2A8| Port  4 EG DLLP  |
 *          |-----------------|     11C| Port 5 EG DLLP  |     2AC| Port  6 EG DLLP  |
 *        D0| Port 0 EG PHY   |        |-----------------|     2B0| Port  8 EG DLLP  |
 *        D4| Port 1 EG PHY   |     120| Port 0 IN PHY   |     2B4| Port 10 EG DLLP  |
 *        D8| Port 2 EG PHY   |     124| Port 1 IN PHY   |     2B8| Port 12 EG DLLP  |
 *        DC| Port 3 EG PHY   |     128| Port 2 IN PHY   |     2BC| Port 14 EG DLLP  |
 *           -----------------      12C| Port 3 IN PHY   |        |------------------|
 *                                  130| Port 4 IN PHY   |     2C0| Port  1 IN DLLP  |
 *                                  134| Port 5 IN PHY   |     2C4| Port  3 IN DLLP  |
 *                                     |-----------------|     2C8| Port  5 IN DLLP  |
 *                                  138| Port 0 EG PHY   |     2CC| Port  7 IN DLLP  |
 *                                  13C| Port 1 EG PHY   |     2D0| Port  9 IN DLLP  |
 *                                  140| Port 2 EG PHY   |     2D4| Port 11 IN DLLP  |
 *                                  144| Port 3 EG PHY   |     2D8| Port 13 IN DLLP  |
 *                                  148| Port 4 EG PHY   |     2DC| Port 15 IN DLLP  |
 *                                  14C| Port 5 EG PHY   |        |------------------|
 *                                      -----------------      2E0| Port  1 EG DLLP  |
 *                                                             2E4| Port  3 EG DLLP  |
 *                                                             2E8| Port  5 EG DLLP  |
 *                                                             2EC| Port  7 EG DLLP  |
 *                                                             2F0| Port  9 EG DLLP  |
 *                                                             2F4| Port 11 EG DLLP  |
 *                                                             2F8| Port 13 EG DLLP  |
 *                                                             2FC| Port 15 EG DLLP  |
 *                                                                |------------------|
 *                                                             300| Port 0 PHY       |
 *                                                                |       :          |
 *                                                                |       :          |
 *                                                             33C| Port 15 PHY      |
 *                                                                 ------------------
 *
 *       Mira                        Capella-1/2                            Atlas
 * -----------------------     -----------------------               -----------------------
 *  14 PCIe counters/port      14 counters/port                      14 counters/port
 *   4 ports/station            4 pts/stn but 5 in RAM               16 ports/station
 *   1 stations (4 ports)       6 stn (24 ports)                      6 stations (96 ports)
 *                            *SW uses 30 pts to read all
 *  86 counters/station                                             224 counters/station
 *  86 counters (86 * 1)        70 counters/station               1,344 counters (224 * 6)
 *                             420 counters (70 * 6)
 *
 * off     Counter             off     Counter                      off     Counter
 *     -----------------           -----------------                    -----------------
 *   0| Port 0 IN PH    |        0| Port 0 IN PH    |                 0| Port 0 IN PH    |
 *   4| Port 0 IN PDW   |        4| Port 0 IN PDW   |                 4| Port 0 IN PDW   |
 *   8| Port 0 IN NPDW  |        8| Port 0 IN NPDW  |                 8| Port 0 IN NPH   |
 *   C| Port 0 IN CPLH  |        C| Port 0 IN CPLH  |                 C| Port 0 IN NPDW  |
 *  10| Port 0 IN CPLDW |       10| Port 0 IN CPLDW |                10| Port 0 IN CPLH  |
 *    |-----------------|         |-----------------|                14| Port 0 IN CPLDW |
 *  14| Port 1 IN PH    |       14| Port 1 IN PH    |                  |-----------------|
 *    |       :         |         |       :         |                18| Port 1 IN PH    |
 *    |       :         |         |       :         |                  |       :         |
 *  24| Port 1 IN CPLDW |       24| Port 1 IN CPLDW |                  |       :         |
 *    |-----------------|         |/\/\/\/\/\/\/\/\/|                2C| Port 1 IN CPLDW |
 *  28| Port 2 IN PH    |         |       :         |                  |/\/\/\/\/\/\/\/\/|
 *    |       :         |         |       :         |                  |       :         |
 *    |       :         |         |       :         |                  |       :         |
 *  38| Port 2 IN CPLDW |         |       :         |                  |       :         |
 *    |-----------------|         |       :         |                  |       :         |
 *  3C| Port 3 IN PH    |         |/\/\/\/\/\/\/\/\/|                  |       :         |
 *    |       :         |       50| Port 4 IN PH    |                  |/\/\/\/\/\/\/\/\/|
 *    |       :         |         |       :         |               168| Port 15 IN PH   |
 *  4C| Port 3 IN CPLDW |         |       :         |                  |       :         |
 *    |-----------------|       60| Port 4 IN CPLDW |                  |       :         |
 *  50| Port 0 EG PH    |         |-----------------|               17C| Port 15 IN CPDW |
 *  54| Port 0 EG PDW   |       64| Port 0 EG PH    |                  |-----------------|
 *  58| Port 0 EG NPDW  |       68| Port 0 EG PDW   |               180| Port 0 EG PH    |
 *  5C| Port 0 EG CPLH  |       6c| Port 0 EG NPDW  |               184| Port 0 EG PDW   |
 *  60| Port 0 EG CPLDW |       70| Port 0 EG CPLH  |               188| Port 0 EG NPH   |
 *    |-----------------|       74| Port 0 EG CPLDW |               18C| Port 0 EG NPDW  |
 *  64| Port 1 EG PH    |         |-----------------|               190| Port 0 EG CPLH  |
 *    |       :         |       78| Port 1 EG PH    |               194| Port 0 EG CPLDW |
 *    |       :         |         |       :         |                  |-----------------|
 *  74| Port 1 EG CPLDW |         |       :         |               198| Port 1 EG PH    |
 *    |-----------------|       88| Port 1 EG CPLDW |                  |       :         |
 *  78| Port 2 EG PH    |         |/\/\/\/\/\/\/\/\/|                  |       :         |
 *    |       :         |         |       :         |               1AC| Port 1 EG CPLDW |
 *    |       :         |         |       :         |                  |/\/\/\/\/\/\/\/\/|
 *  88| Port 2 EG CPLDW |         |       :         |                  |       :         |
 *    |-----------------|         |       :         |                  |       :         |
 *  8C| Port 3 EG PH    |         |       :         |                  |       :         |
 *    |       :         |         |/\/\/\/\/\/\/\/\/|                  |       :         |
 *    |       :         |         |-----------------|                  |       :         |
 *  9C| Port 3 EG CPLDW |       B4| Port 4 EG PH    |                  |/\/\/\/\/\/\/\/\/|
 *    |-----------------|         |       :         |                  |-----------------|
 *  A0| Port 0 IN DLLP  |         |       :         |               2E8| Port 4 EG PH    |
 *  A4| Port 1 IN DLLP  |       C4| Port 4 EG CPLDW |                  |       :         |
 *  A8| Port 2 IN DLLP  |         |-----------------|                  |       :         |
 *  AC| Port 3 IN DLLP  |       C8| Port 0 IN DLLP  |               2FC| Port 4 EG CPLDW |
 *    |-----------------|       CC| Port 1 IN DLLP  |                  |-----------------|
 *  B0| Port 0 EG DLLP  |       D0| Port 2 IN DLLP  |               300| Port 0 IN DLLP  |
 *  B4| Port 1 EG DLLP  |       D4| Port 3 IN DLLP  |               304| Port 1 IN DLLP  |
 *  B8| Port 2 EG DLLP  |       D8| Port 4 IN DLLP  |               308| Port 2 IN DLLP  |
 *  BC| Port 3 EG DLLP  |         |-----------------|               30C| Port 3 IN DLLP  |
 *    |-----------------|       DC| -- Invalid --   |               310| Port 4 IN DLLP  |
 *  C0| GPEP_0 IN PLD   |       E0| Port 0 EG DLLP  |               314| Port 5 IN DLLP  |
 *  C4| GPEP_0 IN RAW   |       E4| Port 1 EG DLLP  |               318| Port 6 IN DLLP  |
 *  C8| GPEP_0 IN PKT   |       E8| Port 2 EG DLLP  |               31C| Port 7 IN DLLP  |
 *    |-----------------|       EC| Port 3 EG DLLP  |*P4 EG DLLP    320| Port 8 IN DLLP  |
 *  CC| GPEP_1 IN PLD   |         |-----------------|  missing      324| Port 9 IN DLLP  |
 *  D0| GPEP_1 IN RAW   |       F0| Port 0 IN PHY   |               328| Port 10 IN DLLP |
 *  D4| GPEP_1 IN PKT   |       F4| Port 1 IN PHY   |               32C| Port 11 IN DLLP |
 *    |-----------------|       F8| Port 2 IN PHY   |               330| Port 12 IN DLLP |
 *  D8| GPEP_2 IN PLD   |       FC| Port 3 IN PHY   |               334| Port 13 IN DLLP |
 *  DC| GPEP_2 IN RAW   |      100| Port 4 IN PHY   |               338| Port 14 IN DLLP |
 *  E0| GPEP_2 IN PKT   |         |-----------------|               33C| Port 15 IN DLLP |
 *    |-----------------|      104| Port 0 EG PHY   |                  |-----------------|
 *  E4| GPEP_3 IN PLD   |      108| Port 1 EG PHY   |               340| Port 0 EG DLLP  |
 *  E8| GPEP_3 IN RAW   |      10C| Port 2 EG PHY   |               344| Port 1 EG DLLP  |
 *  EC| GPEP_3 IN PKT   |      110| Port 3 EG PHY   |               348| Port 2 EG DLLP  |
 *    |-----------------|      114| Port 4 EG PHY   |               34C| Port 3 EG DLLP  |
 *  F0| GPEP_0 OUT PLD  |          -----------------                350| Port 4 EG DLLP  |
 *  F4| GPEP_0 OUT RAW  |                                           354| Port 5 EG DLLP  |
 *  F8| GPEP_0 OUT PKT  |                                           358| Port 6 EG DLLP  |
 *    |-----------------|                                           35C| Port 7 EG DLLP  |
 *  FC| GPEP_1 OUT PLD  |                                           360| Port 8 EG DLLP  |
 * 100| GPEP_1 OUT RAW  |                                           364| Port 9 EG DLLP  |
 * 104| GPEP_1 OUT PKT  |                                           368| Port 10 EG DLLP |
 *    |-----------------|                                           36C| Port 11 EG DLLP |
 * 108| GPEP_2 OUT PLD  |                                           370| Port 12 EG DLLP |
 * 10C| GPEP_2 OUT RAW  |                                           374| Port 13 EG DLLP |
 * 110| GPEP_2 OUT PKT  |                                           378| Port 14 EG DLLP |
 *    |-----------------|                                           37C| Port 15 EG DLLP |
 * 114| GPEP_3 OUT PLD  |                                               -----------------
 * 118| GPEP_3 OUT RAW  |
 * 11C| GPEP_3 OUT PKT  |
 *    |-----------------|
 * 120| EP_0 IN PLD     |
 * 124| EP_0 IN RAW     |
 * 128| EP_0 IN PKT     |
 *    |-----------------|
 * 12C| EP_0 OUT PLD    |
 * 130| EP_0 OUT RAW    |
 * 134| EP_0 OUT PKT    |
 *    |-----------------|
 * 138| PHY (always 0)  |
 * 13C| PHY (always 0)  |
 *     -----------------
 *
 *
 *           Atlas2
 *    -----------------------
 *    14 counters/port
 *    16 ports/station
 *     9 stations (144 ports)
 *
 *   224 counters/station
 * 2,016 counters (224 * 9)
 *
 *  off     Counter
 *    -----------------
 *   0| Port 0 IN PH    |
 *   4| Port 0 IN PDW   |
 *   8| Port 0 IN NPH   |
 *   C| Port 0 IN NPDW  |
 *  10| Port 0 IN CPLH  |
 *  14| Port 0 IN CPLDW |
 *    |-----------------|
 *  18| Port 1 IN PH    |
 *    |       :         |
 *    |       :         |
 *  2C| Port 1 IN CPLDW |
 *    |/\/\/\/\/\/\/\/\/|
 *    |       :         |
 *    |       :         |
 *    |       :         |
 *    |       :         |
 *    |       :         |
 *    |/\/\/\/\/\/\/\/\/|
 * 168| Port 15 IN PH   |
 *    |       :         |
 *    |       :         |
 * 17C| Port 15 IN CPLDW|
 *    |-----------------|
 * 180| Port 0 EG PH    |
 * 184| Port 0 EG PDW   |
 * 188| Port 0 EG NPH   |
 * 18C| Port 0 EG NPDW  |
 * 190| Port 0 EG CPLH  |
 * 194| Port 0 EG CPLDW |
 *    |-----------------|
 * 198| Port 1 EG PH    |
 *    |       :         |
 *    |       :         |
 * 1AC| Port 1 EG CPLDW |
 *    |/\/\/\/\/\/\/\/\/|
 *    |       :         |
 *    |       :         |
 *    |       :         |
 *    |       :         |
 *    |       :         |
 *    |/\/\/\/\/\/\/\/\/|
 *    |-----------------|
 * 2E8| Port 15 EG PH   |
 *    |       :         |
 *    |       :         |
 * 2FC| Port 15 EG CPLDW|
 *    |-----------------|
 * 300| Port 0 IN DLLP  |
 * 304| Port 1 IN DLLP  |
 * 308| Port 2 IN DLLP  |
 * 30C| Port 3 IN DLLP  |
 * 310| Port 4 IN DLLP  |
 * 314| Port 5 IN DLLP  |
 * 318| Port 6 IN DLLP  |
 * 31C| Port 7 IN DLLP  |
 * 320| Port 8 IN DLLP  |
 * 324| Port 9 IN DLLP  |
 * 328| Port 10 IN DLLP |
 * 32C| Port 11 IN DLLP |
 * 330| Port 12 IN DLLP |
 * 334| Port 13 IN DLLP |
 * 338| Port 14 IN DLLP |
 * 33C| Port 15 IN DLLP |
 *    |-----------------|
 * 340| Port 0 EG DLLP  |
 * 344| Port 1 EG DLLP  |
 * 348| Port 2 EG DLLP  |
 * 34C| Port 3 EG DLLP  |
 * 350| Port 4 EG DLLP  |
 * 354| Port 5 EG DLLP  |
 * 358| Port 6 EG DLLP  |
 * 35C| Port 7 EG DLLP  |
 * 360| Port 8 EG DLLP  |
 * 364| Port 9 EG DLLP  |
 * 368| Port 10 EG DLLP |
 * 36C| Port 11 EG DLLP |
 * 370| Port 12 EG DLLP |
 * 374| Port 13 EG DLLP |
 * 378| Port 14 EG DLLP |
 * 37C| Port 15 EG DLLP |
 *    |__ __ __ __ __ __|
 *
  *
 *           Atlas2LLC
 *    -----------------------
 *    14 counters/port
 *    16 ports/station
 *     3 stations (48 ports)
 *
 *   224 counters/station
 *   672 counters (224 * 3)
 *
 *  off     Counter
 *    -----------------
 *   0| Port 0 IN PH    |
 *   4| Port 0 IN PDW   |
 *   8| Port 0 IN NPH   |
 *   C| Port 0 IN NPDW  |
 *  10| Port 0 IN CPLH  |
 *  14| Port 0 IN CPLDW |
 *    |-----------------|
 *  18| Port 1 IN PH    |
 *    |       :         |
 *    |       :         |
 *  2C| Port 1 IN CPLDW |
 *    |/\/\/\/\/\/\/\/\/|
 *    |       :         |
 *    |       :         |
 *    |       :         |
 *    |       :         |
 *    |       :         |
 *    |/\/\/\/\/\/\/\/\/|
 * 168| Port 15 IN PH   |
 *    |       :         |
 *    |       :         |
 * 17C| Port 15 IN CPLDW|
 *    |-----------------|
 * 180| Port 0 EG PH    |
 * 184| Port 0 EG PDW   |
 * 188| Port 0 EG NPH   |
 * 18C| Port 0 EG NPDW  |
 * 190| Port 0 EG CPLH  |
 * 194| Port 0 EG CPLDW |
 *    |-----------------|
 * 198| Port 1 EG PH    |
 *    |       :         |
 *    |       :         |
 * 1AC| Port 1 EG CPLDW |
 *    |/\/\/\/\/\/\/\/\/|
 *    |       :         |
 *    |       :         |
 *    |       :         |
 *    |       :         |
 *    |       :         |
 *    |/\/\/\/\/\/\/\/\/|
 *    |-----------------|
 * 2E8| Port 15 EG PH   |
 *    |       :         |
 *    |       :         |
 * 2FC| Port 15 EG CPLDW|
 *    |-----------------|
 * 300| Port 0 IN DLLP  |
 * 304| Port 1 IN DLLP  |
 * 308| Port 2 IN DLLP  |
 * 30C| Port 3 IN DLLP  |
 * 310| Port 4 IN DLLP  |
 * 314| Port 5 IN DLLP  |
 * 318| Port 6 IN DLLP  |
 * 31C| Port 7 IN DLLP  |
 * 320| Port 8 IN DLLP  |
 * 324| Port 9 IN DLLP  |
 * 328| Port 10 IN DLLP |
 * 32C| Port 11 IN DLLP |
 * 330| Port 12 IN DLLP |
 * 334| Port 13 IN DLLP |
 * 338| Port 14 IN DLLP |
 * 33C| Port 15 IN DLLP |
 *    |-----------------|
 * 340| Port 0 EG DLLP  |
 * 344| Port 1 EG DLLP  |
 * 348| Port 2 EG DLLP  |
 * 34C| Port 3 EG DLLP  |
 * 350| Port 4 EG DLLP  |
 * 354| Port 5 EG DLLP  |
 * 358| Port 6 EG DLLP  |
 * 35C| Port 7 EG DLLP  |
 * 360| Port 8 EG DLLP  |
 * 364| Port 9 EG DLLP  |
 * 368| Port 10 EG DLLP |
 * 36C| Port 11 EG DLLP |
 * 370| Port 12 EG DLLP |
 * 374| Port 13 EG DLLP |
 * 378| Port 14 EG DLLP |
 * 37C| Port 15 EG DLLP |
 *    |__ __ __ __ __ __|
 *
 ******************************************************************************/
PLX_STATUS
PlxDir_PerformanceGetCounters(
    PLX_DEVICE_OBJECT *pDevice,
    PLX_PERF_PROP     *pPerfProps,
    U8                 NumOfObjects
    )
{
    U8   NumCounters;
    U8   CurrStation;
    U8   StnCount;
    U8   StnPortCount;
    U8   bStationBased;
    U8   RamStnPortCount;
    U8   InEgPerPortCount;
    U16  i;
    U16  index;
    U16  IndexBase;
    U32  Offset_Fifo;
    U32  Offset_RamCtrl;
    U32  regVal;
    U32 *pCounters;
    U32 *pCounter;
    U32 *pCounter_Prev;
    U32  Counter_PrevTmp[PERF_COUNTERS_PER_PORT];
    S64  TmpValue;


    // Default to single station access
    bStationBased = FALSE;

    // Assume station port count in RAM is identical to station port count
    RamStnPortCount = 0;

    // Most chips have 5 ingress & egress counters per port
    InEgPerPortCount = 5;

    // Setup parameters for reading counters
    switch (pDevice->Key.PlxFamily)
    {
        case PLX_FAMILY_DENEB:
            Offset_RamCtrl = 0x618;
            Offset_Fifo    = 0x628;
            NumCounters    = 14;
            StnCount       = 3;
            StnPortCount   = 4;
            break;

        case PLX_FAMILY_SIRIUS:
            Offset_RamCtrl = 0x618;
            Offset_Fifo    = 0x628;
            NumCounters    = 13;
            StnCount       = 1;
            StnPortCount   = 16;
            break;

        case PLX_FAMILY_CYGNUS:
            Offset_RamCtrl = 0x3F0;
            Offset_Fifo    = 0x3E4;
            NumCounters    = 14;
            StnCount       = 6;
            StnPortCount   = 4;
            break;

        case PLX_FAMILY_MIRA:
            Offset_RamCtrl = 0x618;
            Offset_Fifo    = 0x628;
            NumCounters    = 12;
            StnCount       = 1;
            StnPortCount   = 4;

            // In MIRA legacy EP mode, PCIe registers start at 1000h
            if (pDevice->Key.DeviceMode == PLX_CHIP_MODE_LEGACY_ADAPTER)
            {
                Offset_RamCtrl += 0x1000;
                Offset_Fifo    += 0x1000;
            }
            break;

        case PLX_FAMILY_DRACO_1:
        case PLX_FAMILY_DRACO_2:
            Offset_RamCtrl = 0x3F0;
            Offset_Fifo    = 0x3E4;
            NumCounters    = 14;
            StnCount       = 3;
            StnPortCount   = 6;
            break;

        case PLX_FAMILY_CAPELLA_1:
        case PLX_FAMILY_CAPELLA_2:
            Offset_RamCtrl  = 0x3F0;
            Offset_Fifo     = 0x3E4;
            NumCounters     = 14;
            StnCount        = 6;
            StnPortCount    = 4;
            RamStnPortCount = 5;
            bStationBased   = TRUE;
            break;

        case PLX_FAMILY_ATLAS:
        case PLX_FAMILY_ATLAS_2:
        case PLX_FAMILY_ATLAS2_LLC:
            Offset_RamCtrl   = ATLAS_PEX_REGS_BASE_OFFSET + 0x3F0;
            Offset_Fifo      = ATLAS_PEX_REGS_BASE_OFFSET + 0x3E4;
            NumCounters      = 14;
            StnCount         = 6;
            StnPortCount     = 16;
            bStationBased    = TRUE;
            InEgPerPortCount = 6;    // Non-Posted header count added

            // Atlas2 support 9 stations
            if (pDevice->Key.PlxFamily == PLX_FAMILY_ATLAS_2)
            {
                StnCount = 9;
            }

            // Atlas2LLC support 3 stations
            if (pDevice->Key.PlxFamily == PLX_FAMILY_ATLAS2_LLC)
            {
                StnCount = 3;
            }
            break;

        default:
            DebugPrintf(("ERROR - Unsupported PLX chip (%04X)\n", pDevice->Key.PlxChip));
            return PLX_STATUS_UNSUPPORTED;
    }

    // Set RAM station port count if matches station port count
    if (RamStnPortCount == 0)
    {
        RamStnPortCount = StnPortCount;
    }

    // Allocate buffer to contain counter data for all ports
    pCounters = malloc( (StnCount * RamStnPortCount) * NumCounters * sizeof(U32) );
    if (pCounters == NULL)
    {
        return PLX_STATUS_INSUFFICIENT_RES;
    }

    // RAM control
    regVal = ((U32)2 << 4) |   // Capture type ([5:4])
             ((U32)1 << 2) |   // Reset read pointer
             ((U32)1 << 0);    // Enable RAM

    // Reset RAM read pointer
    for (i = 0; i < StnCount; i++)
    {
        if ((i == 0) || bStationBased)
        {
            PLX_8000_REG_WRITE(
                pDevice,
                Offset_RamCtrl + (i * StnPortCount * 0x1000),
                regVal
                );
        }
    }

    // Read in all counters
    i           = 0;
    CurrStation = 0;
    while (i < (StnCount * RamStnPortCount * NumCounters))
    {
        // Check if reached station boundary
        if ((i % (NumCounters * RamStnPortCount)) == 0)
        {
            DebugPrintf_Cont(("\n"));
            if (i == 0)
            {
                DebugPrintf(("           Counters\n"));
                DebugPrintf(("----------------------------------\n"));
            }
            else
            {
                // Increment to next station
                CurrStation++;

                // For station based counters use register in station port 0
                if (bStationBased)
                {
                    Offset_Fifo += (StnPortCount * 0x1000);
                }
            }

            DebugPrintf(("Station %d:\n", CurrStation));
            DebugPrintf(("%03X:", (U16)(i * sizeof(U32))));
        }
        else if ((i % 4) == 0)
        {
            DebugPrintf_Cont(("\n"));
            DebugPrintf(("%03X:", (U16)(i * sizeof(U32))));
        }

        // Get next counter
        pCounters[i] = PLX_8000_REG_READ( pDevice, Offset_Fifo );
        DebugPrintf_Cont((" %08X", pCounters[i]));

        // Jump to next counter
        i++;
    }
    DebugPrintf_Cont(("\n"));

    // Assign counter values to enabled ports
    i = 0;
    while (i < NumOfObjects)
    {
        // Verify the station & port numbers are within valid range
        if ( (pPerfProps[i].Station >= StnCount) ||
             (pPerfProps[i].StationPort >= RamStnPortCount) )
        {
            ErrorPrintf((
                "ERROR - Station or station port invalid in perf object %d\n",
                i
                ));
            // Skip to next object
            i++;
            continue;
        }

        // Make a copy of the previous values before overwriting them
        RtlCopyMemory(
            Counter_PrevTmp,
            &(pPerfProps[i].Prev_IngressPostedHeader),
            PERF_COUNTERS_PER_PORT * sizeof(U32)    // All 14 counters in structure
            );

        // Save current values to previous
        RtlCopyMemory(
            &(pPerfProps[i].Prev_IngressPostedHeader),
            &(pPerfProps[i].IngressPostedHeader),
            PERF_COUNTERS_PER_PORT * sizeof(U32)    // All 14 counters in structure
            );

        // Calculate starting index for counters based on port in station
        IndexBase = pPerfProps[i].Station * (NumCounters * RamStnPortCount);

        // Ingress counters start at index 0 from base
        index = IndexBase + 0 + (pPerfProps[i].StationPort * InEgPerPortCount);

        // Get ingress counters (5 or 6 DW/port)
        pPerfProps[i].IngressPostedHeader = pCounters[index++];  // 0
        pPerfProps[i].IngressPostedDW     = pCounters[index++];  // 1
        if ((pDevice->Key.PlxFamily == PLX_FAMILY_ATLAS) ||
            (pDevice->Key.PlxFamily == PLX_FAMILY_ATLAS_2) ||
            (pDevice->Key.PlxFamily == PLX_FAMILY_ATLAS2_LLC))
        {
            // NP header added in Atlas
            pPerfProps[i].IngressNonpostedHdr  = pCounters[index++];  // 2
        }
        pPerfProps[i].IngressNonpostedDW  = pCounters[index++];  // 2 or 3
        pPerfProps[i].IngressCplHeader    = pCounters[index++];  // 3 or 4
        pPerfProps[i].IngressCplDW        = pCounters[index];    // 4 or 5

        // Egress counters start after ingress
        index = IndexBase + (InEgPerPortCount * RamStnPortCount)
                          + (pPerfProps[i].StationPort * InEgPerPortCount);

        // Get egress counters (5 or 6 DW/port)
        pPerfProps[i].EgressPostedHeader = pCounters[index++];   // 0
        pPerfProps[i].EgressPostedDW     = pCounters[index++];   // 1
        if ((pDevice->Key.PlxFamily == PLX_FAMILY_ATLAS) ||
            (pDevice->Key.PlxFamily == PLX_FAMILY_ATLAS_2) ||
            (pDevice->Key.PlxFamily == PLX_FAMILY_ATLAS2_LLC))
        {
            // NP header added in Atlas
            pPerfProps[i].EgressNonpostedHdr  = pCounters[index++];  // 2
        }
        pPerfProps[i].EgressNonpostedDW  = pCounters[index++];   // 2 or 3
        pPerfProps[i].EgressCplHeader    = pCounters[index++];   // 3 or 4
        pPerfProps[i].EgressCplDW        = pCounters[index++];   // 4 or 5

        // DLLP ingress counters start after egress
        index = IndexBase + ((InEgPerPortCount * 2) * RamStnPortCount);

        // DLLP counter location depends upon chip
        if (pDevice->Key.PlxFamily == PLX_FAMILY_SIRIUS)
        {
            // Even port number DLLP counters are first
            index += (pPerfProps[i].StationPort / 2);

            // Odd port number DLLP counters follow even ports
            if (pPerfProps[i].StationPort & (1 << 0))
            {
                index += RamStnPortCount;
            }
        }
        else
        {
            index += pPerfProps[i].StationPort;
        }

        // Get DLLP ingress counters (1 DW/port)
        pPerfProps[i].IngressDllp = pCounters[index];

        // Egress DLLP counters follow Ingress
        if (pDevice->Key.PlxFamily == PLX_FAMILY_SIRIUS)
        {
            // Even ports are grouped together
            index += (RamStnPortCount / 2);
        }
        else
        {
            index += RamStnPortCount;
        }

        // For Capella, egress DLLP skips one offset & port 4 is lost
        if ((pDevice->Key.PlxFamily == PLX_FAMILY_CAPELLA_1) ||
            (pDevice->Key.PlxFamily == PLX_FAMILY_CAPELLA_2))
        {
            index++;
        }

        // Get DLLP egress counters (1 DW/port)
        pPerfProps[i].EgressDllp = pCounters[index];

        /**********************************************************
         * In some cases on Draco 1 chips, device may incorrectly
         * report a counter as 0. The following code checks the
         * current & previous counters to detect this case. If the
         * issue is present, the previous value is used instead to
         * minimize data reporting errors.
         *********************************************************/
        if ((pDevice->Key.PlxFamily == PLX_FAMILY_DRACO_1) &&
            (pPerfProps[i].LinkWidth != 0))
        {
            // Setup initial pointers to stored counters
            pCounter      = &pPerfProps[i].IngressPostedHeader;
            pCounter_Prev = &pPerfProps[i].Prev_IngressPostedHeader;

            // Verify each counter & use previous on error
            for (index = 0; index < PERF_COUNTERS_PER_PORT; index++)
            {
                if (((*pCounter == 0) && (*pCounter_Prev != 0)) || (*pCounter == 0x4C041301))
                {
                    // Store 64-bit counter in case of wrapping
                    TmpValue = *pCounter_Prev;
                    if (*pCounter_Prev < Counter_PrevTmp[index])
                    {
                        TmpValue += ((S64)1 << 32);
                    }

                    // Re-use difference between previous 2 counter values
                    *pCounter = *pCounter_Prev + (U32)(TmpValue - Counter_PrevTmp[index]);
                }

                // Increment to next counter
                pCounter++;
                pCounter_Prev++;
            }
        }

        // Go to next performance object
        i++;
    }

    // Release temporary buffers
    free( pCounters );

    return PLX_STATUS_OK;
}




/***********************************************************
 *
 *               PRIVATE SUPPORT FUNCTIONS
 *
 **********************************************************/


/*******************************************************************************
 *
 * Function   : PlxDir_PciFindCapability
 *
 * Description: Scans a device's capability list to find the base offset of the
 *               specified PCI or PCIe extended capability. If the capability ID
 *               is PCI or PCIe vendor-specific (VSEC), an optional instance
 *               number can be provided since a device could have multiple VSEC
 *               capabilities. A value of 0 returns the 1st matching instance but
 *               the parameter is ignored for non-VSEC capability search.
 *
 ******************************************************************************/
U16
PlxDir_PciFindCapability(
    PLX_DEVICE_OBJECT *pDevice,
    U16                CapID,
    U8                 bPCIeCap,
    U8                 InstanceNum
    )
{
    U8  matchCount;
    U16 offset;
    U16 currID;
    U32 regVal;


    // Get PCI command register (04h)
    PLX_PCI_REG_READ( pDevice, PCI_REG_CMD_STAT, &regVal );

    // Verify device responds to PCI accesses (in case link down)
    if (regVal == PCI_CFG_RD_ERR_VAL)
    {
        return 0;
    }

    // Verify device supports extended capabilities (04h[20])
    if ((regVal & ((U32)1 << 20)) == 0)
    {
        return 0;
    }

    // Set capability pointer offset
    if (bPCIeCap)
    {
        // PCIe capabilities must start at 100h
        offset = 0x100;

        // Ignore instance number for non-VSEC capabilities
        if (CapID != PCIE_CAP_ID_VENDOR_SPECIFIC)
        {
            InstanceNum = 0;
        }
    }
    else
    {
        // Get offset of first capability from capability pointer (34h[7:0])
        PLX_PCI_REG_READ( pDevice, PCI_REG_CAP_PTR, &regVal );

        // Set first capability offset
        offset = (U8)regVal;

        // Ignore instance number for non-VSEC capabilities
        if (CapID != PCI_CAP_ID_VENDOR_SPECIFIC)
        {
            InstanceNum = 0;
        }
    }

    // Start with 1st match
    matchCount = 0;

    // Traverse capability list searching for desired ID
    while ((offset != 0) && (regVal != PCI_CFG_RD_ERR_VAL))
    {
        if ( (offset == 0xB0) &&
             ((pDevice->Key.PlxFamily == PLX_FAMILY_ATLAS) ||
              (pDevice->Key.PlxFamily == PLX_FAMILY_ATLAS_2) ||
              (pDevice->Key.PlxFamily == PLX_FAMILY_ATLAS2_LLC)) &&
             (pDevice->Key.ApiMode != PLX_API_MODE_PCI) )
        {
            if ( (pDevice->Key.PlxFamily == PLX_FAMILY_ATLAS) && 
                 ((pDevice->Key.PlxPort == PLX_FLAG_PORT_MPT1) ||
                  (pDevice->Key.PlxPort == PLX_FLAG_PORT_MPT2) ||
                  (pDevice->Key.PlxPort == PLX_FLAG_PORT_MPT3)) )
            {
                // MPT1-3 break the capability list at B0h (VPD), so override
                regVal = 0x0004803;
            }
            else if ( (pDevice->Key.PlxFamily == PLX_FAMILY_ATLAS_2 ||
                       pDevice->Key.PlxFamily == PLX_FAMILY_ATLAS2_LLC) && 
                      ((pDevice->Key.PlxPort == PLX_FLAG_PORT_ATLAS2_MPT1) ||
                       (pDevice->Key.PlxPort == PLX_FLAG_PORT_ATLAS2_MPT2) ||
                       (pDevice->Key.PlxPort == PLX_FLAG_PORT_ATLAS2_MPT3) ||
                       (pDevice->Key.PlxPort == PLX_FLAG_PORT_ATLAS2_MPT4) ||
                       (pDevice->Key.PlxPort == PLX_FLAG_PORT_ATLAS2_MPT5) ||
                       (pDevice->Key.PlxPort == PLX_FLAG_PORT_ATLAS2_MPT6) ||
                       (pDevice->Key.PlxPort == PLX_FLAG_PORT_ATLAS2_MPT7)) )
            {
                // Atlas2 MPT1-7 break the capability list at B0h (VPD), so override
                regVal = 0x0004803;
            }
            else
            {
                // Get next capability
                PLX_PCI_REG_READ(pDevice, offset, &regVal);
            }
        }
        else
        {
            // Get next capability
            PLX_PCI_REG_READ( pDevice, offset, &regVal );
        }

        // Verify capability is valid
        if ((regVal == 0) || (regVal == PCI_CFG_RD_ERR_VAL))
        {
            return 0;
        }

        // Extract the capability ID
        if (bPCIeCap)
        {
            // PCIe ID in [15:0]
            currID = (U16)((regVal >> 0) & 0xFFFF);
        }
        else
        {
            // PCI ID in [7:0]
            currID = (U16)((regVal >> 0) & 0xFF);
        }

        // Compare with desired capability
        if (currID == CapID)
        {
            // Verify correct instance
            if (InstanceNum == matchCount)
            {
                // Capability found, return base offset
                return offset;
            }

            // Increment count of matches
            matchCount++;
        }

        // Jump to next capability
        if (bPCIeCap)
        {
            // PCIe next cap offset in [31:20]
            offset = (U16)((regVal >> 20) & 0xFFF);
        }
        else
        {
            // PCI next cap offset in [15:8]
            offset = (U8)((regVal >> 8) & 0xFF);
        }
    }

    // Capability not found
    return 0;
}




/******************************************************************************
 *
 * Function   : PlxDir_ChipTypeDetect
 *
 * Description: Attempts to determine PLX chip type and revision
 *
 ******************************************************************************/
BOOLEAN
PlxDir_ChipTypeDetect(
    PLX_DEVICE_OBJECT *pDevice
    )
{
    U8  bSearch;
    U8  bPCIeCap;
    U8  instanceNum;
    U8  offsetChipId;
    U16 capID;
    U16 vsecID;
    U16 offset;
    U16 devID;
    U32 regVal;


    // Default revision to PCI revision
    pDevice->Key.PlxRevision = pDevice->Key.Revision;

    // Default to search for 1st PCIe VSEC
    capID        = PCIE_CAP_ID_VENDOR_SPECIFIC;
    bPCIeCap     = TRUE;
    instanceNum  = 0;
    offsetChipId = 0x8; // PCIe VSEC: ID @ 08h in capability

    /*********************************************************************
     * Search device's PCI/PCIe VSEC capabilities for hard-coded ID. VSEC
     * ID capability start offset varies between different chips. Most
     * possible offsets include DCh, E0h, 950h, & B70h.
     ********************************************************************/
    bSearch = TRUE;
    do
    {
        // Get next VSEC capability instance
        offset =
            PlxDir_PciFindCapability(
                pDevice,
                capID,
                bPCIeCap,
                instanceNum
                );

        if (offset == 0)
        {
            // No VSEC found
            if (bPCIeCap == FALSE)
            {
                // Have already scanned both PCI/PCIe, halt search
                bSearch = FALSE;
            }
            else
            {
                // PCIe VSEC search ended, jump to PCI
                capID        = PCI_CAP_ID_VENDOR_SPECIFIC;
                bPCIeCap     = FALSE;
                instanceNum  = 0;
                offsetChipId = 0x18; // PCI VSEC: ID @ 18h in capability
            }
        }
        else
        {
            // Check VSEC-specific version
            if (bPCIeCap == TRUE)
            {
                // 4h[15:0] is used for VSEC ID per PCIe
                PLX_PCI_REG_READ(
                    pDevice,
                    offset + sizeof(U32),
                    &regVal
                    );
                vsecID = (U16)(regVal >> 0);

                // Valid ID is 1
                if (vsecID != 1)
                {
                    offset = 0;
                }
            }
            else
            {
                // 0h[31:24] is used for VSEC ID since not used per PCI spec
                PLX_PCI_REG_READ( pDevice, offset, &regVal );
                vsecID = (U8)(regVal >> 24);

                // Valid IDs are 0 or 1 & is final capability
                if ( ((vsecID != 0) && (vsecID != 1)) ||
                     ((U8)(regVal >> 8) != 0) )
                {
                    offset = 0;
                }
            }

            // If VSEC ID is valid, continue to check for hard-coded ID
            if (offset != 0)
            {
                // Set to chip ID offset
                offset += offsetChipId;

                // Check for hard-coded ID
                PLX_PCI_REG_READ( pDevice, offset, &regVal );

                // Get device ID
                devID = (U16)(regVal >> 16);

                // Check vendor ID ([15:0])
                regVal &= 0xFFFF;
                if ( (regVal == PLX_PCI_VENDOR_ID_PLX) ||
                     (regVal == PLX_PCI_VENDOR_ID_LSI) )
                {
                    pDevice->Key.PlxChip = devID;

                    // Get hard-coded revision ID, if supported
                    if ((pDevice->Key.PlxChip != 0x8612) &&
                        (pDevice->Key.PlxChip != 0x8616) &&
                        (pDevice->Key.PlxChip != 0x8624) &&
                        (pDevice->Key.PlxChip != 0x8632) &&
                        (pDevice->Key.PlxChip != 0x8647) &&
                        (pDevice->Key.PlxChip != 0x8648))
                    {
                        // Revision should be in next register
                        PLX_PCI_REG_READ( pDevice, offset + sizeof(U32), &regVal );
                        pDevice->Key.PlxRevision = (U8)(regVal & 0xFF);

                        // Newer chips also report a chip ID (A0xxh) in [31:16]
                        if ((pDevice->Key.PlxChip & 0xFF00) == 0xC000)
                        {
                            pDevice->Key.ChipID = (U16)(regVal >> 16);

                            // Override revision ID if necessary
                            if (pDevice->Key.PlxRevision == 0xAA)
                            {
                                pDevice->Key.PlxRevision = 0xA0;
                            }
                        }
                    }

                    // Override MPT EP chip
                    if (pDevice->Key.PlxChip == 0x00B2)
                    {
                        // Use the device or subsystem ID, depending upon which is C0xxh
                        if ((pDevice->Key.DeviceId & 0xFF00) == 0xC000)
                        {
                            pDevice->Key.PlxChip = pDevice->Key.DeviceId;
                        }
                        else if ((pDevice->Key.SubDeviceId & 0xFF00) == 0xC000)
                        {
                            pDevice->Key.PlxChip = pDevice->Key.SubDeviceId;
                        }
                        else
                        {
                            // MPT in BSW doesn't report C0xx in config space, so just pick one
                            pDevice->Key.PlxChip = 0xC010;
                        }
                    }

                    // Skip to assigning family
                    goto _PlxChipAssignFamily;
                }
            }

            // VSEC does not contain hard-coded ID, so try next instance
            instanceNum++;
        }
    }
    while (bSearch == TRUE);

    //
    // Hard-coded ID doesn't exist, revert to Device/Vendor ID
    //

    // Get current device ID
    devID = pDevice->Key.DeviceId;

    // Group some devices
    if ((pDevice->Key.VendorId == PLX_PCI_VENDOR_ID_PLX) ||
        (pDevice->Key.VendorId == PLX_PCI_VENDOR_ID_LSI))
    {
        switch (devID & 0xFF00)
        {
            case 0x2300:
            case 0x3300:
            case 0x8500:
            case 0x8600:
            case 0x8700:
            case 0x9700:
            case 0xC000:
            case 0x8100:
                // Don't include 8311 RDK
                if (devID != 0x86e1)
                {
                    devID = 0x8000;
                }
                break;
        }
    }

    // Compare Device/Vendor ID
    switch (((U32)devID << 16) | pDevice->Key.VendorId)
    {
        case 0x800010B5:        // All 8000 series devices
        case 0x80001000:        // All Avago devices
            pDevice->Key.PlxChip = pDevice->Key.DeviceId;

            // For DMA & NT-Virtual with special ID, use subsystem ID
            if ((pDevice->Key.DeviceId == 0x87B0) ||
                (pDevice->Key.DeviceId == 0x87D0))
            {
                pDevice->Key.PlxChip = pDevice->Key.SubDeviceId;
            }
            break;

        case 0x00B21000:
            // Added for base mode MPT that doesn't yet have PCIe VSEC
            if ( (pDevice->Key.SubVendorId == PLX_PCI_VENDOR_ID_LSI) &&
                 ((pDevice->Key.SubDeviceId & 0xFF00) == 0xA000) )
            {
                pDevice->Key.PlxChip = 0xC010;
            }
            break;

        case 0x905010b5:        // 9050/9052
        case 0x520110b5:        // PLX 9052 RDK
            pDevice->Key.PlxChip = 0x9050;
            break;

        case 0x903010b5:        // 9030
        case 0x300110b5:        // PLX 9030 RDK
        case 0x30c110b5:        // PLX 9030 RDK - cPCI
            pDevice->Key.PlxChip = 0x9030;
            break;

        case 0x908010b5:        // 9080
        case 0x040110b5:        // PLX 9080-401B RDK
        case 0x086010b5:        // PLX 9080-860 RDK
            pDevice->Key.PlxChip = 0x9080;
            break;

        case 0x905410b5:        // 9054
        case 0x540610b5:        // PLX 9054 RDK-LITE
        case 0x186010b5:        // PLX 9054-860 RDK
        case 0xc86010b5:        // PLX 9054-860 RDK - cPCI
            pDevice->Key.PlxChip = 0x9054;
            break;

        case 0x905610b5:        // 9056
        case 0x560110b5:        // PLX 9056 RDK-LITE
        case 0x56c210b5:        // PLX 9056-860 RDK
            pDevice->Key.PlxChip = 0x9056;
            break;

        case 0x965610b5:        // 9656
        case 0x960110b5:        // PLX 9656 RDK-LITE
        case 0x96c210b5:        // PLX 9656-860 RDK
            pDevice->Key.PlxChip = 0x9656;
            break;

        case 0x831110b5:        // 8311
        case 0x86e110b5:        // PLX 8311 RDK
            pDevice->Key.PlxChip = 0x8311;
            break;

        case 0x00213388:        // 6140/6152/6254(NT)
            // Get PCI header type
            PLX_PCI_REG_READ( pDevice, PCI_REG_HDR_CACHE_LN, &regVal );
            regVal = (U8)((regVal >> 16) & 0x7F);

            if (regVal == PCI_HDR_TYPE_0)
            {
                pDevice->Key.PlxChip = 0x6254;
            }
            else
            {
                // Get 6152 VPD register
                PLX_PCI_REG_READ( pDevice, 0xA0, &regVal );

                if ((regVal & 0xF) == PCI_CAP_ID_VPD)
                {
                    pDevice->Key.PlxChip = 0x6152;
                }
                else
                {
                    pDevice->Key.PlxChip = 0x6140;
                }
            }
            break;

        case 0x00223388:        // 6150/6350
        case 0x00a23388:        // 6350
            if (pDevice->Key.PlxRevision == 0x20)
            {
                pDevice->Key.PlxChip = 0x6350;
            }
            else
            {
                pDevice->Key.PlxChip = 0x6150;
            }
            break;

        case 0x00263388:        // 6154
            pDevice->Key.PlxChip = 0x6154;
            break;

        case 0x00313388:        // 6156
            pDevice->Key.PlxChip = 0x6156;
            break;

        case 0x00203388:        // 6254
            pDevice->Key.PlxChip = 0x6254;
            break;

        case 0x00303388:        // 6520
        case 0x652010B5:        // 6520
            pDevice->Key.PlxChip = 0x6520;
            break;

        case 0x00283388:        // 6540      - Transparent mode
        case 0x654010B5:        // 6540/6466 - Transparent mode
        case 0x00293388:        // 6540      - Non-transparent mode
        case 0x654110B5:        // 6540/6466 - Non-transparent mode
        case 0x654210B5:        // 6540/6466 - Non-transparent mode
            pDevice->Key.PlxChip = 0x6540;
            break;

        // Cases where PCIe regs not accessible, use SubID
        case 0x100810B5:        // PLX Synthetic Enabler EP
        case 0x100910B5:        // PLX GEP
            pDevice->Key.PlxChip = pDevice->Key.SubDeviceId;
            break;
    }

    // Detect the PLX chip revision
    PlxDir_ChipRevisionDetect( pDevice );

_PlxChipAssignFamily:

    // Set chip ID to match chip if not supported
    if (pDevice->Key.ChipID == 0x0)
    {
        pDevice->Key.ChipID = pDevice->Key.PlxChip;
    }

    switch (pDevice->Key.PlxChip)
    {
        case 0x9050:        // 9000 series
        case 0x9030:
        case 0x9080:
        case 0x9054:
        case 0x9056:
        case 0x9656:
        case 0x8311:
            pDevice->Key.PlxFamily = PLX_FAMILY_BRIDGE_P2L;
            break;

        case 0x6140:        // 6000 series
        case 0x6150:
        case 0x6152:
        case 0x6154:
        case 0x6156:
        case 0x6254:
        case 0x6350:
        case 0x6520:
        case 0x6540:
        case 0x6466:
            pDevice->Key.PlxFamily = PLX_FAMILY_BRIDGE_PCI_P2P;
            break;

        case 0x8111:
        case 0x8112:
        case 0x8114:
            pDevice->Key.PlxFamily = PLX_FAMILY_BRIDGE_PCIE_P2P;
            break;

        case 0x8525:
        case 0x8533:
        case 0x8547:
        case 0x8548:
            pDevice->Key.PlxFamily = PLX_FAMILY_ALTAIR;
            break;

        case 0x8505:
        case 0x8509:
            pDevice->Key.PlxFamily = PLX_FAMILY_ALTAIR_XL;
            break;

        case 0x8516:
        case 0x8524:
        case 0x8532:
            pDevice->Key.PlxFamily = PLX_FAMILY_VEGA;
            break;

        case 0x8508:
        case 0x8512:
        case 0x8517:
        case 0x8518:
            pDevice->Key.PlxFamily = PLX_FAMILY_VEGA_LITE;
            break;

        case 0x8612:
        case 0x8616:
        case 0x8624:
        case 0x8632:
        case 0x8647:
        case 0x8648:
            pDevice->Key.PlxFamily = PLX_FAMILY_DENEB;
            break;

        case 0x8604:
        case 0x8606:
        case 0x8608:
        case 0x8609:
        case 0x8613:
        case 0x8614:
        case 0x8615:
        case 0x8617:
        case 0x8618:
        case 0x8619:
            pDevice->Key.PlxFamily = PLX_FAMILY_SIRIUS;
            break;

        case 0x8625:
        case 0x8636:
        case 0x8649:
        case 0x8664:
        case 0x8680:
        case 0x8696:
            pDevice->Key.PlxFamily = PLX_FAMILY_CYGNUS;
            break;

        case 0x8700:
            // DMA devices that don't have hard-coded ID
            if ((pDevice->Key.DeviceId == 0x87D0) || (pDevice->Key.DeviceId == 0x87E0))
            {
                pDevice->Key.PlxFamily = PLX_FAMILY_DRACO_1;
            }
            else
            {
                pDevice->Key.PlxFamily = PLX_FAMILY_SCOUT;
            }
            break;

        case 0x8712:
        case 0x8716:
        case 0x8723:
        case 0x8724:
        case 0x8732:
        case 0x8747:
        case 0x8748:
            if (pDevice->Key.PlxRevision == 0xAA)
            {
                pDevice->Key.PlxFamily = PLX_FAMILY_DRACO_1;
            }
            else
            {
                pDevice->Key.PlxFamily = PLX_FAMILY_DRACO_2;
            }
            break;

        case 0x8713:
        case 0x8717:
        case 0x8725:
        case 0x8733:
        case 0x8749:
            pDevice->Key.PlxFamily = PLX_FAMILY_DRACO_2;
            break;

        case 0x2380:
        case 0x3380:
        case 0x3382:
        case 0x8603:
        case 0x8605:
            pDevice->Key.PlxFamily = PLX_FAMILY_MIRA;
            break;

        case 0x8714:
        case 0x8718:
        case 0x8734:
        case 0x8750:
        case 0x8764:
        case 0x8780:
        case 0x8796:
            pDevice->Key.PlxFamily = PLX_FAMILY_CAPELLA_1;
            break;

        case 0x9712:
        case 0x9713:
        case 0x9716:
        case 0x9717:
        case 0x9733:
        case 0x9734:
        case 0x9749:
        case 0x9750:
        case 0x9765:
        case 0x9766:
        case 0x9781:
        case 0x9782:
        case 0x9797:
        case 0x9798:
            pDevice->Key.PlxFamily = PLX_FAMILY_CAPELLA_2;
            break;

        case 0xC010:
        case 0xC011:
        case 0xC012:
            pDevice->Key.PlxFamily = PLX_FAMILY_ATLAS;
            break;

        case 0xC030:
            pDevice->Key.PlxFamily = PLX_FAMILY_ATLAS_2;
            break;

        case 0xC034:
            pDevice->Key.PlxFamily = PLX_FAMILY_ATLAS2_LLC;
            break;

        case 0:
            pDevice->Key.PlxFamily = PLX_FAMILY_NONE;
            break;

        default:
            ErrorPrintf(("ERROR - PLX Family not set for %04X\n", pDevice->Key.PlxChip));
            pDevice->Key.PlxFamily = PLX_FAMILY_UNKNOWN;
            break;
    }

    return TRUE;
}




/******************************************************************************
 *
 * Function   : PlxDir_ChipRevisionDetect
 *
 * Description: Attempts to detect the PLX chip revision
 *
 ******************************************************************************/
VOID
PlxDir_ChipRevisionDetect(
    PLX_DEVICE_OBJECT *pDevice
    )
{
    U32 regVal;


    // Get PCI Class code/Revision ID register
    PLX_PCI_REG_READ( pDevice, PCI_REG_CLASS_REV, &regVal );

    // Default revision to value in chip
    pDevice->Key.PlxRevision = (U8)(regVal & 0xFF);
}




/******************************************************************************
 *
 * Function   : PlxDir_ChipFilterDisabledPorts
 *
 * Description: Probes a chip for disabled ports & removes them from port mask
 *
 ******************************************************************************/
PLX_STATUS
PlxDir_ChipFilterDisabledPorts(
    PLX_DEVICE_OBJECT *pDevice,
    PEX_CHIP_FEAT     *PtrFeat
    )
{
    U8  index;
    U8  maxPorts;
    U32 offset;
    U32 regVal;
    U32 portDW;


    // Set default max ports
    maxPorts = 24;

    // Set register to use for port enabled status
    switch (pDevice->Key.PlxFamily)
    {
        case PLX_FAMILY_ALTAIR:
        case PLX_FAMILY_ALTAIR_XL:
        case PLX_FAMILY_VEGA:
        case PLX_FAMILY_VEGA_LITE:
        case PLX_FAMILY_DENEB:
        case PLX_FAMILY_SIRIUS:
            offset = 0x668;
            break;

        case PLX_FAMILY_MIRA:
            offset = 0x1D8;
            break;

        case PLX_FAMILY_CYGNUS:
        case PLX_FAMILY_SCOUT:
        case PLX_FAMILY_DRACO_1:
        case PLX_FAMILY_DRACO_2:
            offset = 0x314;
            break;

        case PLX_FAMILY_CAPELLA_1:
        case PLX_FAMILY_CAPELLA_2:
            if (pDevice->Key.DeviceMode == PLX_CHIP_MODE_STANDARD)
            {
                offset = 0xF4C;
            }
            else
            {
                offset = 0x30C;
            }
            break;

        case PLX_FAMILY_ATLAS:
            offset = ATLAS_REG_PORT_CLOCK_EN_0;
            if (pDevice->Key.ApiMode != PLX_API_MODE_PCI)
            {
                offset +=  ATLAS_REGS_AXI_BASE_ADDR;
            }
            maxPorts = 128;
            break;

        case PLX_FAMILY_ATLAS_2:
        case PLX_FAMILY_ATLAS2_LLC:
            offset = ATLAS_REG_PORT_CLOCK_EN_0;
            if (pDevice->Key.ApiMode != PLX_API_MODE_PCI)
            {
                offset +=  ATLAS_REGS_AXI_BASE_ADDR;
            }
            maxPorts = 160;
            break;

        default:
            ErrorPrintf(("ERROR - Disabled port filter not implemented (%04X)\n", pDevice->Key.PlxChip));
            return PLX_STATUS_UNSUPPORTED;
    }

    // Round up to ensure DW alignment
    maxPorts = PEX_P2_ROUND_UP( maxPorts, 32 );

    // Traverse port clock registers to remove disabled ports
    for (index = 0; index < (maxPorts / 32); index++)
    {
        // Get port enable status
        regVal = PlxDir_PlxMappedRegRead( pDevice, offset, NULL );

        // For Atlas, ports 116/117 should up in port 96/97 bit locations
        if ( (pDevice->Key.PlxFamily == PLX_FAMILY_ATLAS) && (index == 3) )
        {
            regVal <<= 20;  // Shift into correct bit position
        }

        // Get enabled port mask
        portDW = PEX_BITMASK_GET_DW( PtrFeat->PortMask, index );

        // Remove disabled ports
        portDW &= regVal;

        // Update enabled ports
        PEX_BITMASK_SET_DW( PtrFeat->PortMask, index, portDW );

        // Set offset to next block
        offset += sizeof(U32);

        // For Atlas 2, port clock register for ports 128-143 are at offset 324h
        if ( (pDevice->Key.PlxFamily == PLX_FAMILY_ATLAS_2 || 
              pDevice->Key.PlxFamily == PLX_FAMILY_ATLAS2_LLC) && (index == 3) )
        {
            offset = ATLAS_REGS_AXI_BASE_ADDR + ATLAS2_REG_PORT_CLOCK_EN_4;
        }
    }

    return PLX_STATUS_OK;
}




/******************************************************************************
 *
 * Function   : PlxDir_ProbeSwitch
 *
 * Description: Probes a switch over active out-of-band interface to find active ports
 *
 ******************************************************************************/
PLX_STATUS
PlxDir_ProbeSwitch(
    PLX_DEVICE_OBJECT *pDevice,
    PLX_DEVICE_KEY    *pKey,
    U16                DeviceNumber,
    U16               *pNumMatched
    )
{
    U8             port_Upstream;
    U8             pciHeaderType;
    U8             stnPort;
    U16            pexChip;
    U16            portNum;
    U16            devCount;
    U32            offset;
    U32            devVenID;
    U32            regVal;
    U32            atlasSwMode;
    BOOLEAN        bMatchId;
    BOOLEAN        bMatchLoc;
    PLX_STATUS     status;
    PLX_PORT_PROP  portProp;
    PEX_CHIP_FEAT  chipFeat;
    PLX_DEVICE_KEY tempKey;


    devCount      = 0;
    port_Upstream = (U8)-1;

    // Always reset key but save some properties temporarily to restore
    tempKey = pDevice->Key;
    RtlZeroMemory( &pDevice->Key, sizeof(PLX_DEVICE_KEY) );
    pDevice->Key.ApiIndex       = tempKey.ApiIndex;
    pDevice->Key.DeviceNumber   = tempKey.DeviceNumber;
    pDevice->Key.ApiMode        = tempKey.ApiMode;
    pDevice->Key.DeviceMode     = tempKey.DeviceMode;
    pDevice->Key.ApiInternal[0] = tempKey.ApiInternal[0];
    pDevice->Key.ApiInternal[1] = tempKey.ApiInternal[1];

    // Start with port 0
    portNum = 0;

    // Start with unknown chip type
    pexChip = 0;

    // Start with empty Device ID
    devVenID = 0;

    // Initially allow access only to port 0
    RtlZeroMemory( &chipFeat, sizeof(PEX_CHIP_FEAT) );
    PEX_BITMASK_SET( chipFeat.PortMask, 0 );

    // Probe all possible ports in the switch
    while (portNum < (sizeof(chipFeat.PortMask) * 8))
    {
        // Skip probe if port is known not to exist
        if (PEX_BITMASK_TEST( chipFeat.PortMask, portNum ) == FALSE)
        {
            status = PLX_STATUS_FAILED;
        }
        else
        {
            // Default to unknown port type
            pDevice->Key.PlxPortType = PLX_SPEC_PORT_UNKNOWN;

            // Store the port
            pDevice->Key.PlxPort = (U8)portNum;

            // Set port offset
            pDevice->Key.ApiInternal[1] = (U32)portNum * PEX_PORT_REGS_SIZE;

            // Get Device/Vendor ID
            devVenID = PlxDir_PlxRegRead( pDevice, PCI_REG_DEV_VEN_ID, &status );
        }

        // Verify access worked and device is supported
        if ( (status != PLX_STATUS_OK) ||
             (((devVenID & 0x0000FFFF) != PLX_PCI_VENDOR_ID_PLX) &&
              ((devVenID & 0x0000FFFF) != PLX_PCI_VENDOR_ID_LSI)) )
        {
            // Skip to next port
            portNum++;
            continue;
        }

        DebugPrintf((
            "%s: Port %d detected (ID=%08X)\n",
            DbgGetApiModeStr( pDevice ), portNum, devVenID
            ));

        // Update possible ports
        if (pexChip == 0)
        {
            // Set chip type & revision
            if (pDevice->Key.PlxChip == 0)
            {
                // Set device ID in case needed by chip detection algorithm
                pDevice->Key.DeviceId = (U16)(devVenID >> 16);

                // Probe to determine chip type
                PlxDir_ChipTypeDetect( pDevice );
            }

            // Store connected chip
            if (pDevice->Key.PlxChip != 0)
            {
                pexChip = pDevice->Key.PlxChip;
            }

            // Get chip port mask
            PlxPci_ChipGetPortMask(
                pDevice->Key.ChipID,
                pDevice->Key.PlxRevision,
                &chipFeat
                );

            // For first port, perform additional probing
            if (portNum == 0)
            {
                switch (pDevice->Key.PlxFamily)
                {
                    case PLX_FAMILY_ATLAS:
                    case PLX_FAMILY_ATLAS_2:
                    case PLX_FAMILY_ATLAS2_LLC:

                        // Determine switch mode (CCR B0h[1:0])
                        atlasSwMode =
                            PlxDir_PlxMappedRegRead(
                                pDevice,
                                ATLAS_REG_CCR_PCIE_SW_MODE,
                                NULL
                                );

                        atlasSwMode &= 0x3;
                        if (atlasSwMode == 0)
                        {
                            pDevice->Key.DeviceMode = PLX_CHIP_MODE_STANDARD;

                            // Store upstream port number (360h[7:0])
                            regVal =
                                PlxDir_PlxRegRead(
                                    pDevice,
                                    ATLAS_REG_VS0_UPSTREAM,
                                    NULL
                                    );
                            port_Upstream = (U8)((regVal >> 0) & 0xFF);
                        }
                        else if (atlasSwMode == 1)
                        {
                            pDevice->Key.DeviceMode = PLX_CHIP_MODE_FABRIC;

                            if (pDevice->Key.PlxFamily == PLX_FAMILY_ATLAS)
                            {
                                // Store management port number (CCR 170h[15:8])
                                regVal =
                                    PlxDir_PlxMappedRegRead(
                                        pDevice,
                                        ATLAS_REG_CCR_PCIE_CONFIG,
                                        NULL
                                        );
                                port_Upstream = (U8)((regVal >> 8) & 0xFF);
                            }
                            else
                            {
                                // For Atlas2, store management port number (CCR 1A4h[0:7])
                                regVal =
                                    PlxDir_PlxMappedRegRead(
                                        pDevice,
                                        ATLAS2_REG_CCR_UPSTREAM_PORT,
                                        NULL
                                        );
                                port_Upstream = (U8)(regVal & 0xFF);
                            }
                        }
                        else if ((pDevice->Key.PlxFamily == PLX_FAMILY_ATLAS_2 ||
                                  pDevice->Key.PlxFamily == PLX_FAMILY_ATLAS2_LLC) &&
                                 (atlasSwMode == 2))
                        {
                            pDevice->Key.DeviceMode = PLX_CHIP_MODE_STANDARD;

                            // Store upstream port number (360h[7:0])
                            regVal =
                                PlxDir_PlxRegRead(
                                    pDevice,
                                    ATLAS_REG_VS0_UPSTREAM,
                                    NULL
                                    );
                            port_Upstream = (U8)((regVal >> 0) & 0xFF);
                        }
                        else
                        {
                            pDevice->Key.DeviceMode = PLX_CHIP_MODE_UNKNOWN;
                            port_Upstream = 0;
                        }
                        break;

                    default:
                        ErrorPrintf((
                            "ERROR: Chip mode & UP/NT detection not implemented for %04X\n",
                            pDevice->Key.PlxChip
                            ));
                        return PLX_STATUS_UNSUPPORTED;
                }

                // Remove disabled ports to avoid probing them
                // NOTE: Must be called after Key.DeviceMode is set since may refer to it
                PlxDir_ChipFilterDisabledPorts( pDevice, &chipFeat );

                // If not SSW mode, remove probe for non-existent devices (GEP, MPT, etc)
                if (pDevice->Key.DeviceMode != PLX_CHIP_MODE_FABRIC)
                {
                    PEX_BITMASK_CLEAR( chipFeat.PortMask, PLX_FLAG_PORT_INT_MGMT );
                    PEX_BITMASK_CLEAR( chipFeat.PortMask, PLX_FLAG_PORT_GEP );
                    PEX_BITMASK_CLEAR( chipFeat.PortMask, PLX_FLAG_PORT_GEP_DS );
                    PEX_BITMASK_CLEAR( chipFeat.PortMask, PLX_FLAG_PORT_MPT1);
                    PEX_BITMASK_CLEAR( chipFeat.PortMask, PLX_FLAG_PORT_MPT2 );
                    PEX_BITMASK_CLEAR( chipFeat.PortMask, PLX_FLAG_PORT_MPT3 );
                    PEX_BITMASK_CLEAR( chipFeat.PortMask, PLX_FLAG_PORT_ATLAS2_MPT1 );
                    PEX_BITMASK_CLEAR( chipFeat.PortMask, PLX_FLAG_PORT_ATLAS2_MPT2 );
                    PEX_BITMASK_CLEAR( chipFeat.PortMask, PLX_FLAG_PORT_ATLAS2_MPT3 );
                    PEX_BITMASK_CLEAR( chipFeat.PortMask, PLX_FLAG_PORT_ATLAS2_MPT4 );
                    PEX_BITMASK_CLEAR( chipFeat.PortMask, PLX_FLAG_PORT_ATLAS2_MPT5 );
                    PEX_BITMASK_CLEAR( chipFeat.PortMask, PLX_FLAG_PORT_ATLAS2_MPT6 );
                    PEX_BITMASK_CLEAR( chipFeat.PortMask, PLX_FLAG_PORT_ATLAS2_MPT7 );

                    // For Atlas 2, SW Mode 0 is pure base mode which does not
                    // expose the MPT endpoint to the host.
                    if (( pDevice->Key.PlxFamily == PLX_FAMILY_ATLAS_2 ||
                          pDevice->Key.PlxFamily == PLX_FAMILY_ATLAS2_LLC ) &&
                        ( atlasSwMode == 0 ))
                    {
                        PEX_BITMASK_CLEAR( chipFeat.PortMask, PLX_FLAG_PORT_ATLAS2_MPT0 ); 
                    }
                }

                DebugPrintf((
                    "%s: Chip config: %d stn %d ports/stn StnMask=%04Xh\n",
                    DbgGetApiModeStr( pDevice ), chipFeat.StnCount,
                    chipFeat.PortsPerStn, chipFeat.StnMask
                    ));

                DebugPrintf((
                    "%s: ChipMode=%s  UP=%d(%02Xh)\n",
                    DbgGetApiModeStr( pDevice ),
                    (pDevice->Key.DeviceMode == PLX_CHIP_MODE_STANDARD) ? "BSW" :
                      (pDevice->Key.DeviceMode == PLX_CHIP_MODE_STD_LEGACY_NT) ? "STD NT" :
                      (pDevice->Key.DeviceMode == PLX_CHIP_MODE_STD_NT_DS_P2P) ? "NT P2P" :
                      (pDevice->Key.DeviceMode == PLX_CHIP_MODE_VIRT_SW) ? "VS" :
                      (pDevice->Key.DeviceMode == PLX_CHIP_MODE_FABRIC) ? "SSW" :
                      (pDevice->Key.DeviceMode == PLX_CHIP_MODE_LEGACY_ADAPTER) ? "LEGACY" :
                      "UNKOWN",
                    port_Upstream, port_Upstream
                    ));
            }
        }

        // Get PCI header type
        regVal = PlxDir_PlxRegRead( pDevice, PCI_REG_HDR_CACHE_LN, &status );
        if (status != PLX_STATUS_OK)
        {
            // Set register value to flag failure
            regVal = PCI_CFG_RD_ERR_VAL_32;
        }

        // If read failed, skip to next port
        if (regVal == PCI_CFG_RD_ERR_VAL_32)
        {
            portNum++;
            continue;
        }

        DebugPrintf((
            "%s: Port %d is enabled, get additional properties\n",
            DbgGetApiModeStr( pDevice ), portNum
            ));

        // Set header type field
        pciHeaderType = (U8)((regVal >> 16) & 0x7F);

        // Fill in device key
        pDevice->Key.DeviceId = (U16)(devVenID >> 16);
        pDevice->Key.VendorId = (U16)devVenID;

        // Get port properties
        PlxDir_GetPortProperties( pDevice, &portProp );

        // Set PLX-specific port type
        if (portProp.PortType == PLX_PORT_UPSTREAM)
        {
            if (portNum == PLX_FLAG_PORT_INT_MGMT)
            {
                pDevice->Key.PlxPortType = PLX_SPEC_PORT_INT_MGMT;
            }
            else if ( (pDevice->Key.DeviceMode == PLX_CHIP_MODE_FABRIC) &&
                      (portNum < PEX_MAX_PORT) && (port_Upstream != portNum) )
            {
                // In fabric mode, upstream ports other than chip UP are host ports
                pDevice->Key.PlxPortType = PLX_SPEC_PORT_HOST;
            }
            else
            {
                pDevice->Key.PlxPortType = PLX_SPEC_PORT_UPSTREAM;
            }
        }
        else if (portProp.PortType == PLX_PORT_DOWNSTREAM)
        {
            // Default to DS port
            pDevice->Key.PlxPortType = PLX_SPEC_PORT_DOWNSTREAM;

            // Could be a fabric port in fabric mode
            if ( (portNum < PEX_MAX_PORT) &&
                 (pDevice->Key.DeviceMode == PLX_CHIP_MODE_FABRIC) )
            {
                // Calculate offset for port's configuration
                stnPort = portNum % chipFeat.PortsPerStn;
                offset  = ATLAS_REG_CCR_PORT_TYPE0 +
                          ((portNum / chipFeat.PortsPerStn) * sizeof(U32));

                regVal = PlxDir_PlxMappedRegRead( pDevice, offset, NULL );

                // Get port type in [1:0] (01 = Fabric)
                regVal = ((regVal >> ((stnPort % 16) * 2)) & 0x3);
                if (regVal == 1)
                {
                    pDevice->Key.PlxPortType = PLX_SPEC_PORT_FABRIC;
                }
            }
        }
        else if (portProp.PortType == PLX_PORT_ENDPOINT)
        {
            if ( (portNum == PLX_FLAG_PORT_GEP) ||
                 (pDevice->Key.DeviceId == 0x1009) )
            {
                pDevice->Key.PlxPortType = PLX_SPEC_PORT_GEP;
            }
            else if ( (portNum == PLX_FLAG_PORT_MPT0) ||
                      (portNum == PLX_FLAG_PORT_MPT1) ||
                      (portNum == PLX_FLAG_PORT_MPT2) ||
                      (portNum == PLX_FLAG_PORT_MPT3) ||
                      (portNum == PLX_FLAG_PORT_ATLAS2_MPT0) ||
                      (portNum == PLX_FLAG_PORT_ATLAS2_MPT1) ||
                      (portNum == PLX_FLAG_PORT_ATLAS2_MPT2) ||
                      (portNum == PLX_FLAG_PORT_ATLAS2_MPT3) ||
                      (portNum == PLX_FLAG_PORT_ATLAS2_MPT4) ||
                      (portNum == PLX_FLAG_PORT_ATLAS2_MPT5) ||
                      (portNum == PLX_FLAG_PORT_ATLAS2_MPT6) ||
                      (portNum == PLX_FLAG_PORT_ATLAS2_MPT7) )
            {
                if (pDevice->Key.DeviceId == 0x00B2)
                {
                    pDevice->Key.PlxPortType = PLX_SPEC_PORT_MPT;
                }
                else
                {
                    pDevice->Key.PlxPortType = PLX_SPEC_PORT_MPT_NO_SES;
                }
            }
        }

        // Reset device location
        pDevice->Key.bus      = 0;
        pDevice->Key.slot     = 0;
        pDevice->Key.function = 0;

        // Attempt to determine bus number
        if (pciHeaderType == PCI_HDR_TYPE_1)
        {
            regVal = PlxDir_PlxRegRead( pDevice, PCI_REG_T1_PRIM_SEC_BUS, NULL );

            // Bus is primary bus number field (18h[7:0])
            pDevice->Key.bus = (U8)(regVal >> 0);

            // Store the upstream port bus number
            if (pDevice->Key.PlxPortType == PLX_SPEC_PORT_UPSTREAM)
            {
                DebugPrintf((
                    "%s: UP bus=%02Xh\n",
                    DbgGetApiModeStr( pDevice ), pDevice->Key.bus
                    ));
            }

            // For host/fabric ports, use bus number assigned by manager in [15:8]
            if ((pDevice->Key.PlxPortType == PLX_SPEC_PORT_HOST) ||
                (pDevice->Key.PlxPortType == PLX_SPEC_PORT_FABRIC))
            {
                pDevice->Key.bus = (U8)(regVal >> 8);
            }
        }
        else
        {
            // Get endpoint captured bus (port 97Ch[7:0])
            regVal = PlxDir_PlxRegRead( pDevice, ATLAS_OFF_PORT_CAP_BUS, NULL );
            pDevice->Key.bus = (U8)regVal;
        }

        /***************************************************************
         * If devices aren't enumerated, DS ports & other device bus
         * numbers will not be assigned and will always end up on
         * bus 0. If that is the case, auto-generate bus numbers
         **************************************************************/
        if (pDevice->Key.bus == 0)
        {
            if (portNum == PLX_FLAG_PORT_INT_MGMT)
            {
                pDevice->Key.bus = 0x1;
            }
            else if ( (portNum == PLX_FLAG_PORT_INT_DS_0)  ||
                      (portNum == PLX_FLAG_PORT_INT_DS_4)  ||
                      (portNum == PLX_FLAG_PORT_INT_DS_8)  ||
                      (portNum == PLX_FLAG_PORT_INT_DS_12) ||
                      (portNum == PLX_FLAG_PORT_INT_DS_16) ||
                      (portNum == PLX_FLAG_PORT_GEP_DS) )
            {
                pDevice->Key.bus = 0x2;
            }
            else if ( (portNum == PLX_FLAG_PORT_INT_UP_0) ||
                      (portNum == PLX_FLAG_PORT_INT_UP_4) ||
                      (portNum == PLX_FLAG_PORT_INT_UP_8) ||
                      (portNum == PLX_FLAG_PORT_INT_UP_12) ||
                      (portNum == PLX_FLAG_PORT_INT_UP_16) )
            {
                // Lower fan out switches (10h,20h,30h..)
                pDevice->Key.bus =
                    (U8)0x10 + ((portNum - PLX_FLAG_PORT_INT_UP_0) * 0x10);
            }
            else if ( (pDevice->Key.PlxPortType == PLX_SPEC_PORT_HOST) ||
                      (pDevice->Key.PlxPortType == PLX_SPEC_PORT_FABRIC) )
            {
                // Host & fabric ports - Pick high bus number
                pDevice->Key.bus = 0x80 + pDevice->Key.PlxPort;
            }
            else if (pDevice->Key.PlxPortType == PLX_SPEC_PORT_UPSTREAM)
            {
                pDevice->Key.bus = 0x1;     // Standard upstream in BSW mode
            }
            else if (pDevice->Key.PlxPortType == PLX_SPEC_PORT_DOWNSTREAM)
            {
                // DS ports grouped under fan out switches (11h,21h,31h..)
                pDevice->Key.bus = (U8)(((portNum / PCI_MAX_DEV) * 0x10) + 0x11);
            }
            else if (pDevice->Key.PlxPortType == PLX_SPEC_PORT_GEP)
            {
                pDevice->Key.bus = 0x78;
            }
            else if ( (pDevice->Key.PlxFamily == PLX_FAMILY_ATLAS) &&
                      ((portNum == PLX_FLAG_PORT_MPT0) ||
                       (portNum == PLX_FLAG_PORT_MPT1) ||
                       (portNum == PLX_FLAG_PORT_MPT2) ||
                       (portNum == PLX_FLAG_PORT_MPT3)) )
            {
                pDevice->Key.bus = 0x79 + (U8)(portNum - PLX_FLAG_PORT_MPT0);
            }
            else if ( (pDevice->Key.PlxFamily == PLX_FAMILY_ATLAS_2) &&
                      ((portNum == PLX_FLAG_PORT_ATLAS2_MPT0) ||
                       (portNum == PLX_FLAG_PORT_ATLAS2_MPT1) ||
                       (portNum == PLX_FLAG_PORT_ATLAS2_MPT2) ||
                       (portNum == PLX_FLAG_PORT_ATLAS2_MPT3) ||
                       (portNum == PLX_FLAG_PORT_ATLAS2_MPT4) ||
                       (portNum == PLX_FLAG_PORT_ATLAS2_MPT5) ||
                       (portNum == PLX_FLAG_PORT_ATLAS2_MPT6) ||
                       (portNum == PLX_FLAG_PORT_ATLAS2_MPT7)) )
            {
                pDevice->Key.bus = 0x79 + (U8)(portNum - PLX_FLAG_PORT_ATLAS2_MPT0);
            }
            else
            {
                ErrorPrintf((
                    "ERROR: Bus # auto-gen not implemented for port type %d\n",
                    pDevice->Key.PlxPortType
                    ));
                pDevice->Key.bus = 0xFF;
            }
        }

        // Set non-0 slot numbers
        if ( (portNum == PLX_FLAG_PORT_INT_DS_0)  ||
             (portNum == PLX_FLAG_PORT_INT_DS_4)  ||
             (portNum == PLX_FLAG_PORT_INT_DS_8)  ||
             (portNum == PLX_FLAG_PORT_INT_DS_12) ||
             (portNum == PLX_FLAG_PORT_INT_DS_16) )
        {
            // Internal primary DS
            pDevice->Key.slot =
                 (U8)((portNum - PLX_FLAG_PORT_INT_DS_0) * 4);
        }
        else if (portNum == PLX_FLAG_PORT_GEP_DS)
        {
            pDevice->Key.slot = 0x1C;   // GEP parent DS
        }
        else if (pDevice->Key.PlxPortType == PLX_SPEC_PORT_DOWNSTREAM)
        {
            // Slot tied to port number
            pDevice->Key.slot = (U8)portNum % PCI_MAX_DEV;
        }

        // Get PCI Revision
        regVal = PlxDir_PlxRegRead( pDevice, PCI_REG_CLASS_REV, NULL );
        pDevice->Key.Revision = (U8)regVal;

        // Get subsystem device ID
        if (pciHeaderType == PCI_HDR_TYPE_0)
        {
            regVal = PlxDir_PlxRegRead( pDevice, PCI_REG_TO_SUBSYS_ID, NULL );
        }
        else
        {
            // Get subsytem ID from capability if supported
            offset =
                PlxDir_PciFindCapability(
                    pDevice,
                    PCI_CAP_ID_BRIDGE_SUB_ID,
                    FALSE,
                    0
                    );
            if (offset == 0)
            {
                regVal = 0;
            }
            else
            {
                regVal = PlxDir_PlxRegRead( pDevice, (U16)(offset + 0x04), NULL );
            }
        }

        pDevice->Key.SubDeviceId = (U16)(regVal >> 16);
        pDevice->Key.SubVendorId = (U16)regVal;

        // Assume successful match
        bMatchLoc = TRUE;
        bMatchId  = TRUE;

        //
        // Compare device key information
        //

        // Compare Bus, Slot, Fn numbers
        if ( (pKey->domain   != (U8)PCI_FIELD_IGNORE) ||
             (pKey->bus      != (U8)PCI_FIELD_IGNORE) ||
             (pKey->slot     != (U8)PCI_FIELD_IGNORE) ||
             (pKey->function != (U8)PCI_FIELD_IGNORE) )
        {
            if ( (pKey->domain   != pDevice->Key.domain) ||
                 (pKey->bus      != pDevice->Key.bus)    ||
                 (pKey->slot     != pDevice->Key.slot)   ||
                 (pKey->function != pDevice->Key.function) )
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
            if (pKey->VendorId != pDevice->Key.VendorId)
            {
                bMatchId = FALSE;
            }
        }

        // Compare Device ID
        if (pKey->DeviceId != (U16)PCI_FIELD_IGNORE)
        {
            if (pKey->DeviceId != pDevice->Key.DeviceId)
            {
                bMatchId = FALSE;
            }
        }

        // Compare Subsystem ID only if valid in chip
        if (pDevice->Key.SubVendorId != 0)
        {
            // Compare Subsystem Vendor ID
            if (pKey->SubVendorId != (U16)PCI_FIELD_IGNORE)
            {
                if (pKey->SubVendorId != pDevice->Key.SubVendorId)
                {
                    bMatchId = FALSE;
                }
            }

            // Compare Subsystem Device ID
            if (pKey->SubDeviceId != (U16)PCI_FIELD_IGNORE)
            {
                if (pKey->SubDeviceId != pDevice->Key.SubDeviceId)
                {
                    bMatchId = FALSE;
                }
            }
        }

        // Compare Revision
        if (pKey->Revision != (U8)PCI_FIELD_IGNORE)
        {
            if (pKey->Revision != pDevice->Key.Revision)
            {
                bMatchId = FALSE;
            }
        }

        // Check if match on location and ID
        if (bMatchLoc && bMatchId)
        {
            // Match found, check if it is the desired device
            if (devCount == DeviceNumber)
            {
                DebugPrintf((
                    "Criteria matched device %04X %04X [%02X:%02X.%d]\n",
                    pDevice->Key.DeviceId, pDevice->Key.VendorId,
                    pDevice->Key.bus, pDevice->Key.slot, pDevice->Key.function
                    ));

                // Copy the device information
                *pKey = pDevice->Key;
                return PLX_STATUS_OK;
            }

            // Increment device count
            devCount++;
        }

        // Go to next port
        portNum++;
    }

    // Return number of matched devices
    *pNumMatched = devCount;

    DebugPrintf(("Criteria did not match any devices\n"));
    return PLX_STATUS_INVALID_OBJECT;
}




/***********************************************************
 *
 *          PRIVATE REGISTER DISPATCH FUNCTIONS
 *
 **********************************************************/

/******************************************************************************
 *
 * Function   : PlxDir_PlxRegRead
 *
 * Description:
 *
 ******************************************************************************/
U32
PlxDir_PlxRegRead(
    PLX_DEVICE_OBJECT *pDevice,
    U16                offset,
    PLX_STATUS        *pStatus
    )
{
    if (pDevice->Key.ApiMode == PLX_API_MODE_PCI)
    {
        return PlxPci_PlxRegisterRead(
            pDevice,
            offset,
            pStatus
            );
    }
    else if (pDevice->Key.ApiMode == PLX_API_MODE_I2C_AARDVARK)
    {
        return PlxI2c_PlxRegisterRead(
            pDevice,
            offset,
            pStatus,
            TRUE,       // Adjust for port?
            TRUE        // Retry on error?
            );
    }
    else if (pDevice->Key.ApiMode == PLX_API_MODE_MDIO_SPLICE)
    {
        return MdioSplice_PlxRegisterRead(
            pDevice,
            offset,
            pStatus,
            TRUE,       // Adjust for port?
            TRUE        // Retry on error?
            );
    }
    else if (pDevice->Key.ApiMode == PLX_API_MODE_SDB)
    {
        return Sdb_PlxRegisterRead(
            pDevice,
            offset,
            pStatus,
            TRUE,       // Adjust for port?
            TRUE        // Retry on error?
            );
    }

    return PCI_CFG_RD_ERR_VAL;
}




/******************************************************************************
 *
 * Function   : PlxDir_PlxRegWrite
 *
 * Description:
 *
 ******************************************************************************/
PLX_STATUS
PlxDir_PlxRegWrite(
    PLX_DEVICE_OBJECT *pDevice,
    U16                offset,
    U32                value
    )
{
    if (pDevice->Key.ApiMode == PLX_API_MODE_PCI)
    {
        return PlxPci_PlxRegisterWrite(
            pDevice,
            offset,
            value
            );
    }
    else if (pDevice->Key.ApiMode == PLX_API_MODE_I2C_AARDVARK)
    {
        return PlxI2c_PlxRegisterWrite(
            pDevice,
            offset,
            value,
            TRUE        // Adjust for port?
            );
    }
    else if (pDevice->Key.ApiMode == PLX_API_MODE_MDIO_SPLICE)
    {
        return MdioSplice_PlxRegisterWrite(
            pDevice,
            offset,
            value,
            TRUE        // Adjust for port?
            );
    }
    else if (pDevice->Key.ApiMode == PLX_API_MODE_SDB)
    {
        return Sdb_PlxRegisterWrite(
            pDevice,
            offset,
            value,
            TRUE        // Adjust for port?
            );
    }

    return PLX_STATUS_UNSUPPORTED;
}




/******************************************************************************
 *
 * Function   : PlxDir_PlxMappedRegRead
 *
 * Description:
 *
 ******************************************************************************/
U32
PlxDir_PlxMappedRegRead(
    PLX_DEVICE_OBJECT *pDevice,
    U32                offset,
    PLX_STATUS        *pStatus
    )
{
    if (pDevice->Key.ApiMode == PLX_API_MODE_PCI)
    {
        return PlxPci_PlxMappedRegisterRead(
            pDevice,
            offset,
            pStatus
            );
    }
    else if (pDevice->Key.ApiMode == PLX_API_MODE_I2C_AARDVARK)
    {
        return PlxI2c_PlxRegisterRead(
            pDevice,
            offset,
            pStatus,
            FALSE,      // Adjust for port?
            TRUE        // Retry on error?
            );
    }
    else if (pDevice->Key.ApiMode == PLX_API_MODE_MDIO_SPLICE)
    {
        return MdioSplice_PlxRegisterRead(
            pDevice,
            offset,
            pStatus,
            FALSE,      // Adjust for port?
            TRUE        // Retry on error?
            );
    }
    else if (pDevice->Key.ApiMode == PLX_API_MODE_SDB)
    {
        return Sdb_PlxRegisterRead(
            pDevice,
            offset,
            pStatus,
            FALSE,      // Adjust for port?
            TRUE        // Retry on error?
            );
    }

    return 0;
}




/******************************************************************************
 *
 * Function   : PlxDir_PlxMappedRegWrite
 *
 * Description:
 *
 ******************************************************************************/
PLX_STATUS
PlxDir_PlxMappedRegWrite(
    PLX_DEVICE_OBJECT *pDevice,
    U32                offset,
    U32                value
    )
{
    if (pDevice->Key.ApiMode == PLX_API_MODE_PCI)
    {
        return PlxPci_PlxMappedRegisterWrite(
            pDevice,
            offset,
            value
            );
    }
    else if (pDevice->Key.ApiMode == PLX_API_MODE_I2C_AARDVARK)
    {
        return PlxI2c_PlxRegisterWrite(
            pDevice,
            offset,
            value,
            FALSE       // Adjust for port?
            );
    }
    else if (pDevice->Key.ApiMode == PLX_API_MODE_MDIO_SPLICE)
    {
        return MdioSplice_PlxRegisterWrite(
            pDevice,
            offset,
            value,
            FALSE       // Adjust for port?
            );
    }
    else if (pDevice->Key.ApiMode == PLX_API_MODE_SDB)
    {
        return Sdb_PlxRegisterWrite(
            pDevice,
            offset,
            value,
            FALSE       // Adjust for port?
            );
    }

    return PLX_STATUS_UNSUPPORTED;
}
