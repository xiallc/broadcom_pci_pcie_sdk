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
 *      PciFunc.c
 *
 * Description:
 *
 *      This file contains the PCI support functions
 *
 * Revision History:
 *
 *      05-01-13 : PLX SDK v7.10
 *
 ******************************************************************************/


#include "PciFunc.h"




/***********************************************
 *               Globals
 **********************************************/
// Global ECAM 64-bit address from ACPI table (0=Not Probed, -1=Not Available)
//
// Probing is only enabled on i386 or x64 platforms since it requires
// parsing ACPI tables.  This is not supported on non-x86 platforms.
//
#if defined(__i386__) || defined(__x86_64__)
    static U32 Gbl_Acpi_Addr_ECAM[2] = {ACPI_PCIE_NOT_PROBED, ACPI_PCIE_NOT_PROBED};
#else
    static U32 Gbl_Acpi_Addr_ECAM[2] = {ACPI_PCIE_NOT_AVAILABLE, ACPI_PCIE_NOT_AVAILABLE};
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
    DEVICE_EXTENSION *pdx,
    U16               offset,
    U32              *pValue
    )
{
    int rc;


    // For PCIe extended register, use Enhanced PCIe Mechanism
    if ((offset >= 0x100) && (Gbl_Acpi_Addr_ECAM[0] != ACPI_PCIE_NOT_AVAILABLE))
    {
        return PlxPciExpressRegRead(
                   pdx->Key.bus,
                   pdx->Key.slot,
                   pdx->Key.function,
                   offset,
                   pValue
                   );
    }

    // Default to error
    *pValue = (U32)-1;

    // Offset must be on a 4-byte boundary
    if (offset & 0x3)
        return ApiInvalidOffset;

    // Assume non-VF device
    rc = -1;

    // If the device is a VF, trap some PCI register accesses
    if (Plx_pcie_is_virtfn(pdx->pPciDevice))
    {
        rc = 0;
        switch (offset)
        {
            case 0x00:
                *pValue = ((U32)pdx->pPciDevice->device << 16) | pdx->pPciDevice->vendor;
                break;

            case 0x08:
                *pValue = ((U32)pdx->pPciDevice->class << 8) | pdx->Key.Revision;
                break;

            case 0x10:
                *pValue = pci_resource_start( pdx->pPciDevice, 0 );
                break;

            case 0x14:
                *pValue = pci_resource_start( pdx->pPciDevice, 1 );
                break;

            case 0x18:
                *pValue = pci_resource_start( pdx->pPciDevice, 2 );
                break;

            case 0x1C:
                *pValue = pci_resource_start( pdx->pPciDevice, 3 );
                break;

            case 0x20:
                *pValue = pci_resource_start( pdx->pPciDevice, 4 );
                break;

            case 0x24:
                *pValue = pci_resource_start( pdx->pPciDevice, 5 );
                break;

            case 0x2C:
                *pValue = ((U32)pdx->pPciDevice->subsystem_device << 16) |
                           pdx->pPciDevice->subsystem_vendor;
                break;

            default:
                rc = -1;
                break;
        }
    }

    if (rc != 0)
    {
        rc =
            pci_read_config_dword(
                pdx->pPciDevice,
                offset,
                pValue
                );
    }

    if (rc != 0)
        return ApiConfigAccessFailed;

    return ApiSuccess;
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
    DEVICE_EXTENSION *pdx,
    U16               offset,
    U32               value
    )
{
    int rc;


    // For PCIe extended register, use Enhanced PCIe Mechanism
    if ((offset >= 0x100) && (Gbl_Acpi_Addr_ECAM[0] != ACPI_PCIE_NOT_AVAILABLE))
    {
        return PlxPciExpressRegWrite(
                   pdx->Key.bus,
                   pdx->Key.slot,
                   pdx->Key.function,
                   offset,
                   value
                   );
    }

    // Offset must be on a 4-byte boundary
    if (offset & 0x3)
        return ApiInvalidOffset;

    rc =
        pci_write_config_dword(
            pdx->pPciDevice,
            offset,
            value
            );

    if (rc != 0)
        return ApiConfigAccessFailed;

    return ApiSuccess;
}




/*******************************************************************************
 *
 * Function   :  PlxPciExpressRegRead
 *
 * Description:  Reads a PCI Express register using the enhanced configuration mechanism
 *
 ******************************************************************************/
PLX_STATUS
PlxPciExpressRegRead(
    U8   bus,
    U8   slot,
    U8   function,
    U16  offset,
    U32 *pValue
    )
{
    PLX_UINT_PTR address;


    // Default to error
    *pValue = (U32)-1;

    // Offset must be on a 4-byte boundary
    if (offset & 0x3)
        return ApiInvalidOffset;

    // Check if PCIe ECAM was probed for
    if (Gbl_Acpi_Addr_ECAM[0] == ACPI_PCIE_NOT_PROBED)
        PlxProbeForEcamBase();

    // Check if Enhanced mechanism available through ACPI
    if (Gbl_Acpi_Addr_ECAM[0] == ACPI_PCIE_NOT_AVAILABLE)
        return ApiUnsupportedFunction;

    // Setup base address for register access
    address =
        Gbl_Acpi_Addr_ECAM[0] |
        (bus      << 20)      |
        (slot     << 15)      |
        (function << 12)      |
        (offset   <<  0);

    // Read the register
    *pValue =
        (U32)PlxPhysicalMemRead(
            PLX_INT_TO_PTR(address),
            sizeof(U32)
            );

    return ApiSuccess;
}




