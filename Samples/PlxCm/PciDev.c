/*******************************************************************************
 * Copyright 2013-2018 Avago Technologies
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

/*********************************************************************
 *
 * Module Name:
 *
 *      PciDev.c
 *
 * Abstract:
 *
 *      PCI device functions
 *
 ********************************************************************/


#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include "Monitor.h"
#include "PciDev.h"
#include "PciRegs.h"
#include "PlxApi.h"




/*************************************
 *            Globals
 ************************************/
static DEVICE_NODE *pDeviceList;


struct _PCI_CLASS_CODES
{
    U8   BaseClass;
    U8   SubClass;
    U8   Interface;
    char StrClass[100];
} PciClassCodes[] =
    {
        {0x00, 0x00, 0x00, "Unknown/Pre-PCI v2.0 Device"},
            {0x00, 0x01, 0xFF, "VGA-compatible device"},

        {0x01, 0xFF, 0xFF, "Mass storage controller"},
            {0x01, 0x00, 0x00, "SCSI controller (Vendor-specific)"},
            {0x01, 0x00, 0x11, "SCSI storage device"},
            {0x01, 0x00, 0x12, "SCSI controller"},
            {0x01, 0x00, 0x13, "SCSI controller & storage device"},
            {0x01, 0x00, 0x21, "SCSI over PCIe storage device"},
            {0x01, 0x01, 0xFF, "IDE controller"},
            {0x01, 0x02, 0x00, "Floppy disk controller"},
            {0x01, 0x03, 0x00, "IPI bus controller"},
            {0x01, 0x04, 0x00, "RAID controller"},
            {0x01, 0x05, 0x20, "ATA controller (single DMA)"},
            {0x01, 0x05, 0x30, "ATA controller (chained DMA)"},
            {0x01, 0x06, 0x00, "SATA controller (Vendor-specific)"},
            {0x01, 0x06, 0x01, "SATA controller (AHCI interface)"},
            {0x01, 0x06, 0x02, "Serial storage bus interface"},
            {0x01, 0x07, 0x00, "Serial Attached SCSI (SAS) controller"},
            {0x01, 0x07, 0x01, "-- Obsolete device --"},
            {0x01, 0x08, 0x00, "Non-Volatile Memory (Vendor-specific)"},
            {0x01, 0x08, 0x01, "Non-Volatile Memory (NVMHCI)"},
            {0x01, 0x08, 0x02, "NVMe Endpoint"},
            {0x01, 0x09, 0x00, "Universal Flash Storage (Vend-spec)"},
            {0x01, 0x09, 0x01, "Universal Flash Storage (UFSHCI)"},
            {0x01, 0x80, 0x00, "Other mass storage controller"},

        {0x02, 0xFF, 0xFF, "Network controller"},
            {0x02, 0x00, 0x00, "Ethernet controller"},
            {0x02, 0x01, 0x00, "Token ring controller"},
            {0x02, 0x02, 0x00, "FDDI controller"},
            {0x02, 0x03, 0x00, "ATM controller"},
            {0x02, 0x04, 0x00, "ISDN controller"},
            {0x02, 0x05, 0x00, "WorldFip controller"},
            {0x02, 0x06, 0xFF, "PCIMG 2.14 Multi-Computing"},
            {0x02, 0x07, 0x00, "InfiniBand controller"},
            {0x02, 0x08, 0x00, "Host fabric controller (Vend-spec)"},
            {0x02, 0x80, 0x00, "Other network controller"},

        {0x03, 0xFF, 0xFF, "Display controller"},
            {0x03, 0x00, 0x00, "VGA-compatible display controller"},
            {0x03, 0x00, 0x01, "8514-compatible display controller"},
            {0x03, 0x01, 0x00, "XGA display controller"},
            {0x03, 0x02, 0x00, "3D display controller"},
            {0x03, 0x80, 0x00, "Other display controller"},

        {0x04, 0xFF, 0xFF, "Multimedia device"},
            {0x04, 0x00, 0x00, "Multimedia video device"},
            {0x04, 0x01, 0x00, "Multimedia audio device"},
            {0x04, 0x02, 0x00, "Computer telephony device"},
            {0x04, 0x03, 0x00, "HD Audio (HD-A) 1.0 device"},
            {0x04, 0x03, 0x80, "HD Audio (HD-A) 1.0 with ven-spec IF"},
            {0x04, 0x80, 0x00, "Other multimedia device"},

        {0x05, 0xFF, 0xFF, "Memory controller"},
            {0x05, 0x00, 0x00, "RAM memory controller"},
            {0x05, 0x01, 0x00, "Flash memory controller"},
            {0x05, 0x80, 0x00, "Other memory controller"},

        {0x06, 0xFF, 0xFF, "Bridge device"},
            {0x06, 0x00, 0x00, "Host bridge / Root Complex"},
            {0x06, 0x01, 0x00, "ISA bridge"},
            {0x06, 0x02, 0x00, "EISA bridge"},
            {0x06, 0x03, 0x00, "MCA bridge"},
            {0x06, 0x04, 0x00, "PCI-to-PCI bridge"},
            {0x06, 0x04, 0x01, "PCI-to-PCI bridge (subtractive dec)"},
            {0x06, 0x05, 0x00, "PCMCIA bridge"},
            {0x06, 0x06, 0x00, "NuBus bridge"},
            {0x06, 0x07, 0x00, "CardBus bridge"},
            {0x06, 0x08, 0x00, "RACEway bridge (transparent mode)"},
            {0x06, 0x08, 0x01, "RACEway bridge (end-point mode)"},
            {0x06, 0x09, 0x40, "PCI-to-PCI bridge (primary bus)"},
            {0x06, 0x09, 0x80, "PCI-to-PCI bridge (second bus)"},
            {0x06, 0x0A, 0x00, "InfiniBand-to-PCI host bridge"},
            {0x06, 0x0B, 0x00, "Adv Switch to PCI host bridge (Cust)"},
            {0x06, 0x0B, 0x01, "Adv Switch to PCI bridge (ASI-SIG)"},
            {0x06, 0x80, 0x00, "Other bridge device"},

        {0x07, 0xFF, 0xFF, "Simple communications controller"},
            {0x07, 0x00, 0x00, "Generic XT-compatible serial ctrlr"},
            {0x07, 0x00, 0x01, "16450-compatible serial controller"},
            {0x07, 0x00, 0x02, "16550-compatible serial controller"},
            {0x07, 0x00, 0x03, "16650-compatible serial controller"},
            {0x07, 0x00, 0x04, "16750-compatible serial controller"},
            {0x07, 0x00, 0x05, "16850-compatible serial controller"},
            {0x07, 0x00, 0x06, "16950-compatible serial controller"},
            {0x07, 0x01, 0x00, "Parallel port"},
            {0x07, 0x01, 0x01, "Bi-directional parallel port"},
            {0x07, 0x01, 0x02, "ECP 1.x compliant parallel port"},
            {0x07, 0x01, 0x03, "IEEE 1284 controller"},
            {0x07, 0x01, 0xFE, "IEEE 1284 target device"},
            {0x07, 0x02, 0x00, "Multiport serial controller"},
            {0x07, 0x03, 0x00, "Generic modem"},
            {0x07, 0x03, 0x01, "Hayes-compatible modem (16450)"},
            {0x07, 0x03, 0x02, "Hayes-compatible modem (16550)"},
            {0x07, 0x03, 0x03, "Hayes-compatible modem (16650)"},
            {0x07, 0x03, 0x04, "Hayes-compatible modem (16750)"},
            {0x07, 0x04, 0x00, "GPIB (IEEE 488.1/2) controller"},
            {0x07, 0x05, 0x00, "Smart Card"},
            {0x07, 0x80, 0x00, "Other communications device"},

        {0x08, 0xFF, 0xFF, "Base system peripheral"},
            {0x08, 0x00, 0x00, "Generic 8259 PIC"},
            {0x08, 0x00, 0x01, "ISA PIC"},
            {0x08, 0x00, 0x02, "EISA PIC"},
            {0x08, 0x00, 0x10, "I/O APIC interrupt contoller"},
            {0x08, 0x00, 0x20, "I/O(x) APIC interrupt contoller"},
            {0x08, 0x01, 0x00, "Generic 8237 DMA controller"},
            {0x08, 0x01, 0x01, "ISA DMA controller"},
            {0x08, 0x01, 0x02, "EISA DMA controller"},
            {0x08, 0x02, 0x00, "Generic 8254 system timer"},
            {0x08, 0x02, 0x01, "ISA system timer"},
            {0x08, 0x02, 0x02, "EISA system timers (two timers)"},
            {0x08, 0x02, 0x03, "High Performance Event Timer"},
            {0x08, 0x03, 0x00, "Generic RTC controller"},
            {0x08, 0x03, 0x01, "ISA RTC controller"},
            {0x08, 0x04, 0x00, "Generic PCI Hot-Plug controller"},
            {0x08, 0x05, 0x00, "SD Host Controller"},
            {0x08, 0x06, 0x00, "IOMMU"},
            {0x08, 0x07, 0x00, "Root Complex Event Collector"},
            {0x08, 0x80, 0x00, "Other system peripheral"},

        {0x09, 0xFF, 0xFF, "Input device"},
            {0x09, 0x00, 0x00, "Keyboard controller"},
            {0x09, 0x01, 0x00, "Digitizer (pen)"},
            {0x09, 0x02, 0x00, "Mouse controller"},
            {0x09, 0x03, 0x00, "Scanner controller"},
            {0x09, 0x04, 0x00, "Gameport controller (generic)"},
            {0x09, 0x04, 0x10, "Gameport controller"},
            {0x09, 0x80, 0x00, "Other input controller"},

        {0x0A, 0xFF, 0xFF, "Docking station"},
            {0x0A, 0x00, 0x00, "Generic docking station"},
            {0x0A, 0x80, 0x00, "Other type of docking station"},

        {0x0B, 0xFF, 0xFF, "Processor"},
            {0x0B, 0x00, 0x00, "386"},
            {0x0B, 0x01, 0x00, "486"},
            {0x0B, 0x02, 0x00, "Pentium"},
            {0x0B, 0x10, 0x00, "Alpha"},
            {0x0B, 0x20, 0x00, "PowerPC"},
            {0x0B, 0x30, 0x00, "MIPS"},
            {0x0B, 0x40, 0x00, "Co-processor"},
            {0x0B, 0x80, 0x00, "Other processor"},

        {0x0C, 0xFF, 0xFF, "Serial bus controller"},
            {0x0C, 0x00, 0x00, "IEEE 1394 controller (FireWire)"},
            {0x0C, 0x00, 0x10, "IEEE 1394 controller (OpenHCI)"},
            {0x0C, 0x01, 0x00, "ACCESS bus controller"},
            {0x0C, 0x02, 0x00, "SSA controller"},
            {0x0C, 0x03, 0x00, "USB 1.1 Universal Host controller"},
            {0x0C, 0x03, 0x10, "USB 1.1 Open Host controller"},
            {0x0C, 0x03, 0x20, "USB 2.0 IEHCI Host controller"},
            {0x0C, 0x03, 0x20, "USB xHCI Host controller"},
            {0x0C, 0x03, 0x80, "USB controller (non-spec interface)"},
            {0x0C, 0x03, 0xFE, "USB device"},
            {0x0C, 0x04, 0x00, "Fibre Channel controller"},
            {0x0C, 0x05, 0x00, "System Management Bus (SMBus) ctrlr"},
            {0x0C, 0x06, 0x00, "Legacy InfiniBand controller"},
            {0x0C, 0x07, 0x00, "IPMI SMIC interface controller"},
            {0x0C, 0x07, 0x01, "IPMI Keyboard cntrlr style interface"},
            {0x0C, 0x07, 0x02, "IPMI Block transfer interface"},
            {0x0C, 0x08, 0x00, "SERCOS Inerfact Standard (IEC 61491)"},
            {0x0C, 0x09, 0x00, "CANbus controller"},
            {0x0C, 0x80, 0x00, "Other Serial Bus controller"},

        {0x0D, 0xFF, 0xFF, "Wireless controller"},
            {0x0D, 0x00, 0x00, "iRDA-compatible wireless controller"},
            {0x0D, 0x01, 0x00, "Consumer IR wireless controller"},
            {0x0D, 0x01, 0x10, "UWB Radio wireless controller"},
            {0x0D, 0x10, 0x00, "RF wireless controller"},
            {0x0D, 0x11, 0x00, "Bluetooth wireless controller"},
            {0x0D, 0x12, 0x00, "Broadband wireless controller"},
            {0x0D, 0x20, 0x00, "Ethernet 802.11a wireless controller"},
            {0x0D, 0x21, 0x00, "Ethernet 802.11b wireless controller"},
            {0x0D, 0x40, 0x00, "Cellular controller/modem"},
            {0x0D, 0x41, 0x00, "Cellular ctrl/modem + Ethernet 802.11"},
            {0x0D, 0x80, 0x00, "Other wireless controller"},

        {0x0E, 0xFF, 0xFF, "Intelligent I/O controller"},
            {0x0E, 0x00, 0xFF, "I2O Specification 1.0 controller"},
            {0x0E, 0x00, 0x00, "I2O controller (Message FIFO @ 40h)"},

        {0x0F, 0xFF, 0xFF, "Satellite communication controller"},
            {0x0F, 0x01, 0x00, "TV satellite comm controller"},
            {0x0F, 0x02, 0x00, "Audio satellite comm controller"},
            {0x0F, 0x03, 0x00, "Voice satellite comm controller"},
            {0x0F, 0x04, 0x00, "Data satellite comm controller"},
            {0x0F, 0x80, 0x00, "Other satellite comm controller"},

        {0x10, 0xFF, 0xFF, "Encryption/Decryption controller"},
            {0x10, 0x00, 0x00, "Network & computing en/decryption"},
            {0x10, 0x10, 0x00, "Entertainment en/decryption"},
            {0x10, 0x80, 0x00, "Other en/decryption"},

        {0x11, 0xFF, 0xFF, "Data acquisitn & signal processing ctrlr"},
            {0x11, 0x00, 0x00, "DPIO module"},
            {0x11, 0x01, 0x00, "Perfomance counters"},
            {0x11, 0x10, 0x00, "Communications synch test/measure"},
            {0x11, 0x20, 0x00, "Management card"},
            {0x11, 0x80, 0x00, "Other data acquisition or DSP"},

        {0x12, 0xFF, 0xFF, "Processing accelerator"},
            {0x12, 0x00, 0x00, "Processing accelerator (Vendor-spec)"},

        {0x13, 0xFF, 0xFF, "Non-Essential Instrumentation"},
            {0x13, 0x00, 0x00, "Non-Essential Instr Fn (Vendor-spec)"},

        {0xFF, 0xFF, 0xFF, "Undefined"}
    };




