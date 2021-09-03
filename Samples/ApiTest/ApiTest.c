/******************************************************************************
 *
 * File Name:
 *
 *      ApiTest.c
 *
 * Description:
 *
 *      Tests the PLX API
 *
 * Revision History:
 *
 *      07-01-08 : PLX SDK v6.00
 *
 ******************************************************************************/


#include <time.h>
#include "Plx.h"
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
 *               Functions
 *********************************************/
void
TestChipTypeGet(
    PLX_DEVICE_OBJECT *pDevice
    );

void
TestPlxRegister(
    PLX_DEVICE_OBJECT *pDevice
    );

void
TestPciBarMap(
    PLX_DEVICE_OBJECT *pDevice
    );

void
TestCommonBuffer(
    PLX_DEVICE_OBJECT *pDevice
    );

void
TestPhysicalMemAllocate(
    PLX_DEVICE_OBJECT *pDevice
    );

void
TestEeprom(
    PLX_DEVICE_OBJECT *pDevice
    );

void
TestInterruptNotification(
    PLX_DEVICE_OBJECT *pDevice
    );

void
TestPortInfo(
    PLX_DEVICE_OBJECT *pDevice
    );




/********************************************************
 *               Global Variables
 *******************************************************/
U16 ChipTypeSelected;
U8  ChipRevision;




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
        "\t\t            PLX SDK API Test\n"
        "\t\t              January 2007\n\n"
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

    // Display API Test version information
    Cons_printf(
        "\n"
        "                       PLX API Test\n"
        "    =================================================\n"
        );

    Cons_printf(
        "\nSelected: %04x %04x [b:%02x  s:%02x  f:%x]\n\n",
        DeviceKey.DeviceId, DeviceKey.VendorId,
        DeviceKey.bus, DeviceKey.slot, DeviceKey.function
        );

    // Get PLX Chip Type
    PlxPci_ChipTypeGet(
        &Device,
        &ChipTypeSelected,
        &ChipRevision
        );

    // Start Tests
    if (1)
    {
        /********************************************************
        *
        ********************************************************/
        TestChipTypeGet(&Device);
        _PauseWithExit;

        /********************************************************
        *
        ********************************************************/
        TestPlxRegister(&Device);
        _PauseWithExit;

        /********************************************************
        *
        ********************************************************/
        TestPciBarMap(&Device);
        _PauseWithExit;

        /********************************************************
        *
        ********************************************************/
        TestCommonBuffer(&Device);
        _PauseWithExit;

        /********************************************************
        *
        ********************************************************/
        TestPhysicalMemAllocate(&Device);
        _PauseWithExit;

        /********************************************************
        *
        ********************************************************/
        TestEeprom(&Device);
        _PauseWithExit;

        /********************************************************
        *
        ********************************************************/
        TestInterruptNotification(&Device);
        _PauseWithExit;

        /********************************************************
        *
        ********************************************************/
        TestPortInfo(&Device);
        _PauseWithExit;
    }


    /************************************
    *        Close the Device
    ************************************/
    PlxPci_DeviceClose(
        &Device
        );

    Cons_printf("\n\n");

    ConsoleEnd();

    exit(0);
}




/******************************************************************************
 *
 * Function   :  TestChipTypeGet
 *
 * Description:  
 *
 *****************************************************************************/
void
TestChipTypeGet(
    PLX_DEVICE_OBJECT *pDevice
    )
{
    U8         Revision;
    U16        ChipType;
    PLX_STATUS rc;


    Cons_printf("\nPlxPci_ChipTypeGet():\n");

    Cons_printf("  Getting PLX Chip Type.......... ");
    rc =
        PlxPci_ChipTypeGet(
            pDevice,
            &ChipType,
            &Revision
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
        "    Chip type:  %04x",
        ChipType
        );

    if (ChipType == 0)
    {
        Cons_printf(" (Non-PLX chip)");
    }
    Cons_printf("\n");

    Cons_printf(
        "    Revision :    %02X\n",
        Revision
        );
}




/********************************************************
 *
 *******************************************************/
