/* Needed by libxml2 */
#include <windows.h>
#include <ntstatus.h>
#include <wincrypt.h>
#include <bcrypt.h>

#if _WIN32_WINNT < _WIN32_WINNT_VISTA
NTSTATUS WINAPI BCryptGenRandom(BCRYPT_ALG_HANDLE hAlgorithm, PUCHAR pbBuffer, ULONG cbBuffer, ULONG dwFlags) {
    if (hAlgorithm || !(dwFlags & BCRYPT_USE_SYSTEM_PREFERRED_RNG))
        return STATUS_NOT_IMPLEMENTED;

    HCRYPTPROV hProvider;
    if (CryptAcquireContext(&hProvider, NULL, NULL, PROV_RSA_FULL, CRYPT_VERIFYCONTEXT))
    {
        BOOL ret = CryptGenRandom(hProvider, cbBuffer, pbBuffer);
        CryptReleaseContext(hProvider, 0);
        if (ret)
           return STATUS_SUCCESS;
    }

    return STATUS_INVALID_PARAMETER;
}
#endif
