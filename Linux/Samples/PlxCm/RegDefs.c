/******************************************************************************
 *
 * File Name:
 *
 *      RegDefs.c
 *
 * Description:
 *
 *      Register defintions
 *
 ******************************************************************************/


#include "RegDefs.h"




/*************************************************
 *          Default PCI Type 0 Header
 ************************************************/
REGISTER_SET Pci_Type_0[] =
{
    {0x000, "Device ID | Vendor ID"},
    {0x004, "Status | Command"},
    {0x008, "Base Class | Sub Class | Interface | Revision ID"},
    {0x00c, "BIST | Header Type | Latency Timer | Cache Line Size"},
    {0x010, "PCI Base Address 0"},
    {0x014, "PCI Base Address 1"},
    {0x018, "PCI Base Address 2"},
    {0x01c, "PCI Base Address 3"},
    {0x020, "PCI Base Address 4"},
    {0x024, "PCI Base Address 5"},
    {0x028, "Cardbus CIS Pointer"},
    {0x02c, "Subsystem Device ID | Subsystem Vendor ID"},
    {0x030, "Expansion ROM PCI Base Address"},
    {0x034, "Extended Capability Pointer"},
    {0x038, "Reserved"},
    {0x03c, "Max Lat | Min Grant | Interrupt Pin | Interrupt Line"},
    {0xFFF, ""}
};




/*************************************************
 *           PCI Type 1 Header
 ************************************************/
REGISTER_SET Pci_Type_1[] =
{
    {0x000, "Device ID | Vendor ID"},
    {0x004, "Status | Command"},
    {0x008, "Base Class | Sub Class | Interface | Revision ID"},
    {0x00c, "BIST | Header Type | Prim Latency Timer | Cacheline Size"},
    {0x010, "PCI Base Address 0"},
    {0x014, "PCI Base Address 1"},
    {0x018, "Sec Lat Timer | Sub Bus Num | Sec Bus Num | Prim Bus Num"},
    {0x01c, "Secondary Status | I/O Limit | I/O Base"},
    {0x020, "Memory Limit | Memory Base"},
    {0x024, "Prefetchable Memory Limit | Prefetchable Memory Base"},
    {0x028, "Prefetchable Base (Upper 32-bits)"},
    {0x02c, "Prefetchable Limit (Upper 32-bits)"},
    {0x030, "I/O Limit (Upper 16-bits) | I/O Base (Upper 16-bits)"},
    {0x034, "Extended Capability Pointer"},
    {0x038, "Expansion ROM Base Address"},
    {0x03c, "Bridge Control | Interrupt Pin | Interrupt Line"},
    {0xFFF, ""}
};




/*************************************************
 *       Default PCI Type 2 Header
 ************************************************/
REGISTER_SET Pci_Type_2[] =
{
    {0x000, "Device ID | Vendor ID"},
    {0x004, "Status | Command"},
    {0x008, "Base Class | Sub Class | Interface | Revision ID"},
    {0x00c, "BIST | Header Type | Prim Latency Timer | Cacheline Size"},
    {0x010, "Socket Registers PCI Base Address"},
    {0x014, "Secondary Status | Reserved | Capabilities Pointer"},
    {0x018, "Sec Lat Timer | Sub Bus Num | Sec Bus Num | Prim Bus Num"},
    {0x01c, "Memory Base 0"},
    {0x020, "Memory Limit 0"},
    {0x024, "Memory Base 1"},
    {0x028, "Memory Limit 1"},
    {0x02c, "I/O Base 0"},
    {0x030, "I/O Limit 0"},
    {0x034, "I/O Base 1"},
    {0x038, "I/O Limit 1"},
    {0x03c, "Bridge Control | Interrupt Pin | Interrupt Line"},
    {0xFFF, ""}
};




/*************************************************
 *            9050 and 9052 Registers
 ************************************************/
REGISTER_SET Lcr9050[] =
{
    {0x000, "Space 0 Range"},
    {0x004, "Space 1 Range"},
    {0x008, "Space 2 Range"},
    {0x00c, "Space 3 Range"},
    {0x010, "Expansion ROM Range"},
    {0x014, "Space 0 Local Base Address (Remap)"},
    {0x018, "Space 1 Local Base Address (Remap)"},
    {0x01c, "Space 2 Local Base Address (Remap)"},
    {0x020, "Space 3 Local Base Address (Remap)"},
    {0x024, "Expansion ROM Local Base Address (Remap)"},
    {0x028, "Space 0 Bus Region Descriptor"},
    {0x02c, "Space 1 Bus Region Descriptor"},
    {0x030, "Space 2 Bus Region Descriptor"},
    {0x034, "Space 3 Bus Region Descriptor"},
    {0x038, "Expansion ROM Bus Region Descriptor"},
    {0x03c, "Chip Select 0 Base Address"},
    {0x040, "Chip Select 1 Base Address"},
    {0x044, "Chip Select 2 Base Address"},
    {0x048, "Chip Select 3 Base Address"},
    {0x04c, "Interrupt Control/Status"},
    {0x050, "EEPROM Ctrl | PCI Target Response | User I/O | Init Ctrl"},
    {0xFFF, ""}
};



