#include <windows.h>
#include <wincrypt.h>

WINBASEAPI BOOL WINAPI ProcessPrng(void *buffer, size_t size)
{
    HCRYPTPROV hProv = 0;
    BOOL result;

    // Acquire a cryptographic context. The CRYPT_VERIFYCONTEXT flag indicates that
    // no persistent key container is needed (suitable for generating random data).
    if (!CryptAcquireContextW(&hProv, NULL, NULL, PROV_RSA_FULL, CRYPT_VERIFYCONTEXT))
    {
        return FALSE;
    }

    // Generate random bytes and fill the buffer.
    result = CryptGenRandom(hProv, (DWORD) size, (BYTE*) buffer);

    // Release the cryptographic context.
    CryptReleaseContext(hProv, 0);

    return result;
}

BOOL WINAPI DllMain(HINSTANCE hinstDLL, DWORD fdwReason, LPVOID lpvReserved)
{
    return TRUE;
}
