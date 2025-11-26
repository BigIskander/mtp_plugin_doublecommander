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

#ifndef _FSUTILS_HPP
#define _FSUTILS_HPP

#include <algorithm>
#include "fsplugin.h"
#include "utils.hpp"
#include <libmtp.h>

struct PathElement
{
    wcharstring path;
    uint32_t leaf;
};

struct AvailableDevice
{
   LIBMTP_mtpdevice_t* device;
   wcharstring name;
   std::vector<PathElement> leafCache;
};

std::vector<AvailableDevice> availableDevices;

void filterConnectedDevices() {
    availableDevices.erase(std::remove_if(
        availableDevices.begin(),
        availableDevices.end(),
        [](const AvailableDevice& item)
        {
            int ret = LIBMTP_Get_Storage(item.device, LIBMTP_STORAGE_SORTBY_NOTSORTED);
            if(ret != 0) return true;
            return item.device->storage == NULL;
        }
    ), availableDevices.end());
}

bool isDeviceNameTaken(wcharstring name) {
    return std::find_if(
        availableDevices.begin(), 
        availableDevices.end(), 
        [name](const AvailableDevice& item) 
        {
            return item.name == name;
        }
    ) != availableDevices.end();
}

void addDevice(LIBMTP_mtpdevice_t* newDevice) {
    if(newDevice == NULL) return;
    int ret = LIBMTP_Get_Storage(newDevice, LIBMTP_STORAGE_SORTBY_NOTSORTED);
    if(ret != 0) return;
    if(newDevice->storage == NULL) return;
    char* manufacturer = LIBMTP_Get_Manufacturername(newDevice);
    char* model = LIBMTP_Get_Modelname(newDevice);
    wcharstring originalName = UTF8toUTF16(manufacturer)
        .append((WCHAR*)u" ")
        .append(UTF8toUTF16(model));
    LIBMTP_FreeMemory(manufacturer);
    LIBMTP_FreeMemory(model);
    wcharstring name = wcharstring((WCHAR*)u"").append(originalName);
    int i = 0;
    while (isDeviceNameTaken(name) && i < 1000)
    {
        name.clear();
        name = wcharstring((WCHAR*)u"").append(originalName)
            .append((WCHAR*)u" [")
            .append(int_to_wcharstring(i))
            .append((WCHAR*)u"]");
        i++;
    }
    originalName.clear();
    if(i >= 1000) return;
    AvailableDevice aDevice;
    aDevice.device = newDevice;
    aDevice.name = name;
    availableDevices.push_back(aDevice);
    // gLogProc(gPluginNumber, MSGTYPE_CONNECT, (WCHAR*)UTF8toUTF16("CONNECT /").append(name).data());
}

LIBMTP_mtpdevice_t* getDevice(wcharstring name) 
{
    auto it = std::find_if(
        availableDevices.begin(), 
        availableDevices.end(), 
        [name](const AvailableDevice& item) 
        {
            return item.name == name;
        }
    );
    if(it == availableDevices.end()) return NULL;
    return (*it).device;
}

LIBMTP_devicestorage_t* getStorage(LIBMTP_mtpdevice_t* device, wcharstring storageName) 
{
    if(device == NULL) return NULL;
    LIBMTP_devicestorage_t* storage;
    int ret = LIBMTP_Get_Storage(device, LIBMTP_STORAGE_SORTBY_NOTSORTED);
    if(ret != 0) return NULL;
    storage = device->storage;
    if (storage == NULL) return NULL;
    while (storage != NULL)
    {
        if(UTF8toUTF16(storage->StorageDescription) == storageName) return storage;
        storage = storage->next;
    }
    return NULL;
}

void getTopFolderName(wcharstring wPath, wcharstring& topFolderName, wcharstring& subPath) {
    if(wPath.size() <= 1 || wPath.substr(0, 1).data() != UTF8toUTF16("/"))
    {
        topFolderName = (WCHAR*)u"";
        subPath = (WCHAR*)u"";
    } else {
        int nPos = wPath.find((WCHAR*)u"/", 1);
        if(nPos == std::string::npos) 
        {
            topFolderName = wPath.substr(1);
            subPath = (WCHAR*)u"";
        } else {
            topFolderName = wPath.substr(1, nPos - 1);
            subPath = wPath.substr(nPos);
        }
    }
}

