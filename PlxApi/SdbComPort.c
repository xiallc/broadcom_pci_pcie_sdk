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
 *      SdbComPort.c
 *
 * Description:
 *
 *      Implements the PLX API functions over an SDB COM port interface
 *
 * Revision History:
 *
 *      01-01-19 : PCI/PCIe SDK v8.00
 *
 ******************************************************************************/

/*******************************************************************************
 *
 * SDB COM port API utilization of fields in PLX_DEVICE_KEY
 *
 *  ApiIndex        - COM/TTY port number (0,1,etc)
 *  DeviceNumber    - Flag if UART is USB-to-Serial cable
 *  DeviceMode      - Current chip mode (Std/Base, Fabric/Smart Switch, etc)
 *  PlxPort         - Port number
 *  PlxPortType     - PLX-specific port type (PLX_PORT_TYPE)
 *  ApiInternal[0]  - BAUD rate of connection
 *  ApiInternal[1]  - Last data read address
 *
 ******************************************************************************/


#include "PciRegs.h"
#include "PexApi.h"
#include "PlxApiDebug.h"
#include "PlxApiDirect.h"
#include "SdbComPort.h"

#if defined(PLX_LINUX)
    #include <termios.h>    // For serial port properties
    #include <errno.h>      // For errono variable
#endif




/**********************************************
 *               Definitions
 *********************************************/
#if defined(PLX_MSWINDOWS)

    typedef DCB                                 PLX_COM_PARAMS;
    #define Sdb_Driver_Disconnect               CloseHandle

#elif defined(PLX_LINUX)

    typedef struct termios                      PLX_COM_PARAMS;
    #define Sdb_Driver_Disconnect               close
    #define ReadFile(hdl,buf,sz,pCount,ovl)     *(pCount) = read( (hdl), (buf), (sz) )
    #define WriteFile(hdl,buf,sz,pCount,ovl)    *(pCount) = write( (hdl), (buf), (sz) )
    #define GetLastError()                      errno

#endif




/******************************************************************************
 *
 * Function   :  Sdb_DeviceOpen
 *
 * Description:  Selects a device
 *
 *****************************************************************************/
PLX_STATUS
Sdb_DeviceOpen(
    PLX_DEVICE_OBJECT *pDevice
    )
{
    // Open connection to driver
    if (Sdb_Driver_Connect( pDevice, NULL ) == FALSE)
    {
        return PLX_STATUS_INVALID_OBJECT;
    }

    // Mark object as valid
    ObjectValidate( pDevice );

    // Fill in chip version information
    PlxDir_ChipTypeDetect( pDevice );

    return PLX_STATUS_OK;
}




/******************************************************************************
 *
 * Function   :  Sdb_DeviceClose
 *
 * Description:  Closes a previously opened device
 *
 *****************************************************************************/
PLX_STATUS
Sdb_DeviceClose(
    PLX_DEVICE_OBJECT *pDevice
    )
{
    // Close device
    Sdb_Driver_Disconnect( pDevice->hDevice );

    // Clear handle
    pDevice->hDevice = 0;

    // Flag need to issue init command first
    pDevice->Key.ApiInternal[1] = SDB_NEEDS_INIT_CMD;

    return PLX_STATUS_OK;
}




/******************************************************************************
 *
 * Function   :  Sdb_DeviceFindEx
 *
 * Description:  Locates a specific device
 *
 *****************************************************************************/
