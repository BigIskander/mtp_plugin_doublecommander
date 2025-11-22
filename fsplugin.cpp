/*
MTP plugin for Double Commander

Wfx plugin for working with MTP (Media Transfer Protocol) devices

Copyright (C) 2025 Iskander Sultanov

This library is free software; you can redistribute it and/or
modify it under the terms of the GNU Lesser General Public
        License as published by the Free Software Foundation; either
        version 2.1 of the License, or (at your option) any later version.

This library is distributed in the hope that it will be useful,
        but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
        Lesser General Public License for more details.

You should have received a copy of the GNU Lesser General Public
License along with this library; if not, write to the Free Software
        Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301 USA
*/

#include <cstring>
#include "common.h"
#include "fsplugin.h"
#include "utils.hpp"
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

// MTP Devices data
LIBMTP_raw_device_t * rawdevices;
int numrawdevices;
LIBMTP_error_number_t err;

pResources show_error_entry(char* error) {
    pResources pRes = new tResources;
    pRes->nCount = 0;
    pRes->resource_array.resize(1);

    wcharstring wName = UTF8toUTF16(error);
    size_t str_size = (MAX_PATH > wName.size()+1)? (wName.size()+1): MAX_PATH;
        
    // output error message as unknown file entry
    memcpy(pRes->resource_array[0].cFileName, wName.data(), sizeof(WCHAR) * str_size);
    pRes->resource_array[0].nFileSizeLow = 0;
    pRes->resource_array[0].nFileSizeHigh = 0;
    pRes->resource_array[0].ftCreationTime = get_now_time();
    pRes->resource_array[0].ftLastWriteTime = get_now_time();
    pRes->resource_array[0].ftLastAccessTime = get_now_time();
    return pRes;
}

pResources show_devices_entry() {
    pResources pRes = new tResources;
    pRes->nCount = 0;
    pRes->resource_array.resize(numrawdevices);

    // list all available MTP devices
    for (int i = 0; i < numrawdevices; i++) {                    
        // make folder entry (path) name for each device
        wcharstring wName = UTF8toUTF16("1.");
        if(rawdevices[i].device_entry.vendor != NULL)
            wName.append(UTF8toUTF16(rawdevices[i].device_entry.vendor));
        else
            wName.append(UTF8toUTF16("Noname"));
        if(rawdevices[i].device_entry.product != NULL)
            wName
                .append(UTF8toUTF16(" ("))
                .append(UTF8toUTF16(rawdevices[i].device_entry.product))
                .append(UTF8toUTF16(")"));
        // sanitize data no slashes in path name
        std::replace(wName.begin(), wName.end(), U'\\', U' ');
        std::replace(wName.begin(), wName.end(), U'/', U' ');

        size_t str_size = (MAX_PATH > wName.size()+1)? (wName.size()+1): MAX_PATH;
        
        // output device entry as a folder
        memcpy(pRes->resource_array[i].cFileName, wName.data(), sizeof(WCHAR) * str_size);
        pRes->resource_array[i].dwFileAttributes = FILE_ATTRIBUTE_DIRECTORY;
        pRes->resource_array[i].nFileSizeLow = 0;
        pRes->resource_array[i].nFileSizeHigh = 0;
        pRes->resource_array[i].ftCreationTime = get_now_time();
        pRes->resource_array[i].ftLastWriteTime = get_now_time();
        pRes->resource_array[i].ftLastAccessTime = get_now_time();
    }
    return pRes;
}

HANDLE DCPCALL FsFindFirstW(WCHAR* Path, WIN32_FIND_DATAW *FindData)
{
    pResources pRes = NULL;
    int i;

    LIBMTP_Init();
    err = LIBMTP_Detect_Raw_Devices(&rawdevices, &numrawdevices);
    
    switch(err) {
        case LIBMTP_ERROR_NO_DEVICE_ATTACHED:
            {
                gLogProc(gPluginNumber, MSGTYPE_DETAILS, (WCHAR*) u"No MTP device found.");
                pRes = show_error_entry((char*) "No MTP device found.");  
                break;
            }
        case LIBMTP_ERROR_CONNECTING:
            {
                gLogProc(gPluginNumber, MSGTYPE_IMPORTANTERROR, (WCHAR*) u"Libmtp: connection error.");
                pRes = show_error_entry((char*) "Libmtp: connection error.");
                break;
            }
        case LIBMTP_ERROR_MEMORY_ALLOCATION:
            {
                gLogProc(gPluginNumber, MSGTYPE_IMPORTANTERROR, (WCHAR*) u"Libmtp: memory allocation error.");
                pRes = show_error_entry((char*) "Libmtp: memory allocation error.");
                break;
            }
        case LIBMTP_ERROR_NONE:
            {
                pRes = show_devices_entry();
                break;
            }
        case LIBMTP_ERROR_GENERAL:
        default:
            {
                gLogProc(gPluginNumber, MSGTYPE_IMPORTANTERROR, (WCHAR*) u"Libmtp: unknown connection error.");
                pRes = show_error_entry((char*) "Libmtp: unknown connection error.");
            }
    }

    /* return results */

    if(!pRes || pRes->resource_array.size()==0)
        return (HANDLE)-1;

    if(pRes->resource_array.size()>0){
        memset(FindData, 0, sizeof(WIN32_FIND_DATAW));
        memcpy(FindData, &(pRes->resource_array[0]), sizeof(WIN32_FIND_DATAW));
        pRes->nCount++;
    }

    return (HANDLE) pRes;
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
}

int DCPCALL FsFindClose(HANDLE Hdl) 
{
    pResources pRes = (pResources) Hdl;
    if(pRes){
        pRes->resource_array.clear();
        delete pRes;
    }

    return 0;
}

void DCPCALL FsGetDefRootName(char* DefRootName, int maxlen)
{
    strncpy(DefRootName, _plugin_name, maxlen);
}