void
TestPlxRegister(
    PLX_DEVICE_OBJECT *pDevice
    )
{
    U16        offset;
    U16        PlxChip;
    U32        RegValue;
    U32        ValueToWrite;
    U32        RegSave;
    U32        PciHeaderType;
    PLX_STATUS rc;


    Cons_printf("\nPlxPci_PlxRegisterXxx():\n");

    // Set default write value
    ValueToWrite = 0x1235A5A5;

    PlxChip = (U16)ChipTypeSelected;

    if (((ChipTypeSelected & 0xFF00) == 0x8500) ||
        ((ChipTypeSelected & 0xFF00) == 0x8600) ||
        ((ChipTypeSelected & 0xFF00) == 0x8700))
    {
        PlxChip &= 0xFF00;
    }

    // Setup test parameters
    switch (PlxChip)
    {
        case 0x8111:
        case 0x8112:
            offset = 0x1030;
            break;

        case 0x8114:
        case 0x8500:
        case 0x8600:
        case 0x8700:
            // Get PCI header type
            PciHeaderType = PlxPci_PciRegisterReadFast( pDevice, 0xC, NULL );
            PciHeaderType = (U8)((PciHeaderType >> 16) & 0x7F);

            // For NT mode, use scratchpad registers
            if (PciHeaderType == 1)
                offset = 0x210;
            else
            {
                if ((ChipTypeSelected & 0xFF00) == 0x8500)
                    offset = 0xB0;
                else
                    offset = 0xC6C;
            }
            break;

        case 0x9050:
        case 0x9030:
            offset = 0x14;
            ValueToWrite = 0x0235A5A5;   // Upper nibble not writable
            break;

        case 0x9080:
        case 0x9054:
        case 0x9056:
        case 0x9656:
        case 0x8311:
            offset = 0x78;
            break;

        case 0x0:
        default:
            Cons_printf(
                "  - Unsupported PLX chip type (%04X), skipping tests\n",
                ChipTypeSelected
                );
            return;
    }


    Cons_printf("  Reading PLX-specific reg....... ");
    RegSave =
        PlxPci_PlxRegisterRead(
            pDevice,
            offset,
            &rc
            );

    if (rc != ApiSuccess)
    {
        Cons_printf("*ERROR* - API failed\n");
        PlxSdkErrorDisplay(rc);
        return;
    }
    Cons_printf("Ok (Reg %02X = %08X)\n", offset, RegSave);


    Cons_printf("  Write to PLX reg............... ");
    rc =
        PlxPci_PlxRegisterWrite(
            pDevice,
            offset,
            ValueToWrite
            );

    if (rc != ApiSuccess)
    {
        Cons_printf("*ERROR* - API failed\n");
        PlxSdkErrorDisplay(rc);
        return;
    }
    Cons_printf("Ok (Wrote %08X)\n", ValueToWrite);


    Cons_printf("  Verifying register write....... ");
    RegValue =
        PlxPci_PlxRegisterRead(
            pDevice,
            offset,
            &rc
            );

    if (rc != ApiSuccess)
    {
        Cons_printf("*ERROR* - API failed\n");
        PlxSdkErrorDisplay(rc);
        return;
    }

    if (RegValue != ValueToWrite)
    {
        Cons_printf("*ERROR* - Wrote %08X  Read %08X\n", ValueToWrite, RegValue);
    }
    else
    {
        Cons_printf("Ok (Reg %02X = %08X)\n", offset, RegValue);
    }


    Cons_printf("  Restore original value......... ");
    rc =
        PlxPci_PlxRegisterWrite(
            pDevice,
            offset,
            RegSave
            );

    if (rc != ApiSuccess)
    {
        Cons_printf("*ERROR* - API failed\n");
        PlxSdkErrorDisplay(rc);
        return;
    }
    Cons_printf("Ok (Wrote %08X)\n", RegSave);


    /***********************************
     * Memory-mapped register accesses
     **********************************/
    Cons_printf("  Read mem-mapped PLX reg........ ");
    RegSave =
        PlxPci_PlxMappedRegisterRead(
            pDevice,
            offset,
            &rc
            );

    if (rc != ApiSuccess)
    {
        if ((rc == ApiUnsupportedFunction) &&
            ((ChipTypeSelected == 0x8111) ||
             (ChipTypeSelected == 0x8112)))
        {
            Cons_printf("Ok (Expected rc=ApiUnsupportedFunction)\n");
        }
        else
        {
            Cons_printf("*ERROR* - API call failed\n");
            PlxSdkErrorDisplay(rc);
        }

        // No need to go further
        return;
    }
    else
    {
        Cons_printf("Ok (Reg %02X = %08X)\n", offset, RegSave);
    }


    Cons_printf("  Write to mem-mapped PLX reg.... ");
    rc =
        PlxPci_PlxMappedRegisterWrite(
            pDevice,
            offset,
            ValueToWrite
            );

    if (rc != ApiSuccess)
    {
        Cons_printf("*ERROR* - API failed\n");
        PlxSdkErrorDisplay(rc);
        return;
    }
    Cons_printf("Ok (Wrote %08X)\n", ValueToWrite);


    Cons_printf("  Verifying mapped reg write..... ");
    RegValue =
        PlxPci_PlxMappedRegisterRead(
            pDevice,
            offset,
            &rc
            );

    if (rc != ApiSuccess)
    {
        Cons_printf("*ERROR* - API failed\n");
        PlxSdkErrorDisplay(rc);
        return;
    }

    if (RegValue != ValueToWrite)
    {
        Cons_printf("*ERROR* - Wrote %08X  Read %08X\n", ValueToWrite, RegValue);
    }
    else
    {
        Cons_printf("Ok (Reg %02X = %08X)\n", offset, RegValue);
    }


    Cons_printf("  Restore original mapped value.. ");
    rc =
        PlxPci_PlxMappedRegisterWrite(
            pDevice,
            offset,
            RegSave
            );

    if (rc != ApiSuccess)
    {
        Cons_printf("*ERROR* - API failed\n");
        PlxSdkErrorDisplay(rc);
        return;
    }
    Cons_printf("Ok (Wrote %08X)\n", RegSave);
}