PLX_STATUS
Sdb_DeviceFindEx(
    PLX_DEVICE_KEY *pKey,
    U16             DeviceNumber,
    PLX_MODE_PROP  *pModeProp
    )
{
    U16               numMatched;
    U16               totalMatches;
    U32               regVal;
    BOOLEAN           bFound;
    PLX_STATUS        status;
    PLX_DEVICE_OBJECT devObj;
    PLX_DEVICE_OBJECT devObjTemp;


    // Clear the device object
    RtlZeroMemory( &devObj, sizeof(PLX_DEVICE_OBJECT) );

    // Open connection to driver
    if (Sdb_Driver_Connect( &devObj, pModeProp ) == FALSE)
    {
        return PLX_STATUS_INVALID_OBJECT;
    }

    totalMatches = 0;

    // Must validate object so probe API calls don't fail
    ObjectValidate( &devObj );

    // Default to device not found
    bFound = FALSE;

    // Probe devices to check if valid device
    DebugPrintf(("SDB: ---- Probe via COM %d ----\n", pModeProp->Sdb.Port));

    // Reset temporary device object
    RtlCopyMemory( &devObjTemp, &devObj, sizeof(PLX_DEVICE_OBJECT) );

    // Attempt to read Device/Vendor ID
    regVal =
        Sdb_PlxRegisterRead(
            &devObjTemp,
            PEX_REG_CCR_DEV_ID,     // Port 0 Device/Vendor ID
            &status,
            FALSE,                  // Adjust for port?
            FALSE                   // Retry on error?
            );

    if (status == PLX_STATUS_OK)
    {
        // Check if supported chip ID (eg C010_1000h)
        if ( (regVal & 0xFF00FFFF) == (0xC0000000 | PLX_PCI_VENDOR_ID_LSI) )
        {
            DebugPrintf(("SDB: Detected device %08X\n", regVal));

            // Device found, determine active ports and compare
            status =
                PlxDir_ProbeSwitch(
                    &devObjTemp,
                    pKey,
                    (U16)(DeviceNumber - totalMatches),
                    &numMatched
                    );

            if (status == PLX_STATUS_OK)
            {
                bFound = TRUE;
            }
            else
            {
                // Add number of matched devices
                totalMatches += numMatched;
            }
        }
    }

    // Close the device
    Sdb_DeviceClose( &devObj );

    if (bFound)
    {
        // Store API mode
        pKey->ApiMode = PLX_API_MODE_SDB;

        // Validate key
        ObjectValidate( pKey );
        return PLX_STATUS_OK;
    }

    return PLX_STATUS_INVALID_OBJECT;
}




/*******************************************************************************
 *
 * Function   :  Sdb_PlxRegisterRead
 *
 * Description:  Reads a PLX-specific register
 *
 ******************************************************************************/
