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

/*********************************************************************
 *
 * File Name:
 *
 *      MonCmds.c
 *
 * Description:
 *
 *      Monitor command functions
 *
 ********************************************************************/


#if defined(_WIN32) || defined(_WIN64)
    #define _CRT_NONSTDC_NO_WARNINGS        // Prevents POSIX function warnings
#endif


#include <ctype.h>
#include <math.h>
#include <string.h>
#include <stdlib.h>
#include "CmdLine.h"
#include "Monitor.h"
#include "MonCmds.h"
#include "PciRegs.h"
#include "PlxApi.h"
#include "RegDefs.h"




/**********************************************************
 *
 * Function   :  Cmd_ConsClear
 *
 * Description:  Clear the console
 *
 *********************************************************/
BOOLEAN Cmd_ConsClear( PLX_DEVICE_OBJECT *pDevice, PLXCM_COMMAND *pCmd )
{
    Cons_clear();
    return TRUE;
}




/**********************************************************
 *
 * Function   :  Cmd_Version
 *
 * Description:  Display monitor version information
 *
 *********************************************************/
BOOLEAN Cmd_Version( PLX_DEVICE_OBJECT *pDevice, PLXCM_COMMAND *pCmd )
{
    U8              VersionMajor;
    U8              VersionMinor;
    U8              VersionRevision;
    PLX_VERSION     PlxVersion;
    PLX_DRIVER_PROP DriverProp;


    Cons_printf(
        "PLX Console Monitor, v%d.%d%d [%s]\n\n",
        MONITOR_VERSION_MAJOR,
        MONITOR_VERSION_MINOR,
        MONITOR_VERSION_REVISION,
        __DATE__
        );

    // Display PLX API version
    PlxPci_ApiVersion(
        &VersionMajor,
        &VersionMinor,
        &VersionRevision
        );

    Cons_printf(
        "PLX API   : v%d.%d%d",
        VersionMajor & ~(1 << 7),
        VersionMinor,
        VersionRevision
        );

    if (VersionMajor & (1 << 7))
    {
        Cons_printf(" (Demo)\n");
    }
    else
    {
        Cons_printf("\n");
    }

    // Display driver version
    if (pDevice == NULL)
    {
        Cons_printf( "PLX Driver: N/A (No device selected)\n");
    }
    else
    {
        PlxPci_DriverProperties( pDevice, &DriverProp );

        Cons_printf(
            "PLX driver: v%d.%02d (%s)",
            DriverProp.Version >> 16,
            (DriverProp.Version >> 8) & 0xFF,
            DriverProp.Name
            );

        if (pDevice->Key.PlxChip != 0)
        {
            if (pDevice->Key.ApiMode == PLX_API_MODE_I2C_AARDVARK)
            {
                Cons_printf(" (connected over I2C)");
            }
            else if (pDevice->Key.ApiMode == PLX_API_MODE_MDIO_SPLICE)
            {
                Cons_printf(" (connected over MDIO)");
            }
            else if (pDevice->Key.ApiMode == PLX_API_MODE_SDB)
            {
                Cons_printf(" (connected over SDB)");
            }
            else
            {
                if (DriverProp.bIsServiceDriver)
                {
                    Cons_printf(" (PLX PCI/PCIe Service driver)" );
                }
                else
                {
                    Cons_printf(" (PLX %04X PnP driver)", pDevice->Key.PlxChip );
                }
            }
        }
        Cons_printf("\n");

        // If connected over I2C, display version
        if (pDevice->Key.ApiMode == PLX_API_MODE_I2C_AARDVARK)
        {
            PlxPci_I2cVersion( pDevice->Key.ApiIndex, &PlxVersion );

            Cons_printf(
                "I2C Info  : API:%d.%02d  Software:%d.%02d  Firmware:%d.%02d  Hardware:%d.%02d\n",
                (PlxVersion.I2c.ApiLibrary >> 8), PlxVersion.I2c.ApiLibrary & 0xFF,
                (PlxVersion.I2c.Software >> 8), PlxVersion.I2c.Software & 0xFF,
                (PlxVersion.I2c.Firmware >> 8), PlxVersion.I2c.Firmware & 0xFF,
                (PlxVersion.I2c.Hardware >> 8), PlxVersion.I2c.Hardware & 0xFF
                );
        }
    }

    Cons_printf("\n");

    return TRUE;
}




/**********************************************************
 *
 * Function   :  Cmd_Help
 *
 * Description:  Implements the monitor help command
 *
 *********************************************************/
BOOLEAN Cmd_Help( PLX_DEVICE_OBJECT *pDevice, PLXCM_COMMAND *pCmd )
{
    // Enable throttle output
    ConsoleIoThrottleSet(TRUE);

    Cons_printf("\n");
    Cons_printf("       -----------  General Commands  -----------\n");
    Cons_printf(" cls       Clear terminal display\n");
    Cons_printf(" screen    Display or change screen height\n");
    Cons_printf(" ver       Display monitor version information\n");
    Cons_printf(" history   Display or select command from history\n");
    Cons_printf(" help      Display help screen\n");
    Cons_printf(" exit/quit Exit the application\n");

    Cons_printf("\n");
    Cons_printf("       ------------  Device Access  -------------\n");
    Cons_printf(" dev       Display device list or select new device\n");
    Cons_printf(" i2c       Probe for devices using I2C\n");
    Cons_printf(" mdio      Probe for devices using MDIO\n");
    Cons_printf(" sdb       Probe for devices using SDB\n");
    Cons_printf(" set_chip  Force current device to specific chip type\n");

    Cons_printf("\n");
    Cons_printf("       ------------- PCI/PCIe Info --------------\n");
    Cons_printf(" pci_cap   Display PCI/PCIe device extended capability list\n");
    Cons_printf(" portinfo  Display PCI Express port properties\n");
    Cons_printf(" scan      Scan for all possible PCI devices\n");

    Cons_printf("\n");
    Cons_printf("       -----------  Register Access  ------------\n");
    Cons_printf(" dp        Dump PCI registers\n");
    Cons_printf(" dr        Dump PLX-specific registers\n");
    Cons_printf(" pcr       Access PCI Configuration registers\n");
    Cons_printf(" reg       Generic register access [within port]\n");
    Cons_printf(" mmr       Generic register access [ignore port #]\n");
    Cons_printf(" lcr       Access Local Configuration Regs\n");
    Cons_printf(" rtr       Access Run-Time registers\n");
    Cons_printf(" dma       Access DMA Registers\n");
    Cons_printf(" mqr       Access Message Queue Registers\n");

    Cons_printf("\n");
    Cons_printf("       ------------  EEPROM Access  -------------\n");
    Cons_printf(" eep       Display or modify EEPROM values\n");
    Cons_printf(" eep_load  Load EEPROM values from a file\n");
    Cons_printf(" eep_save  Save EEPROM values to a file\n");

    Cons_printf("\n");
    Cons_printf("       ------------  Memory Access  -------------\n");
    Cons_printf(" db        Read memory  8-bits at a time\n");
    Cons_printf(" dw        Read memory 16-bits at a time\n");
    Cons_printf(" dl        Read memory 32-bits at a time\n");
    Cons_printf(" dq        Read memory 64-bits at a time (if supported)\n");
    Cons_printf(" eb        Write  8-bit data to memory\n");
    Cons_printf(" ew        Write 16-bit data to memory\n");
    Cons_printf(" el        Write 32-bit data to memory\n");
    Cons_printf(" eq        Write 64-bit data to memory (if supported)\n");

    Cons_printf("\n");
    Cons_printf("       -----------  I/O Port Access  ------------\n");
    Cons_printf(" ib        Read I/O port  8-bits at a time\n");
    Cons_printf(" iw        Read I/O port 16-bits at a time\n");
    Cons_printf(" il        Read I/O port 32-bits at a time\n");
    Cons_printf(" ob        Write  8-bit data to I/O port\n");
    Cons_printf(" ow        Write 16-bit data to I/O port\n");
    Cons_printf(" ol        Write 32-bit data to I/O port\n");

    Cons_printf("\n");
    Cons_printf("       ------------  Miscellaneous  -------------\n");
    Cons_printf(" vars      Display variable table\n");
    Cons_printf(" set       Add, update, or delete a user variable\n");
    Cons_printf(" buff      Display DMA buffer properties\n");
    Cons_printf(" reset     Reset the selected device (if supported)\n");
    Cons_printf(" sleep     Delay for a specified amount of time (in milliseconds)\n");

    Cons_printf("\n");

    // Disable throttle output
    ConsoleIoThrottleSet(FALSE);

    return TRUE;
}




/**********************************************************
 *
 * Function   :  Cmd_Sleep
 *
 * Description:  Sleeps for specified timeout
 *
 *********************************************************/
BOOLEAN Cmd_Sleep( PLX_DEVICE_OBJECT *pDevice, PLXCM_COMMAND *pCmd )
{
    PLXCM_ARG *pArg;


    // Verify parameter
    if (pCmd->NumArgs == 0)
    {
        Cons_printf("Usage: sleep {timeout_in_ms}\n");
        return TRUE;
    }

    // Get timeout value
    pArg = CmdLine_ArgGet( pCmd, 0 );
    if (pArg->ArgType != PLXCM_ARG_TYPE_INT)
    {
        Cons_printf("Error: Timeout value is invalid\n");
        return FALSE;
    }

    // Sleep desired amount
    Plx_sleep( (U32)pArg->ArgIntDec );

    return TRUE;
}




/**********************************************************
 *
 * Function   :  Cmd_Screen
 *
 * Description:  Display or modify the screen/terminal size
 *
 *********************************************************/
BOOLEAN Cmd_Screen( PLX_DEVICE_OBJECT *pDevice, PLXCM_COMMAND *pCmd )
{
    PLXCM_ARG *pArg;


    // Get new screen size
    pArg = CmdLine_ArgGet( pCmd, 0 );

    if (pArg == NULL)
    {
        // Get current screen height
        Cons_printf(
            "Current screen height is %d lines\n",
            ConsoleScreenHeightGet()
            );
        return TRUE;
    }

    if (pArg->ArgType != PLXCM_ARG_TYPE_INT)
    {
        Cons_printf("Error: Screen height parameter (%s) is invalid\n", pArg->ArgString);
        return FALSE;
    }

    // Set new screen height
    if (ConsoleScreenHeightSet( (U16)pArg->ArgIntDec ) == 0)
    {
        Cons_printf("Set new screen height to %d lines\n", (U32)pArg->ArgIntDec);
    }
    else
    {
        Cons_printf("Error: Unable to set new screen height to %d lines\n", (U32)pArg->ArgIntDec);
    }

    return TRUE;
}




/**********************************************************
 *
 * Function   :  Cmd_Throttle
 *
 * Description:  Display or modify the currnet throttle toggle
 *
 *********************************************************/
BOOLEAN Cmd_Throttle( PLX_DEVICE_OBJECT *pDevice, PLXCM_COMMAND *pCmd )
{
    PLXCM_ARG *pArg;


    // Get new throttle setting
    pArg = CmdLine_ArgGet( pCmd, 0 );

    if (pArg == NULL)
    {
        // Get current setting
        Gbl_LastRetVal = ConsoleIoThrottleGet();
        Cons_printf(
            "Display throttle: %sABLED\n",
            (Gbl_LastRetVal == TRUE) ? "EN" : "DIS"
            );
        return TRUE;
    }

    if ((pArg->ArgType != PLXCM_ARG_TYPE_INT) ||
        ((pArg->ArgIntDec != 0) && (pArg->ArgIntDec != 1)))
    {
        Cons_printf("Error: Valid parameters are 0 (disable) or 1 (enable)\n");
        return FALSE;
    }

    // Unlock throttle setting
    ConsoleIoThrottleLock( FALSE );

    // Set new toggle
    ConsoleIoThrottleSet( (unsigned char)pArg->ArgIntDec );

    // Lock throttle setting to prevent changes
    ConsoleIoThrottleLock( TRUE );

    // Set return value
    Gbl_LastRetVal = ConsoleIoThrottleGet();

    Cons_printf(
        "Display throttle now %sABLED\n",
        (Gbl_LastRetVal == TRUE) ? "EN" : "DIS"
        );

    return TRUE;
}




/**********************************************************
 *
 * Function   :  Cmd_History
 *
 * Description:  Changes the screen size
 *
 *********************************************************/
BOOLEAN Cmd_History( PLX_DEVICE_OBJECT *pDevice, PLXCM_COMMAND *pCmd )
{
    U16             i;
    BOOLEAN         bSelectCmd;
    PLXCM_ARG      *pArg;
    PLXCM_COMMAND  *pCmdCurr;
    PLX_LIST_ENTRY *pEntry;


    if (Plx_IsListEmpty( &Gbl_ListCmds ))
    {
        Cons_printf(" - No commands in history -\n");
        return TRUE;
    }

    bSelectCmd = FALSE;

    // Check for command number
    pArg = CmdLine_ArgGet( pCmd, 0 );

    if (pArg != NULL)
    {
        if (pArg->ArgType != PLXCM_ARG_TYPE_INT)
        {
            Cons_printf("Usage: history [command_index]\n");
            return FALSE;
        }

        bSelectCmd = TRUE;
    }

    i = 0;

    // Traverse list of commands
    pEntry = Gbl_ListCmds.Flink;

    while (pEntry != &Gbl_ListCmds)
    {
        // Get the object
        pCmdCurr =
            PLX_CONTAINING_RECORD(
                pEntry,
                PLXCM_COMMAND,
                ListEntry
                );

        // Skip over current 'history' command
        if (pCmd != pCmdCurr)
        {
            if (bSelectCmd)
            {
                if (i == pArg->ArgIntDec)
                {
                    Mon_PostCommand( pCmdCurr );
                    return TRUE;
                }
            }
            else
            {
                Cons_printf("%d: %s\n", i, pCmdCurr->szCmdLine);
            }
        }

        // Jump to next item
        i++;
        pEntry = pEntry->Flink;
    }

    if (bSelectCmd)
    {
        Cons_printf("Error: Command #%d does not exist\n", pArg->ArgIntDec);
    }

    return TRUE;
}




/**********************************************************
 *
 * Function   :  Cmd_Reset
 *
 * Description:  Resets a PLX device
 *
 *********************************************************/
BOOLEAN Cmd_Reset( PLX_DEVICE_OBJECT *pDevice, PLXCM_COMMAND *pCmd )
{
    PLX_STATUS status;


    if (pDevice == NULL)
    {
        Cons_printf("Error: No device selected\n");
        return FALSE;
    }

    // Check if device is supported
    if (pDevice->Key.PlxChip == 0)
    {
        Cons_printf("Error: Reset is only supported for PLX devices\n");
        return TRUE;
    }

    Cons_printf("Reset device....");

    status = PlxPci_DeviceReset( pDevice );
    if (status == PLX_STATUS_OK)
    {
        Cons_printf("Ok\n");
    }
    else
    {
        Cons_printf("Error: Unable to reset device (status=%02Xh)\n", status);
    }

    return TRUE;
}




/**********************************************************
 *
 * Function   :  Cmd_Scan
 *
 * Description:
 *
 *********************************************************/
