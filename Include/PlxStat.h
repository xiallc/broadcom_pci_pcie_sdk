#ifndef __PLX_STATUS_H
#define __PLX_STATUS_H

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
 *      PlxStat.h
 *
 * Description:
 *
 *      This file defines all the status codes for PLX SDK
 *
 * Revision:
 *
 *      08-01-14 : PLX SDK v7.20
 *
 ******************************************************************************/


#ifdef __cplusplus
extern "C" {
#endif




/******************************************
 *             Definitions
 *****************************************/
#define PLX_STATUS_START               0x200   // Starting status code

// Return type
typedef int    PLX_STATUS;


// API Return Code Values
typedef enum _PLX_STATUS_CODE
{
    PLX_STATUS_OK               = PLX_STATUS_START,
    PLX_STATUS_FAILED,
    PLX_STATUS_NULL_PARAM,
    PLX_STATUS_UNSUPPORTED,
    PLX_STATUS_NO_DRIVER,
    PLX_STATUS_INVALID_OBJECT,
    PLX_STATUS_VER_MISMATCH,
    PLX_STATUS_INVALID_OFFSET,
    PLX_STATUS_INVALID_DATA,
    PLX_STATUS_INVALID_SIZE,
    PLX_STATUS_INVALID_ADDR,
    PLX_STATUS_INVALID_ACCESS,
    PLX_STATUS_INSUFFICIENT_RES,
    PLX_STATUS_TIMEOUT,
    PLX_STATUS_CANCELED,
    PLX_STATUS_COMPLETE,
    PLX_STATUS_PAUSED,
    PLX_STATUS_IN_PROGRESS,
    PLX_STATUS_PAGE_GET_ERROR,
    PLX_STATUS_PAGE_LOCK_ERROR,
    PLX_STATUS_LOW_POWER,
    PLX_STATUS_IN_USE,
    PLX_STATUS_DISABLED,
    PLX_STATUS_PENDING,
    PLX_STATUS_NOT_FOUND,
    PLX_STATUS_INVALID_STATE,
    PLX_STATUS_BUFF_TOO_SMALL,
    PLX_STATUS_RSVD_LAST_ERROR    // Do not add API errors below this line
} PLX_STATUS_CODE;



// Definitions to support existing applications. Will be removed in future.
#if 1
#define ApiSuccess                      PLX_STATUS_OK
#define ApiFailed                       PLX_STATUS_FAILED
#define ApiNullParam                    PLX_STATUS_NULL_PARAM
#define ApiUnsupportedFunction          PLX_STATUS_UNSUPPORTED
#define ApiNoActiveDriver               PLX_STATUS_NO_DRIVER
#define ApiConfigAccessFailed           PLX_STATUS_FAILED
#define ApiInvalidDeviceInfo            PLX_STATUS_INVALID_OBJECT
#define ApiInvalidDriverVersion         PLX_STATUS_VER_MISMATCH
#define ApiInvalidPciSpace              PLX_STATUS_INVALID_ADDR
#define ApiInvalidOffset                PLX_STATUS_INVALID_OFFSET
#define ApiInvalidData                  PLX_STATUS_INVALID_DATA
#define ApiInvalidSize                  PLX_STATUS_INVALID_SIZE
#define ApiInvalidAddress               PLX_STATUS_INVALID_ADDR
#define ApiInvalidAccessType            PLX_STATUS_INVALID_ACCESS
#define ApiInvalidIndex                 PLX_STATUS_INVALID_DATA
#define ApiInvalidHandle                PLX_STATUS_INVALID_ACCESS
#define ApiInvalidPowerState            PLX_STATUS_INVALID_STATE
#define ApiInvalidIopSpace              PLX_STATUS_INVALID_ACCESS
#define ApiInvalidBusIndex              PLX_STATUS_INVALID_DATA
#define ApiInsufficientResources        PLX_STATUS_INSUFFICIENT_RES
#define ApiWaitTimeout                  PLX_STATUS_TIMEOUT
#define ApiWaitCanceled                 PLX_STATUS_CANCELED
#define ApiDmaDone                      PLX_STATUS_COMPLETE
#define ApiDmaPaused                    PLX_STATUS_PAUSED
#define ApiDmaChannelInvalid            PLX_STATUS_INVALID_ADDR
#define ApiDmaInProgress                PLX_STATUS_IN_PROGRESS
#define ApiDmaSglPagesGetError          PLX_STATUS_PAGE_GET_ERROR
#define ApiDmaSglPagesLockError         PLX_STATUS_PAGE_LOCK_ERROR
#define ApiDmaChannelUnavailable        PLX_STATUS_INVALID_ACCESS
#define ApiDmaCommandInvalid            PLX_STATUS_INVALID_DATA
#define ApiDmaInvalidChannelPriority    PLX_STATUS_INVALID_DATA
#define ApiPowerDown                    PLX_STATUS_LOW_POWER
#define ApiDeviceInUse                  PLX_STATUS_IN_USE
#define ApiDeviceDisabled               PLX_STATUS_DISABLED
#define ApiPending                      PLX_STATUS_PENDING
#define ApiObjectNotFound               PLX_STATUS_NOT_FOUND
#define ApiInvalidState                 PLX_STATUS_INVALID_STATE
#define ApiBufferTooSmall               PLX_STATUS_BUFF_TOO_SMALL
#define ApiMuFifoEmpty                  PLX_STATUS_NOT_FOUND
#define ApiMuFifoFull                   PLX_STATUS_INSUFFICIENT_RES
#define ApiHSNotSupported               PLX_STATUS_UNSUPPORTED
#define ApiVPDNotSupported              PLX_STATUS_UNSUPPORTED
#define ApiLastError                    PLX_STATUS_RSVD_LAST_ERROR
#endif



#ifdef __cplusplus
}
#endif

#endif