U32
Sdb_PlxRegisterRead(
    PLX_DEVICE_OBJECT *pDevice,
    U32                offset,
    PLX_STATUS        *pStatus,
    BOOLEAN            bAdjustForPort,
    U16                bRetryOnError
    )
{
    U8  maxAttempts;
    U8  sdbCmd[SDB_READ_CMD_LEN];
    U8  sdbReply[SDB_READ_REPLY_LEN];
    U32 regVal;
    U32 rxBytes;
    U32 byteCount;


    // Verify register offset
    if (offset & 0x3)
    {
        DebugPrintf(("SDB: ERROR: Invalid register offset (0x%x)\n", offset));
        if (pStatus != NULL)
        {
            *pStatus = PLX_STATUS_INVALID_OFFSET;
        }
        return 0;
    }

    // Default to error
    if (pStatus != NULL)
    {
        *pStatus = PLX_STATUS_FAILED;
    }

    // Adjust offset for port if requested
    if (bAdjustForPort)
    {
        // Adjust offset to port-specific register region
        offset += ATLAS_REGS_AXI_BASE_ADDR + (pDevice->Key.PlxPort * 0x1000);
    }

    // Check for first operation
    if (pDevice->Key.ApiInternal[1] == SDB_NEEDS_INIT_CMD)
    {
        Sdb_Sync_Connection( pDevice );
    }

    // Set max number of attempts
    maxAttempts = SDB_MAX_ATTEMPTS;

    while (maxAttempts)
    {
        // Use READ_NEXT command for optimal performance if sequential read
        if (pDevice->Key.ApiInternal[1] == offset)
        {
            // Prepare command
            sdbCmd[0] = SDB_CMD_READ_NEXT;  // Read next operation
            sdbCmd[1] = SDB_CMD_END;        // End of command (<CR> or <LF>)

            // Send command
            WriteFile(
                pDevice->hDevice,
                sdbCmd,
                SDB_READ_NEXT_CMD_LEN,
                &byteCount,
                NULL
                );

            if (byteCount != SDB_READ_NEXT_CMD_LEN)
            {
                ErrorPrintf((
                    "SDB: ERROR: READ_NEXT command failed, sent %dB of %dB\n",
                    byteCount, SDB_READ_NEXT_CMD_LEN
                    ));
            }
        }
        else
        {
            // Prepare command
            sdbCmd[0] = SDB_CMD_READ;       // Read operation
            sdbCmd[1] = sizeof(U32);        // 4 bytes
            sdbCmd[2] = (U8)(offset >> 24); // 4B address
            sdbCmd[3] = (U8)(offset >> 16);
            sdbCmd[4] = (U8)(offset >> 8);
            sdbCmd[5] = (U8)(offset >> 0);
            sdbCmd[6] = SDB_CMD_END;        // End of command (<CR> or <LF>)

            // Send command
            WriteFile(
                pDevice->hDevice,
                sdbCmd,
                SDB_READ_CMD_LEN,
                &byteCount,
                NULL
                );

            if (byteCount != SDB_READ_CMD_LEN)
            {
                ErrorPrintf((
                    "SDB: ERROR: READ command failed, sent %dB of %dB\n",
                    byteCount, SDB_READ_CMD_LEN
                    ));
            }
        }

        // Get reply by combining all received data
        byteCount = 0;
        do
        {
            ReadFile(
                pDevice->hDevice,
                &(sdbReply[byteCount]),
                sizeof(sdbReply) - byteCount,
                &rxBytes,
                NULL
                );

            // Update total bytes
            byteCount += rxBytes;
        }
        while ( (byteCount != SDB_READ_REPLY_LEN) && (rxBytes != 0) );

        // Check for valid response
        if ( (byteCount != SDB_READ_REPLY_LEN) ||
             ((byteCount < SDB_READ_REPLY_LEN) && (sdbReply[0] == SDB_CMD_ERROR)) ||
             (sdbReply[SDB_READ_REPLY_LEN - 1] != SDB_CMD_ACK) )
        {
            regVal = 0;

            // Decrement attempt count & re-sync
            maxAttempts--;
            Sdb_Sync_Connection( pDevice );
        }
        else
        {
            // Extract data value
            regVal = ((U32)sdbReply[0] << 24) |
                     ((U32)sdbReply[1] << 16) |
                     ((U32)sdbReply[2] <<  8) |
                     ((U32)sdbReply[3] <<  0);

            if (pStatus != NULL)
            {
                *pStatus = PLX_STATUS_OK;
            }

            // Update ofset for next read command
            pDevice->Key.ApiInternal[1] = offset + sizeof(U32);

            // Flag success
            maxAttempts = 0;
        }
    }

    return regVal;
}




/*******************************************************************************
 *
 * Function   :  Sdb_PlxRegisterWrite
 *
 * Description:  Writes to a PLX-specific register
 *
 ******************************************************************************/
