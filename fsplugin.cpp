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

// measure performance
/*
#include <chrono>
*/

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
                        if(newDevice == NULL) newDevice = LIBMTP_Open_Raw_Device(&rawdevices[i]);
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
                    int leaf = getPathLeaf(device, storage, internalPath);
                    LIBMTP_file_t *files;
                    files = LIBMTP_Get_Files_And_Folders(device, storage->id, leaf);
                    // measure performance
                    /*
                    auto t1 = std::chrono::high_resolution_clock::now();
                    
                    auto t2 = std::chrono::high_resolution_clock::now();
                    gLogProc(gPluginNumber, MSGTYPE_CONNECT, (WCHAR*) internalPath.data());
                    auto ms_int = std::chrono::duration_cast<std::chrono::milliseconds>(t2 - t1);
                    gLogProc(gPluginNumber, MSGTYPE_CONNECT, (WCHAR*) int_to_wcharstring(ms_int.count()).data());
                    */
                    
                    if (files != NULL) 
                    {

                        LIBMTP_file_t *file;
                        file = files;
                        int numOfEntries = 0;
                        while (file != NULL) 
                        {
                            numOfEntries++;
                            file = file->next;
                        }
                        // if(internalPath == UTF8toUTF16("/Download")) numOfEntries = 1;

                        pRes = new tResources;
                        pRes->nCount = 0;
                        pRes->resource_array.resize(numOfEntries);
                                                
                        file = files;
                        size_t str_size;
                        for (int i = 0; i < numOfEntries; i++)
                        {
                            str_size = (MAX_PATH > UTF8toUTF16(file->filename).size()+1)? (UTF8toUTF16(file->filename).size()+1): MAX_PATH;
                            memcpy(pRes->resource_array[i].cFileName, UTF8toUTF16(file->filename).data(), str_size * sizeof(WCHAR));
                            if(file->filetype == LIBMTP_FILETYPE_FOLDER) pRes->resource_array[i].dwFileAttributes = FILE_ATTRIBUTE_DIRECTORY;
                            pRes->resource_array[i].nFileSizeLow = 0;
                            pRes->resource_array[i].nFileSizeHigh = 0;
                            pRes->resource_array[i].ftLastWriteTime = get_file_time(file->modificationdate);
                            file = file->next;
                        }
                    }
                    // measure performance
                    /*
                    auto t3 = std::chrono::high_resolution_clock::now();
                    auto ms_int2 = std::chrono::duration_cast<std::chrono::milliseconds>(t3 - t2);
                    gLogProc(gPluginNumber, MSGTYPE_CONNECT, (WCHAR*) int_to_wcharstring(ms_int2.count()).data());
                    */
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