/*******************************************************************************
 * Copyright 2013-2015 Avago Technologies
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
 *      PlxDmaPerf.c
 *
 * Description:
 *
 *      Demonstrates using the PLX API to perform DMA transfers & measure performance
 *
 * Revision History:
 *
 *      03-01-13 : PLX SDK v7.00
 *
 ******************************************************************************/


#include <sys/timeb.h>
#include "PlxApi.h"

#if defined(_WIN32)
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
#define DMA_TIMEOUT_SEC                 3                                       // Max time to wait for DMA completion
#define UPDATE_DISPLAY_SEC              4                                       // Number of seconds between display updates
#define PlxReg(Va, reg)                 (*(volatile U32 *)(((U8*)Va) + (reg)))  // Macro to access PLX Chip registers

 
 
 
/**********************************************
 *               Functions
 *********************************************/
S8
SelectDevice_DMA(
    PLX_DEVICE_KEY *pKey
    );

void
PerformDma_8000(
    PLX_DEVICE_OBJECT *pDevice,
    U32 NTSrcAddr,
	U32 NTDestAddr,
	U32 NTSize
    );




/******************************************************************************
 *
 * Function   :  Do_DMA_Test
 *
 * Description:  The main entry point
 *
 *****************************************************************************/
int
Do_DMA_Test(
    U32                NTSrcAddr,
    U32                NTDestAddr,
    U32                NTSize,
    PLX_DEVICE_OBJECT *pNTDevice
    )
{
    S8                DeviceSelected;
    PLX_STATUS        rc;
    PLX_DEVICE_KEY    DeviceKey;
    PLX_DEVICE_OBJECT Device;
    U16               LutIndex = 0xffff;
    U16               ReqId;
    U32               flags = 0;



    /************************************
     *         Select Device
     ***********************************/
    DeviceSelected =
        SelectDevice_DMA(
            &DeviceKey
            );

    if (DeviceSelected == -1)
    {
        ConsoleEnd();
        exit(0);
    }

    rc =
        PlxPci_DeviceOpen(
            &DeviceKey,
            &Device
            );

    if (rc != PLX_STATUS_OK)
    {
        Cons_printf("\n   ERROR: Unable to find or select a PLX device\n");
        PlxSdkErrorDisplay(rc);
        _Pause;
        ConsoleEnd();
        exit(-1);
    }

    Cons_printf(
        "\nSelected: %04x %04x [b:%02x  s:%02x  f:%x]\n\n",
        DeviceKey.DeviceId, DeviceKey.VendorId,
        DeviceKey.bus, DeviceKey.slot, DeviceKey.function
        );

    // Must add the DMA device ReqID to the NT LUT
    ReqId = ((U16)DeviceKey.bus << 8) | (DeviceKey.slot << 3) | (DeviceKey.function & 0x03);

    PlxPci_Nt_LutAdd(
        pNTDevice,
        &LutIndex,
        ReqId,
        flags
        );

    /************************************
     *        Perform the DMA
     ************************************/
    if (((DeviceKey.PlxChip & 0xF000) == 0x8000) &&
         (DeviceKey.PlxChip != 0x8311))
    {
        PerformDma_8000( &Device, NTSrcAddr, NTDestAddr, NTSize );
    }
    else
    {
        Cons_printf(
            "ERROR: DMA not supported by the selected device (%04X)\n",
            DeviceKey.PlxChip
            );
        goto _Exit_App;
    }


    /************************************
     *        Close the Device
     ***********************************/
_Exit_App:
    PlxPci_DeviceClose(
        &Device
        );

    _Pause;

    Cons_printf("\n\n");

    return 0;
}




/*********************************************************************
 *
 * Function   : SelectDevice_DMA
 *
 * Description: Asks the user which PLX DMA device to select
 *
 * Returns    : Total devices found
 *              -1,  if user cancelled the selection
 *
 ********************************************************************/
