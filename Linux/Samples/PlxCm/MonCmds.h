#ifndef _MONITOR_COMMANDS_H
#define _MONITOR_COMMANDS_H

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
 *          Functions
 ************************************/
BOOLEAN Cmd_Version   ( PLX_DEVICE_OBJECT *pDevice, PLXCM_COMMAND *pCmd );
BOOLEAN Cmd_Help      ( PLX_DEVICE_OBJECT *pDevice, PLXCM_COMMAND *pCmd );
BOOLEAN Cmd_Sleep     ( PLX_DEVICE_OBJECT *pDevice, PLXCM_COMMAND *pCmd );
BOOLEAN Cmd_Boot      ( PLX_DEVICE_OBJECT *pDevice, PLXCM_COMMAND *pCmd );
BOOLEAN Cmd_Screen    ( PLX_DEVICE_OBJECT *pDevice, PLXCM_COMMAND *pCmd );
BOOLEAN Cmd_History   ( PLX_DEVICE_OBJECT *pDevice, PLXCM_COMMAND *pCmd );
BOOLEAN Cmd_Scan      ( PLX_DEVICE_OBJECT *pDevice, PLXCM_COMMAND *pCmd );
BOOLEAN Cmd_SetChip   ( PLX_DEVICE_OBJECT *pDevice, PLXCM_COMMAND *pCmd );
BOOLEAN Cmd_Reset     ( PLX_DEVICE_OBJECT *pDevice, PLXCM_COMMAND *pCmd );
BOOLEAN Cmd_Dev       ( PLX_DEVICE_OBJECT *pDevice, PLXCM_COMMAND *pCmd );
BOOLEAN Cmd_I2cConnect( PLX_DEVICE_OBJECT *pDevice, PLXCM_COMMAND *pCmd );
BOOLEAN Cmd_PciCapList( PLX_DEVICE_OBJECT *pDevice, PLXCM_COMMAND *pCmd );
BOOLEAN Cmd_PortProp  ( PLX_DEVICE_OBJECT *pDevice, PLXCM_COMMAND *pCmd );
BOOLEAN Cmd_MH_Prop   ( PLX_DEVICE_OBJECT *pDevice, PLXCM_COMMAND *pCmd );
BOOLEAN Cmd_VarDisplay( PLX_DEVICE_OBJECT *pDevice, PLXCM_COMMAND *pCmd );
BOOLEAN Cmd_VarSet    ( PLX_DEVICE_OBJECT *pDevice, PLXCM_COMMAND *pCmd );
BOOLEAN Cmd_ShowBuffer( PLX_DEVICE_OBJECT *pDevice, PLXCM_COMMAND *pCmd );
BOOLEAN Cmd_MemRead   ( PLX_DEVICE_OBJECT *pDevice, PLXCM_COMMAND *pCmd );
BOOLEAN Cmd_MemWrite  ( PLX_DEVICE_OBJECT *pDevice, PLXCM_COMMAND *pCmd );
BOOLEAN Cmd_IoRead    ( PLX_DEVICE_OBJECT *pDevice, PLXCM_COMMAND *pCmd );
BOOLEAN Cmd_IoWrite   ( PLX_DEVICE_OBJECT *pDevice, PLXCM_COMMAND *pCmd );
BOOLEAN Cmd_RegPci    ( PLX_DEVICE_OBJECT *pDevice, PLXCM_COMMAND *pCmd );
BOOLEAN Cmd_RegPlx    ( PLX_DEVICE_OBJECT *pDevice, PLXCM_COMMAND *pCmd );
BOOLEAN Cmd_Eep       ( PLX_DEVICE_OBJECT *pDevice, PLXCM_COMMAND *pCmd );
BOOLEAN Cmd_Eep8000   ( PLX_DEVICE_OBJECT *pDevice, PLXCM_COMMAND *pCmd );
BOOLEAN Cmd_EepFile   ( PLX_DEVICE_OBJECT *pDevice, PLXCM_COMMAND *pCmd );




#endif