BOOLEAN Cmd_Scan( PLX_DEVICE_OBJECT *pDevice, PLXCM_COMMAND *pCmd )
{
    char        ClassString[100];
    U8          slot;
    U8          function;
    U16         bus;
    U16         DevicesFound;
    U32         RegValue;
    BOOLEAN     bMultiFn;
    PLX_STATUS  status;
    DEVICE_NODE TmpNode;


    DevicesFound = 0;

    // Enable throttle output
    ConsoleIoThrottleSet(TRUE);

    Cons_printf(
        "\n"
        " Bus Slot Fn  Dev  Ven   Device Type\n"
        "===============================================\n"
        );

    for (bus = 0; bus < PCI_MAX_BUS; bus++)
    {
        // Check for user abort
        if (Cons_kbhit())
        {
            break;
        }

        for (slot = 0; slot < PCI_MAX_DEV; slot++)
        {
            function = 0;
            bMultiFn = FALSE;

            do
            {
                Cons_printf("  %02x  %02x  %02x", bus, slot, function);

                RegValue =
                    PlxPci_PciRegisterRead(
                        (U8)bus,
                        slot,
                        function,
                        PCI_REG_DEV_VEN_ID,
                        &status
                        );

                if ( (status == PLX_STATUS_OK) &&
                     (RegValue != PCI_CFG_RD_ERR_VAL_32) &&
                     (RegValue != 0) )
                {
                    // Clear device node
                    RtlZeroMemory( &TmpNode, sizeof(DEVICE_NODE) );

                    // Set Dev/VenID
                    TmpNode.Key.VendorId = (U16)RegValue;
                    TmpNode.Key.DeviceId = (U16)(RegValue >> 16);
                    Cons_printf(
                        "  %04x %04x",
                        TmpNode.Key.DeviceId, TmpNode.Key.VendorId
                        );

                    // Get device class
                    RegValue =
                        PlxPci_PciRegisterRead(
                            (U8)bus,
                            slot,
                            function,
                            PCI_REG_CLASS_REV,
                            NULL
                            );
                    TmpNode.PciClass = (RegValue >> 8);

                    // Get Header type
                    RegValue =
                        PlxPci_PciRegisterRead(
                            (U8)bus,
                            slot,
                            function,
                            PCI_REG_HDR_CACHE_LN,
                            NULL
                            );
                    TmpNode.PciHeaderType = (U8)((RegValue >> 16) & 0x7F);

                    // Check for multi-function device
                    if ((U8)(RegValue >> 16) & (1 << 7))
                    {
                        bMultiFn = TRUE;

                        // Clear multi-function flag
                        TmpNode.PciHeaderType &= 0x3F;
                    }

                    Device_GetClassString( &TmpNode, ClassString );
                    Cons_printf("  %s\n", ClassString);

                    DevicesFound++;
                }
                else
                {
                    Cons_printf("\r");
                }

                // Increment function
                function++;
            }
            while (bMultiFn && (function < PCI_MAX_FUNC));
        }
    }

    Cons_printf(
        "               \n"
        "PCI Bus Scan: "
        );

    if (Cons_kbhit())
    {
        // Clear the character
        Cons_getch();
        Cons_printf("-Halted- ");
    }
    Cons_printf("%d devices found\n\n", DevicesFound);

    // Disable throttle output
    ConsoleIoThrottleSet(FALSE);

    return TRUE;
}




/**********************************************************
 *
 * Function   :  Cmd_SetChip
 *
 * Description:  Sets a device to new chip type
 *
 *********************************************************/
BOOLEAN Cmd_SetChip( PLX_DEVICE_OBJECT *pDevice, PLXCM_COMMAND *pCmd )
{
    PLXCM_ARG   *pArg;
    PLX_STATUS   status;
    DEVICE_NODE *pNode;


    if (pDevice == NULL)
    {
        Cons_printf("Error: No device selected\n");
        return FALSE;
    }

    // Get chip type
    pArg = CmdLine_ArgGet( pCmd, 0 );
    if (pArg == NULL)
    {
        Cons_printf(
            "Usage: set_chip <PlxChipType>\n"
            "\n"
            "       PlxChipType:\n"
            "            0 = reset type & autodetect\n"
            "            Valid PLX 9000, 6000, or 8000 series device\n"
            "            e.g. 9050, 9656, 8111, 6254, 8532, etc\n"
            "\n"
            );
        return TRUE;
    }

    if (pArg->ArgIntHex == 0)
    {
        Cons_printf("Reset chip type...\n");
    }
    else
    {
        Cons_printf("Set new chip type to: %04X\n", (U32)pArg->ArgIntHex);
    }

    // Get the corresponding device node
    pNode = DeviceNodeGetByKey( &pDevice->Key );
    if (pNode == NULL)
    {
        return FALSE;
    }

    // Set new chip type
    status =
        PlxPci_ChipTypeSet(
            pDevice,
            (U16)pArg->ArgIntHex,
            -1          // Set revision to autodetect
            );

    if (status != PLX_STATUS_OK)
    {
        Cons_printf(
            "Error: Unable to set new chip type to \"%04X\"\n",
            (U32)pArg->ArgIntHex
            );
        return FALSE;
    }

    Cons_printf("Update device information...");

    // Update device node chip type
    pNode->Key.PlxChip     = pDevice->Key.PlxChip;
    pNode->Key.PlxRevision = pDevice->Key.PlxRevision;
    pNode->Key.PlxFamily   = pDevice->Key.PlxFamily;

    Cons_printf("Ok\n");

    return TRUE;
}




/**********************************************************
 *
 * Function   :  Cmd_Dev
 *
 * Description:  Display or select a device
 *
 *********************************************************/
BOOLEAN Cmd_Dev( PLX_DEVICE_OBJECT *pDevice, PLXCM_COMMAND *pCmd )
{
    char         DeviceText[100];
    PLXCM_ARG   *pArg;
    DEVICE_NODE *pNode;


    // Get argument
    pArg = CmdLine_ArgGet( pCmd, 0 );

    // If no argument, just display device list
    if (pArg == NULL)
    {
        ConsoleIoThrottleSet(TRUE);
        DeviceListDisplay();
        ConsoleIoThrottleSet(FALSE);
        return TRUE;
    }

    // Verify argument
    if (pArg->ArgType != PLXCM_ARG_TYPE_INT)
    {
        Cons_printf("Error: Invalid device number specified\n");
        return FALSE;
    }

    // Attempt to select desired device
    pNode = DeviceSelectByIndex( (U16)pArg->ArgIntHex );
    if (pNode == NULL)
    {
        Cons_printf("Error: Invalid device selection\n");
        return FALSE;
    }

    Cons_printf(
        "Selected: [D%d %02X:%02X.%X] %04X_%04X",
        pNode->Key.domain,
        pNode->Key.bus,
        pNode->Key.slot,
        pNode->Key.function,
        pNode->Key.DeviceId,
        pNode->Key.VendorId
        );

    if (pNode->Key.PlxChip != 0)
    {
        if ((pNode->Key.PlxChip == 0x9050) &&
            (pNode->Key.PlxRevision == 0x2))
        {
            Cons_printf(" [9052]");
        }
        else
        {
            Cons_printf(" [%04X]", pNode->Key.PlxChip );
        }
    }

    Device_GetClassString( pNode, DeviceText );
    Cons_printf( " - %s\n", DeviceText );

    return TRUE;
}




/**********************************************************
 *
 * Function   :  Cmd_I2cConnect
 *
 * Description:  Attempts to connect to a device over I2C
 *
 *********************************************************/
BOOLEAN Cmd_I2cConnect( PLX_DEVICE_OBJECT *pDevice, PLXCM_COMMAND *pCmd )
{
#if defined(PLX_DOS)

    Cons_printf( "Error: I2C is not supported in DOS\n" );
    return TRUE;

#else

    U16             i;
    BOOLEAN         bError;
    BOOLEAN         bReselect;
    PLXCM_ARG      *pArg;
    PLX_MODE_PROP   ModeProp;
    PLX_DEVICE_KEY  Key;


    bError = FALSE;

    // Verify argument count
    if (pCmd->NumArgs < 3)
    {
        bError = TRUE;
    }

    // Get I2C port
    if (!bError)
    {
        // Clear properties
        memset( &ModeProp, 0, sizeof(PLX_MODE_PROP) );

        pArg = CmdLine_ArgGet( pCmd, 0 );
        if (pArg->ArgType != PLXCM_ARG_TYPE_INT)
        {
            bError = TRUE;
        }
        else
        {
            ModeProp.I2c.I2cPort = (U16)pArg->ArgIntDec;
        }
    }

    // Get I2C address
    if (!bError)
    {
        pArg = CmdLine_ArgGet( pCmd, 1 );
        if (pArg->ArgType != PLXCM_ARG_TYPE_INT)
        {
            bError = TRUE;
        }
        else
        {
            ModeProp.I2c.SlaveAddr = (U16)pArg->ArgIntHex;

            // Check for auto-probe
            if (ModeProp.I2c.SlaveAddr == 0)
            {
                ModeProp.I2c.SlaveAddr = -1;
            }
        }
    }

    // Get I2C clock rate in KHz
    if (!bError)
    {
        pArg = CmdLine_ArgGet( pCmd, 2 );
        if (pArg->ArgType != PLXCM_ARG_TYPE_INT)
        {
            bError = TRUE;
        }
        else
        {
            ModeProp.I2c.ClockRate = (U32)pArg->ArgIntDec;
        }
    }

    if (bError)
    {
        Cons_printf(
            "Usage: i2c <USB_I2C_Port> <I2C_Address> <I2C_Clock>\n"
            "\n"
            "    USB_I2C_Port : USB I2C device number (e.g. 0,1,2..)\n"
            "    I2C_Address  : I2C bus hex address of chip (0=auto-probe)\n"
            "    I2C_Clock    : Decimal I2C bus clock speed in KHz. (0=auto/100KHz)\n"
            "\n"
            "Examples: 'i2c 0 0 40' - Auto-scan for chip & use 40KHz I2C clock\n"
            "          'i2c 0 68 0' - Auto-scan for chip at 68h using default I2C clock\n"
            "\n"
            );

        return TRUE;
    }

    bReselect = FALSE;

    // If already connected, must release current open device
    if ((pDevice != NULL) && (pDevice->Key.ApiMode == PLX_API_MODE_I2C_AARDVARK))
    {
        bReselect = TRUE;
        RtlCopyMemory( &Key, &pDevice->Key, sizeof(PLX_DEVICE_KEY) );
        PlxPci_DeviceClose( pDevice );
    }

    Cons_printf("Scan for I2C devices (ESC to halt)...\n");

    // Build device list
    i = DeviceListCreate( PLX_API_MODE_I2C_AARDVARK, &ModeProp );

    // Check if user canceled
    if (i & (1 << 15))
    {
        Cons_printf(" -- User aborted scan --   ");
        i &= ~(U8)(1 << 15);
    }

    if (i == 0)
    {
        Cons_printf(" - No I2C devices added -\n");
    }
    else
    {
        Cons_printf(
            " - Detected %d I2C device%s -\n",
            i, (i == 1) ? "" : "s"
            );

        // Select first device if none previously selected
        if (pDevice == NULL)
        {
            DeviceSelectByIndex( 0 );
        }
    }

    // Reselect previously selected I2C device
    if (bReselect)
    {
        PlxPci_DeviceOpen( &Key, pDevice );
    }

    Cons_printf("\n");

    return TRUE;
#endif
}




/**********************************************************
 *
 * Function   :  Cmd_MdioConnect
 *
 * Description:  Attempts to connect to a device over I2C
 *
 *********************************************************/
BOOLEAN Cmd_MdioConnect( PLX_DEVICE_OBJECT *pDevice, PLXCM_COMMAND *pCmd )
{
#if defined(PLX_DOS)

    Cons_printf( "Error: MDIO is not supported in DOS\n" );
    return TRUE;

#else

    U16             i;
    BOOLEAN         bError;
    BOOLEAN         bReselect;
    PLXCM_ARG      *pArg;
    PLX_MODE_PROP   ModeProp;
    PLX_DEVICE_KEY  Key;


    bError = FALSE;

    // Verify argument count
    if (pCmd->NumArgs < 2)
    {
        bError = TRUE;
    }

    // Get MDIO port
    if (!bError)
    {
        // Clear properties
        memset( &ModeProp, 0, sizeof(PLX_MODE_PROP) );

        pArg = CmdLine_ArgGet( pCmd, 0 );
        if (pArg->ArgType != PLXCM_ARG_TYPE_INT)
        {
            bError = TRUE;
        }
        else
        {
            ModeProp.Mdio.Port = (U8)pArg->ArgIntDec;
        }
    }

    // Get MDIO clock rate in KHz
    if (!bError)
    {
        pArg = CmdLine_ArgGet( pCmd, 1 );
        if (pArg->ArgType != PLXCM_ARG_TYPE_INT)
        {
            bError = TRUE;
        }
        else
        {
            ModeProp.Mdio.ClockRate = (U32)pArg->ArgIntDec;
        }
    }

    if (bError)
    {
        Cons_printf(
            "Usage: mdio <USB_MDIO_Port> <MDIO_Clock>\n"
            "\n"
            "    USB_MDIO_Port : USB MDIO device number (e.g. 0,1,2..)\n"
            "    MDIO_Clock    : Decimal I2C bus clock speed in KHz. (0=auto/100KHz)\n"
            "\n"
            "Examples: 'mdio 0 40' - Scan for chip & use 40KHz MDIO clock\n"
            "          'mdio 0 0'  - Scan for chip using default MDIO clock\n"
            "\n"
            );
        return TRUE;
    }

    bReselect = FALSE;

    // If already connected, must release current open device
    if ((pDevice != NULL) && (pDevice->Key.ApiMode == PLX_API_MODE_MDIO_SPLICE))
    {
        bReselect = TRUE;
        RtlCopyMemory( &Key, &pDevice->Key, sizeof(PLX_DEVICE_KEY) );
        PlxPci_DeviceClose( pDevice );
    }

    Cons_printf("Scan for MDIO devices (ESC to halt)...\n");

    // Build device list
    i = DeviceListCreate( PLX_API_MODE_MDIO_SPLICE, &ModeProp );

    // Check if user canceled
    if (i & (1 << 15))
    {
        Cons_printf(" -- User aborted scan --   ");
        i &= ~(U8)(1 << 15);
    }

    if (i == 0)
    {
        Cons_printf(" - No MDIO devices added -\n");
    }
    else
    {
        Cons_printf(
            " - Detected %d MDIO device%s -\n",
            i, (i == 1) ? "" : "s"
            );

        // Select first device if none previously selected
        if (pDevice == NULL)
        {
            DeviceSelectByIndex( 0 );
        }
    }

    // Reselect previously selected I2C device
    if (bReselect)
    {
        PlxPci_DeviceOpen( &Key, pDevice );
    }

    Cons_printf("\n");

    return TRUE;
#endif
}




/**********************************************************
 *
 * Function   :  Cmd_SdbConnect
 *
 * Description:  Attempts to connect to a device over SDB
 *
 *********************************************************/
