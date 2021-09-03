/*******************************************************************************
 * Copyright 2013-2019 Broadcom Inc
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
 *      PciFunc.c
 *
 * Description:
 *
 *      This file contains the PCI support functions
 *
 * Revision History:
 *
 *      03-01-19 : PCI/PCIe SDK v8.00
 *
 ******************************************************************************/


#include "PciFunc.h"




/***********************************************
 *               Globals
 **********************************************/
// Global ECAM 64-bit address from ACPI table
//
// Probing is only enabled on i386 or x64 platforms since it requires
// parsing ACPI tables.  This is not supported on non-x86 platforms.
//
#if (defined(__i386__) || defined(__x86_64__) || defined(PLX_DOS)) && !defined(PLX_COSIM)
    static U32 Gbl_Acpi_Addr_ECAM[3] = { 0, 0, ACPI_PCIE_NOT_PROBED };
#else
    static U32 Gbl_Acpi_Addr_ECAM[3] = { 0, 0, ACPI_PCIE_ALWAYS_USE_OS };
#endif




/*******************************************************************************
 *
 * Function   :  PlxPciRegisterRead_UseOS
 *
 * Description:  Reads a PCI register using OS services
 *
 ******************************************************************************/
PLX_STATUS
PlxPciRegisterRead_UseOS(
    struct pci_dev *pPciDev,
    U16             offset,
    U32            *pValue
    )
{
    // For PCIe register, use ECAM if bypassing OS
    if ((offset >= 0x100) &&
        (Gbl_Acpi_Addr_ECAM[2] != ACPI_PCIE_DEFAULT_TO_OS) &&
        (Gbl_Acpi_Addr_ECAM[2] != ACPI_PCIE_ALWAYS_USE_OS))
    {
        return PlxPciExpressRegRead(
            pci_domain_nr(pPciDev->bus),
            pPciDev->bus->number,
            PCI_SLOT(pPciDev->devfn),
            PCI_FUNC(pPciDev->devfn),
            offset,
            pValue
            );
    }

    // Use standard function by location
    return PlxPciRegRead_ByLoc(
        pci_domain_nr(pPciDev->bus),
        pPciDev->bus->number,
        PCI_SLOT(pPciDev->devfn),
        PCI_FUNC(pPciDev->devfn),
        offset,
        pValue,
        sizeof(U32)
        );
}




/*******************************************************************************
 *
 * Function   :  PlxPciRegisterWrite_UseOS
 *
 * Description:  Writes a value to a PCI register using OS services
 *
 ******************************************************************************/
PLX_STATUS
PlxPciRegisterWrite_UseOS(
    struct pci_dev *pPciDev,
    U16             offset,
    U32             value
    )
{
    // For PCIe register, use ECAM if bypassing OS
    if ((offset >= 0x100) &&
        (Gbl_Acpi_Addr_ECAM[2] != ACPI_PCIE_DEFAULT_TO_OS) &&
        (Gbl_Acpi_Addr_ECAM[2] != ACPI_PCIE_ALWAYS_USE_OS))
    {
        return PlxPciExpressRegWrite(
            pci_domain_nr(pPciDev->bus),
            pPciDev->bus->number,
            PCI_SLOT(pPciDev->devfn),
            PCI_FUNC(pPciDev->devfn),
            offset,
            value
            );
    }

    // Use standard function by location
    return PlxPciRegWrite_ByLoc(
        pci_domain_nr(pPciDev->bus),
        pPciDev->bus->number,
        PCI_SLOT(pPciDev->devfn),
        PCI_FUNC(pPciDev->devfn),
        offset,
        value,
        sizeof(U32)
        );
}




/*******************************************************************************
 *
 * Function   :  PlxPciRegRead_ByLoc
 *
 * Description:  Reads a PCI register of a device specified by location
 *
 ******************************************************************************/