REGISTER_SET Eep9050[] =
{
    {0x000, "Device ID | Vendor ID"},
    {0x004, "Class Code | Revision ID"},
    {0x008, "Subsystem Device ID | Subsystem Vendor ID"},
    {0x00c, "Interrupt Pin | Interrupt Line"},
    {0x010, "Space 0 Range"},
    {0x014, "Space 1 Range"},
    {0x018, "Space 2 Range"},
    {0x01c, "Space 3 Range"},
    {0x020, "Expansion ROM Range"},
    {0x024, "Space 0 Local Base Address (Remap)"},
    {0x028, "Space 1 Local Base Address (Remap)"},
    {0x02c, "Space 2 Local Base Address (Remap)"},
    {0x030, "Space 3 Local Base Address (Remap)"},
    {0x034, "Expansion ROM Local Base Address (Remap)"},
    {0x038, "Space 0 Bus Region Descriptor"},
    {0x03c, "Space 1 Bus Region Descriptor"},
    {0x040, "Space 2 Bus Region Descriptor"},
    {0x044, "Space 3 Bus Region Descriptor"},
    {0x048, "Expansion ROM Bus Region Descriptor"},
    {0x04c, "Chip Select 0 Base Address"},
    {0x050, "Chip Select 1 Base Address"},
    {0x054, "Chip Select 2 Base Address"},
    {0x058, "Chip Select 3 Base Address"},
    {0x05c, "Interrupt Control/Status"},
    {0x060, "EEPROM Ctrl | PCI Target Response | User I/O | Init Ctrl"},
    {0xFFF, ""}
};




/*************************************************
 *                  9030 Registers
 ************************************************/
REGISTER_SET Lcr9030[] =
{
    {0x000, "Space 0 Range"},
    {0x004, "Space 1 Range"},
    {0x008, "Space 2 Range"},
    {0x00c, "Space 3 Range"},
    {0x010, "Expansion ROM Range"},
    {0x014, "Space 0 Local Base Address (Remap)"},
    {0x018, "Space 1 Local Base Address (Remap)"},
    {0x01c, "Space 2 Local Base Address (Remap)"},
    {0x020, "Space 3 Local Base Address (Remap)"},
    {0x024, "Expansion ROM Local Base Address (Remap)"},
    {0x028, "Space 0 Bus Region Descriptor"},
    {0x02c, "Space 1 Bus Region Descriptor"},
    {0x030, "Space 2 Bus Region Descriptor"},
    {0x034, "Space 3 Bus Region Descriptor"},
    {0x038, "Expansion ROM Bus Region Descriptor"},
    {0x03c, "Chip Select 0 Base Address"},
    {0x040, "Chip Select 1 Base Address"},
    {0x044, "Chip Select 2 Base Address"},
    {0x048, "Chip Select 3 Base Address"},
    {0x04c, "EEPROM Write Protect | Interrupt Control/Status"},
    {0x050, "EEPROM Ctrl | PCI Target Response | User I/O | Init Ctrl"},
    {0x054, "General Purpose I/O Control"},
    {0xFFF, ""}
};



REGISTER_SET Eep9030[] =
{
    {0x000, "Device ID | Vendor ID"},
    {0x004, "PCI Command | Status"},
    {0x008, "Class Code | Revision ID"},
    {0x00c, "Subsystem Device ID | Subsystem Vendor ID"},
    {0x010, "New Capability Pointer"},
    {0x014, "Interrupt Pin | Interrupt Line"},
    {0x018, "Power Management Capability | Next Item Pointer | Capability ID"},
    {0x01c, "Power Management Control/Status"},
    {0x020, "Hot Swap Capability | Next Item Pointer | Capability ID"},
    {0x024, "VPD Capability | Next Item Pointer | Capability ID"},
    {0x028, "Space 0 Range"},
    {0x02c, "Space 1 Range"},
    {0x030, "Space 2 Range"},
    {0x034, "Space 3 Range"},
    {0x038, "Expansion ROM Range"},
    {0x03c, "Space 0 Local Base Address (Remap)"},
    {0x040, "Space 1 Local Base Address (Remap)"},
    {0x044, "Space 2 Local Base Address (Remap)"},
    {0x048, "Space 3 Local Base Address (Remap)"},
    {0x04c, "Expansion ROM Local Base Address (Remap)"},
    {0x050, "Space 0 Bus Region Descriptor"},
    {0x054, "Space 1 Bus Region Descriptor"},
    {0x058, "Space 2 Bus Region Descriptor"},
    {0x05c, "Space 3 Bus Region Descriptor"},
    {0x060, "Expansion ROM Bus Region Descriptor"},
    {0x064, "Chip Select 0 Base Address"},
    {0x068, "Chip Select 1 Base Address"},
    {0x06c, "Chip Select 2 Base Address"},
    {0x070, "Chip Select 3 Base Address"},
    {0x074, "Interrupt Control/Status"},
    {0x078, "EEPROM Ctrl | PCI Target Response | User I/O | Init Ctrl"},
    {0x07c, "General Purpose I/O Control"},
    {0x080, "Power Managment Select"},
    {0x084, "Power Managment Scale"},
    {0xFFF, ""}
};




/*************************************************
 *                 9080 Registers
 ************************************************/
REGISTER_SET Lcr9080[] =
{
    {0x000, "Range for PCI-to-Local Address Space 0"},
    {0x004, "Local Base Address (Remap) for PCI-to-Local Space 0"},
    {0x008, "Mode/DMA Arbitration"},
    {0x00c, "Big/Little Endian Descriptor"},
    {0x010, "Range for PCI-to-Local Expansion ROM"},
    {0x014, "Local Base Address for PCI-to-Local Expansion ROM and BREQ"},
    {0x018, "Local Bus Region Descriptor (Space 0) for PCI-to-Local Access"},
    {0x01c, "Range for Direct Master-to-PCI"},
    {0x020, "Local Base Address for Direct Master-to-PCI Memory"},
    {0x024, "Local Base Address for Direct Master-to-PCI IO/CFG"},
    {0x028, "PCI Base Address for Direct Master-to-PCI"},
    {0x02c, "PCI Configuration Address for Direct Master-to-PCI IO/CFG"},
    {0x0f0, "Range for PCI-to-Local Address Space 1"},
    {0x0f4, "Local Base Address (Remap) for PCI-to-Local Space 1"},
    {0x0f8, "Local Bus Region Descriptor (Space 1) for PCI-to-Local Access"},
    {0xFFF, ""}
};



