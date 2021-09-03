/*******************************************************************************
 * Copyright 2013-2016 Avago Technologies
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

/*********************************************************************
 *
 * Module Name:
 *
 *     CmdLine.c
 *
 * Abstract:
 *
 *     Functions to support Command-line processing
 *
 ********************************************************************/


#if defined(_WIN32)
    #define _CRT_NONSTDC_NO_WARNINGS        // Prevents POSIX function warnings
#endif


#include <ctype.h>      // isxdigit, isspace
#include <malloc.h>     // malloc, free
#include "CmdLine.h"
#include "MonCmds.h"
#include "Monitor.h"
#include "PlxTypes.h"




/**********************************************
 *                 Globals
 **********************************************/
PLX_LIST_ENTRY Gbl_ListCmds;
PLX_LIST_ENTRY Gbl_ListVars;




/**********************************************************
 *
 * Function   :  CmdLine_Initialize
 *
 * Description:  Initializes lists
 *
 *********************************************************/
BOOLEAN
CmdLine_Initialize(
    VOID
    )
{
    // Initialize lists
    Plx_InitializeListHead( &Gbl_ListCmds );
    Plx_InitializeListHead( &Gbl_ListVars );

    return TRUE;
}




/**********************************************************
 *
 * Function   :  htol
 *
 * Description:  Converts a Hex string to an integer
 *
 *********************************************************/
PLX_UINT_PTR
htol(
    char *hexString
    )
{
    U8           count;
    PLX_UINT_PTR value;


    value = 0;

    for (count=0; count<strlen(hexString); count++)
    {
        value = value << 4;

        if ( (hexString[count] >= 'A') && (hexString[count] <= 'F') )
        {
            value = value + (hexString[count] - 'A' + 0xA);
        }
        else if ( (hexString[count] >= 'a') && (hexString[count] <= 'f') )
        {
            value = value + (hexString[count] - 'a' + 0xA);
        }
        else
        {
            value = value + (hexString[count] - '0');
        }
    }

    return value;
}




/**********************************************************
 *
 * Function   : CmdLine_IsHexValue
 *
 * Description:
 *
 *********************************************************/
BOOLEAN
CmdLine_IsHexValue(
    char *pStr
    )
{
    size_t count;


    // Get string length
    count = strlen(pStr);

    // Verify string length (2 chars/byte)
    if ((count == 0) || (count > (sizeof(U64) * 2)))
    {
        return FALSE;
    }

    // Check each character
    do
    {
        count--;
        if (!isxdigit( pStr[count] ))
        {
            return FALSE;
        }
    }
    while (count);

    return TRUE;
}




/**********************************************************
 *
 * Function   : CmdLine_GetNextToken
 *
 * Description:
 *
 *********************************************************/
void
CmdLine_GetNextToken(
    char    **pStr,
    char     *strToken,
    BOOLEAN   bAllowOps
    )
{
    // Remove leading whitespaces
    while (isspace( **pStr ))
    {
        (*pStr)++;
    }

    // Check for operations
    if (bAllowOps)
    {
        if ((**pStr == '+') || (**pStr == '-') ||
            (**pStr == '=') || (**pStr == '!'))
        {
            // Copy operation and return
            *strToken = **pStr;
            strToken[1] = '\0';
            (*pStr)++;
            return;
        }
    }

    // Get next token
    while (1)
    {
        // Check for terminating characters
        if ((**pStr == '\0') || isspace( **pStr ))
        {
            break;
        }

        // Check for operations if allowed
        if (bAllowOps && ((**pStr == '+')  || (**pStr == '-') ||
                          (**pStr == '=')  || (**pStr == '!')))
        {
            break;
        }

        // Copy character and increment pointers
        *strToken = **pStr;
        (*pStr)++;
        strToken++;
    }

    *strToken = '\0';
}




/**********************************************************
 *
 * Function   : CmdLine_VarGetByIndex
 *
 * Description: Returns the variable at the specified index
 *
 *********************************************************/
