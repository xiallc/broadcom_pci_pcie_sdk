#ifndef __PLXCHIP_H
#define __PLXCHIP_H

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
 *      PlxChip.h
 *
 * Description:
 *
 *      This file contains definitions specific to a PLX Chip
 *
 * Revision History:
 *
 *      07-01-12 : PLX SDK v7.00
 *
 ******************************************************************************/


#if (PLX_CHIP == 9050)
    #include "Reg9050.h"
#elif (PLX_CHIP == 9030)
    #include "Reg9030.h"
#elif (PLX_CHIP == 9080)
    #include "Reg9080.h"
#elif (PLX_CHIP == 9054)
    #include "Reg9054.h"
#elif (PLX_CHIP == 9056)
    #include "Reg9056.h"
#elif (PLX_CHIP == 9656)
    #include "Reg9656.h"
#elif (PLX_CHIP == 8311)
    #include "Reg8311.h"
#endif




/**********************************************
 *               Definitions
 *********************************************/
#if (PLX_CHIP == 9050)

    #define PLX_CHIP_TYPE                       0x9050
    #define PLX_DRIVER_NAME                     "Plx9050"
    #define PLX_DRIVER_NAME_UNICODE             L"Plx9050"
    #define MAX_PLX_REG_OFFSET                  PCI9050_MAX_REG_OFFSET   // Max PLX register offset
    #define DEFAULT_SIZE_COMMON_BUFFER          0                        // Default size of Common Buffer

#elif (PLX_CHIP == 9030)

    #define PLX_CHIP_TYPE                       0x9030
    #define PLX_DRIVER_NAME                     "Plx9030"
    #define PLX_DRIVER_NAME_UNICODE             L"Plx9030"
    #define MAX_PLX_REG_OFFSET                  PCI9030_MAX_REG_OFFSET   // Max PLX register offset
    #define DEFAULT_SIZE_COMMON_BUFFER          0                        // Default size of Common Buffer

#elif (PLX_CHIP == 9080)

    #define PLX_CHIP_TYPE                       0x9080
    #define PLX_DRIVER_NAME                     "Plx9080"
    #define PLX_DRIVER_NAME_UNICODE             L"Plx9080"
    #define MAX_PLX_REG_OFFSET                  PCI9080_MAX_REG_OFFSET   // Max PLX register offset
    #define DEFAULT_SIZE_COMMON_BUFFER          (64 * 1024)              // Default size of Common Buffer
    #define NUM_DMA_CHANNELS                    2                        // Total number of DMA Channels

#elif (PLX_CHIP == 9054)

    #define PLX_CHIP_TYPE                       0x9054
    #define PLX_DRIVER_NAME                     "Plx9054"
    #define PLX_DRIVER_NAME_UNICODE             L"Plx9054"
    #define MAX_PLX_REG_OFFSET                  PCI9054_MAX_REG_OFFSET   // Max PLX register offset
    #define DEFAULT_SIZE_COMMON_BUFFER          (64 * 1024)              // Default size of Common Buffer
    #define NUM_DMA_CHANNELS                    2                        // Total number of DMA Channels

#elif (PLX_CHIP == 9056)

    #define PLX_CHIP_TYPE                       0x9056
    #define PLX_DRIVER_NAME                     "Plx9056"
    #define PLX_DRIVER_NAME_UNICODE             L"Plx9056"
    #define MAX_PLX_REG_OFFSET                  PCI9056_MAX_REG_OFFSET   // Max PLX register offset
    #define DEFAULT_SIZE_COMMON_BUFFER          (64 * 1024)              // Default size of Common Buffer
    #define NUM_DMA_CHANNELS                    2                        // Total number of DMA Channels

#elif (PLX_CHIP == 9656)

    #define PLX_CHIP_TYPE                       0x9656
    #define PLX_DRIVER_NAME                     "Plx9656"
    #define PLX_DRIVER_NAME_UNICODE             L"Plx9656"
    #define MAX_PLX_REG_OFFSET                  PCI9656_MAX_REG_OFFSET   // Max PLX register offset
    #define DEFAULT_SIZE_COMMON_BUFFER          (64 * 1024)              // Default size of Common Buffer
    #define NUM_DMA_CHANNELS                    2                        // Total number of DMA Channels

#elif (PLX_CHIP == 8311)

    #define PLX_CHIP_TYPE                       0x8311
    #define PLX_DRIVER_NAME                     "Plx8311"
    #define PLX_DRIVER_NAME_UNICODE             L"Plx8311"
    #define MAX_PLX_REG_OFFSET                  PCI8311_MAX_REG_OFFSET   // Max PLX register offset
    #define DEFAULT_SIZE_COMMON_BUFFER          (64 * 1024)              // Default size of Common Buffer
    #define NUM_DMA_CHANNELS                    2                        // Total number of DMA Channels

#endif



/***********************************************************
 * The following definition is required for drivers
 * requiring DMA functionality.
 **********************************************************/
#if defined(NUM_DMA_CHANNELS)
    #define PLX_DMA_SUPPORT
#else
    // No DMA support
#endif



#endif