PLX_STATUS
PlxPciRegRead_ByLoc(
    U8    domain,
    U8    bus,
    U8    slot,
    U8    function,
    U16   offset,
    VOID *pValue,
    U8    AccessSize
    )
{
    int             rc;
    U8              BarNum;
    U32             Flags_OS;
    U32             RegValue;
    struct pci_dev *pPciDev;


    if (pValue == NULL)
    {
        return PLX_STATUS_NULL_PARAM;
    }

    // Verify access type & offset
    switch (AccessSize)
    {
        case sizeof(U8):
            *(U8*)pValue = (U8)-1;
            break;

        case sizeof(U16):
            *(U16*)pValue = (U16)-1;
            if (offset & 0x1)
            {
                return PLX_STATUS_INVALID_OFFSET;
            }
            break;

        case sizeof(U32):
            *(U32*)pValue = (U32)-1;
            if (offset & 0x3)
            {
                return PLX_STATUS_INVALID_OFFSET;
            }
            break;

        default:
            return PLX_STATUS_UNSUPPORTED;
    }

    // Locate PCI device
    pPciDev =
        Plx_pci_get_domain_bus_and_slot(
            domain,
            bus,
            PCI_DEVFN(slot, function)
            );

    if (pPciDev == NULL)
    {
        DebugPrintf((
            "ERROR - Device at [D%d %02X:%02X.%X] does not exist\n",
            domain, bus, slot, function
            ));
        return PLX_STATUS_INVALID_OBJECT;
    }

    // Assume non-VF device
    rc       = -1;
    RegValue = 0;

    // If the device is a VF, trap some PCI register accesses
    if (Plx_pcie_is_virtfn(pPciDev))
    {
        rc = 0;
        switch (offset)
        {
            case 0x00:
                RegValue = ((U32)pPciDev->device << 16) | pPciDev->vendor;
                break;

            case 0x08:
                RegValue = ((U32)pPciDev->class << 8) | pPciDev->revision;
                break;

            case 0x10:
            case 0x14:
            case 0x18:
            case 0x1C:
            case 0x20:
            case 0x24:
                // Build BAR value, accounting for 64-bit
                BarNum   = (offset - 0x10) / sizeof(U32);
                RegValue = PLX_64_LOW_32( pci_resource_start( pPciDev, BarNum ) );
                Flags_OS = pci_resource_flags( pPciDev, BarNum );

                if (Flags_OS & IORESOURCE_IO)
                {
                    RegValue |= (1 << 0);
                }
                if (Flags_OS & IORESOURCE_PREFETCH)
                {
                    RegValue |= (1 << 3);
                }
                if (Flags_OS & IORESOURCE_MEM_64)
                {
                    RegValue |= (2 << 1);
                }

                if ((BarNum > 0) &&
                    (pci_resource_flags( pPciDev, BarNum-1 ) & IORESOURCE_MEM_64))
                {
                    RegValue = PLX_64_HIGH_32( pci_resource_start( pPciDev, BarNum-1 ) );
                }
                break;

            case 0x2C:
                RegValue = ((U32)pPciDev->subsystem_device << 16) |
                           pPciDev->subsystem_vendor;
                break;

            default:
                rc = -1;
                break;
        }
    }

    // Shift desired bytes into position
    RegValue = RegValue >> (offset & 0x3);

    // Get read value
    switch (AccessSize)
    {
        case sizeof(U8):
            if (rc == 0)
            {
                *(U8*)pValue = (U8)RegValue;
            }
            else
            {
                rc = pci_read_config_byte( pPciDev, offset, (U8*)pValue );
            }
            break;

        case sizeof(U16):
            if (rc == 0)
            {
                *(U16*)pValue = (U16)RegValue;
            }
            else
            {
                rc = pci_read_config_word( pPciDev, offset, (U16*)pValue );
            }
            break;

        case sizeof(U32):
            if (rc == 0)
            {
                *(U32*)pValue = RegValue;
            }
            else
            {
                rc = pci_read_config_dword( pPciDev, offset, (U32*)pValue );
            }
            break;

        default:
            rc = -1;
            break;
    }

    // Decrement reference count to device
    pci_dev_put( pPciDev );

    if (rc != 0)
    {
        return PLX_STATUS_FAILED;
    }

    return PLX_STATUS_OK;
}




