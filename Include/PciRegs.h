#ifndef __PCI_REGS_H
#define __PCI_REGS_H

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
 *      PciRegs.h
 *
 * Description:
 *
 *      This file defines the generic PCI Configuration Registers
 *
 * Revision:
 *
 *      08-01-19 : PCI/PCIe SDK v8.00
 *
 ******************************************************************************/



// PCI location max counts
#define PCI_MAX_BUS                             256     // Max PCI Buses
#define PCI_MAX_DEV                             32      // Max PCI Slots
#define PCI_MAX_FUNC                            8       // Max PCI Functions

// PCI config space sizes
#define PCI_CONFIG_SPACE_SIZE                   0x100   // PCI = 256B
#define PCIE_CONFIG_SPACE_SIZE                  0x1000  // PCIe = 4K

// PCI register read error values return to software
#define PCI_CFG_RD_ERR_VAL_8                    ((U8)-1)
#define PCI_CFG_RD_ERR_VAL_16                   ((U16)-1)
#define PCI_CFG_RD_ERR_VAL_32                   ((U32)-1)
#define PCI_CFG_RD_ERR_VAL                      PCI_CFG_RD_ERR_VAL_32

// Special values returned for ID read if CRS SW visibility enabled
#define PCIE_CFG_RD_CRS_VAL_16                  (U16)0x0001
#define PCIE_CFG_RD_CRS_VAL_32                  (U32)0xFFFF0001

// PCI Header types
#define PCI_HDR_TYPE_0                          0       // Endpoint
#define PCI_HDR_TYPE_1                          1       // PCI-to-PCI bridge
#define PCI_HDR_TYPE_2                          2       // Cardbus
#define PCI_NUM_BARS_TYPE_00                    6       // Type 0 total PCI BARs
#define PCI_NUM_BARS_TYPE_01                    2       // Type 1 total PCI BARs

// Standard PCI registers
#define PCI_REG_DEV_VEN_ID                      0x00
#define PCI_REG_CMD_STAT                        0x04
#define PCI_REG_CLASS_REV                       0x08
#define PCI_REG_HDR_CACHE_LN                    0x0C
#define PCI_REG_BAR_0                           0x10
#define PCI_REG_BAR_1                           0x14
#define PCI_REG_CAP_PTR                         0x34
#define PCI_REG_INT_PIN_LN                      0x3C

// Type 0 specific standard registers
#define PCI_REG_T0_BAR_2                        0x18
#define PCI_REG_T0_BAR_3                        0x1C
#define PCI_REG_T0_BAR_4                        0x20
#define PCI_REG_T0_BAR_5                        0x24
#define PCI_REG_TO_CARDBUS_PTR                  0x28
#define PCI_REG_TO_SUBSYS_ID                    0x2C
#define PCI_REG_TO_EXP_ROM                      0x30
#define PCI_REG_TO_RSVD_38H                     0x38

// Type 1 specific standard registers
#define PCI_REG_T1_PRIM_SEC_BUS                 0x18
#define PCI_REG_T1_IO_BASE_LIM                  0x1C
#define PCI_REG_T1_MEM_BASE_LIM                 0x20
#define PCI_REG_T1_PF_MEM_BASE_LIM              0x24
#define PCI_REG_T1_PF_MEM_BASE_HIGH             0x28
#define PCI_REG_T1_PF_MEM_LIM_HIGH              0x2C
#define PCI_REG_T1_IO_BASE_LIM_HIGH             0x30
#define PCI_REG_T1_EXP_ROM                      0x38

// PCIe 1st capability pointer
#define PCIE_REG_CAP_PTR                        0x100


// PCI Extended Capability IDs
#define PCI_CAP_ID_NULL                         0x00
#define PCI_CAP_ID_POWER_MAN                    0x01
#define PCI_CAP_ID_AGP                          0x02
#define PCI_CAP_ID_VPD                          0x03
#define PCI_CAP_ID_SLOT_ID                      0x04
#define PCI_CAP_ID_MSI                          0x05
#define PCI_CAP_ID_HOT_SWAP                     0x06
#define PCI_CAP_ID_PCIX                         0x07
#define PCI_CAP_ID_HYPER_TRANSPORT              0x08
#define PCI_CAP_ID_VENDOR_SPECIFIC              0x09
#define PCI_CAP_ID_DEBUG_PORT                   0x0A
#define PCI_CAP_ID_RESOURCE_CTRL                0x0B
#define PCI_CAP_ID_HOT_PLUG                     0x0C
#define PCI_CAP_ID_BRIDGE_SUB_ID                0x0D
#define PCI_CAP_ID_AGP_8X                       0x0E
#define PCI_CAP_ID_SECURE_DEVICE                0x0F
#define PCI_CAP_ID_PCI_EXPRESS                  0x10
#define PCI_CAP_ID_MSI_X                        0x11
#define PCI_CAP_ID_SATA                         0x12
#define PCI_CAP_ID_ADV_FEATURES                 0x13
#define PCI_CAP_ID_ENHANCED_ALLOCATION          0x14