PLX_STATUS
Sdb_PlxRegisterWrite(
    PLX_DEVICE_OBJECT *pDevice,
    U32                offset,
    U32                value,
    BOOLEAN            bAdjustForPort
    )
{
    U8  maxAttempts;
    U8  sdbCmd[SDB_WRITE_CMD_LEN];
    U8  sdbReply[SDB_WRITE_REPLY_LEN];
    U32 rxBytes;
    U32 byteCount;


    // Verify register offset
    if (offset & 0x3)
    {
        DebugPrintf(("SDB: ERROR - Invalid register offset (0x%x)\n", offset));
        return PLX_STATUS_INVALID_OFFSET;
    }

    // Adjust offset for port if requested
    if (bAdjustForPort)
    {
        // Adjust offset to port-specific register region
        offset += ATLAS_REGS_AXI_BASE_ADDR + (pDevice->Key.PlxPort * 0x1000);
    }

    // Check for first operation
    if (pDevice->Key.ApiInternal[1] == SDB_NEEDS_INIT_CMD)
    {
        Sdb_Sync_Connection( pDevice );
    }

    // Reset next read offset since write operation now
    pDevice->Key.ApiInternal[1] = SDB_NEXT_READ_OFFSET_INIT;

    // Set max number of attempts
    maxAttempts = SDB_MAX_ATTEMPTS;

    while (maxAttempts)
    {
        // Prepare command
        sdbCmd[0]  = SDB_CMD_WRITE;      // Write operation
        sdbCmd[1]  = sizeof(U32);        // 4 bytes
        sdbCmd[2]  = (U8)(offset >> 24); // 4B address
        sdbCmd[3]  = (U8)(offset >> 16);
        sdbCmd[4]  = (U8)(offset >>  8);
        sdbCmd[5]  = (U8)(offset >>  0);
        sdbCmd[6]  = (U8)(value >> 24);  // 4B data
        sdbCmd[7]  = (U8)(value >> 16);
        sdbCmd[8]  = (U8)(value >>  8);
        sdbCmd[9]  = (U8)(value >>  0);
        sdbCmd[10] = SDB_CMD_END;        // End of command (<CR> or <LF>)

        // Send command
        WriteFile(
            pDevice->hDevice,
            sdbCmd,
            SDB_WRITE_CMD_LEN,
            &byteCount,
            NULL
            );

        if (byteCount != SDB_WRITE_CMD_LEN)
        {
            ErrorPrintf((
                "SDB: ERROR: WRITE command failed, sent %dB of %dB\n",
                byteCount, SDB_WRITE_CMD_LEN
                ));
        }

        // Get reply by combining all received data
        byteCount = 0;
        do
        {
            ReadFile(
                pDevice->hDevice,
                &(sdbReply[byteCount]),
                sizeof(sdbReply) - byteCount,
                &rxBytes,
                NULL
                );

            // Update total bytes
            byteCount += rxBytes;
        }
        while ( (byteCount != SDB_WRITE_REPLY_LEN) && (rxBytes != 0) );

        // Check for valid response
        if ( (byteCount != SDB_WRITE_REPLY_LEN) ||
             (sdbReply[0] == SDB_CMD_ERROR) ||
             (sdbReply[SDB_WRITE_REPLY_LEN - 1] != SDB_CMD_ACK) )
        {
            // Decrement attempt count & re-sync
            maxAttempts--;
            Sdb_Sync_Connection( pDevice );
        }
        else
        {
            // Operation succeeded, exit
            return PLX_STATUS_OK;
        }
    }

    return PLX_STATUS_FAILED;
}




/***********************************************************
 *
 *               PRIVATE SUPPORT FUNCTIONS
 *
 **********************************************************/


/******************************************************************************
 *
 * Function   :  Sdb_Driver_Connect
 *
 * Description:  Attempts to connect SDB over COM/TTY port
 *
 * Returns    :  TRUE   - Driver was found and connected to
 *               FALSE  - Driver not found
 *
 *****************************************************************************/