/*******************************************************************************
 *
 * Function   :  PlxPciRegWrite_ByLoc
 *
 * Description:  Writes to a PCI register of a device specified by location
 *
 ******************************************************************************/
PLX_STATUS
PlxPciRegWrite_ByLoc(
    U8  domain,
    U8  bus,
    U8  slot,
    U8  function,
    U16 offset,
    U32 value,
    U8  AccessSize
    )
{
    int             rc;
    struct pci_dev *pPciDev;


    // Verify access type & offset
    switch (AccessSize)
    {
        case sizeof(U8):
            break;

        case sizeof(U16):
            if (offset & 0x1)
            {
                return PLX_STATUS_INVALID_OFFSET;
            }
            break;

        case sizeof(U32):
            if (offset & 0x3)
            {
                return PLX_STATUS_INVALID_OFFSET;
            }
            break;

        default:
            return PLX_STATUS_UNSUPPORTED;
    }

    // Locate PCI device
    pPciDev =
        Plx_pci_get_domain_bus_and_slot(
            domain,
            bus,
            PCI_DEVFN(slot, function)
            );

    if (pPciDev == NULL)
    {
        DebugPrintf((
            "ERROR - Device at [D%d %02X:%02X.%X] does not exist\n",
            domain, bus, slot, function
            ));
        return PLX_STATUS_INVALID_OBJECT;
    }

    switch (AccessSize)
    {
        case sizeof(U8):
            rc = pci_write_config_byte( pPciDev, offset, (U8)value );
            break;

        case sizeof(U16):
            rc = pci_write_config_word( pPciDev, offset, (U16)value );
            break;

        case sizeof(U32):
            rc = pci_write_config_dword( pPciDev, offset, (U32)value );
            break;

        default:
            rc = -1;
            break;
    }

    // Decrement reference count to device
    pci_dev_put( pPciDev );

    if (rc != 0)
    {
        return PLX_STATUS_FAILED;
    }

    return PLX_STATUS_OK;
}




/*******************************************************************************
 *
 * Function   :  PlxPciExpressRegRead
 *
 * Description:  Reads a PCI Express register using ECAM
 *
 ******************************************************************************/
PLX_STATUS
PlxPciExpressRegRead(
    U8   domain,
    U8   bus,
    U8   slot,
    U8   function,
    U16  offset,
    U32 *pValue
    )
{
    U64 address;


    // Default to error
    *pValue = (U32)-1;

    // Only PCI domain 0 currently supported
    if (domain != 0)
    {
        return PLX_STATUS_UNSUPPORTED;
    }

    // Offset must be on a 4-byte boundary
    if (offset & 0x3)
    {
        return PLX_STATUS_INVALID_OFFSET;
    }

    // Check if PCIe ECAM was probed for
    if (Gbl_Acpi_Addr_ECAM[2] == ACPI_PCIE_NOT_PROBED)
    {
        PlxProbeForEcamBase();
    }

    // Check if ECAM available through ACPI
    if (Gbl_Acpi_Addr_ECAM[2] == ACPI_PCIE_ALWAYS_USE_OS)
    {
        return PLX_STATUS_UNSUPPORTED;
    }

    // Setup base address for register access
    address =
        ((U64)Gbl_Acpi_Addr_ECAM[1] << 32) |
        (Gbl_Acpi_Addr_ECAM[0]      <<  0) |
        (bus                        << 20) |
        (slot                       << 15) |
        (function                   << 12) |
        (offset                     <<  0);

    // Read the register
    *pValue =
        (U32)PlxPhysicalMemRead(
            address,
            sizeof(U32)
            );

    return PLX_STATUS_OK;
}




