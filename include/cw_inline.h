#ifndef _CW_INLINE_H
#define _CW_INLINE_H

#include <stdio.h>
#include <time.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/timeb.h>
#include <stdint.h>
#include <io.h>

#ifndef _TIMEZONE_DEFINED
struct timezone {
    int tz_minuteswest; /* minutes W of Greenwich */
    int tz_dsttime;     /* type of dst correction */
};
#endif

static inline int gettimeofday(struct timeval *tv, struct timezone *tz)
{
    struct timeb timebuffer;
    ftime(&timebuffer);
    tv->tv_sec = (long) timebuffer.time;
    tv->tv_usec = 1000 * timebuffer.millitm;
    return 0;
}

extern wchar_t* uncpath(const char* path);
extern const char *cw_get_currentfile(void);
extern void cw_set_currentfile(const char *filename);

static inline int safe_open(const char* path, int flags, ...)
{
    wchar_t* wpath = uncpath(path);
    cw_set_currentfile(path);
    int ret;

    if (!wpath)
        return -1;

    if (flags & O_CREAT) {
        int mode;
        va_list ap;
        va_start(ap, flags);
        mode = va_arg(ap, int);
        va_end(ap);
        ret = _wopen(wpath, flags, mode);
    }
    else
        ret = _wopen(wpath, flags);
    free(wpath);
    return ret;
}

static inline wchar_t *mb2wc(const char *mb)
{
    wchar_t *wc;
    DWORD len = 0;

    if (!(len = MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, mb, -1, NULL, 0)))
        return NULL;

    if (!(wc = (wchar_t *) malloc(len * sizeof(wchar_t))))
    {
        fprintf(stderr, "mb2wc: OOM\n");
        return NULL;
    }

    if (MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, mb, -1, wc, len))
        return wc;

    free(wc);
    return NULL;
}

#endif /* _CW_INLINE_H */
