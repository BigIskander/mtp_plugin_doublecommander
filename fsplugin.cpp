/*
MTP plugin for Double Commander

Wfx plugin for working with MTP (Media Transfer Protocol) devices in macOS.

Copyright (C) 2025 Iskander Sultanov (BigIskander@gmail.com)

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
#include "fsutils.hpp"
#include <libmtp.h>

#define _plugin_name "MTP"

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

HANDLE DCPCALL FsFindFirstW(WCHAR* Path, WIN32_FIND_DATAW *FindData)
{
    pResources pRes = NULL;
    int i;

    wcharstring wPath(Path);
    std::replace(wPath.begin(), wPath.end(), u'\\', u'/');

    LIBMTP_Init();

    if(wPath.length() == 1) { // root folder of plugin
        int err = LIBMTP_Detect_Raw_Devices(&rawdevices, &numrawdevices);
        
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
                    LIBMTP_mtpdevice_t *newDevice;
                    filterConnectedDevices();
                    for(int i = 0; i < numrawdevices; i++) {
                        newDevice = LIBMTP_Open_Raw_Device_Uncached(&rawdevices[i]);
                        addDevice(newDevice);
                    }
                    if(availableDevices.size() == 0) {
                        gLogProc(gPluginNumber, MSGTYPE_DETAILS, (WCHAR*) u"No MTP device found.");
                        pRes = show_error_entry((char*) "No MTP device found."); 
                        break;
                    }
                    pRes =  showDevices(); 
                    break;
                }
            case LIBMTP_ERROR_GENERAL:
            default:
                {
                    gLogProc(gPluginNumber, MSGTYPE_IMPORTANTERROR, (WCHAR*) u"Libmtp: unknown connection error.");
                    pRes = show_error_entry((char*) "Libmtp: unknown connection error.");
                }
        }
    } else {
        LIBMTP_mtpdevice_t *device;
        wcharstring deviceName, storageName, internalPath;
        parsePath(wPath, deviceName, storageName, internalPath);
        device = getDevice(deviceName);
        
        if (device == NULL) {
            pRes = show_error_entry((char*) "No device...");
        } else {
            LIBMTP_devicestorage_t *storage;
            if(storageName == UTF8toUTF16("")) {
                pRes = showStorages(device);
            } else {
                storage = getStorage(device, storageName);
                if(storage != NULL) {
                    uint32_t leaf;
                    if(getPathLeaf(device, storage, internalPath, leaf)) {
                        pRes = showFilesAndFolders(device, storage, leaf);
                    } else {
                        pRes = show_error_entry((char*) "Incorrect path...");
                    }
                } else {
                    pRes = show_error_entry((char*) "No such storage...");
                }
            }
        }
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

BOOL DCPCALL FsDisconnect(char* DisconnectRoot) 
{
    return true;
}

// int DCPCALL FsExecuteFile(HWND MainWin, char* RemoteName, char* Verb)
// {
//     WCHAR* ReturnedText;
//     int maxlen = 0;
//     if(strcmp(Verb, "open") == 0 && strcmp(RemoteName, "/<Update list of devices>") == 0) 
//     {
//         bool upd = gRequestProc(gPluginNumber, RT_MsgYesNo, (WCHAR*)u"Update list of devices?", (WCHAR*)u"All open MTP connections will be closed.", ReturnedText, maxlen); 
//     }
//     return FS_EXEC_OK;
// }