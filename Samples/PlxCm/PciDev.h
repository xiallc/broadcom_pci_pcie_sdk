#ifndef _PCI_DEVICE_H
#define _PCI_DEVICE_H

/******************************************************************************
 *
 * File Name:
 *
 *      PciDevice.h
 *
 * Description:
 *
 *      Definitions for PCI functions
 *
 ******************************************************************************/


#include "PlxApi.h"

#if defined(PLX_DOS)
    #include "PciFunc.h"
#endif




/*************************************
 *          Definitions
 ************************************/
#if defined(PLX_DOS)
    #define PlxCm_MemRead_8                 PHYS_MEM_READ_8
    #define PlxCm_MemRead_16                PHYS_MEM_READ_16
    #define PlxCm_MemRead_32                PHYS_MEM_READ_32
    #define PlxCm_MemRead_64                PHYS_MEM_READ_64

    #define PlxCm_MemWrite_8                PHYS_MEM_WRITE_8
    #define PlxCm_MemWrite_16               PHYS_MEM_WRITE_16
    #define PlxCm_MemWrite_32               PHYS_MEM_WRITE_32
    #define PlxCm_MemWrite_64               PHYS_MEM_WRITE_64
#else
    #define PlxCm_MemRead_8(addr)           (*(U8*)(addr))
    #define PlxCm_MemRead_16(addr)          (*(U16*)(addr))
    #define PlxCm_MemRead_32(addr)          (*(U32*)(addr))
    #define PlxCm_MemRead_64(addr)          (*(U64*)(addr))

    #define PlxCm_MemWrite_8(addr , value)  (*(U8*)(addr)  = (U8)(value))
    #define PlxCm_MemWrite_16(addr, value)  (*(U16*)(addr) = (U16)(value))
    #define PlxCm_MemWrite_32(addr, value)  (*(U32*)(addr) = (U32)(value))
    #define PlxCm_MemWrite_64(addr, value)  (*(U64*)(addr) = (U64)(value))
#endif

#define MATCH_BASE_EXACT          ((U16)1 << 15)
#define MATCH_BASE_GENERIC        ((U16)1 << 12)
#define MATCH_BASE                MATCH_BASE_EXACT | MATCH_BASE_GENERIC

#define MATCH_SUBCLASS_EXACT      ((U16)1 << 11)
#define MATCH_SUBCLASS_GENERIC    ((U16)1 <<  8)
#define MATCH_SUBCLASS            MATCH_SUBCLASS_EXACT | MATCH_SUBCLASS_GENERIC

#define MATCH_INTERFACE_EXACT     ((U16)1 << 7)
#define MATCH_INTERFACE_GENERIC   ((U16)1 << 4)
#define MATCH_INTERFACE           MATCH_INTERFACE_EXACT | MATCH_INTERFACE_GENERIC

// Function number passed to driver in upper 3-bits of slot
#define PCI_DEVFN(slot, fn)       ((char)((((char)(fn)) << 5) | ((char)(slot) & 0x1f)))


typedef struct _DEVICE_NODE
{
    PLX_DEVICE_KEY       Key;
    U8                   PciHeaderType;                 // PCI header type
    U32                  PciClass;                      // PCI Class code
    PLX_PORT_PROP        PortProp;                      // Port properties
    BOOLEAN              bSelected;                     // Flag to specify if device is selected
    BOOLEAN              bEepromVerified;               // Flag to track if EEPROM has been verified
    PLX_UINT_PTR         Va_PciBar[6];                  // Virtual addresses of PCI BAR spaces
    struct _DEVICE_NODE *pNext;                         // Pointer to next node in device list
} DEVICE_NODE;




/*************************************
 *            Functions
 ************************************/
U16
DeviceListCreate(
    PLX_API_MODE   ApiMode,
    PLX_MODE_PROP *pModeProp
    );

void
DeviceListDisplay(
    void
    );

void
DeviceListFree(
    void
    );

DEVICE_NODE *
DeviceNodeAdd(
    PLX_DEVICE_KEY *pKey
    );

BOOL
DeviceNodeExist(
    PLX_DEVICE_KEY *pKey
    );

void
DevicePropertiesFill(
    DEVICE_NODE *pNode
    );

DEVICE_NODE*
DeviceNodeGetByNum(
    U8      index,
    BOOLEAN bPlxOnly
    );

DEVICE_NODE*
DeviceNodeGetByKey(
    PLX_DEVICE_KEY *pKey
    );

VOID
Device_GetClassString(
    DEVICE_NODE *pNode,
    char        *pClassText
    );

BOOLEAN
Plx_EepromFileLoad(
    PLX_DEVICE_OBJECT *pDevice,
    char              *pFileName,
    U16                EepSize,
    U8                 EepPortSize,
    BOOLEAN            bCrc,
    BOOLEAN            bBypassVerify,
    BOOLEAN            bEndianSwap
    );

BOOLEAN
Plx_EepromFileSave(
    PLX_DEVICE_OBJECT *pDevice,
    char              *pFileName,
    U32                EepSize,
    U8                 EepPortSize,
    BOOLEAN            bCrc,
    BOOLEAN            bEndianSwap
    );

BOOLEAN
Plx8000_EepromFileLoad(
    PLX_DEVICE_OBJECT *pDevice,
    char              *pFileName,
    BOOLEAN            bCrc,
    BOOLEAN            bBypassVerify
    );

BOOLEAN
Plx8000_EepromFileSave(
    PLX_DEVICE_OBJECT *pDevice,
    char              *pFileName,
    U32                ByteCount,
    BOOLEAN            bCrc
    );



#endif