BOOLEAN Cmd_SdbConnect( PLX_DEVICE_OBJECT *pDevice, PLXCM_COMMAND *pCmd )
{
#if defined(PLX_DOS)

    Cons_printf( "Error: SDB is not supported in DOS\n" );
    return TRUE;

#else

    U16             i;
    BOOLEAN         bError;
    BOOLEAN         bReselect;
    PLXCM_ARG      *pArg;
    PLX_MODE_PROP   ModeProp;
    PLX_DEVICE_KEY  Key;


    bError = FALSE;

    // Verify argument count
    if (pCmd->NumArgs < 2)
    {
        bError = TRUE;
    }

    // Get COM/TTY port
    if (!bError)
    {
        // Clear properties
        memset( &ModeProp, 0, sizeof(PLX_MODE_PROP) );

        pArg = CmdLine_ArgGet( pCmd, 0 );
        if (pArg->ArgType != PLXCM_ARG_TYPE_INT)
        {
            bError = TRUE;
        }
        else
        {
            ModeProp.Sdb.Port = (U8)pArg->ArgIntDec;
        }
    }

    // Get baud rate
    if (!bError)
    {
        pArg = CmdLine_ArgGet( pCmd, 1 );
        if (pArg->ArgType != PLXCM_ARG_TYPE_INT)
        {
            bError = TRUE;
        }
        else
        {
            if ( ((U8)pArg->ArgIntDec == 0) || ((U8)pArg->ArgIntDec == 2) )
            {
                ModeProp.Sdb.Baud = SDB_BAUD_RATE_115200;
            }
            else if ((U8)pArg->ArgIntDec == 1)
            {
                ModeProp.Sdb.Baud = SDB_BAUD_RATE_19200;
            }
            else
            {
                bError = TRUE;
            }
        }
    }

    // Default connection to standard UART
    ModeProp.Sdb.Cable = SDB_UART_CABLE_UART;

    // Get USB connection option, if specified
    if (!bError && (pCmd->NumArgs > 2))
    {
        pArg = CmdLine_ArgGet( pCmd, 2 );
        if (pArg->ArgType != PLXCM_ARG_TYPE_STRING)
        {
            bError = TRUE;
        }
        else
        {
            if (Plx_strcasecmp( pArg->ArgString, "u" ) == 0)
            {
                ModeProp.Sdb.Cable = SDB_UART_CABLE_USB;
            }
        }
    }

    if (bError)
    {
        Cons_printf(
            "Usage: sdb <COM Port> <BAUD Rate> [u] \n"
            "\n"
            "    COM Port : COM/TTY port number (1,2..)\n"
            "    BAUD Rate: 0=Default  1=19,200  2=115,2000 (default)\n"
            "    u        : [Linux] Specify USB-to-Serial (ttyUSBx)\n"
            "\n"
            "Examples: 'sdb 1 0'   - COM1 115,200 baud\n"
            "          'sdb 2 1 u' - COM2 19,200 baud via USB cable (/dev/ttyUSB1)\n"
            "\n"
            );
        return TRUE;
    }

    bReselect = FALSE;

    // If already connected, must release current open device
    if ((pDevice != NULL) && (pDevice->Key.ApiMode == PLX_API_MODE_SDB))
    {
        bReselect = TRUE;
        RtlCopyMemory( &Key, &pDevice->Key, sizeof(PLX_DEVICE_KEY) );
        PlxPci_DeviceClose( pDevice );
    }

    Cons_printf("Scan for SDB devices (ESC to halt)...\n");

    // Build device list
    i = DeviceListCreate( PLX_API_MODE_SDB, &ModeProp );

    // Check if user canceled
    if (i & (1 << 15))
    {
        Cons_printf(" -- User aborted scan --   ");
        i &= ~(U8)(1 << 7);
    }

    if (i == 0)
    {
        Cons_printf(" - No SDB devices added -\n");
    }
    else
    {
        Cons_printf(
            " - Detected %d SDB device%s -\n",
            i, (i == 1) ? "" : "s"
            );

        // Select first device if none previously selected
        if (pDevice == NULL)
        {
            DeviceSelectByIndex( 0 );
        }
    }

    // Reselect previously selected I2C device
    if (bReselect)
    {
        PlxPci_DeviceOpen( &Key, pDevice );
    }

    Cons_printf("\n");

    return TRUE;
#endif
}




/**********************************************************
 *
 * Function   :  Cmd_PciCapList
 *
 * Description:  Lists PCI capabilities of selected device
 *
 *********************************************************/
BOOLEAN Cmd_PciCapList( PLX_DEVICE_OBJECT *pDevice, PLXCM_COMMAND *pCmd )
{
    char         szCapability[100];
    U16          Offset_Cap;
    U16          CapID;
    U8           CapVer;
    U32          RegValue;
    U32          RegDevVenId;
    BOOLEAN      bPciExpress;
    DEVICE_NODE *pNode;


    if (pDevice == NULL)
    {
        Cons_printf("Error: No device selected\n");
        return FALSE;
    }

    pNode = DeviceNodeGetByKey( &pDevice->Key );

    if (pNode == NULL)
    {
        return FALSE;
    }

    // Verify PCI header is not Type 2
    if (pNode->PciHeaderType == PCI_HDR_TYPE_2)
    {
        Cons_printf("Device is PCI Type 2 (Cardbus) - no extended capabilities\n");
        return TRUE;
    }

    Cons_printf(
        "\n"
        " PCI & PCIe Capabilities\n"
        " ---------------------------------\n"
        );

    // Get offset of first capability
    RegValue = PlxPci_PciRegisterReadFast( pDevice, PCI_REG_CAP_PTR, NULL );

    // If link is down, PCI reg accesses will fail
    if ((RegValue == PCI_CFG_RD_ERR_VAL_32) || (RegValue == 0))
    {
        Cons_printf("  -- Device does not have PCI extended capabilities --\n");
        return TRUE;
    }

    // Store Device/Vendor ID
    RegDevVenId = PlxPci_PciRegisterReadFast( pDevice, PCI_REG_DEV_VEN_ID, NULL );

    // Start with PCI capabilities
    bPciExpress = FALSE;

    // Set first capability
    Offset_Cap = (U16)RegValue;

    // Initialize version
    CapVer = 0;

    // Traverse capability list and display each one
    while (1)
    {
        // Get next capability
        RegValue = PlxPci_PciRegisterReadFast( pDevice, Offset_Cap, NULL );

        // Verify device decodes offset 100h & above
        if ((Offset_Cap == PCIE_REG_CAP_PTR) && (RegDevVenId == RegValue))
        {
            break;
        }

        if ((RegValue == PCI_CFG_RD_ERR_VAL_32) || (RegValue == 0))
        {
            if ((Offset_Cap != PCIE_REG_CAP_PTR) && (RegValue == PCI_CFG_RD_ERR_VAL_32))
            {
                Cons_printf("  -- Error: Invalid entry at offset %02X --\n", Offset_Cap);
            }
        }
        else
        {
            CapID  = (U16)(RegValue & 0xFF);
            CapVer = (U8)((RegValue >> 16) & 0xF);

            // Version only valid for PCIe & extended capabilities
            if ((bPciExpress == FALSE) && (CapID != PCI_CAP_ID_PCI_EXPRESS))
            {
                CapVer = (U8)-1;
            }

            if (bPciExpress)
            {
                switch (CapID)
                {
                    case PCIE_CAP_ID_ADV_ERROR_REPORTING:
                        strcpy( szCapability, "Advanced Error Reporting (AER)" );
                        break;

                    case PCIE_CAP_ID_VIRTUAL_CHANNEL:
                        strcpy( szCapability, "Virtual Channel" );
                        break;

                    case PCIE_CAP_ID_DEV_SERIAL_NUMBER:
                        strcpy( szCapability, "Device Serial Number" );
                        break;

                    case PCIE_CAP_ID_POWER_BUDGETING:
                        strcpy( szCapability, "Power Budgeting" );
                        break;

                    case PCIE_CAP_ID_RC_LINK_DECLARATION:
                        strcpy( szCapability, "Root Complex Link Declaration" );
                        break;

                    case PCIE_CAP_ID_RC_INT_LINK_CONTROL:
                        strcpy( szCapability, "Root Complex Internal Link Control" );
                        break;

                    case PCIE_CAP_ID_RC_EVENT_COLLECTOR:
                        strcpy( szCapability, "Root Complex Event Collector Endpoint Association" );
                        break;

                    case PCIE_CAP_ID_MF_VIRTUAL_CHANNEL:
                        strcpy( szCapability, "Multi-Function Virtual Channel (MFVC)" );
                        break;

                    case PCIE_CAP_ID_VC_WITH_MULTI_FN:
                        strcpy( szCapability, "Virtual Channel (Device also has MFVC)" );
                        break;

                    case PCIE_CAP_ID_RC_REG_BLOCK:
                        strcpy( szCapability, "Root Complex Register Block (RCRB)" );
                        break;

                    case PCIE_CAP_ID_VENDOR_SPECIFIC:
                        strcpy( szCapability, "Vendor-Specific (VSEC)" );
                        break;

                    case PCIE_CAP_ID_CONFIG_ACCESS_CORR:
                        strcpy( szCapability, "Configuration Access Correlation" );
                        break;

                    case PCIE_CAP_ID_ACCESS_CTRL_SERVICES:
                        strcpy( szCapability, "Access Control Services (ACS)" );
                        break;

                    case PCIE_CAP_ID_ALT_ROUTE_ID_INTERPRET:
                        strcpy( szCapability, "Alternate Routing-ID Interpretation (ARI)" );
                        break;

                    case PCIE_CAP_ID_ADDR_TRANS_SERVICES:
                        strcpy( szCapability, "Address Translation Services (ATS)" );
                        break;

                    case PCIE_CAP_ID_SR_IOV:
                        strcpy( szCapability, "Single-Root I/O Virtualization (SR-IOV)" );
                        break;

                    case PCIE_CAP_ID_MR_IOV:
                        strcpy( szCapability, "Multi-Root I/O Virtualization (MR-IOV)" );
                        break;

                    case PCIE_CAP_ID_MULTICAST:
                        strcpy( szCapability, "Multicast" );
                        break;

                    case PCIE_CAP_ID_PAGE_REQUEST:
                        strcpy( szCapability, "Page request (for ATS)" );
                        break;

                    case PCIE_CAP_ID_AMD_RESERVED:
                        strcpy( szCapability, "Reserved for AMD" );
                        break;

                    case PCIE_CAP_ID_RESIZABLE_BAR:
                        strcpy( szCapability, "Resizable BAR" );
                        break;

                    case PCIE_CAP_ID_DYNAMIC_POWER_ALLOC:
                        strcpy( szCapability, "Dynamic Power Allocation (DPA)" );
                        break;

                    case PCIE_CAP_ID_TLP_PROCESSING_HINT:
                        strcpy( szCapability, "TLP Processing Hints (TPH)" );
                        break;

                    case PCIE_CAP_ID_LATENCY_TOLERANCE_REPORT:
                        strcpy( szCapability, "Latency Tolerance Reporting (LTR)" );
                        break;

                    case PCIE_CAP_ID_SECONDARY_PCI_EXPRESS:
                        strcpy( szCapability, "Secondary PCI Express" );
                        break;

                    case PCIE_CAP_ID_PROTOCOL_MULTIPLEX:
                        strcpy( szCapability, "Protocol Multiplexing (PMUX)" );
                        break;

                    case PCIE_CAP_ID_PROCESS_ADDR_SPACE_ID:
                        strcpy( szCapability, "Process Address Space ID (PASID)" );
                        break;

                    case PCIE_CAP_ID_LTWT_NOTIF_REQUESTER:
                        strcpy( szCapability, "Lightweight Notification Requester (LNR)" );
                        break;

                    case PCIE_CAP_ID_DS_PORT_CONTAINMENT:
                        strcpy( szCapability, "Downstream Port Containment (DPC)" );
                        break;

                    case PCIE_CAP_ID_L1_PM_SUBSTRATES:
                        strcpy( szCapability, "L1 Power Management Substrates (L1PM)" );
                        break;

                    case PCIE_CAP_ID_PRECISION_TIME_MEAS:
                        strcpy( szCapability, "Precision Time Measurement (PTM)" );
                        break;

                    case PCIE_CAP_ID_PCIE_OVER_M_PHY:
                        strcpy( szCapability, "PCIe over M-PHY (M-PCIe)" );
                        break;

                    case PCIE_CAP_ID_FRS_QUEUEING:
                        strcpy( szCapability, "Function Readiness Status (FRS) Queueing" );
                        break;

                    case PCIE_CAP_ID_READINESS_TIME_REPORTING:
                        strcpy( szCapability, "Readiness Time Reporting" );
                        break;

                    case PCIE_CAP_ID_DESIGNATED_VEND_SPECIFIC:
                        strcpy( szCapability, "Designated Vendor-specific (DVSEC)" );
                        break;

                    case PCIE_CAP_ID_DATA_LINK_FEATURE:
                        strcpy( szCapability, "Data Link Feature" );
                        break;

                    case PCIE_CAP_ID_PHYS_LAYER_16GT:
                        strcpy( szCapability, "Physical Layer 16 GT/s" );
                        break;

                    case PCIE_CAP_ID_PHYS_LAYER_16GT_MARGINING:
                        strcpy( szCapability, "Physical Layer 16 GT/s Margining" );
                        break;

                    case PCIE_CAP_ID_HIERARCHY_ID:
                        strcpy( szCapability, "Hierarchy ID" );
                        break;

                    case PCIE_CAP_ID_NATIVE_PCIE_ENCL_MGMT:
                        strcpy( szCapability, "Native PCIe Enclosure Management (NPEM)" );
                        break;

                    default:
                        strcpy( szCapability, "?Unknown?" );
                        break;
                }
            }
            else
            {
                switch (CapID)
                {
                    case PCI_CAP_ID_POWER_MAN:
                        strcpy( szCapability, "Power Management" );
                        break;

                    case PCI_CAP_ID_AGP:
                        strcpy( szCapability, "AGP" );
                        break;

                    case PCI_CAP_ID_VPD:
                        strcpy( szCapability, "Vital Product Data (VPD)" );
                        break;

                    case PCI_CAP_ID_SLOT_ID:
                        strcpy( szCapability, "Slot ID" );
                        break;

                    case PCI_CAP_ID_MSI:
                        strcpy( szCapability, "Message Signaled Interrupt (MSI)" );
                        break;

                    case PCI_CAP_ID_HOT_SWAP:
                        strcpy( szCapability, "Hot Swap" );
                        break;

                    case PCI_CAP_ID_PCIX:
                        strcpy( szCapability, "PCI-X" );
                        break;

                    case PCI_CAP_ID_HYPER_TRANSPORT:
                        strcpy( szCapability, "Hyper Transport" );
                        break;

                    case PCI_CAP_ID_VENDOR_SPECIFIC:
                        strcpy( szCapability, "Vendor-Specific" );
                        break;

                    case PCI_CAP_ID_DEBUG_PORT:
                        strcpy( szCapability, "Debug Port" );
                        break;

                    case PCI_CAP_ID_RESOURCE_CTRL:
                        strcpy( szCapability, "Resource Control" );
                        break;

                    case PCI_CAP_ID_HOT_PLUG:
                        strcpy( szCapability, "Hot Plug" );
                        break;

                    case PCI_CAP_ID_BRIDGE_SUB_ID:
                        strcpy( szCapability, "Bridge Subsystem ID" );
                        break;

                    case PCI_CAP_ID_AGP_8X:
                        strcpy( szCapability, "AGP 8X" );
                        break;

                    case PCI_CAP_ID_SECURE_DEVICE:
                        strcpy( szCapability, "Secure Device" );
                        break;

                    case PCI_CAP_ID_PCI_EXPRESS:
                        strcpy( szCapability, "PCI Express" );
                        break;

                    case PCI_CAP_ID_MSI_X:
                        strcpy( szCapability, "Message Signaled Interrupt Extensions (MSI-X)" );
                        break;

                    case PCI_CAP_ID_SATA:
                        strcpy( szCapability, "SATA" );
                        break;

                    case PCI_CAP_ID_ADV_FEATURES:
                        strcpy( szCapability, "Advanced Features" );
                        break;

                    case PCI_CAP_ID_ENHANCED_ALLOCATION:
                        strcpy( szCapability, "Enhanced Allocation" );
                        break;

                    default:
                        strcpy( szCapability, "?Unknown? capability" );
                        break;
                }
            }

            // Display information
            Cons_printf(
                "  %3X : [%02X] %s",
                Offset_Cap, (U8)RegValue, szCapability
                );

            if (CapVer != (U8)-1)
            {
                Cons_printf(" (v%d)\n", CapVer);
            }
            else
            {
                Cons_printf("\n");
            }
        }

        // Jump to next capability
        if (bPciExpress)
        {
            Offset_Cap = (U16)(RegValue >> 20);
        }
        else
        {
            Offset_Cap = (U16)((RegValue >> 8) & 0xFF);
        }

        // Check if reached end of list
        if ((Offset_Cap == 0) || ((U8)Offset_Cap == 0xFF))
        {
            if (bPciExpress)
            {
                break;
            }

            // Switch to PCI Express capabilities
            bPciExpress = TRUE;

            // PCI Express capabilities start at 100h
            Offset_Cap = PCIE_REG_CAP_PTR;
        }
    }

    Cons_printf("\n");

    return TRUE;
}




