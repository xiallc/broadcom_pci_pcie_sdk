#ifndef _PCI_DEVICE_H
#define _PCI_DEVICE_H

/*******************************************************************************
 * Copyright 2013-2019 Broadcom, Inc
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

// Max bytes to read/write for each SPI flash block operation
#define SPI_MAX_BLOCK_SIZE        512


// Device flags
typedef enum _PEX_DEV_FLAGS
{
    PEX_DEV_FLAG_NONE           = 0,        // No flags
    PEX_DEV_FLAG_IS_SYNTH       = (1 << 1), // Device is synthetic
    PEX_DEV_FLAG_IS_SELECTED    = (1 << 2), // Device currently selected
    PEX_DEV_FLAG_EEP_VERIFIED   = (1 << 3)  // Track if EEPROM has been verified
} PEX_DEV_FLAGS;

// Device node properties
typedef struct _DEVICE_NODE
{
    PLX_DEVICE_KEY       Key;
    U8                   PciHeaderType;     // PCI header type
    U32                  PciClass;          // PCI Class code
    PLX_PORT_PROP        PortProp;          // Port properties
    U8                   DevFlags;          // Device flags
    PLX_UINT_PTR         Va_PciBar[6];      // Virtual addresses of PCI BAR spaces
    struct _DEVICE_NODE *pNext;             // Pointer to next node in device list
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
    U16     index,
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

BOOLEAN
Plx_SpiFileLoad(
    PLX_DEVICE_OBJECT *PtrDev,
    PEX_SPI_OBJ       *PtrSpi,
    char              *PtrFileName,
    U32                StartOffset,
    U8                 NvFlags
    );

BOOLEAN
Plx_SpiFileSave(
    PLX_DEVICE_OBJECT *PtrDev,
    PEX_SPI_OBJ       *PtrSpi,
    char              *PtrFileName,
    U32                StartOffset,
    U32                ByteCount,
    U8                 NvFlags
    );



#endif