PLXCM_VAR*
CmdLine_VarGetByIndex(
    U8 index
    )
{
    U8              count;
    PLXCM_VAR      *pVar;
    PLX_LIST_ENTRY *pEntry;


    count = 0;

    pEntry = Gbl_ListVars.Flink;

    // Traverse list to find the desired list objects
    while (pEntry != &Gbl_ListVars)
    {
        // Return desired object if match
        if (count == index)
        {
            // Get the object
            pVar =
                PLX_CONTAINING_RECORD(
                    pEntry,
                    PLXCM_VAR,
                    ListEntry
                    );

            return pVar;
        }

        // Jump to next item
        count++;
        pEntry = pEntry->Flink;
    }

    // Object not found
    return NULL;
}




/**********************************************************
 *
 * Function   : CmdLine_VarLookup
 *
 * Description:
 *
 *********************************************************/
PLXCM_VAR*
CmdLine_VarLookup(
    char *pStr
    )
{
    PLXCM_VAR      *pVar;
    PLX_LIST_ENTRY *pEntry;


    pEntry = Gbl_ListVars.Flink;

    // Traverse list to find the desired list objects
    while (pEntry != &Gbl_ListVars)
    {
        // Get the object
        pVar =
            PLX_CONTAINING_RECORD(
                pEntry,
                PLXCM_VAR,
                ListEntry
                );

        if (Plx_strcasecmp( pVar->strName, pStr ) == 0)
        {
            return pVar;
        }

        // Jump to next item
        pEntry = pEntry->Flink;
    }

    // Object not found
    return NULL;
}




/**********************************************************
 *
 * Function   : CmdLine_VarAdd
 *
 * Description:
 *
 *********************************************************/
PLXCM_VAR*
CmdLine_VarAdd(
    char    *pStrVar,
    char    *pStrValue,
    BOOLEAN  bSystemVar
    )
{
    PLXCM_VAR *pVar;


    // Check if variable exists in table
    pVar = CmdLine_VarLookup( pStrVar );

    // Allocate new entry if not exist
    if (pVar == NULL)
    {
        pVar = malloc( sizeof(PLXCM_VAR) );

        // Add to list
        Plx_InsertTailList( &Gbl_ListVars, &pVar->ListEntry );
    }
    else
    {
        // Don't allow user to update system variables
        if ((pVar->bSystemVar == TRUE) && (bSystemVar == FALSE))
        {
            return NULL;
        }
    }

    // Update object
    pVar->bSystemVar = bSystemVar;
    strcpy( pVar->strName, pStrVar );
    strcpy( pVar->strValue, pStrValue );

    return pVar;
}




/**********************************************************
 *
 * Function   : CmdLine_VarDelete
 *
 * Description:
 *
 *********************************************************/
BOOLEAN
CmdLine_VarDelete(
    char    *pStrVar,
    BOOLEAN  bSystemVar
    )
{
    PLXCM_VAR *pVar;


    // Get variable from table
    pVar = CmdLine_VarLookup( pStrVar );

    if (pVar == NULL)
    {
        return FALSE;
    }

    // Don't allow user to delete system variables
    if ((pVar->bSystemVar == TRUE) && (bSystemVar == FALSE))
    {
        return FALSE;
    }

    // Remove from list
    Plx_RemoveEntryList( &pVar->ListEntry );

    // Release object
    free( pVar );

    return TRUE;
}




/**********************************************************
 *
 * Function   : CmdLine_VarDeleteAll
 *
 * Description:
 *
 *********************************************************/
VOID
CmdLine_VarDeleteAll(
    VOID
    )
{
    PLXCM_VAR      *pVar;
    PLX_LIST_ENTRY *pEntry;


    // Remove all list objects
    do
    {
        // Get and remove first list object
        pEntry = Plx_RemoveHeadList( &Gbl_ListVars );

        if (pEntry == &Gbl_ListVars)
        {
            return;
        }

        // Get the object
        pVar =
            PLX_CONTAINING_RECORD(
                pEntry,
                PLXCM_VAR,
                ListEntry
                );

        // Release object
        free( pVar );
    }
    while (1);
}




/**********************************************************
 *
 * Function   : CmdLine_CmdAdd
 *
 * Description:
 *
 *********************************************************/