/**********************************************************
 *
 * Function   :  Cmd_PortProp
 *
 * Description:  Displays port properties
 *
 *********************************************************/
BOOLEAN Cmd_PortProp( PLX_DEVICE_OBJECT *pDevice, PLXCM_COMMAND *pCmd )
{
    PLX_STATUS    status;
    PLX_PORT_PROP PortProp;


    if (pDevice == NULL)
    {
        Cons_printf("Error: No device selected\n");
        return FALSE;
    }

    Cons_printf("Port Info: ");
    status = PlxPci_GetPortProperties( pDevice, &PortProp );
    if (status != PLX_STATUS_OK)
    {
        Cons_printf("*ERROR* - API failed (status=%02Xh)\n", status);
        return FALSE;
    }
    Cons_printf("\n");

    Cons_printf("    Port Type   : %02d ", PortProp.PortType);

    switch (PortProp.PortType)
    {
        case PLX_PORT_UNKNOWN:
            Cons_printf("(Unknown?)\n");
            break;

        case PLX_PORT_ENDPOINT:  // PLX_PORT_NON_TRANS
            Cons_printf("(Endpoint)\n");
            break;

        case PLX_PORT_UPSTREAM:
            Cons_printf("(Upstream port)\n");
            break;

        case PLX_PORT_DOWNSTREAM:
            Cons_printf("(Downstream port)\n");
            break;

        case PLX_PORT_LEGACY_ENDPOINT:
            Cons_printf("(Legacy Endpoint)\n");
            break;

        case PLX_PORT_ROOT_PORT:
            Cons_printf("(Root Port)\n");
            break;

        case PLX_PORT_PCIE_TO_PCI_BRIDGE:
            Cons_printf("(PCIe-to-PCI Bridge)\n");
            break;

        case PLX_PORT_PCI_TO_PCIE_BRIDGE:
            Cons_printf("(PCI-to-PCIe Bridge)\n");
            break;

        case PLX_PORT_ROOT_ENDPOINT:
            Cons_printf("(Root Complex Endpoint)\n");
            break;

        case PLX_PORT_ROOT_EVENT_COLL:
            Cons_printf("(Root Complex Event Collector)\n");
            break;

        default:
            Cons_printf("(N/A)\n");
            break;
    }

    if (PortProp.bNonPcieDevice)
    {
        Cons_printf("      --- PCIe not supported ---\n");
        return TRUE;
    }

    // Port Number
    Cons_printf("    Port Number : %02d\n", PortProp.PortNumber);

    // Max Payload
    Cons_printf(
        "    Max Payload : %d%sB / %d%sB\n",
        (PortProp.MaxPayloadSize > 1000) ?
          (PortProp.MaxPayloadSize >> 10) : PortProp.MaxPayloadSize,
        (PortProp.MaxPayloadSize > 1000) ? "K" : "",
        (PortProp.MaxPayloadSupported > 1000) ?
          (PortProp.MaxPayloadSupported >> 10) : PortProp.MaxPayloadSupported,
        (PortProp.MaxPayloadSupported > 1000) ? "K" : ""
        );

    // Max Read Req Size
    Cons_printf("    Max Read Req: %02dB\n", PortProp.MaxReadReqSize);

    // PCIe Link
    Cons_printf("    PCIe Link   : ");
    if (PortProp.LinkWidth == 0)
    {
        Cons_printf(" --");
    }
    else
    {
        Cons_printf("G%dx%d", PortProp.LinkSpeed, PortProp.LinkWidth);
    }
    Cons_printf(" / G%dx%d\n", PortProp.MaxLinkSpeed, PortProp.MaxLinkWidth);

    return TRUE;
}




/**********************************************************
 *
 * Function   :  Cmd_MH_Prop
 *
 * Description:  Displays Multi-host properties
 *
 *********************************************************/
BOOLEAN Cmd_MH_Prop( PLX_DEVICE_OBJECT *pDevice, PLXCM_COMMAND *pCmd )
{
    PLX_STATUS          status;
    PLX_MULTI_HOST_PROP MHProp;


    if (pDevice == NULL)
    {
        Cons_printf("Error: No device selected\n");
        return FALSE;
    }

    if (((pDevice->Key.PlxChip & 0xFF00) != 0x8600) &&
        ((pDevice->Key.PlxChip & 0xFF00) != 0x8700) &&
        ((pDevice->Key.PlxChip & 0xFF00) != 0x9700))
    {
        Cons_printf("Command is only supported for PLX 8000 switches\n");
        return TRUE;
    }

    // Get multi-host properties
    status =
        PlxPci_MH_GetProperties(
            pDevice,
            &MHProp
            );

    if (status != PLX_STATUS_OK)
    {
        Cons_printf("Device doesn't support multi-host or unable to get properties\n");
    }
    else
    {
        if (MHProp.SwitchMode == PLX_CHIP_MODE_STANDARD)
        {
            Cons_printf("Switch is in standard mode\n");
        }
        else if (MHProp.bIsMgmtPort == FALSE)
        {
            Cons_printf("Switch is in VS mode, but not management port\n");
        }
        else
        {
            Cons_printf(
                "Mode        : Virtual Switch\n"
                "Enabled VS  : %04X\n"
                "Active Mgmt : %d (%s)\n"
                "Backup Mgmt : %d (%s)\n"
                "VS UP-DS pts: 0:%02d-%08X 1:%02d-%08X 2:%02d-%08X 3:%02d-%08X\n"
                "              4:%02d-%08X 5:%02d-%08X 6:%02d-%08X 7:%02d-%08X\n",
                MHProp.VS_EnabledMask,
                MHProp.MgmtPortNumActive,
                (MHProp.bMgmtPortActiveEn) ? "enabled" : "disabled",
                MHProp.MgmtPortNumRedundant,
                (MHProp.bMgmtPortRedundantEn) ? "enabled" : "disabled",
                MHProp.VS_UpstreamPortNum[0], MHProp.VS_DownstreamPorts[0],
                MHProp.VS_UpstreamPortNum[1], MHProp.VS_DownstreamPorts[1],
                MHProp.VS_UpstreamPortNum[2], MHProp.VS_DownstreamPorts[2],
                MHProp.VS_UpstreamPortNum[3], MHProp.VS_DownstreamPorts[3],
                MHProp.VS_UpstreamPortNum[4], MHProp.VS_DownstreamPorts[4],
                MHProp.VS_UpstreamPortNum[5], MHProp.VS_DownstreamPorts[5],
                MHProp.VS_UpstreamPortNum[6], MHProp.VS_DownstreamPorts[6],
                MHProp.VS_UpstreamPortNum[7], MHProp.VS_DownstreamPorts[7]
                );
        }
    }

    return TRUE;
}




/**********************************************************
 *
 * Function   :  Cmd_NvmeProp
 *
 * Description:  Displays NVMe properties
 *
 *********************************************************/
BOOLEAN Cmd_NvmeProp( PLX_DEVICE_OBJECT *pDevice, PLXCM_COMMAND *pCmd )
{
    U8          *pBarVa;
    U32          regVal;
    U32          pciCmdStat;
    DEVICE_NODE *pNode;


    if (pDevice == NULL)
    {
        Cons_printf("Error: No device selected\n");
        return FALSE;
    }

    // Get device node
    pNode = DeviceNodeGetByKey( &pDevice->Key );
    if (pNode == NULL)
    {
        return FALSE;
    }

    // Verify NVMe device is selected
    if (pNode->PciClass != 0x010802)
    {
        Cons_printf("Error: Command only supported when NVMe endpoint selected\n");
        return FALSE;
    }

    // Ensure BAR 0 is mapped
    if (pNode->Va_PciBar[0] == 0)
    {
        Cons_printf("Error: BAR 0 disabled or not mapped\n");
        return FALSE;
    }

    // Get BAR 0 virtual address
    pBarVa = (U8*)pNode->Va_PciBar[0];

    // Get PCI command/status register (04h)
    pciCmdStat = PlxPci_PciRegisterReadFast( pDevice, PCI_REG_CMD_STAT, NULL );

    // Clear any error status bits (04h[31:27])
    pciCmdStat &= ~((U32)0x1F << 27);

    // Make sure memory BAR access is enabled (04h[1])
    PlxPci_PciRegisterWriteFast( pDevice, PCI_REG_CMD_STAT, pciCmdStat | (1 << 1) );

    // Display (0h[])
    Cons_printf("NVMe Properties:\n");
    Cons_printf("  Vendor      : %04Xh\n", pDevice->Key.VendorId);

    // Version
    regVal = PlxCm_MemRead_32( pBarVa + 0x8 );
    Cons_printf(
        "  Compliance  : v%d.%d%d\n",
        (U8)(regVal >> 16), (U8)(regVal >> 8), (U8)(regVal >> 0)
        );

    // Admin queue (24h Submit=[11:0] Cpl=[27:16]
    //              2Ch/28h=64b Submit addr
    //              34h/30h=64b Cpl addr)
    regVal = PlxCm_MemRead_32( pBarVa + 0x24 );
    Cons_printf(
        "  Admin Queue : Submit: %02X_%08Xh (%d entries)\n"
        "                Compl : %02X_%08Xh (%d entries)\n",
        PlxCm_MemRead_32( pBarVa + 0x2C ), PlxCm_MemRead_32( pBarVa + 0x28 ),
        (regVal >> 0) & 0xFFF,
        PlxCm_MemRead_32( pBarVa + 0x34 ), PlxCm_MemRead_32( pBarVa + 0x30 ),
        (regVal >> 16) & 0xFFF
        );

    // I/O queues (14h Submit=[23:20] Cpl=[19:16])
    regVal = PlxCm_MemRead_32( pBarVa + 0x14 );
    Cons_printf(
        "  I/O Queues  : Submit:%dB  Cpl:%dB (entry sizes)\n",
        (int)pow( 2, ((regVal >> 20) & 0xF) ),
        (int)pow( 2, ((regVal >> 16) & 0xF) )
        );

    // Restore PCI command/status (04h)
    PlxPci_PciRegisterWriteFast( pDevice, PCI_REG_CMD_STAT, pciCmdStat );

    return TRUE;
}




/**********************************************************
 *
 * Function   :  Cmd_VarDisplay
 *
 * Description:
 *
 *********************************************************/
BOOLEAN Cmd_VarDisplay( PLX_DEVICE_OBJECT *pDevice, PLXCM_COMMAND *pCmd )
{
    U8         index;
    size_t     NumSpaces;
    PLXCM_VAR *pVar;


    // Display variables
    Cons_printf(
        "\n"
        "  Variable      Value             Type      Description\n"
        " =================================================================\n"
        );

    index = 0;

    do
    {
        pVar = CmdLine_VarGetByIndex( index );

        if (pVar == NULL)
        {
            if (index == 0)
            {
                Cons_printf("\t\t-- No variables exist --\n");
            }

            return TRUE;
        }

        // Display variable
        Cons_printf( "   %s ", pVar->strName );

        // Go to next item
        NumSpaces = 12 - strlen( pVar->strName );
        while (NumSpaces--)
        {
            Cons_printf(" ");
        }

        // Display value
        Cons_printf( "%s ", pVar->strValue );

        // Go to next item
        NumSpaces = 17 - strlen( pVar->strValue );
        while (NumSpaces--)
        {
            Cons_printf(" ");
        }

        if (pVar->bSystemVar)
        {
            Cons_printf("System    ");
        }
        else
        {
            Cons_printf("User      ");
        }

        // Display description for system variables
        if (pVar->bSystemVar)
        {
            if ((pVar->strName[0] == 'v') || (pVar->strName[0] == 'V'))
            {
                Cons_printf( "BAR %c virtual address", pVar->strName[1] );
            }
            else if (Plx_strcasecmp(pVar->strName, "hbuf") == 0)
            {
                Cons_printf( "PLX DMA buffer virtual address" );
            }
            else if (Plx_strcasecmp(pVar->strName, "RetVal") == 0)
            {
                Cons_printf( "Last command return value" );
            }
        }

        Cons_printf("\n");

        // Go to next variable
        index++;
    }
    while (1);
}




/**********************************************************
 *
 * Function   :  Cmd_VarSet
 *
 * Description:
 *
 *********************************************************/
BOOLEAN Cmd_VarSet( PLX_DEVICE_OBJECT *pDevice, PLXCM_COMMAND *pCmd )
{
    BOOLEAN    bError;
    PLXCM_ARG *pArgVar;
    PLXCM_ARG *pArgValue;
    PLXCM_VAR *pVar;


    bError = FALSE;

    if ((pCmd->NumArgs != 2) && (pCmd->NumArgs != 3))
    {
        bError = TRUE;
        goto _Exit_Cmd_VarSet;
    }

    // Get variable name
    pArgVar = CmdLine_ArgGet( pCmd, 0 );

    // If variable name was converted from existing value, restore it
    if (pArgVar->pVar != NULL)
    {
        // Verify variable is not existing system variable
        if (pArgVar->pVar->bSystemVar)
        {
            Cons_printf(
                "Error: Cannot modify reserved variable '%s'\n",
                pArgVar->pVar->strName
                );
            return FALSE;
        }

        strcpy( pArgVar->ArgString, pArgVar->pVar->strName );
    }

    // Verify variable is not reserved
    if (!Plx_strcasecmp(pArgVar->ArgString, "v0") ||
        !Plx_strcasecmp(pArgVar->ArgString, "v1") ||
        !Plx_strcasecmp(pArgVar->ArgString, "v2") ||
        !Plx_strcasecmp(pArgVar->ArgString, "v3") ||
        !Plx_strcasecmp(pArgVar->ArgString, "v4") ||
        !Plx_strcasecmp(pArgVar->ArgString, "v5") ||
        !Plx_strcasecmp(pArgVar->ArgString, "hbuf") ||
        !Plx_strcasecmp(pArgVar->ArgString, "RetVal"))
    {
        Cons_printf(
            "Error: Variable '%s' is reserved for system use\n",
            pArgVar->ArgString
            );
        return FALSE;
    }

    // Variable name must start with an alpha character
    if (isalpha( pArgVar->ArgString[0] ) == FALSE)
    {
        Cons_printf("Error: Variable name must start with alpha character\n");
        return FALSE;
    }

    // Get 2nd argument
    pArgValue = CmdLine_ArgGet( pCmd, 1 );

    // If '=' used, skip it
    if (pArgValue->ArgString[0] != '=')
    {
        Cons_printf("Error: Missing '=' in parameters\n");
        return FALSE;
    }

    // If no 3rd argument, delete existing variable
    if (pCmd->NumArgs == 2)
    {
        // Delete existing value
        return CmdLine_VarDelete(
            pArgVar->ArgString,
            FALSE           // Non-system variable
            );
    }

    // Get value
    pArgValue = CmdLine_ArgGet( pCmd, 2 );

    // Add or update variable exists already
    pVar =
        CmdLine_VarAdd(
            pArgVar->ArgString,
            pArgValue->ArgString,
            FALSE           // Non-system variable
            );

    if (pVar == NULL)
    {
        Cons_printf("Error: Unable to set or update variable\n");
        return FALSE;
    }

_Exit_Cmd_VarSet:
    if (bError)
    {
        Cons_printf("Usage: set <Variable_Name> = [Value]\n");
        return FALSE;
    }

    return TRUE;
}




