#ifndef __PLXCHIP_H
#define __PLXCHIP_H

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
 *      PlxChip.h
 *
 * Description:
 *
 *      This file contains definitions specific to a PLX Chip
 *
 * Revision History:
 *
 *      02-01-14 : PLX SDK v7.20
 *
 ******************************************************************************/


/**********************************************
 *               Definitions
 *********************************************/
#if (PLX_CHIP == 9050)

    #define PLX_CHIP_TYPE                       0x9050
    #define PLX_DRIVER_NAME                     "Plx9050"
    #define PLX_DRIVER_NAME_UNICODE             L"Plx9050"
    #define DEFAULT_SIZE_COMMON_BUFFER          0                            // Default size of Common Buffer

    // Referenced register definitions
    #define PCI9050_REMAP_SPACE0                0x14
    #define PCI9050_REMAP_SPACE1                0x18
    #define PCI9050_REMAP_SPACE2                0x1C
    #define PCI9050_REMAP_SPACE3                0x20
    #define PCI9050_INT_CTRL_STAT               0x4C
    #define PCI9050_EEPROM_CTRL                 0x50

#elif (PLX_CHIP == 9030)

    #define PLX_CHIP_TYPE                       0x9030
    #define PLX_DRIVER_NAME                     "Plx9030"
    #define PLX_DRIVER_NAME_UNICODE             L"Plx9030"
    #define DEFAULT_SIZE_COMMON_BUFFER          0                            // Default size of Common Buffer

    // Referenced register definitions
    #define PCI9030_PM_CSR                      0x44
    #define PCI9030_HS_CAP_ID                   0x48
    #define PCI9030_REMAP_SPACE0                0x14
    #define PCI9030_REMAP_SPACE1                0x18
    #define PCI9030_REMAP_SPACE2                0x1C
    #define PCI9030_REMAP_SPACE3                0x20
    #define PCI9030_INT_CTRL_STAT               0x4C
    #define PCI9030_EEPROM_CTRL                 0x50

#elif (PLX_CHIP == 9080)

    #define PLX_CHIP_TYPE                       0x9080
    #define PLX_DRIVER_NAME                     "Plx9080"
    #define PLX_DRIVER_NAME_UNICODE             L"Plx9080"
    #define DEFAULT_SIZE_COMMON_BUFFER          (64 * 1024)                  // Default size of Common Buffer
    #define NUM_DMA_CHANNELS                    2                            // Total number of DMA Channels

    // Referenced register definitions
    #define PCI9080_SPACE0_REMAP                0x04
    #define PCI9080_SPACE1_REMAP                0xF4
    #define PCI9080_ENDIAN_DESC                 0x0C
    #define PCI9080_INT_CTRL_STAT               0x68
    #define PCI9080_PCI_DOORBELL                0x64
    #define PCI9080_EEPROM_CTRL_STAT            0x6C
    #define PCI9080_DMA0_MODE                   0x80
    #define PCI9080_DMA1_MODE                   0x94
    #define PCI9080_DMA_COMMAND_STAT            0xA8
    #define PCI9080_OUTPOST_INT_STAT            0x30
    #define PCI9080_OUTPOST_INT_MASK            0x34
    #define PCI9080_FIFO_CTRL_STAT              0xE8

#elif (PLX_CHIP == 9054)

    #define PLX_CHIP_TYPE                       0x9054
    #define PLX_DRIVER_NAME                     "Plx9054"
    #define PLX_DRIVER_NAME_UNICODE             L"Plx9054"
    #define DEFAULT_SIZE_COMMON_BUFFER          (64 * 1024)                  // Default size of Common Buffer
    #define NUM_DMA_CHANNELS                    2                            // Total number of DMA Channels

    // Referenced register definitions
    #define PCI9054_PM_CSR                      0x44
    #define PCI9054_HS_CAP_ID                   0x48
    #define PCI9054_SPACE0_REMAP                0x04
    #define PCI9054_SPACE1_REMAP                0xF4
    #define PCI9054_ENDIAN_DESC                 0x0C
    #define PCI9054_INT_CTRL_STAT               0x68
    #define PCI9054_PCI_DOORBELL                0x64
    #define PCI9054_EEPROM_CTRL_STAT            0x6C
    #define PCI9054_REVISION_ID                 0x74
    #define PCI9054_DMA0_MODE                   0x80
    #define PCI9054_DMA1_MODE                   0x94
    #define PCI9054_DMA_COMMAND_STAT            0xA8
    #define PCI9054_DMA0_PCI_DAC                0xB4
    #define PCI9054_OUTPOST_INT_STAT            0x30
    #define PCI9054_OUTPOST_INT_MASK            0x34
    #define PCI9054_FIFO_CTRL_STAT              0xE8

