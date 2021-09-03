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
 *      PerfMonitor.c
 *
 * Description:
 *
 *      This sample demonstrates how to use the PLX Performance API with 8000
 *      devices to measure PCIe performance
 *
 * Revision History:
 *
 *      03-01-18 : PLX SDK v8.00
 *
 ******************************************************************************/


#include <sys/timeb.h>
#include "PlxApi.h"

#if defined(PLX_MSWINDOWS)
    #include "..\\Shared\\ConsFunc.h"
    #include "..\\Shared\\PlxInit.h"
#endif

#if defined(PLX_LINUX)
    #include "ConsFunc.h"
    #include "PlxInit.h"
#endif




/**********************************************
 *               Definitions
 *********************************************/
#define SLEEP_INTERVAL_MS                   1000    // Num of milliseconds for sleep interval



/**********************************************
 *               Functions
 *********************************************/
void
PerformTest(
    PLX_DEVICE_OBJECT *pDevice
    );

void
GroupDigitsWithTag(
    S64   value,
    char *pStr
    );

S16
SelectDevice_8000(
    PLX_DEVICE_KEY *pKey
    );






/******************************************************************************
 *
 * Function   :  main
 *
 * Description:  The main entry point
 *
 *****************************************************************************/
int 
main(
    void
    )
{
    S16               DeviceSelected;
    PLX_STATUS        status;
    PLX_DEVICE_KEY    DeviceKey;
    PLX_DEVICE_OBJECT Device;


    ConsoleInitialize();

    Cons_clear();

    Cons_printf(
        "\n\n"
        "\t\t     PLX Performance Monitor Sample\n"
        "\n"
        "\t\t        ***********************\n"
        "\tThis application is provided to demonstrate how to measure\n"
        "\tPCIe performance using the PLX 8000 performance counters\n"
        "\tfeature. The PLX Performance API functions are used to retrieve\n"
        "\tthe counter data and calculate the resulting statistics.\n"
        "\tIf you keep this application running and perform accesses to the\n"
        "\t8000 device or an endpoint downstream of it, you will see the\n"
        "\tmeasured performance numbers.  All active ports may be monitored,\n"
        "\tbut this application only displays the measurements for one port.\n\n"
        );


    /************************************
     *         Select Device
     ***********************************/
    DeviceSelected =
        SelectDevice_8000(
            &DeviceKey
            );

    if (DeviceSelected == -1)
    {
        ConsoleEnd();
        exit(0);
    }

    status = PlxPci_DeviceOpen( &DeviceKey, &Device );

    if (status != PLX_STATUS_OK)
    {
        Cons_printf("\n   ERROR: Unable to find or select a PLX device (status=%X)\n", status);
        ConsoleEnd();
        exit(-1);
    }

    Cons_printf(
        "\nSelected: %04x %04x [%02X:%02X.%X]\n\n",
        DeviceKey.DeviceId, DeviceKey.VendorId,
        DeviceKey.bus, DeviceKey.slot, DeviceKey.function
        );



    /************************************
    *        Perform the Test
    ************************************/
    PerformTest( &Device );



    /************************************
    *        Close the Device
    ************************************/
    PlxPci_DeviceClose( &Device );

    ConsoleEnd();

    exit(0);
}




/******************************************************************************
 *
 * Function   :  
 *
 * Description:  
 *
 *****************************************************************************/
