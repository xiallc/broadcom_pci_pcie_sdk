#ifndef __PLX_NET_IOCTL_H
#define __PLX_NET_IOCTL_H

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
 *      PlxNetIoctl.h
 *
 * Description:
 *
 *      Header PLX Network-specific IOCTL messages
 *
 * Revision History:
 *
 *      03-01-11 : PLX Network over NT support
 *
 ******************************************************************************/


#include "PlxIoctl.h"


#ifdef __cplusplus
extern "C" {
#endif




/**********************************************
 *               Definitions
 *********************************************/
// PLX Network message codes
typedef enum _PLX_NET_MSGS
{
    MSG_NET_SOCK_ADDR_GET          = PLX_IOCTL_CODE_BASE + 0x200,
    MSG_NET_SOCK_ADDR_BIND,
    MSG_NET_LISTEN_REGISTER,
    MSG_NET_CONNECTOR_CREATE,
    MSG_NET_CONNECTOR_RELEASE,
    MSG_NET_CONNECTOR_CANCEL_IO,
    MSG_NET_CONNECTOR_PROP,
    MSG_NET_CONNECT_REQ_GET,
    MSG_NET_CONNECT_REQ_SEND,
    MSG_NET_CONNECT_REGISTER_NOTIFY,
    MSG_NET_CONNECT_ACCEPT,
    MSG_NET_CONNECT_REJECT,
    MSG_NET_CONNECT_COMPLETE,
    MSG_NET_CONNECT_GET_BUFFER_PROP,
    MSG_NET_DISCONNECT,
    MSG_NET_PEER_GET_ADDRESS,
    MSG_NET_CQ_CREATE,
    MSG_NET_CQ_ASSIGN_TO_CONNECTOR,
    MSG_NET_CQ_NOTIFY_REGISTER,
    MSG_NET_CQ_NOTIFY_TX_SIGNAL_EVENT
} PLX_NET_MSGS;


#define PLX_IOCTL_NET_SOCK_ADDR_GET             IOCTL_MSG( MSG_NET_SOCK_ADDR_GET )
#define PLX_IOCTL_NET_SOCK_ADDR_BIND            IOCTL_MSG( MSG_NET_SOCK_ADDR_BIND )
#define PLX_IOCTL_NET_LISTEN_REGISTER           IOCTL_MSG( MSG_NET_LISTEN_REGISTER  )
#define PLX_IOCTL_NET_CONNECTOR_CREATE          IOCTL_MSG( MSG_NET_CONNECTOR_CREATE )
#define PLX_IOCTL_NET_CONNECTOR_RELEASE         IOCTL_MSG( MSG_NET_CONNECTOR_RELEASE )
#define PLX_IOCTL_NET_CONNECTOR_CANCEL_IO       IOCTL_MSG( MSG_NET_CONNECTOR_CANCEL_IO )
#define PLX_IOCTL_NET_CONNECTOR_PROP            IOCTL_MSG( MSG_NET_CONNECTOR_PROP )
#define PLX_IOCTL_NET_CONNECT_REQ_GET           IOCTL_MSG( MSG_NET_CONNECT_REQ_GET )
#define PLX_IOCTL_NET_CONNECT_REQ_SEND          IOCTL_MSG( MSG_NET_CONNECT_REQ_SEND )
#define PLX_IOCTL_NET_CONNECT_REGISTER_NOTIFY   IOCTL_MSG( MSG_NET_CONNECT_REGISTER_NOTIFY )
#define PLX_IOCTL_NET_CONNECT_ACCEPT            IOCTL_MSG( MSG_NET_CONNECT_ACCEPT )
#define PLX_IOCTL_NET_CONNECT_REJECT            IOCTL_MSG( MSG_NET_CONNECT_REJECT )
#define PLX_IOCTL_NET_CONNECT_COMPLETE          IOCTL_MSG( MSG_NET_CONNECT_COMPLETE )
#define PLX_IOCTL_NET_CONNECT_GET_BUFFER_PROP   IOCTL_MSG( MSG_NET_CONNECT_GET_BUFFER_PROP )
#define PLX_IOCTL_NET_DISCONNECT                IOCTL_MSG( MSG_NET_DISCONNECT )
#define PLX_IOCTL_NET_PEER_GET_ADDRESS          IOCTL_MSG( MSG_NET_PEER_GET_ADDRESS )
#define PLX_IOCTL_NET_CQ_CREATE                 IOCTL_MSG( MSG_NET_CQ_CREATE )
#define PLX_IOCTL_NET_CQ_ASSIGN_TO_CONNECTOR    IOCTL_MSG( MSG_NET_CQ_ASSIGN_TO_CONNECTOR )
#define PLX_IOCTL_NET_CQ_NOTIFY_REGISTER        IOCTL_MSG( MSG_NET_CQ_NOTIFY_REGISTER )
#define PLX_IOCTL_NET_CQ_NOTIFY_TX_SIGNAL_EVENT IOCTL_MSG( MSG_NET_CQ_NOTIFY_TX_SIGNAL_EVENT )



#ifdef __cplusplus
}
#endif

#endif
