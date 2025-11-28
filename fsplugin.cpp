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
#include "pathutils.h"

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
bool isInit = false;

HANDLE DCPCALL FsFindFirstW(WCHAR* Path, WIN32_FIND_DATAW *FindData)
{
    pResources pRes = NULL;
    int i;

    wcharstring wPath(Path);
    std::replace(wPath.begin(), wPath.end(), u'\\', u'/');

    if(!isInit)
    {
        LIBMTP_Init(); // call this function only once
        isInit = true;
    } 

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

    // do not copy if file exists and no overwrite flag is set
    BOOL isFileExists = file_exists(UTF16toUTF8(LocalName));
    if(isFileExists && !(CopyFlags & FS_COPYFLAGS_OVERWRITE) )
        return FS_FILE_EXISTS;

    LIBMTP_mtpdevice_t* device = getDevice(deviceName);
    if(device == NULL)
        return FS_FILE_NOTFOUND;

    LIBMTP_devicestorage_t* storage = getStorage(device, storageName);
    if(storage == NULL)
        return FS_FILE_NOTFOUND;

    if(gProgressProc(gPluginNumber, RemoteName, LocalName, 0) != 0) 
            return FS_FILE_USERABORT;

    uint32_t leaf;
    if(getPathLeaf(device, storage, internalPath, leaf, false))
    {
        copyFromTo pData;
        pData.From = RemoteName;
        pData.To = LocalName;
        if (
            LIBMTP_Get_File_To_File(
                device, leaf, UTF16toUTF8(LocalName).data(), progressFunc, &pData
            ) != 0
        ) {
            return FS_FILE_READERROR;
        }
        gProgressProc(gPluginNumber, RemoteName, LocalName, 100);
        return FS_FILE_OK;
    }
    
    return FS_FILE_NOTFOUND;
}

int DCPCALL FsPutFileW(WCHAR* LocalName, WCHAR* RemoteName, int CopyFlags) {
    if (CopyFlags & FS_COPYFLAGS_RESUME)
        return FS_FILE_NOTSUPPORTED;

    // this hint is never sent
    if(((CopyFlags & FS_COPYFLAGS_EXISTS_SAMECASE) | (CopyFlags & FS_COPYFLAGS_EXISTS_DIFFERENTCASE)) 
            & !(CopyFlags & FS_COPYFLAGS_OVERWRITE))  
        return FS_FILE_EXISTS;
    
    wcharstring wPath(RemoteName), deviceName, storageName, internalPath, folderPath, fileName;
    std::replace(wPath.begin(), wPath.end(), u'\\', u'/');

    // no copy file to root folder of plugin or to root folder of device (not supported)
    getFolderPath(wPath, folderPath);
    if(folderPath == (WCHAR*)u"/")
        return FS_FILE_NOTSUPPORTED;
    parsePath(wPath, deviceName, storageName, internalPath);
    if(folderPath == wcharstring((WCHAR*)u"/").append(deviceName))
        return FS_FILE_NOTSUPPORTED;

    LIBMTP_mtpdevice_t* device = getDevice(deviceName);
    if(device == NULL)
        return FS_FILE_WRITEERROR;

    LIBMTP_devicestorage_t* storage = getStorage(device, storageName);
    if(storage == NULL)
        return FS_FILE_WRITEERROR;

    getFolderPath(internalPath, folderPath);

    uint32_t folderLeaf;
    if(!getPathLeaf(device, storage, folderPath, folderLeaf))
        return FS_FILE_WRITEERROR;

    uint64_t fileSize;
    if(!get_file_size(UTF16toUTF8(LocalName), fileSize))
        return FS_FILE_READERROR;
    
    getFileName(internalPath, fileName);

    if(gProgressProc(gPluginNumber, LocalName, RemoteName, 0) != 0) 
        return FS_FILE_USERABORT;

    // check if file already exists
    uint32_t leaf;
    if(getPathLeaf(device, storage, internalPath, leaf, false)) 
    {
        if(!(CopyFlags & FS_COPYFLAGS_OVERWRITE))
        {
            return FS_FILE_EXISTS;
        } else {
            // delete already existing file (to replace with new one)
            if(LIBMTP_Delete_Object(device, leaf) != 0) 
            {
                return FS_FILE_WRITEERROR;
            }
        }
    }

    LIBMTP_file_t *genfile = LIBMTP_new_file_t();
    genfile->filesize = fileSize;
    genfile->filename = strdup(UTF16toUTF8(fileName.data()).data());
    genfile->filetype = find_filetype(UTF16toUTF8(fileName.data()).data());
    genfile->parent_id = folderLeaf;
    genfile->storage_id = storage->id;

    copyFromTo pData;
    pData.From = LocalName;
    pData.To = RemoteName;

    if (
        LIBMTP_Send_File_From_File(
            device, UTF16toUTF8(LocalName).data(), genfile, progressFunc, &pData
        ) !=0
    ) {
        LIBMTP_destroy_file_t(genfile);
        return FS_FILE_WRITEERROR;
    }

    LIBMTP_destroy_file_t(genfile);   
    gProgressProc(gPluginNumber, LocalName, RemoteName, 100); 
    return FS_FILE_OK;
}

