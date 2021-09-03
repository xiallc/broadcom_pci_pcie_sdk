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
 *      NT_LinkTest.c
 *
 * Description:
 *
 *      This sample demonstrates basic communication across an NT port between
 *      with NT link plugged/unplugged detection.
 *
 * Revision History:
 *
 *      07-01-14 : PLX SDK v7.20
 *
 ******************************************************************************/


#include <ctype.h>
#include <time.h>
#include "PlxApi.h"

#if defined(_WIN32)
    #include "..\\Shared\\ConsFunc.h"
#endif

#if defined(PLX_LINUX)
    #include "ConsFunc.h"
#endif




/**********************************************
 *               Definitions
 *********************************************/
#define NT_PCI_BUFFER_SIZE                  (500 * 1000)    // Size of buffer for receiving data
#define MAX_DEVICES_TO_LIST                 50              // Max number of devices for user selection

#define NT_MSG_SYSTEM_READY                 0xFEEDFACE      // Code passed between systems to signal ready

////////////// Direct Address Translation ////////////
#define PLX_8000_BAR_2_TRAN_LOWER           0xC3C           // BAR 2 lower 32-address translation reg.
#define PLX_8000_BAR_2_TRAN_UPPER           0xC40           // BAR 2 upper 32-address translation reg.
#define PLX_8000_BAR_4_TRAN_LOWER           0xC44           // BAR 4 lower 32-address translation reg.
#define PLX_8000_BAR_4_TRAN_UPPER           0xC48           // BAR 4 upper 32-address translation reg.
#define PLX_8500_BAR_2_LIMIT_LOWER          0xC4C           // 8500-series BAR 2 limit lower 32-address
#define PLX_8500_BAR_2_LIMIT_UPPER          0xC50           // 8500-series BAR 2 limit upper 32-address
#define PLX_8500_BAR_4_LIMIT_LOWER          0xC54           // 8500-series BAR 4 limit lower 32-address
#define PLX_8500_BAR_4_LIMIT_UPPER          0xC58           // 8500-series BAR 4 limit upper 32-address




/**********************************************
 *              Definitions
 *********************************************/
#define PlxBarMem_32(Va, offset)               *(VU32*)((U8*)Va + offset)


// PLX Direct Address Translation Strture
typedef struct _PLX_DIRECT_ADDRESS
{
    U64 DestinationAddr;
    U64 Size;
} PLX_DIRECT_ADDRESS;




/**********************************************
 *               Gloabals
 *********************************************/
U32 Gbl_Regs[0x14];              // Global storage for PCI register save




/**********************************************
 *               Functions
 *********************************************/
S16
SelectDevice_NT(
    PLX_DEVICE_KEY *pKey
    );

S8
WaitForConnection(
    PLX_DEVICE_OBJECT *pDevice
    );

PLX_STATUS
PlxPci_SetupNtTranslation(
    PLX_DEVICE_OBJECT  *pDevice,
    U8                  BarIndex,
    PLX_DIRECT_ADDRESS  DirAddr
    );

