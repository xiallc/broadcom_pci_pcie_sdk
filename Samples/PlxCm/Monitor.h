#ifndef _MONITOR_H
#define _MONITOR_H

/******************************************************************************
 *
 * File Name:
 *
 *      Monitor.h
 *
 * Description:
 *
 *      Definitions for the the monitor application
 *
 ******************************************************************************/


#include "PciDev.h"
#include "PlxTypes.h"
#include "CmdLine.h"

#if defined(PLX_MSWINDOWS)
    #include "..\\Shared\\ConsFunc.h"
#endif

#if defined(PLX_LINUX) || defined(PLX_DOS)
    #include "ConsFunc.h"
#endif




/*************************************
 *          Definitions
 ************************************/
#define MONITOR_VERSION_MAJOR       2       // Version information
#define MONITOR_VERSION_MINOR       6
#define MONITOR_VERSION_REVISION    0

#define MONITOR_PROMPT              ">"     // The monitor prompt string


#if defined(PLX_DOS)
    #define PlxPreventInWindows()   (getenv("OS") != NULL)
#else
    #define PlxPreventInWindows()   FALSE
#endif




/*************************************
 *            Globals
 ************************************/
extern PLX_LIST_ENTRY Gbl_ListCmds;
extern U64            Gbl_LastRetVal;




/*************************************
 *            Functions
 ************************************/
S8
ProcessMonitorParams(
    int   argc,
    char *argv[]
    );

VOID
Monitor(
    VOID
    );

PLXCM_COMMAND*
ProcessCommand(
    PLX_DEVICE_OBJECT *pDevice,
    char              *buffer
    );

DEVICE_NODE*
DeviceSelectByIndex(
    U8 index
    );

VOID
PciSpacesMap(
    PLX_DEVICE_OBJECT *pDevice
    );

VOID
PciSpacesUnmap(
    PLX_DEVICE_OBJECT *pDevice
    );

VOID
CommonBufferMap(
    PLX_DEVICE_OBJECT *pDevice
    );

VOID
CommonBufferUnmap(
    PLX_DEVICE_OBJECT *pDevice
    );

VOID
Mon_PostCommand(
    PLXCM_COMMAND *pCmd
    );


#endif
