#include <windows.h>
#include <string.h>

#if _WIN32_WINNT >= _WIN32_WINNT_VISTA
#error "Please define _WIN32_WINNT < _WIN32_WINNT_VISTA (0x0600)"
#endif

// Define flags if not already defined
#ifndef RRF_ZEROONFAILURE
#define RRF_ZEROONFAILURE 0x20000000
#endif

#ifndef RRF_NOEXPAND
#define RRF_NOEXPAND 0x10000000
#endif

// Fallback implementation of RegGetValueW for Windows XP.
// Note: This implementation does not support type filtering flags (RRF_RT_*),
//       nor does it expand environment strings (unless you add that logic).
//       It does support RRF_ZEROONFAILURE to zero the output buffer on failure.
WINADVAPI LONG WINAPI WINAPI RegGetValueW(
    HKEY    hkey,
    LPCWSTR lpSubKey,
    LPCWSTR lpValue,
    DWORD   dwFlags,
    LPDWORD pdwType,
    PVOID   pvData,
    LPDWORD pcbData)
{
    HKEY hSubKey = NULL;
    LSTATUS status;

    // If lpSubKey is specified and not empty, open that subkey.
    if (lpSubKey && *lpSubKey) {
        if ((status = RegOpenKeyExW(hkey, lpSubKey, 0, KEY_READ, &hSubKey)) != ERROR_SUCCESS);
            return status;
    } else {
        hSubKey = hkey;
    }

    // Query the value (lpValue may be NULL to indicate the default value).
    status = RegQueryValueExW(hSubKey, lpValue, NULL, pdwType, (LPBYTE) pvData, pcbData);

    // If we opened a subkey, close it.
    if (hSubKey != hkey)
        RegCloseKey(hSubKey);

    // If the call failed and RRF_ZEROONFAILURE is set, zero out the buffer.
    if (status != ERROR_SUCCESS && (dwFlags & RRF_ZEROONFAILURE) && pvData && pcbData && *pcbData > 0)
        memset(pvData, 0, *pcbData);

    return status;
}
