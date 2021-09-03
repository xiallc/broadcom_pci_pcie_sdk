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

#if defined(_WIN32)
    #define _CRT_NONSTDC_NO_WARNINGS        // Prevents POSIX function warnings
#endif

#include <stdlib.h>
#include <ctype.h>
#include <string.h>
#include "CmdLine.h"
#include "Monitor.h"
#include "MonCmds.h"
#include "PciRegs.h"
#include "PlxApi.h"
#include "RegDefs.h"




/**********************************************
 *                 Globals
 **********************************************/
static PLXCM_COMMAND     *Gbl_pPostedCmd;
static DEVICE_NODE       *Gbl_pNodeCurrent;
static PLX_DEVICE_OBJECT  Gbl_DeviceObj;
static FILE              *Gbl_pScriptFile = NULL;

U64 Gbl_LastRetVal = 0;     // Last return value from a command

// Setup the function table
static FN_TABLE Gbl_FnTable[] =
{
    { CMD_CLEAR     , FALSE, "cls/clear"               , Cmd_ConsClear   },
    { CMD_HELP      , FALSE, "help/?"                  , Cmd_Help        },
    { CMD_EXIT      , FALSE, "exit/quit/q"             , NULL            },
    { CMD_VERSION   , FALSE, "ver"                     , Cmd_Version     },
    { CMD_SLEEP     ,  TRUE, "sleep"                   , Cmd_Sleep       },
    { CMD_SCREEN    ,  TRUE, "screen"                  , Cmd_Screen      },
    { CMD_THROTTLE  ,  TRUE, "throttle"                , Cmd_Throttle    },
    { CMD_HISTORY   , FALSE, "history/h/!"             , Cmd_History     },
    { CMD_RESET     , FALSE, "reset"                   , Cmd_Reset       },
    { CMD_SCAN      , FALSE, "scan"                    , Cmd_Scan        },
    { CMD_SET_CHIP  , FALSE, "setchip/set_chip"        , Cmd_SetChip     },
    { CMD_DEV       , FALSE, "dev"                     , Cmd_Dev         },
    { CMD_I2C       , FALSE, "i2c"                     , Cmd_I2cConnect  },
    { CMD_MDIO      , FALSE, "mdio"                    , Cmd_MdioConnect },
    { CMD_SDB       , FALSE, "sdb"                     , Cmd_SdbConnect  },
    { CMD_PCI_CAP   , FALSE, "pcicap/pci_cap"          , Cmd_PciCapList  },
    { CMD_PORT_PROP , FALSE, "portprop/portinfo"       , Cmd_PortProp    },
    { CMD_MH_PROP   , FALSE, "mh_prop"                 , Cmd_MH_Prop     },
    { CMD_VARS      , FALSE, "var/vars"                , Cmd_VarDisplay  },
    { CMD_VAR_SET   ,  TRUE, "set"                     , Cmd_VarSet      },
    { CMD_BUFFER    , FALSE, "buffer/buff"             , Cmd_ShowBuffer  },
    { CMD_MEM_READ  ,  TRUE, "db/dw/dl/dq"             , Cmd_MemRead     },
    { CMD_MEM_WRITE ,  TRUE, "eb/ew/el/eq"             , Cmd_MemWrite    },
    { CMD_IO_READ   ,  TRUE, "ib/iw/il"                , Cmd_IoRead      },
    { CMD_IO_WRITE  ,  TRUE, "ob/ow/ol"                , Cmd_IoWrite     },
    { CMD_REG_PCI   ,  TRUE, "pcr/pci"                 , Cmd_RegPci      },
    { CMD_REG_PLX   ,  TRUE, "reg/mmr/lcr/rtr/dma/mqr" , Cmd_RegPlx      },
    { CMD_REG_DUMP  ,  TRUE, "dp/dr"                   , Cmd_RegDump     },
    { CMD_EEP       ,  TRUE, "eep"                     , Cmd_Eep         },
    { CMD_EEP_FILE  , FALSE, "eep_load/eep_save"       , Cmd_EepFile     },
    { CMD_NVME_PROP , FALSE, "nvmeprop/nvme"           , Cmd_NvmeProp    },
    { CMD_FINAL     , FALSE, "", NULL }    // Must be last entry
};


