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
// #include <chrono>

// Doublecommander API callback functions
int gPluginNumber;
tProgressProcW gProgressProc = NULL;
tLogProcW gLogProc = NULL;
tRequestProcW gRequestProc = NULL;
tCryptProcW gCryptProc = NULL;

struct PathElement
{
    wcharstring path;
    uint32_t leaf;
};

struct PathFolderElement
{
    wcharstring path;
    uint32_t leaf;
    std::vector<PathElement> elementsCache;
};

struct AvailableDevice
{
   LIBMTP_mtpdevice_t* device;
   wcharstring name;
   std::vector<PathFolderElement> leafCache;
};

std::vector<AvailableDevice> availableDevices;

std::vector<wcharstring> busyFolders;

void parsePath(
    wcharstring wPath, 
    wcharstring& deviceName, 
    wcharstring& storageName, 
    wcharstring& internalPath
);
void getFolderPath(wcharstring wPath, wcharstring& folderPath);
void getFileName(wcharstring wPath, wcharstring& fileName);
bool getPathLeaf(
    LIBMTP_mtpdevice_t* device, 
    LIBMTP_devicestorage_t* storage, 
    wcharstring deviceName,
    wcharstring storageName,
    wcharstring internalPath, 
    uint32_t& leaf,
    bool isFolder = true
);

void filterConnectedDevices() {
    availableDevices.erase(std::remove_if(
        availableDevices.begin(),
        availableDevices.end(),
        [](const AvailableDevice& item)
        {
            int ret = LIBMTP_Get_Storage(item.device, LIBMTP_STORAGE_SORTBY_NOTSORTED);
            if(ret != 0) 
            {
                LIBMTP_FreeMemory(item.device);
                return true;
            }
            if(item.device->storage == NULL)
            {
                LIBMTP_FreeMemory(item.device);
                return true;
            }
            return false;
        }
    ), availableDevices.end());
}

bool isDeviceNameTaken(wcharstring name) {
    if(name.length() == 0) return true;
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
    if(ret != 0) 
    {
        LIBMTP_FreeMemory(newDevice);
        return;
    }
    if(newDevice->storage == NULL)
    {
        LIBMTP_FreeMemory(newDevice);
        return;
    } 
    char* friendlyName = LIBMTP_Get_Friendlyname(newDevice);
    wcharstring originalName = (WCHAR*)u"";
    if(friendlyName != NULL) 
    {
        originalName = UTF8toUTF16(friendlyName);
        LIBMTP_FreeMemory(friendlyName);
    }
    if(originalName == (WCHAR*)u"")
    {
        char* manufacturer = LIBMTP_Get_Manufacturername(newDevice);
        if(manufacturer != NULL)
        {
            originalName = originalName.append(UTF8toUTF16(manufacturer));
            LIBMTP_FreeMemory(manufacturer);
        }
        originalName = originalName.append((WCHAR*)u" ");
        char* model = LIBMTP_Get_Modelname(newDevice);
        if(model != NULL)
        {
            originalName = originalName.append(UTF8toUTF16(model));
            LIBMTP_FreeMemory(model);
        }
    }
    // trick to copy value not reference
    wcharstring name = wcharstring((WCHAR*)u"").append(originalName); 
    // no slashes in device name
    std::replace(name.begin(), name.end(), u'\\', u'/');
    std::replace(name.begin(), name.end(), u'/', u' ');
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
}

LIBMTP_mtpdevice_t* getDevice(wcharstring name) 
{
    if(name.length() == 0) return NULL;
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
    if(storageName.length() == 0) return NULL;
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
    if(device == NULL) return;
    if(path.length() == 0) return;
    if(path.substr(0, 1) != (WCHAR*)u"/") return;
    if(path.size() == 1) return;
    auto it = std::find_if(
        availableDevices.begin(), 
        availableDevices.end(), 
        [device](const AvailableDevice& item) 
        {
            return item.device == device;
        }
    );
    if(it == availableDevices.end()) return; // not found such device
    PathFolderElement cache;
    cache.path = path;
    cache.leaf = leaf;
    (*it).leafCache.push_back(cache);
}

