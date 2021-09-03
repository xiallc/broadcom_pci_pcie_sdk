/*******************************************************************************
 * Copyright 2013-2017 Avago Technologies
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
 *      PlxEep.c
 *
 * Description:
 *
 *      EEPROM file support for PLX devices
 *
 ******************************************************************************/


#include <stdlib.h>
#include <time.h>
#include "PlxApi.h"
#include "PlxEep.h"

#if defined(PLX_MSWINDOWS)
    #include "..\\Shared\\ConsFunc.h"
#endif

#if defined(PLX_LINUX) || defined(PLX_DOS)
    #include "ConsFunc.h"
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
    int               ExitCode;
    S16               rc;
    PLX_STATUS        status;
    EEP_OPTIONS       EepOptions;
    PLX_DEVICE_KEY    Key;
    PLX_DEVICE_OBJECT Device;
    PLX_EEPROM_STATUS EepStatus;


    // Initialize the console
    ConsoleInitialize();

    // Set default exit code
    ExitCode = EXIT_CODE_SUCCESS;

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
            );

        ExitCode = EXIT_CODE_NON_DOS;
        goto __Exit_App;
    }

    // Verify the command-line
    rc =
        ProcessCommandLine(
            argc,
            argv,
            &EepOptions
            );

    if (rc == -1)
    {
        goto __Exit_App;
    }
    else if (rc == EXIT_CODE_CMD_LINE_ERR)
    {
        ExitCode = EXIT_CODE_CMD_LINE_ERR;
        goto __Exit_App;
    }

    // Decide which device to select
    rc = SelectDevice( &Key, &EepOptions );

    if (rc == -1)
    {
        // User cancelled
        goto __Exit_App;
    }
    else if (rc == 0)
    {
        Cons_printf("ERROR: Unable to select device or device does not exist\n");
        ExitCode = EXIT_CODE_INVALID_DEV;
        goto __Exit_App;
    }

    // Select a device
    if (PlxPci_DeviceOpen( &Key, &Device ) != PLX_STATUS_OK)
    {
        Cons_printf("ERROR: Unable to select device\n");
        ExitCode = EXIT_CODE_DEV_OPEN_ERR;
        goto __Exit_App;
    }

    Cons_printf(
        "\n"
        "Selected: %04x %04x [%02X:%02X.%X]\n\n",
        Key.DeviceId, Key.VendorId,
        Key.bus, Key.slot, Key.function
        );

    // Verify EEPROM is present
    EepStatus = PlxPci_EepromPresent( &Device, &status );
    if ((status == PLX_STATUS_OK) && (EepStatus == PLX_EEPROM_STATUS_NONE))
    {
        if (EepOptions.bIgnoreWarnings)
        {
            Cons_printf("WARNING: PLX chip reports no EEPROM present, ignoring\n");
        }
        else
        {
            Cons_printf("ERROR: PLX chip reports no EEPROM present, unable to continue\n");
            ExitCode = EXIT_CODE_EEP_NOT_EXIST;
            goto __Exit_App;
        }
    }

    // Perform EEPROM operation
    ExitCode = EepFile( &Device, &EepOptions );

    // Release device
    PlxPci_DeviceClose( &Device );

__Exit_App:

    if (ExitCode != EXIT_CODE_SUCCESS)
    {
        Cons_printf("  -- PlxEep halted (error %02d) --\n", ExitCode);
    }
    Cons_printf("\n");

    // Restore console
    ConsoleEnd();

    exit(ExitCode);
}




/**************************************************************************
 *
 * Function: ProcessCommandLine
 *
 * Abstract:
 *
 *************************************************************************/