S8
SelectDevice_DMA(
    PLX_DEVICE_KEY *pKey
    )
{
    S32               i;
    S32               NumDevices;
    BOOLEAN           bAddDevice;
    PLX_STATUS        status;
    PLX_DEVICE_KEY    DevKey;
    PLX_DEVICE_KEY    DevKey_DMA[MAX_DEVICES_TO_LIST];
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
        status =
            PlxPci_DeviceFind(
                &DevKey,
                (U16)i
                );

        if (status == PLX_STATUS_OK)
        {
            // Default to add device
            bAddDevice = TRUE;

            if (bAddDevice)
            {
                // Verify supported chip type
                switch (DevKey.PlxChip)
                {
                    default:
                        if ((DevKey.PlxChip & 0xF000) == 0x8000)
                        {
                            // DMA is always function 1 or higher
                            if (DevKey.function == 0)
                                bAddDevice = FALSE;
                        }
                        else
                        {
                            bAddDevice = FALSE;
                        }
                        break;
                }
            }

            if (bAddDevice)
            {
                // Open device to get its properties
                PlxPci_DeviceOpen(
                    &DevKey,
                    &Device
                    );
            }

            // Verify driver used is PnP and not Service driver
            if (bAddDevice)
            {
                PlxPci_DriverProperties(
                    &Device,
                    &DriverProp
                    );

                if (DriverProp.bIsServiceDriver)
                {
                    bAddDevice = FALSE;
                }
            }

            // Close device
            PlxPci_DeviceClose(
                &Device
                );

            if (bAddDevice)
            {
                // Copy device key info
                DevKey_DMA[NumDevices] = DevKey;

                Cons_printf(
                    "\t\t  %2d. %04x [b:%02x s:%02x f:%x]\n",
                    (NumDevices + 1), DevKey.PlxChip,
                    DevKey.bus, DevKey.slot, DevKey.function
                    );

                // Increment device count
                NumDevices++;
            }
        }

        // Go to next devices
        i++;
    }
    while ((status == PLX_STATUS_OK) && (NumDevices < MAX_DEVICES_TO_LIST));

    if (NumDevices == 0)
        return 0;

    Cons_printf(
        "\t\t   0. Cancel\n\n"
        );

    do
    {
        Cons_printf("\t  Device Selection --> ");
        Cons_scanf("%d", &i);
    }
    while (i > NumDevices);

    if (i == 0)
        return -1;

    // Return selected device information
    *pKey = DevKey_DMA[i - 1];

    return (S8)NumDevices;
}




/********************************************************
 *
 *******************************************************/