// PCI Express Extended Capability IDs
#define PCIE_CAP_ID_NULL                        0x000       // Empty capability
#define PCIE_CAP_ID_ADV_ERROR_REPORTING         0x001       // Advanced Error Reporting (AER)
#define PCIE_CAP_ID_VIRTUAL_CHANNEL             0x002       // Virtual Channel (VC)
#define PCIE_CAP_ID_DEV_SERIAL_NUMBER           0x003       // Device Serial Number
#define PCIE_CAP_ID_POWER_BUDGETING             0x004       // Power Budgeting
#define PCIE_CAP_ID_RC_LINK_DECLARATION         0x005       // Root Complex Link Declaration
#define PCIE_CAP_ID_RC_INT_LINK_CONTROL         0x006       // Root Complex Internal Link Control
#define PCIE_CAP_ID_RC_EVENT_COLLECTOR          0x007       // Root Complex Event Collector Endpoint Association
#define PCIE_CAP_ID_MF_VIRTUAL_CHANNEL          0x008       // Multi-Function Virtual Channel (MFVC)
#define PCIE_CAP_ID_VC_WITH_MULTI_FN            0x009       // Virtual Channel with Multi-Function
#define PCIE_CAP_ID_RC_REG_BLOCK                0x00A       // Root Complex Register Block (RCRB)
#define PCIE_CAP_ID_VENDOR_SPECIFIC             0x00B       // Vendor-specific (VSEC)
#define PCIE_CAP_ID_CONFIG_ACCESS_CORR          0x00C       // Configuration Access Correlation
#define PCIE_CAP_ID_ACCESS_CTRL_SERVICES        0x00D       // Access Control Services (ACS)
#define PCIE_CAP_ID_ALT_ROUTE_ID_INTERPRET      0x00E       // Alternate Routing-ID Interpretation (ARI)
#define PCIE_CAP_ID_ADDR_TRANS_SERVICES         0x00F       // Address Translation Services (ATS)
#define PCIE_CAP_ID_SR_IOV                      0x010       // SR-IOV
#define PCIE_CAP_ID_MR_IOV                      0x011       // MR-IOV
#define PCIE_CAP_ID_MULTICAST                   0x012       // Multicast
#define PCIE_CAP_ID_PAGE_REQUEST                0x013       // Page Request Interface (PRI)
#define PCIE_CAP_ID_AMD_RESERVED                0x014       // Reserved for AMD
#define PCIE_CAP_ID_RESIZABLE_BAR               0x015       // Resizable BAR
#define PCIE_CAP_ID_DYNAMIC_POWER_ALLOC         0x016       // Dynamic Power Allocation (DPA)
#define PCIE_CAP_ID_TLP_PROCESSING_HINT         0x017       // TLP Processing Hints (TPH)
#define PCIE_CAP_ID_LATENCY_TOLERANCE_REPORT    0x018       // Latency Tolerance Reporting (LTR)
#define PCIE_CAP_ID_SECONDARY_PCI_EXPRESS       0x019       // Secondary PCI Express
#define PCIE_CAP_ID_PROTOCOL_MULTIPLEX          0x01A       // Protocol Multiplexing (PMUX)
#define PCIE_CAP_ID_PROCESS_ADDR_SPACE_ID       0x01B       // Process Address Space ID (PASID)
#define PCIE_CAP_ID_LTWT_NOTIF_REQUESTER        0x01C       // Lightweight Notification Requester (LNR)
#define PCIE_CAP_ID_DS_PORT_CONTAINMENT         0x01D       // Downstream Port Containment (DPC)
#define PCIE_CAP_ID_L1_PM_SUBSTRATES            0x01E       // L1 Power Management Substrates (L1PM)
#define PCIE_CAP_ID_PRECISION_TIME_MEAS         0x01F       // Precision Time Measurement (PTM)
#define PCIE_CAP_ID_PCIE_OVER_M_PHY             0x020       // PCIe over M-PHY (M-PCIe)
#define PCIE_CAP_ID_FRS_QUEUEING                0x021       // FRS Queueing
#define PCIE_CAP_ID_READINESS_TIME_REPORTING    0x022       // Readiness Time Reporting
#define PCIE_CAP_ID_DESIGNATED_VEND_SPECIFIC    0x023       // Designated vendor-specific
#define PCIE_CAP_ID_VF_RESIZABLE_BAR            0x024       // VF resizable BAR
#define PCIE_CAP_ID_DATA_LINK_FEATURE           0x025       // Data Link Feature
#define PCIE_CAP_ID_PHYS_LAYER_16GT             0x026       // Physical Layer 16 GT/s
#define PCIE_CAP_ID_PHYS_LAYER_16GT_MARGINING   0x027       // Physical Layer 16 GT/s Margining
#define PCIE_CAP_ID_HIERARCHY_ID                0x028       // Hierarchy ID
#define PCIE_CAP_ID_NATIVE_PCIE_ENCL_MGMT       0x029       // Native PCIe Enclosure Management (NPEM)

