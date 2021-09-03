/******************************************************************************
 *
 * File Name:
 *
 *      PlxDmaSglNoApi.c
 *
 * Description:
 *
 *      This sample demonstrates how to manually setup an SGL DMA transfer.
 *
 * Revision History:
 *
 *      12-01-07 : PLX SDK v5.20
 *
 ******************************************************************************/


/******************************************************************************
 *
 * NOTE:
 *
 * This sample is provided to demonstrate the setup of a simple SGL DMA
 * transfer using the PLX API.  The PLX API is essentially bypassed except for
 * calls to access the provided PLX DMA buffer.  The SGL DMA is manually
 * setup and controlled by the application.
 *
 * Since PCI system memory is needed, the Common Buffer allocated by the PLX
 * driver is used.  The virtual address, physical address, and size of this
 * buffer are easily available.  The buffer is also contiguous in memory and
 * is reserved for use by PLX applications.  The PLX driver itself does not use
 * the buffer.  These properties make it a perfect candidate for DMA transfers.
 *
 * Users will notice that the code presented here is somewhat complex.  This
 * is due to the fact that the PLX chip must be accessed at a register-level,
 * rather than at the API level.  Some portability of code is lost and the code
 * will not be as easily maintainable.
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
BOOLEAN
SetupDmaDescriptors(
    PLX_DEVICE_OBJECT *pDevice,
    U32               *pSglPciAddress
    );

VOID
PerformSglDmaTransfer(
    PLX_DEVICE_OBJECT *pDevice,
    U32                SglPciAddress
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
    S8                DeviceSelected;
    U32               SglPciAddress;
    PLX_STATUS        rc;
    PLX_DEVICE_KEY    DeviceKey;
    PLX_DEVICE_OBJECT Device;


    ConsoleInitialize();

    Cons_clear();

    Cons_printf(
        "\n\n"
        "\t\t        PLX SGL Sample Application\n"
        "\t\t                January 2007\n\n"
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
        "\nSelected: %04x %04x [b:%02x  s:%02x  f:%02x]\n\n",
        DeviceKey.DeviceId, DeviceKey.VendorId,
        DeviceKey.bus, DeviceKey.slot, DeviceKey.function
        );

    // Verify chip supports DMA
    switch (DeviceKey.PlxChip)
    {
        case 0x9080:
        case 0x9054:
        case 0x9056:
        case 0x9656:
        case 0x8311:
            break;

        default:
            Cons_printf(
                "ERROR: DMA not supported by the selected device (%04X)\n",
                DeviceKey.PlxChip
                );
            goto _Exit_App;
    }



    /*******************************************************
     * First, setup The DMA descriptors in PCI memory
     ******************************************************/
    if (SetupDmaDescriptors(
            &Device,
            &SglPciAddress
            ) == FALSE)
    {
        goto _Exit_App;
    }



    /*******************************************************
     * Next, setup the DMA channel to transfer the SGL
     ******************************************************/
    PerformSglDmaTransfer(
        &Device,
        SglPciAddress
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




/******************************************************************************
 *
 * Function   :  SetupDmaDescriptors
 *
 * Description:  This function sets up DMA SGL descriptors in the DMA common
 *               buffer provided by the PLX driver.  The goal here is to
 *               transfer 2 blocks of data, resulting in the need for 2
 *               SGL descriptors.  The descriptors are placed at the start
 *               of the common buffer, followed by the buffers for the data.
 *
 *               After this function completes, the common buffer should look
 *               similar to the following:
 *
 *
 *              Offset  __________________________
 *                     |                          |
 *                00h  |     First Descriptor     |
 *                     |        (16 bytes)        |
 *                     |--------------------------|
 *                     |                          |
 *                10h  |     Second Decriptor     |
 *                     |        (16 bytes)        |
 *                     |--------------------------|
 *                     |                          |
 *                      \/\/\/\/\/\/\/\/\/\/\/\/\/
 *
 *                      /\/\/\/\/\/\/\/\/\/\/\/\/\
 *                     |                          |
 *               100h  |     First data buffer    |
 *                     |        (256 bytes)       |
 *                     |--------------------------|
 *                     |                          |
 *               200h  |    Second data buffer    |
 *                     |        (256 bytes)       |
 *                     |__________________________| 
 *
 *****************************************************************************/
BOOLEAN
SetupDmaDescriptors(
    PLX_DEVICE_OBJECT *pDevice,
    U32               *pSglPciAddress
    )
{
    U8               *pBuffer;
    U32               LocalAddress;
    VOID             *pDmaBuffer;
    PLX_STATUS        rc;
    PLX_PHYSICAL_MEM  PciBuffer;


    /************************************************
     *
     *            Get a local address
     *
     ***********************************************/
    Cons_printf(
        "Description:\n"
        "     This sample will demonstrate a manual SGL DMA transfer.\n"
        "     It will transfer 2 blocks of data from the common buffer\n"
        "     using DMA chaining and wait for the DMA interrupt.\n"
        );

    Cons_printf(
        "\n"
        " WARNING: There is no safeguard mechanism to protect against invalid\n"
        "          local bus addresses.  Please be careful when selecting local\n"
        "          addresses to transfer data to/from.  System crashes will result\n"
        "          if an invalid address is accessed.\n"
        "\n\n"
        );

    Cons_printf("Please enter a valid local address --> ");
    Cons_scanf("%x", &LocalAddress);
    Cons_printf("\n");



    /************************************************
     *
     *         Get DMA buffer properties
     *
     ***********************************************/
    Cons_printf("  Get DMA buffer properties...... ");

    rc =
        PlxPci_CommonBufferProperties(
            pDevice,
            &PciBuffer
            );

    if (rc != ApiSuccess)
    {
        *pSglPciAddress = 0;
        Cons_printf("*ERROR* - API failed\n");
        PlxSdkErrorDisplay(rc);
        return FALSE;
    }
    Cons_printf("Ok (PCI Addr=%08x)\n", PciBuffer.PhysicalAddr);


    Cons_printf("  Map DMA buffer to user space... ");
    rc =
        PlxPci_CommonBufferMap(
            pDevice,
            &pDmaBuffer
            );

    if (rc != ApiSuccess)
    {
        *pSglPciAddress = 0;
        Cons_printf("*ERROR* - API failed\n");
        PlxSdkErrorDisplay(rc);
        return FALSE;
    }
    Cons_printf("Ok (VA=%p)\n", pDmaBuffer);

    // Set starting address of buffer
    pBuffer = pDmaBuffer;

    // Align buffer address to 16 byte boundary (required by DMA engine)
    while (PciBuffer.PhysicalAddr & 0xF)
    {
        pBuffer                += 1;
        PciBuffer.PhysicalAddr += 1;
    }



    /************************************************
     *
     *          Write first DMA descriptor
     *
     ***********************************************/
    Cons_printf("  Write first DMA Descriptor..... ");

    // PCI physical address of data buffer
    *(U32*)(pBuffer + 0x0) = PLX_LE_DATA_32((U32)PciBuffer.PhysicalAddr + 0x100);

    // Local address
    *(U32*)(pBuffer + 0x4) = PLX_LE_DATA_32(LocalAddress);

    // Number of bytes to transfer
    *(U32*)(pBuffer + 0x8) = PLX_LE_DATA_32(0x100);

    // Next Desc Address & Local->PCI & Desc in PCI space
    *(U32*)(pBuffer + 0xC) =
        PLX_LE_DATA_32(
            (U32)(PciBuffer.PhysicalAddr + 0x10) |
            (1 << 3) | (1 << 0)
            );

    Cons_printf("Ok\n");


    // Increment to next descriptor
    pBuffer += (4 * sizeof(U32));



    /************************************************
     *
     *          Write second DMA descriptor
     *
     ***********************************************/
    Cons_printf("  Write second DMA Descriptor.... ");

    // PCI physical address of data buffer
    *(U32*)(pBuffer + 0x0) = PLX_LE_DATA_32((U32)PciBuffer.PhysicalAddr + 0x200);

    // Local address
    *(U32*)(pBuffer + 0x4) = PLX_LE_DATA_32(LocalAddress + 0x100);

    // Number of bytes to transfer
    *(U32*)(pBuffer + 0x8) = PLX_LE_DATA_32(0x100);

    // Next Desc Address & Local->PCI & End of chain & Desc in PCI space
    *(U32*)(pBuffer + 0xC) =
        PLX_LE_DATA_32(
            0x0 | (1 << 3) | (1 << 1) | (1 << 0)
            );

    Cons_printf("Ok\n");
   

    // Provide PCI address of first descriptor
    *pSglPciAddress = (U32)PciBuffer.PhysicalAddr;


    Cons_printf("  Unmap DMA buffer............... ");
    rc =
        PlxPci_CommonBufferUnmap(
            pDevice,
            (VOID**)&pDmaBuffer
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

    return TRUE;
}




/******************************************************************************
 *
 * Function   :  PerformSglDmaTransfer
 *
 * Description:  Initiates the SGL DMA transfer and waits for completion
 *
 *****************************************************************************/
VOID
PerformSglDmaTransfer(
    PLX_DEVICE_OBJECT *pDevice,
    U32                SglPciAddress
    )
{
    U32               RegValue;
    PLX_STATUS        rc;
    PLX_INTERRUPT     PlxInterrupt;
    PLX_NOTIFY_OBJECT NotifyObject;


    /**********************************************
     * Set DMA mode to:
     *
     * - 32-bit bus
     * - Ready Input
     * - DMA Chaining
     * - DMA Done interrupt enabled
     * - Route DMA interrupt to PCI
     *********************************************/
    PlxPci_PlxRegisterWrite(
        pDevice,
        0x80,             // DMA 0 Mode offset
        0x00020642
        );


    /************************************************
     *
     *    Register for DMA interrupt notification
     *
     ***********************************************/
    Cons_printf("  Register for DMA int notify.... ");

    // Clear interrupt fields
    memset(&PlxInterrupt, 0, sizeof(PLX_INTERRUPT));

    PlxInterrupt.DmaDone = (1 << 0);
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



    /************************************************
     *
     *       Enable the DMA and PCI interrupts
     *
     ***********************************************/
    Cons_printf("  Enable DMA 0 & PCI interrupts.. ");

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



    /************************************************
     *
     *          Initialize DMA Next Descriptor
     *
     ***********************************************/
    // Write address of first desc & desc in PCI space (bit 0)
    PlxPci_PlxRegisterWrite(
        pDevice,
        0x90,               // DMA 0 Descriptor pointer offset
        SglPciAddress | (1 << 0)
        );



    /************************************************
     *
     *              Start DMA engine
     *
     ***********************************************/
    Cons_printf("  Start DMA transfer............. ");
    RegValue =
        PlxPci_PlxRegisterRead(
            pDevice,
            0xA8,       // DMA command/status
            NULL
            );

    // First enable the DMA channel
    RegValue |= (1 << 0);

    PlxPci_PlxRegisterWrite(
        pDevice,
        0xA8,       // DMA command/status
        RegValue
        );

    // Start the transfer
    RegValue |= (1 << 1);

    PlxPci_PlxRegisterWrite(
        pDevice,
        0xA8,       // DMA command/status
        RegValue
        );

    Cons_printf("Ok\n");



    /************************************************
     *
     *           Wait for DMA completion
     *
     ***********************************************/
    Cons_printf("  Wait for interrupt event....... ");

    rc =
        PlxPci_NotificationWait(
            pDevice,
            &NotifyObject,
            10 * 1000
            );

    switch (rc)
    {
        case ApiSuccess:
            Cons_printf("Ok (DMA 0 Int received)\n");
            break;

        case ApiWaitTimeout:
            Cons_printf("*ERROR* - Timeout waiting for interrupt\n");
            break;

        case ApiWaitCanceled:
            Cons_printf("*ERROR* - Interrupt event cancelled\n");
            break;

        default:
            Cons_printf(
                "*ERROR* - API failed (rc=%s)\n",
                PlxSdkErrorText(rc)
                );
            break;
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
        Cons_printf(
            "*ERROR* - rc=%s\n",
            PlxSdkErrorText(rc)
            );
    }
    else
    {
        Cons_printf("Ok\n");
    }
}
