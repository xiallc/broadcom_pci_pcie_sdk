#ifndef _REGISTER_DEFS_H
#define _REGISTER_DEFS_H

/******************************************************************************
 *
 * File Name:
 *
 *      RegDefs.h
 *
 * Description:
 *
 *      Definitions for the register sets
 *
 ******************************************************************************/


#include "PlxTypes.h"




/*************************************
 *          Definitions
 ************************************/
#define REGS_LCR            1
#define REGS_RTR            2
#define REGS_DMA            3
#define REGS_MQR            4
#define REGS_MCR            5


typedef struct _REGISTER_SET
{
    U16  Offset;
    char Description[80];
} REGISTER_SET;




/*************************************
 *            Globals
 ************************************/
extern REGISTER_SET Pci_Type_0[];
extern REGISTER_SET Pci_Type_1[];
extern REGISTER_SET Pci_Type_2[];

extern REGISTER_SET Lcr9050[];
extern REGISTER_SET Eep9050[];

extern REGISTER_SET Lcr9030[];
extern REGISTER_SET Eep9030[];

extern REGISTER_SET Lcr9080[];
extern REGISTER_SET Rtr9080[];
extern REGISTER_SET Dma9080[];
extern REGISTER_SET Mqr9080[];
extern REGISTER_SET Eep9080[];

extern REGISTER_SET Pci9054[];
extern REGISTER_SET Lcr9054[];
extern REGISTER_SET Dma9054[];
extern REGISTER_SET Eep9054[];

extern REGISTER_SET Lcr9656[];
extern REGISTER_SET Eep9656[];

extern REGISTER_SET Pci6540[];
extern REGISTER_SET Eep6540[];

extern REGISTER_SET Eep6254[];

extern REGISTER_SET Pci8111[];
extern REGISTER_SET Lcr8111[];
extern REGISTER_SET Eep8111[];

extern REGISTER_SET Pci8500[];
extern REGISTER_SET Lcr8500[];
extern REGISTER_SET Eep8500[];




/*************************************
 *            Functions
 ************************************/
char*
RegSet_DescrGetByIndex(
    REGISTER_SET *pSet,
    U8            index
    );

char*
RegSet_DescrGetByOffset(
    REGISTER_SET *pSet,
    U32           offset
    );



#endif
