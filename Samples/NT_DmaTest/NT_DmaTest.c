/******************************************************************************
 *
 * File Name:
 *
 *      NT_DmaTest.c
 *
 * Description:
 *
 *      This sample demonstrates communication across an NT port using 
 *      fast DMA transfers.
 *
 * Revision History:
 *
 *      08-01-12 : PLX SDK v7.00
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

extern int
Do_DMA_Test(
    U32 NTSrcAddr,
	U32 NTDestAddr,
	U32 NTSize,
    PLX_DEVICE_OBJECT *pNTDevice
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
    U16                 LutIndex;
    U16                 ReqId_Read;
    U16                 ReqId_Write;
    U32                 size;
    U32                 value;
    U32                 MaxSize;
    U32                 BarOffset;
    U32                 PhysAddr;
    U32                 RemoteBuffSize;
    VOID               *BarVa;
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

    if (status != ApiSuccess)
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
        (DeviceKey.NTPortType == PLX_NT_PORT_LINK) ? "LINK" : "VIRTUAL"
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

    if (status != ApiSuccess)
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

    if (status != ApiSuccess)
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

    if (status != ApiSuccess)
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
            ) != ApiSuccess)
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
            ) != ApiSuccess)
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
                    ) != ApiSuccess)
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

    if (status != ApiSuccess)
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

    if (status != ApiSuccess)
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
    if (DeviceKey.NTPortType == PLX_NT_PORT_LINK)
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
    if (DeviceKey.NTPortType == PLX_NT_PORT_LINK)
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

    if (status != ApiSuccess)
    {
        Cons_printf("ERROR: Unable to setup NT translation\n");
        goto _ExitApp;
    }
    Cons_printf("Ok (BAR offset:%08X  Max transfer:%dKB)\n", BarOffset, MaxSize >> 10);


    /*************************************************************
     * Main loop - send & receive random sized messages between other side
     ************************************************************/
    Cons_printf("\n");

    if (DeviceKey.NTPortType == PLX_NT_PORT_VIRTUAL)
    {
		Do_DMA_Test(
			(U32)PhysBuffer.PhysicalAddr,
			(U32)BarProp.Physical+BarOffset,
			RemoteBuffSize,
			&Device
			);
    }
	else
	{
		value = 0;
        do
		{
            // Check for key press
            if (Cons_kbhit())
            {
                // Get the character
                value = Cons_getch();
            }
			Plx_sleep(1000);
		}
        while (value != 27);
	}

    Cons_printf("\n\n");


_ExitApp:

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

        if (status == ApiSuccess)
        {
            // Default to add device
            bAddDevice = TRUE;

            if (DevKey.NTPortType == PLX_NT_PORT_NONE)
                bAddDevice = FALSE;

            if (bAddDevice)
            {
                // Verify supported chip type
                if (((DevKey.PlxChip & 0xFF00) != 0x8500) &&
                    ((DevKey.PlxChip & 0xFF00) != 0x8600) &&
                    ((DevKey.PlxChip & 0xFF00) != 0x8700))
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
                    (DevKey.NTPortType == PLX_NT_PORT_LINK) ? "Link" : "Virtual"
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
        (pDevice->Key.NTPortType == PLX_NT_PORT_LINK) ? "Virtual" : "Link"
        );

    // Set mailboxes to use
    if (pDevice->Key.NTPortType == PLX_NT_PORT_LINK)
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
        return ApiUnsupportedFunction;

    if (pDevice->Key.NTPortType == PLX_NT_PORT_LINK)
    {
        if (BarIndex == 2)
        {
            if (((DirAddr.DestinationAddr >> 32) != 0) ||
                 ((DirAddr.Size >> 32) != 0))
            {
                return ApiInvalidAddress;
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

    return ApiSuccess;
}