/********************************************************
 *
 *******************************************************/
void
TestPciBarMap(
    PLX_DEVICE_OBJECT *pDevice
    )
{
    U8               i;
    U32              Size;
    VOID            *Va[PCI_NUM_BARS_TYPE_00];
    PLX_STATUS       rc;
    PLX_PCI_BAR_PROP BarProp;


    Cons_printf("\nPlxPci_PciBarMap()...\n");

    for (i=0; i<PCI_NUM_BARS_TYPE_00; i++)
    {
        Cons_printf("  Mapping PCI BAR %d........ ", i);

        // Get BAR size
        PlxPci_PciBarProperties(
            pDevice,
            i,
            &BarProp
            );

        Size = (U32)BarProp.Size;

        rc =
            PlxPci_PciBarMap(
                pDevice,
                i,
                &(Va[i])
                );

        Cons_printf(
            "%s (VA=%p  %d %s)\n",
            PlxSdkErrorText(rc),
            Va[i],
            (Size > (10 << 10)) ? (Size >> 10) : Size,
            (Size > (10 << 10)) ? "KB" : "bytes"
            );
    }

    Cons_printf("\n");

    for (i=0; i<PCI_NUM_BARS_TYPE_00; i++)
    {
        Cons_printf(
            "  Unmapping PCI BAR %d...... ",
            i
            );

        rc =
            PlxPci_PciBarUnmap(
                pDevice,
                &(Va[i])
                );

        Cons_printf(
            "Ok (rc = %s)\n",
            PlxSdkErrorText(rc)
            );
    }
}




/********************************************************
*
********************************************************/
void
TestCommonBuffer(
    PLX_DEVICE_OBJECT *pDevice
    )
{
    VOID             *Va;
    PLX_STATUS        status;
    PLX_PHYSICAL_MEM  CommonBuffer;


    Cons_printf("\nPlxPci_CommonBufferXxx():\n");


    Cons_printf("  Get Common buffer properties....... ");
    status =
        PlxPci_CommonBufferProperties(
            pDevice,
            &CommonBuffer
            );

    if (status != ApiSuccess)
    {
        Cons_printf("*ERROR* - rc=%s\n", PlxSdkErrorText(status));
    }
    else
    {
        Cons_printf("Ok\n");
    }


    Cons_printf("  Map Common buffer to user space.... ");
    status =
        PlxPci_CommonBufferMap(
            pDevice,
            &Va
            );

    if (status != ApiSuccess)
    {
        // Handle case where buffer not allocated
        if (CommonBuffer.Size == 0)
        {
            Cons_printf("Ok (rc=%s)\n", PlxSdkErrorText(status));
        }
        else
        {
            Cons_printf("*ERROR* - rc=%s\n", PlxSdkErrorText(status));
        }
    }
    else
    {
        Cons_printf("Ok\n");
    }

    Cons_printf(
        "      Bus Physical Addr: 0x%08lx\n"
        "      CPU Physical Addr: 0x%08lx\n"
        "      Virtual Address  : 0x%p\n"
        "      Buffer Size      : %d Kb",
        (PLX_UINT_PTR)CommonBuffer.PhysicalAddr,
        (PLX_UINT_PTR)CommonBuffer.CpuPhysical,
        Va,
        (CommonBuffer.Size >> 10)
        );

    Cons_printf("\n");
    Cons_printf("  Unmap Common buffer................ ");
    status =
        PlxPci_CommonBufferUnmap(
            pDevice,
            &Va
            );

    if (status != ApiSuccess)
    {
        // Handle case where buffer not allocated
        if (CommonBuffer.Size == 0)
        {
            Cons_printf("Ok (rc=%s)\n", PlxSdkErrorText(status));
        }
        else
        {
            Cons_printf("*ERROR* - rc=%s\n", PlxSdkErrorText(status));
        }
    }
    else
    {
        Cons_printf("Ok\n");
    }
}




