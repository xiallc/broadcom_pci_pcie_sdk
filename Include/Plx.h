#ifndef __PLX_H
#define __PLX_H

/*******************************************************************************
 * Copyright 2013-2019 Avago Technologies
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
 *      Plx.h
 *
 * Description:
 *
 *      This file contains definitions that are common to all PCI SDK code
 *
 * Revision:
 *
 *      08-01-19 : PCI/PCIe SDK v8.00
 *
 ******************************************************************************/


#ifdef __cplusplus
extern "C" {
#endif




/**********************************************
*               Definitions
**********************************************/
// SDK Version information
#define PLX_SDK_VERSION_MAJOR            8
#define PLX_SDK_VERSION_MINOR            00
#define PLX_SDK_VERSION_STRING           "8.00"
#define PLX_SDK_PRODUCT_NAME_STRING      "Broadcom PCI/PCIe SDK"
#define PLX_SDK_COMPANY_NAME_STRING      "Broadcom Ltd."
#define PLX_SDK_COPYRIGHT_STRING         "\251 Broadcom 2018"

// Device object validity codes
#define PLX_TAG_VALID                    0x5F504C58     // "_PLX" in Hex
#define PLX_TAG_INVALID                  0x564F4944     // "VOID" in Hex
#define ObjectValidate(pObj)             ((pObj)->IsValidTag = PLX_TAG_VALID)
#define ObjectInvalidate(pObj)           ((pObj)->IsValidTag = PLX_TAG_INVALID)
#define IsObjectValid(pObj)              ((pObj)->IsValidTag == PLX_TAG_VALID)

// Used for locating PCI devices
#define PCI_FIELD_IGNORE                 (-1)

// Used for VPD accesses
#define VPD_COMMAND_MAX_RETRIES          5         // Max number VPD command re-issues
#define VPD_STATUS_MAX_POLL              10        // Max number of times to read VPD status
#define VPD_STATUS_POLL_DELAY            5         // Delay between polling VPD status (Milliseconds)

// Define a large value for a signal to the driver
#define FIND_AMOUNT_MATCHED              80001

// Used for performance counter calculations
#define PERF_MAX_PORTS                   96                             // Max # ports in a switch
#define PERF_COUNTERS_PER_PORT           14                             // Number of counters per port
#define PERF_TLP_OH_DW                   2                              // Overhead DW per TLP
#define PERF_TLP_DW                      (3 + PERF_TLP_OH_DW)           // DW per TLP
#define PERF_TLP_SIZE                    (PERF_TLP_DW * sizeof(U32))    // TLP header bytes with overhead
#define PERF_TLP_SIZE_NO_OH              (3 * sizeof(U32))              // TLP header bytes w/o overhead
#define PERF_DLLP_SIZE                   (2 * sizeof(U32))              // Bytes per DLLP
#define PERF_MAX_BPS_GEN_1_0             ((U64)250000000)               // 250 MB/s (2.5 Gbps)
#define PERF_MAX_BPS_GEN_2_0             ((U64)500000000)               // 500 MB/s (5 Gbps)
#define PERF_MAX_BPS_GEN_3_0             ((U64)1000000000)              //   1 GB/s (8 Gbps)
#define PERF_MAX_BPS_GEN_4_0             ((U64)2000000000)              //   2 GB/s (16 Gbps)

// Endian swap macros
#define EndianSwap32(value)              ( ((((value) >>  0) & 0xff) << 24) | \
                                           ((((value) >>  8) & 0xff) << 16) | \
                                           ((((value) >> 16) & 0xff) <<  8) | \
                                           ((((value) >> 24) & 0xff) <<  0) )

#define EndianSwap16(value)              ( ((((value) >>  0) & 0xffff) << 16) | \
                                           ((((value) >> 16) & 0xffff) <<  0) )

// PCIe ReqID support macros - Deprecated & to be removed in future
#define Plx_PciToReqId(bus,slot,fn)      PCIE_REQID_BUILD( (bus), (slot), (fn) )
#define Plx_ReqId_Bus(ReqId)             PCIE_REQID_BUS( (ReqId) )
#define Plx_ReqId_Slot(ReqId)            PCIE_REQID_DEV( (ReqId) )
#define Plx_ReqId_Fn(ReqId)              PCIE_REQID_FN( (ReqId) )



#ifdef __cplusplus
}
#endif

#endif
