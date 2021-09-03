/******************************************************************************
 *
 * File Name:
 *
 *      LocalToPciInt.c
 *
 * Description:
 *
 *      Demonstrates using the PLX API to wait for Local-to-PCI interrupt
 *
 * Revision History:
 *
 *      01-01-08 : PLX SDK v5.20
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
void
InterruptTest(
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
        "\t\t    PLX Local->PCI Interrupt Sample Application\n"
        "\t\t                 January 2008\n\n"
        );


    /************************************
    *         Select Device
    ************************************/
    DeviceSelected =
        SelectDevice(
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

    // Verify chip is supported
    switch (DeviceKey.PlxChip)
    {
        case 0x9050:
        case 0x9030:
        case 0x9080:
        case 0x9054:
        case 0x9056:
        case 0x9656:
        case 0x8311:
            break;

        default:
            Cons_printf(
                "ERROR: Device (%04X) does not support generic Local-to-PCI interrupt\n",
                DeviceKey.PlxChip
                );
            goto _Exit_App;
    }


    /************************************
     *        Perform the DMA
     ************************************/
    InterruptTest(
        &Device
        );


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




/********************************************************
 *
 *******************************************************/
void
InterruptTest(
    PLX_DEVICE_OBJECT *pDevice
    )
{
    PLX_STATUS        rc;
    PLX_INTERRUPT     PlxInterrupt;
    PLX_NOTIFY_OBJECT NotifyObject;


    Cons_printf(
        "Description:\n"
        "     This sample demonstrates how to use the PLX API for\n"
        "     notification of the generic Local->PCI interrupts (e.g. LINTi#).\n"
        "     The interrupt trigger must be initiated by the\n"
        "     OEM hardware.  PLX software is not able to manually\n"
        "     assert the interrupt since its source is OEM-dependent.\n"
        "\n"
        "             Press any key to start...\n"
        "\n"
        );  

    Cons_getch();


    Cons_printf("  Register for notification...... ");

    // Clear interrupt fields
    memset(&PlxInterrupt, 0, sizeof(PLX_INTERRUPT));

    // Setup to wait for either generic interrupt
    PlxInterrupt.LocalToPci = (1 << 0) | (1 << 1);

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


    /***********************************************************
     * This loop will loop only one time.  It is provided here to
     * demonstrate which code should be placed within the loop
     * if waiting for the interrupt multiple times.
     * Note that the generic L->P interrupt must constantly be
     * re-enabled since the PLX driver will mask it.
     **********************************************************/
    do
    {
        Cons_printf("  Enable interrupt(s)............ ");
        rc =
            PlxPci_InterruptEnable(
                pDevice,
                &PlxInterrupt
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



        Cons_printf(
            "\n"
            "     -- You may now trigger the interrupt --\n"
            "\n"
            );



        Cons_printf("  Wait for interrupt event....... ");

        rc =
            PlxPci_NotificationWait(
                pDevice,
                &NotifyObject,
                120 * 1000
                );

        switch (rc)
        {
            case ApiSuccess:
                Cons_printf("Ok (Int received)\n");

                /************************************************
                 *
                 * Custom code to clear the OEM source of the
                 * interrupt should be placed here.  Another option
                 * is to modify the PLX driver interrupt handler
                 * to perform the souce clear.  In that case, it
                 * will not constantly need to be masked/re-enabled.
                 *
                 ***********************************************/
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

            if (PlxInterrupt.LocalToPci & (1 << 0))
                Cons_printf(" L->P 1");

            if (PlxInterrupt.LocalToPci & (1 << 1))
                Cons_printf(" L->P 2");

            Cons_printf(")\n");
        }
        else
        {
            Cons_printf("*ERROR* - API failed\n");
            PlxSdkErrorDisplay(rc);
        }
    }
    while (0);


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
}
