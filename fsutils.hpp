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

#ifndef _FSUTILS_HPP
#define _FSUTILS_HPP

#include "fsplugin.h"
#include "utils.hpp"
#include <libmtp.h>

pResources show_error_entry(char* error) 
{
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

pResources show_devices_entry(LIBMTP_raw_device_t * rawdevices, int numrawdevices) 
{
    pResources pRes = new tResources;
    pRes->nCount = 0;
    pRes->resource_array.resize(numrawdevices);

    // list all available MTP devices
    for (int i = 0; i < numrawdevices; i++) {                    
        // make folder entry (path) name for each device
        wcharstring wName = int_to_wcharstring(i + 1).append(UTF8toUTF16("."));
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

#endif