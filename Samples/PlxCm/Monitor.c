/******************************************************************************
 *
 * File Name:
 *
 *      Monitor.c
 *
 * Description:
 *
 *      Monitor main module
 *
 ******************************************************************************/


#include <stdlib.h>
#include <ctype.h>
#include <string.h>

#include "CmdLine.h"
#include "Monitor.h"
#include "MonCmds.h"
#include "PlxApi.h"
#include "RegDefs.h"




/**********************************************
 *                 Globals
 **********************************************/
static PLXCM_COMMAND     *Gbl_pPostedCmd;
static DEVICE_NODE       *Gbl_pNodeCurrent;
static PLX_DEVICE_OBJECT  Gbl_DeviceObj;




/**********************************************************
 *
 * Function   :  main
 *
 * Description:
 *
 *********************************************************/
int
main(
    int   argc,
    char *argv[]
    )
{
    int ExitCode;


    // Initialize the console
    ConsoleInitialize();

    // Initialize command-line
    CmdLine_Initialize();

    // Set default exit code
    ExitCode = 0;

    // Check if Windows NT flavor is running
    if (PlxPreventInWindows())
    {
        Cons_printf(
            "\n"
            "      ERROR:\n"
            "\n"
            "        This application is designed for execution in a\n"
            "        true DOS environment.  The current OS detected is\n"
            "        Windows NT/2000/XP.  This application will not\n"
            "        function properly in this environment.\n"
            "\n"
            "                   Press any key to exit..."
            );

        Cons_getch();
        Cons_printf("\n");
        ExitCode = 1;
        goto __Exit_App;
    }

    // Start the monitor
    Monitor();

__Exit_App:

    // Free device list
    DeviceListFree();

    // Clear all commands
    CmdLine_CmdDeleteAll();

    // Clear any variables
    CmdLine_VarDeleteAll();

    // Restore console
    ConsoleEnd();

    exit(ExitCode);
}




/**********************************************************
 *
 * Function   :  Monitor
 *
 * Description:
 *
 *********************************************************/