/*********************************************************************
 *
 * Function   :  DeviceListCreate
 *
 * Description:
 *
 ********************************************************************/
U16
DeviceListCreate(
    PLX_API_MODE   ApiMode,
    PLX_MODE_PROP *pModeProp
    )
{
    U16                DevNum;
    U16                DevCount;
    U32                RegValue;
    char               DeviceText[100];
    PLX_STATUS         status;
    DEVICE_NODE       *pNode;
    PLX_DEVICE_KEY     Key;
    PLX_DEVICE_OBJECT  DeviceObj;


    DevCount = 0;

    // Find all devices
    DevNum = 0;

    do
    {
        // For non-PCI access, allow user to cancel with 'ESC' key
        if ( (ApiMode == PLX_API_MODE_I2C_AARDVARK) ||
             (ApiMode == PLX_API_MODE_MDIO_SPLICE) ||
             (ApiMode == PLX_API_MODE_SDB) )
        {
            if (Cons_kbhit())
            {
                if (Cons_getch() == 27)
                {
                    return DevCount | (U8)(1 << 15);
                }
            }
        }

        // Clear device key
        memset(&Key, PCI_FIELD_IGNORE, sizeof(PLX_DEVICE_KEY));

        status =
            PlxPci_DeviceFindEx(
                &Key,
                DevNum,
                ApiMode,
                pModeProp
                );

        if (status != PLX_STATUS_OK)
        {
            if (ApiMode == PLX_API_MODE_I2C_AARDVARK)
            {
                // Display error message for I2C
                switch (status)
                {
                    case PLX_STATUS_NO_DRIVER:
                        Cons_printf("Error: No I2C USB devices detected\n");
                        break;

                    case PLX_STATUS_INVALID_DATA:
                        Cons_printf("Error: I2C device %d not detected\n", pModeProp->I2c.I2cPort);
                        break;

                    case PLX_STATUS_IN_USE:
                        Cons_printf("Error: I2C device %d is in-use\n", pModeProp->I2c.I2cPort);
                        break;

                    case PLX_STATUS_INVALID_OBJECT:
                        // No device matched, nothing to display
                        break;

                    default:
                        Cons_printf("Error: I2C status = %03X\n", status);
                        break;
                }
            }
            else if (ApiMode == PLX_API_MODE_MDIO_SPLICE)
            {
                // Display error message for MDIO
                switch (status)
                {
                    case PLX_STATUS_NO_DRIVER:
                        Cons_printf("Error: No MDIO USB devices detected\n");
                        break;

                    case PLX_STATUS_INVALID_DATA:
                        Cons_printf("Error: MDIO device %d not detected\n", pModeProp->Mdio.Port);
                        break;

                    case PLX_STATUS_IN_USE:
                        Cons_printf("Error: MDIO device %d is in-use\n", pModeProp->Mdio.Port);
                        break;

                    case PLX_STATUS_INVALID_OBJECT:
                        // No device matched, nothing to display
                        break;

                    default:
                        Cons_printf("Error: MDIO status = %03X\n", status);
                        break;
                }
            }
            else if (ApiMode == PLX_API_MODE_SDB)
            {
                // Display error message for MDIO
                switch (status)
                {
                    case PLX_STATUS_NO_DRIVER:
                        Cons_printf("Error: COM/TTY port %d not detected\n", pModeProp->Sdb.Port);
                        break;

                    case PLX_STATUS_INVALID_DATA:
                        Cons_printf("Error: No device detected over SDB\n");
                        break;

                    case PLX_STATUS_IN_USE:
                        Cons_printf("Error: COM/TTY port %d in-use\n", pModeProp->Sdb.Port);
                        break;

                    case PLX_STATUS_INVALID_OBJECT:
                        // No device matched, nothing to display
                        break;

                    default:
                        Cons_printf("Error: SDB status = %03X\n", status);
                        break;
                }
            }
        }
        else
        {
            // Add device to list
            pNode = DeviceNodeAdd( &Key );

            // Set device properties
            if (pNode != NULL)
            {
                // Increment device count
                DevCount++;

                // Select device to get properties
                status = PlxPci_DeviceOpen( &pNode->Key, &DeviceObj );

                if (status == PLX_STATUS_OK)
                {
                    // Store PCI header type
                    RegValue =
                        PlxPci_PciRegisterReadFast(
                            &DeviceObj,
                            PCI_REG_HDR_CACHE_LN,
                            NULL
                            );

                    pNode->PciHeaderType = (U8)(RegValue >> 16);

                    // Clear multi-function flag
                    pNode->PciHeaderType &= 0x3F;

                    // Get port properties
                    PlxPci_GetPortProperties( &DeviceObj, &pNode->PortProp );

                    // Store PCI Class Information
                    pNode->PciClass =
                        PlxPci_PciRegisterReadFast(
                            &DeviceObj,
                            PCI_REG_CLASS_REV,
                            NULL
                            );

                    // Remove Revision ID
                    pNode->PciClass >>= 8;

                    // Release the device
                    PlxPci_DeviceClose( &DeviceObj );
                }

                // Ensure status is ok to prevent halting before finding all devices
                status = PLX_STATUS_OK;

                // In non-PCI, display devices as they are detected
                if ( (ApiMode == PLX_API_MODE_I2C_AARDVARK) ||
                     (ApiMode == PLX_API_MODE_MDIO_SPLICE) ||
                     (ApiMode == PLX_API_MODE_SDB) )
                {
                    Device_GetClassString( pNode, DeviceText );

                    // Display initial access-specific string
                    if (ApiMode == PLX_API_MODE_I2C_AARDVARK)
                    {
                        Cons_printf(" Add: %d-%02X", pNode->Key.ApiIndex, pNode->Key.DeviceNumber );
                    }
                    else if (ApiMode == PLX_API_MODE_MDIO_SPLICE)
                    {
                        Cons_printf(" Add: %d", pNode->Key.ApiIndex );
                    }
                    else if (ApiMode == PLX_API_MODE_SDB)
                    {
                        Cons_printf(" Add: COM%d", pNode->Key.ApiIndex );
                    }

                    Cons_printf(
                        ": %04X %02X P%d%s G%dx%d%s [D%d %02X:%02X.%X] - %s\n",
                        pNode->Key.PlxChip, pNode->Key.PlxRevision,
                        pNode->PortProp.PortNumber,
                        (pNode->PortProp.PortNumber < 10) ? " " : "",
                        pNode->PortProp.LinkSpeed, pNode->PortProp.LinkWidth,
                        (pNode->PortProp.LinkWidth < 10) ? " " : "",
                        pNode->Key.domain, pNode->Key.bus, pNode->Key.slot, pNode->Key.function,
                        DeviceText
                        );
                }
            }
        }

        // Increment to next device
        DevNum++;
    }
    while (status == PLX_STATUS_OK);

    return DevCount;
}




