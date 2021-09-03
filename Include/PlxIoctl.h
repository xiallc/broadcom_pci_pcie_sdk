#ifndef __PLX_IOCTL_H
#define __PLX_IOCTL_H

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
 *      PlxIoctl.h
 *
 * Description:
 *
 *      This file contains the common I/O Control messages shared between
 *      the driver and the PCI API.
 *
 * Revision History:
 *
 *      08-01-11 : PLX SDK v6.50
 *
 ******************************************************************************/


#include "PlxTypes.h"

#if defined(PLX_MSWINDOWS) && !defined(PLX_DRIVER)
    #include <winioctl.h>
#elif defined(PLX_LINUX)
    #include <fcntl.h>
    #include <unistd.h>
    #include <sys/ioctl.h>
#endif


#ifdef __cplusplus
extern "C" {
#endif




// Used to pass IOCTL arguments down to the driver
typedef struct _PLX_PARAMS
{
    PLX_STATUS     ReturnCode;      // API status code
    PLX_DEVICE_KEY Key;             // Device key information
    U64            value[3];        // Generic storage for parameters
    union
    {
        U64                 ExData[5];
        PLX_INTERRUPT       PlxIntr;
        PLX_PHYSICAL_MEM    PciMemory;
        PLX_PORT_PROP       PortProp;
        PLX_PCI_BAR_PROP    BarProp;
        PLX_DMA_PROP        DmaProp;
        PLX_DMA_PARAMS      TxParams;
        PLX_DRIVER_PROP     DriverProp;
        PLX_MULTI_HOST_PROP MH_Prop;
    } u;
} PLX_PARAMS;


#if defined(PLX_MSWINDOWS)
    /**********************************************************
     * Note: Codes 0-2047 (0-7FFh) are reserved by Microsoft
     *       Coded 2048-4095 (800h-FFFh) are reserved for OEMs
     *********************************************************/
    #define PLX_IOCTL_CODE_BASE      0x800
    #define IOCTL_MSG( code )        CTL_CODE(                \
                                         FILE_DEVICE_UNKNOWN, \
                                         code,                \
                                         METHOD_BUFFERED,     \
                                         FILE_ANY_ACCESS      \
                                         )

#elif defined(PLX_LINUX) || defined(PLX_LINUX_DRIVER)

    #define PLX_IOCTL_CODE_BASE      0x0
    #define PLX_MAGIC                'P'
    #define IOCTL_MSG( code )        _IOWR(         \
                                         PLX_MAGIC, \
                                         code,      \
                                         PLX_PARAMS \
                                         )

#elif defined(PLX_DOS)

    #define PLX_IOCTL_CODE_BASE      0x0
    #define IOCTL_MSG( code )        code

#endif