REGISTER_SET Rtr9080[] =
{
    {0x078, "Mailbox 0"},
    {0x07c, "Mailbox 1"},
    {0x048, "Mailbox 2"},
    {0x04c, "Mailbox 3"},
    {0x050, "Mailbox 4"},
    {0x054, "Mailbox 5"},
    {0x058, "Mailbox 6"},
    {0x05c, "Mailbox 7"},
    {0x060, "PCI-to-Local Doorbell"},
    {0x064, "Local-to-PCI Doorbell"},
    {0x068, "Interrupt Control/Status"},
    {0x06c, "EEPROM Ctrl | PCI Cmd Codes | User I/O Ctrl | Init Ctrl"},
    {0x070, "Hardcoded Device/Vendor ID"},
    {0x074, "Hardcoded Revision ID"},
    {0xFFF, ""}
};



REGISTER_SET Dma9080[] =
{
    {0x080, "DMA Ch 0 Mode"},
    {0x084, "DMA Ch 0 PCI Address"},
    {0x088, "DMA Ch 0 Local Address"},
    {0x08c, "DMA Ch 0 Transfer Byte Count"},
    {0x090, "DMA Ch 0 Descriptor Pointer"},
    {0x094, "DMA Ch 1 Mode"},
    {0x098, "DMA Ch 1 PCI Address"},
    {0x09c, "DMA Ch 1 Local Address"},
    {0x0a0, "DMA Ch 1 Transfer Byte Count"},
    {0x0a4, "DMA Ch 1 Descriptor Pointer"},
    {0x0a8, "DMA Ch 1 Cmd/Stat | DMA Ch 0 Cmd/Stat"},
    {0x0ac, "Mode | DMA Arbitration"},
    {0x0b0, "DMA Channels Threshold"},
    {0xFFF, ""}
};



REGISTER_SET Mqr9080[] =
{
    {0x030, "Outbound Post Queue Interrupt Status"},
    {0x034, "Outbound Post Queue Interrupt Mask"},
    {0x0c0, "Messaging Unit Configuration"},
    {0x0c4, "Queue Base Address"},
    {0x0c8, "Inbound Free Head Pointer"},
    {0x0cc, "Inbound Free Tail Pointer"},
    {0x0d0, "Inbound Post Head Pointer"},
    {0x0d4, "Inbound Post Tail Pointer"},
    {0x0d8, "Outbound Free Head Pointer"},
    {0x0dc, "Outbound Free Tail Pointer"},
    {0x0e0, "Outbound Post Head Pointer"},
    {0x0e4, "Outbound Post Tail Pointer"},
    {0x0e8, "Queue Status/Control"},
    {0xFFF, ""}
};



REGISTER_SET Eep9080[] =
{
    {0x000, "Device ID | Vendor ID"},
    {0x004, "Base Class | Sub Class | Interface | Revision ID"},
    {0x008, "Max_Lat | Min_Grant | Interrupt Pin | Interrupt Line"},
    {0x00c, "Mailbox 0"},
    {0x010, "Mailbox 1"},
    {0x014, "Range for PCI-to-Local Address Space 0"},
    {0x018, "Local Base Address (Remap) for PCI-to-Local Space 0"},
    {0x01c, "Local Mode/DMA Arbitration"},
    {0x020, "Big/Little Endian Descriptor"},
    {0x024, "Range for PCI-to-Local Expansion ROM"},
    {0x028, "Local Base Address for PCI-to-Local Expansion ROM and BREQ"},
    {0x02c, "Local Bus Region Descriptor (Space 0) for PCI-to-Local Access"},
    {0x030, "Range for Direct Master-to-PCI"},
    {0x034, "Local Base Address for Direct Master-to-PCI Memory"},
    {0x038, "Local Base Address for Direct Master-to-PCI IO/CFG"},
    {0x03c, "PCI Base Address for Direct Master-to-PCI"},
    {0x040, "PCI Configuration Address for Direct Master-to-PCI IO/CFG"},
    {0x044, "Subsystem Device ID|Subsystem Vendor ID"},
    {0x048, "Range for PCI-to-Local Address Space 1"},
    {0x04c, "Local Base Address (Remap) for PCI-to-Local Space 1"},
    {0x050, "Local Bus Region Descriptor (Space 1) for PCI-to-Local Access"},
    {0x054, "PCI Base Address-to-Local Expansion ROM"},
    {0xFFF, ""}
};




/*************************************************
 *                 9054 Registers
 ************************************************/
REGISTER_SET Pci9054[] =
{
    {0x000, "Device ID | Vendor ID"},
    {0x004, "Status | Command"},
    {0x008, "Base Class | Sub Class | Interface | Revision ID"},
    {0x00c, "BIST|Header Type|Latency Timer|Cache Line Size"},
    {0x010, "PCI Base Address 0"},
    {0x014, "PCI Base Address 1"},
    {0x018, "PCI Base Address 2"},
    {0x01c, "PCI Base Address 3"},
    {0x020, "PCI Base Address 4"},
    {0x024, "PCI Base Address 5"},
    {0x028, "Cardbus CIS Pointer"},
    {0x02c, "Subsystem Device ID | Subsystem Vendor ID"},
    {0x030, "PCI Base Address-to-Local Expansion ROM"},
    {0x034, "Next Capability Pointer"},
    {0x038, "Reserved"},
    {0x03c, "Max_Lat | Min_Grant| Interrupt Pin | Interrupt Line"},
    {0x040, "Power Management Capability | Next Item Pointer | Capability ID"},
    {0x044, "PM Cap: PM Data | Bridge Ext | PM Control/Status"},
    {0x048, "Hot Swap Capability | Next Item Ptr | Capability ID"},
    {0x04c, "VPD Capability | VPD Address | Next Item Ptr | Capability ID"},
    {0x050, "VPD Data"},
    {0xFFF, ""}
};