/**********************************************************
 *
 * Function   :  Cmd_ShowBuffer
 *
 * Description:
 *
 *********************************************************/
BOOLEAN Cmd_ShowBuffer( PLX_DEVICE_OBJECT *pDevice, PLXCM_COMMAND *pCmd )
{
    PLXCM_VAR        *pVar;
    PLXCM_ARG        *pArg;
    PLX_PHYSICAL_MEM  PciBuffer;


    if (pDevice == NULL)
    {
        Cons_printf("Error: No device selected\n");
        return FALSE;
    }

    // Get PCI buffer properties
    PlxPci_CommonBufferProperties( pDevice, &PciBuffer );

    if (PciBuffer.Size == 0)
    {
        Cons_printf("Error: Host DMA buffer is not enabled or not available\n");
        Gbl_LastRetVal = 0;
    }
    else
    {
        // Display properties if buffer enabled
        Cons_printf(
            "Host buffer\n"
            "  PCI address : %08X\n"
            "  Size        : %08X (%d Kb)\n",
            (PLX_UINT_PTR)PciBuffer.PhysicalAddr,
            PciBuffer.Size,
            (PciBuffer.Size >> 10)
            );

        Cons_printf( "  Virtual addr: " );

        // Get buffer address from variable table
        pVar = CmdLine_VarLookup( "hbuf" );

        Cons_printf(
            "%s\n",
            (pVar == NULL) ? " -- Not mapped --\n" : pVar->strValue
            );

        // Check for specific return value request
        pArg = CmdLine_ArgGet( pCmd, 0 );

        if (pArg == NULL)
        {
            Gbl_LastRetVal = 0;
        }
        else if (Plx_strcasecmp( pArg->ArgString, "-p" ) == 0)
        {
            // Return physical address
            Gbl_LastRetVal = PciBuffer.PhysicalAddr;
        }
        else if (Plx_strcasecmp( pArg->ArgString, "-s" ) == 0)
        {
            // Return
            Gbl_LastRetVal = PciBuffer.Size;
        }
        else
        {
            if (pVar == NULL)
            {
                Gbl_LastRetVal = 0;
            }
            else
            {
                Gbl_LastRetVal = htol( pVar->strValue );
            }
        }
    }

    return TRUE;
}




/**********************************************************
 *
 * Function   :  Cmd_MemRead
 *
 * Description:
 *
 *********************************************************/
BOOLEAN Cmd_MemRead( PLX_DEVICE_OBJECT *pDevice, PLXCM_COMMAND *pCmd )
{
    int         i;
    int         x;
    int         CharsToPrint;
    int         SpacesToInsert;
    U8          buffer[20];
    U8          size;
    U8         *pEndAddr;
    static U8  *pCurrAddr = NULL;
    U32         ByteCount;
    U64         CurrValue;
    BOOLEAN     bDone;
    BOOLEAN     bRepeat;
    BOOLEAN     bEndLine;
    PLXCM_ARG  *pArg;


    // Memory access commands only available in PCI access mode
    if (pDevice->Key.ApiMode != PLX_API_MODE_PCI)
    {
        Cons_printf("Error: '%s' is only available in direct PCI access mode\n", pCmd->szCmd);
        return FALSE;
    }

    // Set access size
    if (pCmd->szCmd[1] == 'b')
    {
        size = sizeof(U8);
    }
    else if (pCmd->szCmd[1] == 'w')
    {
        size = sizeof(U16);
    }
    else if (pCmd->szCmd[1] == 'l')
    {
        size = sizeof(U32);
    }
    else if (pCmd->szCmd[1] == 'q')
    {
        size = sizeof(U64);
    }
    else
    {
        return FALSE;
    }

    // Set current address
    if (pCmd->NumArgs >= 1)
    {
        // Get starting address
        pArg = CmdLine_ArgGet( pCmd, 0 );

        // Verify argument
        if (pArg->ArgType != PLXCM_ARG_TYPE_INT)
        {
            Cons_printf("Error: '%s' is not a valid address\n", pArg->ArgString);
            return FALSE;
        }

        // Set current address
        pCurrAddr = PLX_CAST_64_TO_8_PTR( (PLX_UINT_PTR)pArg->ArgIntHex );
    }

    // Check for count
    if (pCmd->NumArgs <= 1)
    {
        // Set default count to 80h
        pEndAddr = (U8*)0x80;
    }
    else
    {
        pArg = CmdLine_ArgGet( pCmd, 1 );

        // Verify argument
        if (pArg->ArgType != PLXCM_ARG_TYPE_INT)
        {
            Cons_printf("Error: '%s' is not a valid byte count\n", pArg->ArgString);
            return FALSE;
        }

        // Set byte count
        pEndAddr = PLX_CAST_64_TO_8_PTR( (PLX_UINT_PTR)pArg->ArgIntHex );
    }

    // Set final end address
    pEndAddr  = pCurrAddr + (PLX_UINT_PTR)pEndAddr;
    ByteCount = (U32)(pEndAddr - pCurrAddr);

    bRepeat = FALSE;

    // Initialize to avoid compiler warning
    CharsToPrint   = 0;
    SpacesToInsert = 0;

    // Enable throttle output
    ConsoleIoThrottleSet(TRUE);

    do
    {
        i        = 0;
        bDone    = FALSE;
        bEndLine = FALSE;

        while (!bDone)
        {
            if (i == 0)
            {
                // Set initial number of characters to display
                CharsToPrint = 0;

                // Set initial spaces to insert
                SpacesToInsert = (U8)(((size * 2) + 1) * (16 / size));

                Cons_printf(
                    "%s%08x: ",
                    (sizeof(PLX_UINT_PTR) == sizeof(U64)) ? ".." : "",
                    (PLX_UINT_PTR)pCurrAddr
                    );
            }

            switch (size)
            {
                case sizeof(U8):
                    CurrValue = PlxCm_MemRead_8(pCurrAddr);
                    Cons_printf("%02x ", (U8)CurrValue);
                    break;

                case sizeof(U16):
                    CurrValue = PlxCm_MemRead_16(pCurrAddr);
                    Cons_printf("%04x ", (U16)CurrValue);
                    break;

                case sizeof(U32):
                    CurrValue = PlxCm_MemRead_32(pCurrAddr);
                    Cons_printf("%08x ", CurrValue);
                    break;
            }

            // Store value for printing
            for (x = 0; x < size; x++)
            {
                buffer[i+x] = ((U8*)(&CurrValue))[x];

                // Verify printable character
                if ((buffer[i+x] != ' ') &&
                    (!isgraph(buffer[i+x])))
                {
                    buffer[i+x] = '.';
                }
            }

            i         += size;
            pCurrAddr += size;

            // Adjust characters to print
            CharsToPrint += size;

            // Adjust spaces
            SpacesToInsert -= (size * 2) + 1;

            if (pCurrAddr >= pEndAddr)
            {
                bDone    = TRUE;
                bEndLine = TRUE;
            }

            if (!bEndLine)
            {
                if (i == 16)
                {
                    i        = 0;
                    bEndLine = TRUE;
                }
            }

            if (bEndLine)
            {
                bEndLine = FALSE;

                // Insert necessary spaces
                while (SpacesToInsert--)
                {
                    Cons_printf(" ");
                }

                // Display characters at end of line
                for (x = 0; x < CharsToPrint; x++)
                {
                    Cons_printf("%c", buffer[x]);
                }

                // Next line but check if user (Q)uit via throttle prompt
                if (Cons_printf("\n") == 0)
                {
                    break;
                }

                // Check for user cancel for larger reads
                if ((ByteCount > MIN_BYTE_CHECK_CANCEL) && Cons_kbhit())
                {
                    if (Cons_getch() == CONS_KEY_ESCAPE)
                    {
                        Cons_printf(" - user abort\n");
                        break;
                    }
                }
            }
        }
    }
    while (bRepeat);

    // Disable throttle output
    ConsoleIoThrottleSet(FALSE);

    return TRUE;
}




/**********************************************************
 *
 * Function   :  Cmd_MemWrite
 *
 * Description:
 *
 *********************************************************/
BOOLEAN Cmd_MemWrite( PLX_DEVICE_OBJECT *pDevice, PLXCM_COMMAND *pCmd )
{
    U8         i;
    U8         size;
    U8        *pAddr;
    BOOLEAN    bVerifyArgs;
    PLXCM_ARG *pArg;


    // Memory access commands only available in PCI access mode
    if (pDevice->Key.ApiMode != PLX_API_MODE_PCI)
    {
        Cons_printf("Error: '%s' is only available in direct PCI access mode\n", pCmd->szCmd);
        return FALSE;
    }

    // Set access size
    if (pCmd->szCmd[1] == 'b')
    {
        size = sizeof(U8);
    }
    else if (pCmd->szCmd[1] == 'w')
    {
        size = sizeof(U16);
    }
    else if (pCmd->szCmd[1] == 'l')
    {
        size = sizeof(U32);
    }
    else if (pCmd->szCmd[1] == 'q')
    {
        size = sizeof(U64);
    }
    else
    {
        return FALSE;
    }

    if (pCmd->NumArgs < 2)
    {
        Cons_printf(
            "Error: Missing parameters(s)\n"
            "Usage:\n"
            "   e<b,w,l,q> <virtual_address> <value1> [value2 value3 ... valueN]\n"
            );

        return TRUE;
    }

    // Get starting address
    pArg = CmdLine_ArgGet( pCmd, 0 );

    // Verify argument
    if (pArg->ArgType != PLXCM_ARG_TYPE_INT)
    {
        Cons_printf("Error: '%s' is not a valid address\n", pArg->ArgString);
        return FALSE;
    }

    // Set current address
    pAddr = PLX_CAST_64_TO_8_PTR( (PLX_UINT_PTR)pArg->ArgIntHex );

    bVerifyArgs = TRUE;

    do
    {
        for (i = 1; i < pCmd->NumArgs; i++)
        {
            // Get argument
            pArg = CmdLine_ArgGet( pCmd, i );

            if (bVerifyArgs)
            {
                // Verify arguments
                if (pArg->ArgType != PLXCM_ARG_TYPE_INT)
                {
                    Cons_printf(
                        "Error: Parameter %d (%s) is not a valid value\n",
                        i+1, pArg->ArgString
                        );
                    return FALSE;
                }
            }
            else
            {
                // Perform writes
                switch (size)
                {
                    case sizeof(U8):
                        PlxCm_MemWrite_8(pAddr, (U8)pArg->ArgIntHex);
                        break;

                    case sizeof(U16):
                        PlxCm_MemWrite_16(pAddr, (U16)pArg->ArgIntHex);
                        break;

                    case sizeof(U32):
                        PlxCm_MemWrite_32(pAddr, (U32)pArg->ArgIntHex);
                        break;
                }

                pAddr += size;
            }
        }

        if (bVerifyArgs == FALSE)
        {
            break;
        }

        bVerifyArgs = FALSE;
    }
    while (1);

    return TRUE;
}




/**********************************************************
 *
 * Function   :  Cmd_IoRead
 *
 * Description:
 *
 *********************************************************/
BOOLEAN Cmd_IoRead( PLX_DEVICE_OBJECT *pDevice, PLXCM_COMMAND *pCmd )
{
    int         i;
    int         x;
    int         CharsToPrint;
    int         SpacesToInsert;
    U8          buffer[20];
    U8          size;
    U32         ByteCount;
    U32         CurrValue;
    U64         EndAddr;
    static U64  CurrAddr = 0;
    BOOLEAN     bDone;
    BOOLEAN     bEndLine;
    PLXCM_ARG  *pArg;
    PLX_STATUS  status;


    if (pDevice == NULL)
    {
        Cons_printf("Error: No device selected\n");
        return FALSE;
    }

    // Set access size
    if (pCmd->szCmd[1] == 'b')
    {
        size = sizeof(U8);
    }
    else if (pCmd->szCmd[1] == 'w')
    {
        size = sizeof(U16);
    }
    else if (pCmd->szCmd[1] == 'l')
    {
        size = sizeof(U32);
    }
    else
    {
        return FALSE;
    }

    // Set current address
    if (pCmd->NumArgs >= 1)
    {
        // Get starting address
        pArg = CmdLine_ArgGet( pCmd, 0 );

        // Verify argument
        if (pArg->ArgType != PLXCM_ARG_TYPE_INT)
        {
            Cons_printf("Error: '%s' is not a valid I/O port\n", pArg->ArgString);
            return FALSE;
        }

        // Set current address
        CurrAddr = pArg->ArgIntHex;
    }

    // Check for count
    if (pCmd->NumArgs <= 1)
    {
        // Set default count to 80h
        EndAddr = 0x80;
    }
    else
    {
        pArg = CmdLine_ArgGet( pCmd, 1 );

        // Verify argument
        if (pArg->ArgType != PLXCM_ARG_TYPE_INT)
        {
            Cons_printf("Error: '%s' is not a valid byte count\n", pArg->ArgString);
            return FALSE;
        }

        // Set byte count
        EndAddr = pArg->ArgIntHex;
    }

    // Set final end address
    EndAddr   = CurrAddr + EndAddr;
    ByteCount = (U32)(EndAddr - CurrAddr);

    // Initialize to avoid compiler warning
    CharsToPrint   = 0;
    SpacesToInsert = 0;

    // Enable throttle output
    ConsoleIoThrottleSet(TRUE);

    i        = 0;
    bDone    = FALSE;
    bEndLine = FALSE;

    while (!bDone)
    {
        if (i == 0)
        {
            // Set initial number of characters to display
            CharsToPrint = 0;

            // Set initial spaces to insert
            SpacesToInsert = (U8)(((size * 2) + 1) * (16 / size));

            Cons_printf("%08x: ", CurrAddr);
        }

        switch (size)
        {
            case sizeof(U8):
                status =
                    PlxPci_IoPortRead(
                        pDevice,
                        CurrAddr,
                        &CurrValue,
                        sizeof(U8),
                        BitSize8
                        );
                Cons_printf( "%02x ", (U8)CurrValue );
                break;

            case sizeof(U16):
                status =
                    PlxPci_IoPortRead(
                        pDevice,
                        CurrAddr,
                        &CurrValue,
                        sizeof(U16),
                        BitSize16
                        );
                Cons_printf( "%04x ", (U16)CurrValue );
                break;

            case sizeof(U32):
                status =
                    PlxPci_IoPortRead(
                        pDevice,
                        CurrAddr,
                        &CurrValue,
                        sizeof(U32),
                        BitSize32
                        );
                Cons_printf( "%08x ", CurrValue );
                break;

            default:
                status = PLX_STATUS_UNSUPPORTED;
                break;
        }

        if (status != PLX_STATUS_OK)
        {
            Cons_printf(
                "\n"
                "Error: Unable to perform I/O read (status=%02Xh)\n",
                status
                );
            break;
        }

        // Store value for printing
        for (x = 0; x < size; x++)
        {
            buffer[i+x] = ((U8*)(&CurrValue))[x];

            // Verify printable character
            if ((buffer[i+x] != ' ') &&
                (!isgraph(buffer[i+x])))
            {
                buffer[i+x] = '.';
            }
        }

        i        += size;
        CurrAddr += size;

        // Adjust characters to print
        CharsToPrint += size;

        // Adjust spaces
        SpacesToInsert -= (size * 2) + 1;

        if (CurrAddr >= EndAddr)
        {
            bDone    = TRUE;
            bEndLine = TRUE;
        }

        if (!bEndLine)
        {
            if (i == 16)
            {
                i        = 0;
                bEndLine = TRUE;
            }
        }

        if (bEndLine)
        {
            bEndLine = FALSE;

            // Insert necessary spaces
            while (SpacesToInsert--)
            {
                Cons_printf(" ");
            }

            // Display characters at end of line
            for (x = 0; x < CharsToPrint; x++)
            {
                Cons_printf("%c", buffer[x]);
            }

            // Next line but check if user (Q)uit via throttle prompt
            if (Cons_printf("\n") == 0)
            {
                break;
            }

            // Check for user cancel for larger reads
            if ((ByteCount > MIN_BYTE_CHECK_CANCEL) && Cons_kbhit())
            {
                if (Cons_getch() == CONS_KEY_ESCAPE)
                {
                    Cons_printf(" - user abort\n");
                    break;
                }
            }
        }
    }

    // Disable throttle output
    ConsoleIoThrottleSet(FALSE);

    return TRUE;
}