int DCPCALL FsRenMovFileW(WCHAR* OldName, WCHAR* NewName, BOOL Move, BOOL OverWrite, RemoteInfoStruct* ri)
{
    wcharstring wPathOld(OldName), wPathNew(NewName);
    wcharstring deviceNameOld, storageNameOld, internalPathOld, folderPathOld;
    wcharstring deviceNameNew, storageNameNew, internalPathNew, folderPathNew;
    std::replace(wPathOld.begin(), wPathOld.end(), u'\\', u'/');
    std::replace(wPathNew.begin(), wPathNew.end(), u'\\', u'/');

    parsePath(wPathOld, deviceNameOld, storageNameOld, internalPathOld);
    parsePath(wPathNew, deviceNameNew, storageNameNew, internalPathNew);
    getFolderPath(wPathOld, folderPathOld);
    getFolderPath(wPathNew, folderPathNew);

    // move or copy directly from one device to another not supported
    if(deviceNameNew != deviceNameOld)
        return FS_FILE_NOTSUPPORTED;
    /* TODO: try to implement device to device file transfer */

    // no copy file if it is root folder of plugin or if it is root folder of device (not supported)
    if(wPathOld == (WCHAR*)u"/")
        return FS_FILE_NOTSUPPORTED;
    if(wPathOld == wcharstring((WCHAR*)u"/").append(deviceNameOld))
        return FS_FILE_NOTSUPPORTED;
    if(wPathOld == wcharstring((WCHAR*)u"/")
        .append(deviceNameOld)
        .append((WCHAR*)u"/")
        .append(storageNameOld)
    )
        return FS_FILE_NOTSUPPORTED;

    // no copy file to root folder of plugin or to root folder of device (not supported)
    if(folderPathNew == (WCHAR*)u"/")
        return FS_FILE_NOTSUPPORTED;
    if(folderPathNew == wcharstring((WCHAR*)u"/").append(deviceNameNew))
        return FS_FILE_NOTSUPPORTED;

    wcharstring fileNameNew;
        getFileName(wPathNew, fileNameNew);
        if(fileNameNew == (WCHAR*)u"") // no empty name
            return FS_FILE_WRITEERROR;

    LIBMTP_mtpdevice_t* deviceOld = getDevice(deviceNameOld);
        if(deviceOld == NULL)
            return FS_FILE_NOTFOUND;
    LIBMTP_devicestorage_t* storageOld = getStorage(deviceOld, storageNameOld);
        if(storageOld == NULL)
            return FS_FILE_NOTFOUND;

    bool isOldFolder = false;
    uint32_t leafOld;
    if(getPathLeaf(deviceOld, storageOld, internalPathOld, leafOld))
        isOldFolder = true;
    else if(!getPathLeaf(deviceOld, storageOld, internalPathOld, leafOld, false))
        return FS_FILE_NOTFOUND;

    // renaming file or folder if necessary
    if(folderPathNew == folderPathOld)
    {
        // Just rename it
        if(isOldFolder)
        {
            // rename folder
            LIBMTP_folder_t *folder = LIBMTP_new_folder_t();
            if(folder == NULL)
                return FS_FILE_WRITEERROR; // it's extreemly unlikely though
            folder->folder_id = leafOld;
            if(
                LIBMTP_Set_Folder_Name(
                    deviceOld, folder, UTF16toUTF8(fileNameNew.data()).data()
                ) == 0
            ) {
                LIBMTP_destroy_folder_t(folder);
                return FS_FILE_OK;
            }
            LIBMTP_destroy_folder_t(folder);
            return FS_FILE_WRITEERROR;
        } else {
            LIBMTP_file_t *file = LIBMTP_new_file_t();
            if(file == NULL)
                return FS_FILE_WRITEERROR; // it's extreemly unlikely though
            file->item_id = leafOld;
            file->filetype = find_filetype(UTF16toUTF8(fileNameNew.data()).data());
            if(
                LIBMTP_Set_File_Name(
                    deviceOld, file, UTF16toUTF8(fileNameNew.data()).data()
                ) == 0
            ) {
                LIBMTP_destroy_file_t(file);
                return FS_FILE_OK;
            }
            LIBMTP_destroy_file_t(file);
            return FS_FILE_WRITEERROR;
        }
    }

    // no copy or move folder
    if(isOldFolder)
        return FS_FILE_NOTSUPPORTED; // or some other error
        
    LIBMTP_devicestorage_t* storageNew;
    if(storageNameNew == storageNameOld)
    {
        storageNew = storageOld;
    } else {
        storageNew = getStorage(deviceOld, storageNameNew);
        if(storageNew == NULL)
            return FS_FILE_WRITEERROR;
    }

    // check if file or folder with destination path already exists
    bool isNewExists = false;
    uint32_t leafNew;
    if(getPathLeaf(deviceOld, storageNew, internalPathNew, leafNew))
        isNewExists = true;
    else if(getPathLeaf(deviceOld, storageNew, internalPathNew, leafNew, false))
        isNewExists = true;

    if(isNewExists & !OverWrite)
        return FS_FILE_EXISTS;

    wcharstring internalParentNew;
    getFolderPath(internalPathNew, internalParentNew);

    uint32_t parentLeafNew;
    if(!getPathLeaf(deviceOld, storageOld, internalParentNew, parentLeafNew))
        return FS_FILE_WRITEERROR;

    if(gProgressProc(gPluginNumber, OldName, NewName, 0) != 0) 
        return FS_FILE_USERABORT;

    if(isNewExists & OverWrite)
    {
        // delete already existing file (to replace with new one)
        if(
            LIBMTP_Delete_Object(deviceOld, leafNew) != 0
        ) 
        {
            return FS_FILE_WRITEERROR;
        }
    }

    // move or copy the file
    if(Move)
    {
        if(
            LIBMTP_Move_Object(
                deviceOld, leafOld, storageOld->id, parentLeafNew
            ) == 0
        ) {
            gProgressProc(gPluginNumber, OldName, NewName, 100);
            return FS_FILE_OK;
        }
        return FS_FILE_WRITEERROR;
    } else {
        if(
            LIBMTP_Copy_Object(
                deviceOld, leafOld, storageOld->id, parentLeafNew
            ) == 0
        ) {
            gProgressProc(gPluginNumber, OldName, NewName, 100);
            return FS_FILE_OK;
        }
        return FS_FILE_WRITEERROR;
    }
    
    return FS_FILE_OK;
}

