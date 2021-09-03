#ifndef __PLX_PCI_FUNCTIONS_H
#define __PLX_PCI_FUNCTIONS_H

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
 *      PciFunc.h
 *
 * Description:
 *
 *      The header file for PCI support functions
 *
 * Revision History:
 *
 *      07-01-14 : PLX SDK v7.20
 *
 ******************************************************************************/


#include <linux/pci.h>
#include "DrvDefs.h"




/**********************************************
 *               Definitions
 *********************************************/
#define PLX_PCI_REG_READ(pdx, offset, pValue) \
    PlxPciRegisterRead_BypassOS( \
        (pdx)->Key.domain,   \
        (pdx)->Key.bus,      \
        (pdx)->Key.slot,     \
        (pdx)->Key.function, \
        (offset),            \
        (pValue)             \
        )

#define PLX_PCI_REG_WRITE(pdx, offset, value) \
    PlxPciRegisterWrite_BypassOS( \
        (pdx)->Key.domain,   \
        (pdx)->Key.bus,      \
        (pdx)->Key.slot,     \
        (pdx)->Key.function, \
        (offset),            \
        (value)              \
        )




/**********************************************
 *               Functions
 *********************************************/
PLX_STATUS
PlxPciRegisterRead_UseOS(
    struct pci_dev *pPciDev,
    U16             offset,
    U32            *pValue
    );

PLX_STATUS
PlxPciRegisterWrite_UseOS(
    struct pci_dev *pPciDev,
    U16             offset,
    U32             value
    );

PLX_STATUS
PlxPciRegRead_ByLoc(
    U8    domain,
    U8    bus,
    U8    slot,
    U8    function,
    U16   offset,
    VOID *pValue,
    U8    AccessSize
    );

PLX_STATUS
PlxPciRegWrite_ByLoc(
    U8  domain,
    U8  bus,
    U8  slot,
    U8  function,
    U16 offset,
    U32 value,
    U8  AccessSize
    );

PLX_STATUS
PlxPciExpressRegRead(
    U8   domain,
    U8   bus,
    U8   slot,
    U8   function,
    U16  offset,
    U32 *pValue
    );

PLX_STATUS
PlxPciExpressRegWrite(
    U8  domain,
    U8  bus,
    U8  slot,
    U8  function,
    U16 offset,
    U32 value
    );

PLX_STATUS
PlxPciRegisterRead_BypassOS(
    U8   domain,
    U8   bus,
    U8   slot,
    U8   function,
    U16  offset,
    U32 *pValue
    );

PLX_STATUS
PlxPciRegisterWrite_BypassOS(
    U8  domain,
    U8  bus,
    U8  slot,
    U8  function,
    U16 offset,
    U32 value
    );

VOID
PlxProbeForEcamBase(
    VOID
    );

U64
PlxPhysicalMemRead(
    U64 address,
    U8  size
    );

U32
PlxPhysicalMemWrite(
    U64 address,
    U64 value,
    U8  size
    );



#endif