REGISTER_SET Lcr9054[] =
{
    {0x000, "Range for PCI-to-Local Address Space 0"},
    {0x004, "Local Base Address (Remap) for PCI-to-Local Space 0"},
    {0x008, "Mode | DMA Arbitration"},
    {0x00c, "Reserved | EEPROM Write Prot | Misc Ctrl | Endian Descriptor"},
    {0x010, "Range for PCI-to-Local Expansion ROM"},
    {0x014, "Local Base Address for PCI-to-Local Expansion ROM and BREQ"},
    {0x018, "Local Bus Region Descriptor (Space 0) for PCI-to-Local Access"},
    {0x01c, "Range for Direct Master-to-PCI"},
    {0x020, "Local Base Address for Direct Master-to-PCI Memory"},
    {0x024, "Local Base Address for Direct Master-to-PCI IO/CFG"},
    {0x028, "PCI Base Address for Direct Master-to-PCI"},
    {0x02c, "PCI Configuration Address Reg for Direct Master-to-PCI CFG"},
    {0x0f0, "Range for PCI-to-Local Address Space 1"},
    {0x0f4, "Local Base Address (Remap) for PCI-to-Local Space 1"},
    {0x0f8, "Local Bus Region Descriptor (Space 1) for PCI-to-Local Access"},
    {0x0fc, "PCI Base Dual Address (Remap) for Direct Master-to-PCI"},
    {0xFFF, ""}
};



REGISTER_SET Dma9054[] =
{
    {0x080, "DMA Ch 0 Mode"},
    {0x084, "DMA Ch 0 PCI Address"},
    {0x088, "DMA Ch 0 Local Address"},
    {0x08c, "DMA Ch 0 Transfer Byte Count"},
    {0x090, "DMA Ch 0 Descriptor Pointer"},
    {0x094, "DMA Ch 1 Mode"},
    {0x098, "DMA Ch 1 PCI Address"},
    {0x09c, "DMA Ch 1 Local Address"},
    {0x0a0, "DMA Ch 1 Transfer Byte Count"},
    {0x0a4, "DMA Ch 1 Descriptor Pointer"},
    {0x0a8, "DMA Ch 1 Cmd/Stat | DMA Ch 0 Cmd/Stat"},
    {0x0ac, "Mode | DMA Arbitration"},
    {0x0b0, "DMA Channels Threshold"},
    {0x0b4, "DMA Ch 0 PCI Dual Address (Upper 32 bits)"},
    {0x0b8, "DMA Ch 1 PCI Dual Address (Upper 32 bits)"},
    {0xFFF, ""}
};



REGISTER_SET Eep9054[] =
{
    {0x000, "Device ID | Vendor ID"},
    {0x004, "Base Class | Sub Class | Interface | Revision ID"},
    {0x008, "Max_Lat | Min_Grant | Interrupt Pin | Interrupt Line"},
    {0x00c, "Mailbox 0"},
    {0x010, "Mailbox 1"},
    {0x014, "Range for PCI-to-Local Address Space 0"},
    {0x018, "Local Base Address (Remap) for PCI-to-Local Space 0"},
    {0x01c, "Local Mode/DMA Arbitration"},
    {0x020, "VPD Boundary | Big/Little Endian Descriptor"},
    {0x024, "Range for PCI-to-Local Expansion ROM"},
    {0x028, "Local Base Address for PCI-to-Local Expansion ROM and BREQ"},
    {0x02c, "Local Bus Region Descriptor (Space 0) for PCI-to-Local Access"},
    {0x030, "Range for Direct Master-to-PCI"},
    {0x034, "Local Base Address for Direct Master-to-PCI Memory"},
    {0x038, "Local Base Address for Direct Master-to-PCI IO/CFG"},
    {0x03c, "PCI Base Address for Direct Master-to-PCI"},
    {0x040, "PCI Configuration Address for Direct Master-to-PCI IO/CFG"},
    {0x044, "Subsystem Device ID | Subsystem Vendor ID"},
    {0x048, "Range for PCI-to-Local Address Space 1"},
    {0x04c, "Local Base Address (Remap) for PCI-to-Local Space 1"},
    {0x050, "Local Bus Region Descriptor (Space 1) for PCI-to-Local Access"},
    {0x054, "Hot Swap Control/Status | Next Item Pointer | Capability ID"},
    {0xFFF, ""}
};




/*************************************************
 *            9656 and 9056 Registers
 ************************************************/

/******************************************
 * The following register sets are
 * identical to other PLX chips:
 *
 * - PCI registers           (9054)
 * - Run-time registers      (9080)
 * - DMA registers           (9054)
 * - Message Queue registers (9080)
 *****************************************/

REGISTER_SET Lcr9656[] =
{
    {0x000, "Range for PCI-to-Local Address Space 0"},
    {0x004, "Local Base Address (Remap) for PCI-to-Local Space 0"},
    {0x008, "Mode/DMA Arbitration"},
    {0x00c, "Reserved|EEPROM Write Prot|Misc Ctrl|Endian Descriptor"},
    {0x010, "Range for PCI-to-Local Expansion ROM"},
    {0x014, "Local Base Address for PCI-to-Local Expansion ROM and BREQ"},
    {0x018, "Local Bus Region Descriptor (Space 0) for PCI-to-Local Access"},
    {0x01c, "Range for Direct Master-to-PCI"},
    {0x020, "Local Base Address for Direct Master-to-PCI Memory"},
    {0x024, "Local Base Address for Direct Master-to-PCI IO/CFG"},
    {0x028, "PCI Base Address for Direct Master-to-PCI"},
    {0x02c, "PCI Configuration Address for Direct Master-to-PCI IO/CFG"},
    {0x0f0, "Range for PCI-to-Local Address Space 1"},
    {0x0f4, "Local Base Address (Remap) for PCI-to-Local Space 1"},
    {0x0f8, "Local Bus Region Descriptor (Space 1) for PCI-to-Local Access"},
    {0x0fc, "PCI Base Dual Address (Remap) for Direct Master-to-PCI"},
    {0x100, "PCI Arbiter Control"},
    {0x104, "PCI Abort Address"},
    {0xFFF, ""}
};