/*********************************************************************
 *
 * Function   :  DeviceListDisplay
 *
 * Description:
 *
 ********************************************************************/
void
DeviceListDisplay(
    void
    )
{
    int          index;
    char         PortText[3];
    char         DeviceText[100];
    DEVICE_NODE *pNode;


    Cons_printf(
        "\n"
        "   # D Bs Dv F Pt Dev  Ven  Chip Rv I/M  Description\n"
        " ------------------------------------------------------------------------------\n"
        );

    if (pDeviceList == NULL)
    {
        Cons_printf("               ***** No devices detected *****\n");
        return;
    }

    // Traverse the list and diplay items
    index = 0;
    pNode = pDeviceList;

    while (pNode != NULL)
    {
        if (pNode->bSelected)
        {
            Cons_printf(
                "%s",
                (index < 0x10) ? "=> " :
                (index < 0x100) ? "=>" : ">"
                );
        }
        else
        {
            Cons_printf(
                "%s ",
                (index < 0x10) ? "  " :
                (index < 0x100) ? " " : ""
                );
        }

        // Build port text
        if (pNode->PortProp.bNonPcieDevice)
        {
            sprintf(PortText, "--");
        }
        else
        {
            sprintf(PortText, "%02X", pNode->PortProp.PortNumber);
        }

        // Display device description
        Cons_printf(
            "%X %d %02X %02X %X %s %04X %04X ",
            index,
            pNode->Key.domain,
            pNode->Key.bus,
            pNode->Key.slot,
            pNode->Key.function,
            PortText,
            pNode->Key.DeviceId,
            pNode->Key.VendorId
            );

        if (pNode->Key.PlxChip != 0)
        {
            // Check for 9052 chip, which is 9050 rev 2
            if ((pNode->Key.PlxChip == 0x9050) &&
                (pNode->Key.PlxRevision == 2))
            {
                Cons_printf("9052 01");
            }
            else
            {
                Cons_printf("%04X ", pNode->Key.PlxChip);

                if (pNode->Key.PlxRevision == 0)
                {
                    Cons_printf("??");
                }
                else
                {
                    Cons_printf("%02X", pNode->Key.PlxRevision);
                }
            }
        }
        else
        {
            Cons_printf(" --  --");
        }

        // Display I2C/MDIO/SDB address
        if (pNode->Key.ApiMode == PLX_API_MODE_I2C_AARDVARK)
        {
            Cons_printf(" %d:%02X", pNode->Key.ApiIndex, pNode->Key.DeviceNumber);
        }
        else if (pNode->Key.ApiMode == PLX_API_MODE_MDIO_SPLICE)
        {
            Cons_printf("  M%d ", pNode->Key.ApiIndex);
        }
        else if (pNode->Key.ApiMode == PLX_API_MODE_SDB)
        {
            Cons_printf("  C%d ", pNode->Key.ApiIndex);
        }
        else
        {
            Cons_printf("  -- ");
        }

        Device_GetClassString( pNode, DeviceText );
        Cons_printf(" %s\n", DeviceText);

        // Jump to next node
        index++;
        pNode = pNode->pNext;
    }
}