// Convert encoding of MPS/MRR to bytes (128 * (2 ^ encoded_val))
#define PCIE_MPS_MRR_TO_BYTES(val)              ( 128 * (1 << (val)) )


// PCI device Power Management states (PM Cntrl/Stat [1:0])
#define PCI_CAP_PM_STATE_D0                     0x00
#define PCI_CAP_PM_STATE_D1                     0x01
#define PCI_CAP_PM_STATE_D2                     0x02
#define PCI_CAP_PM_STATE_D3_HOT                 0x03


// Function codes for PCI BIOS operations
#define PCI_FUNC_ID                             0xb1
#define PCI_FUNC_BIOS_PRESENT                   0x01
#define PCI_FUNC_FIND_PCI_DEVICE                0x02
#define PCI_FUNC_FIND_PCI_CLASS_CODE            0x03
#define PCI_FUNC_GENERATE_SPECIAL_CYC           0x06
#define PCI_FUNC_READ_CONFIG_BYTE               0x08
#define PCI_FUNC_READ_CONFIG_WORD               0x09
#define PCI_FUNC_READ_CONFIG_DWORD              0x0a
#define PCI_FUNC_WRITE_CONFIG_BYTE              0x0b
#define PCI_FUNC_WRITE_CONFIG_WORD              0x0c
#define PCI_FUNC_WRITE_CONFIG_DWORD             0x0d
#define PCI_FUNC_GET_IRQ_ROUTING_OPTS           0x0e
#define PCI_FUNC_SET_PCI_HW_INT                 0x0f


// PCI SIG Vendor IDs
#define PLX_PCI_VENDOR_ID_LSI                   0x1000
#define PLX_PCI_VENDOR_ID_PLX                   0x10B5
#define PLX_PCI_VENDOR_ID_BROADCOM              0x14E4
#define PLX_PCI_VENDOR_ID_INTEL                 0x8086
#define PLX_PCI_VENDOR_ID_NVIDIA                0x10DE


// PCIe ReqID support macros
#define PCIE_REQID_BUILD(bus,dev,fn)            (((U16)(bus) << 8) | ((dev) << 3) | ((fn) << 0))
#define PCIE_REQID_BUS(ReqId)                   ((U8)((ReqId) >> 8) & 0xFF)
#define PCIE_REQID_DEV(ReqId)                   ((U8)((ReqId) >> 3) & 0x1F)
#define PCIE_REQID_FN(ReqId)                    ((U8)((ReqId) >> 0) & 0x7)



// PCIe TLP format
typedef enum _PCIE_TLP_FORMAT
{
    PCIE_TLP_FORMAT_3DW_NO_DATA             = 0x0,
    PCIE_TLP_FORMAT_4DW_NO_DATA             = 0x1,
    PCIE_TLP_FORMAT_3DW_DATA                = 0x2,
    PCIE_TLP_FORMAT_4DW_DATA                = 0x3,
    PCIE_TLP_FORMAT_TLP_PREFIX              = 0x4
} PCIE_TLP_FORMAT;


