/******************************************************************************
 *
 * File Name:
 *
 *      PlxDma.c
 *
 * Description:
 *
 *      Demonstrates using the PLX API to perform DMA transfers
 *
 * Revision History:
 *
 *      03-01-13 : PLX SDK v7.00
 *
 ******************************************************************************/


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
 *               Functions
 *********************************************/
S16
SelectDevice_DMA(
    PLX_DEVICE_KEY *pKey
    );

void
PerformDma_9000(
    PLX_DEVICE_OBJECT *pDevice
    );

void
PerformDma_8000(
    PLX_DEVICE_OBJECT *pDevice
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
    PLX_STATUS        rc;
    PLX_DEVICE_KEY    DeviceKey;
    PLX_DEVICE_OBJECT Device;


    ConsoleInitialize();

    Cons_clear();

    Cons_printf(
        "\n\n"
        "\t\t         PLX DMA Sample Application\n\n"
        );


    /************************************
    *         Select Device
    ************************************/
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

    if (rc != ApiSuccess)
    {
        Cons_printf("\n   ERROR: Unable to find or select a PLX device\n");
        PlxSdkErrorDisplay(rc);
        _Pause;
        ConsoleEnd();
        exit(-1);
    }

    Cons_clear();

    Cons_printf(
        "\nSelected: %04x %04x [b:%02x  s:%02x  f:%x]\n\n",
        DeviceKey.DeviceId, DeviceKey.VendorId,
        DeviceKey.bus, DeviceKey.slot, DeviceKey.function
        );


    /************************************
     *        Perform the DMA
     ************************************/
    if (((DeviceKey.PlxChip & 0xF000) == 0x9000) ||
         (DeviceKey.PlxChip == 0x8311))
    {
        PerformDma_9000( &Device );
    }
    else if ((DeviceKey.PlxChip & 0xF000) == 0x8000)
    {
        PerformDma_8000( &Device );
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

    ConsoleEnd();

    exit(0);
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
S16
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

        if (status == ApiSuccess)
        {
            // Default to add device
            bAddDevice = TRUE;

            if (bAddDevice)
            {
                // Verify supported chip type
                switch (DevKey.PlxChip)
                {
                    case 0x9080:
                    case 0x9054:
                    case 0x9056:
                    case 0x9656:
                    case 0x8311:
                        break;

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
    while ((status == ApiSuccess) && (NumDevices < MAX_DEVICES_TO_LIST));

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

    return (S16)NumDevices;
}




/********************************************************
 *
 *******************************************************/
void
PerformDma_9000(
    PLX_DEVICE_OBJECT *pDevice
    )
{
    U8                DmaChannel;
    U8               *pUserBuffer = NULL;
    U16               ChannelInput;
    U32               LocalAddress;
    PLX_STATUS        rc;
    PLX_DMA_PROP      DmaProp;
    PLX_INTERRUPT     PlxInterrupt;
    PLX_DMA_PARAMS    DmaParams;
    PLX_PHYSICAL_MEM  PciBuffer;
    PLX_NOTIFY_OBJECT NotifyObject;


    Cons_printf(
        "Description:\n"
        "     This sample demonstrates usage of the PLX DMA API by\n"
        "     performing DMA transfers and waiting for DMA completion.\n"
        );

    Cons_printf(
        "\n"
        " WARNING: There is no safeguard mechanism to protect against invalid\n"
        "          local bus addresses.  Please be careful when selecting local\n"
        "          addresses to transfer data to/from.  The DMA engine will hang\n"
        "          if an invalid address is accessed.\n"
        "\n\n"
        );

    Cons_printf("Please enter a valid local address ----> ");
    Cons_scanf("%x", &LocalAddress);

    Cons_printf("Please select a DMA channel (0 or 1) --> ");
    Cons_scanf("%hd", &ChannelInput);
    Cons_printf("\n");

    if ((ChannelInput != 0) && (ChannelInput != 1))
    {
        Cons_printf("ERROR: Unsupported DMA channel, test aborted\n");
        return;
    }

    DmaChannel = (U8)ChannelInput;

    // Get DMA buffer parameters
    PlxPci_CommonBufferProperties(
        pDevice,
        &PciBuffer
        );


    // Clear DMA structure
    memset(&DmaProp, 0, sizeof(PLX_DMA_PROP));

    // Initialize the DMA channel
    DmaProp.LocalBusWidth = 3;   // 32-bit
    DmaProp.ReadyInput    = 1;

    Cons_printf("  Open Channel %i for DMA......... ", DmaChannel);
    rc =
        PlxPci_DmaChannelOpen(
            pDevice,
            DmaChannel,
            &DmaProp
            );

    if (rc == ApiSuccess)
    {
        Cons_printf("Ok\n");
    }
    else
    {
        Cons_printf("*ERROR* - API failed\n");
        PlxSdkErrorDisplay(rc);
    }


    Cons_printf("  Register for notification...... ");

    // Clear interrupt fields
    memset(&PlxInterrupt, 0, sizeof(PLX_INTERRUPT));

    // Setup to wait for selected DMA channel
    PlxInterrupt.DmaDone = (1 << DmaChannel);

    rc =
        PlxPci_NotificationRegisterFor(
            pDevice,
            &PlxInterrupt,
            &NotifyObject
            );

    if (rc != ApiSuccess)
    {
        Cons_printf("*ERROR* - API failed\n");
        PlxSdkErrorDisplay(rc);
    }
    else
    {
        Cons_printf("Ok\n");
    }


    // Transfer the Data
    Cons_printf("  Perform Block DMA transfer..... ");

    // Clear DMA data
    memset(&DmaParams, 0, sizeof(PLX_DMA_PARAMS));

    DmaParams.PciAddr   = PciBuffer.PhysicalAddr;
    DmaParams.LocalAddr = LocalAddress;
    DmaParams.ByteCount = 0x10;
    DmaParams.Direction = PLX_DMA_LOC_TO_PCI;

    rc =
        PlxPci_DmaTransferBlock(
            pDevice,
            DmaChannel,
            &DmaParams,
            0          // Don't wait for completion
            );

    if (rc == ApiSuccess)
    {
        Cons_printf("Ok\n");

        Cons_printf("  Wait for interrupt event....... ");

        rc =
            PlxPci_NotificationWait(
                pDevice,
                &NotifyObject,
                5 * 1000
                );

        switch (rc)
        {
            case ApiSuccess:
                Cons_printf("Ok (DMA Int received)\n");
                break;

            case ApiWaitTimeout:
                Cons_printf("*ERROR* - Timeout waiting for Int Event\n");
                break;

            case ApiWaitCanceled:
                Cons_printf("*ERROR* - Interrupt event cancelled\n");
                break;

            default:
                Cons_printf("*ERROR* - API failed\n");
                PlxSdkErrorDisplay(rc);
                break;
        }
    }
    else
    {
        Cons_printf("*ERROR* - API failed\n");
        PlxSdkErrorDisplay(rc);
    }


    Cons_printf("  Check notification status...... ");
    rc =
        PlxPci_NotificationStatus(
            pDevice,
            &NotifyObject,
            &PlxInterrupt
            );

    if (rc == ApiSuccess)
    {
        Cons_printf("Ok (triggered ints:");

        if (PlxInterrupt.DmaDone & (1 << 0))
            Cons_printf(" DMA_0");

        if (PlxInterrupt.DmaDone & (1 << 1))
            Cons_printf(" DMA_1");

        Cons_printf(")\n");
    }
    else
    {
        Cons_printf("*ERROR* - API failed\n");
        PlxSdkErrorDisplay(rc);
    }


    // Transfer a user-mode buffer using DMA
    Cons_printf("  Transfer a user-mode buffer.... ");

    // Allocate a buffer
    pUserBuffer = malloc(0x10);

    // Clear DMA data
    memset(&DmaParams, 0, sizeof(PLX_DMA_PARAMS));

    DmaParams.UserVa    = (PLX_UINT_PTR)pUserBuffer;
    DmaParams.LocalAddr = LocalAddress;
    DmaParams.ByteCount = 0x10;
    DmaParams.Direction = PLX_DMA_LOC_TO_PCI;

    rc =
        PlxPci_DmaTransferUserBuffer(
            pDevice,
            DmaChannel,
            &DmaParams,
            200         // Specify a timeout to let API perform wait
            );

    if (rc == ApiSuccess)
    {
        Cons_printf("Ok\n");
    }
    else
    {
        Cons_printf("*ERROR* - API failed\n");
        PlxSdkErrorDisplay(rc);
    }


    // Release the interrupt wait object
    Cons_printf("  Cancelling Int Notification.... ");
    rc =
        PlxPci_NotificationCancel(
            pDevice,
            &NotifyObject
            );

    if (rc != ApiSuccess)
    {
        Cons_printf("*ERROR* - API failed\n");
        PlxSdkErrorDisplay(rc);
    }
    else
    {
        Cons_printf("Ok\n");
    }

    // Close DMA Channel
    Cons_printf("  Close DMA Channel.............. ");
    rc =
        PlxPci_DmaChannelClose(
            pDevice,
            DmaChannel
            );

    if (rc == ApiSuccess)
    {
        Cons_printf("Ok\n");
    }
    else
    {
        Cons_printf("*ERROR* - API failed\n");
        PlxSdkErrorDisplay(rc);
    }

    // Release user buffer
    if (pUserBuffer != NULL)
        free(pUserBuffer);
}




/********************************************************
 *
 *******************************************************/
void
PerformDma_8000(
    PLX_DEVICE_OBJECT *pDevice
    )
{
    U8                DmaChannel;
    U8               *pUserBuffer = NULL;
    U8               *pDmaBuffer;
    U16               ChannelInput;
    U32               i;
    VOID             *pCommonBuffer;
    PLX_STATUS        status;
    PLX_DMA_PROP      DmaProp;
    PLX_INTERRUPT     PlxInterrupt;
    PLX_DMA_PARAMS    DmaParams;
    PLX_PHYSICAL_MEM  PciBuffer;
    PLX_NOTIFY_OBJECT NotifyObject;


    Cons_printf(
        "Description:\n"
        "     This sample demonstrates usage of the PLX DMA API by\n"
        "     performing DMA transfers and waiting for DMA completion.\n\n"
        );

    Cons_printf("Please select a DMA channel (0-3) --> ");
    Cons_scanf("%hd", &ChannelInput);
    Cons_printf("\n");

    if (ChannelInput >= 4)
    {
        Cons_printf("ERROR: Unsupported DMA channel, test aborted\n");
        return;
    }

    DmaChannel = (U8)ChannelInput;

    // Get DMA buffer parameters
    PlxPci_CommonBufferProperties(
        pDevice,
        &PciBuffer
        );

    // Map DMA buffer
    PlxPci_CommonBufferMap(
        pDevice,
        &pCommonBuffer
        );

    // Assign virtual address
    pDmaBuffer = pCommonBuffer;

    // Open the DMA channel
    Cons_printf("  Open Channel %i for DMA......... ", DmaChannel);
    status =
        PlxPci_DmaChannelOpen(
            pDevice,
            DmaChannel,
            NULL
            );

    if (status == ApiSuccess)
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

    if (status == ApiSuccess)
    {
        Cons_printf("Ok\n");
    }
    else
    {
        Cons_printf("*ERROR* - API failed\n");
        PlxSdkErrorDisplay(status);
    }



    // Set DMA properties
    Cons_printf("  Set DMA propeties.............. ");

    status =
        PlxPci_DmaSetProperties(
            pDevice,
            DmaChannel,
            &DmaProp
            );

    if (status == ApiSuccess)
    {
        Cons_printf("Ok\n");
    }
    else
    {
        Cons_printf("*ERROR* - API failed\n");
        PlxSdkErrorDisplay(status);
    }


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

    if (status != ApiSuccess)
    {
        Cons_printf("*ERROR* - API failed\n");
        PlxSdkErrorDisplay(status);
    }
    else
    {
        Cons_printf( "Ok\n" );
    }

    // Clear DMA buffer
    memset( pDmaBuffer, 0, PciBuffer.Size );

    // Fill the first half of buffer with values
    i = 0;
    while (i < (PciBuffer.Size / 2))
    {
        *(U32*)(pDmaBuffer + i) = ((U32)rand() << 16) | rand();
        i += sizeof(U32);
    }


    /*****************************************
     *
     *          Transfer the Data
     *
     *****************************************/
    Cons_printf("  Perform Block DMA transfer..... ");

    // Clear DMA data
    memset(&DmaParams, 0, sizeof(PLX_DMA_PARAMS));

    DmaParams.AddrSource = PciBuffer.PhysicalAddr;
    DmaParams.AddrDest   = PciBuffer.PhysicalAddr + (PciBuffer.Size / 2);
    DmaParams.ByteCount  = PciBuffer.Size / 2;

    status =
        PlxPci_DmaTransferBlock(
            pDevice,
            DmaChannel,
            &DmaParams,
            0          // Don't wait for completion
            );

    if (status != ApiSuccess)
    {
        Cons_printf("*ERROR* - API failed\n");
        PlxSdkErrorDisplay(status);
    }
    else
    {
        Cons_printf("Ok (%d KB)\n", (DmaParams.ByteCount >> 10));

        Cons_printf("  Wait for interrupt event....... ");

        status =
            PlxPci_NotificationWait(
                pDevice,
                &NotifyObject,
                5 * 1000
                );

        switch (status)
        {
            case ApiSuccess:
                Cons_printf("Ok (DMA Int received)\n");

                /*****************************************
                 *
                 *          Compare the Data
                 *
                 *****************************************/
                Cons_printf("  Compare Data................... ");
                for (i=0; i<(PciBuffer.Size/2); i+=sizeof(U32))
                {
                    if (*(U32*)(pDmaBuffer + i) !=
                        *(U32*)(pDmaBuffer + PciBuffer.Size/2 + i))
                    {
                        Cons_printf(
                            "\n\tERR - Wrote: %08xh   Read: %08xh",
                            *(U32*)(pDmaBuffer + i),
                            *(U32*)(pDmaBuffer + PciBuffer.Size/2 + i)
                            );

                        _Pause;
                    }
                }
                Cons_printf("Ok\n");
                break;

            case ApiWaitTimeout:
                Cons_printf("*ERROR* - Timeout waiting for Int Event\n");
                break;

            case ApiWaitCanceled:
                Cons_printf("*ERROR* - Interrupt event cancelled\n");
                break;

            default:
                Cons_printf("*ERROR* - API failed\n");
                PlxSdkErrorDisplay(status);
                break;
        }
    }


    Cons_printf("  Check notification status...... ");
    status =
        PlxPci_NotificationStatus(
            pDevice,
            &NotifyObject,
            &PlxInterrupt
            );

    if (status == ApiSuccess)
    {
        Cons_printf("Ok (triggered ints:");

        if (PlxInterrupt.DmaDone & (1 << 0))
            Cons_printf(" DMA_0");

        if (PlxInterrupt.DmaDone & (1 << 1))
            Cons_printf(" DMA_1");

        if (PlxInterrupt.DmaDone & (1 << 2))
            Cons_printf(" DMA_2");

        if (PlxInterrupt.DmaDone & (1 << 3))
            Cons_printf(" DMA_3");

        Cons_printf(")\n");
    }
    else
    {
        Cons_printf("*ERROR* - API failed\n");
        PlxSdkErrorDisplay(status);
    }

    // Transfer a user-mode buffer using DMA
    Cons_printf("  Transfer a user-mode buffer.... ");

    // Allocate a buffer
    if (pUserBuffer == NULL)
        pUserBuffer = malloc(PciBuffer.Size);

    // Clear DMA data
    memset(&DmaParams, 0, sizeof(PLX_DMA_PARAMS));

    DmaParams.UserVa    = (PLX_UINT_PTR)pUserBuffer;
    DmaParams.PciAddr   = PciBuffer.PhysicalAddr;
    DmaParams.ByteCount = PciBuffer.Size;
    DmaParams.Direction = PLX_DMA_PCI_TO_USER;

    status =
        PlxPci_DmaTransferUserBuffer(
            pDevice,
            DmaChannel,
            &DmaParams,
            0
            );

    if (status != ApiSuccess)
    {
        Cons_printf("*ERROR* - API failed\n");
        PlxSdkErrorDisplay(status);
    }
    else
    {
        Cons_printf("Ok (%d KB)\n", (DmaParams.ByteCount >> 10));

        Cons_printf("  Wait for interrupt event....... ");

        status =
            PlxPci_NotificationWait(
                pDevice,
                &NotifyObject,
                5 * 1000
                );

        switch (status)
        {
            case ApiSuccess:
                Cons_printf("Ok (DMA Int received)\n");

                /*****************************************
                 *
                 *          Compare the Data
                 *
                 *****************************************/
                Cons_printf("  Compare Data................... ");
                for (i=0; i<PciBuffer.Size; i+=sizeof(U32))
                {
                    if (*(U32*)(pUserBuffer + i) !=
                        *(U32*)(pDmaBuffer + i))
                    {
                        Cons_printf(
                            "\n\tERR - Wrote: %08xh   Read: %08xh",
                            *(U32*)(pDmaBuffer + i),
                            *(U32*)(pUserBuffer + i)
                            );

                        _Pause;
                    }
                }
                Cons_printf("Ok\n");
                break;

            case ApiWaitTimeout:
                Cons_printf("*ERROR* - Timeout waiting for Int Event\n");
                break;

            case ApiWaitCanceled:
                Cons_printf("*ERROR* - Interrupt event cancelled\n");
                break;

            default:
                Cons_printf("*ERROR* - API failed\n");
                PlxSdkErrorDisplay(status);
                break;
        }
    }

    // Release the interrupt wait object
    Cons_printf("  Cancelling Int Notification.... ");
    status =
        PlxPci_NotificationCancel(
            pDevice,
            &NotifyObject
            );

    if (status != ApiSuccess)
    {
        Cons_printf("*ERROR* - API failed\n");
        PlxSdkErrorDisplay(status);
    }
    else
    {
        Cons_printf("Ok\n");
    }

    // Close DMA Channel
    Cons_printf("  Close DMA Channel.............. ");
    status =
        PlxPci_DmaChannelClose(
            pDevice,
            DmaChannel
            );

    if (status == ApiSuccess)
    {
        Cons_printf("Ok\n");
    }
    else
    {
        Cons_printf("*ERROR* - API failed\n");
        PlxSdkErrorDisplay(status);
    }

    // Release user buffer
    if (pUserBuffer != NULL)
        free(pUserBuffer);
}
