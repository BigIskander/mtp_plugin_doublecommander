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
bool isPut = false;

HANDLE DCPCALL FsFindFirstW(WCHAR* Path, WIN32_FIND_DATAW *FindData)
{
    pResources pRes = NULL;

    wcharstring wPath(Path);
    if(wPath.length() == 0) return (HANDLE)-1;
    std::replace(wPath.begin(), wPath.end(), u'\\', u'/');

    // fix issue with traversing files and folders
    if(wPath.size() > 1 && wPath.substr(wPath.size() - 1) == (WCHAR*)u"/")
    {
        wPath = wPath.substr(0, wPath.size() - 1);
        // ignore this kind of request when put files, for speed
        if(isPut) return (HANDLE)-1;
    }

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
                    // gLogProc(gPluginNumber, MSGTYPE_CONNECT, (WCHAR*) u"Ok.");
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
                    if(getPathLeaf(device, storage, deviceName, storageName, internalPath, leaf)) {
                        pRes = showFilesAndFolders(device, storage, wPath, leaf);
                    } else {
                        gLogProc(
                            gPluginNumber, 
                            MSGTYPE_DETAILS, 
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

    wcharstring wPath(RemoteName), deviceName, storageName, internalPath, wLocal(LocalName);
    if(wPath.length() == 0 || wLocal.length() == 0) return FS_FILE_OK; // just ignore this case
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

    wcharstring folderPath, fileName;
    getFolderPath(internalPath, folderPath);
    
    // check if parent folder exists
    uint32_t leaf;
    if(!getPathLeaf(device, storage, deviceName, storageName, folderPath, leaf))
        return FS_FILE_READERROR; // fail if parent folder was moved or renamed

    // search and get leaf from cache (for speed)
    if(!getLeafFromCachedFolder(device, RemoteName, leaf)) 
        return FS_FILE_NOTFOUND; // if it is not in cache it is not exists probably

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

int DCPCALL FsPutFileW(WCHAR* LocalName, WCHAR* RemoteName, int CopyFlags) {
    if (CopyFlags & FS_COPYFLAGS_RESUME)
        return FS_FILE_NOTSUPPORTED;

    // this hint is never sent
    if(((CopyFlags & FS_COPYFLAGS_EXISTS_SAMECASE) || (CopyFlags & FS_COPYFLAGS_EXISTS_DIFFERENTCASE)) 
            && !(CopyFlags & FS_COPYFLAGS_OVERWRITE))  
        return FS_FILE_EXISTS;
    
    wcharstring wPath(RemoteName), deviceName, storageName, internalPath, 
        folderPath, fileName, wLocal(LocalName);
    if(wPath.length() == 0 || wLocal.length() == 0) return FS_FILE_OK; // just ignore this case
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
    if(!getPathLeaf(device, storage, deviceName, storageName, folderPath, folderLeaf))
        return FS_FILE_WRITEERROR;

    uint64_t fileSize;
    if(!get_file_size(UTF16toUTF8(LocalName), fileSize))
        return FS_FILE_READERROR;
    
    getFileName(internalPath, fileName);

    if(gProgressProc(gPluginNumber, LocalName, RemoteName, 0) != 0) 
        return FS_FILE_USERABORT;

    bool isLeafFound = false;
    uint32_t leaf; 
    // search and get leaf from cache (for speed)
    if(getLeafFromCachedFolder(device, RemoteName, leaf)) 
        isLeafFound = true;

    // check if file already exists
    if(isLeafFound) 
    {
        if(!(CopyFlags & FS_COPYFLAGS_OVERWRITE))
        {
            return FS_FILE_EXISTS;
        } else {
            // delete already existing file or folder (to replace with new one)
            if(!deleteFileOrFolder(device, storage, leaf))
                return FS_FILE_WRITEERROR;
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
    if(wPathOld.length() == 0 || wPathNew.length() == 0) return FS_FILE_OK; // just ignore this case
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
    // search and get leaf from cache (for speed)
    if(!getLeafFromCachedFolder(deviceOld, OldName, leafOld))
        return FS_FILE_NOTFOUND; // if it is not in cache it is not exists probably
    
    // detect is it folder or not
    LIBMTP_file_t* file = LIBMTP_Get_Filemetadata(deviceOld, leafOld);
    isOldFolder = file->filetype == LIBMTP_FILETYPE_FOLDER;
    LIBMTP_destroy_file_t(file);

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

    wcharstring parentFolderNew;
    bool isNewExists = false;
    uint32_t leafNew;
    getFolderPath(NewName, parentFolderNew);
    // make cache if cache not exists (skip this step otherwise)
    if(!isFolderBusy(parentFolderNew)) 
    {
        makeFolderItemsCache(parentFolderNew);
        addBusyFolder(parentFolderNew);
    }
    // search and get leaf from cache (for speed)
    if(getLeafFromCachedFolder(deviceOld, NewName, leafNew))
        isNewExists = true;

    if(isNewExists && !OverWrite)
        return FS_FILE_EXISTS;

    wcharstring internalParentNew;
    getFolderPath(internalPathNew, internalParentNew);

    uint32_t parentLeafNew;
    if(!getPathLeaf(
        deviceOld, storageNew, deviceNameOld, storageNameNew, internalParentNew, parentLeafNew
    ))
        return FS_FILE_WRITEERROR;

    if(gProgressProc(gPluginNumber, OldName, NewName, 0) != 0) 
        return FS_FILE_USERABORT;

    if(isNewExists & OverWrite)
    {
        // delete already existing file or folder (to replace with new one)
        if(!deleteFileOrFolder(deviceOld, storageNew, leafNew))
            return FS_FILE_WRITEERROR;
    }

    // move or copy the file
    if(Move)
    {
        if(
            LIBMTP_Move_Object(
                deviceOld, leafOld, storageNew->id, parentLeafNew
            ) == 0
        ) {
            gProgressProc(gPluginNumber, OldName, NewName, 100);
            return FS_FILE_OK;
        } 
        return FS_FILE_WRITEERROR;
    } else {
        if(
            LIBMTP_Copy_Object(
                deviceOld, leafOld, storageNew->id, parentLeafNew
            ) == 0
        ) {
            gProgressProc(gPluginNumber, OldName, NewName, 100);
            return FS_FILE_OK;
        } 
        return FS_FILE_WRITEERROR;
    }
}

BOOL DCPCALL FsDeleteFileW(WCHAR* RemoteName)
{
    wcharstring wPath(RemoteName), deviceName, storageName, internalPath;
    if(wPath.length() == 0) return true; // just ignore this case
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
    // search and get leaf from cache (for speed)
    if(!getLeafFromCachedFolder(device, RemoteName, leaf)) 
        return false; // if it is not in cache it is not exists probably
    
    if(!deleteFileOrFolder(device, storage, leaf)) 
        return false;

    return true;
}

BOOL DCPCALL FsRemoveDirW(WCHAR* RemoteName)
{
    wcharstring wPath(RemoteName), deviceName, storageName, internalPath;
    if(wPath.length() == 0) return true; // just ignore this case
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
    // search and get leaf from cache (for speed)
    if(!getLeafFromCachedFolder(device, wPath, leaf))
        return false;
    
    // Delete folder
    if(!deleteFileOrFolder(device, storage, leaf))
        return false;
    
    // remove folder from cache after deletion
    removeLeafsFromCache(device, RemoteName);

    return true;
}

BOOL DCPCALL FsMkDirW(WCHAR* Path)
{
    wcharstring wPath(Path), deviceName, storageName, internalPath, folderPath, fileName;
    if(wPath.length() == 0) return true; // just ignore this case
    std::replace(wPath.begin(), wPath.end(), u'\\', u'/');

    // no create folder in root folder of plugin or in root folder of device (not supported)
    getFolderPath(wPath, folderPath);
    if(folderPath == (WCHAR*)u"/")
        return false;
    parsePath(wPath, deviceName, storageName, internalPath);
    if(folderPath == wcharstring((WCHAR*)u"/").append(deviceName))
        return false;

    LIBMTP_mtpdevice_t* device = getDevice(deviceName);
    if(device == NULL)
        return false;

    LIBMTP_devicestorage_t* storage = getStorage(device, storageName);
    if(storage == NULL)
        return false;

    getFolderPath(internalPath, folderPath);

    uint32_t folderLeaf;
    if(!getPathLeaf(device, storage, deviceName, storageName, folderPath, folderLeaf))
        return false;

    // check if folder or file already exists
    uint32_t leaf;
    // search and get leaf from cache (for speed)
    if(getLeafFromCachedFolder(device, wPath, leaf))
    {
        // detect is it folder or not
        LIBMTP_file_t* file = LIBMTP_Get_Filemetadata(device, leaf);
        if(file->filetype == LIBMTP_FILETYPE_FOLDER)
        {
            LIBMTP_destroy_file_t(file);
            return true;
        }
        LIBMTP_destroy_file_t(file);
        return false;
    }

    getFileName(internalPath, fileName);

    if(LIBMTP_Create_Folder(
            device, 
            (char*)UTF16toUTF8((WCHAR*)fileName.data()).data(),
            folderLeaf, 
            storage->id
        ) == 0
    )
        return false;

    return true;
}
    
// managing cache in this function
void DCPCALL FsStatusInfoW(WCHAR* RemoteDir, int InfoStartEnd, int InfoOperation)
{
    wcharstring wPath(RemoteDir);
    // fix for wierd issue when Double Commander sends RemoteDir as empty (basic_string)
    if(wPath.length() == 0) return; 
    std::replace(wPath.begin(), wPath.end(), u'\\', u'/');
    if(wPath.size() > 1 && wPath.substr(wPath.size() - 1) == (WCHAR*)u"/")
    {
        wPath = wPath.substr(0, wPath.size() - 1);
    }
    // get file or files
    if(InfoOperation == FS_STATUS_OP_GET_SINGLE || InfoOperation == FS_STATUS_OP_GET_MULTI)
    {
        if(InfoStartEnd == FS_STATUS_START) 
        {
            // make cache if cache not exists (skip this step otherwise)
            makeParentFolderItemsCacheIfNotExists(wPath);
        }
    }
    // put file or files
    if(InfoOperation == FS_STATUS_OP_PUT_SINGLE || InfoOperation == FS_STATUS_OP_PUT_MULTI)
    {
        if(InfoStartEnd == FS_STATUS_START) 
        {
            isPut = true;
            makeFolderItemsCache(wPath);
        }  
        if(InfoStartEnd == FS_STATUS_END) 
        {
            isPut = false;
        }
    }
    // ren move file or folder
    if(InfoOperation == FS_STATUS_OP_RENMOV_SINGLE || InfoOperation == FS_STATUS_OP_RENMOV_MULTI)
    {
        if(InfoStartEnd == FS_STATUS_START) 
        {
            // make cache if cache not exists (skip this step otherwise) [move copy from folder]
            makeParentFolderItemsCacheIfNotExists(wPath);
            busyFolders.clear(); 
        }
    }
    // delete file or folder
    if(InfoOperation == FS_STATUS_OP_DELETE)
    {
        if(InfoStartEnd == FS_STATUS_START) 
        {
            // make cache if cache not exists (skip this step otherwise)
            makeParentFolderItemsCacheIfNotExists(wPath);
        }
    }
    // mkdir
    if(InfoOperation == FS_STATUS_OP_MKDIR)
    {
        if(InfoStartEnd == FS_STATUS_START)
        {
            // make cache if cache not exists (skip this step otherwise)
            makeParentFolderItemsCacheIfNotExists(wPath);
        }
    }
}