S8
ProcessCommandLine(
    int          argc,
    char        *argv[],
    EEP_OPTIONS *pOptions
    )
{
    short   i;
    char   *pToken;
    BOOLEAN bChipTypeValid;
    BOOLEAN bGetFileName;
    BOOLEAN bGetChipType;
    BOOLEAN bGetEepWidth;
    BOOLEAN bGetByteCount;
    BOOLEAN bGetDeviceNum;


    // Clear options
    RtlZeroMemory( pOptions, sizeof(EEP_OPTIONS) );

    // Set non-zero default options
    pOptions->bLoadFile    = BOOLEAN_UNKNOWN;
    pOptions->DeviceNumber = -1;

    bGetFileName  = FALSE;
    bGetChipType  = FALSE;
    bGetEepWidth  = FALSE;
    bGetDeviceNum = FALSE;
    bGetByteCount = FALSE;

    for (i = 1; i < argc; i++)
    {
        if (bGetFileName)
        {
            if (argv[i][0] == '-')
            {
                Cons_printf("ERROR: File name not specified\n");
                return EXIT_CODE_CMD_LINE_ERR;
            }

            // Get file name
            strcpy(pOptions->FileName, argv[i]);

            // Flag parameter retrieved
            bGetFileName = FALSE;
        }
        else if (bGetChipType)
        {
            if (argv[i][0] == '-')
            {
                Cons_printf("ERROR: PLX chip type not specified\n");
                return EXIT_CODE_CMD_LINE_ERR;
            }

            // Get pointer to chip type
            pToken = strtok(argv[i], ", ");

            // Convert parameter to chip type
            pOptions->LimitPlxChip = (U16)strtol(pToken, NULL, 16);

            // Get pointer to revision
            pToken = strtok(NULL, " ");

            // Convert to a revision if provided
            if (pToken != NULL)
            {
                pOptions->LimitPlxRevision = (U8)strtol(pToken, NULL, 16);
            }

            // Default to valid chip type
            bChipTypeValid = TRUE;

            // Verify supported chip type
            if (((pOptions->LimitPlxChip & 0xFF00) == 0x9000) ||
                ((pOptions->LimitPlxChip & 0xFF00) == 0x9600) ||
                ((pOptions->LimitPlxChip & 0xF000) == 0x6000) ||
                ((pOptions->LimitPlxChip & 0xFF00) == 0x8100) ||
                 (pOptions->LimitPlxChip == 0x8311))
            {
                switch (pOptions->LimitPlxChip)
                {
                    case 0x6150:
                    case 0x6152:
                    case 0x6154:
                    case 0x6156:
                    case 0x6254:
                    case 0x6350:
                    case 0x6520:
                    case 0x6540:
                        break;

                    case 0x9050:
                    case 0x9030:
                    case 0x9080:
                    case 0x9054:
                    case 0x9056:
                    case 0x9656:
                    case 0x8311:
                        break;

                    case 0x8111:
                    case 0x8112:
                    case 0x8114:
                        break;

                    default:
                        bChipTypeValid = FALSE;
                        break;
                }
            }
            else if (((pOptions->LimitPlxChip & 0xFF00) != 0x3300) &&
                     ((pOptions->LimitPlxChip & 0xFF00) != 0x8500) &&
                     ((pOptions->LimitPlxChip & 0xFF00) != 0x8600) &&
                     ((pOptions->LimitPlxChip & 0xFF00) != 0x8700) &&
                     ((pOptions->LimitPlxChip & 0xFF00) != 0x9700))
            {
                // Remaining devices must be 3300/8500/8600/8700/9700
                bChipTypeValid = FALSE;
            }

            if (bChipTypeValid == FALSE)
            {
                Cons_printf(
                    "ERROR: Invalid PLX chip type.  Valid types are:\n"
                    "   6150, 6152, 6154, 6156, 6254, 6350, 6520, 6540,\n"
                    "   9050, 9030, 9080, 9054, 9056, 9656, 8311, 8111, 8112, 8114,\n"
                    "   3300, 8500, 8600, 8700, or 9700 parts\n"
                    );
                return EXIT_CODE_CMD_LINE_ERR;
            }

            // Flag parameter retrieved
            bGetChipType = FALSE;
        }
        else if (bGetEepWidth)
        {
            if (argv[i][0] == '-')
            {
                Cons_printf("ERROR: EEPROM address width override not specified\n");
                return EXIT_CODE_CMD_LINE_ERR;
            }

            // Convert parameter to decimal
            pOptions->EepWidthSet = atoi(argv[i]);

            // Flag parameter retrieved
            bGetEepWidth = FALSE;
        }
        else if (bGetDeviceNum)
        {
            if (argv[i][0] == '-')
            {
                Cons_printf("ERROR: Device number not specified\n");
                return EXIT_CODE_CMD_LINE_ERR;
            }

            // Convert parameter to decimal
            pOptions->DeviceNumber = atoi(argv[i]);

            // Listing starts @ 1, so decrement since device list starts @ 0
            pOptions->DeviceNumber--;

            // Flag parameter retrieved
            bGetDeviceNum = FALSE;
        }
        else if (bGetByteCount)
        {
            if (argv[i][0] == '-')
            {
                Cons_printf("ERROR: Extra byte count not specified\n");
                return EXIT_CODE_CMD_LINE_ERR;
            }

            // Convert parameter to decimal byte count
            pOptions->ExtraBytes = (U16)atol(argv[i]);

            if (pOptions->ExtraBytes == 0)
            {
                Cons_printf("ERROR: Invalid extra byte count\n");
                return EXIT_CODE_CMD_LINE_ERR;
            }

            // Flag parameter retrieved
            bGetByteCount = FALSE;
        }
        else if ((Plx_strcasecmp(argv[i], "-?") == 0) ||
                 (Plx_strcasecmp(argv[i], "-h") == 0))
        {
            DisplayHelp();
            return -1;
        }
        else if (Plx_strcasecmp(argv[i], "-v") == 0)
        {
            pOptions->bVerbose = TRUE;
        }
        else if (Plx_strcasecmp(argv[i], "-l") == 0)
        {
            pOptions->bLoadFile = TRUE;

            // Set flag to get file name
            bGetFileName = TRUE;
        }
        else if (Plx_strcasecmp(argv[i], "-s") == 0)
        {
            pOptions->bLoadFile = FALSE;

            // Set flag to get file name
            bGetFileName = TRUE;
        }
        else if (Plx_strcasecmp(argv[i], "-p") == 0)
        {
            bGetChipType = TRUE;
        }
        else if (Plx_strcasecmp(argv[i], "-w") == 0)
        {
            bGetEepWidth = TRUE;
        }
        else if (Plx_strcasecmp(argv[i], "-d") == 0)
        {
            bGetDeviceNum = TRUE;
        }
        else if (Plx_strcasecmp(argv[i], "-n") == 0)
        {
            bGetByteCount = TRUE;
        }
        else if (Plx_strcasecmp(argv[i], "-i") == 0)
        {
            pOptions->bIgnoreWarnings = TRUE;
        }
        else
        {
            Cons_printf("ERROR: Invalid argument \'%s\'\n", argv[i]);
            return EXIT_CODE_CMD_LINE_ERR;
        }

        // Make sure next parameter exists
        if ((i + 1) == argc)
        {
            if (bGetFileName)
            {
                Cons_printf("ERROR: File name not specified\n");
                return EXIT_CODE_CMD_LINE_ERR;
            }

            if (bGetChipType)
            {
                Cons_printf("ERROR: PLX chip type not specified\n");
                return EXIT_CODE_CMD_LINE_ERR;
            }

            if (bGetEepWidth)
            {
                Cons_printf("ERROR: EEPROM address width not specified\n");
                return EXIT_CODE_CMD_LINE_ERR;
            }

            if (bGetDeviceNum)
            {
                Cons_printf("ERROR: Device number not specified\n");
                return EXIT_CODE_CMD_LINE_ERR;
            }

            if (bGetByteCount)
            {
                Cons_printf("ERROR: Extra byte count not specified\n");
                return EXIT_CODE_CMD_LINE_ERR;
            }
        }
    }

    // Make sure required parameters were provided
    if ((pOptions->bLoadFile == BOOLEAN_UNKNOWN) || (pOptions->FileName[0] == '\0'))
    {
        Cons_printf("ERROR: EEPROM operation not specified. Use 'PlxEep -h' for usage.\n");
        return EXIT_CODE_CMD_LINE_ERR;
    }

    return EXIT_CODE_SUCCESS;
}




