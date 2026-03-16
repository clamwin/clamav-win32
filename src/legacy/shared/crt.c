/**
 * This file has no copyright assigned and is placed in the Public Domain.
 * This file is part of the mingw-w64 runtime package.
 * No warranty is given; refer to the file DISCLAIMER.PD within this package.
 */

#ifdef __GNUC__
#include <assert.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

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

// _freopen_s_compat
errno_t freopen_s_compat(FILE ** stream, const char * fileName, const char * mode, FILE* oldStream)
{
    FILE *fp;

    if (!stream || !mode || !oldStream)
    {
        if (stream)
            *stream = NULL;
        errno = EINVAL;
        return EINVAL;
    }

    fp = freopen(fileName, mode, oldStream);
    *stream = fp;

    if (fp)
        return 0;

    return errno ? errno : ENOENT;
}

errno_t mbstowcs_s_compat(size_t *pReturnValue, wchar_t *wcstr, size_t sizeInWords, const char *mbstr, size_t count)
{
    size_t converted, required, n;
    char *tmp = NULL;
    const char *src = mbstr;

    if (pReturnValue)
        *pReturnValue = 0;

    if (!mbstr || (wcstr == NULL && sizeInWords != 0) || (wcstr != NULL && sizeInWords == 0))
    {
        if (wcstr && sizeInWords)
            wcstr[0] = L'\0';
        errno = EINVAL;
        return EINVAL;
    }

    if (count != 0)
    {
        for (n = 0; n < count && mbstr[n] != '\0'; ++n)
            ;

        if (n == count)
        {
            tmp = malloc(count + 1);
            if (!tmp)
            {
                if (wcstr && sizeInWords)
                    wcstr[0] = L'\0';
                errno = ENOMEM;
                return ENOMEM;
            }

            memcpy(tmp, mbstr, count);
            tmp[count] = '\0';
            src = tmp;
        }
    }

    converted = mbstowcs(NULL, src, 0);
    if (converted == (size_t)-1)
    {
        free(tmp);
        if (wcstr && sizeInWords)
            wcstr[0] = L'\0';
        errno = EILSEQ;
        return EILSEQ;
    }

    required = converted + 1;

    if (pReturnValue)
        *pReturnValue = required;

    if (!wcstr)
    {
        free(tmp);
        return 0;
    }

    if (sizeInWords < required)
    {
        free(tmp);
        wcstr[0] = L'\0';
        errno = ERANGE;
        return ERANGE;
    }

    converted = mbstowcs(wcstr, src, converted + 1);
    free(tmp);
    if (converted == (size_t)-1)
    {
        wcstr[0] = L'\0';
        errno = EILSEQ;
        return EILSEQ;
    }

    return 0;
}

errno_t wcscpy_s_compat(wchar_t *dest, rsize_t dest_size, const wchar_t *src)
{
    rsize_t i;

    if (!dest || dest_size == 0 || !src)
    {
        if (dest && dest_size)
            dest[0] = L'\0';
        errno = EINVAL;
        return EINVAL;
    }

    for (i = 0; i < dest_size; ++i)
    {
        dest[i] = src[i];
        if (src[i] == L'\0')
            return 0;
    }

    dest[0] = L'\0';
    errno = ERANGE;
    return ERANGE;
}

errno_t wcsncpy_s_compat(wchar_t *strDest, size_t numberOfElements, const wchar_t *strSource, size_t count)
{
    size_t i, limit;

    if (!strDest || numberOfElements == 0 || !strSource)
    {
        if (strDest && numberOfElements)
            strDest[0] = L'\0';
        errno = EINVAL;
        return EINVAL;
    }

    limit = count;
    if (limit >= numberOfElements)
        limit = numberOfElements - 1;

    for (i = 0; i < limit; ++i)
    {
        wchar_t ch = strSource[i];
        strDest[i] = ch;
        if (ch == L'\0')
            return 0;
    }

    if (count < numberOfElements && strSource[i] == L'\0')
    {
        strDest[i] = L'\0';
        return 0;
    }

    strDest[0] = L'\0';
    errno = ERANGE;
    return ERANGE;
}

errno_t wcstombs_s_compat(size_t *pReturnValue, char *mbstr, size_t sizeInBytes, const wchar_t *wcstr, size_t count)
{
    size_t converted, required, limit;
    wchar_t *tmp = NULL;
    const wchar_t *src = wcstr;

    if (pReturnValue)
        *pReturnValue = 0;

    if (!wcstr || (mbstr == NULL && sizeInBytes != 0) || (mbstr != NULL && sizeInBytes == 0))
    {
        if (mbstr && sizeInBytes)
            mbstr[0] = '\0';
        errno = EINVAL;
        return EINVAL;
    }

    if (count != 0)
    {
        for (limit = 0; limit < count && wcstr[limit] != L'\0'; ++limit)
            ;

        if (limit == count)
        {
            tmp = malloc((count + 1) * sizeof(wchar_t));
            if (!tmp)
            {
                if (mbstr && sizeInBytes)
                    mbstr[0] = '\0';
                errno = ENOMEM;
                return ENOMEM;
            }

            memcpy(tmp, wcstr, count * sizeof(wchar_t));
            tmp[count] = L'\0';
            src = tmp;
        }
    }

    converted = wcstombs(NULL, src, 0);
    if (converted == (size_t)-1)
    {
        free(tmp);
        if (mbstr && sizeInBytes)
            mbstr[0] = '\0';
        errno = EILSEQ;
        return EILSEQ;
    }

    required = converted + 1;

    if (pReturnValue)
        *pReturnValue = required;

    if (!mbstr)
    {
        free(tmp);
        return 0;
    }

    if (sizeInBytes < required)
    {
        free(tmp);
        mbstr[0] = '\0';
        errno = ERANGE;
        return ERANGE;
    }

    converted = wcstombs(mbstr, src, converted + 1);
    free(tmp);
    if (converted == (size_t)-1)
    {
        mbstr[0] = '\0';
        errno = EILSEQ;
        return EILSEQ;
    }

    return 0;
}

#endif // __GNUC__