VOID
Monitor(
    VOID
    )
{
    U16                i;
    int                NextInp;
    char               buffer[MAX_CMDLN_LEN];
    BOOLEAN            bFlag;
    PLXCM_COMMAND     *pCmd;
    PLX_DEVICE_OBJECT *pDevice;


    Cons_printf("Searching for devices...");

    // Make sure no device is selected
    Gbl_pNodeCurrent = NULL;

    // Build PCI device list
    DeviceListCreate(
        PLX_API_MODE_PCI,
        NULL
        );

    // Select first PLX device
    Gbl_pNodeCurrent =
        DeviceNodeGetByNum(
            0,
            TRUE        // PLX only device
            );

    if (Gbl_pNodeCurrent == NULL)
    {
        Cons_printf("ERROR: No PCI Devices found\n\n");

#if !defined(PLX_MSWINDOWS)
        Cons_printf("              Press any key to exit...\n");
        Cons_getch();
        return;
#endif
    }

    Cons_printf("\n");

    // Select the device
    if (Gbl_pNodeCurrent != NULL)
    {
        PlxPci_DeviceOpen(
            &Gbl_pNodeCurrent->Key,
            &Gbl_DeviceObj
            );

        // Map valid PCI BAR spaces
        PciSpacesMap( &Gbl_DeviceObj );

        // Get Common buffer properties
        CommonBufferMap( &Gbl_DeviceObj );

        // Mark device as selected
        Gbl_pNodeCurrent->bSelected = TRUE;
    }

    Cons_clear();

    // Display devices
    DeviceListDisplay();

    if (Gbl_pNodeCurrent == NULL)
    {
        Cons_printf("\n* Use 'i2c' to connect over I2C or 'q' to exit *\n\n");
    }
    else
    {
        Cons_printf(
            "\n"
            "PLX Console Monitor, v%d.%d%d [%s]\n"
            "Copyright (c) PLX Technology, Inc.\n\n",
            MONITOR_VERSION_MAJOR,
            MONITOR_VERSION_MINOR,
            MONITOR_VERSION_REVISION,
            __DATE__
            );
    }

    // Initialize variables
    i              = 0;
    pCmd           = NULL;
    Gbl_pPostedCmd = NULL;

    memset( buffer, '\0', MAX_CMDLN_LEN );

    Cons_printf(MONITOR_PROMPT);

    do
    {
        // Check for posted command
        if (Gbl_pPostedCmd == NULL)
        {
            // Wait for keypress
            NextInp = Cons_getch();
        }
        else
        {
            // Copy command into buffer
            strcpy( buffer, Gbl_pPostedCmd->szCmdLine );

            // Reset posted command
            Gbl_pPostedCmd = NULL;

            i = strlen( buffer );

            Cons_printf( "%s", buffer );

            // Flag end of line
            NextInp = '\n';
        }

        switch (NextInp)
        {
            case '\n':
            case '\r':
                Cons_printf("\n");

                if (i != 0)
                {
                    // Set device object parameter to pass
                    if (Gbl_pNodeCurrent == NULL)
                        pDevice = NULL;
                    else
                        pDevice = &Gbl_DeviceObj;

                    // Process the command
                    pCmd = ProcessCommand( pDevice, buffer );

                    // Check for exit command
                    if ((pCmd != NULL) && (strcmp(pCmd->szCmd, "quit") == 0))
                    {
                        if (pDevice)
                        {
                            // Unmap PCI BAR spaces
                            PciSpacesUnmap( pDevice );

                            // Release common buffer
                            CommonBufferUnmap( pDevice );

                            // Make sure device is closed
                            PlxPci_DeviceClose( pDevice );
                        }
                        return;
                    }

                    // Reset current command
                    pCmd = NULL;

                    // Reset command line
                    memset( buffer, '\0', i );
                    i = 0;
                }

                Cons_printf(MONITOR_PROMPT);
                break;

            case '\b':                // BackSpace
            case 127:
                if (i != 0)
                {
                    i--;
                    buffer[i] = '\0';
                    Cons_printf("\b \b");
                }
                break;

            case '\t':
                // Ignore TAB
                break;

            case 27:                  // ESC

                // Delay for a bit to wait for additional keys
                Plx_sleep(100);

                // Process ESC only of no other key is pressed
                if (Cons_kbhit() == FALSE)
                {
                    // Clear the command-line
                    while (i)
                    {
                        Cons_printf("\b \b");
                        i--;
                        buffer[i] = '\0';
                    }

                    // Reset command pointer
                    pCmd = NULL;
                }
                break;

            case 0x0:               // Extended key (DOS)
            case 0xe0:              // Extended key (Windows)
            case '[':               // Extended key (Linux)

                bFlag = TRUE;

                // Get extended code
                switch (Cons_getch())
                {
                    case 'A':            // Up arrow (Linux)
                    case 'H':            // Up arrow (Windows)
                        if (pCmd == NULL)
                        {
                            // Return last command
                            if (Plx_IsListEmpty( &Gbl_ListCmds ) == FALSE)
                            {
                                pCmd =
                                    PLX_CONTAINING_RECORD(
                                        Gbl_ListCmds.Blink,
                                        PLXCM_COMMAND,
                                        ListEntry
                                        );
                            }
                        }
                        else
                        {
                            // Get previous command
                            if (pCmd->ListEntry.Blink == &Gbl_ListCmds)
                            {
                                // Reached first command, do nothing
                                bFlag = FALSE;
                            }
                            else
                            {
                                pCmd =
                                    PLX_CONTAINING_RECORD(
                                        pCmd->ListEntry.Blink,
                                        PLXCM_COMMAND,
                                        ListEntry
                                        );
                            }
                        }
                        break;

                    case 'B':           // Down arrow (Linux)
                    case 'P':           // Down arrow (Windows)
                        if (pCmd == NULL)
                        {
                            bFlag = FALSE;
                        }
                        else
                        {
                            // Get next command
                            if (pCmd->ListEntry.Flink == &Gbl_ListCmds)
                            {
                                // Reached last command, reset
                                pCmd = NULL;
                            }
                            else
                            {
                                pCmd =
                                    PLX_CONTAINING_RECORD(
                                        pCmd->ListEntry.Flink,
                                        PLXCM_COMMAND,
                                        ListEntry
                                        );
                            }
                        }
                        break;

                    case 'D':           // Left arrow (Linux)
                    case 'K':           // Left arrow (Windows)
                        // Not currently handled
                        bFlag = FALSE;
                        break;

                    case 'C':           // Right arrow (Linux)
                    case 'M':           // Right arrow (Windows)
                        // Not currently handled
                        bFlag = FALSE;
                        break;

                    default:
                        // Unsupported key, do nothing
                        bFlag = FALSE;
                        break;
                }

                if (bFlag)
                {
                    // Clear the command-line
                    while (i)
                    {
                        Cons_printf("\b \b");
                        i--;
                    }

                    // Clear buffer
                    memset( buffer, '\0', MAX_CMDLN_LEN );

                    if (pCmd != NULL)
                    {
                        // Copy command from history
                        strcpy( buffer, pCmd->szCmdLine );

                        // Display new command
                        Cons_printf( buffer );

                        // Set cursor position
                        i = (U8)strlen( buffer );
                    }
                }
                break;

            default:
                // Check for overflow
                if (i < (MAX_CMDLN_LEN - 1))
                {
                    buffer[i] = (char)NextInp;
                    Cons_printf("%c", (char)NextInp);
                    i++;
                }
                break;
        }
    }
    while (1);
}