void removeLeafsFromCache(LIBMTP_mtpdevice_t* device, wcharstring rPath) {
    if(device == NULL) return;
    if(rPath.length() == 0) return;
    auto it = std::find_if(
        availableDevices.begin(), 
        availableDevices.end(), 
        [device](const AvailableDevice& item) 
        {
            return item.device == device;
        }
    );
    if(it == availableDevices.end()) return; // not found such device
    auto leafCache = &(*it).leafCache;
    leafCache->erase(std::remove_if(
        leafCache->begin(),
        leafCache->end(),
        [rPath](const PathFolderElement& item)
        {
            return item.path.substr(0, rPath.size()) == rPath;
        }
    ), leafCache->end());
}

bool getLeafFromCache(LIBMTP_mtpdevice_t* device, wcharstring path, uint32_t& leaf) {
    if(device == NULL) return false;
    if(path.length() == 0) return false;
    if(path.substr(0, 1) != (WCHAR*)u"/") return false;
    if(path.size() <= 1) return false;
    auto it = std::find_if(
        availableDevices.begin(), 
        availableDevices.end(), 
        [device](const AvailableDevice& item) 
        {
            return item.device == device;
        }
    );
    if(it == availableDevices.end()) return false; // not found such device
    auto cache = &(*it).leafCache;
    auto cacheItem = std::find_if(
        cache->begin(), 
        cache->end(), 
        [path](const PathFolderElement& item) 
        {
            return item.path == path;
        }
    );
    if(cacheItem == cache->end()) return false; // path not cached
    leaf = (*cacheItem).leaf;
    // check if path exists and name matches
    LIBMTP_file_t *file = LIBMTP_Get_Filemetadata(device, leaf);
    if(file == NULL) return false;
    if(file->filetype != LIBMTP_FILETYPE_FOLDER) {
        LIBMTP_destroy_file_t(file);
        removeLeafsFromCache(device, path);
        return false;
    }
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

PathFolderElement* getPathFolderElement(
    LIBMTP_mtpdevice_t* device, 
    wcharstring path
) {
    if(device == NULL) return NULL;
    if(path.length() == 0) return NULL;
    // find device in available devices list
    auto it = std::find_if(
        availableDevices.begin(), 
        availableDevices.end(), 
        [device](const AvailableDevice& item) 
        {
            return item.device == device;
        }
    );
    if(it == availableDevices.end()) return NULL;
    // find path cache element
    auto caches = &(*it).leafCache; // does it makes copy here
    auto cache = std::find_if(
        caches->begin(),
        caches->end(),
        [path](const PathFolderElement& item)
        {
            return item.path == path;
        }
    );
    if(cache == caches->end()) return NULL;
    // return cache if found
    return &(*cache);
}

bool getLeafFromcachedFolderItems(
    std::vector<PathElement> cachedFolderItems,
    wcharstring itemName,
    uint32_t& leaf
) {
    if(cachedFolderItems.size() == 0) return false;
    if(itemName.length() == 0) return false;
    auto el = std::find_if(
        cachedFolderItems.begin(),
        cachedFolderItems.end(),
        [itemName](const PathElement& item)
        {
            return item.path == itemName;
        }
    );
    if(el == cachedFolderItems.end()) return false;
    leaf = (*el).leaf;
    return true;
}

void makeParentFolderItemsCacheIfNotExists(wcharstring path) {
    if(path.length() == 0) return;
    wcharstring deviceName, storageName, internalPath;
    parsePath(path, deviceName, storageName, internalPath);

    LIBMTP_mtpdevice_t* device = getDevice(deviceName);
    if(device == NULL) return;

    LIBMTP_devicestorage_t* storage = getStorage(device, storageName);
    if(storage == NULL) return;

    uint32_t leaf;
    // check if cache already exists
    PathFolderElement* pathCache = getPathFolderElement(device, path);
    if(pathCache == NULL) 
    { 
        if(getPathLeaf(device, storage, deviceName, storageName, internalPath, leaf))
        {
            pathCache = getPathFolderElement(device, path);
        }
    }
    if(pathCache == NULL) return;
    // check if folder content is cached
    if(pathCache->elementsCache.size() == 0)
    {
        // if not cached, make cache
        LIBMTP_file_t *file = LIBMTP_Get_Files_And_Folders(device, storage->id, pathCache->leaf);
        if (file != NULL) 
        {
            LIBMTP_file_t *tmp;
            int numOfEntries = 0;
            PathElement pathE;
            while (file != NULL) 
            {
                // add item to cache
                pathE.leaf = file->item_id;
                pathE.path = UTF8toUTF16(file->filename);
                pathCache->elementsCache.push_back(pathE);
                tmp = file;
                file = file->next;
                LIBMTP_destroy_file_t(tmp);
            }
        }
    }
}

void makeFolderItemsCache(wcharstring folderPath) {
    if(folderPath.length() == 0) return;
    wcharstring deviceName, storageName, internalPath;
    parsePath(folderPath, deviceName, storageName, internalPath);

    LIBMTP_mtpdevice_t* device = getDevice(deviceName);
    if(device == NULL) return;

    LIBMTP_devicestorage_t* storage = getStorage(device, storageName);
    if(storage == NULL) return;

    // check if cache already exists
    PathFolderElement* pathCache = getPathFolderElement(device, folderPath);
    uint32_t leaf;
    if(pathCache == NULL) 
    { 
        if(getPathLeaf(device, storage, deviceName, storageName, internalPath, leaf))
        {
            pathCache = getPathFolderElement(device, folderPath);
        }
    }
    if(pathCache == NULL) return;

    // clear and make new cache
    pathCache->elementsCache.clear();
    LIBMTP_file_t *file = LIBMTP_Get_Files_And_Folders(device, storage->id, pathCache->leaf);
    if (file != NULL) 
    {
        LIBMTP_file_t *tmp;
        int numOfEntries = 0;
        PathElement pathE;
        while (file != NULL) 
        {
            // add item to cache
            pathE.leaf = file->item_id;
            pathE.path = UTF8toUTF16(file->filename);
            pathCache->elementsCache.push_back(pathE);
            tmp = file;
            file = file->next;
            LIBMTP_destroy_file_t(tmp);
        }
    }
}

bool getLeafFromCachedFolder(
    LIBMTP_mtpdevice_t* device,
    wcharstring path,
    uint32_t& leaf
) {
    if(device == NULL) return false;
    if(path.length() == 0) return false;
    wcharstring folderPath, fileName;
    getFolderPath(path, folderPath);
    getFileName(path, fileName);
    // get leaf from cache
    bool isLeafFound = false;
    // search and get leaf from cache (for speed)
    PathFolderElement *cachedFolderItems = getPathFolderElement(device, folderPath);
    if(cachedFolderItems != NULL)
    {
        if(getLeafFromcachedFolderItems(cachedFolderItems->elementsCache, fileName, leaf))
        { 
            LIBMTP_file_t *file = LIBMTP_Get_Filemetadata(device, leaf);
            if(file != NULL) 
            {
                if(file->parent_id == cachedFolderItems->leaf) isLeafFound = true;
            }
            LIBMTP_destroy_file_t(file);
        }
    }
    return isLeafFound;
}

bool getPathLeaf(
    LIBMTP_mtpdevice_t* device, 
    LIBMTP_devicestorage_t* storage, 
    wcharstring deviceName,
    wcharstring storageName,
    wcharstring internalPath, 
    uint32_t& leaf,
    bool isFolder
)
{
    if(device == NULL || storage == NULL) return false; // no device or no storage specified
    if(deviceName.length() == 0 || storageName.length() == 0 || internalPath.length() == 0)
        return false;
    if(internalPath.substr(0, 1).data() != UTF8toUTF16("/")) return false; // error incorrect path
    leaf = LIBMTP_FILES_AND_FOLDERS_ROOT; // is -1
    uint32_t tmpLeaf;
    wcharstring fullPath =  wcharstring((WCHAR*)u"/")
                                .append(deviceName)
                                .append((WCHAR*)u"/")
                                .append(storageName);
    if(internalPath.size() == 1) {
        // special case root folder of storage
        if(!getLeafFromCache(device, fullPath, tmpLeaf))
            // parent id of files and folder in root is 0 for some reason
            addLeafToCache(device, fullPath, 0); 
        return true;
    }
    wcharstring path = (WCHAR*)u"", topFolderName, subFolder = internalPath;
    LIBMTP_file_t *files = NULL, *tmp;
    bool match;
    while (path != internalPath && path.size() <= internalPath.size())
    {
        match = false;
        getTopFolderName(subFolder, topFolderName, subFolder);
        path = path.append((WCHAR*)u"/").append(topFolderName);
        fullPath = fullPath.append((WCHAR*)u"/").append(topFolderName);
        if(isFolder == true || (isFolder == false && subFolder != (WCHAR*)u""))
        {
            // check cache
            if(getLeafFromCache(device, fullPath, tmpLeaf))
            {
                leaf = tmpLeaf;
                if(path == internalPath) 
                {
                    if(files != NULL) LIBMTP_destroy_file_t(files);
                    return true;
                }
                continue;
            }
        }
        // check files and folders in device if cache not found
        files = LIBMTP_Get_Files_And_Folders(device, storage->id, leaf);
        while(files != NULL) 
        {
            if(topFolderName == UTF8toUTF16(files->filename)) 
            {
                match = true;
                if(files->filetype != LIBMTP_FILETYPE_FOLDER) 
                {
                    if(isFolder == false)
                    {
                        if(path == internalPath)
                        {
                            leaf = files->item_id;
                            LIBMTP_destroy_file_t(files);
                            return true;
                        }
                    }
                    LIBMTP_destroy_file_t(files);
                    // error it is not folder and not file we are looking for
                    return false; 
                }
                leaf = files->item_id;
                addLeafToCache(device, fullPath, leaf);
                if(path == internalPath) 
                {
                    LIBMTP_destroy_file_t(files);
                    return true; // leaf found
                }
                subFolder = subFolder;
                LIBMTP_destroy_file_t(files);
                break;
            }
            tmp = files;
            files = files->next;
            LIBMTP_destroy_file_t(tmp);
        }
        if(!match) return false;
    }
    return false; // error folder does not exists
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
        str_size = (MAX_PATH > availableDevices[i].name.size()+1) ? 
            (availableDevices[i].name.size()+1): MAX_PATH;
        memcpy(
            pRes->resource_array[i].cFileName, 
            availableDevices[i].name.data(), 
            str_size * sizeof(WCHAR)
        );
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
    size_t str_size;
    storage = device->storage;
    for(int i = 0; i < numOfStorages; i++) {
        wName = UTF8toUTF16(storage->StorageDescription);
        str_size = (MAX_PATH > wName.size()+1)? (wName.size()+1): MAX_PATH;
        memcpy(pRes->resource_array[i].cFileName, wName.data(), sizeof(WCHAR) * str_size);
        pRes->resource_array[i].dwFileAttributes = FILE_ATTRIBUTE_DIRECTORY;
        storage = storage->next;
    }
    return pRes;
}

pResources showFilesAndFolders(
    LIBMTP_mtpdevice_t* device, 
    LIBMTP_devicestorage_t* storage, 
    wcharstring folderPath,
    uint32_t leaf
) 
{
    // gLogProc(
    //     gPluginNumber, 
    //     MSGTYPE_CONNECT, 
    //     (WCHAR*)folderPath.data()
    // );
    if(device == NULL || storage == NULL) return NULL;
    if(folderPath.length() == 0) return NULL;

    // get and clear cache
    PathFolderElement *cachedFolderItems = getPathFolderElement(device, folderPath);
    if(cachedFolderItems != NULL) cachedFolderItems->elementsCache.clear();

    // get list of files and folders in folder
    LIBMTP_file_t *files, *tmp;
    files = LIBMTP_Get_Files_And_Folders(device, storage->id, leaf);
    if (files != NULL) 
    {
        // auto t1 = std::chrono::high_resolution_clock::now();
        LIBMTP_file_t *file;
        file = files;
        int numOfEntries = 0;
        PathElement pathE;
        while (file != NULL) 
        {
            if(cachedFolderItems != NULL)
            {
                // add item to cache
                pathE.leaf = file->item_id;
                pathE.path = UTF8toUTF16(file->filename);
                cachedFolderItems->elementsCache.push_back(pathE);
            }
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
            str_size = (MAX_PATH > UTF8toUTF16(file->filename).size()+1)? 
                (UTF8toUTF16(file->filename).size()+1): MAX_PATH;
            memcpy(
                pRes->resource_array[i].cFileName, 
                UTF8toUTF16(file->filename).data(), 
                str_size * sizeof(WCHAR)
            );
            if(file->filetype == LIBMTP_FILETYPE_FOLDER) 
                pRes->resource_array[i].dwFileAttributes = FILE_ATTRIBUTE_DIRECTORY;
            // convert from uint64_t to DWORD, 
            // ai answear, "uint64_t to DWORD" search query in google.com
            pRes->resource_array[i].nFileSizeLow = static_cast<DWORD>(file->filesize & 0xFFFFFFFF);
            pRes->resource_array[i].nFileSizeHigh = static_cast<DWORD>(file->filesize >> 32);
            pRes->resource_array[i].ftLastWriteTime = get_file_time(file->modificationdate);
            tmp = file;
            file = file->next;
            LIBMTP_destroy_file_t(tmp);
        }
        // auto t2 = std::chrono::high_resolution_clock::now();
        // int ms_int = std::chrono::duration_cast<std::chrono::milliseconds>(t2 - t1).count();
        // gLogProc(
        //     gPluginNumber, 
        //     MSGTYPE_CONNECT, 
        //     (WCHAR*)int_to_wcharstring(ms_int).data()
        // );
        return pRes;
    }
    return NULL;
}

void parsePath(
    wcharstring wPath, 
    wcharstring& deviceName, 
    wcharstring& storageName, 
    wcharstring& internalPath
)
{
    if(wPath.length() <= 1) // fix for wierd issue
    {
        deviceName = (WCHAR*)u"";
        storageName = (WCHAR*)u"";
        internalPath = (WCHAR*)u"";
        return;
    }

    // try
    // {

    // gLogProc(
    //     gPluginNumber, 
    //     MSGTYPE_IMPORTANTERROR, 
    //     (WCHAR*)int_to_wcharstring(wPath.length()).data()
    // );
    //     /* code */
    // size_t nPos = wPath.find((WCHAR*)u"/", 1);
    // if(nPos == std::string::npos) 
    // {
    //     deviceName = wPath.substr(1);
    //     storageName = (WCHAR*)u"";
    //     internalPath = (WCHAR*)u"";
    //     return;
    // }
    // deviceName = wPath.substr(1, nPos - 1);
    
    // }
    // catch(const std::exception& e)
    // {
    //     gLogProc(
    //         gPluginNumber, 
    //         MSGTYPE_IMPORTANTERROR, 
    //         (WCHAR*)UTF8toUTF16(e.what()).data()
    //     );
    //     return;
    // }

    // /* code */
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

void getFolderPath(wcharstring wPath, wcharstring& folderPath)
{
    if(wPath.length() == 0) {
        folderPath = (WCHAR*)u"/";
        return;
    }
    size_t nPos = wPath.find_last_of((WCHAR*)u"/");
    folderPath = wPath.substr(0, nPos);
    if(folderPath == (WCHAR*)u"") folderPath = (WCHAR*)u"/";
}

void getFileName(wcharstring wPath, wcharstring& fileName)
{
    if(wPath.length() == 0) {
        fileName = (WCHAR*)u"";
        return;
    }
    size_t nPos = wPath.find_last_of((WCHAR*)u"/");
    fileName = wPath.substr(nPos + 1);
}

void addBusyFolder(wcharstring folder) 
{
    if(folder.length() == 0) return;
    busyFolders.push_back(folder);
}

bool isFolderBusy(wcharstring folder)
{
    if(folder.length() == 0) return false;
    auto it = std::find_if(
        busyFolders.begin(), 
        busyFolders.end(), 
        [folder](const wcharstring& item) 
        {
            return item == folder;
        }
    );
    if(it == busyFolders.end()) return false;
    return true;
}

struct copyFromTo {
    WCHAR* From;
    WCHAR* To;
};

// progress function
int progressFunc(const uint64_t sent, const uint64_t total, const void *pData)
{
    gProgressProc(
        gPluginNumber, 
        static_cast<const copyFromTo*>(pData)->From, 
        static_cast<const copyFromTo*>(pData)->To, 
        (sent*100)/total
    );
    return 0;
}

#endif