/*******************************************************************************
 *
 * Function   :  PlxPciExpressRegWrite
 *
 * Description:  Writes a PCI Express register using ECAM
 *
 ******************************************************************************/
PLX_STATUS
PlxPciExpressRegWrite(
    U8  domain,
    U8  bus,
    U8  slot,
    U8  function,
    U16 offset,
    U32 value
    )
{
    U64 address;


    // Only PCI domain 0 currently supported
    if (domain != 0)
    {
        return PLX_STATUS_UNSUPPORTED;
    }

    // Offset must be on a 4-byte boundary
    if (offset & 0x3)
    {
        return PLX_STATUS_INVALID_OFFSET;
    }

    // Check if PCIe ECAM was probed for
    if (Gbl_Acpi_Addr_ECAM[2] == ACPI_PCIE_NOT_PROBED)
    {
        PlxProbeForEcamBase();
    }

    // Check if ECAM available through ACPI
    if (Gbl_Acpi_Addr_ECAM[2] == ACPI_PCIE_ALWAYS_USE_OS)
    {
        return PLX_STATUS_UNSUPPORTED;
    }

    // Setup base address for register access
    address =
        ((U64)Gbl_Acpi_Addr_ECAM[1] << 32) |
        (Gbl_Acpi_Addr_ECAM[0]      <<  0) |
        (bus                        << 20) |
        (slot                       << 15) |
        (function                   << 12) |
        (offset                     <<  0);

    // Write the register
    PlxPhysicalMemWrite(
        address,
        value,
        sizeof(U32)
        );

    return PLX_STATUS_OK;
}




/*******************************************************************************
 *
 * Function   :  PlxPciRegisterRead_BypassOS
 *
 * Description:  Reads a PCI register by bypassing the OS services
 *
 ******************************************************************************/
PLX_STATUS
PlxPciRegisterRead_BypassOS(
    U8   domain,
    U8   bus,
    U8   slot,
    U8   function,
    U16  offset,
    U32 *pValue
    )
{
    U32 RegSave;


    // If bypass disabled, revert to OS call
    if (Gbl_Acpi_Addr_ECAM[2] == ACPI_PCIE_ALWAYS_USE_OS)
    {
        return PlxPciRegRead_ByLoc(
            domain,
            bus,
            slot,
            function,
            offset,
            pValue,
            sizeof(U32)
            );
    }

    // For PCIe extended register, use Enhanced PCIe Mechanism
    if (offset >= 0x100)
    {
        return PlxPciExpressRegRead(
            domain,
            bus,
            slot,
            function,
            offset,
            pValue
            );
    }

    // Default to error
    *pValue = (U32)-1;

    // Only PCI domain 0 currently supported
    if (domain != 0)
    {
        return PLX_STATUS_UNSUPPORTED;
    }

    // Offset must be on a 4-byte boundary
    if (offset & 0x3)
    {
        return PLX_STATUS_INVALID_OFFSET;
    }

    /***************************************************************
     * Access of a PCI register involves using I/O addresses 0xcf8
     * and 0xcfc. These addresses must be used together and no other
     * process must interrupt.
     **************************************************************/

    // Make sure slot is only 5-bits
    slot &= 0x1F;

    // Make sure function is only 3-bits
    function &= 0x7;

    // Save the content of the command register
    RegSave = IO_PORT_READ_32( 0xcf8 );

    // Configure the command register to access the desired location
    IO_PORT_WRITE_32(
        0xcf8,
        (1 << 31) | (bus << 16) | (slot << 11) | (function << 8) | offset
        );

    // Read the register
    *pValue = IO_PORT_READ_32( 0xcfc );

    // Restore the command register
    IO_PORT_WRITE_32( 0xcf8, RegSave );

    return PLX_STATUS_OK;
}




