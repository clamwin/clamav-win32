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

// Define KF_FLAG_CREATE if not already defined.
#ifndef KF_FLAG_CREATE
#define KF_FLAG_CREATE 0x00008000
#endif

// Define a structure to map Known Folder GUIDs to CSIDL values.
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

    // Mapping table: maps a few Known Folder GUIDs to their CSIDL equivalents.
    static const KnownFolderMapping mapping[] = {
        {&FOLDERID_Desktop, CSIDL_DESKTOP},
        {&FOLDERID_Documents, CSIDL_PERSONAL},
        {&FOLDERID_Profile, CSIDL_PROFILE},
        // Extend this table with additional mappings as needed.
    };

    int csidl = -1;
    for (size_t i = 0; i < sizeof(mapping) / sizeof(mapping[0]); i++)
    {
        // Compare the GUIDs by comparing their memory.
        if (memcmp(rfid, mapping[i].rfid, sizeof(GUID)) == 0)
        {
            csidl = mapping[i].csidl;
            break;
        }
    }

    if (csidl == -1)
        return E_FAIL;

    // Map KF_FLAG_CREATE (if specified) to CSIDL_FLAG_CREATE.
    int shFlags = 0;
    if (dwFlags & KF_FLAG_CREATE)
        shFlags |= CSIDL_FLAG_CREATE;

    wchar_t path[MAX_PATH];
    HRESULT hr = SHGetFolderPathW(hToken, csidl, NULL, shFlags, path);
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