/*********************************************************************
 *
 * Function   :  DeviceListFree
 *
 * Description:
 *
 ********************************************************************/
void
DeviceListFree(
    void
    )
{
    DEVICE_NODE *pNode;


    // Traverse list and free all nodes
    while (pDeviceList != NULL)
    {
        pNode = pDeviceList;

        pDeviceList = pNode->pNext;

        // Release node
        free( pNode );
    }
}




/*****************************************************************************************
 *
 * Function   :  DeviceNodeAdd
 *
 * Description:  Adds a device node to the PCI device list
 *
 *****************************************************************************************/
DEVICE_NODE *
DeviceNodeAdd(
    PLX_DEVICE_KEY *pKey
    )
{
    BOOLEAN      bAddDevice;
    DEVICE_NODE *pNode;
    DEVICE_NODE *pNodeCurr;
    DEVICE_NODE *pNodePrev;


    // Check if node already exists
    if (DeviceNodeExist( pKey ))
    {
        return NULL;
    }

    // Allocate a new node for the device list
    pNode = (DEVICE_NODE*)malloc(sizeof(DEVICE_NODE));

    if (pNode == NULL)
    {
        return NULL;
    }

    // Clear node
    memset( pNode, 0, sizeof(DEVICE_NODE) );

    // Copy device key information
    pNode->Key = *pKey;

    // Mark the device as not selected
    pNode->bSelected = FALSE;

    /*******************************************
     *
     *      Add node to sorted device list
     *
     ******************************************/

    pNodePrev = NULL;
    pNodeCurr = pDeviceList;

    // Traverse list & add device in proper order
    while (1)
    {
        bAddDevice = FALSE;

        // Check for end-of-list
        if (pNodeCurr == NULL)
        {
            bAddDevice = TRUE;
        }
        else
        {
            // For I2C/MDIO/SDB mode, skip adding device
            // until end of list is reached

            // Add for PCI mode
            if (pKey->ApiMode == pNodeCurr->Key.ApiMode)
            {
                if ((pNodeCurr->Key.ApiMode == PLX_API_MODE_I2C_AARDVARK) &&
                    ((pKey->ApiIndex > pNodeCurr->Key.ApiIndex) ||
                     (pKey->DeviceNumber > pNodeCurr->Key.DeviceNumber)))
                {
                    // I2C device at higher port/address, add lower in list
                }
                else if ((pNodeCurr->Key.ApiMode == PLX_API_MODE_MDIO_SPLICE) &&
                         (pKey->ApiIndex > pNodeCurr->Key.ApiIndex))
                {
                    // MDIO device at higher port, add lower in list
                }
                else if ((pNodeCurr->Key.ApiMode == PLX_API_MODE_SDB) &&
                         (pKey->ApiIndex > pNodeCurr->Key.ApiIndex))
                {
                    // SDB device at higher port, add lower in list
                }
                else
                {
                    // Compare domain, bus, slot, function numbers
                    if (pKey->domain < pNodeCurr->Key.domain)
                    {
                        bAddDevice = TRUE;
                    }
                    else if (pKey->domain == pNodeCurr->Key.domain)
                    {
                        if (pKey->bus < pNodeCurr->Key.bus)
                        {
                            bAddDevice = TRUE;
                        }
                        else if (pKey->bus == pNodeCurr->Key.bus)
                        {
                            if (pKey->slot < pNodeCurr->Key.slot)
                            {
                                bAddDevice = TRUE;
                            }
                            else if (pKey->slot == pNodeCurr->Key.slot)
                            {
                                if (pKey->function <= pNodeCurr->Key.function)
                                {
                                    bAddDevice = TRUE;
                                }
                            }
                        }
                    }
                }
            }
        }

        if (bAddDevice)
        {
            // Add node to list in current position
            if (pNodePrev == NULL)
            {
                pDeviceList = pNode;
            }
            else
            {
                pNodePrev->pNext = pNode;
            }

            pNode->pNext = pNodeCurr;
            return pNode;
        }

        // Store previous node
        pNodePrev = pNodeCurr;

        // Jump to next node
        pNodeCurr = pNodeCurr->pNext;
    }
}




/*****************************************************************************************
 *
 * Function   :  DeviceNodeExist
 *
 * Description:  Returns TRUE is a device already exists in the PCI device list
 *
 *****************************************************************************************/