/*******************************************************************************
 *
 * Function   :  PlxPciRegisterWrite_BypassOS
 *
 * Description:  Writes to a PCI register by bypassing the OS services
 *
 ******************************************************************************/
PLX_STATUS
PlxPciRegisterWrite_BypassOS(
    U8  domain,
    U8  bus,
    U8  slot,
    U8  function,
    U16 offset,
    U32 value
    )
{
    U32 RegSave;


    // If bypass disabled, revert to OS call
    if (Gbl_Acpi_Addr_ECAM[2] == ACPI_PCIE_ALWAYS_USE_OS)
    {
        return PlxPciRegWrite_ByLoc(
            domain,
            bus,
            slot,
            function,
            offset,
            value,
            sizeof(U32)
            );
    }

    // For PCIe extended register, use Enhanced PCIe Mechanism
    if (offset >= 0x100)
    {
        return PlxPciExpressRegWrite(
            domain,
            bus,
            slot,
            function,
            offset,
            value
            );
    }

    // Only PCI domain 0 currently supported
    if (domain != 0)
    {
        return PLX_STATUS_UNSUPPORTED;
    }

    // Offset must be on a 4-byte boundary
    if (offset & 0x3)
    {
        return PLX_STATUS_INVALID_OFFSET;
    }

    /***************************************************************
     * Access of a PCI register involves using I/O addresses 0xcf8
     * and 0xcfc. These addresses must be used together and no other
     * process must interrupt.
     **************************************************************/

    // Make sure slot is only 5-bits
    slot &= 0x1F;

    // Make sure function is only 3-bits
    function &= 0x7;

    // Save the content of the command register
    RegSave = IO_PORT_READ_32( 0xcf8 );

    // Configure the command register to access the desired location
    IO_PORT_WRITE_32(
        0xcf8,
        (1 << 31) | (bus << 16) | (slot << 11) | (function << 8) | offset
        );

    // Write the register
    IO_PORT_WRITE_32( 0xcfc, value );

    // Restore the command register
    IO_PORT_WRITE_32( 0xcf8, RegSave );

    return PLX_STATUS_OK;
}




/*******************************************************************************
 *
 * Function   :  PlxProbeForEcamBase
 *
 * Description:  Probes for Enhanced Configuration Access Mechanism base
 *               address through ACPI
 *
 ******************************************************************************/
