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
 *      PlxDmaSglNoApi.c
 *
 * Description:
 *
 *      This sample demonstrates how to manually setup an SGL DMA transfer.
 *
 * Revision History:
 *
 *      10-01-12 : PLX SDK v7.00
 *
 ******************************************************************************/


/******************************************************************************
 *
 * NOTE:
 *
 * This sample is provided to demonstrate the setup of a simple SGL DMA
 * transfer using the PLX API. The PLX API is essentially bypassed except for
 * calls to access the provided PLX DMA buffer. The SGL DMA is manually
 * setup and controlled by the application.
 *
 * Since PCI system memory is needed, the Common Buffer allocated by the PLX
 * driver is used.  The virtual address, physical address, and size of this
 * buffer are easily available.  The buffer is also contiguous in memory and
 * is reserved for use by PLX applications.
 *
 * Users will notice that the code presented here is not as elegant compared to
 * using PLX API functions. This is a result of the need to access the PLX chip
 * at a register-level rather than at the API level. Some portability of code is
 * lost and the code will not be as easily maintainable.
 ******************************************************************************/


#include <time.h>   // For time() function
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

PLX_STATUS
SetupDmaDescriptors_9000(
    PLX_DEVICE_OBJECT *pDevice,
    U8                *pDmaBuffer,
    U32               *pSglPciAddress
    );

PLX_STATUS
PerformSglDmaTransfer_9000(
    PLX_DEVICE_OBJECT *pDevice,
    U8                *pDmaBuffer,
    U32                SglPciAddress
    );

PLX_STATUS
SetupDmaDescriptors_8000(
    PLX_DEVICE_OBJECT *pDevice,
    U8                *pDmaBuffer,
    U64               *pSglPciAddress
    );

PLX_STATUS
PerformSglDmaTransfer_8000(
    PLX_DEVICE_OBJECT *pDevice,
    U8                *pDmaBuffer,
    U64                SglPciAddress
    );

VOID
GenerateRandomData(
    U8  *pBuffer,
    U32  ByteCount
    );

