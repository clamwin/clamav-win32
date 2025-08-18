/*
 * Legacy Windows Compatibility Layer
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

#include "legacy.h"
#include "dynload.h"
#include "loadlibrary.h"
#include "initializer.h"

static imp_SystemFunction036 pSystemFunction036 = NULL;

INITIALIZER(init_bcryptprimitives)
{
    OSVERSIONINFO osvi = {0};
    TRACE("Init @ " __FILE__ "\n");

    osvi.dwOSVersionInfoSize = sizeof(osvi);
    GetVersionEx(&osvi);

    // win8 or later
    if ((osvi.dwMajorVersion < 6) || ((osvi.dwMajorVersion == 6) && (osvi.dwMinorVersion < 2)))
        return;

    HMODULE advapi32 = LoadLibraryFromWin32(TEXT("advapi32.dll"));
    if (advapi32)
        IMPORT_FUNCTION(advapi32, SystemFunction036);
}

BOOL WINAPI ProcessPrng(void *buffer, size_t size)
{
    TRACE("ProcessPrng(0x%p, %zu) RtlGenRandom = 0x%p\n", buffer, size, pSystemFunction036);

    // use RtlGenRandom if available
    if (pSystemFunction036)
        return pSystemFunction036(buffer, (ULONG)size);

    HCRYPTPROV hProv;
    BOOL result;

    // Acquire a cryptographic context. The CRYPT_VERIFYCONTEXT flag indicates that
    // no persistent key container is needed (suitable for generating random data).
    if (!CryptAcquireContext(&hProv, NULL, NULL, PROV_RSA_FULL, CRYPT_VERIFYCONTEXT))
        return FALSE;

    // Generate random bytes and fill the buffer.
    result = CryptGenRandom(hProv, (DWORD)size, (BYTE *)buffer);

    // Release the cryptographic context.
    CryptReleaseContext(hProv, 0);

    return result;
}

/*
    UUID Version 4 Requirements:
    - Set version bits to 0100 (4 in hex) in the 16-bit Data3 field
    - Set variant bits to 10xxxxxx in the first byte of Data4
*/
BOOL WINAPI ProcessPrngGuid(GUID *pGUID)
{
    if (!ProcessPrng(pGUID, sizeof(GUID)))
        return FALSE;

    // Data4[0] modifications (Variant Field)
    pGUID->Data4[0] &= 0x3F; // Clear bits 7-6 (mask: 00111111)
    pGUID->Data4[0] |= 0x80; // Set bit 7 (variant 2: 10xxxxxx)
    // Result: Binary pattern 10xxxxxx (Microsoft GUID variant)

    // Data3 modifications (Version Field)
    pGUID->Data3 &= 0x0FFF; // Clear upper 4 bits (mask: 0000111111111111)
    pGUID->Data3 |= 0x4000; // Set version 4 (mask: 0100000000000000)
    // Result: Upper 4 bits become 0100 (version 4 identifier)

    return TRUE;
}