/**********************************************************
 *
 * Function   :  ProcessCommand
 *
 * Description:
 *
 *********************************************************/
PLXCM_COMMAND*
ProcessCommand(
    PLX_DEVICE_OBJECT *pDevice,
    char              *buffer
    )
{
    PLXCM_COMMAND *pCmd;


    // Add command to list if not already
    pCmd = CmdLine_CmdAdd( buffer );

    if (pCmd == NULL)
        return NULL;

    // Check for command-line parsing errors
    if (pCmd->bErrorParse)
        return NULL;

    if (!Plx_strcasecmp(pCmd->szCmd, "cls") ||
        !Plx_strcasecmp(pCmd->szCmd, "clear"))
    {
        Cons_clear();
    }
    else if (!Plx_strcasecmp(pCmd->szCmd, "exit") ||
             !Plx_strcasecmp(pCmd->szCmd, "quit") ||
             !Plx_strcasecmp(pCmd->szCmd, "q"))
    {
        strcpy(pCmd->szCmd, "quit");
    }
    else if (!Plx_strcasecmp(pCmd->szCmd, "ver"))
    {
        Cmd_Version( pDevice, pCmd );
    }
    else if (!Plx_strcasecmp(pCmd->szCmd, "?") ||
             !Plx_strcasecmp(pCmd->szCmd, "help"))
    {
        Cmd_Help( pDevice, pCmd );
    }
    else if (!Plx_strcasecmp(pCmd->szCmd, "sleep"))
    {
        Cmd_Sleep( pDevice, pCmd );
    }
    else if (!Plx_strcasecmp(pCmd->szCmd, "screen"))
    {
        Cmd_Screen( pDevice, pCmd );
    }
    else if (!Plx_strcasecmp(pCmd->szCmd, "history") ||
             !Plx_strcasecmp(pCmd->szCmd, "h") ||
             !Plx_strcasecmp(pCmd->szCmd, "!"))
    {
        Cmd_History( pDevice, pCmd );
    }
    else if (!Plx_strcasecmp(pCmd->szCmd, "boot"))
    {
        Cmd_Boot( pDevice, pCmd );
    }
    else if (!Plx_strcasecmp(pCmd->szCmd, "reset"))
    {
        Cmd_Reset( pDevice, pCmd );
    }
    else if (!Plx_strcasecmp(pCmd->szCmd, "scan"))
    {
        Cmd_Scan( pDevice, pCmd );
    }
    else if (!Plx_strcasecmp(pCmd->szCmd, "set_chip"))
    {
        Cmd_SetChip( pDevice, pCmd );
    }
    else if (!Plx_strcasecmp(pCmd->szCmd, "dev"))
    {
        Cmd_Dev( pDevice, pCmd );
    }
    else if (!Plx_strcasecmp(pCmd->szCmd, "i2c"))
    {
        Cmd_I2cConnect( pDevice, pCmd );
    }
    else if (!Plx_strcasecmp(pCmd->szCmd, "pci_cap"))
    {
        Cmd_PciCapList( pDevice, pCmd );
    }
    else if (!Plx_strcasecmp(pCmd->szCmd, "portinfo"))
    {
        Cmd_PortProp( pDevice, pCmd );
    }
    else if (!Plx_strcasecmp(pCmd->szCmd, "mh_prop"))
    {
        Cmd_MH_Prop( pDevice, pCmd );
    }
    else if (!Plx_strcasecmp(pCmd->szCmd, "var") ||
             !Plx_strcasecmp(pCmd->szCmd, "vars"))
    {
        Cmd_VarDisplay( pDevice, pCmd );
    }
    else if (!Plx_strcasecmp(pCmd->szCmd, "set"))
    {
        Cmd_VarSet( pDevice, pCmd );
    }
    else if (!Plx_strcasecmp(pCmd->szCmd, "buff") ||
             !Plx_strcasecmp(pCmd->szCmd, "buffer"))
    {
        Cmd_ShowBuffer( pDevice, pCmd );
    }
    else if (!Plx_strcasecmp(pCmd->szCmd, "db") ||
             !Plx_strcasecmp(pCmd->szCmd, "dw") ||
             !Plx_strcasecmp(pCmd->szCmd, "dl") ||
             !Plx_strcasecmp(pCmd->szCmd, "dq"))
    {
        Cmd_MemRead( pDevice, pCmd );
    }
    else if (!Plx_strcasecmp(pCmd->szCmd, "eb") ||
             !Plx_strcasecmp(pCmd->szCmd, "ew") ||
             !Plx_strcasecmp(pCmd->szCmd, "el") ||
             !Plx_strcasecmp(pCmd->szCmd, "eq"))
    {
        Cmd_MemWrite( pDevice, pCmd );
    }
    else if (!Plx_strcasecmp(pCmd->szCmd, "ib") ||
             !Plx_strcasecmp(pCmd->szCmd, "iw") ||
             !Plx_strcasecmp(pCmd->szCmd, "il"))
    {
        Cmd_IoRead( pDevice, pCmd );
    }
    else if (!Plx_strcasecmp(pCmd->szCmd, "ob") ||
             !Plx_strcasecmp(pCmd->szCmd, "ow") ||
             !Plx_strcasecmp(pCmd->szCmd, "ol"))
    {
        Cmd_IoWrite( pDevice, pCmd );
    }
    else if (!Plx_strcasecmp(pCmd->szCmd, "pcr"))
    {
        Cmd_RegPci( pDevice, pCmd );
    }
    else if (!Plx_strcasecmp(pCmd->szCmd, "reg") ||
             !Plx_strcasecmp(pCmd->szCmd, "lcr") ||
             !Plx_strcasecmp(pCmd->szCmd, "rtr") ||
             !Plx_strcasecmp(pCmd->szCmd, "dma") ||
             !Plx_strcasecmp(pCmd->szCmd, "mqr"))
    {
        Cmd_RegPlx( pDevice, pCmd );
    }
    else if (!Plx_strcasecmp(pCmd->szCmd, "eep"))
    {
        Cmd_Eep( pDevice, pCmd );
    }
    else if (!Plx_strcasecmp(pCmd->szCmd, "eep_load") ||
             !Plx_strcasecmp(pCmd->szCmd, "eep_save"))
    {
        Cmd_EepFile( pDevice, pCmd );
    }
    else
    {
        Cons_printf(
            "\"%s\" is not a valid command. Type "
            "\"help\" for more information.\n",
            pCmd->szCmd
            );
    }

    return pCmd;
}




