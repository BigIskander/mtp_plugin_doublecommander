/*
    This is modified version of original code copied from (accessed 22 nov 2025):
    https://github.com/ivanenko/cloud_storage/blob/master/plugin_utils.h

Copyright (C) 2019 Ivanenko Danil (ivanenko.danil@gmail.com)
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

#ifndef _UTILS_HPP
#define _UTILS_HPP

#include "common.h"
#include <codecvt>
#include <vector>

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
    file_time.dwLowDateTime = ft & 0xffff;
    file_time.dwHighDateTime = ft >> 32;
    return file_time;
}

wcharstring UTF8toUTF16(const std::string &str)
{
    std::wstring_convert<std::codecvt_utf8<char16_t>, char16_t> convert2;
    std::u16string utf16 = convert2.from_bytes(str);
    return wcharstring((WCHAR*)utf16.data());
}

// add function to convert int to wcharstring
wcharstring int_to_wcharstring(int num) 
{
    int i = 1, n = abs(num) / 10, curnum, s = 0;
    while (n > 0)
    {
        n = n / 10;
        i++;
    }
    n = abs(num);
    char* numchar;
    if(num < 0)
    {
        numchar = new char[i + 1];
        numchar[0] = '-';
        s = 1;
    } else {
        numchar = new char[i];
    }
    for(int j = 0; j < i; j++) {
        curnum = n % 10;
        n = n / 10;
        switch (curnum)
        {
            case 0:
                numchar[i - j - 1 + s] = '0';
                break;
            case 1:
                numchar[i - j - 1 + s] = '1';
                break;
            case 2:
                numchar[i - j - 1 + s] = '2';
                break;
            case 3:
                numchar[i - j - 1 + s] = '3';
                break;
            case 4:
                numchar[i - j - 1 + s] = '4';
                break;
            case 5:
                numchar[i - j - 1 + s] = '5';
                break;
            case 6:
                numchar[i - j - 1 + s] = '6';
                break;
            case 7:
                numchar[i - j - 1 + s] = '7';
                break;
            case 8:
                numchar[i - j - 1 + s] = '8';
                break;
            case 9:
                numchar[i - j - 1 + s] = '9';
                break;
        }
    }
    wcharstring output = UTF8toUTF16(numchar);
    delete numchar;
    return output;
}

#endif