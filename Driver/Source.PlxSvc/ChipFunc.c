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
 *      ChipFunc.c
 *
 * Description:
 *
 *      This file contains the PLX chip-specific functions
 *
 * Revision History:
 *
 *      09-01-11 : PLX SDK v6.50
 *
 ******************************************************************************/


#include "ChipFunc.h"
#include "DrvDefs.h"
#include "PciFunc.h"
#include "SuppFunc.h"




/******************************************************************************
 *
 * Function   :  PlxRegisterRead_8111
 *
 * Description:  Reads an 8111 PLX-specific register
 *
 *****************************************************************************/
U32
PlxRegisterRead_8111(
    PLX_DEVICE_NODE *pNode,
    U32              offset,
    PLX_STATUS      *pStatus
    )
{
    U32 value;
    U32 RegSave;


    // Verify register offset
    if ((offset < 0x1000) || (offset > 0x1064))
    {
        if (pStatus != NULL)
            *pStatus = ApiInvalidOffset;
        return 0;
    }

    // Adjust offset
    offset -= 0x1000;

    // Save the current index register
    PLX_PCI_REG_READ(
        pNode,
        0x84,
        &RegSave
        );

    // Set the new index
    PLX_PCI_REG_WRITE(
        pNode,
        0x84,
        offset
        );

    // Get the value
    PLX_PCI_REG_READ(
        pNode,
        0x88,
        &value
        );

    // Restore the current index register
    PLX_PCI_REG_WRITE(
        pNode,
        0x84,
        RegSave
        );

    if (pStatus != NULL)
        *pStatus = ApiSuccess;

    return value;
}




/******************************************************************************
 *
 * Function   :  PlxRegisterWrite_8111
 *
 * Description:  Writes to an 8111 PLX-specific control register
 *
 *****************************************************************************/
PLX_STATUS
PlxRegisterWrite_8111(
    PLX_DEVICE_NODE *pNode,
    U32              offset,
    U32              value
    )
{
    U32 RegSave;


    // Verify register offset
    if ((offset < 0x1000) || (offset > 0x1064))
        return ApiInvalidOffset;

    // Adjust offset
    offset -= 0x1000;

    // Save the current index register
    PLX_PCI_REG_READ(
        pNode,
        0x84,
        &RegSave
        );

    // Set the new index
    PLX_PCI_REG_WRITE(
        pNode,
        0x84,
        offset
        );

    // Write the value
    PLX_PCI_REG_WRITE(
        pNode,
        0x88,
        value
        );

    // Restore the current index register
    PLX_PCI_REG_WRITE(
        pNode,
        0x84,
        RegSave
        );

    return ApiSuccess;
}




/******************************************************************************
 *
 * Function   :  PlxRegisterRead_8000
 *
 * Description:  Reads an 8000 PLX-specific register
 *
 *****************************************************************************/
U32
PlxRegisterRead_8000(
    PLX_DEVICE_NODE *pNode,
    U32              offset,
    PLX_STATUS      *pStatus,
    BOOLEAN          bAdjustForPort
    )
{
    int rc;
    U16 Offset_PcieCap;
    U32 value;
    U32 RegValue;
    U32 OffsetAdjustment;


    // Verify that register access is setup
    if (pNode->pRegNode == NULL)
    {
        DebugPrintf(("ERROR: Register access not setup, unable to access PLX registers\n"));

        if (pStatus != NULL)
            *pStatus = ApiInvalidData;
        return 0;
    }

    // Check if BAR 0 has been mapped
    if (pNode->pRegNode->PciBar[0].pVa == NULL)
    {
        DebugPrintf(("Mapping BAR 0 for PLX reg access\n"));

        // Attempt to map BAR 0
        rc =
            PlxPciBarResourceMap(
                pNode->pRegNode,
                0
                );

        if (rc != 0)
        {
            DebugPrintf(("ERROR: Unable to map BAR 0 for PLX registers\n"));

            if (pStatus != NULL)
                *pStatus = ApiInsufficientResources;
            return 0;
        }
    }

    // Adjust offset for port if requested
    if (bAdjustForPort)
    {
        if (pNode->Key.NTPortType == PLX_NT_PORT_NONE)
        {
            // Determine port number if hasn't been done
            if (pNode->PortNumber == (U8)-1)
            {
                // Get the offset of the PCI Express capability 
                Offset_PcieCap =
                    PlxGetExtendedCapabilityOffset(
                        pNode,
                        0x10       // CAP_ID_PCI_EXPRESS
                        );

                if (Offset_PcieCap == 0)
                {
                    // No PCIe capability, default to port 0
                    pNode->PortNumber = 0;
                }
                else
                {
                    // Get PCIe Link Capabilities
                    PLX_PCI_REG_READ(
                        pNode,
                        Offset_PcieCap + 0x0C,
                        &RegValue
                        );

                    // Get port number
                    pNode->PortNumber = (U8)((RegValue >> 24) & 0xFF);
                }
            }

            // Adjust the offset based on port number
            OffsetAdjustment = (pNode->PortNumber * (4 * 1024));
        }
        else
        {
            // Add base for NT port
            OffsetAdjustment = pNode->Offset_NtRegBase;
        }

        // For MIRA enhanced mode, USB EP regs start at 0 instead of port 3
        if ((pNode->Key.PlxFamily == PLX_FAMILY_MIRA) &&
            (pNode->PciHeaderType == 0) &&
            (pNode->PortNumber == 3))
        {
            DebugPrintf(("Override offset adjust for MIRA USB EP (3000 ==> 0)\n"));
            OffsetAdjustment = 0;
        }

        DebugPrintf((
            "Adjusting offset by %02X for port %d\n",
            (int)OffsetAdjustment, pNode->PortNumber
            ));

        offset += OffsetAdjustment;
    }

    // Verify offset
    if (offset >= pNode->pRegNode->PciBar[0].Properties.Size)
    {
        DebugPrintf(("Error - Offset (%02X) exceeds maximum\n", (unsigned)offset));
        if (pStatus != NULL)
            *pStatus = ApiInvalidOffset;
        return 0;
    }

    if (pStatus != NULL)
        *pStatus = ApiSuccess;

    // For Draco 1, some register cause problems if accessed
    if (pNode->Key.PlxFamily == PLX_FAMILY_DRACO_1)
    {
        if ((offset == 0x856C)  || (offset == 0x8570) ||
            (offset == 0x1056C) || (offset == 0x10570))
        {
            return 0;
        }
    }

    value =
        PHYS_MEM_READ_32(
            (U32*)(pNode->pRegNode->PciBar[0].pVa + offset)
            );

    return value;
}




