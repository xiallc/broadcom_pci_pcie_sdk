#ifndef PLXINIT_H
#define PLXINIT_H

/******************************************************************************
 *
 * File Name:
 *
 *      PlxInit.h
 *
 * Description:
 *
 *      Header file for the PlxInit.c module
 *
 * Revision History:
 *
 *      12-01-07 : PLX SDK v5.20
 *
 ******************************************************************************/


#ifdef __cplusplus
extern "C" {
#endif


#include "PlxTypes.h"




/******************************
*        Definitions
******************************/
#define MAX_DEVICES_TO_LIST        100

typedef struct _API_ERRORS
{
    PLX_STATUS  code;
    char       *text;
} API_ERRORS;




/********************************************************************************
*        Functions
*********************************************************************************/
S16
SelectDevice(
    PLX_DEVICE_KEY *pKey
    );

char*
PlxSdkErrorText(
    PLX_STATUS code
    );

void
PlxSdkErrorDisplay(
    PLX_STATUS code
    );




#ifdef __cplusplus
}
#endif

#endif
