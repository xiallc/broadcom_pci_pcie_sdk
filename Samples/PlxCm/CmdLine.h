#ifndef _CMD_LINE_H
#define _CMD_LINE_H

/******************************************************************************
 *
 * File Name:
 *
 *      CmdLine.h
 *
 * Description:
 *
 *      Header for the functions to support Command-line processing
 *
 ******************************************************************************/


#include "PlxTypes.h"
#if defined(PLX_MSWINDOWS)
    #include "..\\Shared\\PlxLists.h"
#else
    #include "PlxLists.h"
#endif




/*************************************
 *          Definitions
 ************************************/
#define MAX_CMDLN_LEN               512     // Max command-line length
#define MAX_CMD_LEN                 30      // Max length of commands
#define MAX_ARG_LEN                 25      // Max length of each argument
#define VAR_TABLE_SIZE              100     // Num entries in variable table


typedef enum _PLXCM_ARG_TYPE
{
    PLXCM_ARG_TYPE_INT,
    PLXCM_ARG_TYPE_STRING
} PLXCM_ARG_TYPE;


typedef enum _PLXCM_ARG_OP
{
    PLXCM_ARG_OP_NONE,
    PLXCM_ARG_OP_ADD,
    PLXCM_ARG_OP_SUB
} PLXCM_ARG_OP;


typedef struct _PLXCM_VAR
{
    PLX_LIST_ENTRY ListEntry;
    BOOLEAN        bSystemVar;
    char           strName[MAX_ARG_LEN];
    char           strValue[MAX_ARG_LEN];
} PLXCM_VAR;


typedef struct _PLXCM_ARG
{
    PLX_LIST_ENTRY  ListEntry;
    PLXCM_ARG_TYPE  ArgType;
    U64             ArgIntDec;               // Decimal value of string
    U64             ArgIntHex;               // Hex value of string
    char            ArgString[MAX_ARG_LEN];  // String value
    PLXCM_VAR      *pVar;                    // Point to variable if in table
} PLXCM_ARG;


typedef struct _PLXCM_COMMAND
{
    PLX_LIST_ENTRY         ListEntry;
    struct _PLXCM_COMMAND *pNextCmd;
    char                   szCmdLine[MAX_CMDLN_LEN];
    char                   szCmd[MAX_CMD_LEN];
    BOOLEAN                bParsed;
    BOOLEAN                bContainString;
    BOOLEAN                bErrorParse;
    VOID                  *pCmdRoutine;
    U8                     NumArgs;
    PLX_LIST_ENTRY         List_Args;
} PLXCM_COMMAND;


typedef
BOOLEAN
(*CMD_ROUTINE) (
    PLX_DEVICE_OBJECT *pDevice,
    PLXCM_COMMAND     *pCmd
    );




/*************************************
 *          Functions
 ************************************/
BOOLEAN
CmdLine_Initialize(
    VOID
    );

PLX_UINT_PTR
htol(
    char *hexString
    );

BOOLEAN
CmdLine_IsHexValue(
    char *pStr
    );

void
CmdLine_GetNextToken(
    char **Args,
    char  *buffer
    );

PLXCM_VAR*
CmdLine_VarLookup(
    char *pStr
    );

PLXCM_VAR*
CmdLine_VarGetByIndex(
    U8 index
    );

PLXCM_VAR*
CmdLine_VarAdd(
    char    *pStrVar,
    char    *pStrValue,
    BOOLEAN  bSystemVar
    );

BOOLEAN
CmdLine_VarDelete(
    char    *pStrVar,
    BOOLEAN  bSystemVar
    );

VOID
CmdLine_VarDeleteAll(
    VOID
    );

PLXCM_COMMAND*
CmdLine_CmdAdd(
    char *buffer
    );

VOID
CmdLine_CmdDeleteAll(
    VOID
    );

PLXCM_COMMAND*
CmdLine_CmdExists(
    char *buffer
    );

BOOLEAN
CmdLine_CmdParse(
    PLXCM_COMMAND *pCmd
    );

PLXCM_ARG*
CmdLine_ArgGet(
    PLXCM_COMMAND *pCmd,
    U8             ArgNum
    );

VOID
CmdLine_ArgDeleteAll(
    PLXCM_COMMAND *pCmd
    );

#endif