BOOLEAN
Sdb_Driver_Connect(
    PLX_DEVICE_OBJECT *pDevice,
    PLX_MODE_PROP     *pModeProp
    )
{
    U8   bReconnect;
    char strPortName[100];


    // If re-connecting, must close current handle
    bReconnect = FALSE;
    if (pDevice->hDevice != 0)
    {
        Sdb_Driver_Disconnect( pDevice->hDevice );
        pDevice->hDevice = 0;
        bReconnect = TRUE;
    }

    // Added to avoid compiler warning
    if (bReconnect)
    {
    }

    // If mode properties supplied, copy into device object
    if (pModeProp != NULL)
    {
        pDevice->Key.ApiMode  = PLX_API_MODE_SDB;
        pDevice->Key.ApiIndex = (U8)pModeProp->Sdb.Port;

        // Set baud rate
        pDevice->Key.ApiInternal[0] = SDB_OS_BAUD_115200;
        if (pModeProp->Sdb.Baud == SDB_BAUD_RATE_19200)
        {
            pDevice->Key.ApiInternal[0] = SDB_OS_BAUD_19200;
        }

        // Set cable type
        pDevice->Key.DeviceNumber = SDB_UART_CABLE_UART;
        if (pModeProp->Sdb.Cable == SDB_UART_CABLE_USB)
        {
            pDevice->Key.DeviceNumber = SDB_UART_CABLE_USB;
        }
    }

    // Flag need to issue init command first
    pDevice->Key.ApiInternal[1] = SDB_NEEDS_INIT_CMD;

    //
    // Attempt connection to serial port
    //

#if defined(PLX_MSWINDOWS)

    {
        HANDLE         hComm;
        COMMTIMEOUTS   commTimeouts = { 0 };
        PLX_COM_PARAMS commParams = { 0 };

        // Build port name (eg \\.\COM1)
        sprintf(
            strPortName,
            "%s%d",
            SDB_OS_COM_PORT_UART, pDevice->Key.ApiIndex
            );

        DebugPrintf((
            "SDB: Attempt %sconnect via %s\n",
            (bReconnect) ? "re" : "",
            strPortName
            ));

        // Attempt to open port
        hComm =
            CreateFile(
                strPortName,                    // Port name
                GENERIC_READ | GENERIC_WRITE,   // Read/Write
                0,                              // No Sharing
                NULL,                           // No Security
                OPEN_EXISTING,                  // Open existing port
                0,                              // Non-Overlapped I/O
                NULL                            // Null for Comm Devices
                );

        if (hComm == INVALID_HANDLE_VALUE)
        {
            DebugPrintf(("SDB: ERROR: Unable to open %s\n", strPortName));
            return FALSE;
        }

        // Store handle
        pDevice->hDevice = hComm;

        //
        // Configure serial properties
        //

        // Recommend Input & Ouput buffer sizes to driver
        SetupComm( hComm, 1500, 1500 );

        // Clear any existing buffer data & halt any existing operations
        PurgeComm(
            hComm,
            PURGE_TXABORT | PURGE_RXABORT |
            PURGE_TXCLEAR | PURGE_RXCLEAR
            );

        // Set timeouts
        commTimeouts.ReadIntervalTimeout         = 50;
        commTimeouts.ReadTotalTimeoutConstant    = 50;
        commTimeouts.ReadTotalTimeoutMultiplier  = 10;
        commTimeouts.WriteTotalTimeoutConstant   = 50;
        commTimeouts.WriteTotalTimeoutMultiplier = 10;
        SetCommTimeouts( hComm, &commTimeouts );

        // Get current comm properties
        commParams.DCBlength = sizeof(DCB);
        GetCommState( hComm, &commParams );

        // Set baud rate
        commParams.BaudRate = pDevice->Key.ApiInternal[0];

        // Set connection properties to 8-N-1
        commParams.ByteSize = 8;
        commParams.Parity   = NOPARITY;
        commParams.StopBits = ONESTOPBIT;

        // Additional standard required properties
        commParams.fBinary     = TRUE;      // Only binary mode is supported
        commParams.fDtrControl = DTR_CONTROL_DISABLE;
        commParams.fRtsControl = RTS_CONTROL_DISABLE;

        // Disable flow control
        commParams.fOutxCtsFlow = FALSE;
        commParams.fOutxDsrFlow = FALSE;
        commParams.fInX         = FALSE;
        commParams.fOutX        = FALSE;

        // Update comm properties
        SetCommState( hComm, &commParams );
    }

#elif defined(PLX_LINUX)

    {
        U8             portNum;
        HANDLE         hComm;
        PLX_COM_PARAMS commParams;

        // Set port number (eg COM1 = index 0)
        portNum = pDevice->Key.ApiIndex;
        if (portNum > 0)
        {
            portNum--;
        }

        // Build port name (eg COM1 = /dev/ttyS0)
        sprintf(
            strPortName,
            "%s%d",
            (pDevice->Key.DeviceNumber == SDB_UART_CABLE_USB) ?
                SDB_OS_COM_PORT_USB : SDB_OS_COM_PORT_UART,
            portNum
            );

        DebugPrintf((
            "SDB: Attempt %sconnect via %s\n",
            (bReconnect) ? "re" : "",
            strPortName
            ));

        // Attempt to open port
        hComm = open( strPortName, O_RDWR | O_NOCTTY );
        if (hComm == INVALID_HANDLE_VALUE)
        {
            DebugPrintf(("SDB: ERROR: Unable to open %s\n", strPortName));
            return FALSE;
        }

        // Store handle
        pDevice->hDevice = hComm;

        // Clear any existing buffer data & halt any existing operations
        tcflush( hComm, TCIOFLUSH );

        //
        // Configure serial properties
        //

        // Get current comm properties
        tcgetattr( hComm, &commParams );

        // Set baud rate for both read & write
        cfsetispeed( &commParams, (speed_t)pDevice->Key.ApiInternal[0] );
        cfsetospeed( &commParams, (speed_t)pDevice->Key.ApiInternal[0] );

        // Disable input & output processing
        commParams.c_iflag &= ~(IGNBRK | BRKINT | PARMRK | ISTRIP |
                                INLCR | IGNCR | ICRNL);
        commParams.c_oflag &= ~OPOST;

        // Set connection properties to 8-N-1
        commParams.c_cflag &= ~CSIZE;
        commParams.c_cflag |=  CS8;
        commParams.c_cflag &= ~PARENB;
        commParams.c_cflag &= ~CSTOPB;

        // Disable flow control
        commParams.c_cflag &= ~CRTSCTS;
        commParams.c_iflag &= ~(IXON | IXOFF | IXANY);

        // Enable serial port receiver
        commParams.c_cflag |= CREAD | CLOCAL;

        // Set non-canonical mode for raw binary operation
        commParams.c_lflag &= ~(ICANON | ECHO | ECHOE | ECHONL | ISIG | IEXTEN);

        // Set timeouts
        commParams.c_cc[VMIN]  = 0;     // Min characters to wait for
        commParams.c_cc[VTIME] = 3;     // In deciseconds (1ds = 100ms)

        // Update comm properties
        tcsetattr( hComm, TCSANOW, &commParams );
    }

#endif

    DebugPrintf(("SDB: Connect complete\n"));
    return TRUE;
}