/******************************************************************************
 *
 * Function   :  PlxRegisterWrite_8000
 *
 * Description:  Writes to an 8000 PLX-specific control register
 *
 *****************************************************************************/
PLX_STATUS
PlxRegisterWrite_8000(
    PLX_DEVICE_NODE *pNode,
    U32              offset,
    U32              value,
    BOOLEAN          bAdjustForPort
    )
{
    int rc;
    U16 Offset_PcieCap;
    U32 RegValue;
    U32 OffsetAdjustment;


    // Verify that register access is setup
    if (pNode->pRegNode == NULL)
    {
        DebugPrintf(("ERROR: Register access not setup, unable to access PLX registers\n"));
        return ApiInvalidData;
    }

    // Check if BAR 0 has been mapped
    if (pNode->pRegNode->PciBar[0].pVa == NULL)
    {
        DebugPrintf(("Mapping BAR 0 for PLX reg access\n"));

        // Attempt to map BAR 0
        rc =
            PlxPciBarResourceMap(
                pNode->pRegNode,
                0
                );

        if (rc != 0)
        {
            DebugPrintf(("ERROR: Unable to map BAR 0 for PLX registers\n"));
            return ApiInsufficientResources;
        }
    }

    // Adjust offset for port if requested
    if (bAdjustForPort)
    {
        if (pNode->Key.NTPortType == PLX_NT_PORT_NONE)
        {
            // Determine port number if hasn't been done
            if (pNode->PortNumber == (U8)-1)
            {
                // Get the offset of the PCI Express capability 
                Offset_PcieCap =
                    PlxGetExtendedCapabilityOffset(
                        pNode,
                        0x10       // CAP_ID_PCI_EXPRESS
                        );

                if (Offset_PcieCap == 0)
                {
                    // No PCIe capability, default to port 0
                    pNode->PortNumber = 0;
                }
                else
                {
                    // Get PCIe Link Capabilities
                    PLX_PCI_REG_READ(
                        pNode,
                        Offset_PcieCap + 0x0C,
                        &RegValue
                        );

                    // Get port number
                    pNode->PortNumber = (U8)((RegValue >> 24) & 0xFF);
                }
            }

            // Adjust the offset based on port number
            OffsetAdjustment = (pNode->PortNumber * (4 * 1024));
        }
        else
        {
            // Add base for NT port
            OffsetAdjustment = pNode->Offset_NtRegBase;
        }

        // For MIRA enhanced mode, USB EP regs start at 0 instead of port 3
        if ((pNode->Key.PlxFamily == PLX_FAMILY_MIRA) &&
            (pNode->PciHeaderType == 0) &&
            (pNode->PortNumber == 3))
        {
            DebugPrintf(("Override offset adjust for MIRA USB EP (3000 ==> 0)\n"));
            OffsetAdjustment = 0;
        }

        DebugPrintf((
            "Adjusting offset by %02X for port %d\n",
            (int)OffsetAdjustment, pNode->PortNumber
            ));

        offset += OffsetAdjustment;
    }

    // Verify offset
    if (offset >= pNode->pRegNode->PciBar[0].Properties.Size)
    {
        DebugPrintf(("Error - Offset (%02X) exceeds maximum\n", (unsigned)offset));
        return ApiInvalidOffset;
    }

    // For Draco 1, some register cause problems if accessed
    if (pNode->Key.PlxFamily == PLX_FAMILY_DRACO_1)
    {
        if ((offset == 0x856C)  || (offset == 0x8570) ||
            (offset == 0x1056C) || (offset == 0x10570))
        {
            return ApiSuccess;
        }
    }

    PHYS_MEM_WRITE_32(
        (U32*)(pNode->pRegNode->PciBar[0].pVa + offset),
        value
        );

    return ApiSuccess;
}
