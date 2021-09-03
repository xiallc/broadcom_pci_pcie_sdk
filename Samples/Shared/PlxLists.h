#ifndef __PLX_LISTS_H
#define __PLX_LISTS_H

/*******************************************************************************
 * Copyright (c) PLX Technology, Inc.
 *
 * PLX Technology Inc. licenses this source file under the GNU Lesser General Public
 * License (LGPL) version 2.  This source file may be modified or redistributed
 * under the terms of the LGPL and without express permission from PLX Technology.
 *
 * PLX Technology, Inc. provides this software AS IS, WITHOUT ANY WARRANTY,
 * EXPRESS OR IMPLIED, INCLUDING, WITHOUT LIMITATION, ANY WARRANTY OF
 * MERCHANTABILITY OR FITNESS FOR A PARTICULAR PURPOSE.  PLX makes no guarantee
 * or representations regarding the use of, or the results of the use of,
 * the software and documentation in terms of correctness, accuracy,
 * reliability, currentness, or otherwise; and you rely on the software,
 * documentation and results solely at your own risk.
 *
 * IN NO EVENT SHALL PLX BE LIABLE FOR ANY LOSS OF USE, LOSS OF BUSINESS,
 * LOSS OF PROFITS, INDIRECT, INCIDENTAL, SPECIAL OR CONSEQUENTIAL DAMAGES
 * OF ANY KIND.
 *
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