void
PerformTest(
    PLX_DEVICE_OBJECT *pDevice
    )
{
    char           Str_TotalBytes[20];
    char           Str_TotalRate[20];
    char           Str_PayloadTotal[20];
    char           Str_PayloadRate[20];
    U32            ElapsedTime_ms;
    struct timeb   PrevTime, EndTime;
    PLX_STATUS     status;
    PLX_PERF_PROP  PerfProp;
    PLX_PERF_STATS PerfStats;


    Cons_printf("Sleep interval: %d ms\n", SLEEP_INTERVAL_MS);

    Cons_printf(
        "\n\n"
        "          Link      Total      Total        Pyld        Pyld       Pyld   \n"
        "   Port   Util       Data       Rate        Total       Rate      Avg/TLP \n"
        " --------------------------------------------------------------------------\n"
        );

    // Initialize performance objects
    status =
        PlxPci_PerformanceInitializeProperties(
            pDevice,
            &PerfProp
            );
    if (status != PLX_STATUS_OK)
    {
        PlxSdkErrorDisplay( status );
        return;
    }

    // Start performance monitor
    PlxPci_PerformanceMonitorControl(
        pDevice,
        PLX_PERF_CMD_START
        );

    // Reset counters
    PlxPci_PerformanceResetCounters(
        pDevice,
        &PerfProp,
        1           // Only one object
        );

    // Get starting time
    ftime( &PrevTime );

    do
    {
        Plx_sleep( SLEEP_INTERVAL_MS );

        // Get statistics
        PlxPci_PerformanceGetCounters(
            pDevice,
            &PerfProp,
            1           // Only one object
            );

        // Get end time
        ftime( &EndTime );

        // Calculate elapsed time in milliseconds
        ElapsedTime_ms = (((U32)EndTime.time * 1000) + EndTime.millitm) -
                         (((U32)PrevTime.time * 1000) + PrevTime.millitm);

        // Store previous time
        PrevTime = EndTime;

        // Calculate performance statistics
        PlxPci_PerformanceCalcStatistics(
            &PerfProp,
            &PerfStats,
            ElapsedTime_ms
            );

        //
        // Ingress port
        //
        GroupDigitsWithTag(
            (S64)PerfStats.IngressTotalBytes,
            Str_TotalBytes
            );

        GroupDigitsWithTag(
            (S64)PerfStats.IngressTotalByteRate,
            Str_TotalRate
            );

        GroupDigitsWithTag(
            (S64)PerfStats.IngressPayloadTotalBytes,
            Str_PayloadTotal
            );

        GroupDigitsWithTag(
            (S64)PerfStats.IngressPayloadByteRate,
            Str_PayloadRate
            );

        // Display statistics
       Cons_printf(
            " %d Ing: %6.2lf%% %10s %10s/s %10s %10s/s %7.2lf B\n",
            PerfProp.PortNumber,
            (double)PerfStats.IngressLinkUtilization,
            Str_TotalBytes,
            Str_TotalRate,
            Str_PayloadTotal,
            Str_PayloadRate,
            (double)PerfStats.IngressPayloadAvgPerTlp
            );

        //
        // Egress port
        //
        GroupDigitsWithTag(
            (S64)PerfStats.EgressPayloadByteRate,
            Str_PayloadRate
            );

        GroupDigitsWithTag(
            (S64)PerfStats.EgressTotalBytes,
            Str_TotalBytes
            );

        GroupDigitsWithTag(
            (S64)PerfStats.EgressPayloadTotalBytes,
            Str_PayloadTotal
            );

        GroupDigitsWithTag(
            (S64)PerfStats.EgressTotalByteRate,
            Str_TotalRate
            );

        // Display statistics
       Cons_printf(
            " %d Egr: %6.2lf%% %10s %10s/s %10s %10s/s %7.2lf B\n\n",
            PerfProp.PortNumber,
            (double)PerfStats.EgressLinkUtilization,
            Str_TotalBytes,
            Str_TotalRate,
            Str_PayloadTotal,
            Str_PayloadRate,
            (double)PerfStats.EgressPayloadAvgPerTlp
            );

        if (Cons_kbhit())
        {
            if (Cons_getch() == 27)
            {
                Cons_printf("\n");
                break;
            }
        }
    }
    while (1);

    // Stop monitor
    PlxPci_PerformanceMonitorControl(
        pDevice,
        PLX_PERF_CMD_STOP
        );
}




/*********************************************************************
 *
 * Function   :  GroupDigitsWithTag
 *
 * Description:  Formats a decimal integer to make more readable
 *
 ********************************************************************/
void
GroupDigitsWithTag(
    S64   value,
    char *pStr
    )
{
    double DecimalValue;


    if (value < (1 << 10))              // bytes
    {
        sprintf(pStr, "%u B", (U32)value);
    }
    else if (value < (1 << 20))         // KB
    {
        DecimalValue = (double)value / (1 << 10);
        sprintf(pStr, "%.2lf KB", DecimalValue);
    }
    else if (value < ((S64)1 << 30))    // MB
    {
        DecimalValue = (double)value / (1 << 20);
        sprintf(pStr, "%.2lf MB", DecimalValue);
    }
    else if (value < ((S64)1 << 40))    // GB
    {
        DecimalValue = (long double)value / (1 << 30);
        sprintf(pStr, "%.2lf GB", DecimalValue);
    }
    else                                // TB
    {
        DecimalValue = (long double)value / ((S64)1 << 40);
        sprintf(pStr, "%.2lf TB", DecimalValue);
    }
}




/*********************************************************************
 *
 * Function   : SelectDevice_8000
 *
 * Description: Asks the user which PLX device to select
 *
 * Returns    : Total devices found
 *              -1,  if user cancelled the selection
 *
 ********************************************************************/
