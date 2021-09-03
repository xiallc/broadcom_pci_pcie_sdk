#ifndef __PLX_NET_TYPES_H
#define __PLX_NET_TYPES_H

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
 *      PlxNetTypes.h
 *
 * Description:
 *
 *      This file includes definitions for PLX Networking over PCIe
 *
 * Revision:
 *
 *      05-01-11 : PLX SDK v6.50
 *
 ******************************************************************************/


#include "PlxTypes.h"



#ifdef __cplusplus
extern "C" {
#endif




/******************************************
 *           Definitions
 ******************************************/

// Notification event types
typedef enum _PLX_ND_NOTIFY_TYPE
{
    PLX_ND_NOTIFY_ANY           = 0,
    PLX_ND_NOTIFY_ERRORS        = 1,
    PLX_ND_NOTIFY_SOLICITED     = 2
} PLX_ND_NOTIFY_TYPE;

// Notification event types
typedef enum _PLX_CQ_USE_TYPE
{
    PLX_CQ_USE_NONE             = 0,
    PLX_CQ_USE_RX               = (1 << 0),
    PLX_CQ_USE_TX               = (1 << 1)
} PLX_CQ_USE_TYPE;



/****************************************************************************************************
 * PLX message buffer structure shared by apps & driver
 *
 *     ______________________4_____________________8_____________________C___________________
 *  0 | Off_DataStart       | Off_DataEnd         | Off_DataCurr        | Off_DataNext       |
 * 10 | Off_RxDataStart     | Off_RxDataEnd       | Off_RxDataCurr      | Off_RxDataNext     |
 * 20 | bRxCqArmed          | bTxCqArmed          | Reserved_1[0]       | Reserved_1[1]      |
 *    |---------------------|---------------------|---------------------|--------------------|
 * 30 | Off_PeerDataStart   | Off_PeerDataEnd     | Off_PeerDataCurr    | Off_PeerDataNext   |
 * 40 | Off_PeerRxDataStart | Off_PeerRxDataEnd   | Off_PeerRxDataCurr  | Off_PeerRxDataNext |
 * 50 | ConnectState        | Reserved_2[0]       | Reserved_2[1]       | Reserved_2[2]      |
 *     --------------------------------------------------------------------------------------
 ***************************************************************************************************/
typedef struct _PLX_MSG_BUFFER
{
    VU32 Offset_DataStart;
    VU32 Offset_DataEnd;
    VU32 Offset_DataCurr;
    VU32 Offset_DataNext;
    VU32 Offset_RxReqStart;
    VU32 Offset_RxReqEnd;
    VU32 Offset_RxReqCurr;
    VU32 Offset_RxReqNext;
    VU32 bRxCqArmed;
    VU32 bTxCqArmed;
    VU32 Reserved_1[2];
    VU32 Offset_PeerDataStart;
    VU32 Offset_PeerDataEnd;
    VU32 Offset_PeerDataCurr;
    VU32 Offset_PeerDataNext;
    VU32 Offset_PeerRxReqStart;
    VU32 Offset_PeerRxReqEnd;
    VU32 Offset_PeerRxReqCurr;
    VU32 Offset_PeerRxReqNext;
    VU32 ConnectState;
    VU32 Reserved_2[3];
} PLX_MSG_BUFFER;




#ifdef __cplusplus
}
#endif

#endif