PLXCM_COMMAND*
CmdLine_CmdAdd(
    char     *buffer,
    FN_TABLE *pFnTable
    )
{
    PLXCM_COMMAND *pCmd;


    // Remove leading empty characters
    while (isspace( *buffer ))
    {
        buffer++;
    }

    // Ignore empty commands
    if (*buffer == '\0')
    {
        return NULL;
    }

    // Ignore comment lines
    if ((buffer[0] == '#') || (strncmp( buffer, "//", 2 ) == 0))
    {
        return NULL;
    }

    // Get existing command if it exists
    pCmd = CmdLine_CmdExists( buffer );

    if (pCmd == NULL)
    {
        // Allocate new command
        pCmd = (PLXCM_COMMAND*)malloc( sizeof(PLXCM_COMMAND) );

        if (pCmd == NULL)
        {
            return NULL;
        }

        RtlZeroMemory( pCmd, sizeof(PLXCM_COMMAND) );

        // Copy entire command line
        strcpy( pCmd->szCmdLine, buffer );

        // Initialize argument list
        Plx_InitializeListHead( &pCmd->List_Args );
    }
    else
    {
        // Remove item from list
        Plx_RemoveEntryList( &pCmd->ListEntry );
    }

    // If command contained strings, force re-parse in case values have changed
    if (pCmd->bContainString)
    {
        pCmd->bParsed = FALSE;

        // Delete existing arguments
        CmdLine_ArgDeleteAll( pCmd );
    }

    // Parse command and arguments if not already
    if (pCmd->bParsed == FALSE)
    {
        CmdLine_CmdParse( pCmd, pFnTable );
    }

    // Add item to head of list
    Plx_InsertTailList( &Gbl_ListCmds, &pCmd->ListEntry );

    return pCmd;
}




/**********************************************************
 *
 * Function   : CmdLine_CmdDeleteAll
 *
 * Description:
 *
 *********************************************************/
VOID
CmdLine_CmdDeleteAll(
    VOID
    )
{
    PLXCM_COMMAND  *pCmd;
    PLXCM_COMMAND  *pCmdCurr;
    PLX_LIST_ENTRY *pEntry;


    // Remove all list objects
    do
    {
        // Get and remove first list object
        pEntry = Plx_RemoveHeadList( &Gbl_ListCmds );

        if (pEntry == &Gbl_ListCmds)
        {
            return;
        }

        // Get the object
        pCmd =
            PLX_CONTAINING_RECORD(
                pEntry,
                PLXCM_COMMAND,
                ListEntry
                );

        while (pCmd)
        {
            // Delete all arguments
            CmdLine_ArgDeleteAll( pCmd );

            // Copy current command
            pCmdCurr = pCmd;

            // Get next command
            pCmd = pCmd->pNextCmd;

            // Release current command
            free( pCmdCurr );
        }
    }
    while (1);
}




/**********************************************************
 *
 * Function   : CmdLine_CmdExists
 *
 * Description:
 *
 *********************************************************/
PLXCM_COMMAND*
CmdLine_CmdExists(
    char *buffer
    )
{
    PLXCM_COMMAND  *pCmd;
    PLX_LIST_ENTRY *pEntry;


    pEntry = Gbl_ListCmds.Flink;

    // Traverse list to find the desired list objects
    while (pEntry != &Gbl_ListCmds)
    {
        // Get the object
        pCmd =
            PLX_CONTAINING_RECORD(
                pEntry,
                PLXCM_COMMAND,
                ListEntry
                );

        if (strcmp( pCmd->szCmdLine, buffer ) == 0)
        {
            return pCmd;
        }

        // Jump to next item
        pEntry = pEntry->Flink;
    }

    // Object not found
    return NULL;
}




/**********************************************************
 *
 * Function   : CmdLine_CmdParse
 *
 * Description:
 *
 *********************************************************/