S16
SelectDevice_8000(
    PLX_DEVICE_KEY *pKey
    )
{
    S32               i;
    S32               NumDevices;
    BOOLEAN           bAddDevice;
    PLX_STATUS        status;
    PLX_PORT_PROP     PortProp;
    PLX_DEVICE_KEY    DevKey;
    PLX_DEVICE_KEY    DevKey_US[MAX_DEVICES_TO_LIST];
    PLX_DRIVER_PROP   DriverProp;
    PLX_DEVICE_OBJECT Device;


    Cons_printf("\n");

    i          = 0;
    NumDevices = 0;

    do
    {
        // Setup for next device
        memset(&DevKey, PCI_FIELD_IGNORE, sizeof(PLX_DEVICE_KEY));

        // Check if device exists
        status = PlxPci_DeviceFind( &DevKey, (U16)i );

        if (status == PLX_STATUS_OK)
        {
            // Default to add device
            bAddDevice = TRUE;

            // Verify supported chip type
            if (((DevKey.PlxChip & 0xFF00) != 0x2300) &&
                ((DevKey.PlxChip & 0xFF00) != 0x3300) &&
                ((DevKey.PlxChip & 0xFF00) != 0x8600) &&
                ((DevKey.PlxChip & 0xFF00) != 0x8700) &&
                ((DevKey.PlxChip & 0xFF00) != 0x9700) &&
                ((DevKey.PlxChip & 0xFF00) != 0xC000))
            {
                bAddDevice = FALSE;
            }

            // Open device to get its properties
            if (bAddDevice)
            {
                PlxPci_DeviceOpen( &DevKey, &Device );

                // Get port properties
                PlxPci_GetPortProperties( &Device, &PortProp );

                // Only certain port types are allowed
                if ((PortProp.PortType != PLX_PORT_UPSTREAM)   &&
                    (PortProp.PortType != PLX_PORT_DOWNSTREAM) &&
                    (PortProp.PortType != PLX_PORT_ENDPOINT)   &&
                    (PortProp.PortType != PLX_PORT_LEGACY_ENDPOINT))
                {
                    bAddDevice = FALSE;
                }

                // For Atlas, ignore special internal ports
                if ( (DevKey.PlxFamily == PLX_FAMILY_ATLAS) &&
                     (PortProp.PortNumber >= 96) )
                {
                    bAddDevice = FALSE;
                }

                // For MIRA USB EP, PM only available in Legacy mode
                if ( (DevKey.PlxFamily == PLX_FAMILY_MIRA) &&
                     (PortProp.PortType == PLX_PORT_LEGACY_ENDPOINT) )
                {
                    if (DevKey.DeviceMode == PLX_PORT_ENDPOINT)
                    {
                        bAddDevice = FALSE;
                    }
                }

                // For endpoints, only NT virtual is supported
                if ((PortProp.PortType == PLX_PORT_ENDPOINT) &&
                    (DevKey.PlxPortType != PLX_SPEC_PORT_NT_VIRTUAL))
                {
                    bAddDevice = FALSE;
                }
            }

            // Verify driver used is Service driver
            if (bAddDevice)
            {
                PlxPci_DriverProperties( &Device, &DriverProp );
                if (DriverProp.bIsServiceDriver == FALSE)
                {
                    bAddDevice = FALSE;
                }
            }

            // Close device
            PlxPci_DeviceClose( &Device );

            if (bAddDevice)
            {
                // Copy device key info
                DevKey_US[NumDevices] = DevKey;

                Cons_printf(
                    "\t\t  %2d. %04X Port %2d [%02x:%02x.%x] ",
                    (NumDevices + 1), DevKey.PlxChip, PortProp.PortNumber,
                    DevKey.bus, DevKey.slot, DevKey.function
                    );

                if (DevKey.PlxPortType == PLX_SPEC_PORT_UPSTREAM)
                    Cons_printf("PLX Upstream port\n");
                else if (DevKey.PlxPortType == PLX_SPEC_PORT_DOWNSTREAM)
                    Cons_printf("PLX Downstream port\n");
                else if (DevKey.PlxPortType == PLX_SPEC_PORT_NT_VIRTUAL)
                    Cons_printf("PLX NT Virtual port\n");
                else if (DevKey.PlxPortType == PLX_SPEC_PORT_NT_LINK)
                    Cons_printf("PLX NT Link port\n");
                else if (DevKey.PlxPortType == PLX_SPEC_PORT_LEGACY_EP)
                    Cons_printf("PLX USB Controller\n");
                else if (DevKey.PlxPortType == PLX_SPEC_PORT_DMA)
                    Cons_printf("PLX DMA controller\n");
                else if (DevKey.PlxPortType == PLX_SPEC_PORT_HOST)
                    Cons_printf("PLX Host port\n");
                else if (DevKey.PlxPortType == PLX_SPEC_PORT_FABRIC)
                    Cons_printf("PLX Fabric port\n");
                else if (DevKey.PlxPortType == PLX_SPEC_PORT_GEP)
                    Cons_printf("PLX GEP\n");
                else
                    Cons_printf("**Unknown device**\n");

                // Increment to next device
                NumDevices++;
            }

            i++;
        }
    }
    while ((status == PLX_STATUS_OK) && (NumDevices < MAX_DEVICES_TO_LIST));

    if (NumDevices == 0)
    {
        return 0;
    }

    Cons_printf( "\t\t   0. Cancel\n\n" );

    do
    {
        Cons_printf("\t  Device Selection --> ");
        Cons_scanf("%d", &i);
    }
    while (i > NumDevices);

    if (i == 0)
    {
        Cons_printf("\n");
        return -1;
    }

    // Return selected device information
    *pKey = DevKey_US[i - 1];

    return (S16)NumDevices;
}