VOID
PlxProbeForEcamBase(
    VOID
    )
{
    U8              Str_ID[9];
    U8             *pEntry;
    U8             *pAddress;
    U8             *Va_BiosRom;
    U8             *Va_RSDT;
    U8             *Va_Table;
    U8             *pAcpi_Addr_RSDP;
    U8             *pAcpi_Addr_RSDT;
    U16             NumEntries;
    U32             value;
    BOOLEAN         bFound;
    ACPI_RSDT_v1_0  Acpi_Rsdt;


    // Do not probe again if previously did
    if (Gbl_Acpi_Addr_ECAM[2] != ACPI_PCIE_NOT_PROBED)
    {
       return;
    }

    // Default to ACPI and/or ECAM not detected
    Gbl_Acpi_Addr_ECAM[2] = ACPI_PCIE_ALWAYS_USE_OS;

    // Default to ECAM not found
    bFound = FALSE;

    // Initialize virtual addresses
    Va_BiosRom = NULL;
    Va_RSDT    = NULL;

    // Mark end of string
    Str_ID[8] = '\0';

    // Map BIOS ROM into kernel space
    Va_BiosRom =
        ioremap(
            BIOS_MEM_START,
            (BIOS_MEM_END - BIOS_MEM_START)
            );
    if (Va_BiosRom == NULL)
    {
        goto _Exit_PlxProbeForEcamBase;
    }

    // Set physical and virtual starting addresses
    pAcpi_Addr_RSDP = (U8*)BIOS_MEM_START;
    pAddress        = Va_BiosRom;

    // Scan system ROM for ACPI RSDP pointer
    do
    {
        // Read 8-bytes
        *(U32*)Str_ID       = PHYS_MEM_READ_32( (U32*)pAddress );
        *(U32*)(Str_ID + 4) = PHYS_MEM_READ_32( (U32*)(pAddress + 4) );

        // Check for header signature
        if (memcmp(
                "RSD PTR ",
                Str_ID,
                8       // 8 bytes
                ) == 0)
        {
            bFound = TRUE;
        }
        else
        {
            // Increment to next 16-byte boundary
            pAddress        += 16;
            pAcpi_Addr_RSDP += 16;
        }
    }
    while (!bFound && (pAcpi_Addr_RSDP < (U8*)BIOS_MEM_END));

    if (!bFound)
    {
        DebugPrintf(("ACPI Probe: ACPI not detected\n"));
        goto _Exit_PlxProbeForEcamBase;
    }

    // Reset flag
    bFound = FALSE;

    // Get ACPI revision
    value = PHYS_MEM_READ_8( (U8*)(pAddress + 15) );

    DebugPrintf((
        "ACPI Probe: ACPI is v%s (rev=%d)\n",
        (value == 0) ? "1.0" : "2.0 or higher",
        (int)value
        ));

    DebugPrintf((
        "ACPI Probe: 'RSD PTR ' found at %08lX\n",
        PLX_PTR_TO_INT( pAcpi_Addr_RSDP )
        ));

    // Store RSDT address
    pAcpi_Addr_RSDT = (U8*)PLX_INT_TO_PTR( PHYS_MEM_READ_32( (U32*)(pAddress + 16) ) );

    // Map RSDT table
    Va_RSDT = ioremap_prot( PLX_PTR_TO_INT( pAcpi_Addr_RSDT ), 1024, 0 );
    if (Va_RSDT == NULL)
    {
        goto _Exit_PlxProbeForEcamBase;
    }

    // Get RSDT size
    Acpi_Rsdt.Length = PHYS_MEM_READ_32( (U32*)(Va_RSDT + 4) );
    if (Acpi_Rsdt.Length == 0)
    {
        DebugPrintf(("ACPI Probe: Unable to read RSDT table length\n"));
        goto _Exit_PlxProbeForEcamBase;
    }

    // Calculate number of entries
    NumEntries = (U16)((Acpi_Rsdt.Length - sizeof(ACPI_RSDT_v1_0)) / sizeof(U32));

    DebugPrintf((
        "ACPI Probe: RSD table at %08lX has %d entries\n",
        PLX_PTR_TO_INT( pAcpi_Addr_RSDT ), NumEntries
        ));

    if (NumEntries > 100)
    {
        DebugPrintf(("ACPI Probe: Unable to determine RSDT entry count\n"));
        goto _Exit_PlxProbeForEcamBase;
    }

    // Get address of first entry
    pEntry = Va_RSDT + sizeof(ACPI_RSDT_v1_0);

    // Parse entry pointers for MCFG table
    while (NumEntries != 0)
    {
        // Get address of entry
        pAddress = (U8*)PLX_INT_TO_PTR( PHYS_MEM_READ_32( (U32*)pEntry ) );

        // Map table
        Va_Table = ioremap_prot( PLX_PTR_TO_INT( pAddress ), 200, 0 );
        if (Va_Table == NULL)
        {
            goto _Exit_PlxProbeForEcamBase;
        }

        // Get table signature
        value = PHYS_MEM_READ_32( (U32*)Va_Table );

        DebugPrintf((
            "ACPI Probe: %c%c%c%c table at %08lX\n",
            (char)(value >>  0),
            (char)(value >>  8),
            (char)(value >> 16),
            (char)(value >> 24),
            PLX_PTR_TO_INT( pAddress )
            ));

        // Check if MCFG table
        if (memcmp( "MCFG", &value, sizeof(U32) ) == 0)
        {
            // Get 64-bit base address of Enhanced Config Access Mechanism
            Gbl_Acpi_Addr_ECAM[0] = PHYS_MEM_READ_32( (U32*)(Va_Table + 44) );
            Gbl_Acpi_Addr_ECAM[1] = PHYS_MEM_READ_32( (U32*)(Va_Table + 48) );
            bFound = TRUE;

            // Flag ok to use ECAM
            Gbl_Acpi_Addr_ECAM[2] = ACPI_PCIE_BYPASS_OS_OK;
        }

        // Unmap table
        iounmap( Va_Table );

        // Get address of next entry
        pEntry += sizeof(U32);

        // Decrement count
        NumEntries--;
    }

_Exit_PlxProbeForEcamBase:

    // Release the BIOS ROM mapping
    if (Va_BiosRom != NULL)
    {
        iounmap( Va_BiosRom );
    }

    // Release RSDT mapping
    if (Va_RSDT != NULL)
    {
        iounmap( Va_RSDT );
    }

    if (bFound)
    {
        DebugPrintf((
            "ACPI Probe: PCIe ECAM at %02X_%08X\n",
            (unsigned int)Gbl_Acpi_Addr_ECAM[1], (unsigned int)Gbl_Acpi_Addr_ECAM[0]
            ));

        // Default to use OS calls
        DebugPrintf(("ACPI Probe: Will default to OS for PCIe reg accesses\n"));
        Gbl_Acpi_Addr_ECAM[2] = ACPI_PCIE_DEFAULT_TO_OS;
    }
    else
    {
        DebugPrintf(("ACPI Probe: MCFG entry not found (PCIe ECAM not supported)\n"));
    }
}