/**********************************************************
 *
 * Function   :  Cmd_IoWrite
 *
 * Description:
 *
 *********************************************************/
BOOLEAN Cmd_IoWrite( PLX_DEVICE_OBJECT *pDevice, PLXCM_COMMAND *pCmd )
{
    U8          i;
    U8          size;
    U64         Addr;
    BOOLEAN     bVerifyArgs;
    PLXCM_ARG  *pArg;
    PLX_STATUS  status;


    if (pDevice == NULL)
    {
        Cons_printf("Error: No device selected\n");
        return FALSE;
    }

    // Set access size
    if (pCmd->szCmd[1] == 'b')
    {
        size = sizeof(U8);
    }
    else if (pCmd->szCmd[1] == 'w')
    {
        size = sizeof(U16);
    }
    else if (pCmd->szCmd[1] == 'l')
    {
        size = sizeof(U32);
    }
    else if (pCmd->szCmd[1] == 'q')
    {
        size = sizeof(U64);
    }
    else
    {
        return FALSE;
    }

    if (pCmd->NumArgs < 2)
    {
        Cons_printf(
            "Error: Missing parameter(s)\n"
            "Usage:\n"
            "   o%c <IO_Port> <value1> [value2 value3 ... valueN]\n",
            pCmd->szCmd[1]
            );
        return TRUE;
    }

    // Get starting address
    pArg = CmdLine_ArgGet( pCmd, 0 );

    // Verify argument
    if (pArg->ArgType != PLXCM_ARG_TYPE_INT)
    {
        Cons_printf("Error: '%s' is not a valid address\n", pArg->ArgString);
        return FALSE;
    }

    // Set current address
    Addr = pArg->ArgIntHex;

    bVerifyArgs = TRUE;

    do
    {
        for (i = 1; i < pCmd->NumArgs; i++)
        {
            // Get argument
            pArg = CmdLine_ArgGet( pCmd, i );

            if (bVerifyArgs)
            {
                // Verify arguments
                if (pArg->ArgType != PLXCM_ARG_TYPE_INT)
                {
                    Cons_printf(
                        "Error: Parameter %d (%s) is not a valid value\n",
                        i+1, pArg->ArgString
                        );
                    return FALSE;
                }
            }
            else
            {
                // Perform writes
                switch (size)
                {
                    case sizeof(U8):
                        status =
                            PlxPci_IoPortWrite(
                                pDevice,
                                Addr,
                                &pArg->ArgIntHex,
                                sizeof(U8),
                                BitSize8
                                );
                        break;

                    case sizeof(U16):
                        status =
                            PlxPci_IoPortWrite(
                                pDevice,
                                Addr,
                                &pArg->ArgIntHex,
                                sizeof(U16),
                                BitSize16
                                );
                        break;

                    case sizeof(U32):
                        status =
                            PlxPci_IoPortWrite(
                                pDevice,
                                Addr,
                                &pArg->ArgIntHex,
                                sizeof(U32),
                                BitSize32
                                );
                        break;

                    default:
                        status = PLX_STATUS_UNSUPPORTED;
                        break;
                }

                if (status != PLX_STATUS_OK)
                {
                    Cons_printf(
                        "Error: Unable to perform I/O write (status=%02Xh)\n",
                        status
                        );
                    break;
                }

                Addr += size;
            }
        }

        if (bVerifyArgs == FALSE)
        {
            break;
        }

        bVerifyArgs = FALSE;
    }
    while (1);

    return TRUE;
}




/**********************************************************
 *
 * Function   :  Cmd_RegPci
 *
 * Description:  Implements the PCI register command
 *
 *********************************************************/
BOOLEAN Cmd_RegPci( PLX_DEVICE_OBJECT *pDevice, PLXCM_COMMAND *pCmd )
{
    char         *pStr;
    U8            i;
    U16           offset;
    U32           value;
    BOOLEAN       bRead;
    PLXCM_ARG    *pArg;
    PLX_STATUS    status;
    DEVICE_NODE  *pNode;
    REGISTER_SET *RegSet;


    if (pDevice == NULL)
    {
        Cons_printf("Error: No device selected\n");
        return FALSE;
    }

    if (pCmd->NumArgs > 2)
    {
        Cons_printf("Usage: pcr [offset [value]]\n");
        return FALSE;
    }

    // Initialize to avoid compiler warning
    offset = 0;
    value  = 0;

    // Verify arguments
    if (pCmd->NumArgs >= 1)
    {
        // Get & verify offet
        pArg = CmdLine_ArgGet( pCmd, 0 );

        if ( (pArg->ArgType != PLXCM_ARG_TYPE_INT) || (pArg->ArgIntHex & 0x3) )
        {
            Cons_printf("Error: '%s' is not a valid 4B aliged register offset\n", pArg->ArgString);
            return FALSE;
        }

        if (pArg->ArgIntHex >= PCIE_CONFIG_SPACE_SIZE)
        {
            Cons_printf("Error: Offset '%s' exceeds PCIe config space (4K)\n", pArg->ArgString);
            return FALSE;
        }

        offset = (U16)pArg->ArgIntHex;

        // Get data value if supplied
        if (pCmd->NumArgs == 2)
        {
            pArg = CmdLine_ArgGet( pCmd, 1 );

            if ((pArg->ArgType != PLXCM_ARG_TYPE_INT) ||
                (PLX_64_HIGH_32(pArg->ArgIntHex) != 0))
            {
                Cons_printf("Error: '%s' is not a valid 32-bit value\n", pArg->ArgString);
                return FALSE;
            }

            value = (U32)pArg->ArgIntHex;
        }
    }

    // Select register set
    switch (pDevice->Key.PlxChip)
    {
        case 0x9030:
        case 0x9056:
        case 0x9656:
        case 0x9054:
        case 0x8311:
            RegSet = Pci9054;
            break;

        case 0x6540:
            RegSet = Pci6540;
            break;

        case 0x8111:
        case 0x8112:
            RegSet = Pci8111;
            break;

        default:
            pNode = DeviceNodeGetByKey( &pDevice->Key );

            if (pNode == NULL)
            {
                return FALSE;
            }

            // Check Header Type
            if (pNode->PciHeaderType == PCI_HDR_TYPE_1)
            {
                // Type 1 - P-to-P Bridge
                RegSet = Pci_Type_1;
            }
            else if (pNode->PciHeaderType == PCI_HDR_TYPE_2)
            {
                // Type 2 - CardBus Bridge
                RegSet = Pci_Type_2;
            }
            else
            {
                // Type 0 - All other devices
                RegSet = Pci_Type_0;
            }
            break;
    }

    // Operation is a read if less than 2 parameters
    if (pCmd->NumArgs < 2)
    {
        bRead = TRUE;
    }
    else
    {
        bRead = FALSE;
    }

    // Enable throttle output
    ConsoleIoThrottleSet(TRUE);

    i = 0;

    do
    {
        if (pCmd->NumArgs == 0)
        {
            offset = RegSet[i].Offset;

            // Check for final entry
            if (offset == 0xFFF)
            {
                break;
            }
        }

        // Access device
        if (bRead)
        {
            value = PlxPci_PciRegisterReadFast( pDevice, offset, &status );
            Cons_printf( " %03x: ", offset );

            if (status != PLX_STATUS_OK)
            {
                Cons_printf("????????\n");
            }
            else
            {
                Cons_printf("%08x", value);

                // Store last value in return code
                Gbl_LastRetVal = value;

                // Display description if available
                pStr = RegSet_DescrGetByOffset( RegSet, offset );
                if (pStr != NULL)
                {
                    Cons_printf( "  %s", pStr );
                }

                // Next line but check if user (Q)uit via throttle prompt
                if (Cons_printf("\n") == 0)
                {
                    break;
                }
            }
        }
        else
        {
            status = PlxPci_PciRegisterWriteFast( pDevice, offset, value );
        }

        if (status != PLX_STATUS_OK)
        {
            if (status == PLX_STATUS_INVALID_OFFSET)
            {
                Cons_printf("Error: Register offset invalid or exceeds max\n");
            }
            else if (status == PLX_STATUS_UNSUPPORTED)
            {
                Cons_printf("Error: PCI register access is not supported\n");
            }
            else
            {
                Cons_printf("Error: Register access failed (status=%02Xh)\n", status);
            }
            break;
        }

        // Jump to next item or quit
        if (pCmd->NumArgs == 0)
        {
            i++;
        }
        else
        {
            break;
        }
    }
    while (1);

    // Disable throttle output
    ConsoleIoThrottleSet(FALSE);

    return TRUE;
}




/**********************************************************
 *
 * Function   :  Cmd_RegPlx
 *
 * Description:  Implements the register commands
 *
 *********************************************************/
BOOLEAN Cmd_RegPlx( PLX_DEVICE_OBJECT *pDevice, PLXCM_COMMAND *pCmd )
{
    char         *pStr;
    U8            i;
    U8            Regs;
    U32           offset;
    U32           value;
    U32           PlxChip;
    BOOLEAN       bRead;
    BOOLEAN       bAdjustForPort;
    PLXCM_ARG    *pArg;
    PLX_STATUS    status;
    REGISTER_SET *RegSet;


    if (pDevice == NULL)
    {
        Cons_printf("Error: No device selected\n");
        return FALSE;
    }

    if (pCmd->NumArgs > 2)
    {
        Cons_printf("Usage: %s [offset [value]]\n", pCmd->szCmd);
        return FALSE;
    }

    // Check if device is supported
    if (pDevice->Key.PlxChip == 0)
    {
        Cons_printf("Error: This command only supports PLX devices\n");
        return FALSE;
    }

    // Initialize to avoid compiler warning
    offset = 0;
    value  = 0;

    // Verify arguments
    if (pCmd->NumArgs >= 1)
    {
        // Get & verify offet
        pArg = CmdLine_ArgGet( pCmd, 0 );

        if ((pArg->ArgType != PLXCM_ARG_TYPE_INT) || (pArg->ArgIntHex & 0x3))
        {
            Cons_printf("Error: '%s' is not a valid 4B aligned offset\n", pArg->ArgString);
            return FALSE;
        }

        offset = (U32)pArg->ArgIntHex;

        // Get data value if supplied
        if (pCmd->NumArgs == 2)
        {
            pArg = CmdLine_ArgGet( pCmd, 1 );

            if ((pArg->ArgType != PLXCM_ARG_TYPE_INT) ||
                (PLX_64_HIGH_32(pArg->ArgIntHex) != 0))
            {
                Cons_printf("Error: '%s' is not a valid 32-bit value\n", pArg->ArgString);
                return FALSE;
            }

            value = (U32)pArg->ArgIntHex;
        }
    }

    // Generalize by chip type
    PlxChip = pDevice->Key.PlxChip;

    if (((PlxChip & 0xFF00) == 0x2300) ||
        ((PlxChip & 0xFF00) == 0x3300) ||
        ((PlxChip & 0xFF00) == 0x8500) ||
        ((PlxChip & 0xFF00) == 0x8600) ||
        ((PlxChip & 0xFF00) == 0x8700) ||
        ((PlxChip & 0xFF00) == 0x9700) ||
        ((PlxChip & 0xFF00) == 0xC000))
    {
        PlxChip = PlxChip & 0xFF00;
    }
    else if ((PlxChip & 0xF000) == 0x6000)
    {
        PlxChip = PlxChip & 0xF000;
    }

    if ((Plx_strcasecmp( pCmd->szCmd, "reg" ) == 0) ||
        (Plx_strcasecmp( pCmd->szCmd, "mmr" ) == 0) ||
        (Plx_strcasecmp( pCmd->szCmd, "lcr" ) == 0))
    {
        Regs = REGS_LCR;
    }
    else if (Plx_strcasecmp( pCmd->szCmd, "rtr" ) == 0)
    {
        Regs = REGS_RTR;
    }
    else if (Plx_strcasecmp( pCmd->szCmd, "dma" ) == 0)
    {
        Regs = REGS_DMA;
    }
    else if (Plx_strcasecmp( pCmd->szCmd, "mqr" ) == 0)
    {
        Regs = REGS_MQR;
    }
    else
    {
        return FALSE;
    }

    RegSet = NULL;

    // Select register set
    switch (PlxChip)
    {
        case 0x9050:
            if (Regs == REGS_LCR)
            {
                RegSet = Lcr9050;
            }
            break;

        case 0x9030:
            if (Regs == REGS_LCR)
            {
                RegSet = Lcr9030;
            }
            break;

        case 0x8111:
        case 0x8112:
            if (Regs == REGS_LCR)
            {
                RegSet = Lcr8111;
            }
            break;

        case 0x9080:
            switch (Regs)
            {
                case REGS_LCR:
                    RegSet = Lcr9080;
                    break;

                case REGS_RTR:
                    RegSet = Rtr9080;
                    break;

                case REGS_DMA:
                    RegSet = Dma9080;
                    break;

                case REGS_MQR:
                    RegSet = Mqr9080;
                    break;
            }
            break;

        case 0x9056:
        case 0x9656:
        case 0x8311:
            switch (Regs)
            {
                case REGS_LCR:
                    RegSet = Lcr9656;
                    break;

                case REGS_RTR:
                    RegSet = Rtr9080;
                    break;

                case REGS_DMA:
                    RegSet = Dma9054;
                    break;

                case REGS_MQR:
                    RegSet = Mqr9080;
                    break;
            }
            break;

        case 0x9054:
            switch (Regs)
            {
                case REGS_LCR:
                    RegSet = Lcr9054;
                    break;

                case REGS_RTR:
                    RegSet = Rtr9080;
                    break;

                case REGS_DMA:
                    RegSet = Dma9054;
                    break;

                case REGS_MQR:
                    RegSet = Mqr9080;
                    break;
            }
            break;

        case 0x2300:
        case 0x3300:
        case 0x8114:
        case 0x8500:
        case 0x8600:
        case 0x8700:
        case 0x9700:
            if (Regs == REGS_LCR)
            {
                RegSet = Lcr8500;
            }
            break;

        case 0xC000:
            if (Regs == REGS_LCR)
            {
                RegSet = LcrGeneric;
            }
            break;

        case 0x6000:
            Cons_printf(
                "Error: %04X PCI-to-PCI Bridge does not contain local registers\n",
                pDevice->Key.PlxChip
                );
            return FALSE;

        default:
            Cons_printf(
                "Error: Local registers not available on %X devices\n",
                pDevice->Key.PlxChip
                );
            return FALSE;
    }

    if (RegSet == NULL)
    {
        Cons_printf(
            "Error: Invalid register set for %X device\n",
            pDevice->Key.PlxChip
            );
        return FALSE;
    }

    // Operation is a read if less than 2 parameters
    if (pCmd->NumArgs < 2)
    {
        bRead = TRUE;
    }
    else
    {
        bRead = FALSE;
    }

    // Check if offset should be adjusted for port
    if (Plx_strcasecmp( pCmd->szCmd, "mmr" ) == 0)
    {
        bAdjustForPort = FALSE;
    }
    else
    {
        bAdjustForPort = TRUE;
    }

    // Enable throttle output
    ConsoleIoThrottleSet(TRUE);

    i = 0;

    do
    {
        if (pCmd->NumArgs == 0)
        {
            offset = RegSet[i].Offset;

            // Check for final entry
            if (offset == 0xFFF)
            {
                break;
            }
        }

        // Access device
        if (bRead)
        {
            if (bAdjustForPort)
            {
                value = PlxPci_PlxRegisterRead( pDevice, offset, &status );
            }
            else
            {
                value = PlxPci_PlxMappedRegisterRead( pDevice, offset, &status );
            }

            Cons_printf( " %03x: ", offset );
            if (status != PLX_STATUS_OK)
            {
                Cons_printf("????????\n");
            }
            else
            {
                Cons_printf( "%08x", value );

                // Store last value in return code
                Gbl_LastRetVal = value;

                // Display description if available
                pStr = RegSet_DescrGetByOffset( RegSet, offset );
                if (pStr != NULL)
                {
                    Cons_printf( "  %s", pStr );
                }

                // Next line but check if user (Q)uit via throttle prompt
                if (Cons_printf("\n") == 0)
                {
                    break;
                }
            }
        }
        else
        {
            if (bAdjustForPort)
            {
                status = PlxPci_PlxRegisterWrite( pDevice, offset, value );
            }
            else
            {
                status = PlxPci_PlxMappedRegisterWrite( pDevice, offset, value );
            }
        }

        if (status != PLX_STATUS_OK)
        {
            if (status == PLX_STATUS_INVALID_OFFSET)
            {
                Cons_printf("Error: Register offset invalid or exceeds max\n");
            }
            else if (status == PLX_STATUS_UNSUPPORTED)
            {
                Cons_printf("Error: Device not supported or doesn't have device-specific registers\n");
            }
            else
            {
                Cons_printf("Error: Register access failed (status=%d)\n", status);
            }
            break;
        }

        // Jump to next item or quit
        if (pCmd->NumArgs == 0)
        {
            i++;
        }
        else
        {
            break;
        }
    }
    while (1);

    // Disable throttle output
    ConsoleIoThrottleSet(FALSE);

    return TRUE;
}




