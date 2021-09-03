#ifndef __PLX_H
#define __PLX_H

/*******************************************************************************
 * Copyright 2013-2017 Avago Technologies
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
 *      07-07-16 : PLX SDK v7.25
 *
 ******************************************************************************/


#ifdef __cplusplus
extern "C" {
#endif




/**********************************************
*               Definitions
**********************************************/
// SDK Version information
#define PLX_SDK_VERSION_MAJOR            7
#define PLX_SDK_VERSION_MINOR            25
#define PLX_SDK_VERSION_STRING           "7.25"
#define PLX_SDK_COPYRIGHT_STRING         "\251 Avago Technologies, Inc. 2017"

#define PCI_MAX_BUS                      256            // Max PCI Buses
#define PCI_MAX_DEV                      32             // Max PCI Slots
#define PCI_MAX_FUNC                     8              // Max PCI Functions
#define PCI_NUM_BARS_TYPE_00             6              // Total PCI BARs for Type 0 Header
#define PCI_NUM_BARS_TYPE_01             2              // Total PCI BARs for Type 1 Header

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
#define PERF_MAX_PORTS                   30                             // Max # ports in a switch
#define PERF_COUNTERS_PER_PORT           14                             // Number of counters per port
#define PERF_TLP_OH_DW                   2                              // Overhead DW per TLP
#define PERF_TLP_DW                      (3 + PERF_TLP_OH_DW)           // DW per TLP
#define PERF_TLP_SIZE                    (PERF_TLP_DW * sizeof(U32))    // TLP header bytes with overhead
#define PERF_TLP_SIZE_NO_OH              (3 * sizeof(U32))              // TLP header bytes w/o overhead
#define PERF_DLLP_SIZE                   (2 * sizeof(U32))              // Bytes per DLLP
#define PERF_MAX_BPS_GEN_1_0             ((U64)250000000)               // 250 MBps (2.5 Gbps * 80%)
#define PERF_MAX_BPS_GEN_2_0             ((U64)500000000)               // 500 MBps (5 Gbps * 80%)
#define PERF_MAX_BPS_GEN_3_0             ((U64)1000000000)              //   1 GBps (8 Gbps)

// Endian swap macros
#define EndianSwap32(value)              ( ((((value) >>  0) & 0xff) << 24) | \
                                           ((((value) >>  8) & 0xff) << 16) | \
                                           ((((value) >> 16) & 0xff) <<  8) | \
                                           ((((value) >> 24) & 0xff) <<  0) )

#define EndianSwap16(value)              ( ((((value) >>  0) & 0xffff) << 16) | \
                                           ((((value) >> 16) & 0xffff) <<  0) )

// PCIe ReqID support macros
#define Plx_PciToReqId(bus,slot,fn)     (((U16)(bus) << 8) | ((slot) << 3) | ((fn) << 0))
#define Plx_ReqId_Bus(ReqId)            ((U8)((ReqId) >> 8) & 0xFF)
#define Plx_ReqId_Slot(ReqId)           ((U8)((ReqId) >> 3) & 0x1F)
#define Plx_ReqId_Fn(ReqId)             ((U8)((ReqId) >> 0) & 0x7)


// PCI SIG VENDOR IDS
#define PLX_PCI_VENDOR_ID_LSI            0x1000
#define PLX_PCI_VENDOR_ID_PLX            0x10B5
#define PLX_PCI_VENDOR_ID_INTEL          0x8086
#define PLX_PCI_VENDOR_ID_NVIDIA         0x10DE

// For compatibility with legacy code
#define PLX_VENDOR_ID                    PLX_PCI_VENDOR_ID_PLX


// Device IDs of PLX reference boards
#define PLX_9080RDK_960_DEVICE_ID        0x0960
#define PLX_9080RDK_401B_DEVICE_ID       0x0401
#define PLX_9080RDK_860_DEVICE_ID        0x0860
#define PLX_9054RDK_860_DEVICE_ID        0x1860
#define PLX_9054RDK_LITE_DEVICE_ID       0x5406
#define PLX_CPCI9054RDK_860_DEVICE_ID    0xC860
#define PLX_9056RDK_LITE_DEVICE_ID       0x5601
#define PLX_9056RDK_860_DEVICE_ID        0x56c2
#define PLX_9656RDK_LITE_DEVICE_ID       0x9601
#define PLX_9656RDK_860_DEVICE_ID        0x96c2
#define PLX_9030RDK_LITE_DEVICE_ID       0x3001
#define PLX_CPCI9030RDK_LITE_DEVICE_ID   0x30c1
#define PLX_9050RDK_LITE_DEVICE_ID       0x9050
#define PLX_9052RDK_LITE_DEVICE_ID       0x5201



#ifdef __cplusplus
}
#endif

#endif
