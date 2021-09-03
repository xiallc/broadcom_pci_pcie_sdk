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
 *      PlxLists.c
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


#include "PlxLists.h"




/******************************************************************************
 *
 * Function   :  
 *
 * Description:  
 *
 ******************************************************************************/
VOID
Plx_InitializeListHead(
    PLX_LIST_ENTRY *ListHead
    )
{
    ListHead->Flink = ListHead->Blink = ListHead;
}




/******************************************************************************
 *
 * Function   :  
 *
 * Description:  
 *
 ******************************************************************************/
BOOLEAN
Plx_IsListEmpty(
    const PLX_LIST_ENTRY *ListHead
    )
{
    return (BOOLEAN)(ListHead->Flink == ListHead);
}




/******************************************************************************
 *
 * Function   :  
 *
 * Description:  
 *
 ******************************************************************************/
BOOLEAN
Plx_RemoveEntryList(
    PLX_LIST_ENTRY *Entry
    )
{
    PLX_LIST_ENTRY *Blink;
    PLX_LIST_ENTRY *Flink;


    Flink        = Entry->Flink;
    Blink        = Entry->Blink;
    Blink->Flink = Flink;
    Flink->Blink = Blink;

    return (BOOLEAN)(Flink == Blink);
}




/******************************************************************************
 *
 * Function   :  
 *
 * Description:  
 *
 ******************************************************************************/
PLX_LIST_ENTRY*
Plx_RemoveHeadList(
    PLX_LIST_ENTRY *ListHead
    )
{
    PLX_LIST_ENTRY *Flink;
    PLX_LIST_ENTRY *Entry;


    Entry           = ListHead->Flink;
    Flink           = Entry->Flink;
    ListHead->Flink = Flink;
    Flink->Blink    = ListHead;

    return Entry;
}




/******************************************************************************
 *
 * Function   :  
 *
 * Description:  
 *
 ******************************************************************************/
PLX_LIST_ENTRY*
Plx_RemoveTailList(
    PLX_LIST_ENTRY *ListHead
    )
{
    PLX_LIST_ENTRY *Blink;
    PLX_LIST_ENTRY *Entry;


    Entry           = ListHead->Blink;
    Blink           = Entry->Blink;
    ListHead->Blink = Blink;
    Blink->Flink    = ListHead;

    return Entry;
}




/******************************************************************************
 *
 * Function   :  
 *
 * Description:  
 *
 ******************************************************************************/
VOID
Plx_InsertTailList(
    PLX_LIST_ENTRY *ListHead,
    PLX_LIST_ENTRY *Entry
    )
{
    PLX_LIST_ENTRY *Blink;


    Blink           = ListHead->Blink;
    Entry->Flink    = ListHead;
    Entry->Blink    = Blink;
    Blink->Flink    = Entry;
    ListHead->Blink = Entry;
}




/******************************************************************************
 *
 * Function   :  
 *
 * Description:  
 *
 ******************************************************************************/
VOID
Plx_InsertHeadList(
    PLX_LIST_ENTRY *ListHead,
    PLX_LIST_ENTRY *Entry
    )
{
    PLX_LIST_ENTRY *Flink;


    Flink           = ListHead->Flink;
    Entry->Flink    = Flink;
    Entry->Blink    = ListHead;
    Flink->Blink    = Entry;
    ListHead->Flink = Entry;
}