/**********************************************************
 *
 * Function   :  Cmd_RegDump
 *
 * Description:  Dumps PLX registers
 *
 *********************************************************/
BOOLEAN Cmd_RegDump( PLX_DEVICE_OBJECT *pDevice, PLXCM_COMMAND *pCmd )
{
    int         i;
    U32         CurrValue = 0;
    U32         ByteCount;
    U32         OffsetEnd;
    static U32  OffsetCurr = 0;
    BOOLEAN     bDone;
    BOOLEAN     bRepeat;
    BOOLEAN     bPciRegs;
    BOOLEAN     bEndLine;
    PLXCM_ARG  *pArg;
    PLX_STATUS  status;


    if (pDevice == NULL)
    {
        Cons_printf("Error: No device selected\n");
        return FALSE;
    }

    // Determine register set to access
    if (pCmd->szCmd[1] == 'p')
    {
        bPciRegs = TRUE;
    }
    else if (pCmd->szCmd[1] == 'r')
    {
        bPciRegs = FALSE;
    }
    else
    {
        return FALSE;
    }

    // Set current address
    if (pCmd->NumArgs >= 1)
    {
        // Get starting offset
        pArg = CmdLine_ArgGet( pCmd, 0 );

        // Verify argument
        if (pArg->ArgType != PLXCM_ARG_TYPE_INT)
        {
            Cons_printf("Error: '%s' is not a valid offset\n", pArg->ArgString);
            return FALSE;
        }

        // Set current offset
        OffsetCurr = (U32)pArg->ArgIntHex;
    }

    if (OffsetCurr & 0x3)
    {
        Cons_printf("Error: Starting offset must be 4B aligned\n");
        return FALSE;
    }

    // Set byte count
    if (pCmd->NumArgs <= 1)
    {
        // Set default if not provided
        ByteCount = 0x80;
    }
    else
    {
        pArg = CmdLine_ArgGet( pCmd, 1 );
        if (pArg->ArgType != PLXCM_ARG_TYPE_INT)
        {
            Cons_printf("Error: '%s' is not a valid byte count\n", pArg->ArgString);
            return FALSE;
        }
        ByteCount = (U32)pArg->ArgIntHex;
    }

    if (ByteCount & 0x3)
    {
        Cons_printf("Error: Byte count must be a multiple of 4B\n");
        return FALSE;
    }

    // Set final end offset
    OffsetEnd = OffsetCurr + ByteCount;

    bRepeat = FALSE;

    // Enable throttle output
    ConsoleIoThrottleSet(TRUE);

    do
    {
        i        = 0;
        bDone    = FALSE;
        bEndLine = FALSE;

        while (!bDone)
        {
            // Display offset
            if (i == 0)
            {
                Cons_printf("%05x: ", OffsetCurr);
            }

            // Perform the register access
            if (bPciRegs)
            {
                // Verify offset in valid PCIe config space
                if (OffsetCurr >= PCIE_CONFIG_SPACE_SIZE)
                {
                    status = PLX_STATUS_INVALID_OFFSET;
                }
                else
                {
                    CurrValue = PlxPci_PciRegisterReadFast( pDevice, (U16)OffsetCurr, &status );
                }
            }
            else
            {
                CurrValue = PlxPci_PlxMappedRegisterRead( pDevice, OffsetCurr, &status );
            }

            if (status != PLX_STATUS_OK)
            {
                Cons_printf("????????\n");
                if (status == PLX_STATUS_INVALID_OFFSET)
                {
                    Cons_printf("Error: Register offset invalid or exceeds max\n");
                }
                else if (status == PLX_STATUS_UNSUPPORTED)
                {
                    Cons_printf("Error: Device not supported or doesn't have device-specific registers\n");
                }
                else
                {
                    Cons_printf("Error: Register access failed (status=%d)\n", status);
                }
                break;
            }

            // Store last value in return code
            Gbl_LastRetVal = CurrValue;

            // Display the value
            Cons_printf("%08x ", CurrValue);

            i          += sizeof(U32);
            OffsetCurr += sizeof(U32);

            if (OffsetCurr >= OffsetEnd)
            {
                bDone    = TRUE;
                bEndLine = TRUE;
            }

            if (!bEndLine)
            {
                if (i == 16)
                {
                    i        = 0;
                    bEndLine = TRUE;
                }
            }

            if (bEndLine)
            {
                bEndLine = FALSE;

                // Next line but check if user (Q)uit via throttle prompt
                if (Cons_printf("\n") == 0)
                {
                    break;
                }

                // Check for user cancel for larger reads
                if ((ByteCount > MIN_BYTE_CHECK_CANCEL) && Cons_kbhit())
                {
                    if (Cons_getch() == CONS_KEY_ESCAPE)
                    {
                        Cons_printf(" - user abort\n");
                        break;
                    }
                }
            }
        }
    }
    while (bRepeat);

    // Disable throttle output
    ConsoleIoThrottleSet(FALSE);

    return TRUE;
}




/**********************************************************
 *
 * Function   :  Cmd_Eep
 *
 * Description:  Implements the EEPROM commands
 *
 *********************************************************/
BOOLEAN Cmd_Eep( PLX_DEVICE_OBJECT *pDevice, PLXCM_COMMAND *pCmd )
{
    char         *pStr;
    U8            i;
    U16           offset;
    U16           value_16;
    U32           value_32;
    U32           PlxChip;
    BOOLEAN       b16Bit;
    BOOLEAN       bRead;
    PLXCM_ARG    *pArg;
    PLX_STATUS    status;
    DEVICE_NODE  *pNode;
    REGISTER_SET *RegSet;


    if (pDevice == NULL)
    {
        Cons_printf("Error: No device selected\n");
        return FALSE;
    }

    // Check if device is supported
    if (pDevice->Key.PlxChip == 0)
    {
        Cons_printf("Error: This command only supports PLX devices\n");
        return FALSE;
    }

    if (pCmd->NumArgs > 2)
    {
        Cons_printf("Usage: eep [offset [value]]\n");
        return FALSE;
    }

    // Initialize to avoid compiler warning
    offset = 0;

    // Default to 32-bit data
    b16Bit = FALSE;

    // Generalize by chip type
    PlxChip = pDevice->Key.PlxChip;

    if (((PlxChip & 0xFF00) == 0x2300) ||
        ((PlxChip & 0xFF00) == 0x3300) ||
        ((PlxChip & 0xFF00) == 0x8500) ||
        ((PlxChip & 0xFF00) == 0x8600) ||
        ((PlxChip & 0xFF00) == 0x8700) ||
        ((PlxChip & 0xFF00) == 0x9700))
    {
        PlxChip = PlxChip & 0xFF00;
    }

    // Select correct data set
    switch (PlxChip)
    {
        case 0x9050:
            RegSet = Eep9050;
            break;

        case 0x9030:
            RegSet = Eep9030;
            break;

        case 0x9080:
            RegSet = Eep9080;
            break;

        case 0x9054:
            RegSet = Eep9054;
            break;

        case 0x9056:
        case 0x9656:
        case 0x8311:
            RegSet = Eep9656;
            break;

        case 0x6254:
            RegSet = Eep6254;
            b16Bit = TRUE;
            break;

        case 0x6540:
            RegSet = Eep6540;
            b16Bit = TRUE;
            break;

        case 0x8111:
        case 0x8112:
            RegSet = Eep8111;
            break;

        case 0x2300:
        case 0x3300:
        case 0x8114:
        case 0x8500:
        case 0x8600:
        case 0x8700:
        case 0x9700:
            RegSet = Eep8500;
            break;

        case 0x6140:
        case 0x6150:
        case 0x6152:
        case 0x6154:
        case 0x6156:
        case 0x6350:
        case 0x6520:
        default:
            Cons_printf(
                "** %X EEPROM access not implemented\n",
                pDevice->Key.PlxChip
                );
            return FALSE;
    }

    // Verify arguments
    if (pCmd->NumArgs >= 1)
    {
        // Get & verify offet
        pArg = CmdLine_ArgGet( pCmd, 0 );

        if ((pArg->ArgType != PLXCM_ARG_TYPE_INT)          ||   // Hex value
            ((b16Bit == TRUE)  && (pArg->ArgIntHex & 0x1)) ||   // Aligned on 16-bit boundary
            ((b16Bit == FALSE) && (pArg->ArgIntHex & 0x3)))     // Aligned on 32-bit boundary
        {
            Cons_printf("Error: '%s' is not a valid offset\n", pArg->ArgString);
            return FALSE;
        }

        offset = (U16)pArg->ArgIntHex;

        // Get data value if supplied
        if (pCmd->NumArgs == 2)
        {
            pArg = CmdLine_ArgGet( pCmd, 1 );

            if ((pArg->ArgType != PLXCM_ARG_TYPE_INT)        ||     // Hex value
                (b16Bit && (pArg->ArgIntHex & ~(U64)0xFFFF)) ||     // Valid 16-bit value
                (PLX_64_HIGH_32(pArg->ArgIntHex)))                  // Valid 32-bit value
            {
                Cons_printf("Error: '%s' is not a valid value\n", pArg->ArgString);
                return FALSE;
            }

            value_32 = (U32)pArg->ArgIntHex;
        }
    }

    // Get corresponding node
    pNode = DeviceNodeGetByKey( &pDevice->Key );

    if (pNode == NULL)
    {
        return FALSE;
    }

    // Check if an EEPROM is present
    if (pNode->bEepromVerified == FALSE)
    {
        if (PlxPci_EepromPresent(
                pDevice,
                &status
                ) == PLX_EEPROM_STATUS_NONE)
        {
            if (status == PLX_STATUS_UNSUPPORTED)
            {
                Cons_printf("Error: EEPROM access not supported for this device\n");
                return TRUE;
            }

            Cons_printf(
                " -- The chip reports no EEPROM present --\n"
                "\n"
                "Do you want to proceed [y/n]? "
                );

            // Wait for input
            i = (U8)Cons_getch();

            Cons_printf("%c\n", i);

            if (tolower(i) != 'y')
            {
                return TRUE;
            }
        }

        // Verified EEPROM is present or ignore
        pNode->bEepromVerified = TRUE;
    }

    // Process some 8000 devices differently
    if (pCmd->NumArgs == 0)
    {
        // Reset 8500 to PLX chip type
        if (PlxChip == 0x8500)
        {
            PlxChip = pDevice->Key.PlxChip;
        }

        switch (PlxChip)
        {
            case 0x2300:
            case 0x3300:
            case 0x8111:
            case 0x8112:
            case 0x8505:
            case 0x8509:
            case 0x8525:
            case 0x8533:
            case 0x8547:
            case 0x8548:
            case 0x8600:
            case 0x8700:
            case 0x9700:
                return Cmd_Eep8000( pDevice, pCmd );
        }
    }

    // Operation is a read if less than 2 parameters
    if (pCmd->NumArgs < 2)
    {
        bRead = TRUE;
    }
    else
    {
        bRead = FALSE;
    }

    // Enable throttle output
    ConsoleIoThrottleSet(TRUE);

    i = 0;

    do
    {
        // Get offset if not specified
        if (pCmd->NumArgs == 0)
        {
            offset = RegSet[i].Offset;

            // Check for final entry
            if (offset == 0xFFF)
            {
                break;
            }
        }

        // Access device
        if (bRead)
        {
            if (b16Bit)
            {
                PlxPci_EepromReadByOffset_16( pDevice, offset, &value_16 );
                Cons_printf( " %02x: %04x", offset, value_16 );

                // Store last value in return code
                Gbl_LastRetVal = value_16;
            }
            else
            {
                PlxPci_EepromReadByOffset( pDevice, offset, &value_32 );
                Cons_printf( " %03x: %08x", offset, value_32 );

                // Store last value in return code
                Gbl_LastRetVal = value_32;
            }

            // Display description if available
            pStr = RegSet_DescrGetByOffset( RegSet, offset );
            if (pStr != NULL)
            {
                Cons_printf( "  %s", pStr );
            }

            // Next line but check if user (Q)uit via throttle prompt
            if (Cons_printf("\n") == 0)
            {
                break;
            }
        }
        else
        {
            if (b16Bit)
            {
                PlxPci_EepromWriteByOffset_16( pDevice, offset, (U16)value_32 );
            }
            else
            {
                PlxPci_EepromWriteByOffset( pDevice, offset, value_32 );
            }
        }

        // Jump to next item or quit
        if (pCmd->NumArgs == 0)
        {
            i++;
        }
        else
        {
            break;
        }
    }
    while (1);

    // Disable throttle output
    ConsoleIoThrottleSet(FALSE);

    return TRUE;
}




/**********************************************************
 *
 * Function   :  Cmd_Eep8000
 *
 * Description:  Implements the EEPROM commands
 *
 *********************************************************/