/********************************************************
 *
 *******************************************************/
void
TestPhysicalMemAllocate(
    PLX_DEVICE_OBJECT *pDevice
    )
{
    U32              RequestSize;
    PLX_STATUS       rc;
    PLX_PHYSICAL_MEM PhysBuffer;


    Cons_printf("\nPlxPci_PhysicalMemoryXxx():\n");

    // Set buffer size to request
    RequestSize = 0x100000;


    Cons_printf("  Allocate buffer...... ");
    PhysBuffer.Size = RequestSize;
    rc =
        PlxPci_PhysicalMemoryAllocate(
            pDevice,
            &PhysBuffer,
            TRUE             // Smaller buffer ok
            );

    if (rc != ApiSuccess)
    {
        if (rc == ApiUnsupportedFunction)
        {
            Cons_printf("*ERROR* - ApiUnsupportedFunction returned\n");
            Cons_printf("     -- PLX Service driver used, Physical Mem API not supported --\n");
        }
        else
        {
            Cons_printf("*ERROR* - Unable to allocate physical buffer\n");
            PlxSdkErrorDisplay(rc);
        }
        return;
    }
    Cons_printf("Ok\n");

    Cons_printf("  Map buffer........... ");
    rc =
        PlxPci_PhysicalMemoryMap(
            pDevice,
            &PhysBuffer
            );

    if (rc != ApiSuccess)
    {
        Cons_printf("*ERROR* - (rc=%s)\n", PlxSdkErrorText(rc));
    }
    else
    {
        Cons_printf("Ok\n");
    }

    Cons_printf(
        "      Bus Physical Addr: 0x%08lx\n"
        "      CPU Physical Addr: 0x%08lx\n"
        "      Virtual Address  : 0x%p\n"
        "      Buffer Size      : %d Kb",
        (PLX_UINT_PTR)PhysBuffer.PhysicalAddr,
        (PLX_UINT_PTR)PhysBuffer.CpuPhysical,
        (PLX_UINT_PTR)PhysBuffer.UserAddr,
        (PhysBuffer.Size >> 10)
        );

    if (RequestSize != PhysBuffer.Size)
    {
        Cons_printf(
            " (req=%d Kb)\n",
            (RequestSize >> 10)
            );
    }
    else
    {
        Cons_printf("\n");
    }


    Cons_printf("\n");
    Cons_printf("  Unmap buffer......... ");
    rc =
        PlxPci_PhysicalMemoryUnmap(
            pDevice,
            &PhysBuffer
            );

    Cons_printf("Ok (rc=%s)\n", PlxSdkErrorText(rc));

    Cons_printf("  Free buffer.......... ");
    rc =
        PlxPci_PhysicalMemoryFree(
            pDevice,
            &PhysBuffer
            );

    if (rc != ApiSuccess)
    {
        Cons_printf("*ERROR* - Unable to free physical buffer\n");
        PlxSdkErrorDisplay(rc);
    }
    else
    {
        Cons_printf("Ok\n");
    }
}




/********************************************************
 *
 *******************************************************/