typedef enum _DRIVER_MSGS
{
    MSG_DRIVER_VERSION = PLX_IOCTL_CODE_BASE,
    MSG_DRIVER_PROPERTIES,
    MSG_DRIVER_SCHEDULE_RESCAN,
    MSG_CHIP_TYPE_GET,
    MSG_CHIP_TYPE_SET,
    MSG_GET_PORT_PROPERTIES,
    MSG_PCI_DEVICE_RESET,
    MSG_PCI_DEVICE_FIND,
    MSG_PCI_BAR_PROPERTIES,
    MSG_PCI_BAR_MAP,
    MSG_PCI_BAR_UNMAP,
    MSG_PCI_REGISTER_READ,
    MSG_PCI_REGISTER_WRITE,
    MSG_PCI_REG_READ_BYPASS_OS,
    MSG_PCI_REG_WRITE_BYPASS_OS,
    MSG_REGISTER_READ,
    MSG_REGISTER_WRITE,
    MSG_MAPPED_REGISTER_READ,
    MSG_MAPPED_REGISTER_WRITE,
    MSG_PHYSICAL_MEM_ALLOCATE,
    MSG_PHYSICAL_MEM_FREE,
    MSG_PHYSICAL_MEM_MAP,
    MSG_PHYSICAL_MEM_UNMAP,
    MSG_COMMON_BUFFER_PROPERTIES,
    MSG_IO_PORT_READ,
    MSG_IO_PORT_WRITE,
    MSG_PCI_BAR_SPACE_READ,
    MSG_PCI_BAR_SPACE_WRITE,
    MSG_VPD_READ,
    MSG_VPD_WRITE,
    MSG_EEPROM_PRESENT,
    MSG_EEPROM_PROBE,
    MSG_EEPROM_GET_ADDRESS_WIDTH,
    MSG_EEPROM_SET_ADDRESS_WIDTH,
    MSG_EEPROM_CRC_GET,
    MSG_EEPROM_CRC_UPDATE,
    MSG_EEPROM_READ_BY_OFFSET,
    MSG_EEPROM_WRITE_BY_OFFSET,
    MSG_EEPROM_READ_BY_OFFSET_16,
    MSG_EEPROM_WRITE_BY_OFFSET_16,
    MSG_MAILBOX_READ,
    MSG_MAILBOX_WRITE,
    MSG_INTR_ENABLE,
    MSG_INTR_DISABLE,
    MSG_INTR_STATUS_GET,
    MSG_NOTIFICATION_REGISTER_FOR,
    MSG_NOTIFICATION_CANCEL,
    MSG_NOTIFICATION_WAIT,
    MSG_NOTIFICATION_STATUS,
    MSG_DMA_CHANNEL_OPEN,
    MSG_DMA_GET_PROPERTIES,
    MSG_DMA_SET_PROPERTIES,
    MSG_DMA_CONTROL,
    MSG_DMA_STATUS,
    MSG_DMA_TRANSFER_BLOCK,
    MSG_DMA_TRANSFER_USER_BUFFER,
    MSG_DMA_CHANNEL_CLOSE,
    MSG_PERFORMANCE_INIT_PROPERTIES,
    MSG_PERFORMANCE_MONITOR_CTRL,
    MSG_PERFORMANCE_RESET_COUNTERS,
    MSG_PERFORMANCE_GET_COUNTERS,
    MSG_MH_GET_PROPERTIES,
    MSG_MH_MIGRATE_DS_PORTS,
    MSG_NT_PROBE_REQ_ID,
    MSG_NT_LUT_PROPERTIES,
    MSG_NT_LUT_ADD,
    MSG_NT_LUT_DISABLE
} DRIVER_MSGS;




#define PLX_IOCTL_DRIVER_VERSION                IOCTL_MSG( MSG_DRIVER_VERSION )
#define PLX_IOCTL_DRIVER_PROPERTIES             IOCTL_MSG( MSG_DRIVER_PROPERTIES )
#define PLX_IOCTL_DRIVER_SCHEDULE_RESCAN        IOCTL_MSG( MSG_DRIVER_SCHEDULE_RESCAN )
#define PLX_IOCTL_CHIP_TYPE_GET                 IOCTL_MSG( MSG_CHIP_TYPE_GET )
#define PLX_IOCTL_CHIP_TYPE_SET                 IOCTL_MSG( MSG_CHIP_TYPE_SET )
#define PLX_IOCTL_GET_PORT_PROPERTIES           IOCTL_MSG( MSG_GET_PORT_PROPERTIES )

#define PLX_IOCTL_PCI_DEVICE_FIND               IOCTL_MSG( MSG_PCI_DEVICE_FIND )
#define PLX_IOCTL_PCI_DEVICE_RESET              IOCTL_MSG( MSG_PCI_DEVICE_RESET )
#define PLX_IOCTL_PCI_BAR_PROPERTIES            IOCTL_MSG( MSG_PCI_BAR_PROPERTIES )
#define PLX_IOCTL_PCI_BAR_MAP                   IOCTL_MSG( MSG_PCI_BAR_MAP )
#define PLX_IOCTL_PCI_BAR_UNMAP                 IOCTL_MSG( MSG_PCI_BAR_UNMAP )