U32
CompareBuffers(
    U8  *pBuffer_1,
    U8  *pBuffer_2,
    U32  ByteCount
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
    U8                bLegacyDma;
    U8               *pDmaBuffer;
    S16               DeviceSelected;
    U32               SglPciAddr_32;
    U64               SglPciAddr_64;
    PLX_STATUS        status;
    PLX_DEVICE_KEY    DeviceKey;
    PLX_DEVICE_OBJECT Device;


    ConsoleInitialize();

    Cons_clear();

    Cons_printf(
        "                                                    _______________________ \n"
        "    PLX Manual SGL Sample Application          00h |   Descriptor 0 (16B)  |\n"
        "                                                   |-----------------------|\n"
        "  This sample demonstrates manual setup of     10h |   Descriptor 1 (16B)  |\n"
        "  DMA SGL descriptors in the PLX DMA buffer.       |-----------------------|\n"
        "  There will be 2 descriptors for 2 data           |         ...           |\n"
        "  data blocks. The descriptors are placed          |-----------------------|\n"
        "  at the start of the DMA buffer, followed    100h |  Buffer 0 (9000 DMA)  |\n"
        "  by the buffers for the data.                     |  Source 0 (8000 DMA)  |\n"
        "                              __________|\\         |-----------------------|\n"
        "                             |             \\  200h |  Buffer 1 (9000 DMA)  |\n"
        "  The DMA will resemble this |__________   /       |  Source 1 (8000 DMA)  |\n"
        "                                        |/         |-----------------------|\n"
        "                                              300h |   Dest 0 (8000 DMA)   |\n"
        "                                                   |                       |\n"
        "                                                   |-----------------------|\n"
        "                                              400h |   Dest 1 (8000 DMA)   |\n"
        "                                                   |                       |\n"
        "                                                   |_______________________|\n"
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

    status =
        PlxPci_DeviceOpen(
            &DeviceKey,
            &Device
            );

    if (status != PLX_STATUS_OK)
    {
        Cons_printf("\n   ERROR: Unable to find or select a PLX device\n");
        PlxSdkErrorDisplay(status);
        _Pause;
        ConsoleEnd();
        exit(-1);
    }

    Cons_printf(
        "\nSelected: %04x %04x [b:%02x s:%02x f:%x]\n\n",
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
            bLegacyDma = TRUE;
            break;

        default:
            if ((DeviceKey.PlxChip & 0xF000) == 0x8000)
                bLegacyDma = FALSE;
            else
            {
                Cons_printf(
                    "ERROR: DMA not supported by the selected device (%04X)\n",
                    DeviceKey.PlxChip
                    );
                goto _Exit_App;
            }
            break;
    }



    /*******************************************************
     * Map the DMA buffer to user space
     ******************************************************/
    Cons_printf("  Map DMA buffer to user space... ");
    status = PlxPci_CommonBufferMap( &Device, (VOID**)&pDmaBuffer );
    if (status != PLX_STATUS_OK)
    {
        Cons_printf("*ERROR* - API failed\n");
        PlxSdkErrorDisplay(status);
        goto _Exit_App;
    }
    Cons_printf("Ok (VA=%p)\n", pDmaBuffer);



    /*******************************************************
     * Setup DMA descriptors in PCI memory
     ******************************************************/
    if (bLegacyDma)
        status = SetupDmaDescriptors_9000( &Device, pDmaBuffer, &SglPciAddr_32 );
    else
        status = SetupDmaDescriptors_8000( &Device, pDmaBuffer, &SglPciAddr_64 );

    if (status != PLX_STATUS_OK)
    {
        Cons_printf("\n   ERROR: Unable to setup DMA descriptors\n");
        PlxSdkErrorDisplay(status);
        goto _Exit_App;
    }



    /*******************************************************
     * Setup the DMA channel & transfer the SGL
     ******************************************************/
    if (bLegacyDma)
        status = PerformSglDmaTransfer_9000( &Device, pDmaBuffer, SglPciAddr_32 );
    else
        status = PerformSglDmaTransfer_8000( &Device, pDmaBuffer, SglPciAddr_64 );

    if (status != PLX_STATUS_OK)
    {
        Cons_printf("\n   ERROR: Unable to perform SGL DMA\n");
        PlxSdkErrorDisplay(status);
        goto _Exit_App;
    }



    /*******************************************************
     * Unmap the DMA buffer from user space
     ******************************************************/
    Cons_printf("  Unmap DMA buffer............... ");
    status = PlxPci_CommonBufferUnmap( &Device, (VOID**)&pDmaBuffer );
    if (status != PLX_STATUS_OK)
    {
        Cons_printf("*ERROR* - API failed\n");
        PlxSdkErrorDisplay(status);
        goto _Exit_App;
    }
    Cons_printf("Ok\n");



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

        if (status == PLX_STATUS_OK)
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

    return (S16)NumDevices;
}




/******************************************************************************
 *
 * Function   :  SetupDmaDescriptors_9000
 *
 * Description:  Sets up DMA SGL descriptors in the DMA buffer for 9000 DMA
 *
 *****************************************************************************/
PLX_STATUS
SetupDmaDescriptors_9000(
    PLX_DEVICE_OBJECT *pDevice,
    U8                *pDmaBuffer,
    U32               *pSglPciAddress
    )
{
    U8               *pSgl;
    U32               LocalAddress;
    PLX_STATUS        status;
    PLX_PHYSICAL_MEM  PciBuffer;


    /************************************************
     *
     *            Get a local address
     *
     ***********************************************/
    Cons_printf(
        "\n"
        " WARNING: Local bus addresses are not verified for validity\n"
        "\n"
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

    status = PlxPci_CommonBufferProperties( pDevice, &PciBuffer );
    if (status != PLX_STATUS_OK)
    {
        *pSglPciAddress = 0;
        Cons_printf("*ERROR* - API failed\n");
        PlxSdkErrorDisplay(status);
        return status;
    }
    Cons_printf("Ok (PCI Addr=%08x)\n", (U32)PciBuffer.PhysicalAddr);


    // Set starting address of buffer
    pSgl = pDmaBuffer;

    // Clear buffer region
    memset( pSgl, 0, 0x500 );

    // Align SGL start address to 16 byte boundary (required by DMA engine)
    while (PciBuffer.PhysicalAddr & 0xF)
    {
        pSgl                   += 1;
        PciBuffer.PhysicalAddr += 1;
    }



    /************************************************
     *
     *          Write first DMA descriptor
     *
     ***********************************************/
    Cons_printf("  Write first DMA Descriptor..... ");

    // PCI physical address of data buffer
    *(U32*)(pSgl + 0x0) = PLX_LE_DATA_32((U32)PciBuffer.PhysicalAddr + 0x100);

    // Local address
    *(U32*)(pSgl + 0x4) = PLX_LE_DATA_32(LocalAddress);

    // Number of bytes to transfer
    *(U32*)(pSgl + 0x8) = PLX_LE_DATA_32(0x100);

    // Next Desc Address & Local->PCI & Desc in PCI space
    *(U32*)(pSgl + 0xC) =
        PLX_LE_DATA_32(
            (U32)(PciBuffer.PhysicalAddr + 0x10) |
            (1 << 3) | (1 << 0)
            );

    Cons_printf("Ok\n");


    // Increment to next descriptor
    pSgl += (4 * sizeof(U32));



    /************************************************
     *
     *          Write second DMA descriptor
     *
     ***********************************************/
    Cons_printf("  Write second DMA Descriptor.... ");

    // PCI physical address of data buffer
    *(U32*)(pSgl + 0x0) = PLX_LE_DATA_32((U32)PciBuffer.PhysicalAddr + 0x200);

    // Local address
    *(U32*)(pSgl + 0x4) = PLX_LE_DATA_32(LocalAddress + 0x100);

    // Number of bytes to transfer
    *(U32*)(pSgl + 0x8) = PLX_LE_DATA_32(0x100);

    // Next Desc Address & Local->PCI & End of chain & Desc in PCI space
    *(U32*)(pSgl + 0xC) =
        PLX_LE_DATA_32(
            0x0 | (1 << 3) | (1 << 1) | (1 << 0)
            );

    Cons_printf("Ok\n");
   

    // Provide PCI address of first descriptor
    *pSglPciAddress = (U32)PciBuffer.PhysicalAddr;

    return PLX_STATUS_OK;
}




/******************************************************************************
 *
 * Function   :  PerformSglDmaTransfer_9000
 *
 * Description:  Initiates the 9000 SGL DMA transfer and waits for completion
 *
 *****************************************************************************/
PLX_STATUS
PerformSglDmaTransfer_9000(
    PLX_DEVICE_OBJECT *pDevice,
    U8                *pDmaBuffer,
    U32                SglPciAddress
    )
{
    U32               RegValue;
    PLX_STATUS        status;
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
    status = PlxPci_NotificationRegisterFor( pDevice, &PlxInterrupt, &NotifyObject );
    if (status != PLX_STATUS_OK)
    {
        Cons_printf("*ERROR* - API failed\n");
        PlxSdkErrorDisplay(status);
        return status;
    }
    Cons_printf("Ok\n");



    /************************************************
     *
     *       Enable the DMA and PCI interrupts
     *
     ***********************************************/
    Cons_printf("  Enable DMA 0 & PCI interrupts.. ");

    PlxInterrupt.PciMain = 1;
    status = PlxPci_InterruptEnable( pDevice, &PlxInterrupt );
    if (status != PLX_STATUS_OK)
    {
        Cons_printf("*ERROR* - API failed\n");
        PlxSdkErrorDisplay(status);
        return status;
    }
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

    // Get DMA command/status
    RegValue = PlxPci_PlxRegisterRead( pDevice, 0xA8, NULL );

    // Enable the DMA channel
    RegValue |= (1 << 0);
    PlxPci_PlxRegisterWrite( pDevice, 0xA8, RegValue );

    // Start the transfer
    RegValue |= (1 << 1);
    PlxPci_PlxRegisterWrite( pDevice, 0xA8, RegValue );

    Cons_printf("Ok\n");



    /************************************************
     *
     *           Wait for DMA completion
     *
     ***********************************************/
    Cons_printf("  Wait for interrupt event....... ");

    status = PlxPci_NotificationWait( pDevice, &NotifyObject, 10 * 1000 );

    switch (status)
    {
        case PLX_STATUS_OK:
            Cons_printf("Ok (DMA 0 Int received)\n");
            break;

        case PLX_STATUS_TIMEOUT:
            Cons_printf("*ERROR* - Timeout waiting for interrupt\n");
            break;

        case PLX_STATUS_CANCELED:
            Cons_printf("*ERROR* - Interrupt event cancelled\n");
            break;

        default:
            Cons_printf("*ERROR* - API failed (status=%s)\n", PlxSdkErrorText(status));
            break;
    }

    // Release the interrupt wait object
    Cons_printf("  Cancel Int Notification........ ");
    status = PlxPci_NotificationCancel( pDevice, &NotifyObject );

    if (status != PLX_STATUS_OK)
    {
        Cons_printf("*ERROR* - status=%s\n", PlxSdkErrorText(status));
        return status;
    }
    Cons_printf("Ok\n");

    return PLX_STATUS_OK;
}




/******************************************************************************
 *
 * Function   :  SetupDmaDescriptors_8000
 *
 * Description:  Sets up DMA SGL descriptors in the DMA buffer for 8000 DMA
 *
 *****************************************************************************/
PLX_STATUS
SetupDmaDescriptors_8000(
    PLX_DEVICE_OBJECT *pDevice,
    U8                *pDmaBuffer,
    U64               *pSglPciAddress
    )
{
    U8               *pSgl;
    U32               TmpValue;
    U64               AddrSrc;
    U64               AddrDest;
    U64               MaskBits64;
    PLX_STATUS        status;
    PLX_PHYSICAL_MEM  PciBuffer;


    /************************************************
     *
     *         Get DMA buffer properties
     *
     ***********************************************/
    Cons_printf("  Get DMA buffer properties...... ");

    status = PlxPci_CommonBufferProperties( pDevice, &PciBuffer );
    if (status != PLX_STATUS_OK)
    {
        *pSglPciAddress = 0;
        Cons_printf("*ERROR* - API failed\n");
        PlxSdkErrorDisplay(status);
        return status;
    }
    Cons_printf(
        "Ok (PCI Addr=%08x_%08x)\n",
        PLX_64_HIGH_32(PciBuffer.PhysicalAddr),
        PLX_64_LOW_32(PciBuffer.PhysicalAddr)
        );

    // Calculate a mask to check if extended descriptors are needed
    MaskBits64 = ~(((U64)1 << 48) - 1);
    if (PciBuffer.PhysicalAddr & MaskBits64)
    {
        Cons_printf(
            " -- ERROR: Address requires > 48-bit (Extended Descriptors not supported)\n"
            );
        return PLX_STATUS_UNSUPPORTED;
    }

    // Set starting address of buffer
    pSgl = pDmaBuffer;

    // Clear buffer region
    memset( pSgl, 0, 0x500 );

    // Align SGL start address to 64 byte boundary (required by DMA engine)
    while (PciBuffer.PhysicalAddr & 0x3F)
    {
        pSgl                   += 1;
        PciBuffer.PhysicalAddr += 1;
    }

    // Fill source buffers with random data
    GenerateRandomData( pSgl + 0x100, 0x100 );
    GenerateRandomData( pSgl + 0x200, 0x100 );


    /************************************************
     *
     *          Write first DMA descriptor
     *
     ***********************************************/
    Cons_printf("  Write first DMA Descriptor..... ");

    AddrSrc  = PciBuffer.PhysicalAddr + 0x100;
    AddrDest = PciBuffer.PhysicalAddr + 0x300;

    // Offset 00h - Valid bit & transfer count
    TmpValue = PLX_LE_U32_BIT( 31 ) |       // Descriptor valid
               PLX_LE_DATA_32( 0x100 );     // Transfer byte count
    *(U32*)(pSgl + 0x0) = TmpValue;

    // Offset 04h - Upper address bits of source & dest ([47:32])
    TmpValue  = (PLX_64_HIGH_32( AddrSrc ) & 0x0000FFFF) << 16;
    TmpValue |= (PLX_64_HIGH_32( AddrDest ) & 0x0000FFFF) << 0;
    *(U32*)(pSgl + 0x4) = PLX_LE_DATA_32( TmpValue );

    // Offset 08h - Low bits of destination address ([31:0])
    TmpValue = PLX_64_LOW_32( AddrDest );
    *(U32*)(pSgl + 0x8) = PLX_LE_DATA_32( TmpValue );

    // Offset 0Ch - Low bits of source address ([31:0])
    TmpValue = PLX_64_LOW_32( AddrSrc );
    *(U32*)(pSgl + 0xC) = PLX_LE_DATA_32( TmpValue );

    Cons_printf("Ok\n");


    // Increment to next descriptor
    pSgl += (4 * sizeof(U32));



    /************************************************
     *
     *          Write second DMA descriptor
     *
     ***********************************************/
    Cons_printf("  Write second DMA Descriptor.... ");

    AddrSrc  = PciBuffer.PhysicalAddr + 0x200;
    AddrDest = PciBuffer.PhysicalAddr + 0x400;

    // Offset 00h - Valid bit & transfer count
    TmpValue = PLX_LE_U32_BIT( 31 ) |       // Descriptor valid
               PLX_LE_U32_BIT( 30 ) |       // Interrupt when done
               PLX_LE_DATA_32( 0x100 );     // Transfer byte count
    *(U32*)(pSgl + 0x0) = TmpValue;

    // Offset 04h - Upper address bits of source & dest ([47:32])
    TmpValue  = (PLX_64_HIGH_32( AddrSrc ) & 0x0000FFFF) << 16;
    TmpValue |= (PLX_64_HIGH_32( AddrDest ) & 0x0000FFFF) << 0;
    *(U32*)(pSgl + 0x4) = PLX_LE_DATA_32( TmpValue );

    // Offset 08h - Low bits of destination address ([31:0])
    TmpValue = PLX_64_LOW_32( AddrDest );
    *(U32*)(pSgl + 0x8) = PLX_LE_DATA_32( TmpValue );

    // Offset 0Ch - Low bits of source address ([31:0])
    TmpValue = PLX_64_LOW_32( AddrSrc );
    *(U32*)(pSgl + 0xC) = PLX_LE_DATA_32( TmpValue );

    Cons_printf("Ok\n");
   

    // Provide PCI address of first descriptor
    *pSglPciAddress = (U32)PciBuffer.PhysicalAddr;

    return PLX_STATUS_OK;
}




/******************************************************************************
 *
 * Function   :  PerformSglDmaTransfer_8000
 *
 * Description:  Initiates the 8000 SGL DMA transfer and waits for completion
 *
 *****************************************************************************/
PLX_STATUS
PerformSglDmaTransfer_8000(
    PLX_DEVICE_OBJECT *pDevice,
    U8                *pDmaBuffer,
    U64                SglPciAddress
    )
{
    U8                NumDescriptors;
    U16               OffsetDmaBase;
    U32               NumErrors;
    U32               RegValue;
    PLX_STATUS        status;
    PLX_INTERRUPT     PlxInterrupt;
    PLX_NOTIFY_OBJECT NotifyObject;


    /************************************************
     *
     *    Register for DMA interrupt notification
     *
     ***********************************************/
    Cons_printf("  Register for DMA int notify.... ");

    // Clear interrupt fields
    memset(&PlxInterrupt, 0, sizeof(PLX_INTERRUPT));

    PlxInterrupt.DmaDone = (1 << 0);
    status = PlxPci_NotificationRegisterFor( pDevice, &PlxInterrupt, &NotifyObject );
    if (status != PLX_STATUS_OK)
    {
        Cons_printf("*ERROR* - API failed\n");
        PlxSdkErrorDisplay(status);
        return status;
    }
    Cons_printf("Ok\n");


    /************************************************
     *
     *         Initialize for SGL DMA
     *
     ***********************************************/
    // Sample uses only 2 descriptors
    NumDescriptors = 2;

    // Set DMA register base to channel 0
    OffsetDmaBase = 0x200;

    // Make sure DMA descriptors are set to external ([2] = 0)
    if (pDevice->Key.PlxFamily == PLX_FAMILY_SIRIUS)
    {
        RegValue = PlxPci_PlxRegisterRead( pDevice, 0x1FC, NULL );
        PlxPci_PlxRegisterWrite( pDevice, 0x1FC, RegValue & ~(1 << 2) );
    }

    // Make sure DMA prefetch doesn't exceed descriptor count & is a multiple of 4
    if (NumDescriptors < 4)
        RegValue = 1;
    else if (NumDescriptors >= 256)
        RegValue = 0;
    else
        RegValue = (NumDescriptors & (U8)~0x3);
    PlxPci_PlxRegisterWrite( pDevice, OffsetDmaBase + 0x34, RegValue );

    // Clear all DMA registers
    PlxPci_PlxRegisterWrite( pDevice, OffsetDmaBase + 0x00, 0 );
    PlxPci_PlxRegisterWrite( pDevice, OffsetDmaBase + 0x04, 0 );
    PlxPci_PlxRegisterWrite( pDevice, OffsetDmaBase + 0x08, 0 );
    PlxPci_PlxRegisterWrite( pDevice, OffsetDmaBase + 0x0C, 0 );
    PlxPci_PlxRegisterWrite( pDevice, OffsetDmaBase + 0x10, 0 );

    // Descriptor ring address
    PlxPci_PlxRegisterWrite( pDevice, OffsetDmaBase + 0x14, PLX_64_LOW_32(SglPciAddress) );
    PlxPci_PlxRegisterWrite( pDevice, OffsetDmaBase + 0x18, PLX_64_HIGH_32(SglPciAddress) );

    // Current descriptor address
    PlxPci_PlxRegisterWrite( pDevice, OffsetDmaBase + 0x1C, PLX_64_LOW_32(SglPciAddress) );

    // Descriptor ring size
    PlxPci_PlxRegisterWrite( pDevice, OffsetDmaBase + 0x20, NumDescriptors );

    // Current descriptor transfer size
    PlxPci_PlxRegisterWrite( pDevice, OffsetDmaBase + 0x28, 0 );

    // Disable invalid descriptor interrupt (x3C[1])
    RegValue = PlxPci_PlxRegisterRead( pDevice, OffsetDmaBase + 0x3C, NULL );
    RegValue &= ~(1 << 1);
    PlxPci_PlxRegisterWrite( pDevice, OffsetDmaBase + 0x3C, RegValue );

    // Get DMA control/status
    RegValue = PlxPci_PlxRegisterRead( pDevice, OffsetDmaBase + 0x38, NULL );

    // Clear any active status bits ([31,12:8])
    RegValue |= ((1 << 31) | (0x1F << 8));

    // Enable SGL off-chip mode & descriptor fetch stops at end
    if (pDevice->Key.PlxFamily == PLX_FAMILY_SIRIUS)
    {
        RegValue |= (1 << 5) | (1 << 4);        // SGL mode (4) & descriptor halt mode (5)
    }
    else
    {
        RegValue &= ~(3 << 5);
        RegValue |= (2 << 5) | (1 << 4);        // SGL mode ([6:5]) & descriptor halt mode (4)
    }



    /************************************************
     *
     *              Start DMA engine
     *
     ***********************************************/
    Cons_printf("  Start DMA transfer............. ");

    // Start DMA (x38[3])
    PlxPci_PlxRegisterWrite( pDevice, OffsetDmaBase + 0x38, RegValue | (1 << 3) );

    Cons_printf("Ok\n");



    /************************************************
     *
     *           Wait for DMA completion
     *
     ***********************************************/
    Cons_printf("  Wait for interrupt event....... ");

    status = PlxPci_NotificationWait( pDevice, &NotifyObject, 10 * 1000 );

    switch (status)
    {
        case PLX_STATUS_OK:
            Cons_printf("Ok (DMA 0 Int received)\n");
            break;

        case PLX_STATUS_TIMEOUT:
            Cons_printf("*ERROR* - Timeout waiting for interrupt\n");
            break;

        case PLX_STATUS_CANCELED:
            Cons_printf("*ERROR* - Interrupt event cancelled\n");
            break;

        default:
            Cons_printf("*ERROR* - API failed (status=%s)\n", PlxSdkErrorText(status));
            break;
    }

    // Release the interrupt wait object
    Cons_printf("  Cancel Int Notification........ ");
    status = PlxPci_NotificationCancel( pDevice, &NotifyObject );
    if (status != PLX_STATUS_OK)
    {
        Cons_printf("*ERROR* - status=%s\n", PlxSdkErrorText(status));
        return status;
    }
    Cons_printf("Ok\n");



    /************************************************
     *
     *              Compare Buffers
     *
     ***********************************************/

    Cons_printf("  Compare Source/Dest 1 data..... ");
    NumErrors = CompareBuffers( pDmaBuffer + 0x100, pDmaBuffer + 0x300, 0x100 );
    Cons_printf("%s\n", (NumErrors == 0) ? "Ok" : "");

    Cons_printf("  Compare Source/Dest 2 data..... ");
    NumErrors = CompareBuffers( pDmaBuffer + 0x200, pDmaBuffer + 0x400, 0x100 );
    Cons_printf("%s\n", (NumErrors == 0) ? "Ok" : "");

    return PLX_STATUS_OK;
}




/******************************************************************************
 *
 * Function   :  GenerateRandomData
 *
 * Description:  Generates random data in the provided buffer
 *
 *****************************************************************************/
VOID
GenerateRandomData(
    U8  *pBuffer,
    U32  ByteCount
    )
{
    U32 offset;


    // Seed random number generator
    srand( time(NULL) );

    // Fill buffer with random data
    offset = 0;
    while (offset < ByteCount)
    {
        *(U16*)(pBuffer + offset) = rand();
        offset += sizeof(U16);
    }
}




/******************************************************************************
 *
 * Function   :  CompareBuffers
 *
 * Description:  Compares two buffers & returns number of errors found
 *
 *****************************************************************************/
U32
CompareBuffers(
    U8  *pBuffer_1,
    U8  *pBuffer_2,
    U32  ByteCount
    )
{
    U32 offset;
    U32 NumErrors;


    offset    = 0;
    NumErrors = 0;

    while (offset < ByteCount)
    {
        if (*(U32*)(pBuffer_1 + offset) !=
            *(U32*)(pBuffer_2 + offset))
        {
            Cons_printf(
                "\n\tERR - Wrote: %08xh   Read: %08xh",
                *(U32*)(pBuffer_1 + offset),
                *(U32*)(pBuffer_2 + offset)
                );
            NumErrors++;

            // Halt on max errors
            if (NumErrors > 10)
                break;
        }

        offset += sizeof(U32);
    }

    return NumErrors;
}