BOOL
DeviceNodeExist(
    PLX_DEVICE_KEY *pKey
    )
{
    DEVICE_NODE *pNode;


    pNode = pDeviceList;

    // Traverse list and compare with existing devices
    while (pNode != NULL)
    {
        // Check for a match by location and method accessed
        if ((pKey->domain   == pNode->Key.domain)   &&
            (pKey->bus      == pNode->Key.bus)      &&
            (pKey->slot     == pNode->Key.slot)     &&
            (pKey->function == pNode->Key.function) &&
            (pKey->ApiMode  == pNode->Key.ApiMode))
        {
            if (pKey->ApiMode == PLX_API_MODE_PCI)
            {
                return TRUE;
            }
            else if (pKey->ApiMode == PLX_API_MODE_I2C_AARDVARK)
            {
                // For I2C, also compare I2C USB device, I2C address, & PLX port
                if ((pKey->ApiIndex     == pNode->Key.ApiIndex) &&
                    (pKey->PlxPort      == pNode->Key.PlxPort)  &&
                    (pKey->DeviceNumber == pNode->Key.DeviceNumber))
                {
                    return TRUE;
                }
            }
            else if (pKey->ApiMode == PLX_API_MODE_MDIO_SPLICE)
            {
                // For MDIO, also compare MDIO USB device & PLX port
                if ((pKey->ApiIndex == pNode->Key.ApiIndex) &&
                    (pKey->PlxPort  == pNode->Key.PlxPort))
                {
                    return TRUE;
                }
            }
            else if (pKey->ApiMode == PLX_API_MODE_SDB)
            {
                // For SDB, also compare SDB COM port & PLX port
                if ((pKey->ApiIndex == pNode->Key.ApiIndex) &&
                    (pKey->PlxPort  == pNode->Key.PlxPort))
                {
                    return TRUE;
                }
            }
        }

        // Go to next device
        pNode = pNode->pNext;
    }

    return FALSE;
}




/******************************************************************************
 *
 * Function   :  DeviceNodeGetByNum
 *
 * Description:
 *
 ******************************************************************************/
DEVICE_NODE*
DeviceNodeGetByNum(
    U16     index,
    BOOLEAN bPlxOnly
    )
{
    U16          count;
    DEVICE_NODE *pNode;


    // Traverse the list to return desired node
    count = 0;
    pNode = pDeviceList;

    while (pNode != NULL)
    {
        if (bPlxOnly)
        {
            // Increment count only if a PLX device
            if (pNode->Key.PlxChip != 0)
            {
                count++;
            }
        }
        else
        {
            count++;
        }

        // Check if desired device
        if (index == (count - 1))
        {
            return pNode;
        }

        // Jump to next node
        pNode = pNode->pNext;
    }

    // If no PLX devices, return first device
    if (bPlxOnly)
    {
        return pDeviceList;
    }

    return NULL;
}




/******************************************************************************
 *
 * Function   :  DeviceNodeGetByKey
 *
 * Description:
 *
 ******************************************************************************/
DEVICE_NODE*
DeviceNodeGetByKey(
    PLX_DEVICE_KEY *pKey
    )
{
    DEVICE_NODE *pNode;


    // Traverse the list to return desired node
    pNode = pDeviceList;

    while (pNode != NULL)
    {
        // Compare keys
        if ((pKey->domain       == pNode->Key.domain)   &&
            (pKey->bus          == pNode->Key.bus)      &&
            (pKey->slot         == pNode->Key.slot)     &&
            (pKey->function     == pNode->Key.function) &&
            (pKey->ApiMode      == pNode->Key.ApiMode)  &&
            (pKey->ApiIndex     == pNode->Key.ApiIndex) &&
            (pKey->DeviceNumber == pNode->Key.DeviceNumber))
        {
            return pNode;
        }

        // Jump to next node
        pNode = pNode->pNext;
    }

    return NULL;
}




/******************************************************************************
 *
 * Function   :  Device_GetClassString
 *
 * Description:
 *
 ******************************************************************************/
