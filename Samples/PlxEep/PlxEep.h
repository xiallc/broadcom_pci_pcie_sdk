#ifndef _PLX_EEP_H
#define _PLX_EEP_H

/******************************************************************************
 *
 * File Name:
 *
 *      PlxEep.h
 *
 * Description:
 *
 *      Definitions for the the EEPROM application
 *
 ******************************************************************************/


#include "PlxTypes.h"




/*************************************
 *          Definitions
 ************************************/
#define APP_VERSION_MAJOR           2       // Version information
#define APP_VERSION_MINOR           6
#define APP_VERSION_REVISION        0

#define BOOLEAN_UNKNOWN             4
#define MAX_DEVICES_TO_LIST         50

#define EXIT_CODE_SUCCESS           0
#define EXIT_CODE_NON_DOS           1
#define EXIT_CODE_CMD_LINE_ERR      2
#define EXIT_CODE_INVALID_DEV       3
#define EXIT_CODE_DEV_OPEN_ERR      4
#define EXIT_CODE_EEP_FAIL          5
#define EXIT_CODE_EEP_NOT_EXIST     6
#define EXIT_CODE_INVALID_PLX       7
#define EXIT_CODE_EEP_WIDTH_ERR     8


#if defined(PLX_DOS)
    #define PlxPreventInWindows()   (getenv("OS") != NULL)
#else
    #define PlxPreventInWindows()   FALSE
#endif


typedef struct _EEP_OPTIONS
{
    BOOLEAN bVerbose;
    BOOLEAN bLoadFile;
    BOOLEAN bIgnoreWarnings;
    char    FileName[200];
    S8      DeviceNumber;
    U8      EepWidthSet;
    U16     LimitPlxChip;
    U8      LimitPlxRevision;
    U16     ExtraBytes;
} EEP_OPTIONS;




/*************************************
 *            Functions
 ************************************/
S16
SelectDevice(
    PLX_DEVICE_KEY *pKey,
    EEP_OPTIONS    *pOptions
    );

S8
ProcessCommandLine(
    int          argc,
    char        *argv[],
    EEP_OPTIONS *pOptions
    );

VOID
DisplayHelp(
    VOID
    );

S8
EepFile(
    PLX_DEVICE_OBJECT *pDevice,
    EEP_OPTIONS       *pOptions
    );

S8
Plx_EepromFileLoad(
    PLX_DEVICE_OBJECT *pDevice,
    EEP_OPTIONS       *pOptions,
    U32                EepSize,
    U8                 EepPortSize,
    BOOLEAN            bCrc,
    BOOLEAN            bEndianSwap
    );

S8
Plx_EepromFileSave(
    PLX_DEVICE_OBJECT *pDevice,
    EEP_OPTIONS       *pOptions,
    U32                EepSize,
    U8                 EepPortSize,
    BOOLEAN            bCrc,
    BOOLEAN            bEndianSwap
    );

S8
Plx8000_EepromFileLoad(
    PLX_DEVICE_OBJECT *pDevice,
    EEP_OPTIONS       *pOptions,
    BOOLEAN            bCrc
    );

S8
Plx8000_EepromFileSave(
    PLX_DEVICE_OBJECT *pDevice,
    EEP_OPTIONS       *pOptions,
    U32                ByteCount,
    BOOLEAN            bCrc
    );



#endif