void
TestEeprom(
    PLX_DEVICE_OBJECT *pDevice
    )
{
    U8                CrcStatus;
    U16               offset;
    U16               PlxChip;
    U16               ReadSave_16;
    U16               ReadValue_16;
    U32               Crc;
    U32               ReadSave;
    U32               ReadValue;
    U32               WriteValue;
    BOOLEAN           bCrc;
    BOOLEAN           bEepromPresent;
    PLX_STATUS        rc;
    PLX_EEPROM_STATUS EepromStatus;


    Cons_printf("\nPlxPci_EepromXxx():\n");

    PlxChip = (U16)ChipTypeSelected;

    if (((ChipTypeSelected & 0xFF00) == 0x8600) ||
        ((ChipTypeSelected & 0xFF00) == 0x8700))
    {
        PlxChip &= 0xFF00;
    }

    // Setup test parameters
    switch (PlxChip)
    {
        case 0x8111:
        case 0x8112:
        case 0x8509:
        case 0x8505:
        case 0x8533:
        case 0x8547:
        case 0x8548:
        case 0x8600:
        case 0x8700:
            offset = 0x0;
            bCrc   = FALSE;
            break;

        case 0x8114:
        case 0x8508:
        case 0x8512:
        case 0x8516:
        case 0x8517:
        case 0x8518:
        case 0x8524:
        case 0x8525:
        case 0x8532:
            offset = 0x1000;
            bCrc   = TRUE;
            break;

        case 0x9050:
        case 0x9030:
        case 0x9080:
        case 0x9054:
        case 0x9056:
        case 0x9656:
        case 0x8311:
            offset = 0x0;
            bCrc   = FALSE;
            break;

        case 0x0:
        default:
            Cons_printf(
                "  - Unsupported PLX chip type (%04X), skipping tests\n",
                ChipTypeSelected
                );
            return;
    }

    // Set value to write
    WriteValue = 0x12AB06A5;

    Cons_printf("  Checking if EEPROM present..... ");
    EepromStatus =
        PlxPci_EepromPresent(
            pDevice,
            &rc
            );

    if (rc != ApiSuccess)
    {
        Cons_printf("*ERROR* - API call failed\n");
        PlxSdkErrorDisplay(rc);
    }
    else
    {
        Cons_printf("Ok (");

        if (EepromStatus == PLX_EEPROM_STATUS_NONE)
            Cons_printf("No EEPROM Present)\n");
        else if (EepromStatus == PLX_EEPROM_STATUS_VALID)
            Cons_printf("EEPROM present with valid data)\n");
        else if (EepromStatus == PLX_EEPROM_STATUS_INVALID_DATA)
            Cons_printf("Present but invalid data/CRC error/blank)\n");
        else
            Cons_printf("?Unknown? (%d))\n", EepromStatus);
    }


    Cons_printf("  Probing for EEPROM............. ");
    bEepromPresent =
        PlxPci_EepromProbe(
            pDevice,
            &rc
            );

    if (rc != ApiSuccess)
    {
        Cons_printf("*ERROR* - API call failed\n");
        PlxSdkErrorDisplay(rc);
    }
    else
    {
        if (bEepromPresent)
            Cons_printf("Ok (EEPROM presence detected)\n");
        else
            Cons_printf("Ok (EEPROM not detected)\n");
    }


    // No need to continue if EEPROM not detect or non-PLX devices
    if (bEepromPresent == FALSE)
    {
        Cons_printf("      -- EEPROM not detected, skipping remaing tests --\n");
        return;
    }


    // Read 16-bit from EEPROM
    Cons_printf("  Read EEPROM (16-bit)........... ");

    rc =
        PlxPci_EepromReadByOffset_16(
            pDevice,
            offset,
            &ReadSave_16
            );

    if (rc != ApiSuccess)
    {
        Cons_printf("*ERROR* - API call failed\n");
        PlxSdkErrorDisplay(rc);
    }
    else
    {
        Cons_printf(
            "Ok  - (val[%02X] = 0x%04x)\n",
            offset, ReadSave_16
            );
    }


    // Write 16-bit to EEPROM
    Cons_printf("  Write EEPROM (16-bit).......... ");

    rc =
        PlxPci_EepromWriteByOffset_16(
            pDevice,
            offset,
            (U16)WriteValue
            );

    if (rc != ApiSuccess)
    {
        Cons_printf("*ERROR* - API call failed\n");
        PlxSdkErrorDisplay(rc);
    }
    else
    {
        Cons_printf(
            "Ok  - (value = 0x%04x)\n",
            (U16)WriteValue
            );
    }


    // Verify Write
    Cons_printf("  Verify write................... ");

    rc =
        PlxPci_EepromReadByOffset_16(
            pDevice,
            offset,
            &ReadValue_16
            );

    if (rc != ApiSuccess)
    {
        Cons_printf("*ERROR* - API call failed\n");
        PlxSdkErrorDisplay(rc);
    }
    else
    {
        if (ReadValue_16 == (U16)WriteValue)
        {
            Cons_printf(
                "Ok  - (value = 0x%04x)\n",
                ReadValue_16
                );
        }
        else
        {
            Cons_printf(
                "*ERROR* - Rd (0x%04x) != Wr (0x%04x)\n",
                ReadValue_16, (U16)WriteValue
                );
        }
    }


    // Restore Original Value
    Cons_printf("  Restore EEPROM................. ");

    rc =
        PlxPci_EepromWriteByOffset_16(
            pDevice,
            offset,
            ReadSave_16
            );

    if (rc != ApiSuccess)
    {
        Cons_printf("*ERROR* - API call failed\n");
        PlxSdkErrorDisplay(rc);
    }
    else
        Cons_printf("Ok\n");


    // Read from EEPROM by offset
    Cons_printf("  Read EEPROM (32-bit)........... ");

    rc =
        PlxPci_EepromReadByOffset(
            pDevice,
            offset,
            &ReadSave
            );

    if (rc != ApiSuccess)
    {
        Cons_printf("*ERROR* - API call failed\n");
        PlxSdkErrorDisplay(rc);
    }
    else
    {
        Cons_printf(
            "Ok  - (val[%02X] = 0x%08x)\n",
            offset, ReadSave
            );
    }


    // Write to EEPROM by offset
    Cons_printf("  Write EEPROM (32-bit).......... ");

    rc =
        PlxPci_EepromWriteByOffset(
            pDevice,
            offset,
            WriteValue
            );

    if (rc != ApiSuccess)
    {
        Cons_printf("*ERROR* - API call failed\n");
        PlxSdkErrorDisplay(rc);
    }
    else
    {
        Cons_printf(
            "Ok  - (value = 0x%08x)\n",
            WriteValue
            );
    }


    // Verify Write
    Cons_printf("  Verify write................... ");

    rc =
        PlxPci_EepromReadByOffset(
            pDevice,
            offset,
            &ReadValue
            );

    if (rc != ApiSuccess)
    {
        Cons_printf("*ERROR* - API call failed\n");
        PlxSdkErrorDisplay(rc);
    }
    else
    {
        if (ReadValue == WriteValue)
        {
            Cons_printf(
                "Ok  - (value = 0x%04x)\n",
                ReadValue
                );
        }
        else
        {
            Cons_printf(
                "*ERROR* - Rd (0x%04x) != Wr (0x%04x)\n",
                ReadValue, WriteValue
                );
        }
    }


    // Restore Original Value
    Cons_printf("  Restore EEPROM................. ");

    rc =
        PlxPci_EepromWriteByOffset(
            pDevice,
            offset,
            ReadSave
            );

    if (rc != ApiSuccess)
    {
        Cons_printf("*ERROR* - API call failed\n");
        PlxSdkErrorDisplay(rc);
    }
    else
        Cons_printf("Ok\n");


    // Get EEPROM CRC
    Cons_printf("  Getting current EEPROM CRC..... ");
    rc =
        PlxPci_EepromCrcGet(
            pDevice,
            &Crc,
            &CrcStatus
            );

    if (rc != ApiSuccess)
    {
        if ((rc == ApiUnsupportedFunction) && (bCrc == FALSE))
        {
            Cons_printf("Ok (Expected rc=ApiUnsupportedFunction)\n");
        }
        else
        {
            Cons_printf("*ERROR* - API call failed\n");
            PlxSdkErrorDisplay(rc);
        }
    }
    else
    {
        Cons_printf(
            "Ok (CRC=%08x  Status=%s)\n",
            Crc,
            (CrcStatus == PLX_CRC_VALID) ? "Valid" : "Invalid"
            );
    }


    // Update EEPROM CRC
    Cons_printf("  Update EEPROM CRC.............. ");
    rc =
        PlxPci_EepromCrcUpdate(
            pDevice,
            &Crc,
            FALSE       // Don't update EEPROM
            );

    if (rc != ApiSuccess)
    {
        if ((rc == ApiUnsupportedFunction) && (bCrc == FALSE))
        {
            Cons_printf("Ok (Expected rc=ApiUnsupportedFunction)\n");
        }
        else
        {
            Cons_printf("*ERROR* - API call failed\n");
            PlxSdkErrorDisplay(rc);
        }
    }
    else
    {
        Cons_printf(
            "Ok (New CRC=%08x)\n",
            Crc
            );
    }
}