#elif (PLX_CHIP == 9056)

    #define PLX_CHIP_TYPE                       0x9056
    #define PLX_DRIVER_NAME                     "Plx9056"
    #define PLX_DRIVER_NAME_UNICODE             L"Plx9056"
    #define DEFAULT_SIZE_COMMON_BUFFER          (64 * 1024)                  // Default size of Common Buffer
    #define NUM_DMA_CHANNELS                    2                            // Total number of DMA Channels

    // Referenced register definitions
    #define PCI9056_PM_CSR                      0x44
    #define PCI9056_HS_CAP_ID                   0x48
    #define PCI9056_SPACE0_REMAP                0x04
    #define PCI9056_SPACE1_REMAP                0xF4
    #define PCI9056_ENDIAN_DESC                 0x0C
    #define PCI9056_INT_CTRL_STAT               0x68
    #define PCI9056_PCI_DOORBELL                0x64
    #define PCI9056_EEPROM_CTRL_STAT            0x6C
    #define PCI9056_DMA0_MODE                   0x80
    #define PCI9056_DMA1_MODE                   0x94
    #define PCI9056_DMA_COMMAND_STAT            0xA8
    #define PCI9056_DMA0_PCI_DAC                0xB4
    #define PCI9056_OUTPOST_INT_STAT            0x30
    #define PCI9056_OUTPOST_INT_MASK            0x34
    #define PCI9056_FIFO_CTRL_STAT              0xE8

#elif (PLX_CHIP == 9656)

    #define PLX_CHIP_TYPE                       0x9656
    #define PLX_DRIVER_NAME                     "Plx9656"
    #define PLX_DRIVER_NAME_UNICODE             L"Plx9656"
    #define DEFAULT_SIZE_COMMON_BUFFER          (64 * 1024)                  // Default size of Common Buffer
    #define NUM_DMA_CHANNELS                    2                            // Total number of DMA Channels

    // Referenced register definitions
    #define PCI9656_PM_CSR                      0x44
    #define PCI9656_HS_CAP_ID                   0x48
    #define PCI9656_SPACE0_REMAP                0x04
    #define PCI9656_SPACE1_REMAP                0xF4
    #define PCI9656_ENDIAN_DESC                 0x0C
    #define PCI9656_INT_CTRL_STAT               0x68
    #define PCI9656_PCI_DOORBELL                0x64
    #define PCI9656_EEPROM_CTRL_STAT            0x6C
    #define PCI9656_DMA0_MODE                   0x80
    #define PCI9656_DMA1_MODE                   0x94
    #define PCI9656_DMA_COMMAND_STAT            0xA8
    #define PCI9656_DMA0_PCI_DAC                0xB4
    #define PCI9656_OUTPOST_INT_STAT            0x30
    #define PCI9656_OUTPOST_INT_MASK            0x34
    #define PCI9656_FIFO_CTRL_STAT              0xE8

#elif (PLX_CHIP == 8311)

    #define PLX_CHIP_TYPE                       0x8311
    #define PLX_DRIVER_NAME                     "Plx8311"
    #define PLX_DRIVER_NAME_UNICODE             L"Plx8311"
    #define DEFAULT_SIZE_COMMON_BUFFER          (64 * 1024)                  // Default size of Common Buffer
    #define NUM_DMA_CHANNELS                    2                            // Total number of DMA Channels

    // Referenced register definitions
    #define PCI8311_PM_CSR                      0x44
    #define PCI8311_HS_CAP_ID                   0x48
    #define PCI8311_SPACE0_REMAP                0x04
    #define PCI8311_SPACE1_REMAP                0xF4
    #define PCI8311_ENDIAN_DESC                 0x0C
    #define PCI8311_INT_CTRL_STAT               0x68
    #define PCI8311_PCI_DOORBELL                0x64
    #define PCI8311_EEPROM_CTRL_STAT            0x6C
    #define PCI8311_DMA0_MODE                   0x80
    #define PCI8311_DMA1_MODE                   0x94
    #define PCI8311_DMA_COMMAND_STAT            0xA8
    #define PCI8311_DMA0_PCI_DAC                0xB4
    #define PCI8311_OUTPOST_INT_STAT            0x30
    #define PCI8311_OUTPOST_INT_MASK            0x34
    #define PCI8311_FIFO_CTRL_STAT              0xE8

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