REGISTER_SET Eep9656[] =
{
    {0x000, "Device ID | Vendor ID"},
    {0x004, "Base Class | Sub Class | Interface | Revision ID"},
    {0x008, "Max_Lat | Min_Grant | Interrupt Pin | Interrupt Line"},
    {0x00c, "Mailbox 0"},
    {0x010, "Mailbox 1"},
    {0x014, "Range for PCI-to-Local Address Space 0"},
    {0x018, "Local Base Address (Remap) for PCI-to-Local Space 0"},
    {0x01c, "Local Mode/DMA Arbitration"},
    {0x020, "VPD Boundary | Big/Little Endian Descriptor"},
    {0x024, "Range for PCI-to-Local Expansion ROM"},
    {0x028, "Local Base Address for PCI-to-Local Expansion ROM and BREQ"},
    {0x02c, "Local Bus Region Descriptor (Space 0) for PCI-to-Local Access"},
    {0x030, "Range for Direct Master-to-PCI"},
    {0x034, "Local Base Address for Direct Master-to-PCI Memory"},
    {0x038, "Local Base Address for Direct Master-to-PCI IO/CFG"},
    {0x03c, "PCI Base Address for Direct Master-to-PCI"},
    {0x040, "PCI Configuration Address for Direct Master-to-PCI IO/CFG"},
    {0x044, "Subsystem Device ID | Subsystem Vendor ID"},
    {0x048, "Range for PCI-to-Local Address Space 1"},
    {0x04c, "Local Base Address (Remap) for PCI-to-Local Space 1"},
    {0x050, "Local Bus Region Descriptor (Space 1) for PCI-to-Local Access"},
    {0x054, "Hot Swap Control/Status | Next Item Pointer | Capability ID"},
    {0x058, "PCI Arbiter Control"},
    {0x05c, "Power Management Capability | Next Item Pointer | Capability ID"},
    {0x060, "Power Management Control/Status"},
    {0xFFF, ""}
};




/*************************************************
 *         PCI-to-PCI Bridge Registers
 ************************************************/
REGISTER_SET Pci6540[] =
{
    {0x000, "Device ID | Vendor ID"},
    {0x004, "Status | Command"},
    {0x008, "Base Class | Sub Class | Interface | Revision ID"},
    {0x00c, "BIST | Header Type | Prim Latency Timer | Cacheline Size"},
    {0x010, "PCI Base Address 0"},
    {0x014, "PCI Base Address 1"},
    {0x018, "Sec Lat Timer | Sub Bus Num | Sec Bus Num | Prim Bus Num"},
    {0x01c, "Secondary Status | I/O Limit | I/O Base"},
    {0x020, "Memory Limit | Memory Base"},
    {0x024, "Prefetchable Memory Limit | Prefetchable Memory Base"},
    {0x028, "Prefetchable Base (Upper 32-bits)"},
    {0x02c, "Prefetchable Limit (Upper 32-bits)"},
    {0x030, "I/O Limit (Upper 16-bits) | I/O Base (Upper 16-bits)"},
    {0x034, "Extended Capability Pointer"},
    {0x038, "Expansion ROM Base Address"},
    {0x03c, "Bridge Control | Interrupt Pin | Interrupt Line"},
    {0x040, "Arbiter Control | Diagnostic Control | Chip Control"},
    {0x044, "Misc Options | Timeout Control | Primary flow-through control"},
    {0x048, "Sec Inc PF cnt|Prim Inc PF cnt|Sec PF Line cnt|Prim PF Line cnt"},
    {0x04c, "Buff cntrl|Sec flow-thru ctrl|Sec Max PF cnt|Prim Max PF cnt"},
    {0x050, "Rsvd | Test Register | Internal Arbiter control"},
    {0x054, "EEPROM Data | EEPROM Address | EEPROM control"},
    {0x060, "Timer Counter | Timer Control | Reserved"},
    {0x064, "GPIO[3:0] Inp Data | Output En | Ouput Data | P_SERR event dis"},
    {0x068, "Clkrun Register | P_SERR status | Clock control"},
    {0x06c, "Private Memory Limit | Private Memory Base"},
    {0x070, "Private Memory Base (upper 32-bits)"},
    {0x074, "Private Memory Limit (upper 32-bits)"},
    {0x09c, "GPIO[7:4] Inp Data|Output En|Ouput Data|Hotswap SW ROR ctrl"},
    {0x0a0, "GPIO[15:8] Input Data | Output En | Ouput Data | Powerup Status"},
    {0x0d0, "Extended Register Index | Reserved"},
    {0x0d4, "Extended Register Dataport"},
    {0x0dc, "Power Management Capability | Next Item Pointer | Capability ID"},
    {0x0e0, "PM Cap: PM Data | Bridge Ext | PM Control/Status"},
    {0x0e4, "Hot Swap Contrl/Status | Next Item Pointer | Capability ID"},
    {0x0e8, "VPD Flag | VPD Address | Next Item Pointer | Capability ID"},
    {0x0ec, "VPD Data"},
    {0x0f0, "PCI-X Secondary Status | Next Item Pointer | Capability ID"},
    {0x0f4, "PCI-X Bridge Status"},
    {0x0f8, "Upstream Split Transaction Control"},
    {0x0fc, "Downstream Split Transaction Control"},
    {0xFFF, ""}
};



