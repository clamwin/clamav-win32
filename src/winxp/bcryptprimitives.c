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

#include <wincrypt.h>

WINBASEAPI BOOL WINAPI ProcessPrng(void *buffer, size_t size)
{
    HCRYPTPROV hProv = 0;
    BOOL result;

    TRACE(L"ProcessPrng(0x%p, %d)\n", buffer, size);

    // Acquire a cryptographic context. The CRYPT_VERIFYCONTEXT flag indicates that
    // no persistent key container is needed (suitable for generating random data).
    if (!CryptAcquireContextW(&hProv, NULL, NULL, PROV_RSA_FULL, CRYPT_VERIFYCONTEXT))
    {
        return FALSE;
    }

    // Generate random bytes and fill the buffer.
    result = CryptGenRandom(hProv, (DWORD)size, (BYTE *)buffer);

    // Release the cryptographic context.
    CryptReleaseContext(hProv, 0);

    return result;
}