// PCIe TLP Types
typedef enum _PCIE_TLP_TYPE
{
    TLP_TYPE_MEM_READ_32                    = 0x00,
    TLP_TYPE_MEM_READ_64                    = 0x20,
    TLP_TYPE_MEM_READ_LOCK_32               = 0x01,
    TLP_TYPE_MEM_READ_LOCK_64               = 0x21,
    TLP_TYPE_MEM_WRITE_32                   = 0x40,
    TLP_TYPE_MEM_WRITE_64                   = 0x60,
    TLP_TYPE_IO_READ                        = 0x02,
    TLP_TYPE_IO_WRITE                       = 0x42,
    TLP_TYPE_CFG_READ_TYPE_0                = 0x04,
    TLP_TYPE_CFG_WRITE_TYPE_0               = 0x44,
    TLP_TYPE_CFG_READ_TYPE_1                = 0x05,
    TLP_TYPE_CFG_WRITE_TYPE_1               = 0x45,
    TLP_TYPE_MSG_TO_RC                      = 0x30,
    TLP_TYPE_MSG_BY_ADDRESS                 = 0x31,
    TLP_TYPE_MSG_BY_ID                      = 0x32,
    TLP_TYPE_MSG_RC_BROADCAST               = 0x33,
    TLP_TYPE_MSG_TO_RECEIVER                = 0x34,
    TLP_TYPE_MSG_GATHERED_TO_RC             = 0x35,
    TLP_TYPE_MSGD_TO_RC                     = 0x70,
    TLP_TYPE_MSGD_BY_ADDRESS                = 0x71,
    TLP_TYPE_MSGD_BY_ID                     = 0x72,
    TLP_TYPE_MSGD_RC_BROADCAST              = 0x73,
    TLP_TYPE_MSGD_TO_RECEIVER               = 0x74,
    TLP_TYPE_MSGD_GATHERED_TO_RC            = 0x75,
    TLP_TYPE_CPL_NO_DATA                    = 0x0A,
    TLP_TYPE_CPL_WITH_DATA                  = 0x4A,
    TLP_TYPE_CPL_LOCKED_NO_DATA             = 0x0B,
    TLP_TYPE_CPL_LOCKED_WITH_DATA           = 0x4B,
    TLP_TYPE_FINAL_ENTRY                    = 0xFF      // Must be last entry
} PCIE_TLP_TYPE;


// PCIe Completion TLP status
typedef enum _PCIE_TLP_CPL_STATUS
{
    TLP_CPL_STATUS_SUCCESS                  = 0x00,
    TLP_CPL_STATUS_UNSUPP_REQ               = 0x01,
    TLP_CPL_STATUS_CONFIG_RETRY             = 0x02,
    TLP_CPL_STATUS_COMPLETER_ABORT          = 0x04
} PCIE_TLP_CPL_STATUS;


// PCIe Message TLP routing
typedef enum _PCIE_TLP_MSG_ROUTE
{
    TLP_MSG_ROUTE_TO_RC                     = 0x00,
    TLP_MSG_ROUTE_BY_ADDR                   = 0x01,
    TLP_MSG_ROUTE_BY_ID                     = 0x02,
    TLP_MSG_ROUTE_RC_BROADCAST              = 0x03,
    TLP_MSG_ROUTE_LOCAL_TERMINATE           = 0x04,
    TLP_MSG_ROUTE_GATHERED_TO_RC            = 0x05,
} PCIE_TLP_MSG_ROUTE;


// PCIe Message TLP types
typedef enum _PCIE_TLP_MSG
{
    TLP_MSG_UNLOCK                          = 0x00,
    TLP_MSG_LATENCY_TOLERANCE_REP           = 0x10,
    TLP_MSG_OPT_BUFF_FLUSH_FILL             = 0x12,
    TLP_MSG_PM_ACTIVE_STATE_NAK             = 0x14,
    TLP_MSG_PM_PME                          = 0x18,
    TLP_MSG_PM_PME_TURN_OFF                 = 0x19,
    TLP_MSG_PM_PME_TO_ACK                   = 0x1B,
    TLP_MSG_ERROR_CORRECTABLE               = 0x30,
    TLP_MSG_ERROR_NON_FATAL                 = 0x31,
    TLP_MSG_ERROR_FATAL                     = 0x33,
    TLP_MSG_ASSERT_INTA                     = 0x20,
    TLP_MSG_ASSERT_INTB                     = 0x21,
    TLP_MSG_ASSERT_INTC                     = 0x22,
    TLP_MSG_ASSERT_INTD                     = 0x23,
    TLP_MSG_DEASSERT_INTA                   = 0x24,
    TLP_MSG_DEASSERT_INTB                   = 0x25,
    TLP_MSG_DEASSERT_INTC                   = 0x26,
    TLP_MSG_DEASSERT_INTD                   = 0x27,
    TLP_MSG_SET_SLOT_POWER_LIMIT            = 0x50,
    TLP_MSG_VENDOR_DEFINED_TYPE_0           = 0x7E,
    TLP_MSG_VENDOR_DEFINED_TYPE_1           = 0x7F,
    TLP_MSG_FINAL_ENTRY                     = 0xFF      // Must be last entry
} PCIE_TLP_MSG;



#endif
