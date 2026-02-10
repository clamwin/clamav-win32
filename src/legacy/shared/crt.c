/**
 * This file has no copyright assigned and is placed in the Public Domain.
 * This file is part of the mingw-w64 runtime package.
 * No warranty is given; refer to the file DISCLAIMER.PD within this package.
 */

#ifdef __GNUC__
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <assert.h>
#include <tchar.h>

/* _wassert is not available on XP, so forward it to _assert if needed */
__MINGW_ATTRIB_NORETURN
void __cdecl emu__wassert(const wchar_t *_Message, const wchar_t *_File, unsigned _Line)
{
    char *message = NULL, *file = NULL;
    size_t len;

    if ((len = wcstombs(NULL, _Message, 0)) != (size_t)-1)
    {
        message = malloc(len + 1);
        wcstombs(message, _Message, len + 1);
    }

    if ((len = wcstombs(NULL, _File, 0)) != (size_t)-1)
    {
        file = malloc(len + 1);
        wcstombs(file, _File, len + 1);
    }

    _assert(message, file, _Line);
    abort();
}

// added in curl 8.18.0
errno_t _wsopen_s_compat(int *pfh, const wchar_t *filename, int oflag, int shflag, int pmode)
{
    if (pfh == NULL)
    {
        errno = EINVAL;
        return EINVAL;
    }

    *pfh = _wsopen(filename, oflag, shflag, pmode);

    if (*pfh == -1)
        return errno;

    return 0;
}

errno_t _wfreopen_s_compat(FILE **stream, const wchar_t *fileName, const wchar_t *mode, FILE *oldStream)
{
    if (stream == NULL)
    {
        errno = EINVAL;
        return EINVAL;
    }

    *stream = _wfreopen(fileName, mode, oldStream);

    if (*stream == NULL)
        return errno;

    return 0;
}

errno_t wcscpy_s_compat(wchar_t *dest, rsize_t dest_size, const wchar_t *src)
{
    if ((dest == NULL) || (src == NULL))
    {
        if ((dest != NULL) && (dest_size > 0))
            dest[0] = L'\0';
        errno = EINVAL;
        return EINVAL;
    }

    if (dest_size == 0)
    {
        errno = EINVAL;
        return EINVAL;
    }

    size_t src_len = wcslen(src);

    if (src_len >= dest_size)
    {
        dest[0] = L'\0';
        errno = ERANGE;
        return ERANGE;
    }

    wcscpy(dest, src);

    return 0;
}

errno_t wcsncpy_s_compat(wchar_t *dest, rsize_t dest_size, const wchar_t *src, rsize_t count)
{
    if ((dest == NULL) || (src == NULL))
    {
        if ((dest != NULL) && (dest_size > 0))
            dest[0] = L'\0';
        errno = EINVAL;
        return EINVAL;
    }

    if (dest_size == 0)
    {
        errno = EINVAL;
        return EINVAL;
    }

    size_t src_len = wcsnlen(src, count);
    size_t copy_len = (src_len < count) ? src_len : count;

    if (copy_len >= dest_size)
    {
        dest[0] = L'\0';
        errno = ERANGE;
        return ERANGE;
    }

    wcsncpy(dest, src, copy_len);
    dest[copy_len] = L'\0';

    return 0;
}

errno_t wcstombs_s_compat(size_t *pReturnValue, char *mbstr, size_t sizeInBytes, const wchar_t *wcstr, size_t count)
{
    size_t result;

    if ((mbstr == NULL) && (sizeInBytes == 0))
    {
        if (wcstr == NULL)
        {
            errno = EINVAL;
            return EINVAL;
        }

        result = wcstombs(NULL, wcstr, 0);
        if (result == (size_t)-1)
            return errno;

        if (pReturnValue != NULL)
            *pReturnValue = result + 1;
        return 0;
    }

    if ((mbstr == NULL) || (wcstr == NULL) || (sizeInBytes == 0))
    {
        if ((mbstr != NULL) && (sizeInBytes > 0))
            mbstr[0] = '\0';
        errno = EINVAL;
        return EINVAL;
    }

    size_t max_convert = (count < sizeInBytes) ? count : sizeInBytes - 1;

    result = wcstombs(mbstr, wcstr, max_convert);

    if (result == (size_t)-1)
    {
        mbstr[0] = '\0';
        if (pReturnValue != NULL)
            *pReturnValue = 0;
        return errno;
    }

    if (result < sizeInBytes)
        mbstr[result] = '\0';
    else
        mbstr[sizeInBytes - 1] = '\0';

    if (pReturnValue != NULL)
        *pReturnValue = result;

    return 0;
}
#endif // __GNUC__