/**************************************************************************
 *
 * Function: DisplayHelp
 *
 * Abstract:
 *
 *************************************************************************/
VOID
DisplayHelp(
    VOID
    )
{
    // Throttle the output
    ConsoleIoThrottleSet( TRUE );

    Cons_printf(
        "\n"
        "PlxEep.exe v%d.%d%d - EEPROM file utility for PLX devices.\n"
        "\n"
        " Usage: PlxEep -l|-s file [-p chip[,rev]] [-d dev] [-w width]\n"
        "                          [-i] [-n bytes] [-v]\n"
        "\n"
        " Options:\n"
        "   -l | -s       Load (-l) file to EEPROM -OR- Save (-s) EEPROM to file\n"
        "   file          Specifies the file to load or save\n"
        "   -p chip,rev   Limits device selection to a specific PLX chip type. If\n"
        "                  revision provided, it further limits to a specific revision.\n"
        "   -d dev        Specifies device number to select. Numbering starts at 1.\n"
        "   -w width      Specifies an EEPROM address width (1, 2, or 3) to override\n"
        "   -i            Ignore warnings & continue (e.g. EEPROM not detected)\n"
        "   -n bytes      Number of extra bytes (after PLX portion) to save to file\n"
        "   -v            Verbose output (for debug purposes)\n"
        "   -h or -?      This help screen\n"
        "\n"
        "  Exit codes (for DOS ERRORLEVEL):\n"
        "       %d         EEPROM operation ok\n"
        "       %d         Attempt to execute in non-DOS environment\n"
        "       %d         Command-line parameter error\n"
        "       %d         Invalid device selection\n"
        "       %d         Device open error\n"
        "       %d         EEPROM operation failed\n"
        "       %d         EEPROM chip not present\n"
        "       %d         Invalid PLX chip type supplied\n"
        "       %d         EEPROM address width override set error\n"
        "\n"
        "  Sample batch file\n"
        "  -----------------\n"
        "  PlxEep -l MyEeprom.bin -p 8532,BA -d 1\n"
        "  IF ERRORLEVEL == 1 goto _PlxEepError\n"
        "  IF ERRORLEVEL == 0 goto _PlxEepOk\n"
        "  \n"
        "  :_PlxEepError\n"
        "  Echo Program EEPROM: -- ERROR --\n"
        "  goto _Exit\n"
        "  \n"
        "  :_PlxEepOk\n"
        "  Echo Program EEPROM: Ok\n"
        "  goto _Exit\n"
        "  \n"
        "  :_Exit\n"
        "\n",
        APP_VERSION_MAJOR,
        APP_VERSION_MINOR,
        APP_VERSION_REVISION,
        EXIT_CODE_SUCCESS,
        EXIT_CODE_NON_DOS,
        EXIT_CODE_CMD_LINE_ERR,
        EXIT_CODE_INVALID_DEV,
        EXIT_CODE_DEV_OPEN_ERR,
        EXIT_CODE_EEP_FAIL,
        EXIT_CODE_EEP_NOT_EXIST,
        EXIT_CODE_INVALID_PLX,
        EXIT_CODE_EEP_WIDTH_ERR
        );

    // Cancel output throttle
    ConsoleIoThrottleSet( FALSE );
}