REGISTER_SET Eep6540[] =
{
    {0x000, "<< Group 1 >> : EEPROM signature (valid = 1516h)"},
    {0x002, "Enable Misc Functions | Enable EEPROM regions"},
    {0x004, "<< Group 2 >> : Vendor ID"},
    {0x006, "Device ID"},
    {0x008, "Transparent mode Class code (lower byte) | Reserved"},
    {0x00A, "Transparent mode Class code (upper bytes"},
    {0x00C, "Non-Transp mode class code (lower byte) | Transparent mode Hdr type"},
    {0x00E, "Non-Transparent mode Class code (upper bytes)"},
    {0x010, "Built-in Self Test (BIST) | Non-Transparent mode Header type"},
    {0x012, "Internal Arbiter Control"},
    {0x014, "<< Group 3 >> : Timeout Control | Primary flow-through control"},
    {0x016, "Miscellaneous Options"},
    {0x018, "Secondary Initial Prefetch count | Primary Initial Prefetch count"},
    {0x01A, "Sec Incremental Prefetch count | Primary Incremental Prefetch count"},
    {0x01C, "Secondary Maximum Prefetch count | Primary Maximum Prefetch count"},
    {0x01E, "Power Management Data | Secondary flow-through control"},
    {0x020, "Power Management Control/Status"},
    {0x022, "Power Management Capabilities"},
    {0x024, "<< Group 4 >> : Subsystem Vendor ID"},
    {0x026, "Subsytem Device ID"},
    {0x028, "<< Grp 5 >> : Upst- Trans En | B0 I/O | B0 Trans Addr[15:12] | Rsv"},
    {0x02A, "Upstream BAR0 Translation address[31:16]"},
    {0x02C, "Ups- B1 Trans[31:24] | B0/1/2 PF | B1 64-bit | B1 Trans[23:20]"},
    {0x02E, "Upstream BAR 2 Translation address[15:0]"},
    {0x030, "Upstream BAR 2 Translation address[31:16]"},
    {0x032, "Upstream- BAR 0/1/2 Translation Masks"},
    {0x034, "Downstr- Addr Trans En | BAR0 I/O | BAR0 Trans Addr[15:12] | Rsv"},
    {0x036, "Downstream BAR0 Translation address[31:16]"},
    {0x038, "Downstr- B1 Trans[31:24] | B0/1/2 PF | B1 64-bit | B1 Trans[23:20]"},
    {0x03A, "Downstream BAR 2 Translation address[15:0]"},
    {0x03C, "Downstream BAR 2 Translation address[31:16]"},
    {0x03E, "Downstream- BAR 0/1/2 Translation Masks"},
    {0xFFF, ""}
};



REGISTER_SET Eep6254[] =
{
    {0x000, "Offset 00"},
    {0x002, "Offset 02"},
    {0x004, "Offset 04"},
    {0x006, "Offset 06"},
    {0x008, "Offset 08"},
    {0x00A, "Offset 0A"},
    {0x00C, "Offset 0C"},
    {0x00E, "Offset 0E"},
    {0x010, "Offset 10"},
    {0x012, "Offset 12"},
    {0x014, "Offset 14"},
    {0x016, "Offset 16"},
    {0x018, "Offset 18"},
    {0x01A, "Offset 1A"},
    {0x01C, "Offset 1C"},
    {0x01E, "Offset 1E"},
    {0x020, "Offset 20"},
    {0x022, "Offset 22"},
    {0x024, "Offset 24"},
    {0x026, "Offset 26"},
    {0x028, "Offset 28"},
    {0x02A, "Offset 2A"},
    {0x02C, "Offset 2C"},
    {0x02E, "Offset 2E"},
    {0x030, "Offset 30"},
    {0x024, "Offset 34"},
    {0x026, "Offset 36"},
    {0x028, "Offset 38"},
    {0x02A, "Offset 3A"},
    {0x02C, "Offset 3C"},
    {0x02E, "Offset 3E"},
    {0xFFF, ""}
};




/*************************************************
 *              8111 Registers
 ************************************************/
REGISTER_SET Pci8111[] =
{
    {0x000, "Device ID | Vendor ID"},
    {0x004, "Status | Command"},
    {0x008, "Base Class | Sub Class | Interface | Revision ID"},
    {0x00c, "BIST | Header Type | Prim Latency Timer | Cacheline Size"},
    {0x010, "PCI Base Address 0"},
    {0x014, "PCI Base Address 1"},
    {0x018, "Sec Lat Timer | Sub Bus Num | Sec Bus Num | Prim Bus Num"},
    {0x01c, "Secondary Status | I/O Limit | I/O Base"},
    {0x020, "Memory Limit | Memory Base"},
    {0x024, "Prefetchable Memory Limit | Prefetchable Memory Base"},
    {0x028, "Prefetchable Base (Upper 32-bits)"},
    {0x02c, "Prefetchable Limit (Upper 32-bits)"},
    {0x030, "I/O Limit (Upper 16-bits) | I/O Base (Upper 16-bits)"},
    {0x034, "Extended Capability Pointer"},
    {0x038, "Expansion ROM Base Address"},
    {0x03c, "Bridge Control | Interrupt Pin | Interrupt Line"},
    {0x040, "Power Management Capability | Next Item Pointer | Capability ID"},
    {0x044, "PM Cap: PM Data | Bridge Ext | PM Control/Status"},
    {0x048, "Device-specific Control"},
    {0x050, "MSI Message Capability | Next Item Pointer | Capability ID"},
    {0x054, "MSI Cap: Message Address (lower 32-bits)"},
    {0x058, "MSI Cap: Message Address (upper 32-bits)"},
    {0x05C, "MSI Cap: Message Data"},
    {0x060, "PCI Express Capability | Next Item Pointer | Capability ID"},
    {0x064, "PCIe Cap: Device Capabilities"},
    {0x068, "PCIe Cap: Device Status | Device Control"},
    {0x06C, "PCIe Cap: Link Capabilities"},
    {0x070, "PCIe Cap: Link Status | Link Control"},
    {0x074, "PCIe Cap: Slot Capabilities"},
    {0x078, "PCIe Cap: Slot Status | Slot Control"},
    {0x07C, "PCIe Cap: Root Control"},
    {0x080, "PCIe Cap: Root Status"},
    {0x084, "Main Control Register Index"},
    {0x088, "Main Control Register Data"},
    {0x100, "Power Budgeting Capability: Next Cap Offset | Version | ID"},
    {0x104, "PB Cap: Data Select"},
    {0x108, "PB Cap: Data Register"},
    {0x10C, "PB Cap: Power Budget Capability"},
    {0x110, "Serial Number Capability: Next Cap Offset | Version | ID"},
    {0x114, "SN Cap: Serial Number (lower 32-bits)"},
    {0x118, "SN Cap: Serial Number (upper 32-bits)"},
    {0xFFF, ""}
};