#define PLX_IOCTL_PCI_REGISTER_READ             IOCTL_MSG( MSG_PCI_REGISTER_READ )
#define PLX_IOCTL_PCI_REGISTER_WRITE            IOCTL_MSG( MSG_PCI_REGISTER_WRITE )
#define PLX_IOCTL_PCI_REG_READ_BYPASS_OS        IOCTL_MSG( MSG_PCI_REG_READ_BYPASS_OS )
#define PLX_IOCTL_PCI_REG_WRITE_BYPASS_OS       IOCTL_MSG( MSG_PCI_REG_WRITE_BYPASS_OS )

#define PLX_IOCTL_REGISTER_READ                 IOCTL_MSG( MSG_REGISTER_READ )
#define PLX_IOCTL_REGISTER_WRITE                IOCTL_MSG( MSG_REGISTER_WRITE )
#define PLX_IOCTL_MAPPED_REGISTER_READ          IOCTL_MSG( MSG_MAPPED_REGISTER_READ )
#define PLX_IOCTL_MAPPED_REGISTER_WRITE         IOCTL_MSG( MSG_MAPPED_REGISTER_WRITE )
#define PLX_IOCTL_MAILBOX_READ                  IOCTL_MSG( MSG_MAILBOX_READ )
#define PLX_IOCTL_MAILBOX_WRITE                 IOCTL_MSG( MSG_MAILBOX_WRITE )

#define PLX_IOCTL_PHYSICAL_MEM_ALLOCATE         IOCTL_MSG( MSG_PHYSICAL_MEM_ALLOCATE )
#define PLX_IOCTL_PHYSICAL_MEM_FREE             IOCTL_MSG( MSG_PHYSICAL_MEM_FREE )
#define PLX_IOCTL_PHYSICAL_MEM_MAP              IOCTL_MSG( MSG_PHYSICAL_MEM_MAP )
#define PLX_IOCTL_PHYSICAL_MEM_UNMAP            IOCTL_MSG( MSG_PHYSICAL_MEM_UNMAP )
#define PLX_IOCTL_COMMON_BUFFER_PROPERTIES      IOCTL_MSG( MSG_COMMON_BUFFER_PROPERTIES )

#define PLX_IOCTL_IO_PORT_READ                  IOCTL_MSG( MSG_IO_PORT_READ )
#define PLX_IOCTL_IO_PORT_WRITE                 IOCTL_MSG( MSG_IO_PORT_WRITE )
#define PLX_IOCTL_PCI_BAR_SPACE_READ            IOCTL_MSG( MSG_PCI_BAR_SPACE_READ )
#define PLX_IOCTL_PCI_BAR_SPACE_WRITE           IOCTL_MSG( MSG_PCI_BAR_SPACE_WRITE )

#define PLX_IOCTL_VPD_READ                      IOCTL_MSG( MSG_VPD_READ )
#define PLX_IOCTL_VPD_WRITE                     IOCTL_MSG( MSG_VPD_WRITE )

#define PLX_IOCTL_EEPROM_PRESENT                IOCTL_MSG( MSG_EEPROM_PRESENT )
#define PLX_IOCTL_EEPROM_PROBE                  IOCTL_MSG( MSG_EEPROM_PROBE )
#define PLX_IOCTL_EEPROM_GET_ADDRESS_WIDTH      IOCTL_MSG( MSG_EEPROM_GET_ADDRESS_WIDTH )
#define PLX_IOCTL_EEPROM_SET_ADDRESS_WIDTH      IOCTL_MSG( MSG_EEPROM_SET_ADDRESS_WIDTH )
#define PLX_IOCTL_EEPROM_CRC_GET                IOCTL_MSG( MSG_EEPROM_CRC_GET )
#define PLX_IOCTL_EEPROM_CRC_UPDATE             IOCTL_MSG( MSG_EEPROM_CRC_UPDATE )
#define PLX_IOCTL_EEPROM_READ_BY_OFFSET         IOCTL_MSG( MSG_EEPROM_READ_BY_OFFSET )
#define PLX_IOCTL_EEPROM_WRITE_BY_OFFSET        IOCTL_MSG( MSG_EEPROM_WRITE_BY_OFFSET )
#define PLX_IOCTL_EEPROM_READ_BY_OFFSET_16      IOCTL_MSG( MSG_EEPROM_READ_BY_OFFSET_16 )
#define PLX_IOCTL_EEPROM_WRITE_BY_OFFSET_16     IOCTL_MSG( MSG_EEPROM_WRITE_BY_OFFSET_16 )

