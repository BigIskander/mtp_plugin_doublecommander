/*
MTP plugin for Double Commander

Wfx plugin for working with MTP (Media Transfer Protocol) devices

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

struct AvailableDevice
{
   LIBMTP_mtpdevice_t* device;
   wcharstring name;
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
    wcharstring originalName = UTF8toUTF16(LIBMTP_Get_Manufacturername(newDevice))
        .append((WCHAR*)u" ")
        .append(UTF8toUTF16(LIBMTP_Get_Modelname(newDevice)));
    wcharstring name = originalName;
    int i = 0;
    while (isDeviceNameTaken(name) && i < 1000)
    {
        name = originalName.append((WCHAR*)u" [")
            .append(int_to_wcharstring(i))
            .append((WCHAR*)u"]");
        i++;
    }
    if(i >= 1000) return;
    AvailableDevice aDevice;
    aDevice.device = newDevice;
    aDevice.name = name;
    availableDevices.push_back(aDevice);
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
        pRes->resource_array[i].nFileSizeLow = 0;
        pRes->resource_array[i].nFileSizeHigh = 0;
        pRes->resource_array[i].ftCreationTime = get_now_time();
        pRes->resource_array[i].ftLastWriteTime = get_now_time();
        pRes->resource_array[i].ftLastAccessTime = get_now_time();
    }
    return pRes;
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