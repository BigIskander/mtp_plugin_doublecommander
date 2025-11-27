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

int DCPCALL FsInitW(
    int PluginNr, tProgressProcW pProgressProc, tLogProcW pLogProc, tRequestProcW pRequestProc
) 
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
                    filterConnectedDevices(); // to remove disconnected devices from the list
                    gLogProc(gPluginNumber, MSGTYPE_DETAILS, (WCHAR*) u"No MTP device found.");
                    pRes = NULL;
                    break;
                }
            case LIBMTP_ERROR_CONNECTING:
                {
                    gLogProc(
                        gPluginNumber, 
                        MSGTYPE_IMPORTANTERROR, 
                        (WCHAR*) u"Libmtp: connection error."
                    );
                    pRes = NULL;
                    gRequestProc(gPluginNumber, RT_MsgOK, 
                        (WCHAR*)u"", 
                        (WCHAR*)u"Libmtp: connection error.", 
                        (WCHAR*)u"", 0
                    );
                    break;
                }
            case LIBMTP_ERROR_MEMORY_ALLOCATION:
                {
                    gLogProc(
                        gPluginNumber, 
                        MSGTYPE_IMPORTANTERROR, 
                        (WCHAR*) u"Libmtp: memory allocation error."
                    );
                    pRes = NULL;
                    gRequestProc(gPluginNumber, RT_MsgOK, 
                        (WCHAR*)u"", 
                        (WCHAR*)u"Libmtp: memory allocation error.", 
                        (WCHAR*)u"", 0
                    );
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
                    LIBMTP_FreeMemory(rawdevices);
                    if(availableDevices.size() == 0) {
                        gLogProc(
                            gPluginNumber, 
                            MSGTYPE_DETAILS, 
                            (WCHAR*) u"No MTP device with storage found."
                        );
                        pRes = NULL; 
                        break;
                    }
                    pRes =  showDevices(); 
                    break;
                }
            case LIBMTP_ERROR_GENERAL:
            default:
                {
                    gLogProc(
                        gPluginNumber, 
                        MSGTYPE_IMPORTANTERROR, 
                        (WCHAR*) u"Libmtp: unknown connection error."
                    );
                    pRes = NULL;
                    gRequestProc(gPluginNumber, RT_MsgOK, 
                        (WCHAR*)u"", 
                        (WCHAR*)u"Libmtp: unknown connection error.", 
                        (WCHAR*)u"", 0
                    );
                }
        }
    } else {
        LIBMTP_mtpdevice_t *device;
        wcharstring deviceName, storageName, internalPath;
        parsePath(wPath, deviceName, storageName, internalPath);
        device = getDevice(deviceName);
        
        if (device == NULL) {
            gLogProc(
                gPluginNumber, 
                MSGTYPE_IMPORTANTERROR, 
                (WCHAR*) wcharstring((WCHAR*)u"MTP device \"")
                    .append(deviceName)
                    .append((WCHAR*)u"\" not found.")
                    .data()
            );
            pRes = NULL;
        } else {
            LIBMTP_devicestorage_t *storage;
            if(storageName == UTF8toUTF16("")) {
                pRes = showStorages(device);
                if(pRes == NULL)
                    gLogProc(
                        gPluginNumber, 
                        MSGTYPE_IMPORTANTERROR, 
                        (WCHAR*) wcharstring((WCHAR*)u"MTP device \"")
                            .append(deviceName)
                            .append((WCHAR*)u"\" no storage found.")
                            .data()
                    );
            } else {
                storage = getStorage(device, storageName);
                if(storage != NULL) {
                    uint32_t leaf;
                    if(getPathLeaf(device, storage, internalPath, leaf)) {
                        pRes = showFilesAndFolders(device, storage, leaf);
                    } else {
                        gLogProc(
                            gPluginNumber, 
                            MSGTYPE_IMPORTANTERROR, 
                            (WCHAR*) wcharstring((WCHAR*)u"MTP device \"")
                                .append(deviceName)
                                .append((WCHAR*)u"\" storage name \"")
                                .append(storageName)
                                .append((WCHAR*)u"\" folder with path \"")
                                .append(internalPath)
                                .append((WCHAR*)u"\" not found.")
                                .data()
                        ); 
                        pRes = NULL;
                    }
                } else {
                    gLogProc(
                        gPluginNumber, 
                        MSGTYPE_IMPORTANTERROR, 
                        (WCHAR*) wcharstring((WCHAR*)u"MTP device \"")
                            .append(deviceName)
                            .append((WCHAR*)u"\" storage name \"")
                            .append(storageName)
                            .append((WCHAR*)u"\" not found.")
                            .data()
                    );
                    pRes = NULL;
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
        if(pRes != NULL)
        {
            pRes->resource_array.clear();
            delete pRes;
        }
        return false;
    }
}

int DCPCALL FsFindClose(HANDLE Hdl) 
{
    // this function is not even called
    return 0;
}

void DCPCALL FsGetDefRootName(char* DefRootName, int maxlen)
{
    strncpy(DefRootName, _plugin_name, maxlen);
}

int DCPCALL FsGetFileW(WCHAR* RemoteName, WCHAR* LocalName, int CopyFlags, RemoteInfoStruct* ri)
{
    if(CopyFlags & FS_COPYFLAGS_RESUME) // resume copy not supported
        return FS_FILE_NOTSUPPORTED;

    wcharstring wPath(RemoteName), deviceName, storageName, internalPath;
    std::replace(wPath.begin(), wPath.end(), u'\\', u'/');

    // no copy file if it is root folder of plugin or if it is root folder of device (not supported)
    if(wPath == (WCHAR*)u"/")
        return FS_FILE_NOTSUPPORTED;
    parsePath(wPath, deviceName, storageName, internalPath);
    if(wPath == wcharstring((WCHAR*)u"/").append(deviceName))
        return FS_FILE_NOTSUPPORTED;
    if(wPath == wcharstring((WCHAR*)u"/").append(deviceName).append((WCHAR*)u"/").append(storageName))
        return FS_FILE_NOTSUPPORTED;

    LIBMTP_mtpdevice_t* device = getDevice(deviceName);
    if(device == NULL)
        return FS_FILE_NOTFOUND;

    LIBMTP_devicestorage_t* storage = getStorage(device, storageName);
    if(storage == NULL)
        return FS_FILE_NOTFOUND;

    uint32_t leaf;
    if(getPathLeaf(device, storage, internalPath, leaf, false))
    {
        // TODO: check if file already exists in destination path
        copyFromTo pData;
        pData.RemoteName = RemoteName;
        pData.LocalName = LocalName;
        gProgressProc(gPluginNumber, RemoteName, LocalName, 0);
        if (
            LIBMTP_Get_File_To_File(
                device, leaf, UTF16toUTF8(LocalName).data(), progressFunc, &pData
            ) != 0
        ) {
            return FS_FILE_READERROR; // or maybe FS_FILE_WRITEERROR
        }
        gProgressProc(gPluginNumber, RemoteName, LocalName, 100);
        return FS_FILE_OK;
    }
    
    return FS_FILE_NOTFOUND;
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