/********************************************************
 *
 *******************************************************/
void
TestInterruptNotification(
    PLX_DEVICE_OBJECT *pDevice
    )
{
    U16               DB_Value;
    U16               Offset_IrqBase;
    U16               Offset_IrqSet;
    U16               Offset_IrqMaskSet;
    U16               Offset_IrqMaskClear;
    U32               RegValue;
    U32               RegSave;
    PLX_INTERRUPT     PlxInterrupt;
    PLX_STATUS        rc;
    PLX_NOTIFY_OBJECT NotifyObject;


    Cons_printf("\nPlxPci_NotificationXxx()...\n");

    switch (ChipTypeSelected)
    {
        case 0x8114:
        case 0x8505:
        case 0x8508:
        case 0x8509:
        case 0x8512:
        case 0x8516:
        case 0x8517:
        case 0x8518:
        case 0x8524:
        case 0x8525:
        case 0x8532:
        case 0x8533:
        case 0x8547:
        case 0x8548:
        case 0x8612:
        case 0x8616:
        case 0x8624:
        case 0x8632:
        case 0x8647:
        case 0x8648:
            // Verify device is in NT mode (PCI header type must be 0)
            RegValue =
                PlxPci_PciRegisterReadFast(
                    pDevice,
                    0x0c,       // PCI Header type / Cache line
                    NULL
                    );

            if (((RegValue >> 16) & 0x7F) != 0)
            {
                Cons_printf("  ERROR: Device is not in NT mode, interrupts not supported\n");
                return;
            }
            break;

        case 0x0:
        default:
            Cons_printf(
                "  - Unsupported PLX chip type (%04X), skipping tests\n",
                ChipTypeSelected
                );
            return;
    }

    // Set IRQ base offset
    if ((ChipTypeSelected & 0xFF00) == 0x8500)
        Offset_IrqBase = 0x90;
    else
        Offset_IrqBase = 0xC4C;

    // Add offset based on virtual or link side
    if (pDevice->Key.NTPortType == PLX_NT_PORT_LINK)
    {
        // Skip over Virtual-side registers
        Offset_IrqBase += 0x10;
    }

    // Set final offsets
    Offset_IrqSet       = Offset_IrqBase + 0x00;
    Offset_IrqMaskSet   = Offset_IrqBase + 0x08;
    Offset_IrqMaskClear = Offset_IrqBase + 0x0C;

    // Register for interrupt notification
    Cons_printf("  Register for Int. notification..... ");

    // Clear interrupt fields
    memset(&PlxInterrupt, 0, sizeof(PLX_INTERRUPT));

    // Seed the random-number generator
    srand( (unsigned)time( NULL ) );

    // Select a random 16-bit number for doorbells
    DB_Value              = rand();
    PlxInterrupt.Doorbell = DB_Value;

    // Register for interrupt notification
    rc =
        PlxPci_NotificationRegisterFor(
            pDevice,
            &PlxInterrupt,
            &NotifyObject
            );

    if (rc != ApiSuccess)
    {
        if (rc == ApiUnsupportedFunction)
        {
            Cons_printf("*ERROR* - ApiUnsupportedFunction returned\n");
            Cons_printf("     -- PLX Service driver used, Notification API not supported --\n");
            return;
        }

        Cons_printf("*ERROR* - API failed\n");
        PlxSdkErrorDisplay(rc);
    }
    else
    {
        Cons_printf(
            "Ok (DB val=%04X)\n",
            PlxInterrupt.Doorbell
            );
    }


    // Save current IRQ mask
    Cons_printf("  Save current IRQ mask.............. ");
    RegSave =
        PlxPci_PlxRegisterRead(
            pDevice,
            Offset_IrqMaskSet,
            NULL
            );
    Cons_printf("Ok (mask=%04X)\n", RegSave);


    // Enable all doorbell interrupts
    Cons_printf("  Enable all doorbell interrupts..... ");
    PlxPci_PlxRegisterWrite(
        pDevice,
        Offset_IrqMaskClear,
        0xFFFF
        );
    Cons_printf("Ok\n");


    // Wait for interrupt event
    Cons_printf("  Generate and wait for interrupt.... ");

    // Trigger a doorbell interrupt
    PlxPci_PlxRegisterWrite(
        pDevice,
        Offset_IrqSet,
        PlxInterrupt.Doorbell
        );

    rc =
        PlxPci_NotificationWait(
            pDevice,
            &NotifyObject,
            5 * 1000
            );

    switch (rc)
    {
        case ApiSuccess:
            Cons_printf("Ok (Int received)\n");
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

    // Get the interrupt status
    Cons_printf("  Get Interrupt Status............... ");

    rc =
        PlxPci_NotificationStatus(
            pDevice,
            &NotifyObject,
            &PlxInterrupt
            );

    if (rc != ApiSuccess)
    {
        Cons_printf("*ERROR* - API failed\n");
        PlxSdkErrorDisplay(rc);
    }
    else
    {
        Cons_printf(
            "Ok (DB val=%04X)\n",
            PlxInterrupt.Doorbell
            );
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


    // Restore interrupt mask
    Cons_printf("  Restore interrupt mask............. ");
    PlxPci_PlxRegisterWrite(
        pDevice,
        Offset_IrqMaskSet,
        RegSave
        );
    Cons_printf("Ok\n");
}




/********************************************************
 *
 *******************************************************/
void
TestPortInfo(
    PLX_DEVICE_OBJECT *pDevice
    )
{
    PLX_STATUS    rc;
    PLX_PORT_PROP PortProp;


    Cons_printf("\nPlxPci_GetPortInfo()...\n");

    Cons_printf("  Get Port properties................ ");
    rc =
        PlxPci_GetPortProperties(
            pDevice,
            &PortProp
            );

    if (rc != ApiSuccess)
    {
        Cons_printf("*ERROR* - API failed\n");
        PlxSdkErrorDisplay(rc);
        return;
    }

    Cons_printf("Ok\n");

    Cons_printf(
        "      Port Type  : %02d ",
        PortProp.PortType
        );

    switch (PortProp.PortType)
    {
        case PLX_PORT_UNKNOWN:
            Cons_printf("(Unknown?)\n");
            break;

        case PLX_PORT_ENDPOINT:  // PLX_PORT_NON_TRANS
            Cons_printf("(Endpoint or NT port)\n");
            break;

        case PLX_PORT_UPSTREAM:
            Cons_printf("(Upstream)\n");
            break;

        case PLX_PORT_DOWNSTREAM:
            Cons_printf("(Downstream)\n");
            break;

        case PLX_PORT_LEGACY_ENDPOINT:
            Cons_printf("(Endpoint)\n");
            break;

        case PLX_PORT_ROOT_PORT:
            Cons_printf("(Root Port)\n");
            break;

        case PLX_PORT_PCIE_TO_PCI_BRIDGE:
            Cons_printf("(PCIe-to-PCI Bridge)\n");
            break;

        case PLX_PORT_PCI_TO_PCIE_BRIDGE:
            Cons_printf("(PCI-to-PCIe Bridge)\n");
            break;

        case PLX_PORT_ROOT_ENDPOINT:
            Cons_printf("(Root Complex Endpoint)\n");
            break;

        case PLX_PORT_ROOT_EVENT_COLL:
            Cons_printf("(Root Complex Event Collector)\n");
            break;

        default:
            Cons_printf("(N/A)\n");
            break;
    }

    if (PortProp.bNonPcieDevice)
    {
        Cons_printf("- Device does not support PCI Express Capabilities -\n");
        return;
    }

    Cons_printf(
        "      Port Number: %02d\n",
        PortProp.PortNumber
        );

    Cons_printf(
        "      Max Payload: %02d\n",
        PortProp.MaxPayloadSize
        );

    Cons_printf(
        "      Link Width : x%d (Max=x%d)\n",
        PortProp.LinkWidth, PortProp.MaxLinkWidth
        );

    Cons_printf(
        "      Link Speed : %s Gbps (Max=%s Gbps)\n",
        (PortProp.LinkSpeed == PLX_LINK_SPEED_5_0_GBPS) ? "5.0" : "2.5",
        (PortProp.MaxLinkSpeed == PLX_LINK_SPEED_5_0_GBPS) ? "5.0" : "2.5"
        );
}
