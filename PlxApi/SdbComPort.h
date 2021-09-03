#ifndef __SDB_COM_PORT_H
#define __SDB_COM_PORT_H

/*******************************************************************************
 * Copyright 2013-2019 Broadcom Inc
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

/*******************************************************************************
 *
 * File Name:
 *
 *      SdbComPort.h
 *
 * Description:
 *
 *      Header file for SDB COM port interface functions
 *
 * Revision History:
 *
 *      01-01-19 : PCI/PCIe SDK v8.00
 *
 ******************************************************************************/


#include "PlxIoctl.h"


#ifdef __cplusplus
extern "C" {
#endif


/******************************************
 *             Definitions
 ******************************************/
// OS-specific prefix for COM/TTY port name
#if defined(PLX_MSWINDOWS)
    #define SDB_OS_COM_PORT_UART        "\\\\.\\COM"
    #define SDB_OS_COM_PORT_USB         SDB_OS_COM_PORT_UART
    #define SDB_OS_BAUD_19200           CBR_19200
    #define SDB_OS_BAUD_115200          CBR_115200
#elif defined(PLX_LINUX)
    #define SDB_OS_COM_PORT_UART        "/dev/ttyS"
    #define SDB_OS_COM_PORT_USB         "/dev/ttyUSB"
    #define SDB_OS_BAUD_19200           B19200
    #define SDB_OS_BAUD_115200          B115200
#endif

#define SDB_MAX_ATTEMPTS                2            // Max num of attempts if failure
#define SDB_NEEDS_INIT_CMD              (0xFFFFFFFE) // Needs initial sync command
#define SDB_NEXT_READ_OFFSET_INIT       (0xFFFFFFFF) // Init offset to force full read cmd

// SDB command & reply sizes
#define SDB_READ_CMD_LEN                (1 + 1 + 4 + 1)     // Cmd + size + addr + end
#define SDB_READ_NEXT_CMD_LEN           (1 + 1)             // Cmd + end
#define SDB_READ_REPLY_LEN              (4 + 1)             // 4B data + ACK('%')
#define SDB_WRITE_CMD_LEN               (1 + 1 + 4 + 4 + 1) // Cmd + size + addr + data + end
#define SDB_WRITE_REPLY_LEN             (1)                 // 1B ACK('%')

// SDB binary commands
#define SDB_CMD_READ                    'G'
#define SDB_CMD_READ_NEXT               'N'
#define SDB_CMD_WRITE                   'P'
#define SDB_CMD_END                     '\n'
#define SDB_CMD_ACK                     '%'
#define SDB_CMD_ERROR                   'E'
#define SDB_CMD_INIT                    "\n~%\n"




/******************************************
 *      Device Selection Functions
 *****************************************/
PLX_STATUS
Sdb_DeviceOpen(
    PLX_DEVICE_OBJECT *pDevice
    );

PLX_STATUS
Sdb_DeviceClose(
    PLX_DEVICE_OBJECT *pDevice
    );

PLX_STATUS
Sdb_DeviceFindEx(
    PLX_DEVICE_KEY *pKey,
    U16             DeviceNumber,
    PLX_MODE_PROP  *pModeProp
    );


/******************************************
 *      MDIO Private Support Functions
 *****************************************/
BOOLEAN
Sdb_Driver_Connect(
    PLX_DEVICE_OBJECT *pDevice,
    PLX_MODE_PROP     *pModeProp
    );

BOOLEAN
Sdb_Sync_Connection(
    PLX_DEVICE_OBJECT *pDevice
    );

S32
Sdb_Dispatch_IoControl(
    PLX_DEVICE_OBJECT *pDevice,
    U32                IoControlCode,
    PLX_PARAMS        *pIoBuffer,
    U32                Size
    );


/******************************************
 * Device-specific Register Access Functions
 *****************************************/
U32
Sdb_PlxRegisterRead(
    PLX_DEVICE_OBJECT *pDevice,
    U32                offset,
    PLX_STATUS        *pStatus,
    BOOLEAN            bAdjustForPort,
    U16                bRetryOnError
    );

PLX_STATUS
Sdb_PlxRegisterWrite(
    PLX_DEVICE_OBJECT *pDevice,
    U32                offset,
    U32                value,
    BOOLEAN            bAdjustForPort
    );



#ifdef __cplusplus
}
#endif

#endif
