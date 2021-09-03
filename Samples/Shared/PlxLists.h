#ifndef __PLX_LISTS_H
#define __PLX_LISTS_H

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
 *      PlxLists.h
 *
 * Description:
 *
 *      Generic double-linked list support
 *
 * Revision History:
 *
 *      02-01-10 : PLX SDK v6.40
 *
 ******************************************************************************/


#include "PlxTypes.h"


#ifdef __cplusplus
extern "C" {
#endif




/**********************************************
 *               Definitions
 *********************************************/

// Define list functions
// Declare and initialize a list
#define PLX_LIST_HEAD_INIT(name)                            { &(name), &(name) }
#define PLX_LIST_HEAD(name)                                 struct _PLX_LIST_ENTRY = PLX_LIST_HEAD_INIT(name)

// Calculate the address of the base of the structure given its type, and an
// address of a field within the structure.
#define PLX_CONTAINING_RECORD(address, type, field)         ((type *)( \
                                                             (char*)(address) - \
                                                             (PLX_UINT_PTR)(&((type *)0)->field)))

// Locked list operations
#define Plx_ExInterlockedInsertTailList(List,Entry,Lock)    Plx_InsertTailList( (List), (Entry) )
#define Plx_ExInterlockedInsertHeadList(List,Entry,Lock)    Plx_InsertHeadList( (List), (Entry) )


//  Doubly linked list structure
typedef struct _PLX_LIST_ENTRY
{
    struct _PLX_LIST_ENTRY *Flink;
    struct _PLX_LIST_ENTRY *Blink;
} PLX_LIST_ENTRY;


#if defined(PLX_DOS)
    #define CONTAINING_RECORD                   PLX_CONTAINING_RECORD
    #define ExInterlockedInsertTailList         Plx_ExInterlockedInsertTailList
    #define ExInterlockedInsertHeadList         Plx_ExInterlockedInsertHeadList
    #define InitializeListHead                  Plx_InitializeListHead
    #define IsListEmpty                         Plx_IsListEmpty
    #define RemoveHeadList                      Plx_RemoveHeadList
    #define RemoveEntryList                     Plx_RemoveEntryList
    #define InsertTailList                      Plx_InsertTailList
    #define InsertHeadList                      Plx_InsertHeadList
    typedef PLX_LIST_ENTRY                      LIST_ENTRY;
#endif




/**********************************************
 *               Functions
 *********************************************/
VOID
Plx_InitializeListHead(
    PLX_LIST_ENTRY *ListHead
    );

BOOLEAN
Plx_IsListEmpty(
    const PLX_LIST_ENTRY * ListHead
    );

BOOLEAN
Plx_RemoveEntryList(
    PLX_LIST_ENTRY *Entry
    );

PLX_LIST_ENTRY*
Plx_RemoveHeadList(
    PLX_LIST_ENTRY *ListHead
    );

PLX_LIST_ENTRY*
Plx_RemoveTailList(
    PLX_LIST_ENTRY *ListHead
    );

VOID
Plx_InsertTailList(
    PLX_LIST_ENTRY *ListHead,
    PLX_LIST_ENTRY *Entry
    );

VOID
Plx_InsertHeadList(
    PLX_LIST_ENTRY *ListHead,
    PLX_LIST_ENTRY *Entry
    );



#ifdef __cplusplus
}
#endif

#endif
