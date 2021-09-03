#ifndef __MDIO_SPLICE_USB_H
#define __MDIO_SPLICE_USB_H

/*******************************************************************************
 * Copyright 2013-2018 Broadcom, Inc
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
 *      MdioSpliceUsb.h
 *
 * Description:
 *
 *      Header file for Splice MDIO USB interface functions
 *
 * Revision History:
 *
 *      03-01-18 : PLX SDK v8.00
 *
 ******************************************************************************/


#include "PlxIoctl.h"
#if defined(PLX_LINUX)
    #include <dlfcn.h>    // For dynamic library functions
#endif




#ifdef __cplusplus
extern "C" {
#endif


/******************************************
 *             Definitions
 ******************************************/
// Defaults for loading the MDIO USB library
#if defined(PLX_MSWINDOWS)
    #define MDIO_SPLICE_LIB_NAME                "MdioSpliceUsb.dll"
    #define MDIO_SPLICE_ROOT_PATH_DEFAULT       "C:\\Plx\\PlxSdk\\MdioSpliceUsb"
#elif defined(PLX_LINUX)
    #define MDIO_SPLICE_LIB_NAME                ""
    #define MDIO_SPLICE_ROOT_PATH_DEFAULT       ""

    // Linux dynamic library load functions for portability
    #define LoadLibrary( name )                 dlopen( (name), RTLD_LAZY )
    #define FreeLibrary                         dlclose
    #define GetProcAddress( hdl, fn )           dlsym( (hdl), (fn) )
    typedef void*                               HINSTANCE;
    #define TEXT( str )                         (str)
    #define __cdecl
#elif defined(PLX_DOS)
    #define __cdecl
#endif

#define MDIO_MAX_DEVICES                        10  // Max MDIO USB devices supported
#define MDIO_DEFAULT_CLOCK_RATE                 100 // MDIO default clock rate in Khz

// Splice API status codes
#define MDIO_SPLICE_STATUS_OK                   0
#define MDIO_SPLICE_STATUS_ERROR                1
typedef int                                     MDIO_SPLICE_STATUS;

// Invalid entry when looking up an address for corresponding MDIO command
#define MDIO_ADDR_TABLE_IDX_INVALID             (U16)0xFFFF

// Build a 32-bit ID to identify the last accessed region
#define MDIO_ACCESS_ID( idx, AddrHigh )         (((U32)(idx) << 16) | (U16)(AddrHigh))

// MDIO command build ([28:24]=DEV  [20:16]=PHY/Port  [15:0]=Address/data)
#define MDIO_CMD_BUILD( phy, dev, data )        ( ((U32)(dev) << 24) | \
                                                  ((U32)(phy) << 16) | \
                                                  ((U32)(data) & 0xFFFF) )

// Set data field ([15:0]) in MDIO command
#define MDIO_CMD_SET_DATA( cmd, data )          ( (cmd) = ((cmd) & 0xFFFF0000) | \
                                                          ((data) & 0xFFFF) )

// Register read value to return on error
#define MDIO_REG_READ_U32_FAIL_VAL              ((U32)-1)




/******************************************
 *      Device Selection Functions
 *****************************************/
PLX_STATUS
MdioSplice_DeviceOpen(
    PLX_DEVICE_OBJECT *pDevice
    );

PLX_STATUS
MdioSplice_DeviceClose(
    PLX_DEVICE_OBJECT *pDevice
    );

PLX_STATUS
MdioSplice_DeviceFindEx(
    PLX_DEVICE_KEY *pKey,
    U16             DeviceNumber,
    PLX_MODE_PROP  *pModeProp
    );


/******************************************
 *    MDIO Private Support Functions
 *****************************************/
BOOLEAN
MdioSplice_Driver_Connect(
    PLX_DEVICE_OBJECT *pDevice,
    PLX_MODE_PROP     *pModeProp
    );

S32
MdioSplice_Dispatch_IoControl(
    PLX_DEVICE_OBJECT *pDevice,
    U32                IoControlCode,
    PLX_PARAMS        *pIoBuffer,
    U32                Size
    );


/******************************************
 * Device-specific Register Access Functions
 *****************************************/
U32
MdioSplice_PlxRegisterRead(
    PLX_DEVICE_OBJECT *pDevice,
    U32                offset,
    PLX_STATUS        *pStatus,
    BOOLEAN            bAdjustForPort,
    U16                bRetryOnError
    );

PLX_STATUS
MdioSplice_PlxRegisterWrite(
    PLX_DEVICE_OBJECT *pDevice,
    U32                offset,
    U32                value,
    BOOLEAN            bAdjustForPort
    );


/******************************************
 * Private functions
 *****************************************/
U16
MdioGetAccessTableIndex(
    U32 Address
    );


/******************************************
 * Splice MDIO library functions
 *****************************************/

