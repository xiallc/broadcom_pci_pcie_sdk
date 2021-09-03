#ifndef __PLX_STATUS_H
#define __PLX_STATUS_H

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
 *      PlxStat.h
 *
 * Description:
 *
 *      This file defines all the status codes for PLX SDK
 *
 * Revision:
 *
 *      06-01-11 : PLX SDK v6.50
 *
 ******************************************************************************/


#ifdef __cplusplus
extern "C" {
#endif




/******************************************
*             Definitions
******************************************/
#define PLX_STATUS_START               0x200   // Starting status code


// API Return Code Values
typedef enum _PLX_STATUS
{
    ApiSuccess = PLX_STATUS_START,
    ApiFailed,
    ApiNullParam,
    ApiUnsupportedFunction,
    ApiNoActiveDriver,
    ApiConfigAccessFailed,
    ApiInvalidDeviceInfo,
    ApiInvalidDriverVersion,
    ApiInvalidOffset,
    ApiInvalidData,
    ApiInvalidSize,
    ApiInvalidAddress,
    ApiInvalidAccessType,
    ApiInvalidIndex,
    ApiInvalidPowerState,
    ApiInvalidIopSpace,
    ApiInvalidHandle,
    ApiInvalidPciSpace,
    ApiInvalidBusIndex,
    ApiInsufficientResources,
    ApiWaitTimeout,
    ApiWaitCanceled,
    ApiDmaChannelUnavailable,
    ApiDmaChannelInvalid,
    ApiDmaDone,
    ApiDmaPaused,
    ApiDmaInProgress,
    ApiDmaCommandInvalid,
    ApiDmaInvalidChannelPriority,
    ApiDmaSglPagesGetError,
    ApiDmaSglPagesLockError,
    ApiMuFifoEmpty,
    ApiMuFifoFull,
    ApiPowerDown,
    ApiHSNotSupported,
    ApiVPDNotSupported,
    ApiDeviceInUse,
    ApiDeviceDisabled,
    ApiPending,
    ApiObjectNotFound,
    ApiInvalidState,
    ApiBufferTooSmall,
    ApiLastError               // Do not add API errors below this line
} PLX_STATUS;



#ifdef __cplusplus
}
#endif

#endif