PLX_STATUS
PlxPciRegisterSaveRestore(
    PLX_DEVICE_OBJECT *pDevice,
    U32               *pPciRegs,
    BOOLEAN            bRestore
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
    S16                 DeviceSelected;
    U8                  BarNum;
    U8                 *pSysMem;
    U16                 MsgId;
    U16                 LutIndex;
    U16                 TxPriority;
    U16                 ReqId_Read;
    U16                 ReqId_Write;
    U16                 DataIncrement;
    U32                 i;
    U32                 size;
    U32                 value;
    U32                 RegBar;
    U32                 MaxSize;
    U32                 Count_Tx;
    U32                 Count_Rx;
    U32                 Count_Loop;
    U32                 BarOffset;
    U32                 PhysAddr;
    U32                 RemoteBuffSize;
    VOID               *BarVa;
    BOOLEAN             bLinkDown;
    PLX_STATUS          status;
    PLX_DEVICE_KEY      DeviceKey;
    PLX_PHYSICAL_MEM    PhysBuffer;
    PLX_PCI_BAR_PROP    BarProp;
    PLX_DEVICE_OBJECT   Device;
    PLX_DIRECT_ADDRESS  DirAddr;


    // Mark physical buffer as not allocated
    PhysBuffer.PhysicalAddr = 0;

    ConsoleInitialize();

    Cons_clear();

    Cons_printf(
        "\n\n"
        "\t\t                  PLX NT Link Test\n"
        "\t\t    =================================================\n\n"
        );


    /************************************
     *         Select Device
     ***********************************/
    DeviceSelected =
        SelectDevice_NT(
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
        Cons_printf(
            "\n"
            "    ERROR: Unable to find or select a PLX device\n"
            );

        goto _ExitApp;
    }

    // Display test version information
    Cons_printf(
        "\n"
        "                  PLX NT Link Test\n"
        "    =================================================\n"
        );

    Cons_printf(
        "\nSelected: %.4x %.4x [b:%02x s:%02x f:%x]\n\n",
        DeviceKey.DeviceId, DeviceKey.VendorId,
        DeviceKey.bus, DeviceKey.slot, DeviceKey.function
        );


    // Seed random number generator
    srand( time(NULL) );

    /************************************
     *     Display the NT side
     ***********************************/
    Cons_printf(
        "Communicating from: %s side\n\n",
        (DeviceKey.PlxPortType == PLX_SPEC_PORT_NT_LINK) ? "LINK" : "VIRTUAL"
        );


    /*************************************************************
     * Get PCI BAR and map
     ************************************************************/
    // Only PCI BAR 2 is currently supported
    BarNum = 2;

    Cons_printf("Get BAR %d properties       : ", BarNum);
    status =
        PlxPci_PciBarProperties(
            &Device,
            BarNum,
            &BarProp
            );

    if (status != PLX_STATUS_OK)
    {
        Cons_printf("ERROR: Unable to get PCI BAR properties\n");
        goto _ExitApp;
    }
    Cons_printf("Ok (Size: %d MB)\n", (BarProp.Size >> 20));

    // Map the BAR to a virtual address
    Cons_printf("Map BAR %d to user space    : ", BarNum);
    status =
        PlxPci_PciBarMap(
            &Device,
            BarNum,
            &BarVa
            );

    if (status != PLX_STATUS_OK)
    {
        Cons_printf("ERROR: Unable to map PCI BAR\n");
        goto _ExitApp;
    }
    Cons_printf("Ok (VA:%p)\n", BarVa);


    /*************************************************************
     * Determine Requester ID & add LUT entry
     ************************************************************/
    Cons_printf("Probe for write ReqID      : ");

    status =
        PlxPci_Nt_ReqIdProbe(
            &Device,
            FALSE,          // Probe for writes
            &ReqId_Write
            );

    if (status != PLX_STATUS_OK)
    {
        Cons_printf("ERROR: Unable to probe ReqID, auto-add 0,0,0\n");
        ReqId_Write = 0;
    }
    else
    {
        Cons_printf(
            "Ok (ReqID=%04X [b:%02X s:%02X f:%01X])\n",
            ReqId_Write,
            (ReqId_Write >> 8) & 0xFF,
            (ReqId_Write >> 3) & 0x1F,
            (ReqId_Write >> 0) & 0x03
            );
    }

    Cons_printf("Add write Req ID to LUT    : ");

    // Default to auto-selected LUT index
    LutIndex = (U16)-1;

    if (PlxPci_Nt_LutAdd(
            &Device,
            &LutIndex,
            ReqId_Write,
            FALSE       // Snoop must be disabled
            ) != PLX_STATUS_OK)
    {
        Cons_printf("ERROR: Unable to add LUT entry\n");
    }
    else
    {
        Cons_printf("Ok (LUT_Index=%d No_Snoop=OFF)\n", LutIndex);
    }

    Cons_printf("Probe for read ReqID       : ");

    if (PlxPci_Nt_ReqIdProbe(
            &Device,
            TRUE,           // Probe for reads
            &ReqId_Read
            ) != PLX_STATUS_OK)
    {
        Cons_printf("ERROR: Unable to probe ReqID\n");
    }
    else
    {
        Cons_printf(
            "Ok (ReqID=%04X [b:%02X s:%02X f:%01X])\n",
            ReqId_Read,
            (ReqId_Read >> 8) & 0xFF,
            (ReqId_Read >> 3) & 0x1F,
            (ReqId_Read >> 0) & 0x03
            );

        Cons_printf("Add read Req ID to LUT     : ");

        if (ReqId_Read == ReqId_Write)
        {
            Cons_printf("-- Read Req ID matches write, skip LUT add --\n");
        }
        else
        {
            // Default to auto-selected LUT index
            LutIndex = (U16)-1;

            if (PlxPci_Nt_LutAdd(
                    &Device,
                    &LutIndex,
                    ReqId_Read,
                    FALSE       // Snoop must be disabled
                    ) != PLX_STATUS_OK)
            {
                Cons_printf("ERROR: Unable to add LUT entry\n");
            }
            else
            {
                Cons_printf("Ok (LUT_Index=%d No_Snoop=OFF)\n", LutIndex);
            }
        }
    }

    /*************************************************************
     * Allocate & map PCI buffer
     ************************************************************/
    Cons_printf("Allocate PCI buffer        : ");

    // Set desired size of buffer
    PhysBuffer.Size = NT_PCI_BUFFER_SIZE;

    status =
        PlxPci_PhysicalMemoryAllocate(
            &Device,
            &PhysBuffer,
            TRUE
            );

    if (status != PLX_STATUS_OK)
    {
        Cons_printf("ERROR: Unable to allocate buffer for data transfers\n");
        goto _ExitApp;
    }
    Cons_printf(
        "Ok (PCI:%08lX  Size:%ldKB)\n",
        (PLX_UINT_PTR)PhysBuffer.PhysicalAddr,
        (PLX_UINT_PTR)PhysBuffer.Size >> 10
        );

    Cons_printf("Map PCI buffer             : ");
    status =
        PlxPci_PhysicalMemoryMap(
            &Device,
            &PhysBuffer
            );

    if (status != PLX_STATUS_OK)
    {
        Cons_printf("ERROR: Unable to map to user space\n");
        goto _ExitApp;
    }
    Cons_printf(
        "Ok (VA:%p)\n",
        PLX_INT_TO_PTR(PhysBuffer.UserAddr)
        );

    // Setup a pointer to the buffer
    pSysMem = PLX_INT_TO_PTR(PhysBuffer.UserAddr);


    /*************************************************************
     * Post buffer properties for other side
     ************************************************************/
    Cons_printf("Broadcast buffer properties: ");

    // Set mailbox to write
    if (DeviceKey.PlxPortType == PLX_SPEC_PORT_NT_LINK)
        value = 6;
    else
        value = 3;

    // Post buffer address
    PlxPci_PlxMailboxWrite(
        &Device,
        (U16)value,
        (U32)PhysBuffer.PhysicalAddr
        );

    // Post buffer size
    PlxPci_PlxMailboxWrite(
        &Device,
        (U16)(value + 1),
        (U32)PhysBuffer.Size
        );

    Cons_printf("Ok\n");


    // Set initial value
    *(VU32*)pSysMem = 0xABCD;


    /*************************************************************
     * Establish connection with other side
     ************************************************************/
    if (WaitForConnection(
            &Device
            ) != 0)
    {
        Cons_printf("ERROR: Unable to establish connection\n");
        goto _ExitApp;
    }



    /*************************************************************
     * Get buffer information from other side
     ************************************************************/
    Cons_printf("Get buffer properties: ");

    // Set mailbox to read
    if (DeviceKey.PlxPortType == PLX_SPEC_PORT_NT_LINK)
        value = 3;
    else
        value = 6;

    // Get buffer address
    PhysAddr =
        PlxPci_PlxMailboxRead(
            &Device,
            (U16)value,
            NULL
            );

    // Get buffer size
    RemoteBuffSize =
        PlxPci_PlxMailboxRead(
            &Device,
            (U16)(value + 1),
            NULL
            );

    Cons_printf("Ok (Addr:%08X  Size:%dKB)\n", PhysAddr, (RemoteBuffSize >> 10));



    /*************************************************************
     * Set up NT translation
     ************************************************************/
    Cons_printf("Setup NT translation: ");

    // Convert BAR size to range 
    size = ~((U32)BarProp.Size - 1);

    // Calculate BAR offset
    BarOffset = PhysAddr & ~size;

    // Verify we don't exceed BAR limits
    if ((BarOffset + RemoteBuffSize) > (U32)BarProp.Size)
    {
        Cons_printf("ERROR: Remote buffer exceeds BAR space limits\n");
        goto _ExitApp;
    }

    // Determine maximum transfer size
    MaxSize = (U32)BarProp.Size - BarOffset;

    // Do not exceed PCI buffer size
    if (MaxSize > RemoteBuffSize)
        MaxSize = RemoteBuffSize;

    // Subtract a little just in case
    MaxSize -= (2 * sizeof(U32));

    // Calculate base address for translation
    PhysAddr &= size;

    // Prepare for NT translation setup
    DirAddr.DestinationAddr = PhysAddr;
    DirAddr.Size            = BarProp.Size;

    // Set BAR direct address translation
    status =
        PlxPci_SetupNtTranslation(
            &Device,
            BarNum,
            DirAddr
            );

    if (status != PLX_STATUS_OK)
    {
        Cons_printf("ERROR: Unable to setup NT translation\n");
        goto _ExitApp;
    }
    Cons_printf("Ok (BAR offset:%08X  Max transfer:%dKB)\n", BarOffset, MaxSize >> 10);


    Plx_sleep(500);

    // Save some registers in case of link up/down testing
    PlxPciRegisterSaveRestore(
        &Device,
        Gbl_Regs,
        FALSE           // Save registers
        );


    /*************************************************************
     * Main loop - send & receive random sized messages between other side
     ************************************************************/
    Cons_printf("\n");

    // Mark link as up
    bLinkDown = FALSE;

    // Clear any pending message
    *(VU32*)pSysMem = 0;

    Count_Tx   = 0;
    Count_Rx   = 0;
    Count_Loop = 0;

    // Use slightly different priorities to skew Tx/Rx (larger = higher priority)
    if (DeviceKey.PlxPortType == PLX_SPEC_PORT_NT_VIRTUAL)
        TxPriority = 50;
    else
        TxPriority = 1000;

    do
    {
        // Periodically send a message
        if ((rand() < TxPriority) && (PlxBarMem_32(BarVa, BarOffset) == 0))
        {
            // Generate new MsgID
            MsgId = rand();

            // Generate msg size
            size = (U32)(((float)rand() / RAND_MAX) * ((float)MaxSize));

            // Make sure size doesn't exceed max
            if (size > MaxSize)
                size = MaxSize;

            // Post message size
            PlxBarMem_32(BarVa, BarOffset + 4) = size;

            // Generate random 32-bit start data & increment
            value         = ((U32)rand() << 16) | rand();
            DataIncrement = rand();

            // Post message data
            for (i=0; i<size; i+=sizeof(U32))
            {
                PlxBarMem_32(
                    BarVa,
                    BarOffset + 8 + i
                    ) = value;

                value += DataIncrement;
            }

            Cons_printf(
                "Tx Msg %03d: %04X (%0.2f KB)==>\n",
                Count_Tx,
                MsgId,
                (float)size / 1024
                );

            // Post message ID
            PlxBarMem_32(BarVa, BarOffset + 0) = MsgId;

            // Increment message count
            Count_Tx++;
        }

        // Check if message received
        value = *(VU32*)(pSysMem + 0);

        // Check for termination request
        if (value == (U32)-1)
        {
            Cons_printf("\n\n    -- Other side terminated, closing connection -- \n");
            break;
        }

        if (value != 0)
        {
            // Get message size
            size = *(VU32*)(pSysMem + 4);

            Cons_printf(
                "<== Rx Msg %03d: %04X (%0.2f KB)\n",
                Count_Rx,
                value,
                ((float)size) / 1024
                );

            // Increment counter
            Count_Rx++;

            // Clear message
            *(VU32*)(pSysMem + 4) = 0;
            *(VU32*)(pSysMem + 0) = 0;
        }

        // Periodically verify device exists
        if ((Count_Loop & 0x3FFF) == 0)
        {
            value = 0;
            do
            {
                // Get link status
                if (DeviceKey.PlxPortType == PLX_SPEC_PORT_NT_VIRTUAL)
                {
                    // Read NT-Link side PCIe Link status (1000h past NT-Virtual registers)
                    RegBar = PlxPci_PlxRegisterRead( &Device, 0x1000 + 0x78, NULL );

                    if (((RegBar >> 20) & 0x3F) == 0)
                        bLinkDown = TRUE;
                    else
                        bLinkDown = FALSE;
                }
                else
                {
                    // Perform PCI config read to probe for device
                    RegBar = PlxPci_PciRegisterReadFast( &Device, 0x18, NULL );

                    if (RegBar == (U32)-1)
                        bLinkDown = TRUE;
                    else
                        bLinkDown = FALSE;
                }

                if (bLinkDown)
                {
                    if (value == 0)
                    {
                        Cons_printf("\n ---- Link down ----\n");
                        value = 1;
                    }

                    // Small delay between probes
                    Plx_sleep( 100 );
                }
            }
            while (bLinkDown);

            if ((value == 1) && (bLinkDown == FALSE))
            {
                Cons_printf(
                    "\n ---- Link back up ----\n"
                    "Restore registers: "
                    );

                // Restore saved registers
                PlxPciRegisterSaveRestore(
                    &Device,
                    Gbl_Regs,
                    TRUE            // Restore registers
                    );

                Cons_printf(
                    "Ok\n"
                    "Resuming data transfer...\n\n"
                    );

                Plx_sleep( 1000 );
            }
        }

        // Periodically check for user cancel
        if ((Count_Loop & 0x3FFF) == 0)
        {
            // Check for key press
            if (Cons_kbhit())
            {
                // Get the character
                value = Cons_getch();

                // Exit if ESC pressed
                if (value == 27)
                {
                    // Inform other side to halt
                    PlxBarMem_32(BarVa, BarOffset) = (U32)-1;
                    Cons_printf("\n\n   -- User halt, closing connection -- \n");
                    Plx_sleep(10);
                    PlxBarMem_32(BarVa, BarOffset) = (U32)-1;
                    break;
                }
            }
        }

        // Increment loop counter
        Count_Loop++;
    }
    while (1);

    Cons_printf("\n\n");


_ExitApp:

    // Small delay to let remote side halt Tx
    Plx_sleep( 500 );

    if (PhysBuffer.PhysicalAddr != 0)
    {
        Cons_printf("Release resources: ");

        // Free system memory
        PlxPci_PhysicalMemoryFree(
            &Device,
            &PhysBuffer
            );

        Cons_printf("Ok\n");
    }

    Cons_printf("\nPress any key to exit...\n");
    Cons_getch();

    ConsoleEnd();

    return 0;
}




