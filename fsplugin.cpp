#include <cstring>
#include <vector>
#include <codecvt>
#include "common.h"
#include "fsplugin.h"
#include <libmtp.h>

#define _plugin_name "MTP plugin for Double Commander"

typedef struct {
    int nCount;
    std::vector<WIN32_FIND_DATAW> resource_array;
} tResources, *pResources;

typedef std::basic_string<WCHAR> wcharstring;

FILETIME get_now_time()
{
    time_t t2 = time(0);
    int64_t ft = (int64_t) t2 * 10000000 + 116444736000000000;
    FILETIME file_time;
    file_time.dwLowDateTime = ft & 0xffff;
    file_time.dwHighDateTime = ft >> 32;
    return file_time;
}

wcharstring UTF8toUTF16(const std::string &str)
{
    std::wstring_convert<std::codecvt_utf8<char16_t>, char16_t> convert2;
    std::u16string utf16 = convert2.from_bytes(str);
    return wcharstring((WCHAR*)utf16.data());
}

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
    pResources pRes = NULL;

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
                pResources pRes = new tResources;
                pRes->nCount = 0;
                pRes->resource_array.resize(numrawdevices);

                for (i = 0; i < numrawdevices; i++) {
                    gLogProc(gPluginNumber, MSGTYPE_CONNECT, (WCHAR*) u"Device:");
                    gLogProc(gPluginNumber, MSGTYPE_CONNECT, (WCHAR*) rawdevices[i].device_entry.product);

                    wcharstring wName = UTF8toUTF16(rawdevices[i].device_entry.product);
                    size_t str_size = (MAX_PATH > wName.size()+1)? (wName.size()+1): MAX_PATH;

                    memcpy(pRes->resource_array[i].cFileName, wName.data(), sizeof(WCHAR) * str_size);
                    pRes->resource_array[i].dwFileAttributes = FILE_ATTRIBUTE_DIRECTORY;
                    pRes->resource_array[i].nFileSizeLow = 0;
                    pRes->resource_array[i].nFileSizeHigh = 0;
                    pRes->resource_array[i].ftCreationTime = get_now_time();
                    pRes->resource_array[i].ftLastWriteTime = get_now_time();
                    pRes->resource_array[i].ftLastAccessTime = get_now_time();
                }

                if(!pRes || pRes->resource_array.size()==0)
                    return (HANDLE)-1;

                if(pRes->resource_array.size()>0){
                    memset(FindData, 0, sizeof(WIN32_FIND_DATAW));
                    memcpy(FindData, &(pRes->resource_array[0]), sizeof(WIN32_FIND_DATAW));
                    pRes->nCount++;
                }

                return (HANDLE) pRes;
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
    pResources pRes = (pResources) Hdl;

    if(pRes && (pRes->nCount < pRes->resource_array.size()) ){
        memcpy(FindData, &pRes->resource_array[pRes->nCount], sizeof(WIN32_FIND_DATAW));
        pRes->nCount++;
        return true;
    } else {
        return false;
    }

    /* not implemented yet */
    // return false;
}

int DCPCALL FsFindClose(HANDLE Hdl) 
{
    pResources pRes = (pResources) Hdl;
    if(pRes){
        pRes->resource_array.clear();
        delete pRes;
    }

    return 0;
    /* not implemented yet */
    // return 0;
}

void DCPCALL FsGetDefRootName(char* DefRootName, int maxlen)
{
    strncpy(DefRootName, _plugin_name, maxlen);
}