/*********************************************************************
 *
 * Function   : SelectDevice
 *
 * Description: Asks the user which device to select
 *
 * Returns    : Total devices found
 *              -1,  if user cancelled the selection
 *
 ********************************************************************/
S16
SelectDevice(
    PLX_DEVICE_KEY *pKey,
    EEP_OPTIONS    *pOptions
    )
{
    int               i;
    int               j;
    S16               NumDevices;
    BOOLEAN           bAddDevice;
    PLX_STATUS        status;
    PLX_PORT_PROP     PortProp;
    PLX_DEVICE_KEY    DevKey;
    PLX_DEVICE_KEY    DevKeyList[MAX_DEVICES_TO_LIST];
    PLX_DEVICE_OBJECT Device;


    i          = 0;
    NumDevices = 0;
    do
    {
        // Reset device key structure
        memset(&DevKey, PCI_FIELD_IGNORE, sizeof(PLX_DEVICE_KEY));

        // Check if device exists
        status = PlxPci_DeviceFind( &DevKey, (U16)i );

        if (status == PLX_STATUS_OK)
        {
            if (pOptions->bVerbose)
            {
                Cons_printf(
                    "App: Found device %04x %04x [%02X:%02X.%X]\n",
                    DevKey.DeviceId, DevKey.VendorId,
                    DevKey.bus, DevKey.slot, DevKey.function
                    );
            }

            // Default to add device
            bAddDevice = TRUE;

            // Verify device is not already in list
            for (j = 0; j < NumDevices; j++)
            {
                // Do not add device if already in list
                if ((DevKey.bus      == DevKeyList[j].bus) &&
                    (DevKey.slot     == DevKeyList[j].slot) &&
                    (DevKey.function == DevKeyList[j].function))
                {
                    bAddDevice = FALSE;
                    if (pOptions->bVerbose)
                    {
                        Cons_printf("App: Device already in list, skip\n");
                    }
                }
            }

            if (bAddDevice)
            {
                // Get device properties
                PlxPci_DeviceOpen( &DevKey, &Device );
                PlxPci_GetPortProperties( &Device, &PortProp );
                PlxPci_DeviceClose( &Device );
            }

            // Verify supported PLX chip
            if (bAddDevice && (DevKey.PlxChip == 0))
            {
                bAddDevice = FALSE;
                if (pOptions->bVerbose)
                {
                    Cons_printf("App: Device is not PLX chip\n");
                }
            }

            // Verify supported device types
            if ( bAddDevice &&
                 (DevKey.PlxPortType != PLX_SPEC_PORT_NT_VIRTUAL) &&
                 (DevKey.PlxPortType != PLX_SPEC_PORT_NT_LINK) &&
                 (DevKey.PlxPortType != PLX_SPEC_PORT_UPSTREAM) &&
                 (DevKey.PlxPortType != PLX_SPEC_PORT_P2P_BRIDGE) &&
                 (DevKey.PlxPortType != PLX_SPEC_PORT_LEGACY_EP) &&
                 (DevKey.PlxPortType != PLX_SPEC_PORT_GEP) )
            {
                bAddDevice = FALSE;
                if (pOptions->bVerbose)
                {
                    Cons_printf("App: Device type does not support EEPROM access\n");
                }
            }

            // Check if limited to a specific chip type
            if (bAddDevice && (pOptions->LimitPlxChip != 0))
            {
                if (pOptions->LimitPlxChip != DevKey.PlxChip)
                {
                    bAddDevice = FALSE;
                    if (pOptions->bVerbose)
                    {
                        Cons_printf("App: Device is not %04X, ignore\n", pOptions->LimitPlxChip);
                    }
                }
                else if (pOptions->LimitPlxRevision != 0)
                {
                    if (pOptions->LimitPlxRevision != DevKey.PlxRevision)
                    {
                        bAddDevice = FALSE;
                        if (pOptions->bVerbose)
                        {
                            Cons_printf("App: Device not rev %02X, ignore\n", pOptions->LimitPlxRevision);
                        }
                    }
                }
            }

            if (bAddDevice)
            {
                // Copy device key info
                DevKeyList[NumDevices] = DevKey;

                // Increment to next device
                NumDevices++;

                if (pOptions->DeviceNumber == -1)
                {
                    if (NumDevices == 1)
                    {
                        Cons_printf("\n");
                    }

                    Cons_printf(
                        "\t\t    %2d. %04x %04x  [%04X %02X @ %02X:%02X.%X]\n",
                        NumDevices, DevKey.DeviceId, DevKey.VendorId,
                        DevKey.PlxChip, DevKey.PlxRevision,
                        DevKey.bus, DevKey.slot, DevKey.function
                        );
                }
                else
                {
                    if (pOptions->DeviceNumber == (NumDevices - 1))
                    {
                        // Copy key information
                        *pKey = DevKey;
                        return NumDevices;
                    }
                }
            }

            i++;
        }
    }
    while ((status == PLX_STATUS_OK) && (NumDevices < MAX_DEVICES_TO_LIST));

    // Check devices exist and one was selected
    if ((NumDevices == 0) || (pOptions->DeviceNumber != -1))
    {
        return 0;
    }

    Cons_printf("\t\t     0. Cancel\n\n");

    do
    {
        Cons_printf("\t   Device selection --> ");
        Cons_scanf("%d", &i);
    }
    while (i > NumDevices);

    if (i == 0)
    {
        return -1;
    }

    // Copy selected device information
    *pKey = DevKeyList[i - 1];

    return (S16)NumDevices;
}




