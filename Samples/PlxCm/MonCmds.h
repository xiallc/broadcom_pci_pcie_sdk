#ifndef _MONITOR_COMMANDS_H
#define _MONITOR_COMMANDS_H

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
 *      MonCmds.h
 *
 * Description:
 *
 *      Header for the Monitor commands
 *
 ******************************************************************************/


#include "CmdLine.h"




/*************************************
 *          Definitions
 ************************************/
#define MIN_BYTE_CHECK_CANCEL         (4 * 1024)   // Min bytes before check for user abort

typedef enum
{
    CMD_CLEAR       = 50,
    CMD_VERSION,
    CMD_HELP,
    CMD_EXIT,
    CMD_SLEEP,
    CMD_SCREEN,
    CMD_THROTTLE,
    CMD_HISTORY,
    CMD_SCAN,
    CMD_SET_CHIP,
    CMD_RESET,
    CMD_DEV,
    CMD_I2C,
    CMD_MDIO,
    CMD_SDB,
    CMD_PCI_CAP,
    CMD_PORT_PROP,
    CMD_MH_PROP,
    CMD_VARS,
    CMD_VAR_SET,
    CMD_BUFFER,
    CMD_MEM_READ,
    CMD_MEM_WRITE,
    CMD_IO_READ,
    CMD_IO_WRITE,
    CMD_REG_PCI,
    CMD_REG_PLX,
    CMD_REG_DUMP,
    CMD_EEP,
    CMD_EEP_FILE,
    CMD_NVME_PROP
} MON_CMD_ID;



/*************************************
 *          Functions
 ************************************/
BOOLEAN Cmd_ConsClear  ( PLX_DEVICE_OBJECT *pDevice, PLXCM_COMMAND *pCmd );
BOOLEAN Cmd_Version    ( PLX_DEVICE_OBJECT *pDevice, PLXCM_COMMAND *pCmd );
BOOLEAN Cmd_Help       ( PLX_DEVICE_OBJECT *pDevice, PLXCM_COMMAND *pCmd );
BOOLEAN Cmd_Sleep      ( PLX_DEVICE_OBJECT *pDevice, PLXCM_COMMAND *pCmd );
BOOLEAN Cmd_Screen     ( PLX_DEVICE_OBJECT *pDevice, PLXCM_COMMAND *pCmd );
BOOLEAN Cmd_Throttle   ( PLX_DEVICE_OBJECT *pDevice, PLXCM_COMMAND *pCmd );
BOOLEAN Cmd_History    ( PLX_DEVICE_OBJECT *pDevice, PLXCM_COMMAND *pCmd );
BOOLEAN Cmd_Scan       ( PLX_DEVICE_OBJECT *pDevice, PLXCM_COMMAND *pCmd );
BOOLEAN Cmd_SetChip    ( PLX_DEVICE_OBJECT *pDevice, PLXCM_COMMAND *pCmd );
BOOLEAN Cmd_Reset      ( PLX_DEVICE_OBJECT *pDevice, PLXCM_COMMAND *pCmd );
BOOLEAN Cmd_Dev        ( PLX_DEVICE_OBJECT *pDevice, PLXCM_COMMAND *pCmd );
BOOLEAN Cmd_I2cConnect ( PLX_DEVICE_OBJECT *pDevice, PLXCM_COMMAND *pCmd );
BOOLEAN Cmd_MdioConnect( PLX_DEVICE_OBJECT *pDevice, PLXCM_COMMAND *pCmd );
BOOLEAN Cmd_SdbConnect ( PLX_DEVICE_OBJECT *pDevice, PLXCM_COMMAND *pCmd );
BOOLEAN Cmd_PciCapList ( PLX_DEVICE_OBJECT *pDevice, PLXCM_COMMAND *pCmd );
BOOLEAN Cmd_PortProp   ( PLX_DEVICE_OBJECT *pDevice, PLXCM_COMMAND *pCmd );
BOOLEAN Cmd_MH_Prop    ( PLX_DEVICE_OBJECT *pDevice, PLXCM_COMMAND *pCmd );
BOOLEAN Cmd_VarDisplay ( PLX_DEVICE_OBJECT *pDevice, PLXCM_COMMAND *pCmd );
BOOLEAN Cmd_VarSet     ( PLX_DEVICE_OBJECT *pDevice, PLXCM_COMMAND *pCmd );
BOOLEAN Cmd_ShowBuffer ( PLX_DEVICE_OBJECT *pDevice, PLXCM_COMMAND *pCmd );
BOOLEAN Cmd_MemRead    ( PLX_DEVICE_OBJECT *pDevice, PLXCM_COMMAND *pCmd );
BOOLEAN Cmd_MemWrite   ( PLX_DEVICE_OBJECT *pDevice, PLXCM_COMMAND *pCmd );
BOOLEAN Cmd_IoRead     ( PLX_DEVICE_OBJECT *pDevice, PLXCM_COMMAND *pCmd );
BOOLEAN Cmd_IoWrite    ( PLX_DEVICE_OBJECT *pDevice, PLXCM_COMMAND *pCmd );
BOOLEAN Cmd_RegPci     ( PLX_DEVICE_OBJECT *pDevice, PLXCM_COMMAND *pCmd );
BOOLEAN Cmd_RegPlx     ( PLX_DEVICE_OBJECT *pDevice, PLXCM_COMMAND *pCmd );
BOOLEAN Cmd_RegDump    ( PLX_DEVICE_OBJECT *pDevice, PLXCM_COMMAND *pCmd );
BOOLEAN Cmd_Eep        ( PLX_DEVICE_OBJECT *pDevice, PLXCM_COMMAND *pCmd );
BOOLEAN Cmd_Eep8000    ( PLX_DEVICE_OBJECT *pDevice, PLXCM_COMMAND *pCmd );
BOOLEAN Cmd_EepFile    ( PLX_DEVICE_OBJECT *pDevice, PLXCM_COMMAND *pCmd );
BOOLEAN Cmd_NvmeProp   ( PLX_DEVICE_OBJECT *pDevice, PLXCM_COMMAND *pCmd );



#endif