void addLeafToCache(LIBMTP_mtpdevice_t* device, wcharstring path, uint32_t leaf) {
    auto it = std::find_if(
        availableDevices.begin(), 
        availableDevices.end(), 
        [device](const AvailableDevice& item) 
        {
            return item.device == device;
        }
    );
    if(it == availableDevices.end()) return; // not found such device
    PathElement cache;
    cache.path = path;
    cache.leaf = leaf;
    (*it).leafCache.push_back(cache);
}

void removeLeafsFromCache(LIBMTP_mtpdevice_t* device, wcharstring path) {
    auto it = std::find_if(
        availableDevices.begin(), 
        availableDevices.end(), 
        [device](const AvailableDevice& item) 
        {
            return item.device == device;
        }
    );
    if(it == availableDevices.end()) return; // not found such device
    auto leafCache = (*it).leafCache;
    leafCache.erase(std::remove_if(
        leafCache.begin(),
        leafCache.end(),
        [path](const PathElement& item)
        {
            return item.path.substr(0, path.size()) == path;
        }
    ), leafCache.end());
}

bool getLeafFromCache(LIBMTP_mtpdevice_t* device, wcharstring path, uint32_t& leaf) {
    if(device == NULL) return false;
    if(path.substr(0, 1) != (WCHAR*)u"/") return false;
    auto it = std::find_if(
        availableDevices.begin(), 
        availableDevices.end(), 
        [device](const AvailableDevice& item) 
        {
            return item.device == device;
        }
    );
    if(it == availableDevices.end()) return false; // not found such device
    auto cache = (*it).leafCache;
    auto cacheItem = std::find_if(
        cache.begin(), 
        cache.end(), 
        [path](const PathElement& item) 
        {
            return item.path == path;
        }
    );
    if(cacheItem == cache.end()) return false; // path not cached
    leaf = (*cacheItem).leaf;
    // check if path exists and name matches
    LIBMTP_file_t *file = LIBMTP_Get_Filemetadata(device, leaf);
    if(file == NULL) return false;
    if(file->filetype != LIBMTP_FILETYPE_FOLDER) {
        LIBMTP_destroy_file_t(file);
        removeLeafsFromCache(device, path);
        return false;
    }
    wcharstring folderName;
    size_t nPos = path.find_last_of((WCHAR*)u"/");
    if(path.substr(nPos + 1) != UTF8toUTF16(file->filename)) 
    {
        LIBMTP_destroy_file_t(file);
        removeLeafsFromCache(device, path);
        return false;
    }
    LIBMTP_destroy_file_t(file);
    return true;
}

bool getPathLeaf(
    LIBMTP_mtpdevice_t* device, 
    LIBMTP_devicestorage_t* storage, 
    wcharstring internalPath, 
    uint32_t& leaf
)
{
    if(device == NULL || storage == NULL) return false; // no device or no storage specified
    if(internalPath.substr(0, 1).data() != UTF8toUTF16("/")) return false; // error incorrect path
    leaf = LIBMTP_FILES_AND_FOLDERS_ROOT;
    if(internalPath.size() == 1) return true; // root folder
    wcharstring path = (WCHAR*)u"", topFolderName, subFolder = internalPath;
    LIBMTP_file_t *files, *tmp;
    while (path != internalPath && path.size() <= internalPath.size())
    {
        files = LIBMTP_Get_Files_And_Folders(device, storage->id, leaf);
        getTopFolderName(subFolder, topFolderName, subFolder);
        while(files != NULL) 
        {
            if(topFolderName == UTF8toUTF16(files->filename)) 
            {
                if(files->filetype != LIBMTP_FILETYPE_FOLDER) 
                {
                    LIBMTP_destroy_file_t(files);
                    return false; // error it is not folder
                }
                path = path.append((WCHAR*)u"/").append(topFolderName);
                leaf = files->item_id;
                if(path == internalPath) 
                {
                    LIBMTP_destroy_file_t(files);
                    return true; // leaf found
                }
                subFolder = subFolder;
                break;
            }
            tmp = files;
            files = files->next;
            LIBMTP_destroy_file_t(tmp);
        }
    }
    return false; // error folder does not exists
}