/*******************************************************************************
 *
 * Function   :  PlxPhysicalMemRead
 *
 * Description:  Maps a memory location and performs a read from it
 *
 ******************************************************************************/
U64
PlxPhysicalMemRead(
    U64 address,
    U8  size
    )
{
    U64   value;
    VOID *KernelVa;


    // Map address into kernel space
    KernelVa = ioremap( (unsigned long)address, sizeof(U64) );
    if (KernelVa == NULL)
    {
        DebugPrintf(("ERROR - Unable to map %p\n", PLX_CAST_64_TO_8_PTR(address)));
        return -1;
    }

    // Read memory
    switch (size)
    {
        case sizeof(U8):
            value = PHYS_MEM_READ_8( KernelVa );
            break;

        case sizeof(U16):
            value = PHYS_MEM_READ_16( KernelVa );
            break;

        case sizeof(U32):
            value = PHYS_MEM_READ_32( KernelVa );
            break;

        default:
            value = 0;
            break;
    }

    // Release the mapping
    iounmap( KernelVa );

    return value;
}




/*******************************************************************************
 *
 * Function   :  PlxPhysicalMemWrite
 *
 * Description:  Maps a memory location and performs a write to it
 *
 ******************************************************************************/
U32
PlxPhysicalMemWrite(
    U64 address,
    U64 value,
    U8  size
    )
{
    VOID *KernelVa;


    // Map address into kernel space
    KernelVa = ioremap( (unsigned long)address, sizeof(U64) );
    if (KernelVa == NULL)
    {
        DebugPrintf(("ERROR - Unable to map %p\n", PLX_CAST_64_TO_8_PTR(address)));
        return -1;
    }

    // Write memory
    switch (size)
    {
        case sizeof(U8):
            PHYS_MEM_WRITE_8( KernelVa, (U8)value );
            break;

        case sizeof(U16):
            PHYS_MEM_WRITE_16( KernelVa, (U16)value );
            break;

        case sizeof(U32):
            PHYS_MEM_WRITE_32( KernelVa, (U32)value );
            break;
    }

    // Release the mapping
    iounmap( KernelVa );

    return 0;
}