/**********************************************************
 *
 * Function   :  EepFile
 *
 * Description:
 *
 *********************************************************/
S8
EepFile(
    PLX_DEVICE_OBJECT *pDevice,
    EEP_OPTIONS       *pOptions
    )
{
    U8         EepPortSize;
    U16        PlxChip;
    U16        EepSize;
    BOOLEAN    rc;
    BOOLEAN    bCrc;
    BOOLEAN    bEndianSwap;
    PLX_STATUS status;


    // Default to no CRC
    bCrc = FALSE;

    // Default to 32-bit EEPROM port size
    EepPortSize = sizeof(U32);

    // Attempt to set EEPROM address width if requested
    if (pOptions->EepWidthSet != 0)
    {
        Cons_printf("Set address width..... ");
        Cons_fflush( stdout );

        status =
            PlxPci_EepromSetAddressWidth(
                pDevice,
                pOptions->EepWidthSet
                );

        if (status != PLX_STATUS_OK)
        {
            Cons_printf(
                "ERROR: Unable to set to %dB addressing\n",
                pOptions->EepWidthSet
                );

            if (pOptions->bIgnoreWarnings == FALSE)
            {
                return EXIT_CODE_EEP_WIDTH_ERR;
            }
        }
        else
        {
            Cons_printf("Ok (%d-byte)\n", pOptions->EepWidthSet);
        }
    }

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

    // Process some 8000 devices differently
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

        case 0x8111:
        case 0x8112:
        case 0x8505:
        case 0x8509:
        case 0x8525:
        case 0x8533:
        case 0x8547:
        case 0x8548:
        case 0x3300:
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
            if (pOptions->bLoadFile)
            {
                return Plx8000_EepromFileLoad(
                    pDevice,
                    pOptions,
                    bCrc
                    );
            }
            else
            {
                return Plx8000_EepromFileSave(
                    pDevice,
                    pOptions,
                    0,          // 0 = Entire EEPROM
                    bCrc
                    );
            }

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
            return EXIT_CODE_EEP_FAIL;
    }

    if (pOptions->bLoadFile)
    {
        // Load file into EEPROM
        rc =
            Plx_EepromFileLoad(
                pDevice,
                pOptions,
                EepSize,
                EepPortSize,
                bCrc,
                bEndianSwap
                );
    }
    else
    {
        // Adjust for extra bytes
        EepSize += pOptions->ExtraBytes;

        // Make sure size aligned on 32-bit boundary
        EepSize = (U16)((EepSize + 3) & ~(U32)0x3);

        // Save EEPROM to file
        rc =
            Plx_EepromFileSave(
                pDevice,
                pOptions,
                EepSize,
                EepPortSize,
                bCrc,
                bEndianSwap
                );
    }

    return rc;
}




/******************************************************************************
 *
 * Function   :  Plx_EepromFileLoad
 *
 * Description:  Loads the EEPROM contents from a file
 *
 ******************************************************************************/
