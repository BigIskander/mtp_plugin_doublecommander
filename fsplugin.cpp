#include <cstring>
#include "common.h"
#include "fsplugin.h"
#include <libmtp.h>

#define _plugin_name "MTP plugin for Double Commander"

int gPluginNumber;
tProgressProcW gProgressProc = NULL;
tLogProcW gLogProc = NULL;
tRequestProcW gRequestProc = NULL;
tCryptProcW gCryptProc = NULL;

int DCPCALL FsInitW(int PluginNr, tProgressProcW pProgressProc, tLogProcW pLogProc, tRequestProcW pRequestProc) 
{
    gProgressProc = pProgressProc;
    gLogProc = pLogProc;
    gRequestProc = pRequestProc;
    gPluginNumber = PluginNr;

    return 0;
}

HANDLE DCPCALL FsFindFirstW(WCHAR* Path, WIN32_FIND_DATAW *FindData)
{
    LIBMTP_raw_device_t * rawdevices;
    int numrawdevices;
    LIBMTP_error_number_t err;
    int i;

    gLogProc(gPluginNumber, MSGTYPE_CONNECT, (WCHAR*) u"FsFindFirstW was called... success...");

    LIBMTP_Init();
    err = LIBMTP_Detect_Raw_Devices(&rawdevices, &numrawdevices);
    
    switch(err) {
        case LIBMTP_ERROR_NO_DEVICE_ATTACHED:
            gLogProc(gPluginNumber, MSGTYPE_CONNECT, (WCHAR*) u"No raw devices found.");
            break;
        case LIBMTP_ERROR_CONNECTING:
            gLogProc(gPluginNumber, MSGTYPE_CONNECT, (WCHAR*) u"Detect: There has been an error connecting. Exiting");
            break;
        case LIBMTP_ERROR_MEMORY_ALLOCATION:
            gLogProc(gPluginNumber, MSGTYPE_CONNECT, (WCHAR*) u"Detect: Encountered a Memory Allocation Error. Exiting");
            break;
        case LIBMTP_ERROR_NONE:
            {
                gLogProc(gPluginNumber, MSGTYPE_CONNECT, (WCHAR*) u"Detect: Yes devices... kinda.. not implemented yet");
                for (i = 0; i < numrawdevices; i++) {
                    gLogProc(gPluginNumber, MSGTYPE_CONNECT, (WCHAR*) u"Device:");
                    gLogProc(gPluginNumber, MSGTYPE_CONNECT, (WCHAR*) rawdevices[i].device_entry.product);
                }
            }
            break;
        case LIBMTP_ERROR_GENERAL:
        default:
            gLogProc(gPluginNumber, MSGTYPE_CONNECT, (WCHAR*) u"Unknown connection error.");
    }
    
    /* not implemented yet */
    return (HANDLE)-1;
}

BOOL DCPCALL FsFindNextW(HANDLE Hdl, WIN32_FIND_DATAW *FindData)
{
    /* not implemented yet */
    return false;
}

int DCPCALL FsFindClose(HANDLE Hdl) 
{
    /* not implemented yet */
    return 0;
}

void DCPCALL FsGetDefRootName(char* DefRootName, int maxlen)
{
    strncpy(DefRootName, _plugin_name, maxlen);
}
