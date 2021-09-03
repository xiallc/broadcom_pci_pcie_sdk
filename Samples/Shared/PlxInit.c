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
 *      PlxInit.c
 *
 * Description:
 *
 *      This file provides some common functionality, which is used by the
 *      various sample applications.
 *
 * Revision History:
 *
 *      02-01-14 : PLX SDK v7.20
 *
 ******************************************************************************/


#include "ConsFunc.h"
#include "PlxApi.h"
#include "PlxInit.h"




/**********************************************
*               Globals
**********************************************/
API_ERRORS ApiErrors[] =
{
    { PLX_STATUS_OK,                "OK"                },
    { PLX_STATUS_FAILED,            "FAILED"            },
    { PLX_STATUS_NULL_PARAM,        "NULL_PARAM"        },
    { PLX_STATUS_UNSUPPORTED,       "UNSUPPORTED"       },
    { PLX_STATUS_NO_DRIVER,         "NO_DRIVER"         },
    { PLX_STATUS_INVALID_OBJECT,    "INVALID_OBJECT"    },
    { PLX_STATUS_VER_MISMATCH,      "VER_MISMATCH"      },
    { PLX_STATUS_INVALID_OFFSET,    "INVALID_OFFSET"    },
    { PLX_STATUS_INVALID_DATA,      "INVALID_DATA"      },
    { PLX_STATUS_INVALID_SIZE,      "INVALID_SIZE"      },
    { PLX_STATUS_INVALID_ADDR,      "INVALID_ADDR"      },
    { PLX_STATUS_INVALID_ACCESS,    "INVALID_ACCESS"    },
    { PLX_STATUS_INSUFFICIENT_RES,  "INSUFFICIENT_RES"  },
    { PLX_STATUS_TIMEOUT,           "TIMEOUT"           },
    { PLX_STATUS_CANCELED,          "CANCELED"          },
    { PLX_STATUS_COMPLETE,          "COMPLETE"          },
    { PLX_STATUS_PAUSED,            "PAUSED"            },
    { PLX_STATUS_IN_PROGRESS,       "IN_PROGRESS"       },
    { PLX_STATUS_PAGE_GET_ERROR,    "PAGE_GET_ERROR"    },
    { PLX_STATUS_PAGE_LOCK_ERROR,   "PAGE_LOCK_ERROR"   },
    { PLX_STATUS_LOW_POWER,         "LOW_POWER"         },
    { PLX_STATUS_IN_USE,            "IN_USE"            },
    { PLX_STATUS_DISABLED,          "DISABLED"          },
    { PLX_STATUS_PENDING,           "PENDING"           },
    { PLX_STATUS_NOT_FOUND,         "NOT_FOUND"         },
    { PLX_STATUS_INVALID_STATE,     "INVALID_STATE"     },
    { PLX_STATUS_BUFF_TOO_SMALL,    "BUFF_TOO_SMALL"    },
    { PLX_STATUS_RSVD_LAST_ERROR,   "?UNKNOWN?"         }
};




/*********************************************************************
 *
 * Function   : SelectDevice
 *
 * Description: Asks the user which device to select
 *
 * Returns    : Total devices found
 *              -1,  if user cancelled the selection
 *
 ********************************************************************/
S16
SelectDevice(
    PLX_DEVICE_KEY *pKey
    )
{
    S16            i;
    S16            j;
    S16            NumDevices;
    BOOLEAN        bAddDevice;
    PLX_STATUS     status;
    PLX_DEVICE_KEY DevKey;
    PLX_DEVICE_KEY DevList[MAX_DEVICES_TO_LIST];


    i          = 0;
    NumDevices = 0;
    do
    {
        // Reset device key structure
        memset(&DevKey, PCI_FIELD_IGNORE, sizeof(PLX_DEVICE_KEY));

        // Check if device exists
        status =
            PlxPci_DeviceFind(
                &DevKey,
                (U16)i
                );

        if (status == PLX_STATUS_OK)
        {
            // Default to add device
            bAddDevice = TRUE;

            // Verify device is not already in list
            for (j=0; j < NumDevices; j++)
            {
                // Do not add device if already in list
                if ((DevKey.bus      == DevList[j].bus) &&
                    (DevKey.slot     == DevList[j].slot) &&
                    (DevKey.function == DevList[j].function))
                {
                    bAddDevice = FALSE;
                }
            }

            if (bAddDevice)
            {
                // Copy device key info
                DevList[NumDevices] = DevKey;

                // Increment to next device
                NumDevices++;

                Cons_printf(
                    "\t\t    %2d. %04x %04x  [b:%02x s:%02x f:%x]\n",
                    NumDevices, DevKey.DeviceId, DevKey.VendorId,
                    DevKey.bus, DevKey.slot, DevKey.function
                    );
            }

            i++;
        }
    }
    while ((status == PLX_STATUS_OK) && (i < MAX_DEVICES_TO_LIST));

    // Check devices exist
    if (NumDevices == 0)
        return 0;

    Cons_printf("\t\t     0. Cancel\n\n");

    do
    {
        Cons_printf("\t   Device selection --> ");
        Cons_scanf("%hd", &i);
    }
    while (i > NumDevices);

    if (i == 0)
        return -1;

    Cons_printf("\n");

    // Return key information
    *pKey = DevList[i - 1];

    return NumDevices;
}




/*********************************************************************
 *
 * Function   :  PlxSdkErrorText
 *
 * Description:  Returns the text string associated with a PLX_STATUS
 *
 ********************************************************************/
char*
PlxSdkErrorText(
    PLX_STATUS code
    )
{
    U16 i;


    i = 0;

    while (ApiErrors[i].code != PLX_STATUS_RSVD_LAST_ERROR)
    {
        if (ApiErrors[i].code == code)
            return ApiErrors[i].text;

        i++;
    }

    return ApiErrors[i].text;
}




/*********************************************************************
 *
 * Function   :  PlxSdkErrorDisplay
 *
 * Description:  Displays the API error code and corresponding text
 *
 ********************************************************************/
void
PlxSdkErrorDisplay(
    PLX_STATUS code
    )
{
    Cons_printf(
        "\tAPI Error: %s (%03Xh)\n",
        PlxSdkErrorText(code),
        code
        );
}