VOID
Device_GetClassString(
    DEVICE_NODE *pNode,
    char        *pClassText
    )
{
    U16 Index_BestFit;
    U16 Index_Current;
    U16 BestScore;
    U16 CurrentScore;
    U32 PlxChip;
    U32 PciClass;


    // Get device class code
    PciClass = pNode->PciClass;

    // Start with empty string
    pClassText[0] = '\0';

    // Get chip type
    PlxChip = pNode->Key.PlxChip;

    // Check for device name overrides
    switch (PlxChip)
    {
        case 0x9050:
        case 0x9030:
        case 0x9080:
        case 0x9054:
        case 0x9056:
        case 0x9656:
            strcpy( pClassText, "PLX PCI <==> Local bus bridge" );
            break;

        case 0x8311:
            strcpy( pClassText, "PLX PCIe <==> Local bus bridge" );
            break;

        case 0x6140:
        case 0x6152:
        case 0x6154:
        case 0x6156:
        case 0x6350:
        case 0x6150:
        case 0x6520:
        case 0x6254:
        case 0x6540:
        case 0x6466:
            if (pNode->Key.PlxPortType == PLX_SPEC_PORT_NT_VIRTUAL)
            {
                strcpy( pClassText, "PLX PCI NT Primary-side" );
            }
            else if (pNode->Key.PlxPortType == PLX_SPEC_PORT_NT_LINK)
            {
                strcpy( pClassText, "PLX PCI NT Secondary-side" );
            }
            else
            {
                strcpy( pClassText, "PLX PCI <==> PCI bridge" );
            }
            break;

        case 0x8111:
        case 0x8112:
            strcpy( pClassText, "PLX PCIe <==> PCI bridge" );
            break;

        case 0x8114:
            strcpy( pClassText, "PLX PCIe <==> PCI-X bridge" );
            break;

        case 0x0:
            // Not PLX/Broadcom chip
            break;

        default:
            //
            // All other PLX/Broadcom devices (2000/8000/9000/C000/etc)
            //

            // Default to unknown EP
            strcpy( pClassText, "* Broadcom unknown endpoint *" );

            if (pNode->PortProp.PortType == PLX_PORT_UPSTREAM)
            {
                if (pNode->Key.SubDeviceId == 0x100B)
                {
                    strcpy( pClassText, "Broadcom synthetic PCIe upstream" );
                }
                else if (pNode->Key.PlxPortType == PLX_SPEC_PORT_HOST)
                {
                    strcpy( pClassText, "Broadcom PCIe host port" );
                }
                else
                {
                    strcpy( pClassText, "Broadcom PCIe upstream port" );
                }
            }
            else if (pNode->PortProp.PortType == PLX_PORT_DOWNSTREAM)
            {
                if (pNode->Key.SubDeviceId == 0x100B)
                {
                    strcpy( pClassText, "Broadcom synthetic PCIe downstream" );
                }
                else if (pNode->Key.PlxPortType == PLX_SPEC_PORT_FABRIC)
                {
                    strcpy( pClassText, "Broadcom PCIe fabric port" );
                }
                else
                {
                    strcpy( pClassText, "Broadcom PCIe downstream port" );
                }
            }
            else if (pNode->PortProp.PortType == PLX_PORT_ENDPOINT)
            {
                if (pNode->PciClass == 0x068000)        // Bridge devices
                {
                    if (pNode->Key.PlxPortType == PLX_SPEC_PORT_SYNTH_TWC)
                    {
                        strcpy( pClassText, "Broadcom synthetic TWC endpoint" );
                    }
                    else if (pNode->Key.PlxPortType == PLX_SPEC_PORT_NT_VIRTUAL)
                    {
                        strcpy( pClassText, "Broadcom PCIe NT virtual port" );
                    }
                    else if (pNode->Key.PlxPortType == PLX_SPEC_PORT_NT_LINK)
                    {
                        strcpy( pClassText, "Broadcom PCIe NT link port" );
                    }
                    else if (pNode->Key.PlxPortType == PLX_SPEC_PORT_SYNTH_NT)
                    {
                        strcpy( pClassText, "Broadcom synthetic NT 2.0 endpoint" );
                    }
                }
                else if (pNode->PciClass == 0x088000)   // Peripheral devices
                {
                    if (pNode->Key.PlxPortType == PLX_SPEC_PORT_GEP)
                    {
                        strcpy( pClassText, "Broadcom Global Endpoint (GEP)" );
                    }
                    else if (pNode->Key.PlxPortType == PLX_SPEC_PORT_SYNTH_EN_EP)
                    {
                        strcpy( pClassText, "Broadcom synthetic enabler endpoint" );
                    }
                    else if (pNode->Key.PlxPortType == PLX_SPEC_PORT_DMA)
                    {
                        strcpy( pClassText, "Broadcom PCIe DMA controller" );
                    }
                    else if (pNode->Key.PlxPortType == PLX_SPEC_PORT_SYNTH_GDMA)
                    {
                        strcpy( pClassText, "Broadcom synthetic gDMA endpoint" );
                    }
                }
                else if (pNode->PciClass == 0x028000)   // Network controllers
                {
                    if (pNode->Key.PlxPortType == PLX_SPEC_PORT_SYNTH_NIC)
                    {
                        strcpy( pClassText, "Broadcom synthetic NIC endpoint" );
                    }
                    else
                    {
                        strcpy( pClassText, "Broadcom PCIe ExpressNIC (NT)" );
                    }
                }
                else if (pNode->PciClass == 0x020000)
                {
                    strcpy( pClassText, "Broadcom PCIe network controller" );
                }
                else if (pNode->Key.PlxPortType == PLX_SPEC_PORT_MPT)
                {
                    strcpy( pClassText, "Broadcom MPT SES endpoint" );
                }
                else if (pNode->Key.PlxPortType == PLX_SPEC_PORT_MPT_NO_SES)
                {
                    strcpy( pClassText, "Broadcom MPT endpoint (No SES)" );
                }
                else if (pNode->Key.PlxPortType == PLX_SPEC_PORT_SYNTH_MPT)
                {
                    strcpy( pClassText, "Broadcom synthetic MPT SES endpoint" );
                }
            }
            else if (pNode->PortProp.PortType == PLX_PORT_LEGACY_ENDPOINT)
            {
                if (pNode->PciClass == 0x0C03FE)
                {
                    strcpy( pClassText, "Broadcom USB controller" );
                }
                else
                {
                    strcpy( pClassText, "* Broadcom unknown legacy endpoint *" );
                }
            }
            else
            {
                strcpy( pClassText, "* Broadcom PCIe unknown port type *" );
            }
            break;
    }

    // Check for standard PCIe port types
    if ((pClassText[0] == '\0') && (pNode->PortProp.bNonPcieDevice == FALSE))
    {
        if (pNode->PortProp.PortType == PLX_PORT_UPSTREAM)
        {
            strcpy( pClassText, "PCIe upstream port" );
        }
        else if (pNode->PortProp.PortType == PLX_PORT_DOWNSTREAM)
        {
            strcpy( pClassText, "PCIe downstream port" );
        }
        else if (pNode->PortProp.PortType == PLX_PORT_ROOT_PORT)
        {
            strcpy( pClassText, "PCIe Root Complex Root Port" );
        }
    }

    // Halt if already have match
    if (pClassText[0] != '\0')
    {
        return;
    }

    Index_BestFit = 0;
    Index_Current = 0;

    BestScore = 0;

    do
    {
        // Reset the score
        CurrentScore = 0;

        // Check for match on base class
        if (((U8)(PciClass >> 16)) == PciClassCodes[Index_Current].BaseClass)
        {
            CurrentScore = MATCH_BASE_EXACT;
        }
        else
        {
            if (PciClassCodes[Index_Current].BaseClass == 0xFF)
            {
                CurrentScore = MATCH_BASE_GENERIC;
            }
        }

        // Check for match on sub class
        if (CurrentScore & (MATCH_BASE))
        {
            if (((U8)(PciClass >> 8)) ==
                PciClassCodes[Index_Current].SubClass)
            {
                CurrentScore += MATCH_SUBCLASS_EXACT;
            }
            else
            {
                if (PciClassCodes[Index_Current].SubClass == 0xFF)
                {
                    CurrentScore += MATCH_SUBCLASS_GENERIC;
                }
            }
        }

        // Check for match on interface
        if (CurrentScore & (MATCH_SUBCLASS))
        {
            if (((U8)(PciClass >> 0)) ==
                PciClassCodes[Index_Current].Interface)
            {
                CurrentScore += MATCH_INTERFACE_EXACT;
            }
            else
            {
                if (PciClassCodes[Index_Current].Interface == 0xFF)
                {
                    CurrentScore += MATCH_INTERFACE_GENERIC;
                }
            }
        }

        // Check if we have a better match
        if (CurrentScore > BestScore)
        {
            BestScore     = CurrentScore;
            Index_BestFit = Index_Current;
        }

        // Increment to next class
        Index_Current++;
    }
    while (PciClassCodes[Index_Current].BaseClass != 0xFF);

    strcpy(
        pClassText,
        PciClassCodes[Index_BestFit].StrClass
        );
}




/******************************************************************************
 *
 * Function   :  Plx_EepromFileLoad
 *
 * Description:  Saves the contents of the EEPROM to a file
 *
 ******************************************************************************/
