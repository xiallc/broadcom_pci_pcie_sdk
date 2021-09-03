#ifndef __DRIVER_H
#define __DRIVER_H

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
 *      Driver.h
 *
 * Description:
 *
 *      The header file for the PLX Chip Windows Device driver
 *
 * Revision History:
 *
 *      07-01-12 : PLX SDK v7.00
 *
 ******************************************************************************/


#include <linux/pci.h>
#include "DrvDefs.h"




/**********************************************
*               Functions
**********************************************/
int
Plx_init_module(
    void
    );

void
Plx_cleanup_module(
    void
    );

int
PlxDispatch_Probe(
    struct pci_dev             *pDev,
    const struct pci_device_id *pID
    );

void
PlxDispatch_Remove(
    struct pci_dev *pDev
    );

int
AddDevice(
    DRIVER_OBJECT  *pDriverObject,
    struct pci_dev *pPciDev
    );

int
RemoveDevice(
    DEVICE_OBJECT *fdo
    );

int
StartDevice(
    DEVICE_OBJECT *fdo
    );

VOID
StopDevice(
    DEVICE_OBJECT *fdo
    );




#endif
