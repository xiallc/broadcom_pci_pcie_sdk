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
 *      DSlave.c
 *
 * Description:
 *
 *      The "Direct Slave" sample application, which demonstrates how to perform
 *      a transfer using the PLX API.
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
*               Definitions
**********************************************/
#define SIZE_BUFFER         0x100           // Number of bytes to transfer




/**********************************************
*               Functions
**********************************************/
void
PerformDirectSlave(
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
        "\t\t   PLX Direct Slave Sample Application\n"
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



    /************************************
    *        Perform the Test
    ************************************/
    PerformDirectSlave(
        &Device
        );



    /************************************
    *        Close the Device
    ************************************/
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
 * Function   :  PerformDirectSlave
 *
 * Description:  Performs a direct slave transfer using the PLX API
 *
 *****************************************************************************/
void
PerformDirectSlave(
    PLX_DEVICE_OBJECT *pDevice
    )
{
    U8          BarIndex;
    U32         i;
    U32         DevVenId;
    U32         LocalAddress;
    U32        *pBufferDest;
    U32        *pBufferSrc;
    PLX_STATUS  rc;


    // Read Device/Vendor ID
    DevVenId =
        PlxPci_PciRegisterReadFast(
            pDevice,
            0,
            NULL
            );

    // Set PCI BAR to use
    BarIndex = 2;

    // Set local address to access
    switch (DevVenId)
    {
        case 0x905010b5:        // 9050 RDK
        case 0x520110b5:        // 9052 RDK    
            BarIndex     = 4;
            LocalAddress = 0x01000000;
            break;

        case 0x300110b5:        // 9030 RDK-LITE
        case 0x30c110b5:        // cPCI 9030 RDK-LITE
        case 0x960110b5:        // 9656 RDK-LITE
        case 0x560110b5:        // 9056 RDK-LITE
        case 0x86e110b5:        // 8311 RDK
            LocalAddress = 0x00000000;
            break;

        case 0x040110b5:        // 9080 RDK-401b
        case 0x186010b5:        // 9054 RDK-860
        case 0xc86010b5:        // cPCI 9054 RDK-860
        case 0x96c210b5:        // cPCI 9656 RDK-860
        case 0x56c210b5:        // cPCI 9056 RDK-860
            LocalAddress = 0x00100000;
            break;

        case 0x086010b5:        // 9080 RDK-860
            LocalAddress = 0x10100000;
            break;

        case 0x540610b5:        // 9054 RDK-LITE
            LocalAddress = 0x20000000;
            break;

        default:
            Cons_printf(
                "  ERROR - Test not configured for device (%04X_%04X)\n",
                (DevVenId >> 16), (U16)DevVenId
                );
            return;
    }

    // First test without remapping
    Cons_printf(
        "  Without Remapping: BAR %d, 32-bit, offset = 0\n",
        BarIndex
        );

    Cons_printf("    Preparing buffers............ ");
    pBufferDest = malloc(SIZE_BUFFER);
    if (pBufferDest == NULL)
    {
        Cons_printf("*ERROR* - Destination buffer allocation failed\n");
        return;
    }

    pBufferSrc  = malloc(SIZE_BUFFER);
    if (pBufferSrc == NULL)
    {
        Cons_printf("*ERROR* - Source buffer allocation failed\n");
        return;
    }

    for (i=0; i < (SIZE_BUFFER >> 2); i++)
        pBufferSrc[i] = i;

    // Clear destination buffer
    memset(
        pBufferDest,
        0,
        SIZE_BUFFER
        );
    Cons_printf("Ok\n");


    Cons_printf("    Writing Data to Local Bus.... ");
    rc =
        PlxPci_PciBarSpaceWrite(
            pDevice,
            BarIndex,
            0x0,            // Starting offset
            pBufferSrc,
            SIZE_BUFFER,
            BitSize32,
            FALSE           // Treat address as an offset from BAR
            );

    if (rc != PLX_STATUS_OK)
    {
        Cons_printf("*ERROR* - API failed\n");
        PlxSdkErrorDisplay(rc);
        return;
    }
    Cons_printf("Ok\n");


    Cons_printf("    Reading Data from Local Bus.. ");
    rc =
        PlxPci_PciBarSpaceRead(
            pDevice,
            BarIndex,
            0x0,            // Starting offset
            pBufferDest,
            SIZE_BUFFER,
            BitSize32,
            FALSE           // Treat address as an offset from BAR
            );

    if (rc != PLX_STATUS_OK)
    {
        Cons_printf("*ERROR* - API failed\n");
        PlxSdkErrorDisplay(rc);
        return;
    }
    Cons_printf("Ok\n");


    Cons_printf("    Verifying data............... ");
    if (memcmp(
            pBufferSrc,
            pBufferDest,
            SIZE_BUFFER
            ) != 0)
    {
        Cons_printf("*ERROR* - Buffers do not match\n");
        return;
    }
    Cons_printf("Ok\n");



    // Now test an absolute address with remapping
    Cons_printf(
        "\n  With Remapping: BAR %d, 8-bit, address = 0x%08x\n",
        BarIndex, LocalAddress
        );


    Cons_printf("    Preparing buffers............ ");
    memset(
        pBufferDest,
        0,
        SIZE_BUFFER
        );
    Cons_printf("Ok\n");


    Cons_printf("    Writing Data to Local Bus.... ");
    rc =
        PlxPci_PciBarSpaceWrite(
            pDevice,
            BarIndex,
            LocalAddress,   // Local address to access
            pBufferSrc,
            SIZE_BUFFER,
            BitSize8,
            TRUE            // Treat address as the true local bus address
            );

    if (rc != PLX_STATUS_OK)
    {
        Cons_printf("*ERROR* - API failed\n");
        PlxSdkErrorDisplay(rc);
        return;
    }
    Cons_printf("Ok\n");


    Cons_printf("    Reading Data from Local Bus.. ");
    rc =
        PlxPci_PciBarSpaceRead(
            pDevice,
            BarIndex,
            LocalAddress,   // Local address to access
            pBufferDest,
            SIZE_BUFFER,
            BitSize8,
            TRUE            // Treat address as the true local bus address
            );

    if (rc != PLX_STATUS_OK)
    {
        Cons_printf("*ERROR* - API failed\n");
        PlxSdkErrorDisplay(rc);
        return;
    }
    Cons_printf("Ok\n");


    Cons_printf("    Verifying data............... ");
    if (memcmp(
            pBufferSrc,
            pBufferDest,
            SIZE_BUFFER
            ) != 0)
    {
        Cons_printf("*ERROR*  -  Buffers do not match\n");
        return;
    }
    Cons_printf("Ok\n");


    Cons_printf("    Freeing buffers.............. ");
    if (pBufferDest != NULL)
        free(pBufferDest);

    if (pBufferSrc != NULL)
        free(pBufferSrc);
    Cons_printf("Ok\n");
}
