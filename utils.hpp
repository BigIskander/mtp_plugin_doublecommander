/*
    This is modified version of original code copied from (accessed 22 nov 2025):
    https://github.com/ivanenko/cloud_storage/blob/master/plugin_utils.h

Copyright (C) 2019 Ivanenko Danil (ivanenko.danil@gmail.com)
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

#ifndef _UTILS_HPP
#define _UTILS_HPP

#include "common.h"
#include <codecvt>
#include <locale>
#include <vector>
#include <sys/stat.h>

typedef struct {
    int nCount;
    std::vector<WIN32_FIND_DATAW> resource_array;
} tResources, *pResources;

typedef std::basic_string<WCHAR> wcharstring;

FILETIME get_now_time()
{
    time_t t2 = time(0);
    int64_t ft = (int64_t) t2 * 10000000 + 116444736000000000;
    FILETIME file_time;
    // deleted: &0xffff - it causes time shift to ~5 min
    file_time.dwLowDateTime = ft;
    file_time.dwHighDateTime = ft >> 32;
    return file_time;
}

// add function converting time_t to FILETIME
FILETIME get_file_time(time_t t)
{
    int64_t ft = (int64_t) t * 10000000 + 116444736000000000;
    FILETIME file_time;
    // 
    file_time.dwLowDateTime = ft;
    file_time.dwHighDateTime = ft >> 32;
    return file_time;
}

std::string UTF16toUTF8(const WCHAR *p)
{
    // replaced codecvt_utf8 with codecvt_utf8_utf16 to fix an conversion error
    std::wstring_convert<std::codecvt_utf8_utf16<char16_t>, char16_t> convert2;
    return convert2.to_bytes(std::u16string((char16_t*) p));
}

wcharstring UTF8toUTF16(const std::string &str)
{
    // replaced codecvt_utf8 with codecvt_utf8_utf16 to fix an conversion error
    std::wstring_convert<std::codecvt_utf8_utf16<char16_t>, char16_t> convert2;
    std::u16string utf16 = convert2.from_bytes(str);
    return wcharstring((WCHAR*)utf16.data());
}

BOOL file_exists(const std::string& filename)
{
    struct stat buf;
    return stat(filename.c_str(), &buf) == 0;
}

// add function to gel local file size
BOOL get_file_size(const std::string& filename, uint64_t& fileSize)
{
    struct stat buf;
    if(stat(filename.c_str(), &buf) != 0) return false;
    fileSize = buf.st_size;
    return true;
}

// add function to convert int to wcharstring
wcharstring int_to_wcharstring(int num) 
{
    return UTF8toUTF16(std::to_string(num));
}

#endif