REGISTER_SET Lcr8111[] =
{
    {0x1000, "Device Initialization"},
    {0x1004, "EEPROM Control"},
    {0x1008, "EEPROM Clock Frequency"},
    {0x100c, "PCI Control"},
    {0x1010, "PCI Express Interrupt Enable"},
    {0x1014, "PCI Interrupt Enable"},
    {0x1018, "Interrupt Status"},
    {0x101c, "Power Required"},
    {0x1020, "General Purpose I/O Control"},
    {0x1024, "General Purpose I/O Status"},
    {0x1030, "Mailbox 0"},
    {0x1034, "Mailbox 1"},
    {0x1038, "Mailbox 2"},
    {0x103c, "Mailbox 3"},
    {0x1040, "Chip Silicon Revision"},
    {0x1044, "Diagnostics"},
    {0x1048, "TLP Controller Configuration 0"},
    {0x104c, "TLP Controller Configuration 1"},
    {0x1050, "TLP Controller Configuration 2"},
    {0x1054, "TLP Controller Tag"},
    {0x1058, "TLP Controller Time Limit 0"},
    {0x105c, "TLP Controller Time Limit 1"},
    {0x1060, "CSR Retry Timer"},
    {0x1064, "Enhanced Configuration Address"},
    {0xFFF, ""}
};


REGISTER_SET Eep8111[] =
{
    {0x000, "Offset 00"},
    {0x002, "Offset 02"},
    {0x004, "Offset 04"},
    {0x006, "Offset 06"},
    {0x008, "Offset 08"},
    {0x00A, "Offset 0A"},
    {0x00C, "Offset 0C"},
    {0x00E, "Offset 0E"},
    {0x010, "Offset 10"},
    {0x012, "Offset 12"},
    {0x014, "Offset 14"},
    {0x016, "Offset 16"},
    {0x018, "Offset 18"},
    {0x01A, "Offset 1A"},
    {0x01C, "Offset 1C"},
    {0x01E, "Offset 1E"},
    {0x020, "Offset 20"},
    {0x022, "Offset 22"},
    {0x024, "Offset 24"},
    {0x026, "Offset 26"},
    {0x028, "Offset 28"},
    {0x02A, "Offset 2A"},
    {0x02C, "Offset 2C"},
    {0x02E, "Offset 2E"},
    {0x030, "Offset 30"},
    {0xFFF, ""}
};




/*************************************************
 *              8532 Registers
 ************************************************/