BOOLEAN
CmdLine_CmdParse(
    PLXCM_COMMAND *pCmd,
    FN_TABLE      *pFnTable
    )
{
    char         *pBuffer;
    char          strArg[MAX_ARG_LEN];
    BOOLEAN       bOpOk;
    PLXCM_ARG    *pArg;
    PLXCM_ARG    *pOperand;
    PLXCM_VAR    *pVar;
    PLXCM_ARG_OP  CurrOp;


    // Get the command
    pBuffer = pCmd->szCmdLine;
    CmdLine_GetNextToken( &pBuffer, pCmd->szCmd, FALSE );

    // Lookup the command & set function pointer
    if (pCmd->pCmdRoutine == NULL)
    {
        CmdLine_CmdLookup( pCmd, pFnTable );
        if (pCmd->pCmdRoutine == NULL)
        {
            return pCmd->bErrorParse;
        }
    }

    // Start with no operations
    bOpOk    = FALSE;
    CurrOp   = PLXCM_ARG_OP_NONE;
    pOperand = NULL;

    // Assume no errors
    pCmd->bErrorParse = FALSE;

    // Parse arguments
    do
    {
        // Get next argument
        CmdLine_GetNextToken( &pBuffer, strArg, pCmd->bAllowOps );

        // Reset variable from table
        pVar = NULL;

        // Add argument to list
        if (strArg[0] != '\0')
        {
            // Default to not add argument
            pArg = NULL;

            // If variables allowed, check for it & convert to hex string
            if (pCmd->bAllowOps)
            {
                pVar = CmdLine_VarLookup( strArg );
                if (pVar != NULL)
                {
                    strcpy( strArg, pVar->strValue );

                    // Flag string exists in command
                    pCmd->bContainString = TRUE;
                }
            }

            if (pCmd->bAllowOps && ((strArg[0] == '+') || (strArg[0] == '-')))   // Math operation
            {
                // Check if operation allowed
                if (bOpOk == FALSE)
                {
                    pCmd->bErrorParse = TRUE;
                    Cons_printf("Error: Unexpected operation (%c)\n", strArg[0]);
                    return FALSE;
                }

                // Determine operation
                switch (strArg[0])
                {
                    case '+':
                        CurrOp = PLXCM_ARG_OP_ADD;
                        break;

                    case '-':
                        CurrOp = PLXCM_ARG_OP_SUB;
                        break;

                    default:
                        pCmd->bErrorParse = TRUE;
                        Cons_printf("Error: Invalid operation (%c)\n", strArg[0]);
                        return FALSE;
                }

                // Next token cannot be operation
                bOpOk = FALSE;
            }
            else
            {
                // Allocate argument for list
                pArg = malloc( sizeof(PLXCM_ARG) );

                // Clear argument
                memset( pArg, 0, sizeof(PLXCM_ARG) );

                // Store string
                strcpy(pArg->ArgString, strArg);

                // If argument was variable store it
                if (pVar != NULL)
                {
                    pArg->pVar = pVar;
                }

                // If value is a number, convert it
                if (CmdLine_IsHexValue( strArg ))
                {
                    pArg->ArgType   = PLXCM_ARG_TYPE_INT;
                    pArg->ArgIntDec = atol( strArg );
                    pArg->ArgIntHex = htol( strArg );

                    // If operation pending, perform it
                    if (CurrOp != PLXCM_ARG_OP_NONE)
                    {
                        // Verify previous command
                        if (pOperand == NULL)
                        {
                            pCmd->bErrorParse = TRUE;
                            free( pArg );
                            Cons_printf("Error: Operand missing\n");
                            return FALSE;
                        }

                        // Perform operation on previous operand
                        switch (CurrOp)
                        {
                            case PLXCM_ARG_OP_ADD:
                                pOperand->ArgIntDec += pArg->ArgIntDec;
                                pOperand->ArgIntHex += pArg->ArgIntHex;
                                sprintf(pOperand->ArgString, "%08lX", (PLX_UINT_PTR)pOperand->ArgIntHex);
                                break;

                            case PLXCM_ARG_OP_SUB:
                                pOperand->ArgIntDec -= pArg->ArgIntDec;
                                pOperand->ArgIntHex -= pArg->ArgIntHex;
                                sprintf(pOperand->ArgString, "%08lX", (PLX_UINT_PTR)pOperand->ArgIntHex);
                                break;

                            default:
                                pCmd->bErrorParse = TRUE;
                                Cons_printf("Error: Operation not implemented\n");
                                return FALSE;
                        }

                        // Reset operation
                        CurrOp = PLXCM_ARG_OP_NONE;
                        free( pArg );
                        pArg = NULL;
                    }
                    else
                    {
                        // Store current operand in case of operation
                        pOperand = pArg;
                    }

                    // Next token can be operation
                    bOpOk = TRUE;
                }
                else
                {
                    pArg->ArgType = PLXCM_ARG_TYPE_STRING;

                    pCmd->bContainString = TRUE;

                    // If operation pending, return error
                    if (CurrOp != PLXCM_ARG_OP_NONE)
                    {
                        pCmd->bErrorParse = TRUE;
                        free( pArg );
                        Cons_printf("Error: Operand missing\n");
                        return FALSE;
                    }

                    // Next token cannot be operation
                    bOpOk = FALSE;
                }
            }

            // Add argument if ready
            if (pArg != NULL)
            {
                pCmd->NumArgs++;

                // Add to end of argument list
                Plx_InsertTailList(
                    &pCmd->List_Args,
                    &pArg->ListEntry
                    );

                // Reset argument
                pArg = NULL;
            }
        }
    }
    while (strArg[0] != '\0');

    // Verify trailing operand is not missing
    if (CurrOp != PLXCM_ARG_OP_NONE)
    {
        pCmd->bErrorParse = TRUE;
        Cons_printf("Error: Missing operand\n");
        return FALSE;
    }

    // Mark command as parsed
    pCmd->bParsed = TRUE;

    return TRUE;
}




