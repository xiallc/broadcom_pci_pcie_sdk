#ifndef __PLX_NET_TYPES_H
#define __PLX_NET_TYPES_H

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