S8
Plx_EepromFileLoad(
    PLX_DEVICE_OBJECT *pDevice,
    EEP_OPTIONS       *pOptions,
    U32                EepSize,
    U8                 EepPortSize,
    BOOLEAN            bCrc,
    BOOLEAN            bEndianSwap
    )
{
    S8       rc;
    U16      offset;
    U32      Crc;
    U32      value;
    U32      Verify_Value;
    U32      FileSize;
    U32     *pBuffer;
    FILE    *pFile;
    clock_t  start;
    clock_t  end;


    // Note start time
    start = clock();

    Cons_printf("Load EEPROM file....... ");
    Cons_fflush( stdout );

    pBuffer = NULL;

    // Open the file to read
    pFile = fopen( pOptions->FileName, "rb" );
    if (pFile == NULL)
    {
        Cons_printf(
            "ERROR: Unable to load \"%s\"\n",
            pOptions->FileName
            );
        return EXIT_CODE_EEP_FAIL;
    }

    // Move to end-of-file
    fseek( pFile, 0, SEEK_END );

    // Determine file size
    FileSize = ftell( pFile );

    // Move back to start of file
    fseek( pFile, 0, SEEK_SET );

    // Allocate a buffer for the data
    pBuffer = malloc(FileSize);
    if (pBuffer == NULL)
    {
        fclose( pFile );
        return EXIT_CODE_EEP_FAIL;
    }

    // Read data from file
    fread(
        pBuffer,        // Buffer for data
        sizeof(U8),     // Item size
        FileSize,       // Buffer size
        pFile           // File pointer
        );

    // Close the file
    fclose( pFile );

    Cons_printf("Ok (%dB)\n", FileSize);

    if (FileSize < EepSize)
    {
        Cons_printf("WARNING: File is less than minimum size (%dB)\n", EepSize);
    }

    // Default to successful operation
    rc = EXIT_CODE_SUCCESS;

    Cons_printf("Program EEPROM......... ");

    // Write buffer into EEPROM
    for (offset = 0; offset < FileSize; offset += sizeof(U32))
    {
        // Periodically update status
        if ((offset & 0xF) == 0)
        {
            Cons_printf(
                "%02d%%\b\b\b",
                (U16)((offset * 100) / FileSize)
                );
            Cons_fflush( stdout );
        }

        // Get next value to write
        value = pBuffer[offset / sizeof(U32)];

        // Endian swap if requested to
        if (bEndianSwap)
        {
            // Endian swap based on port size
            if (EepPortSize == sizeof(U16))
            {
                value = EndianSwap16(value);
            }
            else
            {
                value = EndianSwap32(value);
            }
        }

        // Store next value
        PlxPci_EepromWriteByOffset( pDevice, offset, value );

        // Re-read to verify
        PlxPci_EepromReadByOffset( pDevice, offset, &Verify_Value );

        if (Verify_Value != value)
        {
            Cons_printf(
                "ERROR: offset:%02X  wrote:%08lX  read:%08lX\n",
                offset, value, Verify_Value
                );
            rc = EXIT_CODE_EEP_FAIL;
            goto _Exit_File_Load;
        }
    }

    Cons_printf("Ok \n");

    // Update CRC if requested
    if (bCrc)
    {
        Cons_printf("Update CRC............. ");
        Cons_fflush( stdout );
        PlxPci_EepromCrcUpdate( pDevice, &Crc, TRUE );
        Cons_printf("Ok (CRC=%08X)\n", (int)Crc);
    }

_Exit_File_Load:
    // Release the buffer
    if (pBuffer != NULL)
    {
        free(pBuffer);
    }

    // Note completion time
    end = clock();

    Cons_printf(
        " -- Complete (%.2f sec) --\n",
        ((double)end - start) / CLOCKS_PER_SEC
        );

    return rc;
}




/******************************************************************************
 *
 * Function   :  Plx_EepromFileSave
 *
 * Description:  Saves the contents of the EEPROM to a file
 *
 ******************************************************************************/
S8
Plx_EepromFileSave(
    PLX_DEVICE_OBJECT *pDevice,
    EEP_OPTIONS       *pOptions,
    U32                EepSize,
    U8                 EepPortSize,
    BOOLEAN            bCrc,
    BOOLEAN            bEndianSwap
    )
{
    U16      offset;
    U32      value;
    FILE    *pFile;
    clock_t  start;
    clock_t  end;


    // Note start time
    start = clock();

    // Open the file to write
    pFile = fopen( pOptions->FileName, "wb" );
    if (pFile == NULL)
    {
        return EXIT_CODE_EEP_FAIL;
    }

    // Adjust EEPROM size for devices with CRC
    if (bCrc)
    {
        EepSize += sizeof(U32);
    }

    Cons_printf(
        "Write EEPROM data to file \"%s\".....",
        pOptions->FileName
        );

    // Read EEPROM and write to file
    for (offset = 0; offset < EepSize; offset += sizeof(U32))
    {
        // Periodically update status
        if ((offset & 0xF) == 0)
        {
            Cons_printf(
                "%02d%%\b\b\b",
                (U16)((offset * 100) / EepSize)
                );
            Cons_fflush( stdout );
        }

        // Get next data to write
        PlxPci_EepromReadByOffset( pDevice, offset, &value );

        // Endian swap if requested to
        if (bEndianSwap)
        {
            // Endian swap based on port size
            if (EepPortSize == sizeof(U16))
            {
                value = EndianSwap16(value);
            }
            else
            {
                value = EndianSwap32(value);
            }
        }

        // Write data to file
        fwrite(
            &value,         // Buffer to write
            sizeof(U32),    // Item size
            1,              // Item count
            pFile           // File pointer
            );
    }

    // In case of 8114 revision, some versions require an
    // extra 32-bit '0' after CRC due to an erratum
    if (pDevice->Key.PlxChip == 0x8114)
    {
        value = 0;

        // Write data to file
        fwrite(
            &value,         // Buffer to write
            sizeof(U32),    // Item size
            1,              // Item count
            pFile           // File pointer
            );

        // Update EEPROM size
        EepSize += sizeof(U32);
    }

    // Close the file
    fclose( pFile );

    Cons_printf("Ok (%dB)\n", (int)EepSize);

    // Note completion time
    end = clock();

    Cons_printf(
        " -- Complete (%.2f sec) --\n",
        ((double)end - start) / CLOCKS_PER_SEC
        );

    return EXIT_CODE_SUCCESS;
}