/**********************************************************
 *
 * Function   :  DeviceNodeSelectByIndex
 *
 * Description:
 *
 *********************************************************/
DEVICE_NODE*
DeviceSelectByIndex(
    U8 index
    )
{
    DEVICE_NODE *pNode;


    // Get 
    pNode =
        DeviceNodeGetByNum(
            index,
            FALSE           // Non-PLX device is ok
            );

    if (pNode == NULL)
        return NULL;

    // De-select current device is selected
    if (Gbl_pNodeCurrent != NULL)
    {
        Gbl_pNodeCurrent->bSelected = FALSE;
        PciSpacesUnmap( &Gbl_DeviceObj );
        CommonBufferUnmap( &Gbl_DeviceObj );
        PlxPci_DeviceClose( &Gbl_DeviceObj );
    }

    // Select new device
    PlxPci_DeviceOpen(
        &pNode->Key,
        &Gbl_DeviceObj
        );

    // Update current node
    Gbl_pNodeCurrent = pNode;

    // Map valid PCI BAR spaces
    PciSpacesMap( &Gbl_DeviceObj );

    // Get Common buffer properties
    CommonBufferMap( &Gbl_DeviceObj );

    pNode->bSelected = TRUE;

    return pNode;
}




/**********************************************************
 *
 * Function   :  PciSpacesMap
 *
 * Description:
 *
 *********************************************************/