/*************************************************************************************************
 * Function:	UsbConnect()
 *
 * Arguments:	port	-	This port handle will be returned and used in calls to
 *							Read/Write functions listed below such as
 *								UsbReadMdio(port),
 *								UsbWriteMdio(port)
 *				root	-	Root directory for all the dll, firmware, etc.
 *				dev		-	Target device "D6S" in this case
 *				index	-	first Splice board = 0, second = 1
 *
 * Description:	Opens a communication link between the application and the USB device.
 *
 * Return:      0 - on success otherwise failure.
 *				1 - Failed to find Splice USB Device
 *				2 - Failed to connect to Splice Board
 *				3 - Failed to Program USB Firmware
 *				4 - Failed to Re-Connect to USB after USB Firmware Programming
 *				5 - Failed to Program FPGA Firmware
 *				6 - Failed to Program FPGA Firmware
 *				7 - Failed to verify FPGA version after FPGA Firmware Programming
 *************************************************************************************************/
typedef int (__cdecl *Fn_UsbConnect)(void **port, const char *root, const char *dev, unsigned int index);




/*************************************************************************************************
 * Function:	UsbDisConnect()
 * Arguments:	port	-	An opaque port handle of type (void *) returned from
 *							UsbConnect() function call.
 *
 * Description:	Closes the specified port and ends the communication to the USB device.
 *
 * Return:      0 on success and 1 on failure.
 *************************************************************************************************/
typedef int (__cdecl *Fn_UsbDisConnect)(void *port);




/*************************************************************************************************
 * Function:	UsbReadMdio()
 *
 * Arguments:	port	-	An opaque port handle of type (void *) returned from
 *							UsbConnect() function call.
 *				addr	-	This is addr array of size count to read from.
 *				data 	-	This is data array of size count to read into.
 *				count 	-	This is the length of data and addr array pairs.
 *				b32		-	set 1 for 32-bit MDIO, 0 for 16-bit MDIO
 *
 * Description:	Reads data from MDIO registers on a device from addr -> (addr + count)
 *
 * Return:      0 on success and 1 on failure.
 *************************************************************************************************/
typedef int (__cdecl *Fn_UsbReadMdio)(const void *port, const void *addr, void *data, unsigned int count, unsigned int b32);




/*************************************************************************************************
 * Function:	UsbWriteMdio()
 *
 * Arguments:	port	-	An opaque port handle of type (void *) returned from
 *							UsbConnect() function call.
 *				addr	-	This is addr array of size count to write to.
 *						-	Address Format is as Follows:
 *						-	Bits [28:24] = DEVTYPE
 *						-	Bits [20:16] = PHYADDR
 *						-	Bits [15:00] = MDIO Address
 *				data 	-	This is data array of size count to write to.
 *				count 	-	This is the length of data and addr array pairs.
 *				b32		-	set 1 for 32-bit MDIO, 0 for 16-bit MDIO
 *
 * Description:	Writes data to MDIO registers on a device from addr -> (addr + count)
 *
 * Return:      0 on success and 1 on failure.
 *************************************************************************************************/
typedef int (__cdecl *Fn_UsbWriteMdio)(const void *port, const void *addr, const void *data, unsigned int count, unsigned int b32);



/*************************************************************************************************
 * Function:	UsbReadCfgReg()
 *
 * Arguments:	port	-	An opaque port handle of type (void *) returned from
 *							UsbConnect() function call.
 *				addr	-	This is addr array of size count to read from.
 *						-	Address Format is as Follows:
 *						-	Bits [28:24] = DEVTYPE
 *						-	Bits [20:16] = PHYADDR
 *						-	Bits [15:00] = MDIO Address
 *				data 	-	This is data array of size count to read into.
 *				count 	-	This is the length of data and addr array pairs.
 *
 * Description:	Reads data from config registers on a device from addr -> (addr + count)
 *
 * Return:      0 on success and 1 on failure.
 *************************************************************************************************/
typedef int (__cdecl *Fn_UsbReadCfgReg)(const void *port, const void *addr, void *data, unsigned int count);



/*************************************************************************************************
 * Function:	UsbWriteCfgReg()
 *
 * Arguments:	port	-	An opaque port handle of type (void *) returned from
 *							UsbConnect() function call.
 *				addr	-	This is addr array of size count to write to.
 *				data 	-	This is data array of size count to write to.
 *				count 	-	This is the length of data and addr array pairs.
 *
 * Description:	Wites data to config registers on a device from addr -> (addr + count)
 *
 * Return:      0 on success and 1 on failure.
 *************************************************************************************************/
typedef int (__cdecl *Fn_UsbWriteCfgReg)(const void *port, const void *addr, void *data, unsigned int count);



#ifdef __cplusplus
}
#endif

#endif
