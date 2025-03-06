/*
 * Windows XP Compatibility Layer
 *
 * Copyright (c) 2025 Gianluigi Tiesi <sherpya@gmail.com>
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

#include "winxp_compat.h"

#include <shlobj.h>
#include <objbase.h>
#include <strsafe.h>

#ifndef KF_FLAG_CREATE
#define KF_FLAG_CREATE 0x00008000
#endif

#ifndef KF_FLAG_DONT_VERIFY
#define KF_FLAG_DONT_VERIFY 0x00004000
#endif

#ifndef KF_FLAG_DONT_UNEXPAND
#define KF_FLAG_DONT_UNEXPAND 0x00002000
#endif

#ifndef KF_FLAG_NO_ALIAS
#define KF_FLAG_NO_ALIAS 0x00001000
#endif

#ifndef KF_FLAG_INIT
#define KF_FLAG_INIT 0x00000800
#endif

#ifndef KF_FLAG_DEFAULT_PATH
#define KF_FLAG_DEFAULT_PATH 0x00000400
#endif

#ifndef KF_FLAG_NOT_PARENT_RELATIVE
#define KF_FLAG_NOT_PARENT_RELATIVE 0x00000200
#endif

#ifndef KF_FLAG_SIMPLE_IDLIST
#define KF_FLAG_SIMPLE_IDLIST 0x00000100
#endif

typedef struct _KnownFolderMapping
{
    const GUID *rfid;
    int csidl;
} KnownFolderMapping;

STDAPI SHGetKnownFolderPath(REFKNOWNFOLDERID rfid, DWORD dwFlags, HANDLE hToken, PWSTR *ppszPath)
{
    if (!rfid)
    {
        TRACE(L"SHGetKnownFolderPath(NULL, %d, 0x%p, 0x%p)\n", dwFlags, hToken, ppszPath);
        return E_INVALIDARG;
    }

    TRACE(L"SHGetKnownFolderPath({%08lX-%04hX-%04hX-%02hhX%02hhX-%02hhX%02hhX%02hhX%02hhX%02hhX%02hhX}, %d, 0x%p, 0x%p)\n",
          rfid->Data1, rfid->Data2, rfid->Data3,
          rfid->Data4[0], rfid->Data4[1], rfid->Data4[2], rfid->Data4[3],
          rfid->Data4[4], rfid->Data4[5], rfid->Data4[6], rfid->Data4[7],
          dwFlags,
          hToken,
          ppszPath);

    if (ppszPath == NULL)
        return E_INVALIDARG;

    *ppszPath = NULL;

    // Mapping table: maps Known Folder GUIDs to their CSIDL equivalents
    static const KnownFolderMapping mapping[] = {
        {&FOLDERID_Desktop, CSIDL_DESKTOP},
        {&FOLDERID_Documents, CSIDL_PERSONAL},
        {&FOLDERID_Profile, CSIDL_PROFILE},
        {&FOLDERID_Programs, CSIDL_PROGRAMS},
        {&FOLDERID_StartMenu, CSIDL_STARTMENU},
        {&FOLDERID_Startup, CSIDL_STARTUP},
        {&FOLDERID_Recent, CSIDL_RECENT},
        {&FOLDERID_SendTo, CSIDL_SENDTO},
        {&FOLDERID_Templates, CSIDL_TEMPLATES},
        {&FOLDERID_Favorites, CSIDL_FAVORITES},
        {&FOLDERID_NetHood, CSIDL_NETHOOD},
        {&FOLDERID_PrintHood, CSIDL_PRINTHOOD},
        {&FOLDERID_History, CSIDL_HISTORY},
        {&FOLDERID_Cookies, CSIDL_COOKIES},
        {&FOLDERID_InternetCache, CSIDL_INTERNET_CACHE},
        {&FOLDERID_LocalAppData, CSIDL_LOCAL_APPDATA},
        {&FOLDERID_RoamingAppData, CSIDL_APPDATA},
        {&FOLDERID_ProgramData, CSIDL_COMMON_APPDATA},
        {&FOLDERID_Windows, CSIDL_WINDOWS},
        {&FOLDERID_System, CSIDL_SYSTEM},
        {&FOLDERID_ProgramFiles, CSIDL_PROGRAM_FILES},
        {&FOLDERID_ProgramFilesX86, CSIDL_PROGRAM_FILESX86},
        {&FOLDERID_ProgramFilesCommon, CSIDL_PROGRAM_FILES_COMMON},
        {&FOLDERID_ProgramFilesCommonX86, CSIDL_PROGRAM_FILES_COMMONX86},
        {&FOLDERID_AdminTools, CSIDL_ADMINTOOLS},
        {&FOLDERID_CommonAdminTools, CSIDL_COMMON_ADMINTOOLS},
        {&FOLDERID_Music, CSIDL_MYMUSIC},
        {&FOLDERID_Pictures, CSIDL_MYPICTURES},
        {&FOLDERID_Videos, CSIDL_MYVIDEO},
        {&FOLDERID_NetworkFolder, CSIDL_NETWORK},
        {&FOLDERID_Fonts, CSIDL_FONTS},
        {&FOLDERID_CommonStartMenu, CSIDL_COMMON_STARTMENU},
        {&FOLDERID_CommonPrograms, CSIDL_COMMON_PROGRAMS},
        {&FOLDERID_CommonStartup, CSIDL_COMMON_STARTUP},
        {&FOLDERID_PublicDesktop, CSIDL_COMMON_DESKTOPDIRECTORY},
        {&FOLDERID_CommonTemplates, CSIDL_COMMON_TEMPLATES},
        {&FOLDERID_ResourceDir, CSIDL_RESOURCES},
        {&FOLDERID_LocalizedResourcesDir, CSIDL_RESOURCES_LOCALIZED},
        {&FOLDERID_CommonOEMLinks, CSIDL_COMMON_OEM_LINKS},
        {&FOLDERID_CDBurning, CSIDL_CDBURN_AREA},
    };

    int csidl = -1;

    // Find matching GUID in the mapping table
    for (size_t i = 0; i < sizeof(mapping) / sizeof(mapping[0]); i++)
    {
        if (memcmp(rfid, mapping[i].rfid, sizeof(GUID)) == 0)
        {
            csidl = mapping[i].csidl;
            break;
        }
    }

    if (csidl == -1)
        return E_FAIL;

    int shFlags = SHGFP_TYPE_CURRENT; // Default to current

    // Map flags
    if (dwFlags & KF_FLAG_CREATE)
        shFlags |= CSIDL_FLAG_CREATE;
    if (dwFlags & KF_FLAG_DONT_VERIFY)
        shFlags |= CSIDL_FLAG_DONT_VERIFY;
    if (dwFlags & KF_FLAG_NO_ALIAS)
        shFlags |= CSIDL_FLAG_NO_ALIAS;

    // Get the path using SHGetFolderPathW
    wchar_t path[MAX_PATH] = {0};
    HRESULT hr = SHGetFolderPathW(NULL, csidl, hToken, shFlags, path);

    if (FAILED(hr))
        return hr;

    size_t len = wcslen(path) + 1;
    PWSTR result = CoTaskMemAlloc(len * sizeof(wchar_t));
    if (!result)
        return E_OUTOFMEMORY;

    hr = StringCchCopyW(result, len, path);
    if (FAILED(hr))
    {
        CoTaskMemFree(result);
        return hr;
    }

    *ppszPath = result;
    return S_OK;
}
