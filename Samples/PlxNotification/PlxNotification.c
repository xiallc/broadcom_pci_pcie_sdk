/******************************************************************************
 *
 * File Name:
 *
 *      PlxNotification.c
 *
 * Description:
 *
 *      Demonstrates the PlxNotificationXxx API for interrupt handling
 *
 * Revision History:
 *
 *      12-01-07 : PLX SDK v5.20
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
TestAttachLocalInt(
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
        "\t\t   PLX Interrupt Notification Sample Application\n"
        "\t\t                August 2007\n\n"
        );


    /************************************
     *         Select Device
     ***********************************/
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



    /************************************
     *      Verify chip is supported
     ***********************************/

    switch (DeviceKey.PlxChip)
    {
        case 0x9030:
        case 0x9050:
            break;

        case 0x9080:
        case 0x9054:
        case 0x9056:
        case 0x9656:
        case 0x8311:
            Cons_printf(
                "\nERROR: For PLX %04X chips, please refer to the PLX DMA sample\n",
                DeviceKey.PlxChip
                );
            goto _Exit_App;

        default:
            Cons_printf(
                "\nERROR: Undefined or unsupported PLX Chip type (%04X)\n",
                DeviceKey.PlxChip
                );
            goto _Exit_App;
    }



    /************************************
     *        Perform the Test
     ***********************************/
    TestAttachLocalInt(
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
TestAttachLocalInt(
    PLX_DEVICE_OBJECT *pDevice
    )
{
    U16               OffsetRegInt;
    U32               RegValue;
    PLX_INTERRUPT     PlxInterrupt;
    PLX_STATUS        rc;
    PLX_NOTIFY_OBJECT NotifyObject;


    Cons_printf(
        "Description:\n"
        "     This sample will test the Interrupt Attach API by manually triggering\n"
        "     the Software interrupt of the PLX chip.  This is a feature of the PLX\n"
        "     chip which forces the device to generate a generic PCI interrupt.\n"
        "\n\n"
        );

    _Pause;

    switch (pDevice->Key.PlxChip)
    {
        case 0x9030:
        case 0x9050:
            OffsetRegInt = 0x4C;        // Interrupt Control/Status register
            break;

        default:
            Cons_printf(
                "\nERROR:  Unsupported PLX Chip type (%04X)\n",
                pDevice->Key.PlxChip
                );
            return;
    }

    // Clear Interrupt fields
    memset(&PlxInterrupt, 0, sizeof(PLX_INTERRUPT));

    Cons_printf("  Enable the PCI Interrupt........... ");
    PlxInterrupt.PciMain = 1;
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
        Cons_printf("Ok\n");


    Cons_printf("  Register for Int. notification..... ");

    // Clear interrupt fields
    memset(&PlxInterrupt, 0, sizeof(PLX_INTERRUPT));

    // Setup to wait for the Software interrupt
    PlxInterrupt.SwInterrupt = 1;
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


    // Manually trigger interrupt
    Cons_printf("  Trigger & wait for Interrupt....... ");

    // Delay a bit before triggering interrupt
    Plx_sleep(500);

    // Get Interrupt Control/Status register
    RegValue =
        PlxPci_PlxRegisterRead(
            pDevice,
            OffsetRegInt,
            NULL
            );

    // Manually trigger Software interrupt
    PlxPci_PlxRegisterWrite(
        pDevice,
        OffsetRegInt,
        RegValue | (1 << 7)
        );

    // Wait for interrupt event
    rc =
        PlxPci_NotificationWait(
            pDevice,
            &NotifyObject,
            10 * 1000
            );

    switch (rc)
    {
        case ApiSuccess:
            Cons_printf("Ok (Software Int received)\n");
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

    // Release the interrupt wait object
    Cons_printf("  Cancelling Int Notification........ ");
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
