#ifndef __PLX_TYPES_H
#define __PLX_TYPES_H

/*******************************************************************************
 * Copyright (c) PLX Technology, Inc.
 *
 * PLX Technology Inc. licenses this source file under the GNU Lesser General Public
 * License (LGPL) version 2.  This source file may be modified or redistributed
 * under the terms of the LGPL and without express permission from PLX Technology.
 *
 * PLX Technology, Inc. provides this software AS IS, WITHOUT ANY WARRANTY,
 * EXPRESS OR IMPLIED, INCLUDING, WITHOUT LIMITATION, ANY WARRANTY OF
 * MERCHANTABILITY OR FITNESS FOR A PARTICULAR PURPOSE.  PLX makes no guarantee
 * or representations regarding the use of, or the results of the use of,
 * the software and documentation in terms of correctness, accuracy,
 * reliability, currentness, or otherwise; and you rely on the software,
 * documentation and results solely at your own risk.
 *
 * IN NO EVENT SHALL PLX BE LIABLE FOR ANY LOSS OF USE, LOSS OF BUSINESS,
 * LOSS OF PROFITS, INDIRECT, INCIDENTAL, SPECIAL OR CONSEQUENTIAL DAMAGES
 * OF ANY KIND.
 *
 ******************************************************************************/

/******************************************************************************
 * 
 * File Name:
 *
 *      PlxTypes.h
 *
 * Description:
 *
 *      This file includes PLX SDK types and definitions
 *
 * Revision:
 *
 *      07-01-13 : PLX SDK v7.00
 *
 ******************************************************************************/


#include "Plx.h"
#include "PlxDefCk.h"
#include "PlxStat.h"
#include "PciTypes.h"