void
PerformDma_8000(
    PLX_DEVICE_OBJECT *pDevice,
    U32 NTSrcAddr,
	U32 NTDestAddr,
	U32 NTSize
    )
{
    U8               *VaBar0 = 0;
    U8                DmaChannel;
    U16               UserInput;
    U32               LoopCount;
    U32               PollCount;
    U32               OffsetDmaCmd;
    U32               ElapsedTime_ms;
    VOID             *BarVa;
    double            Stat_TxTotalCount;
    double            Stat_TxTotalBytes;
    BOOLEAN           bInterrupts;
    PLX_STATUS        status;
    struct timeb      StartTime, EndTime;
    PLX_DMA_PROP      DmaProp;
    PLX_INTERRUPT     PlxInterrupt;
    PLX_DMA_PARAMS    DmaParams;
    PLX_NOTIFY_OBJECT NotifyObject;


    Cons_printf("\n\n");
    Cons_printf("Please select a DMA channel (0-3)   --> ");
    Cons_scanf("%hd", &UserInput);

    if (UserInput >= 4)
    {
        Cons_printf("ERROR: Unsupported DMA channel, test aborted\n");
        return;
    }

    DmaChannel = (U8)UserInput;

    // Set offset of DMA command register
    OffsetDmaCmd  = 0x200 + (DmaChannel * 0x100) + 0x38;

    // Determine whether to use interrupts or polling
    Cons_printf("Use interrupts(i) or poll(p) [i/p]? --> ");
    UserInput = Cons_getch();
    Cons_printf("%c\n\n", UserInput);

    if (UserInput == 'i' || UserInput == 'I')
        bInterrupts = TRUE;
    else
        bInterrupts = FALSE;


    /**************************************************************
     *
     *************************************************************/
    // Open the DMA channel
    Cons_printf("  Open Channel %i for DMA......... ", DmaChannel);
    status =
        PlxPci_DmaChannelOpen(
            pDevice,
            DmaChannel,
            NULL
            );

    if (status == PLX_STATUS_OK)
    {
        Cons_printf("Ok\n");
    }
    else
    {
        Cons_printf("*ERROR* - API failed\n");
        PlxSdkErrorDisplay(status);
    }


    // Get current DMA properties
    Cons_printf("  Get DMA propeties.............. ");
    status =
        PlxPci_DmaGetProperties(
            pDevice,
            DmaChannel,
            &DmaProp
            );

    if (status != PLX_STATUS_OK)
    {
        Cons_printf("*ERROR* - API failed\n");
        PlxSdkErrorDisplay(status);
        goto _ExitDmaTest;
    }
    Cons_printf("Ok\n");



    // Update any DMA properties
    Cons_printf("  Set DMA propeties.............. ");

    // Set to support 128B TLP read request size
    DmaProp.MaxSrcXferSize = PLX_DMA_MAX_SRC_TSIZE_128B;

    status =
        PlxPci_DmaSetProperties(
            pDevice,
            DmaChannel,
            &DmaProp
            );

    if (status != PLX_STATUS_OK)
    {
        Cons_printf("*ERROR* - API failed\n");
        PlxSdkErrorDisplay(status);
        goto _ExitDmaTest;
    }
    Cons_printf("Ok\n");


    /**************************************************************
     *
     *************************************************************/
    if (bInterrupts)
    {
        Cons_printf("  Register for notification...... ");

        // Clear interrupt fields
        memset( &PlxInterrupt, 0, sizeof(PLX_INTERRUPT) );

        // Setup to wait for selected DMA channel
        PlxInterrupt.DmaDone = (1 << DmaChannel);

        status =
            PlxPci_NotificationRegisterFor(
                pDevice,
                &PlxInterrupt,
                &NotifyObject
                );

        if (status != PLX_STATUS_OK)
        {
            Cons_printf("*ERROR* - API failed\n");
            PlxSdkErrorDisplay(status);
            return;
        }
        Cons_printf( "Ok\n" );
    }
    else
    {
        Cons_printf("  Map BAR 0 for register access.. ");

        status =
            PlxPci_PciBarMap(
                pDevice,
                0,
                &BarVa
                );

        if (status != PLX_STATUS_OK)
        {
            Cons_printf("*ERROR* - API failed\n");
            PlxSdkErrorDisplay(status);
            return;
        }
        Cons_printf("Ok (VA=%p)\n", BarVa);

        VaBar0 = BarVa;
    }


    /*****************************************
     *
     *          Transfer the Data
     *
     *****************************************/
    Cons_printf("\n\n");
    Cons_printf("  --- Performing DMA transfers (Press ESC to halt) ---\n");

    // Clear DMA data
    memset(&DmaParams, 0, sizeof(PLX_DMA_PARAMS));

    DmaParams.AddrSource = NTSrcAddr;
    DmaParams.AddrDest   = NTDestAddr;
    DmaParams.ByteCount  = NTSize;

    // If polling, disable DMA interrupt
    if (bInterrupts == FALSE)
        DmaParams.bIgnoreBlockInt = TRUE;

    LoopCount = 0;

    // Reset stats
    Stat_TxTotalCount = 0;
    Stat_TxTotalBytes = 0;

    // Get initial start time
    ftime( &StartTime );

    do
    {
        // Periodically display statistics
        if ((LoopCount & 0x0000003F) == 0)
        {
            // Get end time
            ftime( &EndTime );

            // Calculate elapsed time in milliseconds
            ElapsedTime_ms = (((U32)EndTime.time * 1000) + EndTime.millitm) -
                             (((U32)StartTime.time * 1000) + StartTime.millitm);

            if (ElapsedTime_ms >= (UPDATE_DISPLAY_SEC * 1000))
            {
                // Display statistics
                if (ElapsedTime_ms != 0)
                {
                    Cons_printf(
                        " Transfers: %0.0lf   Bytes: %0.2lf MB   Time: %ldms   Rate:%6.3lf MB/s\n",
                        Stat_TxTotalCount, Stat_TxTotalBytes / (1 << 20), ElapsedTime_ms,
                        ((Stat_TxTotalBytes * 1000) / (double)ElapsedTime_ms) / (double)(1 << 20)
                        );
                }

                // Reset stats
                Stat_TxTotalCount = 0;
                Stat_TxTotalBytes = 0;

                // Check for user cancel
                if (Cons_kbhit())
                {
                    if (Cons_getch() == 27)
                        goto _ExitDmaTest;
                }

                // Get new start time
                ftime( &StartTime );
            }
        }

        status =
            PlxPci_DmaTransferBlock(
                pDevice,
                DmaChannel,
                &DmaParams,
                0          // Don't wait for completion
                );

        if (status != PLX_STATUS_OK)
        {
            Cons_printf("*ERROR* - API failed\n");
            PlxSdkErrorDisplay(status);
            goto _ExitDmaTest;
        }

        if (bInterrupts)
        {
            status =
                PlxPci_NotificationWait(
                    pDevice,
                    &NotifyObject,
                    DMA_TIMEOUT_SEC * 1000
                    );

            switch (status)
            {
                case PLX_STATUS_OK:
                    break;

                case PLX_STATUS_TIMEOUT:
                    Cons_printf("*ERROR* - Timeout waiting for Int Event\n");
                    goto _ExitDmaTest;

                case PLX_STATUS_CANCELED:
                    Cons_printf("*ERROR* - Interrupt event cancelled\n");
                    goto _ExitDmaTest;

                default:
                    Cons_printf("*ERROR* - API failed\n");
                    PlxSdkErrorDisplay(status);
                    goto _ExitDmaTest;
            }
        }
        else
        {
            PollCount = 1000000;

            // Poll for DMA completion
            do
            {
                PollCount--;

                if ((PlxReg(VaBar0, OffsetDmaCmd) & (1 << 30)) == 0)
                    break;
            }
            while (PollCount != 0);

            // Check if DMA is truly complete
            if (PollCount == 0)
            {
                Cons_printf("*ERROR* - Timeout waiting for DMA to complete\n");
                goto _ExitDmaTest;
            }
        }

        // Update statistics
        Stat_TxTotalCount++;
        Stat_TxTotalBytes += NTSize;

        LoopCount++;
    }
    while (1);


_ExitDmaTest:

    Cons_printf("\n       ------------------\n");

    // Release the interrupt wait object
    if (bInterrupts)
    {
        Cons_printf("  Cancelling Int Notification.... ");
        status =
            PlxPci_NotificationCancel(
                pDevice,
                &NotifyObject
                );

        if (status != PLX_STATUS_OK)
        {
            Cons_printf("*ERROR* - API failed\n");
            PlxSdkErrorDisplay(status);
        }
        else
        {
            Cons_printf("Ok\n");
        }
    }

    // Close DMA Channel
    Cons_printf("  Close DMA Channel.............. ");
    status =
        PlxPci_DmaChannelClose(
            pDevice,
            DmaChannel
            );

    if (status == PLX_STATUS_OK)
    {
        Cons_printf("Ok\n");
    }
    else
    {
        Cons_printf("*ERROR* - API failed\n");
        PlxSdkErrorDisplay(status);
    }
}