// Comment out for now until help system built
#if 0
// Setup the help table
static HELP_TABLE Gbl_HelpTable[] =
{
    { CMD_CLEAR     , "clear"               , "Clears the screen" },
    { CMD_HELP      , "Help [command]"      , "Displays global help or for the specified command" },
    { CMD_EXIT      , "exit"                , "Exits the application" },
    { CMD_VERSION   , "ver"                 , "Displays detailed version information" },
    { CMD_SLEEP     , "sleep <ms>"          , "Sleeps for the specified number of milliseconds" },
    { CMD_SCREEN    , "screen [NumLines]"   , "Display current screen size or change to specified NumLines" },
    { CMD_HISTORY   , "history/h/!"         , "" },
    { CMD_RESET     , "reset"               , "" },
    { CMD_SCAN      , "scan"                , "" },
    { CMD_SET_CHIP  , "set_chip"            , "" },
    { CMD_DEV       , "dev"                 , "" },
    { CMD_I2C       , "i2c"                 , "" },
    { CMD_PCI_CAP   , "pci_cap"             , "" },
    { CMD_PORT_PROP , "portinfo"            , "" },
    { CMD_MH_PROP   , "mh_prop"             , "" },
    { CMD_VARS      , "var/vars"            , "" },
    { CMD_VAR_SET   , "set"                 , "" },
    { CMD_BUFFER    , "buffer/buff"         , "" },
    { CMD_MEM_READ  , "db/dw/dl/dq"         , "" },
    { CMD_MEM_WRITE , "eb/ew/el/eq"         , "" },
    { CMD_IO_READ   , "ib/iw/il"            , "" },
    { CMD_IO_WRITE  , "ob/ow/ol"            , "" },
    { CMD_REG_PCI   , "pcr/pci"             , "" },
    { CMD_REG_PLX   , "reg/lcr/rtr/dma/mqr" , "" },
    { CMD_EEP       , "eep"                 , "" },
    { CMD_EEP_FILE  , "eep_load/eep_save"   , "" },
    { CMD_FINAL     , "", "" }    // Must be last entry
};
#endif




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

    // Process command-line
    ExitCode = ProcessMonitorParams( argc, argv );
    if (ExitCode != 0)
    {
        goto __Exit_App;
    }

    // Start the monitor
    Monitor();

    // Free device list
    DeviceListFree();

    // Clear all commands
    CmdLine_CmdDeleteAll();

    // Clear any variables
    CmdLine_VarDeleteAll();

__Exit_App:

    // Restore console
    ConsoleEnd();

    exit(ExitCode);
}




/**************************************************************************
 *
 * Function: ProcessMonitorParams
 *
 * Abstract:
 *
 *************************************************************************/