/******************************************************************************
 *
 * Function   :  Sdb_Sync_Connection
 *
 * Description:  Re-initializes the SDB connection
 *
 ******************************************************************************/
BOOLEAN
Sdb_Sync_Connection(
    PLX_DEVICE_OBJECT *pDevice
    )
{
    U8  reply;
    U32 byteCount;
    U32 totalBytes;


    DebugPrintf(("SDB: Attempt sync by sending INIT string\n"));

    // Send init sequence
    WriteFile(
        pDevice->hDevice,
        SDB_CMD_INIT,
        strlen(SDB_CMD_INIT),
        &byteCount,
        NULL
        );

    if (byteCount != strlen(SDB_CMD_INIT))
    {
        ErrorPrintf((
            "SDB: ERROR: Sent %dB of sync string %dB (err=%d)\n",
            byteCount, strlen(SDB_CMD_INIT), GetLastError()
            ));
        /************************************************************
         * In some case, such as if the connected device is powered
         * off/on, the serial port will refuse to write any data. The
         * only recovery is close the handle & re-connect.
         ***********************************************************/
        Sdb_Driver_Connect( pDevice, NULL );
        return FALSE;
    }

    // Flush out any reply
    totalBytes = 0;
    do
    {
        ReadFile(
            pDevice->hDevice,
            &reply,
            sizeof(U8),
            &byteCount,
            NULL
            );

        // Track total bytes received
        totalBytes += byteCount;
    }
    while (byteCount != 0);

    if (totalBytes == 0)
    {
        ErrorPrintf(("SDB: ERROR: No data received after INIT string\n"));
        return FALSE;
    }

    // Reset next read offset
    pDevice->Key.ApiInternal[1] = SDB_NEXT_READ_OFFSET_INIT;

    DebugPrintf(("SDB: Sync complete\n"));
    return TRUE;
}




/******************************************************************************
 *
 * Function   :  Sdb_Dispatch_IoControl
 *
 * Description:  Processes the IOCTL messages
 *
 ******************************************************************************/
