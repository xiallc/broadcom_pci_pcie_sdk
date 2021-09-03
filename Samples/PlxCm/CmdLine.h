#ifndef _CMD_LINE_H
#define _CMD_LINE_H

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
#define MAX_ARG_LEN                 75      // Max length of each argument
#define VAR_TABLE_SIZE              100     // Num entries in variable table
#define CMD_FINAL                   0       // To mark final command in function table


// Added to avoid compile error
typedef struct _PLXCM_COMMAND *PTR_PLXCM_COMMAND;

typedef
BOOLEAN
(CMD_ROUTINE)(
    PLX_DEVICE_OBJECT     *pDevice,
    struct _PLXCM_COMMAND *pCmd
    );

typedef struct _FN_TABLE
{
    U8           CmdId;                 // Command ID
    U8           bAllowOps;             // Whether operation/variables are allowed
    char         strCmd[MAX_CMD_LEN];   // Command string
    CMD_ROUTINE *pCmdRoutine;           // Command routine
} FN_TABLE;

typedef struct _HELP_TABLE
{
    U8   CmdId;                         // Command ID
    char strHelpUsage[80*2];            // Help usage text
    char strHelpDetail[80*10];          // Help details
} HELP_TABLE;

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
    struct _PLXCM_COMMAND *pNextCmd;                    // Next command in list
    char                   szCmdLine[MAX_CMDLN_LEN];    // Full command line
    char                   szCmd[MAX_CMD_LEN];          // Actual command text
    BOOLEAN                bParsed;                     // Flag if command has been parsed
    BOOLEAN                bContainString;              // Flag if variable parameters exist
    BOOLEAN                bErrorParse;                 // Flag if parse error exists
    BOOLEAN                bAllowOps;                   // Flag if command allows variables/operations
    CMD_ROUTINE           *pCmdRoutine;                 // Pointer to the command routine
    U8                     NumArgs;                     // Total number of parameters
    PLX_LIST_ENTRY         List_Args;                   // Parameter list
} PLXCM_COMMAND;




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
    char    **pStr,
    char     *strToken,
    BOOLEAN   bAllowOps
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
    char     *buffer,
    FN_TABLE *pFnTable
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
    PLXCM_COMMAND *pCmd,
    FN_TABLE      *pFnTable
    );

BOOLEAN
CmdLine_CmdLookup(
    PLXCM_COMMAND *pCmd,
    FN_TABLE      *pFnTable
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