BOOLEAN Cmd_Eep8000( PLX_DEVICE_OBJECT *pDevice, PLXCM_COMMAND *pCmd )
{
    S16 count;
    S16 ByteCount;
    U16 port;
    U16 Offset;
    U16 Value_16;
    U32 EepHeader;
    U32 Data;


    // Enable throttle output
    ConsoleIoThrottleSet(TRUE);

    Cons_printf(
        "\n   ------ %04X EEPROM Header ------\n",
        pDevice->Key.PlxChip
        );

    // Get EEPROM header
    PlxPci_EepromReadByOffset( pDevice, 0, &EepHeader );

    ByteCount = (U16)(EepHeader >> 16);

    // Diplay header information
    Cons_printf(
        " Signature    : %02X [%s]\n",
        (U8)EepHeader, (U8)EepHeader == 0x5A ? "Valid" : "Invalid"
        );

    if (pDevice->Key.PlxFamily == PLX_FAMILY_BRIDGE_PCIE_P2P)
    {
        Cons_printf(
            " Load Regs    : %s\n"
            " Load Sh Mem  : %s\n",
            (EepHeader & (1 << 8)) ? "Yes" : "No",
            (EepHeader & (1 << 9)) ? "Yes" : "No"
            );
    }
    else if (pDevice->Key.PlxFamily == PLX_FAMILY_MIRA)
    {
        Cons_printf(
            " Load Regs    : %s\n"
            " Load 8051 Mem: %s\n"
            " Start 8051   : %s\n",
            (EepHeader & (1 << 8)) ? "No" : "Yes",   // On Mira, [8]=1 is DISABLE reg load
            (EepHeader & (1 << 9)) ? "Yes" : "No",
            (EepHeader & (1 << 10)) ? "Yes" : "No"
            );
    }
    else if ((pDevice->Key.PlxFamily == PLX_FAMILY_DRACO_2)   ||
             (pDevice->Key.PlxFamily == PLX_FAMILY_CAPELLA_1) ||
             (pDevice->Key.PlxFamily == PLX_FAMILY_CAPELLA_2))
    {
        Cons_printf(
            " CRC          : %s\n",
            (EepHeader & (1 << 15)) ? "ENABLED" : "DISABLED"
            );
    }

    Cons_printf( " Registers    : %d bytes (%d regs)\n", ByteCount, ByteCount / 6 );

    if (ByteCount != 0)
    {
        Cons_printf(
            "\n"
            "   #    Port     Offset     Value\n"
            " -----------------------------------\n"
            );
    }

    // Start at first register
    Offset = 0x4;

    while (ByteCount > 0)
    {
        // Display count
        Cons_printf(" %3d", (Offset - 4) / 6);

        // Get register port & offset
        PlxPci_EepromReadByOffset_16( pDevice, Offset, &Value_16 );

        // Get bits [15:10] for port number
        port = (Value_16 >> 10) & 0x3F;

        // Display port
        switch (pDevice->Key.PlxFamily)
        {
            case PLX_FAMILY_BRIDGE_PCIE_P2P:
                Cons_printf("     --       ");
                break;

            case PLX_FAMILY_ALTAIR:
            case PLX_FAMILY_ALTAIR_XL:
                // Port numbers are 1:1 correspondence except for 12 & higher
                if (port >= 16)
                    port = port - 4;

                Cons_printf("     %02d       ", port);
                break;

            case PLX_FAMILY_DENEB:
                if ((port & 0x3F) == 0x38)      // [15:10]=11_1000b - NT Link
                    Cons_printf("  NT Link     ");
                else
                {
                    port = ((port & ~0x7) >> 1) + (port & 0x3); // Transparent ports
                    Cons_printf("     %02d       ", port);
                }
                break;

            case PLX_FAMILY_SIRIUS:
                if ((port & 0x3F) == 0x30)      // [15:10]=11_0000b - NT Link
                    Cons_printf("  NT Link     ");
                else if ((port & 0x3F) == 0x31) // [15:10]=11_0001b - NT P2P
                    Cons_printf("  NT P2P      ");
                else if ((port & 0x3F) == 0x32) // [15:10]=11_0010b - DMA
                    Cons_printf("    DMA       ");
                else if ((port & 0x3F) == 0x33) // [15:10]=11_0011b - DMA RAM
                    Cons_printf("  DMA RAM     ");
                else
                    Cons_printf("     %02d       ", port); // Transparent ports are 1:1
                break;

            case PLX_FAMILY_CYGNUS:
                if ((port & 0x3F) == 0x30)      // [15:10]=11_0000b - NT Virtual
                    Cons_printf("  NT Virtual  ");
                else if ((port & 0x3F) == 0x38) // [15:10]=11_1000b - NT Link
                    Cons_printf("  NT Link     ");
                else
                {
                    port = ((port & ~0x7) >> 1) + (port & 0x7); // Transparent ports
                    Cons_printf("     %02d       ", port);
                }
                break;

            case PLX_FAMILY_MIRA:
            case PLX_FAMILY_SCOUT:
                Cons_printf("     %02d       ", port); // Transparent ports are 1:1
                break;

            case PLX_FAMILY_DRACO_1:
            case PLX_FAMILY_DRACO_2:
                if ((port & 0x38) == 0x28)      // [15:10]=10_1xxxb - DMA channels
                {
                    // Lower 3 bits are DMA channel # or descriptor RAM (4)
                    if ((port & 0x7) == 4)
                        Cons_printf("  DMA RAM     ");
                    else
                        Cons_printf("   DMA %d      ", (port & 0x7));
                }
                else if ((port & 0x3C) == 0x20) // [15:10]=10_00xxb - ALUT RAM
                {
                    // Lower 2 bits are ALUT #
                    Cons_printf(" ALUT RAM %d   ", (port & 0x3));
                }
                else if ((port & 0x38) == 0x30) // [15:10]=11_0xxxb - VS station-specific
                {
                    // Lower 3 bits are station #
                    Cons_printf(" VS Mode S%dP0 ", (port & 0x7));
                }
                else if ((port & 0x38) == 0x38) // [15:10]=11_1xxxb - NT ports
                {
                    // Port[1]=NT # & Port[0]=Virtual(1)/Link(0)
                    Cons_printf(
                        "  NT%d %s ",
                        ((port >> 1) & 1),
                        (port & (1 << 0)) ? "Virtual" : "Link   "
                        );
                }
                else
                {
                    Cons_printf("     %02d       ", port); // Transparent ports are 1:1
                }
                break;

            case PLX_FAMILY_CAPELLA_1:
                if ((port & 0x3C) == 0x2C)      // [15:10]=10_11xxb - ALUT
                {
                    // Lower 2 bits are ALUT #
                    Cons_printf(" ALUT RAM %d   ", (port & 0x3));
                }
                else if ((port & 0x38) == 0x30) // [15:10]=11_0xxxb - VS station-specific
                {
                    // Lower 3 bits are station #
                    Cons_printf(" VS Mode S%dP0 ", (port & 0x7));
                }
                else if ((port & 0x38) == 0x38) // [15:10]=11_1xxxb - NT ports
                {
                    // Bit 1 is NT# & bit 0 is NT port (0=NTV, 1=NTL)
                    Cons_printf(
                        "  NT%d %s ",
                        ((port >> 1) & 1),
                        (port & (1 << 0)) ? "Virtual" : "Link   "
                        );
                }
                else
                {
                    port = ((port & ~0x7) >> 1) + (port & 0x7); // Transparent ports
                    Cons_printf("     %02d       ", port);
                }
                break;

            case PLX_FAMILY_CAPELLA_2:
            default:
                Cons_printf("  UNKNOWN     ");
                break;
        }

        // DWORD index (not offset) is [9:0]
        if (pDevice->Key.PlxFamily != PLX_FAMILY_BRIDGE_PCIE_P2P)
        {
            Value_16 = (Value_16 & 0x3FF) << 2;
        }

        // Display register offset
        Cons_printf("%04X     ", Value_16);

        // Go to reg data (lower 16-bits)
        Offset += sizeof(U16);

        // Get lower 16-bits of data
        PlxPci_EepromReadByOffset_16( pDevice, Offset, &Value_16 );
        Data    = Value_16;
        Offset += sizeof(U16);

        // Get upper 16-bits of data
        PlxPci_EepromReadByOffset_16( pDevice, Offset, &Value_16 );
        Data   |= ((U32)Value_16) << 16;
        Offset += sizeof(U16);

        // Update byte count
        ByteCount -= (sizeof(U16) + sizeof(U32));

        Cons_printf("%08X\n", Data);
    }

    // Display shared memory
    if ( ((pDevice->Key.PlxFamily == PLX_FAMILY_BRIDGE_PCIE_P2P) ||
          (pDevice->Key.PlxFamily == PLX_FAMILY_MIRA)) &&
          (EepHeader & (1 << 9)) )
    {
        // Get shared mem size
        PlxPci_EepromReadByOffset_16( pDevice, Offset, (U16*)&ByteCount );

        // Go to next data
        Offset += sizeof(U16);

        Cons_printf(
            "\n\n"
            "   --- %s Memory (%d bytes) ---",
            (pDevice->Key.PlxFamily == PLX_FAMILY_BRIDGE_PCIE_P2P) ? "Shared" : "8051",
            ByteCount
            );

        count = 0;

        while (ByteCount > 0)
        {
            // Display start of line
            if ((count & 0xF) == 0)
            {
                Cons_printf("\n %03X:", count);
            }

            // Get next data
            PlxPci_EepromReadByOffset_16( pDevice, Offset, &Value_16 );

            Cons_printf(" %04X", Value_16);

            // Adjust count
            ByteCount -= sizeof(U16);
            count     += sizeof(U16);

            // Go to next data
            Offset += sizeof(U16);
        }

        Cons_printf("\n\n");
    }

    // Display CRC for devices that support it
    if ((pDevice->Key.PlxFamily == PLX_FAMILY_DRACO_2)   ||
        (pDevice->Key.PlxFamily == PLX_FAMILY_CAPELLA_1) ||
        (pDevice->Key.PlxFamily == PLX_FAMILY_CAPELLA_2))
    {
        // Get CRC & status if enabled
        if (EepHeader & (1 << 15))
        {
            PlxPci_EepromCrcGet( pDevice, &Data, NULL );
            Cons_printf("\n Current CRC: %08X (offset=%02X)\n", Data, Offset);
        }
    }

    Cons_printf("\n");

    // Disable throttle output
    ConsoleIoThrottleSet(FALSE);

    return TRUE;
}




/**********************************************************
 *
 * Function   :  Cmd_EepFile
 *
 * Description:
 *
 *********************************************************/
BOOLEAN Cmd_EepFile( PLX_DEVICE_OBJECT *pDevice, PLXCM_COMMAND *pCmd )
{
    U8         i;
    U8         EepPortSize;
    U16        EepSize;
    U32        PlxChip;
    BOOLEAN    bCrc;
    BOOLEAN    bLoadFile;
    BOOLEAN    bEndianSwap;
    BOOLEAN    bBypassVerify;
    PLXCM_ARG *pArg;
    PLXCM_ARG *pArgFile;


    if (pDevice == NULL)
    {
        Cons_printf("Error: No device selected\n");
        return FALSE;
    }

    // Default not to bypass data verification
    bBypassVerify = FALSE;

    // Flag file name not set
    pArgFile = NULL;

    i = 0;
    while (i < pCmd->NumArgs)
    {
        // Get next param
        pArg = CmdLine_ArgGet( pCmd, i );

        if (Plx_strcasecmp( pArg->ArgString, "-b" ) == 0)
        {
            bBypassVerify = TRUE;
        }
        else if ((pArg->ArgString[1] != '\0') && (pArg->ArgString[0] == '-'))
        {
            Cons_printf("WARNING: Ignoring invalid parameter - '%s'\n", pArg->ArgString);
        }
        else
        {
            // Parameter is a file name
            if (pArgFile != NULL)
            {
                Cons_printf("ERROR: File name already specified\n");
                return FALSE;
            }
            pArgFile = pArg;
        }

        // Jump to next parameter
        i++;
    }

    // Verify file name
    if (pArgFile == NULL)
    {
        Cons_printf("ERROR: File name not specified\n");
        return FALSE;
    }

    // Determine load or save file
    if (Plx_strcasecmp( pCmd->szCmd, "eep_load" ) == 0)
    {
        bLoadFile = TRUE;
    }
    else
    {
        bLoadFile = FALSE;
    }

    // Default to no CRC
    bCrc = FALSE;

    // Default to 32-bit EEPROM port size
    EepPortSize = sizeof(U32);

    // Generalize by chip type
    PlxChip = pDevice->Key.PlxChip;

    if (((PlxChip & 0xFF00) == 0x2300) ||
        ((PlxChip & 0xFF00) == 0x3300) ||
        ((PlxChip & 0xFF00) == 0x8600) ||
        ((PlxChip & 0xFF00) == 0x8700) ||
        ((PlxChip & 0xFF00) == 0x9700))
    {
        PlxChip = PlxChip & 0xFF00;
    }

    // Set endian swap setting
    if ((PlxChip & 0xF000) == 0x6000)
    {
        bEndianSwap = FALSE;
    }
    else
    {
        bEndianSwap = TRUE;
    }

    // Setup parameters
    switch (PlxChip)
    {
        case 0x8114:
            if (pDevice->Key.PlxRevision >= 0xBA)
            {
                EepSize = 0x3EC;
            }
            else
            {
                EepSize = 0x378;
            }
            bCrc = TRUE;
            break;

        case 0x8508:
        case 0x8512:
        case 0x8517:
        case 0x8518:
            EepSize = 0x78F * sizeof(U32);
            bCrc    = TRUE;
            break;

        case 0x8516:
        case 0x8524:
        case 0x8532:
            EepSize = 0xBE4 * sizeof(U32);
            bCrc    = TRUE;
            break;

        case 0x2300:
        case 0x3300:
        case 0x8111:
        case 0x8112:
        case 0x8505:
        case 0x8509:
        case 0x8525:
        case 0x8533:
        case 0x8547:
        case 0x8548:
        case 0x8600:
        case 0x8700:
        case 0x9700:
            if ((pDevice->Key.PlxFamily == PLX_FAMILY_DRACO_2)   ||
                (pDevice->Key.PlxFamily == PLX_FAMILY_CAPELLA_1) ||
                (pDevice->Key.PlxFamily == PLX_FAMILY_CAPELLA_2))
            {
                bCrc = TRUE;
            }

            // Process some 8000 devices differently
            if (bLoadFile)
            {
                Plx8000_EepromFileLoad(
                    pDevice,
                    pArgFile->ArgString,     // File name
                    bCrc,
                    bBypassVerify
                    );
            }
            else
            {
                Plx8000_EepromFileSave(
                    pDevice,
                    pArgFile->ArgString,    // File name
                    0,                      // 0 = Entire EEPROM
                    bCrc
                    );
            }
            return TRUE;

        case 0x9050:
        case 0x9056:
        case 0x9656:
        case 0x8311:
            EepSize     = 0x64;
            EepPortSize = sizeof(U16);
            break;

        case 0x9030:
            EepSize     = 0x88;
            EepPortSize = sizeof(U16);
            break;

        case 0x9080:
        case 0x9054:
            EepSize     = 0x58;
            EepPortSize = sizeof(U16);
            break;

        case 0x6150:
        case 0x6154:
        case 0x6350:
            EepSize     = 0x24;
            EepPortSize = sizeof(U16);
            break;

        case 0x6152:
            EepSize     = 0x18;
            EepPortSize = sizeof(U16);
            break;

        case 0x6156:
            EepSize     = 0x14;
            EepPortSize = sizeof(U16);
            break;

        case 0x6254:
        case 0x6520:
        case 0x6540:
            EepSize     = 0x40;
            EepPortSize = sizeof(U16);
            break;

        case 0:
        default:
            Cons_printf("ERROR: Only PLX devices are supported\n");
            return FALSE;
    }

    if (bLoadFile)
    {
        // Load file into EEPROM
        Plx_EepromFileLoad(
            pDevice,
            pArgFile->ArgString,
            EepSize,
            EepPortSize,
            bCrc,
            bBypassVerify,
            bEndianSwap
            );
    }
    else
    {
        // Make sure size aligned on 32-bit boundary
        EepSize = (U16)((EepSize + 3) & ~(U32)0x3);

        // Save EEPROM to file
        Plx_EepromFileSave(
            pDevice,
            pArgFile->ArgString,
            EepSize,
            EepPortSize,
            bCrc,
            bEndianSwap
            );
    }

    return TRUE;
}