/**********************************************************
 *
 * Function   : CmdLine_CmdLookup
 *
 * Description: Locate a command in the function table
 *
 *********************************************************/
BOOLEAN
CmdLine_CmdLookup(
    PLXCM_COMMAND *pCmd,
    FN_TABLE      *pFnTable
    )
{
    U16   i;
    char *Token;
    char  strCmd[MAX_CMD_LEN];


    if (pFnTable == NULL)
    {
        return FALSE;
    }

    // Lookup command in function table to assign pointer
    i = 0;
    while (pFnTable[i].CmdId != CMD_FINAL)
    {
        // Make a copy of the command
        strcpy( strCmd, pFnTable[i].strCmd );

        // Get next command
        Token = strtok( strCmd, "/" );
        while (Token != NULL)
        {
            // Check for a match
            if (Plx_strcasecmp( Token, pCmd->szCmd ) == 0)
            {
                pCmd->pCmdRoutine = pFnTable[i].pCmdRoutine;
                pCmd->bAllowOps   = pFnTable[i].bAllowOps;
                return TRUE;
            }

            // Get next alias
            Token = strtok( NULL, "/" );
        }

        // Jump to next command
        i++;
    }

    pCmd->pCmdRoutine = NULL;
    pCmd->bErrorParse = TRUE;
    return TRUE;
}




/**********************************************************
 *
 * Function   : CmdLine_GetArg
 *
 * Description: Returns the specified argument
 *
 *********************************************************/
PLXCM_ARG*
CmdLine_ArgGet(
    PLXCM_COMMAND *pCmd,
    U8             ArgNum
    )
{
    U8              count;
    PLXCM_ARG      *pArg;
    PLX_LIST_ENTRY *pEntry;


    // Verify argument exists
    if ((pCmd->NumArgs == 0) || (ArgNum >= pCmd->NumArgs))
    {
        return NULL;
    }

    count  = 0;
    pEntry = pCmd->List_Args.Flink;

    do
    {
        if (count == ArgNum)
        {
            pArg =
                PLX_CONTAINING_RECORD(
                    pEntry,
                    PLXCM_ARG,
                    ListEntry
                    );

            return pArg;
        }

        count++;
        pEntry = pEntry->Flink;
    }
    while (count <= ArgNum);

    // Argument not found
    return NULL;
}




/**********************************************************
 *
 * Function   : CmdLine_ArgDeleteAll
 *
 * Description: Deletes all arguments of a command
 *
 *********************************************************/
VOID
CmdLine_ArgDeleteAll(
    PLXCM_COMMAND *pCmd
    )
{
    PLXCM_ARG      *pArg;
    PLX_LIST_ENTRY *pEntry;


    // Remove all list objects
    do
    {
        // Get and remove first list object
        pEntry = Plx_RemoveHeadList( &pCmd->List_Args );

        // Return if no more items
        if (pEntry == &pCmd->List_Args)
        {
            return;
        }

        // Get the object
        pArg =
            PLX_CONTAINING_RECORD(
                pEntry,
                PLXCM_ARG,
                ListEntry
                );

        // Decrement argument count
        pCmd->NumArgs--;

        // Release object
        free( pArg );
    }
    while (1);
}
