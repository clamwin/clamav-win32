#include <windows.h>
#include <shlobj.h>
#include <objbase.h>
#include <wchar.h>
#include <stdlib.h>
#include <string.h>

// Define KF_FLAG_CREATE if not already defined.
#ifndef KF_FLAG_CREATE
#define KF_FLAG_CREATE 0x00008000
#endif

// Define a structure to map Known Folder GUIDs to CSIDL values.
typedef struct _KnownFolderMapping {
    const GUID *rfid;
    int csidl;
} KnownFolderMapping;

STDAPI SHGetKnownFolderPath(REFKNOWNFOLDERID rfid, DWORD dwFlags, HANDLE hToken, PWSTR *ppszPath)
{
    if (ppszPath == NULL)
        return E_INVALIDARG;
    *ppszPath = NULL;

    // Mapping table: maps a few Known Folder GUIDs to their CSIDL equivalents.
    static const KnownFolderMapping mapping[] = {
        { &FOLDERID_Desktop,   CSIDL_DESKTOP },
        { &FOLDERID_Documents,  CSIDL_PERSONAL },
        { &FOLDERID_Profile,    CSIDL_PROFILE },
        // Extend this table with additional mappings as needed.
    };

    int csidl = -1;
    for (size_t i = 0; i < sizeof(mapping)/sizeof(mapping[0]); i++) {
        // Compare the GUIDs by comparing their memory.
        if (memcmp(rfid, mapping[i].rfid, sizeof(GUID)) == 0) {
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
    PWSTR result = (PWSTR)CoTaskMemAlloc(len * sizeof(wchar_t));
    if (!result)
        return E_OUTOFMEMORY;

    wcscpy(result, path);
    *ppszPath = result;
    return S_OK;
}