BOOL DCPCALL FsDeleteFileW(WCHAR* RemoteName)
{
    wcharstring wPath(RemoteName), deviceName, storageName, internalPath;
    std::replace(wPath.begin(), wPath.end(), u'\\', u'/');

    // no delete file if it is root folder of plugin or if it is root folder of device (not supported)
    if(wPath == (WCHAR*)u"/")
        return false;
    parsePath(wPath, deviceName, storageName, internalPath);
    if(wPath == wcharstring((WCHAR*)u"/").append(deviceName))
        return false;
    if(wPath == wcharstring((WCHAR*)u"/").append(deviceName).append((WCHAR*)u"/").append(storageName))
        return false;

    LIBMTP_mtpdevice_t* device = getDevice(deviceName);
    if(device == NULL)
        return false;

    LIBMTP_devicestorage_t* storage = getStorage(device, storageName);
    if(storage == NULL)
        return false;

    uint32_t leaf;
    if(!getPathLeaf(device, storage, internalPath, leaf, false)) 
        return false;
    
    if(LIBMTP_Delete_Object(device, leaf) != 0) 
        return false;

    return true;
}

BOOL DCPCALL FsRemoveDirW(WCHAR* RemoteName)
{
    wcharstring wPath(RemoteName), deviceName, storageName, internalPath;
    std::replace(wPath.begin(), wPath.end(), u'\\', u'/');

    // no delete file if it is root folder of plugin or if it is root folder of device (not supported)
    if(wPath == (WCHAR*)u"/")
        return false;
    parsePath(wPath, deviceName, storageName, internalPath);
    if(wPath == wcharstring((WCHAR*)u"/").append(deviceName))
        return false;
    if(wPath == wcharstring((WCHAR*)u"/").append(deviceName).append((WCHAR*)u"/").append(storageName))
        return false;

    LIBMTP_mtpdevice_t* device = getDevice(deviceName);
    if(device == NULL)
        return false;

    LIBMTP_devicestorage_t* storage = getStorage(device, storageName);
    if(storage == NULL)
        return false;

    uint32_t leaf;
    if(!getPathLeaf(device, storage, internalPath, leaf)) 
        return false;
    
    // Check if folder is empty
    LIBMTP_file_t *files;
    files = LIBMTP_Get_Files_And_Folders(device, storage->id, leaf);
    if (files != NULL) 
    {
        // delete non empty folder not rocommended
        // recursively deleting content of folder
        if(files->filetype == LIBMTP_FILETYPE_FOLDER) 
        {
            if(!FsRemoveDirW(
                    (WCHAR*)UTF8toUTF16("")
                        .append(RemoteName)
                        .append((WCHAR*)u"/")
                        .append(UTF8toUTF16(files->filename))
                        .data()
                )
            ) {
                LIBMTP_destroy_file_t(files);
                return false;
            }
        } else {
            if(!FsDeleteFileW(
                    (WCHAR*)UTF8toUTF16("")
                        .append(RemoteName)
                        .append((WCHAR*)u"/")
                        .append(UTF8toUTF16(files->filename))
                        .data()
                )
            ) {
                LIBMTP_destroy_file_t(files);
                return false;
            }
        }
        LIBMTP_destroy_file_t(files);
    }

    // remove folder after it's content is empty
    if(LIBMTP_Delete_Object(device, leaf) != 0) 
        return false;

    return true;
}
    
    // https://github.com/libmtp/libmtp/blob/master/src/libmtp.c#L5467 // file to handler
    // https://github.com/libmtp/libmtp/blob/master/src/libmtp.c#L6175 // file from handler
    // https://github.com/libmtp/libmtp/blob/master/src/libmtp.c#L7584 // create folder
    // https://github.com/libmtp/libmtp/blob/master/src/libmtp.c#L8910 // representative sample format
    // https://github.com/libmtp/libmtp/blob/master/src/libmtp.c#L9212 // thumbnail


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