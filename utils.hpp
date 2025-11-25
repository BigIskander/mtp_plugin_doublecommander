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

// add function to convert int to wcharstring
int wcharstring_to_int(wcharstring num) {
    int outn = 0, len = num.size(), mul = 1;
    WCHAR* wnum = (WCHAR*)num.data();
    for (int i = len - 1; i >= 0; i--)
    {        
        switch ((WCHAR)wnum[i])
        {
        case (WCHAR)u'-':
            if(i == 0) outn = -1 * outn;
            else outn = outn + 0 * mul;
            break;
        case (WCHAR)u'0':
            outn = outn + 0 * mul;
            break;
        case (WCHAR)u'1':
            outn = outn + 1 * mul;
            break;
        case (WCHAR)u'2':
            outn = outn + 2 * mul;
            break;
        case (WCHAR)u'3':
            outn = outn + 3 * mul;
            break;
        case (WCHAR)u'4':
            outn = outn + 4 * mul;
            break;
        case (WCHAR)u'5':
            outn = outn + 5 * mul;
            break;
        case (WCHAR)u'6':
            outn = outn + 6 * mul;
            break;
        case (WCHAR)u'7':
            outn = outn + 7 * mul;
            break;
        case (WCHAR)u'8':
            outn = outn + 8 * mul;
            break;
        case (WCHAR)u'9':
            outn = outn + 9 * mul;
            break;
        default:
            outn = outn + 0 * mul;
            break;
        }
        mul = mul * 10;
    }
    return outn;
}

#endif