S32
Sdb_Dispatch_IoControl(
    PLX_DEVICE_OBJECT *pDevice,
    U32                IoControlCode,
    PLX_PARAMS        *pIoBuffer,
    U32                Size
    )
{
    DebugPrintf_Cont(("\n"));
    DebugPrintf(("Received PLX SDB message ===> "));

    // Handle the PLX specific message
    switch (IoControlCode)
    {
        /******************************************
         * Driver Query Functions
         *****************************************/
        case PLX_IOCTL_DRIVER_VERSION:
            DebugPrintf_Cont(("PLX_IOCTL_DRIVER_VERSION\n"));

            pIoBuffer->value[0] =
                (PLX_SDK_VERSION_MAJOR << 16) |
                (PLX_SDK_VERSION_MINOR <<  8) |
                (0                     <<  0);
            break;

        case PLX_IOCTL_CHIP_TYPE_GET:
            DebugPrintf_Cont(("PLX_IOCTL_CHIP_TYPE_GET\n"));

            pIoBuffer->ReturnCode =
                PlxDir_ChipTypeGet(
                    pDevice,
                    (U16*)&(pIoBuffer->value[0]),
                    (U8*)&(pIoBuffer->value[1])
                    );
            break;

        case PLX_IOCTL_GET_PORT_PROPERTIES:
            DebugPrintf_Cont(("PLX_IOCTL_GET_PORT_PROPERTIES\n"));

            pIoBuffer->ReturnCode =
                PlxDir_GetPortProperties(
                    pDevice,
                    &(pIoBuffer->u.PortProp)
                    );
            break;


        /******************************************
         * PCI Register Access Functions
         *****************************************/
        case PLX_IOCTL_PCI_REGISTER_READ:
            DebugPrintf_Cont(("PLX_IOCTL_PCI_REGISTER_READ\n"));

            pIoBuffer->value[1] =
                Sdb_PlxRegisterRead(
                    pDevice,
                    (U32)pIoBuffer->value[0],
                    &(pIoBuffer->ReturnCode),
                    TRUE,       // Adjust for port?
                    TRUE        // Retry on error?
                    );

            DebugPrintf((
                "PCI Reg %03X = %08lX\n",
                (U16)pIoBuffer->value[0],
                (U32)pIoBuffer->value[1]
                ));
            break;

        case PLX_IOCTL_PCI_REGISTER_WRITE:
            DebugPrintf_Cont(("PLX_IOCTL_PCI_REGISTER_WRITE\n"));

            pIoBuffer->ReturnCode =
                Sdb_PlxRegisterWrite(
                    pDevice,
                    (U32)pIoBuffer->value[0],
                    (U32)pIoBuffer->value[1],
                    TRUE        // Adjust for port?
                    );

            DebugPrintf((
                "Wrote %08lX to PCI Reg %03X\n",
                (U32)pIoBuffer->value[1],
                (U16)pIoBuffer->value[0]
                ));
            break;


        /******************************************
         * PLX-specific Register Access Functions
         *****************************************/
        case PLX_IOCTL_REGISTER_READ:
            DebugPrintf_Cont(("PLX_IOCTL_REGISTER_READ\n"));

            pIoBuffer->value[1] =
                Sdb_PlxRegisterRead(
                    pDevice,
                    (U32)pIoBuffer->value[0],
                    &(pIoBuffer->ReturnCode),
                    TRUE,       // Adjust for port?
                    TRUE        // Retry on error?
                    );

            DebugPrintf((
                "PLX Reg %03lX = %08lX\n",
                (U32)pIoBuffer->value[0],
                (U32)pIoBuffer->value[1]
                ));
            break;

        case PLX_IOCTL_REGISTER_WRITE:
            DebugPrintf_Cont(("PLX_IOCTL_REGISTER_WRITE\n"));

            pIoBuffer->ReturnCode =
                Sdb_PlxRegisterWrite(
                    pDevice,
                    (U32)pIoBuffer->value[0],
                    (U32)pIoBuffer->value[1],
                    TRUE        // Adjust for port?
                    );

            DebugPrintf((
                "Wrote %08lX to PLX Reg %03lX\n",
                (U32)pIoBuffer->value[1],
                (U32)pIoBuffer->value[0]
                ));
            break;

        case PLX_IOCTL_MAPPED_REGISTER_READ:
            DebugPrintf_Cont(("PLX_IOCTL_MAPPED_REGISTER_READ\n"));

            pIoBuffer->value[1] =
                Sdb_PlxRegisterRead(
                    pDevice,
                    (U32)pIoBuffer->value[0],
                    &(pIoBuffer->ReturnCode),
                    FALSE,      // Adjust for port?
                    TRUE        // Retry on error?
                    );

            DebugPrintf((
                "PLX Mapped Reg %03lX = %08lX\n",
                (U32)pIoBuffer->value[0],
                (U32)pIoBuffer->value[1]
                ));
            break;

        case PLX_IOCTL_MAPPED_REGISTER_WRITE:
            DebugPrintf_Cont(("PLX_IOCTL_MAPPED_REGISTER_WRITE\n"));

            pIoBuffer->ReturnCode =
                Sdb_PlxRegisterWrite(
                    pDevice,
                    (U32)pIoBuffer->value[0],
                    (U32)pIoBuffer->value[1],
                    FALSE       // Adjust for port?
                    );

            DebugPrintf((
                "Wrote %08lX to PLX Mapped Reg %03lX\n",
                (U32)pIoBuffer->value[1],
                (U32)pIoBuffer->value[0]
                ));
            break;


        /******************************************
         * PCI BAR Functions
         *****************************************/
        case PLX_IOCTL_PCI_BAR_PROPERTIES:
            DebugPrintf_Cont(("PLX_IOCTL_PCI_BAR_PROPERTIES\n"));

            pIoBuffer->ReturnCode =
                PlxDir_PciBarProperties(
                    pDevice,
                    (U8)(pIoBuffer->value[0]),
                    &(pIoBuffer->u.BarProp)
                    );
            break;


        /******************************************
         * PLX Performance Object Functions
         *****************************************/
        case PLX_IOCTL_PERFORMANCE_INIT_PROPERTIES:
            DebugPrintf_Cont(("PLX_IOCTL_PERFORMANCE_INIT_PROPERTIES\n"));

            pIoBuffer->ReturnCode =
                PlxDir_PerformanceInitializeProperties(
                    pDevice,
                    PLX_INT_TO_PTR(pIoBuffer->value[0])
                    );
            break;

        case PLX_IOCTL_PERFORMANCE_MONITOR_CTRL:
            DebugPrintf_Cont(("PLX_IOCTL_PERFORMANCE_MONITOR_CTRL\n"));

            pIoBuffer->ReturnCode =
                PlxDir_PerformanceMonitorControl(
                    pDevice,
                    (PLX_PERF_CMD)pIoBuffer->value[0]
                    );
            break;

        case PLX_IOCTL_PERFORMANCE_RESET_COUNTERS:
            DebugPrintf_Cont(("PLX_IOCTL_PERFORMANCE_RESET_COUNTERS\n"));

            pIoBuffer->ReturnCode =
                PlxDir_PerformanceResetCounters(
                    pDevice
                    );
            break;

        case PLX_IOCTL_PERFORMANCE_GET_COUNTERS:
            DebugPrintf_Cont(("PLX_IOCTL_PERFORMANCE_GET_COUNTERS\n"));

            pIoBuffer->ReturnCode =
                PlxDir_PerformanceGetCounters(
                    pDevice,
                    PLX_INT_TO_PTR(pIoBuffer->value[0]),
                    (U8)pIoBuffer->value[1]
                    );
            break;


        /******************************************
         * Unsupported Messages
         *****************************************/
        default:
            DebugPrintf_Cont((
                "Unsupported PLX_IOCTL_Xxx (%08Xh)\n",
                IoControlCode
                ));

            pIoBuffer->ReturnCode = PLX_STATUS_UNSUPPORTED;
            break;
    }

    DebugPrintf(("...Completed message\n"));
    return 0;
}