/*********************************************************************
 *
 * Function   : SelectDevice_NT
 *
 * Description: Asks the user which PLX NT port device to select
 *
 * Returns    : Total devices found
 *              -1,  if user cancelled the selection
 *
 ********************************************************************/
S16
SelectDevice_NT(
    PLX_DEVICE_KEY *pKey
    )
{
    S32               i;
    S32               NumDevices;
    BOOLEAN           bAddDevice;
    PLX_STATUS        status;
    PLX_PORT_PROP     PortProp;
    PLX_DEVICE_KEY    DevKey;
    PLX_DEVICE_KEY    DevKey_NT[MAX_DEVICES_TO_LIST];
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

            if (DevKey.PlxPortType == PLX_SPEC_PORT_UNKNOWN)
                bAddDevice = FALSE;

            if (bAddDevice)
            {
                // Verify supported chip type
                if (((DevKey.PlxChip & 0xFF00) != 0x8500) &&
                    ((DevKey.PlxChip & 0xFF00) != 0x8600) &&
                    ((DevKey.PlxChip & 0xFF00) != 0x8700) &&
                    ((DevKey.PlxChip & 0xFF00) != 0x9700))
                {
                    bAddDevice = FALSE;
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

            // Verify driver used is NT PnP and not Service driver
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

            // Get port properties
            if (bAddDevice)
            {
                // Get port properties
                PlxPci_GetPortProperties(
                    &Device,
                    &PortProp
                    );
            }

            // Close device
            PlxPci_DeviceClose(
                &Device
                );

            if (bAddDevice)
            {
                // Copy device key info
                DevKey_NT[NumDevices] = DevKey;

                Cons_printf(
                    "\t\t  %2d. %04x Port %d [b:%02x s:%02x] (%s-side)\n",
                    (NumDevices + 1), DevKey.PlxChip, PortProp.PortNumber,
                    DevKey.bus, DevKey.slot,
                    (DevKey.PlxPortType == PLX_SPEC_PORT_NT_LINK) ? "Link" : "Virtual"
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
    *pKey = DevKey_NT[i - 1];

    return (S16)NumDevices;
}




/******************************************************************************
 *
 * Function   :  WaitForConnection
 *
 * Description:  
 *
 *****************************************************************************/
S8
WaitForConnection(
    PLX_DEVICE_OBJECT *pDevice
    )
{
    char DispStat[] = "/|\\-";
    S16  LoopCount;
    U16  MB_Read;
    U16  MB_Write;
    U32  RegValue;


    Cons_printf(
        "\n"
        "Wait for %s side (ESC to cancel): ",
        (pDevice->Key.PlxPortType == PLX_SPEC_PORT_NT_LINK) ? "Virtual" : "Link"
        );

    // Set mailboxes to use
    if (pDevice->Key.PlxPortType == PLX_SPEC_PORT_NT_LINK)
    {
        MB_Read  = 2;
        MB_Write = 5;
    }
    else
    {
        MB_Read  = 5;
        MB_Write = 2;
    }

    // Notify other side ready to connect
    PlxPci_PlxMailboxWrite(
        pDevice,
        MB_Write,
        NT_MSG_SYSTEM_READY
        );

    // Set counter
    LoopCount = 180;

    // Wait for other side to respond
    do
    {
        Cons_printf(
            "%c\b",
            DispStat[(LoopCount % 4)]
            );
        Cons_fflush( stdout );

        LoopCount--;

        // Small delay
        Plx_sleep(500);

        // Check for a message
        RegValue =
            PlxPci_PlxMailboxRead(
                pDevice,
                MB_Read,
                NULL
                );

        // Check for ESC key to cancel
        if (Cons_kbhit())
        {
            if (Cons_getch() == 27)
            {
                LoopCount = -1;
            }
        }
    }
    while ((LoopCount > 0) && (RegValue != NT_MSG_SYSTEM_READY));

    // Clear connect message on fail
    if (LoopCount <= 0)
    {
        PlxPci_PlxMailboxWrite(
            pDevice,
            MB_Write,
            0
            );
    }

    // Clear response message
    PlxPci_PlxMailboxWrite(
        pDevice,
        MB_Read,
        0
        );

    if (LoopCount == 0)
    {
        Cons_printf("ERROR - Timeout\n");
        return -1;
    }

    if (LoopCount == -1)
    {
        Cons_printf("User cancelled\n");
        return -1;
    }

    Cons_printf("Ok\n");
    Cons_printf("\n    -- Connection established! --\n\n");

    return 0;
}




/******************************************************************************
 *
 * Function   :  PlxPci_SetupNtTranslation
 *
 * Description:  
 *
 *****************************************************************************/
PLX_STATUS
PlxPci_SetupNtTranslation(
    PLX_DEVICE_OBJECT  *pDevice,
    U8                  BarIndex,
    PLX_DIRECT_ADDRESS  DirAddr
    )
{
    U16 DestBaseLow;
    U16 DestBaseHi;
    U16 DestLimitLow;
    U16 DestLimitHi;


    // Only BAR 2 currently supported
    if (BarIndex != 2)
        return PLX_STATUS_UNSUPPORTED;

    if (pDevice->Key.PlxPortType == PLX_SPEC_PORT_NT_LINK)
    {
        if (BarIndex == 2)
        {
            if (((DirAddr.DestinationAddr >> 32) != 0) ||
                 ((DirAddr.Size >> 32) != 0))
            {
                return PLX_STATUS_INVALID_ADDR;
            }
        }
    }

    if (BarIndex == 2)
    {
        DestBaseLow  = PLX_8000_BAR_2_TRAN_LOWER;
        DestBaseHi   = PLX_8000_BAR_2_TRAN_UPPER;
        DestLimitLow = PLX_8500_BAR_2_LIMIT_LOWER;
        DestLimitHi  = PLX_8500_BAR_2_LIMIT_UPPER;
    }
    else
    {
        DestBaseLow  = PLX_8000_BAR_4_TRAN_LOWER;
        DestBaseHi   = PLX_8000_BAR_4_TRAN_UPPER;
        DestLimitLow = PLX_8500_BAR_4_LIMIT_LOWER;
        DestLimitHi  = PLX_8500_BAR_4_LIMIT_UPPER;
    }

    PlxPci_PlxRegisterWrite(
        pDevice,
        DestBaseLow,
        (U32)DirAddr.DestinationAddr
        );

    PlxPci_PlxRegisterWrite(
        pDevice,
        DestBaseHi,
        (U32)(DirAddr.DestinationAddr >> 32)
        );

    // Set limit registers to 0 on 8500 series
    if ((pDevice->Key.PlxChip & 0xFF00) == 0x8500)
    {
        PlxPci_PlxRegisterWrite(
            pDevice,
            DestLimitLow,
            0
            );

        PlxPci_PlxRegisterWrite(
            pDevice,
            DestLimitHi,
            0
            );
    }

    return PLX_STATUS_OK;
}




/******************************************************************************
 *
 * Function   :  PlxPciRegisterSaveRestore
 *
 * Description:  
 *
 *****************************************************************************/
PLX_STATUS
PlxPciRegisterSaveRestore(
    PLX_DEVICE_OBJECT *pDevice,
    U32               *pPciRegs,
    BOOLEAN            bRestore
    )
{
    U8 offset;


    // Save NT-Link side PCI registers
    if (pDevice->Key.PlxPortType == PLX_SPEC_PORT_NT_LINK)
    {
        for (offset=0; offset < 0x40; offset += sizeof(U32))
        {
            if (bRestore)
            {
                PlxPci_PciRegisterWriteFast(
                    pDevice,
                    offset,
                    pPciRegs[offset / sizeof(U32)]
                    );
            }
            else
            {
                pPciRegs[offset / sizeof(U32)] =
                    PlxPci_PciRegisterReadFast(
                        pDevice,
                        offset,
                        NULL
                        );
            }
        }
    }

    if (bRestore)
    {
        // BAR 2 Translation
        PlxPci_PlxRegisterWrite( pDevice, 0xC3C, pPciRegs[0x10] );

        // ReqID LUT 0 & 1
        if (pDevice->Key.PlxPortType == PLX_SPEC_PORT_NT_VIRTUAL)
        {
            PlxPci_PlxRegisterWrite( pDevice, 0xD94, pPciRegs[0x11] );
            PlxPci_PlxRegisterWrite( pDevice, 0xD98, pPciRegs[0x12] );
        }
        else
        {
            PlxPci_PlxRegisterWrite( pDevice, 0xDB4, pPciRegs[0x11] );
            PlxPci_PlxRegisterWrite( pDevice, 0xDB8, pPciRegs[0x12] );
        }
    }
    else
    {
        // BAR 2 Translation
        pPciRegs[0x10] = PlxPci_PlxRegisterRead( pDevice, 0xC3C, NULL );

        // ReqID LUT 0 & 1
        if (pDevice->Key.PlxPortType == PLX_SPEC_PORT_NT_VIRTUAL)
        {
            pPciRegs[0x11] = PlxPci_PlxRegisterRead( pDevice, 0xD94, NULL );
            pPciRegs[0x12] = PlxPci_PlxRegisterRead( pDevice, 0xD98, NULL );
        }
        else
        {
            pPciRegs[0x11] = PlxPci_PlxRegisterRead( pDevice, 0xDB4, NULL );
            pPciRegs[0x12] = PlxPci_PlxRegisterRead( pDevice, 0xDB8, NULL );
        }
    }

    return PLX_STATUS_OK;
}