/******************************************************************************
 *
 * Function   :  Plx8000_EepromFileLoad
 *
 * Description:  Saves the contents of the EEPROM to a file
 *
 ******************************************************************************/
S8
Plx8000_EepromFileLoad(
    PLX_DEVICE_OBJECT *pDevice,
    EEP_OPTIONS       *pOptions,
    BOOLEAN            bCrc
    )
{
    S8       rc;
    U8       bCrcEn;
    U8      *pBuffer;
    U16      Verify_Value_16;
    U32      value;
    U32      Verify_Value;
    U32      Crc;
    U32      offset;
    U32      FileSize;
    U32      EepHeader;
    FILE    *pFile;
    clock_t  start;
    clock_t  end;


    // Note start time
    start = clock();

    pBuffer   = NULL;
    EepHeader = 0;

    // Start with CRC option matching CRC support
    bCrcEn = bCrc;

    Cons_printf("Load EEPROM file... " );
    Cons_fflush(stdout);

    // Open the file to read
    pFile = fopen( pOptions->FileName, "rb" );
    if (pFile == NULL)
    {
        Cons_printf(
            "ERROR: Unable to load \"%s\"\n",
            pOptions->FileName
            );
        return EXIT_CODE_EEP_FAIL;
    }

    // Move to end-of-file
    fseek( pFile, 0, SEEK_END );

    // Determine file size
    FileSize = ftell( pFile );

    // Move back to start of file
    fseek( pFile, 0, SEEK_SET );

    // Allocate a buffer for the data
    pBuffer = malloc( FileSize );
    if (pBuffer == NULL)
    {
        fclose( pFile );
        return EXIT_CODE_EEP_FAIL;
    }

    // Read data from file
    fread(
        pBuffer,        // Buffer for data
        sizeof(U8),     // Item size
        FileSize  ,     // Buffer size
        pFile           // File pointer
        );

    // Close the file
    fclose( pFile );

    Cons_printf( "Ok (%dB)\n", (int)FileSize );

    // Default to successful operation
    rc = EXIT_CODE_SUCCESS;

    Cons_printf( "Program EEPROM..... " );

    // Write 32-bit aligned buffer into EEPROM
    for (offset = 0; offset < (FileSize & ~0x3); offset += sizeof(U32))
    {
        // Periodically update status
        if ((offset & 0x7) == 0)
        {
            // Display current status
            Cons_printf(
                "%02ld%%\b\b\b",
                ((offset * 100) / FileSize)
                );
            Cons_fflush( stdout );
        }

        // Get next value
        value = *(U32*)(pBuffer + offset);

        // For chips that support CRC, verify CRC is enabled (0h[15])
        if (offset == 0)
        {
            // Store EEPROM header
            EepHeader = value;

            // Remove CRC option if disabled
            if ((bCrcEn == TRUE) && ((value & (1 << 15)) == 0))
            {
                bCrcEn = FALSE;
            }
        }

        // Write value & read back to verify
        PlxPci_EepromWriteByOffset( pDevice, offset, value );
        PlxPci_EepromReadByOffset( pDevice, offset, &Verify_Value );

        if (Verify_Value != value)
        {
            Cons_printf(
                "ERROR: offset:%02X  wrote:%08X  read:%08X\n",
                offset, value, Verify_Value
                );
            rc = EXIT_CODE_EEP_FAIL;
            goto _Exit_File_Load_8000;
        }
    }

    // Write any remaining 16-bit unaligned value
    if (offset < FileSize)
    {
        // Get next value
        value = *(U16*)(pBuffer + offset);

        // Write value & read back to verify
        PlxPci_EepromWriteByOffset_16( pDevice, offset, (U16)value );
        PlxPci_EepromReadByOffset_16( pDevice, offset, &Verify_Value_16 );

        if (Verify_Value_16 != (U16)value)
        {
            Cons_printf(
                "ERROR: offset:%02X  wrote:%04X  read:%04X\n",
                offset, value, Verify_Value_16
                );
            goto _Exit_File_Load_8000;
        }
    }
    Cons_printf( "Ok \n" );

    // Update CRC if requested
    if (bCrc)
    {
        Cons_printf("Update CRC......... ");
        if (bCrcEn == FALSE)
        {
            Cons_printf("*DISABLED*\n");
        }
        else
        {
            Cons_fflush( stdout );
            PlxPci_EepromCrcUpdate( pDevice, &Crc, TRUE );
            Cons_printf("Ok (CRC=%08X offset=%02Xh)\n", (int)Crc, (EepHeader >> 16) + sizeof(U32));
        }
    }

_Exit_File_Load_8000:
    // Release the buffer
    if (pBuffer != NULL)
    {
        free(pBuffer);
    }

    // Note completion time
    end = clock();

    Cons_printf(
        " -- Complete (%.2f sec) --\n",
        ((double)end - start) / CLOCKS_PER_SEC
        );

    return rc;
}