S8
ProcessMonitorParams(
    int   argc,
    char *argv[]
    )
{
    U8    bGetFileName;
    short i;


    // Set default options
    Gbl_pScriptFile = NULL;

    bGetFileName = FALSE;

    for (i=1; i<argc; i++)
    {
        if (bGetFileName)
        {
            if (argv[i][0] == '-')
            {
                Cons_printf("ERROR: File name not specified\n");
                return -1;
            }

            // Attempt to open the file
            Gbl_pScriptFile = fopen( argv[i], "rt" );
            if (Gbl_pScriptFile == NULL)
            {
                Cons_printf("ERROR: Unable to open script file\n");
                return -1;
            }

            // Flag parameter retrieved
            bGetFileName = FALSE;
        }
        else if (Plx_strcasecmp(argv[i], "-s") == 0)
        {
            // Set flag to get file name
            bGetFileName = TRUE;
        }
        else
        {
            Cons_printf("ERROR: Invalid argument \'%s\'\n", argv[i]);
            return -1;
        }

        // Make sure next parameter exists
        if ((i + 1) == argc)
        {
            if (bGetFileName)
            {
                Cons_printf("ERROR: Script file name not specified\n");
                return -1;
            }
        }
    }

    return 0;
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
    size_t             count;
    int                PrevInp;
    int                NextInp;
    char               buffer[MAX_CMDLN_LEN];
    BOOLEAN            bFlag;
    BOOLEAN            bInsertMode;
    PLXCM_COMMAND     *pCmd;
    PLX_DEVICE_OBJECT *pDevice;


    Cons_printf("Searching for devices...");
    Cons_fflush( stdout );

    // Make sure no device is selected
    Gbl_pNodeCurrent = NULL;

    // Build PCI device list
    DeviceListCreate( PLX_API_MODE_PCI, NULL );

    // Select first PLX device
    Gbl_pNodeCurrent =
        DeviceNodeGetByNum(
            0,
            TRUE        // PLX only device
            );

    if (Gbl_pNodeCurrent != NULL)
    {
        // Select the device
        PlxPci_DeviceOpen( &Gbl_pNodeCurrent->Key, &Gbl_DeviceObj );

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
        Cons_printf("\n* Use i2c/mdio/sdb to connect over I2C/MDIO/SDB or 'q' to exit *\n\n");
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
    NextInp        = 0;
    PrevInp        = 0;
    bInsertMode    = 1;
    Gbl_pPostedCmd = NULL;

    memset( buffer, '\0', MAX_CMDLN_LEN );

    Cons_printf(MONITOR_PROMPT);

    do
    {
        // Check for posted command
        if (Gbl_pPostedCmd == NULL)
        {
            // Store previous key
            PrevInp = NextInp;

            // Get next key from script
            if (Gbl_pScriptFile != NULL)
            {
                NextInp = fgetc( Gbl_pScriptFile );

                // If reached end-of-file, close scipt file
                if (feof( Gbl_pScriptFile ))
                {
                    fclose( Gbl_pScriptFile );
                    Gbl_pScriptFile = NULL;
                }
            }

            // Wait for keypress
            if (Gbl_pScriptFile == NULL)
            {
                NextInp = Cons_getch();
            }
        }
        else
        {
            // Copy command into buffer
            strcpy( buffer, Gbl_pPostedCmd->szCmdLine );

            // Reset posted command
            Gbl_pPostedCmd = NULL;

            i = (U16)strlen( buffer );

            Cons_printf( "%s", buffer );

            // Flag end of line
            NextInp = '\n';
        }

        switch (NextInp)
        {
            case CONS_KEY_NEWLINE:
            case CONS_KEY_CARRIAGE_RET:
                Cons_printf("\n");

                if (buffer[0] != '\0')
                {
                    // Set device object parameter to pass
                    if (Gbl_pNodeCurrent == NULL)
                    {
                        pDevice = NULL;
                    }
                    else
                    {
                        pDevice = &Gbl_DeviceObj;
                    }

                    // Process the command
                    pCmd = ProcessCommand( pDevice, buffer );

                    // Update 'RetVal' system variable to latest value
                    sprintf( buffer, "%08lX", (unsigned long)Gbl_LastRetVal );
                    CmdLine_VarAdd( "RetVal", buffer, TRUE );

                    // Check for exit command
                    if ((pCmd != NULL) &&
                        (pCmd->pCmdRoutine == NULL) &&
                        (pCmd->bErrorParse == FALSE))
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
                    memset( buffer, '\0', MAX_CMDLN_LEN );
                    i = 0;
                }

                Cons_printf(MONITOR_PROMPT);
                break;

            case CONS_KEY_BACKSPACE:
            case CONS_KEY_EXT_BACKSPACE:
                if (i > 0)
                {
                    // Delete left character & shift text from right
                    i--;
                    count = 0;
                    Cons_printf("\b");
                    while (buffer[i] != '\0')
                    {
                        buffer[i] = buffer[i+1];
                        if (buffer[i+1] == '\0')
                        {
                            Cons_printf(" \b");
                        }
                        else
                        {
                            Cons_printf("%c", buffer[i]);
                            i++;
                            count++;
                        }
                    }

                    // Restore cursor to original position
                    while (count)
                    {
                        Cons_printf("\b");
                        i--;
                        count--;
                    }
                }
                break;

            case CONS_KEY_TAB:
                // Ignore TAB
                break;

            case CONS_KEY_ESCAPE:
                // If another key already pending, then extended code
                if (Cons_kbhit())
                {
                    break;
                }

                // Delay for a bit to wait for additional keys
                Plx_sleep(10);

                // Process ESC only of no other key is pending
                if (Cons_kbhit() == FALSE)
                {
                    // Make sure at end of line 1st
                    while (buffer[i] != '\0')
                    {
                        Cons_printf("%c", buffer[i]);
                        i++;
                    }

                    // Clear the command-line
                    while (i)
                    {
                        Cons_printf("\b \b");
                        i--;
                        buffer[i] = '\0';
                    }

                    // Reset command pointer
                    pCmd = NULL;

                    // Clear previous key
                    PrevInp = 0;
                }
                break;

            case CONS_KEY_EXT_CODE:
            case CONS_KEY_KEYPAD_CODE:
                // Default to not update command-line
                bFlag = FALSE;

                // Ensure Linux extended codes are preceded by ESC (27)
                if (PrevInp != CONS_KEY_ESCAPE)
                {
                    // Linux extended codes are standard keys
                    if ((NextInp == '[') || (NextInp == 'O'))
                    {
                        // Accept key as standard key
                        goto _Monitor_StandardKey;
                    }
                }

                // Process actual extended key
                NextInp = Cons_getch();
                switch (NextInp)
                {
                    case CONS_KEY_ARROW_UP:
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
                                bFlag = TRUE;
                            }
                        }
                        else
                        {
                            // Get previous command
                            if (pCmd->ListEntry.Blink != &Gbl_ListCmds)
                            {
                                pCmd =
                                    PLX_CONTAINING_RECORD(
                                        pCmd->ListEntry.Blink,
                                        PLXCM_COMMAND,
                                        ListEntry
                                        );
                                bFlag = TRUE;
                            }
                        }
                        break;

                    case CONS_KEY_ARROW_DOWN:
                        if (pCmd != NULL)
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
                            bFlag = TRUE;
                        }
                        break;

                    case CONS_KEY_ARROW_LEFT:
                        if (i)
                        {
                            Cons_printf("\b");
                            i--;
                        }
                        break;

                    case CONS_KEY_ARROW_RIGHT:
                        if (buffer[i] != '\0')
                        {
                            Cons_printf("%c", buffer[i]);
                            i++;
                        }
                        break;

                    case CONS_KEY_HOME:
                    case CONS_KEY_HOME_XTERM:
                        while (i)
                        {
                            Cons_printf("\b");
                            i--;
                        }
                        break;

                    case CONS_KEY_END:
                    case CONS_KEY_END_XTERM:
                        while (buffer[i] != '\0')
                        {
                            Cons_printf("%c", buffer[i]);
                            i++;
                        }
                        break;

                    case CONS_KEY_INSERT:
                        bInsertMode ^= 1;
                        if (bInsertMode)
                        {
                            ConsoleCursorPropertiesSet( CONS_CURSOR_INSERT );
                        }
                        else
                        {
                            ConsoleCursorPropertiesSet( CONS_CURSOR_DEFAULT );
                        }
                        break;

                    case CONS_KEY_DELETE:
                        // Delete current character & shift text from right
                        count = 0;
                        while (buffer[i] != '\0')
                        {
                            buffer[i] = buffer[i+1];
                            if (buffer[i+1] == '\0')
                            {
                                Cons_printf(" \b");
                            }
                            else
                            {
                                Cons_printf("%c", buffer[i]);
                                i++;
                                count++;
                            }
                        }

                        // Restore cursor to original position
                        while (count)
                        {
                            Cons_printf("\b");
                            i--;
                            count--;
                        }
                        break;

                    default:
                        // Unsupported key, do nothing
                        break;
                }

                // Linux also adds '~' in some cases, so ignore it
                if (Cons_kbhit())
                {
                    Cons_getch();
                }

                // Update command line if requested to
                if (bFlag)
                {
                    // Go to end of line
                    while (buffer[i] != '\0')
                    {
                        Cons_printf("%c", buffer[i]);
                        i++;
                    }

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
_Monitor_StandardKey:
                // Check for overflow
                if (i < (MAX_CMDLN_LEN - 1))
                {
                    // If insert mode, shift character right
                    if (bInsertMode)
                    {
                        count = strlen(buffer + i) + i;
                        while (count > i)
                        {
                            buffer[count] = buffer[count-1];
                            count--;
                        }
                    }
                    buffer[i] = (char)NextInp;
                    Cons_printf("%c", (char)NextInp);
                    i++;

                    // Display updated string & restore cursor position
                    if (bInsertMode)
                    {
                        count = strlen(buffer + i);
                        if (count)
                        {
                            Cons_printf("%s", (buffer + i));
                            while (count--)
                                Cons_printf("\b");
                        }
                    }
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
    pCmd = CmdLine_CmdAdd( buffer, Gbl_FnTable );

    if (pCmd == NULL)
    {
        return NULL;
    }

    if (pCmd->pCmdRoutine == NULL)
    {
        if (pCmd->bErrorParse)
        {
            Cons_printf(
                "\"%s\" is not a valid command. Type "
                "\"help\" for more information.\n",
                pCmd->szCmd
                );
        }
    }
    else
    {
        // Check for command-line parsing errors
        if (pCmd->bErrorParse)
        {
            return NULL;
        }

        // Call the command
        pCmd->pCmdRoutine( pDevice, pCmd );
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
    U16 index
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
    {
        return NULL;
    }

    // De-select current device if selected
    if (Gbl_pNodeCurrent != NULL)
    {
        // Do nothing if re-selecting same node
        if (pNode == Gbl_pNodeCurrent)
        {
            return pNode;
        }

        Gbl_pNodeCurrent->bSelected = FALSE;
        PciSpacesUnmap( &Gbl_DeviceObj );
        CommonBufferUnmap( &Gbl_DeviceObj );

        // Release the device
        PlxPci_DeviceClose( &Gbl_DeviceObj );
    }

    // Select new device
    PlxPci_DeviceOpen( &pNode->Key, &Gbl_DeviceObj );

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

    for (i = 0; i < PCI_NUM_BARS_TYPE_00; i++)
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
    {
        return;
    }

    for (i = 0; i < PCI_NUM_BARS_TYPE_00; i++)
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
    PlxPci_CommonBufferProperties( pDevice, &PciBuffer );

    if (PciBuffer.Size != 0)
    {
        // Map buffer into user space
        PlxPci_CommonBufferMap( pDevice, &pBuffer );

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
        PlxPci_CommonBufferUnmap( pDevice, &pBuffer );

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