BOOLEAN
Plx_EepromFileLoad(
    PLX_DEVICE_OBJECT *pDevice,
    char              *pFileName,
    U16                EepSize,
    U8                 EepPortSize,
    BOOLEAN            bCrc,
    BOOLEAN            bBypassVerify,
    BOOLEAN            bEndianSwap
    )
{
    S8       rc;
    U32      offset;
    U32      Crc;
    U32      value;
    U32      Verify_Value;
    U32      FileSize;
    U32     *pBuffer;
    FILE    *pFile;
    clock_t  start;
    clock_t  end;


    // Note start time
    start = clock();

    Cons_printf("Load EEPROM file....... ");
    Cons_fflush( stdout );

    pBuffer = NULL;

    // Open the file to read
    pFile = fopen( pFileName, "rb" );
    if (pFile == NULL)
    {
        Cons_printf("ERROR: Unable to load \"%s\"\n", pFileName);
        return FALSE;
    }

    // Move to end-of-file
    fseek( pFile, 0, SEEK_END );

    // Determine file size
    FileSize = ftell( pFile );

    // Move back to start of file
    fseek( pFile, 0, SEEK_SET );

    // Allocate a buffer for the data
    pBuffer = malloc( FileSize );
    if (pBuffer == NULL)
    {
        fclose( pFile );
        return FALSE;
    }

    // Read data from file
    if (fread(
            pBuffer,        // Buffer for data
            sizeof(U8),     // Item size
            FileSize,       // Buffer size
            pFile           // File pointer
            ) == 0)
    {
        // Avoid compiler warning
    }

    // Close the file
    fclose( pFile );

    Cons_printf("Ok (%dB)\n", FileSize);

    if (FileSize < EepSize)
    {
        Cons_printf("WARNING: File is less than minimum size (%d B)\n", EepSize);
    }

    // Default to successful operation
    rc = TRUE;

    Cons_printf(
        "Verify option.......... %s\n",
        bBypassVerify ? "DISABLED (Errors won't be detected)"
          : "ENABLED (Use '-b' to disable)"
        );

    Cons_printf("Program EEPROM......... ");

    // Write buffer into EEPROM
    for (offset = 0; offset < FileSize; offset += sizeof(U32))
    {
        // Periodically update status
        if ((offset & 0xF) == 0)
        {
            Cons_printf(
                "%02d%%\b\b\b",
                (U16)((offset * 100) / FileSize)
                );
            Cons_fflush( stdout );
        }

        // Get next value to write
        value = pBuffer[offset / sizeof(U32)];

        // Endian swap if requested to
        if (bEndianSwap)
        {
            // Endian swap based on port size
            if (EepPortSize == sizeof(U16))
            {
                value = EndianSwap16(value);
            }
            else
            {
                value = EndianSwap32(value);
            }
        }

        // Store next value
        PlxPci_EepromWriteByOffset( pDevice, offset, value );

        // Verify value unless ignore option enabled
        if (bBypassVerify)
        {
            Verify_Value = value;
        }
        else
        {
            PlxPci_EepromReadByOffset( pDevice, offset, &Verify_Value );
        }

        if (Verify_Value != value)
        {
            Cons_printf(
                "ERROR: offset:%02X  wrote:%08X  read:%08X\n",
                offset, value, Verify_Value
                );
            rc = FALSE;
            goto _Exit_File_Load;
        }
    }

    Cons_printf("Ok \n");

    // Update CRC if requested
    if (bCrc)
    {
        Cons_printf("Update CRC............. ");
        Cons_fflush( stdout );
        PlxPci_EepromCrcUpdate(
            pDevice,
            &Crc,
            TRUE
            );
        Cons_printf("Ok (CRC=%08X)\n", (int)Crc);
    }

_Exit_File_Load:
    // Release the buffer
    if (pBuffer != NULL)
    {
        free( pBuffer );
    }

    // Note completion time
    end = clock();

    Cons_printf(
        " -- Complete (%.2f sec) --\n",
        ((double)end - start) / CLOCKS_PER_SEC
        );

    return rc;
}




/******************************************************************************
 *
 * Function   :  Plx_EepromFileSave
 *
 * Description:  Saves the contents of the EEPROM to a file
 *
 ******************************************************************************/
BOOLEAN
Plx_EepromFileSave(
    PLX_DEVICE_OBJECT *pDevice,
    char              *pFileName,
    U32                EepSize,
    U8                 EepPortSize,
    BOOLEAN            bCrc,
    BOOLEAN            bEndianSwap
    )
{
    U32      offset;
    U32      value;
    FILE    *pFile;
    clock_t  start;
    clock_t  end;


    // Note start time
    start = clock();

    // Open the file to write
    pFile = fopen( pFileName, "wb" );
    if (pFile == NULL)
    {
        return FALSE;

    }

    // Adjust EEPROM size for devices with CRC
    if (bCrc)
    {
        EepSize += sizeof(U32);
    }

    Cons_printf(
        "Write EEPROM data to file \"%s\".....",
        pFileName
        );

    // Read EEPROM and write to file
    for (offset = 0; offset < EepSize; offset += sizeof(U32))
    {
        // Periodically update status
        if ((offset & 0xF) == 0)
        {
            Cons_printf(
                "%02d%%\b\b\b",
                (U16)((offset * 100) / EepSize)
                );
            Cons_fflush( stdout );
        }

        // Get next data to write
        PlxPci_EepromReadByOffset( pDevice, offset, &value );

        // Endian swap if requested to
        if (bEndianSwap)
        {
            // Endian swap based on port size
            if (EepPortSize == sizeof(U16))
            {
                value = EndianSwap16(value);
            }
            else
            {
                value = EndianSwap32(value);
            }
        }

        // Write data to file
        if (fwrite(
                &value,         // Buffer to write
                sizeof(U32),    // Item size
                1,              // Item count
                pFile           // File pointer
                ) == 0)
        {
            // Avoid compiler warning
        }
    }

    // In case of 8114 revision, some versions require an
    // extra 32-bit '0' after CRC due to an erratum
    if (pDevice->Key.PlxChip == 0x8114)
    {
        value = 0;

        // Write data to file
        if (fwrite(
                &value,         // Buffer to write
                sizeof(U32),    // Item size
                1,              // Item count
                pFile           // File pointer
                ) == 0)
        {
            // Avoid compiler warning
        }

        // Update EEPROM size
        EepSize += sizeof(U32);
    }

    // Close the file
    fclose( pFile );

    Cons_printf("Ok (%d B)\n", (int)EepSize);

    // Note completion time
    end = clock();

    Cons_printf(
        " -- Complete (%.2f sec) --\n",
        ((double)end - start) / CLOCKS_PER_SEC
        );

    return TRUE;
}




/******************************************************************************
 *
 * Function   :  Plx8000_EepromFileLoad
 *
 * Description:  Saves the contents of the EEPROM to a file
 *
 ******************************************************************************/