/******************************************************************************
 *
 * Function   :  Plx8000_EepromFileSave
 *
 * Description:  Saves the contents of the EEPROM to a file
 *
 ******************************************************************************/
S8
Plx8000_EepromFileSave(
    PLX_DEVICE_OBJECT *pDevice,
    EEP_OPTIONS       *pOptions,
    U32                ByteCount,
    BOOLEAN            bCrc
    )
{
    U8      *pBuffer;
    U16      value16;
    U32      value;
    U32      offset;
    U32      EepSize;
    FILE    *pFile;
    clock_t  start;
    clock_t  end;


    // Note start time
    start = clock();

    Cons_printf( "Get EEPROM data size.. " );

    pBuffer = NULL;

    if (ByteCount != 0)
    {
        EepSize = ByteCount;
    }
    else
    {
        // Start with EEPROM header size
        EepSize = sizeof(U32);

        // Get EEPROM header
        PlxPci_EepromReadByOffset( pDevice, 0x0, &value );

        // Add register byte count
        EepSize += (value >> 16);

        // Add CRC count if supported
        if (bCrc)
        {
            // Remove CRC option if disabled
            if (value & (1 << 15))
            {
                EepSize += sizeof(U32);
            }
            else
            {
                bCrc = FALSE;
            }
        }

        // Get Shared(811x)/8051(MIRA) memory count
        if ((pDevice->Key.PlxFamily == PLX_FAMILY_BRIDGE_PCIE_P2P) ||
            (pDevice->Key.PlxFamily == PLX_FAMILY_MIRA))
        {
            // Get Shared memory count
            PlxPci_EepromReadByOffset_16( pDevice, EepSize, &value16 );

            // Get byte count
            value16 = value16 & 0xFF;

            // Add mem byte count
            EepSize += sizeof(U16);

            // Add mem data
            EepSize += value16;
        }
    }

    Cons_printf(
        "Ok (%dB%s",
        EepSize, (bCrc) ? " inc 32-bit CRC" : ""
        );

    if (pOptions->ExtraBytes)
    {
        Cons_printf(" + %dB extra", pOptions->ExtraBytes);

        // Adjust for extra bytes
        EepSize += pOptions->ExtraBytes;

        // Make sure size aligned on 16-bit boundary
        EepSize = (EepSize + 1) & ~(U32)0x1;
    }
    Cons_printf(")\n");

    Cons_printf( "Read EEPROM data...... " );
    Cons_fflush( stdout );

    // Allocate a buffer for the EEPROM data
    pBuffer = malloc( EepSize );
    if (pBuffer == NULL)
    {
        return EXIT_CODE_EEP_FAIL;
    }

    // Read 32-bit aligned EEPROM data into buffer
    for (offset = 0; offset < (EepSize & ~0x3); offset += sizeof(U32))
    {
        PlxPci_EepromReadByOffset( pDevice, offset, (U32*)(pBuffer + offset) );
    }

    // Read any remaining 16-bit aligned byte
    if (offset < EepSize)
    {
        PlxPci_EepromReadByOffset_16( pDevice, offset, (U16*)(pBuffer + offset) );
    }
    Cons_printf( "Ok\n" );

    Cons_printf( "Write data to file.... " );
    Cons_fflush( stdout );

    // Open the file to write
    pFile = fopen( pOptions->FileName, "wb" );
    if (pFile == NULL)
    {
        return EXIT_CODE_EEP_FAIL;
    }

    // Write buffer to file
    fwrite(
        pBuffer,        // Buffer to write
        sizeof(U8),     // Item size
        EepSize,        // Buffer size
        pFile           // File pointer
        );

    // Close the file
    fclose( pFile );

    // Release the buffer
    if (pBuffer != NULL)
    {
        free(pBuffer);
    }

    Cons_printf( "Ok (%s)\n", pOptions->FileName );

    // Note completion time
    end = clock();

    Cons_printf(
        " -- Complete (%.2f sec) --\n",
        ((double)end - start) / CLOCKS_PER_SEC
        );

    return EXIT_CODE_SUCCESS;
}