#ifdef __cplusplus
extern "C" {
#endif




/******************************************
 *   Definitions for Code Portability
 ******************************************/
// Memory set and copy
#if !defined(PLX_MSWINDOWS)
    #define RtlZeroMemory(pDest, count)           memset((pDest), 0, (count))
    #define RtlCopyMemory(pDest, pSrc, count)     memcpy((pDest), (pSrc), (count))
    #define RtlFillMemory(pDest, count, value)    memset((pDest), (value), (count))
#endif

// Convert pointer to an integer
#define PLX_PTR_TO_INT( ptr )                     ((PLX_UINT_PTR)(ptr))

// Convert integer to a pointer
#define PLX_INT_TO_PTR( int )                     ((VOID*)(PLX_UINT_PTR)(int))

// Macros that guarantee correct endian format regardless of CPU platform
#if defined(PLX_BIG_ENDIAN)
    #define PLX_LE_DATA_32(value)                 EndianSwap32( (value) )
    #define PLX_BE_DATA_32(value)                 (value)
#else
    #define PLX_LE_DATA_32(value)                 (value)
    #define PLX_BE_DATA_32(value)                 EndianSwap32( (value) )
#endif

// Macros to support portable type casting on BE/LE platforms
#if (PLX_SIZE_64 == 4)
    #define PLX_64_HIGH_32(value)                 0
    #define PLX_64_LOW_32(value)                  ((U32)(value))
#else
    #if defined(PLX_BIG_ENDIAN)
        #define PLX_64_HIGH_32(value)             ((U32)((U64)value))
        #define PLX_64_LOW_32(value)              ((U32)(((U64)value) >> 32))

        #define PLX_CAST_64_TO_8_PTR( ptr64 )     (U8*) ((U8*)(ptr64) + (7*sizeof(U8)))
        #define PLX_CAST_64_TO_16_PTR( ptr64 )    (U16*)((U8*)(ptr64) + (6*sizeof(U8)))
        #define PLX_CAST_64_TO_32_PTR( ptr64 )    (U32*)((U8*)(ptr64) + sizeof(U32))

        #define PLX_LE_U32_BIT( pos )             ((U32)(1 << (31 - (pos))))
    #else
        #define PLX_64_HIGH_32(value)             ((U32)(((U64)value) >> 32))
        #define PLX_64_LOW_32(value)              ((U32)((U64)value))

        #define PLX_CAST_64_TO_8_PTR( ptr64 )     (U8*) (ptr64)
        #define PLX_CAST_64_TO_16_PTR( ptr64 )    (U16*)(ptr64)
        #define PLX_CAST_64_TO_32_PTR( ptr64 )    (U32*)(ptr64)

        #define PLX_LE_U32_BIT( pos )             ((U32)(1 << (pos)))
    #endif
#endif



/******************************************
 *      Miscellaneous definitions
 ******************************************/
#if !defined(VOID)
    typedef void              VOID;
#endif

// Linux actypes.h contains conflicting definition for BOOLEAN
#if (!defined(PLX_MSWINDOWS)) || defined(PLX_VXD_DRIVER)
    #if !defined(BOOLEAN) && !defined(__ACTYPES_H__)
        typedef S8            BOOLEAN;
    #endif
#endif

#if !defined(PLX_MSWINDOWS)
    #if !defined(BOOL)
        typedef S8            BOOL;
    #endif
#endif

#if !defined(NULL)
    #define NULL              ((VOID *) 0x0)
#endif

#if !defined(TRUE)
    #define TRUE              1
#endif

#if !defined(FALSE)
    #define FALSE             0
#endif

#if defined(PLX_MSWINDOWS) 
    #define PLX_TIMEOUT_INFINITE        INFINITE
#elif defined(PLX_LINUX) || defined(PLX_LINUX_DRIVER)
    #define PLX_TIMEOUT_INFINITE        MAX_SCHEDULE_TIMEOUT

    /*********************************************************
     * Convert milliseconds to jiffies.  The following
     * formula is used:
     *
     *                      ms * HZ
     *           jiffies = ---------
     *                       1,000
     *
     *  where:  HZ      = System-defined clock ticks per second
     *          ms      = Timeout in milliseconds
     *          jiffies = Number of HZ's per second
     ********************************************************/
    #define Plx_ms_to_jiffies( ms )     ( ((ms) * HZ) / 1000 )
    #define Plx_jiffies_to_ms( jiff )   ( ((jiff) * 1000) / HZ )
#endif




/******************************************
 *   PLX-specific types & structures
 ******************************************/

// Mode PLX API uses to access device
typedef enum _PLX_API_MODE
{
    PLX_API_MODE_PCI,                   // Device accessed via PLX driver over PCI/PCIe
    PLX_API_MODE_I2C_AARDVARK,          // Device accessed via Aardvark I2C USB
    PLX_API_MODE_TCP                    // Device accessed via TCP/IP
} PLX_API_MODE;


// Access Size Type
typedef enum _PLX_ACCESS_TYPE
{
    BitSize8,
    BitSize16,
    BitSize32,
    BitSize64
} PLX_ACCESS_TYPE;


// Mode PLX API uses to access device
typedef enum _PLX_CHIP_FAMILY
{
    PLX_FAMILY_NONE = 0,
    PLX_FAMILY_UNKNOWN,
    PLX_FAMILY_BRIDGE_P2L,              // 9000 series & 8311
    PLX_FAMILY_BRIDGE_PCI_P2P,          // 6000 series
    PLX_FAMILY_BRIDGE_PCIE_P2P,         // 8111,8112,8114
    PLX_FAMILY_ALTAIR,                  // 8525,8533,8547,8548
    PLX_FAMILY_ALTAIR_XL,               // 8505,8509
    PLX_FAMILY_VEGA,                    // 8516,8524,8532
    PLX_FAMILY_VEGA_LITE,               // 8508,8512,8517,8518
    PLX_FAMILY_DENEB,                   // 8612,8616,8624,8632,8647,8648
    PLX_FAMILY_SIRIUS,                  // 8604,8606,8608,8609,8613,8614,8615,8617,8618,8619
    PLX_FAMILY_CYGNUS,                  // 8625,8636,8649,8664,8680,8696
    PLX_FAMILY_SCOUT,                   // 8700
    PLX_FAMILY_DRACO_1,                 // 8408,8416,8712,8716,8724,8732,8747,8748
    PLX_FAMILY_DRACO_2,                 // 8713,8717,8725,8733,8749
    PLX_FAMILY_MIRA,                    // 2380,3380,3382,8603,8605
    PLX_FAMILY_CAPELLA_1,               // 8714,8718,8734,8750,8764,8780,8796
    PLX_FAMILY_CAPELLA_2                // 8715,8719,8735,8751,8765,8781,8797
} PLX_CHIP_FAMILY;


// PLX port flags for mask
typedef enum _PLX_FLAG_PORT
{
    PLX_FLAG_PORT_NT_LINK_1     = 63,   // Bit for NT Link port 0
    PLX_FLAG_PORT_NT_LINK_0     = 62,   // Bit for NT Link port 1
    PLX_FLAG_PORT_NT_VIRTUAL_1  = 61,   // Bit for NT Virtual port 0
    PLX_FLAG_PORT_NT_VIRTUAL_0  = 60,   // Bit for NT Virtual port 1
    PLX_FLAG_PORT_NT_DS_P2P     = 59,   // Bit for NT DS P2P port (Virtual)
    PLX_FLAG_PORT_DMA_RAM       = 58,   // Bit for DMA RAM
    PLX_FLAG_PORT_DMA_3         = 57,   // Bit for DMA channel 3
    PLX_FLAG_PORT_DMA_2         = 56,   // Bit for DMA channel 2
    PLX_FLAG_PORT_DMA_1         = 55,   // Bit for DMA channel 1
    PLX_FLAG_PORT_DMA_0         = 54,   // Bit for DMA ch 0 or Func 1 (all 4 ch)
    PLX_FLAG_PORT_PCIE_TO_USB   = 53,   // Bit for PCIe-to-USB P2P or Root Port
    PLX_FLAG_PORT_USB           = 52,   // Bit for USB Host/Bridge
    PLX_FLAG_PORT_ALUT_3        = 51,   // Bit for ALUT RAM arrays 0
    PLX_FLAG_PORT_ALUT_2        = 50,   // Bit for ALUT RAM arrays 1
    PLX_FLAG_PORT_ALUT_1        = 49,   // Bit for ALUT RAM arrays 2
    PLX_FLAG_PORT_ALUT_0        = 48,   // Bit for ALUT RAM arrays 3
    PLX_FLAG_PORT_VS_REGS_S5    = 47,   // Bit for VS mode station 0 specific regs
    PLX_FLAG_PORT_VS_REGS_S4    = 46,   // Bit for VS mode station 1 specific regs
    PLX_FLAG_PORT_VS_REGS_S3    = 45,   // Bit for VS mode station 2 specific regs
    PLX_FLAG_PORT_VS_REGS_S2    = 44,   // Bit for VS mode station 3 specific regs
    PLX_FLAG_PORT_VS_REGS_S1    = 43,   // Bit for VS mode station 4 specific regs
    PLX_FLAG_PORT_VS_REGS_S0    = 42,   // Bit for VS mode station 5 specific regs
    PLX_FLAG_PORT_MAX           = 41    // Bit for highest possible standard port
} PLX_FLAG_PORT;


// Generic states used internally by PLX software
typedef enum _PLX_STATE
{
    PLX_STATE_OK,
    PLX_STATE_WORKING,
    PLX_STATE_ERROR,
    PLX_STATE_ENABLED,
    PLX_STATE_DISABLED,
    PLX_STATE_UNINITIALIZED,
    PLX_STATE_INITIALIZING,
    PLX_STATE_INITIALIZED,
    PLX_STATE_IDLE,
    PLX_STATE_BUSY,
    PLX_STATE_STARTED,
    PLX_STATE_STARTING,
    PLX_STATE_STOPPED,
    PLX_STATE_STOPPING,
    PLX_STATE_CANCELED,
    PLX_STATE_DELETED,
    PLX_STATE_MARKED_FOR_DELETE,
    PLX_STATE_OK_TO_DELETE,
    PLX_STATE_TRIGGERED,
    PLX_STATE_PENDING,
    PLX_STATE_WAITING,
    PLX_STATE_TIMEOUT,
    PLX_STATE_REQUESTING,
    PLX_STATE_REQUESTED,
    PLX_STATE_ACCEPTING,
    PLX_STATE_ACCEPTED,
    PLX_STATE_REJECTED,
    PLX_STATE_COMPLETING,
    PLX_STATE_COMPLETED,
    PLX_STATE_CONNECTING,
    PLX_STATE_CONNECTED,
    PLX_STATE_DISCONNECTING,
    PLX_STATE_DISCONNECTED
} PLX_STATE;


// BAR flags
typedef enum _PLX_BAR_FLAG
{
    PLX_BAR_FLAG_MEM               = (1 << 0),
    PLX_BAR_FLAG_IO                = (1 << 1),
    PLX_BAR_FLAG_BELOW_1MB         = (1 << 2),
    PLX_BAR_FLAG_32_BIT            = (1 << 3),
    PLX_BAR_FLAG_64_BIT            = (1 << 4),
    PLX_BAR_FLAG_PREFETCHABLE      = (1 << 5),
    PLX_BAR_FLAG_UPPER_32          = (1 << 6),
    PLX_BAR_FLAG_PROBED            = (1 << 7)
} PLX_BAR_FLAG;


// EEPROM status
typedef enum _PLX_EEPROM_STATUS
{
    PLX_EEPROM_STATUS_NONE         = 0,     // Not present
    PLX_EEPROM_STATUS_VALID        = 1,     // Present with valid data
    PLX_EEPROM_STATUS_INVALID_DATA = 2,     // Present w/invalid data or CRC error
    PLX_EEPROM_STATUS_BLANK        = PLX_EEPROM_STATUS_INVALID_DATA,
    PLX_EEPROM_STATUS_CRC_ERROR    = PLX_EEPROM_STATUS_INVALID_DATA
} PLX_EEPROM_STATUS;


// EEPROM port numbers
typedef enum _PLX_EEPROM_PORT
{
    PLX_EEPROM_PORT_NONE        = 0,
    PLX_EEPROM_PORT_NT_VIRT_0   = 254,
    PLX_EEPROM_PORT_NT_LINK_0   = 253,
    PLX_EEPROM_PORT_NT_VIRT_1   = 252,
    PLX_EEPROM_PORT_NT_LINK_1   = 251,
    PLX_EEPROM_PORT_DMA_0       = 250,
    PLX_EEPROM_PORT_DMA_1       = 249,
    PLX_EEPROM_PORT_DMA_2       = 248,
    PLX_EEPROM_PORT_DMA_3       = 247,
    PLX_EEPROM_PORT_SHARED_MEM  = 246
} PLX_EEPROM_PORT;


// EEPROM CRC status
typedef enum _PLX_CRC_STATUS
{
    PLX_CRC_INVALID             = 0,
    PLX_CRC_VALID               = 1,
    PLX_CRC_UNSUPPORTED         = 2,
    PLX_CRC_UNKNOWN             = 3
} PLX_CRC_STATUS;


// PCI Express Link Speeds
typedef enum _PLX_LINK_SPEED
{
    PLX_LINK_SPEED_2_5_GBPS     = 1,
    PLX_LINK_SPEED_5_0_GBPS     = 2,
    PLX_LINK_SPEED_8_0_GBPS     = 3,
    PLX_PCIE_GEN_1_0            = PLX_LINK_SPEED_2_5_GBPS,
    PLX_PCIE_GEN_2_0            = PLX_LINK_SPEED_5_0_GBPS,
    PLX_PCIE_GEN_3_0            = PLX_LINK_SPEED_8_0_GBPS
} PLX_LINK_SPEED;


// Interrupt generation types
typedef enum _PLX_IRQ_TYPE
{
    PLX_IRQ_TYPE_NONE           = 0,                       // No interrupt
    PLX_IRQ_TYPE_UNKNOWN        = 1,                       // Undefined interrupt type
    PLX_IRQ_TYPE_INTX           = 2,                       // Legacy INTx interrupt (INTA,INTB,etc)
    PLX_IRQ_TYPE_MSI            = 3,                       // MSI interrupt
    PLX_IRQ_TYPE_MSIX           = 4                        // MSI-X interrupt
} PLX_IRQ_TYPE;


// Port types
typedef enum _PLX_PORT_TYPE
{
    PLX_PORT_UNKNOWN            = 0xFF,
    PLX_PORT_ENDPOINT           = 0,
    PLX_PORT_LEGACY_ENDPOINT    = 1,
    PLX_PORT_ROOT_PORT          = 4,
    PLX_PORT_UPSTREAM           = 5,
    PLX_PORT_DOWNSTREAM         = 6,
    PLX_PORT_PCIE_TO_PCI_BRIDGE = 7,
    PLX_PORT_PCI_TO_PCIE_BRIDGE = 8,
    PLX_PORT_ROOT_ENDPOINT      = 9,
    PLX_PORT_ROOT_EVENT_COLL    = 10
} PLX_PORT_TYPE;


// Non-transparent Port types
typedef enum _PLX_NT_PORT_TYPE
{
    PLX_NT_PORT_NONE            = 0,                       // Not an NT port
    PLX_NT_PORT_PRIMARY         = 1,                       // NT Primary Host side port
    PLX_NT_PORT_SECONDARY       = 2,                       // NT Seconday Host side port
    PLX_NT_PORT_VIRTUAL         = PLX_NT_PORT_PRIMARY,     // NT Virtual-side port
    PLX_NT_PORT_LINK            = PLX_NT_PORT_SECONDARY,   // NT Link-side port
    PLX_NT_PORT_UNKOWN          = 0xFF                     // NT side is undetermined
} PLX_NT_PORT_TYPE;


// NT port configuration types
typedef enum _PLX_NT_CONFIG_TYPE
{
    PLX_NT_CONFIG_TYPE_NONE     = 0,
    PLX_NT_CONFIG_TYPE_LINK_DOWN,
    PLX_NT_CONFIG_TYPE_STANDARD,
    PLX_NT_CONFIG_TYPE_BACK_TO_BACK
} PLX_NT_CONFIG_TYPE;


// Non-transparent LUT flags
typedef enum _PLX_NT_LUT_FLAG
{
    PLX_NT_LUT_FLAG_NONE        = 0,
    PLX_NT_LUT_FLAG_NO_SNOOP    = (1 << 0),
    PLX_NT_LUT_FLAG_READ        = (1 << 1),
    PLX_NT_LUT_FLAG_WRITE       = (1 << 2)
} PLX_NT_LUT_FLAG;


// DMA control commands
typedef enum _PLX_DMA_COMMAND
{
    DmaPause,
    DmaPauseImmediate,
    DmaResume,
    DmaAbort
} PLX_DMA_COMMAND;


// DMA transfer direction
typedef enum _PLX_DMA_DIR
{
    PLX_DMA_PCI_TO_LOC         = 0,                       // DMA PCI --> Local bus   (9000 DMA)
    PLX_DMA_LOC_TO_PCI         = 1,                       // DMA Local bus --> PCI   (9000 DMA)
    PLX_DMA_USER_TO_PCI        = PLX_DMA_PCI_TO_LOC,      // DMA User buffer --> PCI (8000 DMA)
    PLX_DMA_PCI_TO_USER        = PLX_DMA_LOC_TO_PCI       // DMA PCI --> User buffer (8000 DMA)
} PLX_DMA_DIR;


// DMA Descriptor Mode
typedef enum _PLX_DMA_DESCR_MODE
{
    PLX_DMA_MODE_BLOCK         = 0,                      // DMA Block transfer mode
    PLX_DMA_MODE_SGL           = 1,                      // DMA SGL with descriptors off-chip
    PLX_DMA_MODE_SGL_INTERNAL  = 2                       // DMA SGL with descriptors on-chip
} PLX_DMA_DESCR_MODE;


// DMA Ring Delay Period
typedef enum _PLX_DMA_RING_DELAY_TIME
{
    PLX_DMA_RING_DELAY_0       = 0,
    PLX_DMA_RING_DELAY_1us     = 1,
    PLX_DMA_RING_DELAY_2us     = 2,
    PLX_DMA_RING_DELAY_8us     = 3,
    PLX_DMA_RING_DELAY_32us    = 4,
    PLX_DMA_RING_DELAY_128us   = 5,
    PLX_DMA_RING_DELAY_512us   = 6,
    PLX_DMA_RING_DELAY_1ms     = 7
} PLX_DMA_RING_DELAY_TIME;


// DMA Maximum Source Transfer Size
typedef enum _PLX_DMA_MAX_SRC_TSIZE
{
    PLX_DMA_MAX_SRC_TSIZE_64B  = 0,
    PLX_DMA_MAX_SRC_TSIZE_128B = 1,
    PLX_DMA_MAX_SRC_TSIZE_256B = 2,
    PLX_DMA_MAX_SRC_TSIZE_512B = 3,
    PLX_DMA_MAX_SRC_TSIZE_1K   = 4,
    PLX_DMA_MAX_SRC_TSIZE_2K   = 5,
    PLX_DMA_MAX_SRC_TSIZE_4K   = 7
} PLX_DMA_SRC_MAX_TSIZE;


// Performance monitor control
typedef enum _PLX_PERF_CMD
{
    PLX_PERF_CMD_START,
    PLX_PERF_CMD_STOP,
} PLX_PERF_CMD;


// DMA Maximum Source Transfer Size
typedef enum _PLX_SWITCH_MODE
{
    PLX_SWITCH_MODE_STANDARD   = 0,
    PLX_SWITCH_MODE_MULTI_HOST = 2
} PLX_SWITCH_MODE;


// Used for device power state. Added for code compatability with Linux
#if !defined(PLX_MSWINDOWS) 
    typedef enum _DEVICE_POWER_STATE
    {
        PowerDeviceUnspecified = 0,
        PowerDeviceD0,
        PowerDeviceD1,
        PowerDeviceD2,
        PowerDeviceD3,
        PowerDeviceMaximum
    } DEVICE_POWER_STATE;
#endif


// Properties of API access mode
typedef struct _PLX_MODE_PROP
{
    union
    {
        struct
        {
            U16 I2cPort;
            U16 SlaveAddr;
            U32 ClockRate;
        } I2c;

        struct
        {
            U64 IpAddress;
        } Tcp;
    };
} PLX_MODE_PROP;


// PLX version information
typedef struct _PLX_VERSION
{
    PLX_API_MODE ApiMode;

    union
    {
        struct
        {
            U16 ApiLibrary;     // API library version
            U16 Software;       // Software version
            U16 Firmware;       // Firmware version
            U16 Hardware;       // Hardware version
            U16 SwReqByFw;      // Firmware requires software must be >= this version
            U16 FwReqBySw;      // Software requires firmware must be >= this version
            U16 ApiReqBySw;     // Software requires API interface must be >= this version
            U32 Features;       // Bitmask of supported features
        } I2c;
    };
} PLX_VERSION;


// PCI Memory Structure
typedef struct _PLX_PHYSICAL_MEM
{
    U64 UserAddr;                    // User-mode virtual address
    U64 PhysicalAddr;                // Bus physical address
    U64 CpuPhysical;                 // CPU physical address
    U32 Size;                        // Size of the buffer
} PLX_PHYSICAL_MEM;


// PLX Driver Properties
typedef struct _PLX_DRIVER_PROP
{
    U32  Version;                    // Driver version
    char Name[16];                   // Driver name
    char FullName[255];              // Full driver name
    U8   bIsServiceDriver;           // Is service driver or PnP driver?
    U64  AcpiPcieEcam;               // Base address of PCIe ECAM
    U8   Reserved[40];               // Reserved for future use
} PLX_DRIVER_PROP;


// PCI BAR Properties
typedef struct _PLX_PCI_BAR_PROP
{
    U64 BarValue;                    // Actual value in BAR
    U64 Physical;                    // BAR Physical Address
    U64 Size;                        // Size of BAR space
    U32 Flags;                       // Additional BAR properties
} PLX_PCI_BAR_PROP;


// Used for getting the port properties and status
typedef struct _PLX_PORT_PROP
{
    U8  PortType;                    // Port configuration
    U8  PortNumber;                  // Internal port number
    U8  LinkWidth;                   // Negotiated port link width
    U8  MaxLinkWidth;                // Max link width device is capable of
    U8  LinkSpeed;                   // Negotiated link speed
    U8  MaxLinkSpeed;                // Max link speed device is capable of
    U16 MaxReadReqSize;              // Max read request size allowed
    U16 MaxPayloadSize;              // Max payload size setting
    U16 MaxPayloadSupported;         // Max payload size supported by device
    U8  bNonPcieDevice;              // Flag whether device is a PCIe device
} PLX_PORT_PROP;


// Used for getting the multi-host switch properties
typedef struct _PLX_MULTI_HOST_PROP
{
    U8  SwitchMode;                  // Current switch mode
    U16 VS_EnabledMask;              // Bit for each enabled Virtual Switch
    U8  VS_UpstreamPortNum[8];       // Upstream port number of each Virtual Switch
    U32 VS_DownstreamPorts[8];       // Downstream ports associated with a Virtual Switch
    U8  bIsMgmtPort;                 // Is selected device management port
    U8  bMgmtPortActiveEn;           // Is active management port enabled
    U8  MgmtPortNumActive;           // Active management port
    U8  bMgmtPortRedundantEn;        // Is redundant management port enabled
    U8  MgmtPortNumRedundant;        // Redundant management port
} PLX_MULTI_HOST_PROP;


// PLX EEPROM entry
typedef struct _PLX_EEPROM_ENTRY
{
    U8   ChipPort;                   // Destination port
    U32  Offset;                     // Register offset
    U32  Value;                      // Register value
    char Comment[100];               // User entry-specific comments
} PLX_EEPROM_ENTRY;


// PLX EEEPROM structure
typedef struct _PLX_EEPROM_PROP
{
    U8               Signature;      // EEPROM signature (5Ah = Valid)
    U8               bLoadRegs;      // Load registers from EEPROM? (8111/8112)
    U8               bLoadSharedMem; // Load shared mem from EEPROM? (8111/8112)
    char             Comment[200];   // User comments about the EEPROM
    U16              RegCount;       // Number of register entries
    PLX_EEPROM_ENTRY RegEntry[1];    // EEPROM register entries
} PLX_EEPROM_PROP;


// PCI Device Key Identifier
typedef struct _PLX_DEVICE_KEY
{
    U32 IsValidTag;                  // Magic number to determine validity
    U8  domain;                      // Physical device location
    U8  bus;
    U8  slot;
    U8  function;
    U16 VendorId;                    // Device Identifier
    U16 DeviceId;
    U16 SubVendorId;
    U16 SubDeviceId;
    U8  Revision;
    U16 PlxChip;                     // PLX chip type
    U8  PlxRevision;                 // PLX chip revision
    U8  PlxFamily;                   // PLX chip family
    U8  ApiIndex;                    // Used internally by the API
    U16 DeviceNumber;                // Used internally by device drivers
    U8  ApiMode;                     // Mode API uses to access device
    U8  PlxPort;                     // PLX port number of device
    U8  NTPortType;                  // If NT port, stores NT port type
    U8  NTPortNum;                   // If NT port exists, store NT port number
    U8  DeviceMode;                  // Device mode used internally by API
    U32 ApiInternal[2];              // Reserved for internal PLX API use
} PLX_DEVICE_KEY;


// PLX Device Object Structure
typedef struct _PLX_DEVICE_OBJECT
{
    U32               IsValidTag;    // Magic number to determine validity
    PLX_DEVICE_KEY    Key;           // Device location key identifier
    PLX_DRIVER_HANDLE hDevice;       // Handle to driver
    PLX_PCI_BAR_PROP  PciBar[6];     // PCI BAR properties
    U64               PciBarVa[6];   // For PCI BAR user-mode BAR mappings
    U8                BarMapRef[6];  // BAR map count used by API
    PLX_PHYSICAL_MEM  CommonBuffer;  // Used to store common buffer information
    U64               PrivateData[4];// Private storage for user application
} PLX_DEVICE_OBJECT;


// PLX Notification Object
typedef struct _PLX_NOTIFY_OBJECT
{
    U32 IsValidTag;                  // Magic number to determine validity
    U64 pWaitObject;                 // -- INTERNAL -- Wait object used by the driver
    U64 hEvent;                      // User event handle (HANDLE can be 32 or 64 bit)
} PLX_NOTIFY_OBJECT;


// PLX Interrupt Structure 
typedef struct _PLX_INTERRUPT
{
    U32 Doorbell;                    // Up to 32 doorbells
    U8  PciMain                :1;
    U8  PciAbort               :1;
    U8  LocalToPci             :2;   // Local->PCI interrupts 1 & 2
    U8  DmaDone                :4;   // DMA channel 0-3 interrupts
    U8  DmaPauseDone           :4;
    U8  DmaAbortDone           :4;
    U8  DmaImmedStopDone       :4;
    U8  DmaInvalidDescr        :4;
    U8  DmaError               :4;
    U8  MuInboundPost          :1;
    U8  MuOutboundPost         :1;
    U8  MuOutboundOverflow     :1;
    U8  TargetRetryAbort       :1;
    U8  Message                :4;   // 6000 NT 0-3 message interrupts
    U8  SwInterrupt            :1;
    U8  ResetDeassert          :1;
    U8  PmeDeassert            :1;
    U8  GPIO_4_5               :1;   // 6000 NT GPIO 4/5 interrupt
    U8  GPIO_14_15             :1;   // 6000 NT GPIO 14/15 interrupt
    U8  NTV_LE_Correctable     :1;   // 8000 NT Virtual - Link-side error interrupts
    U8  NTV_LE_Uncorrectable   :1;
    U8  NTV_LE_LinkStateChange :1;
    U8  NTV_LE_UncorrErrorMsg  :1;
    U8  HotPlugAttention       :1;
    U8  HotPlugPowerFault      :1;
    U8  HotPlugMrlSensor       :1;
    U8  HotPlugChangeDetect    :1;
    U8  HotPlugCmdCompleted    :1;
} PLX_INTERRUPT;


// DMA Channel Properties Structure
typedef struct _PLX_DMA_PROP
{
    // 8000 DMA properties
    U8  CplStatusWriteBack  :1;
    U8  DescriptorMode      :2;
    U8  DescriptorPollMode  :1;
    U8  RingHaltAtEnd       :1;
    U8  RingWrapDelayTime   :3;
    U8  RelOrderDescrRead   :1;
    U8  RelOrderDescrWrite  :1;
    U8  RelOrderDataReadReq :1;
    U8  RelOrderDataWrite   :1;
    U8  NoSnoopDescrRead    :1;
    U8  NoSnoopDescrWrite   :1;
    U8  NoSnoopDataReadReq  :1;
    U8  NoSnoopDataWrite    :1;
    U8  MaxSrcXferSize      :3;
    U8  MaxDestWriteSize    :3;
    U8  TrafficClass        :3;
    U8  MaxPendingReadReq   :6;
    U8  DescriptorPollTime;
    U8  MaxDescriptorFetch;
    U16 ReadReqDelayClocks;

    // 9000 DMA properties
    U8  ReadyInput          :1;
    U8  Burst               :1;
    U8  BurstInfinite       :1;
    U8  SglMode             :1;
    U8  DoneInterrupt       :1;
    U8  RouteIntToPci       :1;
    U8  ConstAddrLocal      :1;
    U8  WriteInvalidMode    :1;
    U8  DemandMode          :1;
    U8  EnableEOT           :1;
    U8  FastTerminateMode   :1;
    U8  ClearCountMode      :1;
    U8  DualAddressMode     :1;
    U8  EOTEndLink          :1;
    U8  ValidMode           :1;
    U8  ValidStopControl    :1;
    U8  LocalBusWidth       :2;
    U8  WaitStates          :4;
} PLX_DMA_PROP;


// DMA Transfer Parameters
typedef struct _PLX_DMA_PARAMS
{
    U64 UserVa;                     // User buffer virtual address
    U64 AddrSource;                 // Source address      (8000 DMA)
    U64 AddrDest;                   // Destination address (8000 DMA)
    U64 PciAddr;                    // PCI address         (9000 DMA)
    U32 LocalAddr;                  // Local bus address   (9000 DMA)
    U32 ByteCount;                  // Number of bytes to transfer
    U8  Direction;                  // Direction of transfer (Local<->PCI, User<->PCI) (9000 DMA)
    U8  bConstAddrSrc   :1;         // Constant source PCI address?      (8000 DMA)
    U8  bConstAddrDest  :1;         // Constant destination PCI address? (8000 DMA)
    U8  bForceFlush     :1;         // Force DMA to flush write on final descriptor (8000 DMA)
    U8  bIgnoreBlockInt :1;         // For block mode only, do not enable DMA done interrupt
} PLX_DMA_PARAMS;


// Performance properties
typedef struct _PLX_PERF_PROP
{
    U32 IsValidTag;   // Magic number to determine validity

    // Port properties
    U8  PortNumber;
    U8  LinkWidth;
    U8  LinkSpeed;
    U8  Station;
    U8  StationPort;

    // Ingress counters
    U32 IngressPostedHeader;
    U32 IngressPostedDW;
    U32 IngressNonpostedDW;
    U32 IngressCplHeader;
    U32 IngressCplDW;
    U32 IngressDllp;
    U32 IngressPhy;

    // Egress counters
    U32 EgressPostedHeader;
    U32 EgressPostedDW;
    U32 EgressNonpostedDW;
    U32 EgressCplHeader;
    U32 EgressCplDW;
    U32 EgressDllp;
    U32 EgressPhy;

    // Storage for previous counter values

    // Previous Ingress counters
    U32 Prev_IngressPostedHeader;
    U32 Prev_IngressPostedDW;
    U32 Prev_IngressNonpostedDW;
    U32 Prev_IngressCplHeader;
    U32 Prev_IngressCplDW;
    U32 Prev_IngressDllp;
    U32 Prev_IngressPhy;

    // Previous Egress counters
    U32 Prev_EgressPostedHeader;
    U32 Prev_EgressPostedDW;
    U32 Prev_EgressNonpostedDW;
    U32 Prev_EgressCplHeader;
    U32 Prev_EgressCplDW;
    U32 Prev_EgressDllp;
    U32 Prev_EgressPhy;
} PLX_PERF_PROP;


// Performance statistics
typedef struct _PLX_PERF_STATS
{
    S64         IngressTotalBytes;              // Total bytes including overhead
    long double IngressTotalByteRate;           // Total byte rate
    S64         IngressCplAvgPerReadReq;        // Average number of completion TLPs for read requests
    S64         IngressCplAvgBytesPerTlp;       // Average number of bytes per completion TLPs
    S64         IngressPayloadReadBytes;        // Payload bytes read (Completion TLPs)
    S64         IngressPayloadReadBytesAvg;     // Average payload bytes for reads (Completion TLPs)
    S64         IngressPayloadWriteBytes;       // Payload bytes written (Posted TLPs)
    S64         IngressPayloadWriteBytesAvg;    // Average payload bytes for writes (Posted TLPs)
    S64         IngressPayloadTotalBytes;       // Payload total bytes
    double      IngressPayloadAvgPerTlp;        // Payload average size per TLP
    long double IngressPayloadByteRate;         // Payload byte rate
    long double IngressLinkUtilization;         // Total link utilization

    S64         EgressTotalBytes;               // Total byte including overhead
    long double EgressTotalByteRate;            // Total byte rate
    S64         EgressCplAvgPerReadReq;         // Average number of completion TLPs for read requests
    S64         EgressCplAvgBytesPerTlp;        // Average number of bytes per completion TLPs
    S64         EgressPayloadReadBytes;         // Payload bytes read (Completion TLPs)
    S64         EgressPayloadReadBytesAvg;      // Average payload bytes for reads (Completion TLPs)
    S64         EgressPayloadWriteBytes;        // Payload bytes written (Posted TLPs)
    S64         EgressPayloadWriteBytesAvg;     // Average payload bytes for writes (Posted TLPs)
    S64         EgressPayloadTotalBytes;        // Payload total bytes
    double      EgressPayloadAvgPerTlp;         // Payload average size per TLP
    long double EgressPayloadByteRate;          // Payload byte rate
    long double EgressLinkUtilization;          // Total link utilization
} PLX_PERF_STATS;




#ifdef __cplusplus
}
#endif

#endif
