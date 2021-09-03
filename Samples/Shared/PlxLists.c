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
