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
 *      DSlave_BypassApi.c
 *
 * Description:
 *
 *      The "Direct Slave" sample application, which demonstrates how to perform
 *      a transfer using the virtual address obtained with the PLX API.
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
        "\t\t   PLX Direct Slave Virtual Address Sample\n"
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
 * Description:  Performs a direct slave transfer by mapping the BAR
 *
 *****************************************************************************/
void
PerformDirectSlave(
    PLX_DEVICE_OBJECT *pDevice
    )
{
    U8         *pBufferSrc;
    U8         *pBufferDest;
    U16         BarIndex;
    U32         i;
    U32         offset;
    U8         *VaPciBar;
    VOID       *VaTemp;    // Used to avoid compiler error in Linux
    PLX_STATUS  rc;


    Cons_printf(
        "\n"
        " WARNING: This sample will access a PCI BAR memory space starting at\n"
        "          the provided offset.  System crashes will result if an\n"
        "          this memory access is not supported by the selected device.\n"
        "          This sample is for reference purposes only.\n"
        );

    Cons_printf("\n\n");
    Cons_printf("Please enter PCI BAR to access --> ");
    Cons_scanf("%hd", &BarIndex);
    Cons_printf("Please enter a valid offset    --> ");
    Cons_scanf("%x", &offset);
    

    // Test an absolute address with remapping
    Cons_printf(
        "\n  PCI BAR %d: offset = 0x%08x (%d bytes)\n",
        BarIndex, offset, SIZE_BUFFER
        );

    Cons_printf("    Map BAR to user space........ ");
    rc =
        PlxPci_PciBarMap(
            pDevice,
            (U8)BarIndex,
            &VaTemp
            );

    if (rc != PLX_STATUS_OK)
    {
        Cons_printf("*ERROR* - API failed\n");
        PlxSdkErrorDisplay(rc);
        return;
    }

    // Store virtual address
    VaPciBar = VaTemp;

    Cons_printf("Ok (%p)\n", VaPciBar);


    Cons_printf("    Allocate buffers............. ");
    pBufferDest = malloc(SIZE_BUFFER);
    if (pBufferDest == NULL)
    {
        Cons_printf("*ERROR* - Destination buffer allocation failed\n");
        return;
    }

    pBufferSrc = malloc(SIZE_BUFFER);
    if (pBufferSrc == NULL)
    {
        Cons_printf("*ERROR* - Source buffer allocation failed\n");
        return;
    }
    Cons_printf("Ok\n");

    Cons_printf("    Preparing buffer data........ ");
    for (i=0; i < SIZE_BUFFER; i += sizeof(U32))
        *(U32*)(pBufferSrc + i) = i;
    Cons_printf("Ok\n");


    Cons_printf("    Write 8-bit data to BAR...... ");
    memcpy(
        VaPciBar + offset,
        pBufferSrc,
        SIZE_BUFFER
        );
    Cons_printf("Ok\n");


    Cons_printf("    Read 8-bit data from BAR..... ");
    memcpy(
        pBufferDest,
        VaPciBar + offset,
        SIZE_BUFFER
        );
    Cons_printf("Ok\n");


    Cons_printf("    Verifying data............... ");
    if (memcmp(
            pBufferSrc,
            pBufferDest,
            SIZE_BUFFER
            ) != 0)
    {
        Cons_printf("*ERROR*  -  Buffers do not match\n");
    }
    else
    {
        Cons_printf("Ok\n");
    }


    Cons_printf("    Write 32-bit data to BAR..... ");
    for (i=0; i<SIZE_BUFFER; i += sizeof(U32))
    {
        *(U32*)(VaPciBar + offset + i) = *(U32*)(pBufferSrc + i);
    }
    Cons_printf("Ok\n");


    Cons_printf("    Read 32-bit data from BAR.... ");
    for (i=0; i<SIZE_BUFFER; i += sizeof(U32))
    {
        *(U32*)(pBufferDest + i) = *(U32*)(VaPciBar + offset + i);
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
    }
    else
    {
        Cons_printf("Ok\n");
    }


    Cons_printf("    Freeing buffers.............. ");
    if (pBufferDest != NULL)
        free(pBufferDest);

    if (pBufferSrc != NULL)
        free(pBufferSrc);
    Cons_printf("Ok\n");


    Cons_printf("    Unmap PCI Space.............. ");
    PlxPci_PciBarUnmap(
        pDevice,
        &VaTemp
        );
    Cons_printf("Ok\n");
}