#define PLX_IOCTL_INTR_ENABLE                   IOCTL_MSG( MSG_INTR_ENABLE )
#define PLX_IOCTL_INTR_DISABLE                  IOCTL_MSG( MSG_INTR_DISABLE )
#define PLX_IOCTL_INTR_STATUS_GET               IOCTL_MSG( MSG_INTR_STATUS_GET )
#define PLX_IOCTL_NOTIFICATION_REGISTER_FOR     IOCTL_MSG( MSG_NOTIFICATION_REGISTER_FOR )
#define PLX_IOCTL_NOTIFICATION_CANCEL           IOCTL_MSG( MSG_NOTIFICATION_CANCEL )
#define PLX_IOCTL_NOTIFICATION_WAIT             IOCTL_MSG( MSG_NOTIFICATION_WAIT )
#define PLX_IOCTL_NOTIFICATION_STATUS           IOCTL_MSG( MSG_NOTIFICATION_STATUS )

#define PLX_IOCTL_DMA_CHANNEL_OPEN              IOCTL_MSG( MSG_DMA_CHANNEL_OPEN )
#define PLX_IOCTL_DMA_GET_PROPERTIES            IOCTL_MSG( MSG_DMA_GET_PROPERTIES )
#define PLX_IOCTL_DMA_SET_PROPERTIES            IOCTL_MSG( MSG_DMA_SET_PROPERTIES )
#define PLX_IOCTL_DMA_CONTROL                   IOCTL_MSG( MSG_DMA_CONTROL )
#define PLX_IOCTL_DMA_STATUS                    IOCTL_MSG( MSG_DMA_STATUS )
#define PLX_IOCTL_DMA_TRANSFER_BLOCK            IOCTL_MSG( MSG_DMA_TRANSFER_BLOCK )
#define PLX_IOCTL_DMA_TRANSFER_USER_BUFFER      IOCTL_MSG( MSG_DMA_TRANSFER_USER_BUFFER )
#define PLX_IOCTL_DMA_CHANNEL_CLOSE             IOCTL_MSG( MSG_DMA_CHANNEL_CLOSE )

#define PLX_IOCTL_PERFORMANCE_INIT_PROPERTIES   IOCTL_MSG( MSG_PERFORMANCE_INIT_PROPERTIES )
#define PLX_IOCTL_PERFORMANCE_MONITOR_CTRL      IOCTL_MSG( MSG_PERFORMANCE_MONITOR_CTRL )
#define PLX_IOCTL_PERFORMANCE_RESET_COUNTERS    IOCTL_MSG( MSG_PERFORMANCE_RESET_COUNTERS )
#define PLX_IOCTL_PERFORMANCE_GET_COUNTERS      IOCTL_MSG( MSG_PERFORMANCE_GET_COUNTERS )

#define PLX_IOCTL_MH_GET_PROPERTIES             IOCTL_MSG( MSG_MH_GET_PROPERTIES )
#define PLX_IOCTL_MH_MIGRATE_DS_PORTS           IOCTL_MSG( MSG_MH_MIGRATE_DS_PORTS )

#define PLX_IOCTL_NT_PROBE_REQ_ID               IOCTL_MSG( MSG_NT_PROBE_REQ_ID )
#define PLX_IOCTL_NT_LUT_PROPERTIES             IOCTL_MSG( MSG_NT_LUT_PROPERTIES )
#define PLX_IOCTL_NT_LUT_ADD                    IOCTL_MSG( MSG_NT_LUT_ADD )
#define PLX_IOCTL_NT_LUT_DISABLE                IOCTL_MSG( MSG_NT_LUT_DISABLE )


#ifdef __cplusplus
}
#endif

#endif