REGISTER_SET Pci8500[] =
{
    {0x000, "Device ID | Vendor ID"},
    {0x004, "Status | Command"},
    {0x008, "Base Class | Sub Class | Interface | Revision ID"},
    {0x00c, "BIST | Header Type | Prim Latency Timer | Cacheline Size"},
    {0x010, "PCI Base Address 0"},
    {0x014, "PCI Base Address 1"},
    {0x018, "Sec Lat Timer | Sub Bus Num | Sec Bus Num | Prim Bus Num"},
    {0x01c, "Secondary Status | I/O Limit | I/O Base"},
    {0x020, "Memory Limit | Memory Base"},
    {0x024, "Prefetchable Memory Limit | Prefetchable Memory Base"},
    {0x028, "Prefetchable Base (Upper 32-bits)"},
    {0x02c, "Prefetchable Limit (Upper 32-bits)"},
    {0x030, "I/O Limit (Upper 16-bits) | I/O Base (Upper 16-bits)"},
    {0x034, "Extended Capability Pointer"},
    {0x038, "Expansion ROM Base Address"},
    {0x03c, "Bridge Control | Interrupt Pin | Interrupt Line"},
    {0x040, "Power Management Capability | Next Item Pointer | Capability ID"},
    {0x044, "PM Cap: PM Data | Bridge Ext | PM Control/Status"},
    {0x048, "MSI Message Capability | Next Item Pointer | Capability ID"},
    {0x04C, "MSI Cap: Message Address (lower 32-bits)"},
    {0x040, "MSI Cap: Message Address (upper 32-bits)"},
    {0x054, "MSI Cap: Message Data"},
    {0x068, "PCI Express Capability | Next Item Pointer | Capability ID"},
    {0x06C, "PCIe Cap: Device Capabilities"},
    {0x070, "PCIe Cap: Device Status | Device Control"},
    {0x074, "PCIe Cap: Link Capabilities"},
    {0x078, "PCIe Cap: Link Status | Link Control"},
    {0x07C, "PCIe Cap: Slot Capabilities"},
    {0x080, "PCIe Cap: Slot Status | Slot Control"},
    {0x100, "Serial Number Capability: Next Cap Offset | Version | ID"},
    {0x104, "SN Cap: Serial Number (lower 32-bits)"},
    {0x108, "SN Cap: Serial Number (upper 32-bits)"},
    {0x138, "Power Budgeting Capability: Next Cap Offset | Version | ID"},
    {0x13C, "PB Cap: Data Select"},
    {0x140, "PB Cap: Data Register"},
    {0x144, "PB Cap: Power Budget Capability"},
    {0x148, "Virtual Channel Capability: Next Cap Offset | Version | ID"},
    {0x14C, "VC Cap: Port VC Capability 1"},
    {0x150, "VC Cap: Port VC Capability 2"},
    {0x154, "VC Cap: Port VC Status | Port VC Control"},
    {0x158, "VC Cap: VC 0 Resource Capability"},
    {0x15C, "VC Cap: VC 0 Resource Control"},
    {0x160, "VC Cap: VC 0 Resource Status | Reserved"},
    {0x164, "VC Cap: VC 1 Resource Capability"},
    {0x168, "VC Cap: VC 1 Resource Control"},
    {0x16C, "VC Cap: VC 1 Resource Status | Reserved"},
    {0x1B8, "VC Cap: VC Arbitration Table (Phases 0-7)"},
    {0x1BC, "VC Cap: VC Arbitration Table (Phases 8-15)"},
    {0x1C0, "VC Cap: VC Arbitration Table (Phases 16-23)"},
    {0x1C4, "VC Cap: VC Arbitration Table (Phases 24-32)"},
    {0xFB4, "Advanced Error Reporting Cap: Next Cap Offset | Version | ID"},
    {0xFB8, "AER Cap: Uncorrectable Error Status"},
    {0xFBC, "AER Cap: Uncorrectable Error Mask"},
    {0xFC0, "AER Cap: Uncorrectable Error Severity"},
    {0xFC4, "AER Cap: Correctable Error Status"},
    {0xFC8, "AER Cap: Correctable Error Mask"},
    {0xFCC, "AER Cap: Advanced Error Capabilities & Control"},
    {0xFD0, "AER Cap: Header Log 0"},
    {0xFD4, "AER Cap: Header Log 1"},
    {0xFD8, "AER Cap: Header Log 2"},
    {0xFDC, "AER Cap: Header Log 3"},
    {0xFFF, ""}
};


REGISTER_SET Lcr8500[] =
{
    {0x000, "Device ID | Vendor ID"},
    {0x004, "Status | Command"},
    {0x008, "Base Class | Sub Class | Interface | Revision ID"},
    {0x00c, "BIST | Header Type | Prim Latency Timer | Cacheline Size"},
    {0x010, "PCI Base Address 0"},
    {0x014, "PCI Base Address 1"},
    {0x018, "Sec Lat Timer | Sub Bus Num | Sec Bus Num | Prim Bus Num"},
    {0x01c, "Secondary Status | I/O Limit | I/O Base"},
    {0x020, "Memory Limit | Memory Base"},
    {0x024, "Prefetchable Memory Limit | Prefetchable Memory Base"},
    {0x028, "Prefetchable Base (Upper 32-bits)"},
    {0x02c, "Prefetchable Limit (Upper 32-bits)"},
    {0x030, "I/O Limit (Upper 16-bits) | I/O Base (Upper 16-bits)"},
    {0x034, "Extended Capability Pointer"},
    {0x038, "Expansion ROM Base Address"},
    {0x03c, "Bridge Control | Interrupt Pin | Interrupt Line"},
    {0x100, "Offset 100"},
    {0x104, "Offset 104"},
    {0x108, "Offset 108"},
    {0x10c, "Offset 10c"},
    {0x110, "Offset 110"},
    {0x114, "Offset 114"},
    {0x118, "Offset 118"},
    {0x11c, "Offset 11c"},
    {0x120, "Offset 120"},
    {0xFFF, ""}
};


REGISTER_SET Eep8500[] =
{
    {0x000, "Offset 00"},
    {0x004, "Offset 04"},
    {0x008, "Offset 08"},
    {0x00C, "Offset 0C"},
    {0x010, "Offset 10"},
    {0x014, "Offset 14"},
    {0x018, "Offset 18"},
    {0x01C, "Offset 1C"},
    {0x020, "Offset 20"},
    {0x024, "Offset 24"},
    {0x028, "Offset 28"},
    {0x02C, "Offset 2C"},
    {0x030, "Offset 30"},
    {0xFFF, ""}
};








/********************************************************************************************
 *
 *                                        Functions
 *
 *******************************************************************************************/

/**********************************************************
 *
 * Function   :  RegSet_DescrGetByIndex
 *
 * Description:  Returns register description by index
 *
 *********************************************************/
char*
RegSet_DescrGetByIndex(
    REGISTER_SET *pSet,
    U8            index
    )
{
    U8 i;


    i = 0;

    while (pSet[i].Offset != 0xFFF)
    {
        if (i == index)
            return pSet[i].Description;

        // Go to next entry
        i++;
    }

    return NULL;
}




/**********************************************************
 *
 * Function   :  RegSet_DescrGetByOffset
 *
 * Description:  Returns register description by offset
 *
 *********************************************************/
char*
RegSet_DescrGetByOffset(
    REGISTER_SET *pSet,
    U32           offset
    )
{
    U8 i;


    i = 0;

    while (pSet[i].Offset != 0xFFF)
    {
        if (offset == pSet[i].Offset)
            return pSet[i].Description;

        // Go to next entry
        i++;
    }

    return NULL;
}
