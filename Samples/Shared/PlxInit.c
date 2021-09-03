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
 *      12-01-07 : PLX SDK v5.20
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
    { ApiSuccess,                   "ApiSuccess"                   },
    { ApiFailed,                    "ApiFailed"                    },
    { ApiNullParam,                 "ApiNullParam"                 },
    { ApiUnsupportedFunction,       "ApiUnsupportedFunction"       },
    { ApiNoActiveDriver,            "ApiNoActiveDriver"            },
    { ApiConfigAccessFailed,        "ApiConfigAccessFailed"        },
    { ApiInvalidDeviceInfo,         "ApiInvalidDeviceInfo"         },
    { ApiInvalidDriverVersion,      "ApiInvalidDriverVersion"      },
    { ApiInvalidOffset,             "ApiInvalidOffset"             },
    { ApiInvalidData,               "ApiInvalidData"               },
    { ApiInvalidSize,               "ApiInvalidSize"               },
    { ApiInvalidAddress,            "ApiInvalidAddress"            },
    { ApiInvalidAccessType,         "ApiInvalidAccessType"         },
    { ApiInvalidIndex,              "ApiInvalidIndex"              },
    { ApiInvalidPowerState,         "ApiInvalidPowerState"         },
    { ApiInvalidIopSpace,           "ApiInvalidIopSpace"           },
    { ApiInvalidHandle,             "ApiInvalidHandle"             },
    { ApiInvalidPciSpace,           "ApiInvalidPciSpace"           },
    { ApiInvalidBusIndex,           "ApiInvalidBusIndex"           },
    { ApiInsufficientResources,     "ApiInsufficientResources"     },
    { ApiWaitTimeout,               "ApiWaitTimeout"               },
    { ApiWaitCanceled,              "ApiWaitCanceled"              },
    { ApiDmaChannelUnavailable,     "ApiDmaChannelUnavailable"     },
    { ApiDmaChannelInvalid,         "ApiDmaChannelInvalid"         },
    { ApiDmaDone,                   "ApiDmaDone"                   },
    { ApiDmaPaused,                 "ApiDmaPaused"                 },
    { ApiDmaInProgress,             "ApiDmaInProgress"             },
    { ApiDmaCommandInvalid,         "ApiDmaCommandInvalid"         },
    { ApiDmaInvalidChannelPriority, "ApiDmaInvalidChannelPriority" },
    { ApiDmaSglPagesGetError,       "ApiDmaSglPagesGetError"       },
    { ApiDmaSglPagesLockError,      "ApiDmaSglPagesLockError"      },
    { ApiMuFifoEmpty,               "ApiMuFifoEmpty"               },
    { ApiMuFifoFull,                "ApiMuFifoFull"                },
    { ApiPowerDown,                 "ApiPowerDown"                 },
    { ApiHSNotSupported,            "ApiHSNotSupported"            },
    { ApiVPDNotSupported,           "ApiVPDNotSupported"           },
    { ApiDeviceInUse,               "ApiDeviceInUse"               },
    { ApiDeviceDisabled,            "ApiDeviceDisabled"            },
    { ApiLastError,                 "Unknown"                      }
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
    PLX_STATUS     rc;
    PLX_DEVICE_KEY DevKey;
    PLX_DEVICE_KEY DevList[MAX_DEVICES_TO_LIST];


    i          = 0;
    NumDevices = 0;
    do
    {
        // Reset device key structure
        memset(&DevKey, PCI_FIELD_IGNORE, sizeof(PLX_DEVICE_KEY));

        // Check if device exists
        rc =
            PlxPci_DeviceFind(
                &DevKey,
                (U16)i
                );

        if (rc == ApiSuccess)
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
    while ((rc == ApiSuccess) && (i < MAX_DEVICES_TO_LIST));

    // Check devices exist
    if (NumDevices == 0)
    {
        return 0;
    }

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

    while (ApiErrors[i].code != ApiLastError)
    {
        if (ApiErrors[i].code == code)
        {
            return ApiErrors[i].text;
        }

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