pResources show_error_entry(wcharstring error) {
    pResources pRes = new tResources;
    pRes->nCount = 0;
    pRes->resource_array.resize(1);

    wcharstring wName = error;
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

pResources show_error_entry(char* error) 
{
    return show_error_entry(UTF8toUTF16(error));
}

pResources showDevices() 
{
    int devicesCount = availableDevices.size();
    pResources pRes = new tResources;
    pRes->nCount = 0;
    pRes->resource_array.resize(devicesCount);
    
    size_t str_size;
    for(int i = 0; i < devicesCount; i++) 
    {
        str_size = (MAX_PATH > availableDevices[i].name.size()+1)? (availableDevices[i].name.size()+1): MAX_PATH;
        memcpy(pRes->resource_array[i].cFileName, availableDevices[i].name.data(), str_size * sizeof(WCHAR));
        pRes->resource_array[i].dwFileAttributes = FILE_ATTRIBUTE_DIRECTORY;
    }
    return pRes;
}

pResources showStorages(LIBMTP_mtpdevice_t* device) {
    if(device == NULL) return NULL;
    int ret = LIBMTP_Get_Storage(device, LIBMTP_STORAGE_SORTBY_NOTSORTED);
    if(ret != 0) return NULL;
    if(device->storage == NULL) return NULL;

    LIBMTP_devicestorage_t *storage;
    int numOfStorages = 0;
    for (storage = device->storage; storage != 0; storage = storage->next) {
        numOfStorages++;
    }
    if(numOfStorages == 0) return NULL;
    pResources pRes = new tResources;
    pRes->nCount = 0;
    pRes->resource_array.resize(numOfStorages);
    wcharstring wName;
    storage = device->storage;
    for(int i = 0; i < numOfStorages; i++) {
        wName = UTF8toUTF16(storage->StorageDescription);
        size_t str_size = (MAX_PATH > wName.size()+1)? (wName.size()+1): MAX_PATH;
        memcpy(pRes->resource_array[i].cFileName, wName.data(), sizeof(WCHAR) * str_size);
        pRes->resource_array[i].dwFileAttributes = FILE_ATTRIBUTE_DIRECTORY;
        storage = storage->next;
    }
    return pRes;
}

pResources showFilesAndFolders(LIBMTP_mtpdevice_t* device, LIBMTP_devicestorage_t* storage, uint32_t leaf) 
{
    if(device == NULL || storage == NULL) return NULL;
    LIBMTP_file_t *files, *tmp;
    files = LIBMTP_Get_Files_And_Folders(device, storage->id, leaf);
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
        if(numOfEntries == 0) return NULL;
        pResources pRes = new tResources;
        pRes->nCount = 0;
        pRes->resource_array.resize(numOfEntries);
                                
        file = files;
        size_t str_size;
        for (int i = 0; i < numOfEntries; i++)
        {
            str_size = (MAX_PATH > UTF8toUTF16(file->filename).size()+1)? (UTF8toUTF16(file->filename).size()+1): MAX_PATH;
            memcpy(pRes->resource_array[i].cFileName, UTF8toUTF16(file->filename).data(), str_size * sizeof(WCHAR));
            if(file->filetype == LIBMTP_FILETYPE_FOLDER) pRes->resource_array[i].dwFileAttributes = FILE_ATTRIBUTE_DIRECTORY;
            // convert from uint64_t to DWORD, ai answear, "uint64_t to DWORD" search query in google.com
            pRes->resource_array[i].nFileSizeLow = static_cast<DWORD>(file->filesize & 0xFFFFFFFF);
            pRes->resource_array[i].nFileSizeHigh = static_cast<DWORD>(file->filesize >> 32);
            pRes->resource_array[i].ftLastWriteTime = get_file_time(file->modificationdate);
            tmp = file;
            file = file->next;
            LIBMTP_destroy_file_t(tmp);
        }
        return pRes;
    }
    return NULL;
}

void parsePath(wcharstring wPath, wcharstring& deviceName, wcharstring& storageName, wcharstring& internalPath)
{
    if(wPath.length() == 1) 
    {
        deviceName = (WCHAR*)u"";
        storageName = (WCHAR*)u"";
        internalPath = (WCHAR*)u"";
        return;
    }

    size_t nPos = wPath.find((WCHAR*)u"/", 1);
    if(nPos == std::string::npos) 
    {
        deviceName = wPath.substr(1);
        storageName = (WCHAR*)u"";
        internalPath = (WCHAR*)u"";
        return;
    }
    deviceName = wPath.substr(1, nPos - 1);

    size_t nPos2 = wPath.find((WCHAR*)u"/", nPos + 1);
    if(nPos2 == std::string::npos) {
        storageName = wPath.substr(nPos + 1);
        internalPath = (WCHAR*)u"/";
        return;
    }
    storageName = wPath.substr(nPos + 1, nPos2 - nPos - 1);
    internalPath = wPath.substr(nPos2);
}

#endif