/*******************************************************************************
 *
 * Function   :  PlxPciExpressRegWrite
 *
 * Description:  Writes a PCI Express register using the enhanced configuration mechanism
 *
 ******************************************************************************/
PLX_STATUS
PlxPciExpressRegWrite(
    U8  bus,
    U8  slot,
    U8  function,
    U16 offset,
    U32 value
    )
{
    PLX_UINT_PTR address;


    // Offset must be on a 4-byte boundary
    if (offset & 0x3)
        return ApiInvalidOffset;

    // Check if PCIe ECAM was probed for
    if (Gbl_Acpi_Addr_ECAM[0] == ACPI_PCIE_NOT_PROBED)
        PlxProbeForEcamBase();

    // Check if Enhanced mechanism available through ACPI
    if (Gbl_Acpi_Addr_ECAM[0] == ACPI_PCIE_NOT_AVAILABLE)
        return ApiUnsupportedFunction;

    // Setup base address for register access
    address =
        Gbl_Acpi_Addr_ECAM[0] |
        (bus      << 20)      |
        (slot     << 15)      |
        (function << 12)      |
        (offset   <<  0);

    // Write the register
    PlxPhysicalMemWrite(
        PLX_INT_TO_PTR(address),
        value,
        sizeof(U32)
        );

    return ApiSuccess;
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
    U8   bus,
    U8   slot,
    U8   function,
    U16  offset,
    U32 *pValue
    )
{
// Non-X86 architectures may not support CF8/CFC
#if !defined(__i386__) && !defined(__x86_64__)

    PLX_DEVICE_KEY Key;


    // Prepare key with device information
    RtlZeroMemory( &Key, sizeof(PLX_DEVICE_KEY) );
    Key.bus      = bus;
    Key.slot     = slot;
    Key.function = function;

    // Revert to standard OS method
    return PlxPciRegisterRead_UseOS( &Key, offset, pValue );

#else

    U32 RegSave;


    // For PCIe extended register, use Enhanced PCIe Mechanism
    if (offset >= 0x100)
    {
        return PlxPciExpressRegRead(
                   bus,
                   slot,
                   function,
                   offset,
                   pValue
                   );
    }

    // Offset must be on a 4-byte boundary
    if (offset & 0x3)
    {
        *pValue = (U32)-1;
        return ApiInvalidOffset;
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
    RegSave =
        IO_PORT_READ_32(
            0xcf8
            );

    // Configure the command register to access the desired location
    IO_PORT_WRITE_32(
        0xcf8,
        (1 << 31) | (bus << 16) | (slot << 11) | (function << 8) | offset
        );

    // Read the register
    *pValue =
        IO_PORT_READ_32(
            0xcfc
            );

    // Restore the command register
    IO_PORT_WRITE_32(
        0xcf8,
        RegSave
        );

    return ApiSuccess;

#endif
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
    U8  bus,
    U8  slot,
    U8  function,
    U16 offset,
    U32 value
    )
{
// Non-X86 architectures may not support CF8/CFC
#if !defined(__i386__) && !defined(__x86_64__)

    PLX_DEVICE_KEY Key;


    // Prepare key with device information
    RtlZeroMemory( &Key, sizeof(PLX_DEVICE_KEY) );
    Key.bus      = bus;
    Key.slot     = slot;
    Key.function = function;

    // Revert to standard OS method
    return PlxPciRegisterWrite_UseOS( &Key, offset, value );

#else

    U32 RegSave;


    // For PCIe extended register, use Enhanced PCIe Mechanism
    if (offset >= 0x100)
    {
        return PlxPciExpressRegWrite(
                   bus,
                   slot,
                   function,
                   offset,
                   value
                   );
    }

    // Offset must be on a 4-byte boundary
    if (offset & 0x3)
    {
        return ApiInvalidOffset;
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
    RegSave =
        IO_PORT_READ_32(
            0xcf8
            );

    // Configure the command register to access the desired location
    IO_PORT_WRITE_32(
        0xcf8,
        (1 << 31) | (bus << 16) | (slot << 11) | (function << 8) | offset
        );

    // Write the register
    IO_PORT_WRITE_32(
        0xcfc,
        value
        );

    // Restore the command register
    IO_PORT_WRITE_32(
        0xcf8,
        RegSave
        );

    return ApiSuccess;

#endif
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
    if (Gbl_Acpi_Addr_ECAM[0] != ACPI_PCIE_NOT_PROBED)
       return;

    // Default to ACPI and/or ECAM not detected
    Gbl_Acpi_Addr_ECAM[0] = ACPI_PCIE_NOT_AVAILABLE;

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
        goto _Exit_PlxProbeForEcamBase;

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
        DebugPrintf(("ACPI Probe - ACPI not detected\n"));
        goto _Exit_PlxProbeForEcamBase;
    }

    // Reset flag
    bFound = FALSE;

    // Get ACPI revision
    value = PHYS_MEM_READ_8( (U8*)(pAddress + 15) );

    DebugPrintf((
        "ACPI Probe - ACPI is v%s (rev=%d)\n",
        (value == 0) ? "1.0" : "2.0 or higher",
        (int)value
        ));

    DebugPrintf((
        "ACPI Probe - 'RSD PTR ' found at %p\n",
        pAcpi_Addr_RSDP
        ));

    // Store RSDT address
    pAcpi_Addr_RSDT = (U8*)PLX_INT_TO_PTR( PHYS_MEM_READ_32( (U32*)(pAddress + 16) ) );

    // Map RSDT table
    Va_RSDT =
        ioremap_prot(
            PLX_PTR_TO_INT( pAcpi_Addr_RSDT ),
            1024,
            0
            );

    if (Va_RSDT == NULL)
        goto _Exit_PlxProbeForEcamBase;

    // Get RSDT size
    Acpi_Rsdt.Length = PHYS_MEM_READ_32( (U32*)(Va_RSDT + 4) );

    if (Acpi_Rsdt.Length == 0)
    {
        DebugPrintf(("ACPI Probe - Unable to read RSDT table length\n"));
        goto _Exit_PlxProbeForEcamBase;
    }

    // Calculate number of entries
    NumEntries = (U16)((Acpi_Rsdt.Length - sizeof(ACPI_RSDT_v1_0)) / sizeof(U32));

    DebugPrintf((
        "ACPI Probe - RSD table at %p has %d entries\n",
        pAcpi_Addr_RSDT,
        NumEntries
        ));

    if (NumEntries > 100)
    {
        DebugPrintf(("ACPI Probe - Unable to determine number of entries in RSDT table\n"));
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
        Va_Table =
            ioremap_prot(
                PLX_PTR_TO_INT( pAddress ),
                200,
                0
                );

        if (Va_Table == NULL)
            goto _Exit_PlxProbeForEcamBase;

        // Get table signature
        value = PHYS_MEM_READ_32( (U32*)Va_Table );

        DebugPrintf((
            "ACPI Probe - %c%c%c%c table at %p\n",
            (char)(value >>  0),
            (char)(value >>  8),
            (char)(value >> 16),
            (char)(value >> 24),
            pAddress
            ));

        // Check if MCFG table
        if (memcmp(
                "MCFG",
                &value,
                sizeof(U32)
                ) == 0)
        {
            // Get 64-bit base address of Enhanced Config Access Mechanism
            Gbl_Acpi_Addr_ECAM[0] = PHYS_MEM_READ_32( (U32*)(Va_Table + 44) );
            Gbl_Acpi_Addr_ECAM[1] = PHYS_MEM_READ_32( (U32*)(Va_Table + 48) );

            bFound = TRUE;
        }

        // Unmap table
        iounmap(
            Va_Table
            );

        // Get address of next entry
        pEntry += sizeof(U32);

        // Decrement count
        NumEntries--;
    }

_Exit_PlxProbeForEcamBase:

    // Release the BIOS ROM mapping
    if (Va_BiosRom != NULL)
    {
        iounmap(
            Va_BiosRom
            );
    }

    // Release RSDT mapping
    if (Va_RSDT != NULL)
    {
        iounmap(
            Va_RSDT
            );
    }

    if (bFound)
    {
        DebugPrintf((
            "ACPI Probe - PCIe ECAM at %08X_%08X\n",
            (unsigned int)Gbl_Acpi_Addr_ECAM[1], (unsigned int)Gbl_Acpi_Addr_ECAM[0]
            ));
    }
    else
    {
        DebugPrintf(("ACPI Probe - MCFG entry not found (PCIe ECAM not supported)\n"));
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
    VOID *pAddress,
    U8    size
    )
{
    U64   value;
    VOID *KernelVa;


    // Map address into kernel space
    KernelVa =
        ioremap(
            PLX_PTR_TO_INT( pAddress ),
            sizeof(U64)
            );

    if (KernelVa == NULL)
    {
        DebugPrintf(("ERROR - Unable to map %p ==> kernel space\n", pAddress));
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
    iounmap(
        KernelVa
        );

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
    VOID *pAddress,
    U64   value,
    U8    size
    )
{
    VOID *KernelVa;


    // Map address into kernel space
    KernelVa =
        ioremap(
            PLX_PTR_TO_INT( pAddress ),
            sizeof(U64)
            );

    if (KernelVa == NULL)
    {
        DebugPrintf(("ERROR - Unable to map %p ==> kernel space\n", pAddress));
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
    iounmap(
        KernelVa
        );

    return 0;
}