VOID
PciSpacesMap(
    PLX_DEVICE_OBJECT *pDevice
    )
{
    U8   i;
    char strVar[5];
    char strVal[25];


    // Skip if no device is selected
    if (Gbl_pNodeCurrent == NULL)
        return;

    for (i=0; i<6; i++)
    {
        // Map PCI BAR into user space
        PlxPci_PciBarMap(
            pDevice,
            i,
            (VOID**)&(Gbl_pNodeCurrent->Va_PciBar[i])
            );

        // Add to variable table
        if (Gbl_pNodeCurrent->Va_PciBar[i] != 0)
        {
            // Prepare variable & add to table
            sprintf( strVar, "V%d", i);
            sprintf( strVal, "%08lX", (PLX_UINT_PTR)Gbl_pNodeCurrent->Va_PciBar[i] );

            CmdLine_VarAdd(
                strVar,
                strVal,
                TRUE        // System variable
                );
        }
    }
}




/**********************************************************
 *
 * Function   :  PciSpacesUnmap
 *
 * Description:
 *
 *********************************************************/
VOID
PciSpacesUnmap(
    PLX_DEVICE_OBJECT *pDevice
    )
{
    U8   i;
    char strVar[5];


    // Skip if no device is selected
    if (Gbl_pNodeCurrent == NULL)
        return;

    for (i=0; i<6; i++)
    {
        if (Gbl_pNodeCurrent->Va_PciBar[i] != 0)
        {
            // Unmap PCI BAR from user space
            PlxPci_PciBarUnmap(
                pDevice,
                (VOID**)&(Gbl_pNodeCurrent->Va_PciBar[i])
                );

            // Prepare variable & remove from table
            sprintf( strVar, "V%d", i);

            CmdLine_VarDelete(
                strVar,
                TRUE        // System variable
                );
        }
    }
}




/**********************************************************
 *
 * Function   :  CommonBufferMap
 *
 * Description:  Get common buffer information & map to virtual space
 *
 *********************************************************/
VOID
CommonBufferMap(
    PLX_DEVICE_OBJECT *pDevice
    )
{
    char              strVal[25];
    VOID             *pBuffer;
    PLX_PHYSICAL_MEM  PciBuffer;


    // Get PCI buffer properties
    PlxPci_CommonBufferProperties(
        pDevice,
        &PciBuffer
        );

    if (PciBuffer.Size != 0)
    {
        // Map buffer into user space
        PlxPci_CommonBufferMap(
            pDevice,
            &pBuffer
            );

        // Add to variable table
        if (pBuffer != 0)
        {
            // Prepare variable
            sprintf( strVal, "%08lX", (PLX_UINT_PTR)pBuffer );
            CmdLine_VarAdd(
                "hbuf",
                strVal,
                TRUE        // System variable
                );
        }
    }
}




/**********************************************************
 *
 * Function   :  CommonBufferUnmap
 *
 * Description:  Unmap common buffer & clear information
 *
 *********************************************************/
VOID
CommonBufferUnmap(
    PLX_DEVICE_OBJECT *pDevice
    )
{
    VOID      *pBuffer;
    PLXCM_VAR *pVar;


    // Get buffer address from variable table
    pVar = CmdLine_VarLookup( "hbuf" );

    if (pVar != NULL)
    {
        pBuffer = (VOID*)htol( pVar->strValue );

        // Unmap buffer
        PlxPci_CommonBufferUnmap(
            pDevice,
            pBuffer
            );

        // Remove variable from table
        CmdLine_VarDelete(
            "hbuf",
            TRUE        // System variable
            );
    }
}




/**********************************************************
 *
 * Function   :  Mon_PostCommand
 *
 * Description:  Places a command into the buffer for processing
 *
 *********************************************************/
VOID
Mon_PostCommand(
    PLXCM_COMMAND *pCmd
    )
{
    // Store the command
    Gbl_pPostedCmd = pCmd;
}