BOOLEAN
Plx8000_EepromFileLoad(
    PLX_DEVICE_OBJECT *pDevice,
    char              *pFileName,
    BOOLEAN            bCrc,
    BOOLEAN            bBypassVerify
    )
{
    U8       bCrcEn;
    U8      *pBuffer;
    U16      Verify_Value_16;
    U32      value;
    U32      Verify_Value;
    U32      Crc;
    U32      offset;
    U32      FileSize;
    U32      EepHeader;
    FILE    *pFile;
    clock_t  start;
    clock_t  end;


    // Note start time
    start = clock();

    pBuffer   = NULL;
    EepHeader = 0;

    // Start with CRC option matching CRC support
    bCrcEn = bCrc;

    Cons_printf( "Load EEPROM file... " );
    Cons_fflush( stdout );

    // Open the file to read
    pFile = fopen( pFileName, "rb" );
    if (pFile == NULL)
    {
        Cons_printf("ERROR: Unable to load \"%s\"\n", pFileName);
        return FALSE;
    }

    // Move to end-of-file
    fseek( pFile, 0, SEEK_END );

    // Determine file size
    FileSize = ftell( pFile );

    // Move back to start of file
    fseek( pFile, 0, SEEK_SET );

    // Allocate a buffer for the data
    pBuffer = malloc( FileSize );
    if (pBuffer == NULL)
    {
        fclose( pFile );
        return FALSE;
    }

    // Read data from file
    if (fread(
            pBuffer,        // Buffer for data
            sizeof(U8),     // Item size
            FileSize,       // Buffer size
            pFile           // File pointer
            ) == 0)
    {
        // Avoid compiler warning
    }

    // Close the file
    fclose( pFile );

    Cons_printf( "Ok (%dB)\n", (int)FileSize );
    Cons_printf(
        "Verify option...... %s\n",
        bBypassVerify ? "DISABLED (Errors won't be detected)"
          : "ENABLED (Use '-b' to disable)"
        );

    Cons_printf("Program EEPROM..... ");

    // Write 32-bit aligned buffer into EEPROM
    for (offset = 0; offset < (FileSize & ~0x3); offset += sizeof(U32))
    {
        // Periodically update status
        if ((offset & 0x7) == 0)
        {
            // Display current status
            Cons_printf(
                "%02ld%%\b\b\b",
                ((offset * 100) / FileSize)
                );
            Cons_fflush( stdout );
        }

        // Get next value
        value = *(U32*)(pBuffer + offset);

        // For chips that support CRC, verify CRC is enabled (0h[15])
        if (offset == 0)
        {
            // Store EEPROM header
            EepHeader = value;

            // Remove CRC option if disabled
            if ((bCrcEn == TRUE) && ((value & (1 << 15)) == 0))
            {
                bCrcEn = FALSE;
            }
        }

        // Write value & read back to verify
        PlxPci_EepromWriteByOffset( pDevice, offset, value );

        // Verify value unless ignore option enabled
        if (bBypassVerify)
        {
            Verify_Value = value;
        }
        else
        {
            PlxPci_EepromReadByOffset( pDevice, offset, &Verify_Value );
        }

        if (Verify_Value != value)
        {
            Cons_printf(
                "ERROR: offset:%02X  wrote:%08X  read:%08X\n",
                offset, value, Verify_Value
                );
            goto _Exit_File_Load_8000;
        }
    }

    // Write any remaing 16-bit unaligned value
    if (offset < FileSize)
    {
        // Get next value
        value = *(U16*)(pBuffer + offset);

        // Write value & read back to verify
        PlxPci_EepromWriteByOffset_16( pDevice, offset, (U16)value );

        // Verify value unless ignore option enabled
        if (bBypassVerify)
        {
            Verify_Value_16 = (U16)value;
        }
        else
        {
            PlxPci_EepromReadByOffset_16( pDevice, offset, &Verify_Value_16 );
        }

        if (Verify_Value_16 != (U16)value)
        {
            Cons_printf(
                "ERROR: offset:%02X  wrote:%04X  read:%04X\n",
                offset, value, Verify_Value_16
                );
            goto _Exit_File_Load_8000;
        }
    }
    Cons_printf( "Ok \n" );

    // Update CRC if requested
    if (bCrc)
    {
        Cons_printf("Update CRC......... ");
        if (bCrcEn == FALSE)
        {
            Cons_printf("*DISABLED*\n");
        }
        else
        {
            Cons_fflush( stdout );
            PlxPci_EepromCrcUpdate( pDevice, &Crc, TRUE );
            Cons_printf(
                "Ok (CRC=%08X offset=%02Xh)\n",
                (int)Crc, (EepHeader >> 16) + sizeof(U32)
                );
        }
    }

_Exit_File_Load_8000:
    // Release the buffer
    if (pBuffer != NULL)
    {
        free( pBuffer );
    }

    // Note completion time
    end = clock();

    Cons_printf(
        " -- Complete (%.2f sec) --\n",
        ((double)end - start) / CLOCKS_PER_SEC
        );

    return TRUE;
}




/******************************************************************************
 *
 * Function   :  Plx8000_EepromFileSave
 *
 * Description:  Saves the contents of the EEPROM to a file
 *
 ******************************************************************************/
BOOLEAN
Plx8000_EepromFileSave(
    PLX_DEVICE_OBJECT *pDevice,
    char              *pFileName,
    U32                ByteCount,
    BOOLEAN            bCrc
    )
{
    U8      *pBuffer;
    U16      value16;
    U32      value;
    U32      offset;
    U32      EepSize;
    FILE    *pFile;
    clock_t  start;
    clock_t  end;


    // Note start time
    start = clock();

    Cons_printf("Get EEPROM data size.. ");

    pBuffer = NULL;

    if (ByteCount != 0)
    {
        EepSize = ByteCount;
    }
    else
    {
        // Start with EEPROM header size
        EepSize = sizeof(U32);

        // Get EEPROM register count
        PlxPci_EepromReadByOffset( pDevice, 0x0, &value );

        // Add register byte count
        EepSize += (value >> 16);

        // Add CRC count if supported
        if (bCrc)
        {
            // Remove CRC option if disabled
            if (value & (1 << 15))
            {
                EepSize += sizeof(U32);
            }
            else
            {
                bCrc = FALSE;
            }
        }

        // Get Shared(811x)/8051(MIRA) memory count
        if ((pDevice->Key.PlxFamily == PLX_FAMILY_BRIDGE_PCIE_P2P) ||
            (pDevice->Key.PlxFamily == PLX_FAMILY_MIRA))
        {
            PlxPci_EepromReadByOffset_16( pDevice, EepSize, &value16 );

            // Get byte count
            value16 = value16 & 0xFF;

            // Add mem byte count
            EepSize += sizeof(U16);

            // Add mem data
            EepSize += value16;
        }
   }

    Cons_printf(
        "Ok (%dB%s)\n",
        EepSize,
        (bCrc) ? " inc 32-bit CRC" : ""
        );

    Cons_printf("Read EEPROM data...... ");
    Cons_fflush( stdout );

    // Allocate a buffer for the EEPROM data
    pBuffer = malloc( EepSize );
    if (pBuffer == NULL)
    {
        return FALSE;
    }

    // Read 32-bit aligned EEPROM data into buffer
    for (offset = 0; offset < (EepSize & ~0x3); offset += sizeof(U32))
    {
        // Periodically update status
        if ((offset & 0x7) == 0)
        {
            // Display current status
            Cons_printf(
                "%02ld%%\b\b\b",
                ((offset * 100) / EepSize)
                );
            Cons_fflush( stdout );
        }

        PlxPci_EepromReadByOffset( pDevice, offset, (U32*)(pBuffer + offset) );
    }

    // Read any remaining 16-bit aligned byte
    if (offset < EepSize)
    {
        PlxPci_EepromReadByOffset_16( pDevice, offset, (U16*)(pBuffer + offset) );
    }
    Cons_printf( "Ok \n" );

    Cons_printf("Write data to file.... ");
    Cons_fflush(stdout);

    // Open the file to write
    pFile = fopen( pFileName, "wb" );
    if (pFile == NULL)
    {
        return FALSE;
    }

    // Write buffer to file
    if (fwrite(
            pBuffer,        // Buffer to write
            sizeof(U8),     // Item size
            EepSize,        // Buffer size
            pFile           // File pointer
            ) == 0)
    {
        // Avoid compiler warning
    }

    // Close the file
    fclose( pFile );

    // Release the buffer
    if (pBuffer != NULL)
    {
        free( pBuffer );
    }

    Cons_printf( "Ok (%s)\n", pFileName );

    // Note completion time
    end = clock();

    Cons_printf(
        " -- Complete (%.2f sec) --\n",
        ((double)end - start) / CLOCKS_PER_SEC
        );

    return TRUE;
}
