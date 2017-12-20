#ifndef _CW_INLINE_H
#define _CW_INLINE_H

#include <stdio.h>
#include <time.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/timeb.h>
#include <inttypes.h>
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

static inline int cw_open(const char *filename, int oflag, ...)
{
    va_list ap;
    int pmode = 0;

    va_start(ap, oflag);
    pmode = va_arg(ap, int);
    va_end(ap);

    if (oflag & _O_CREAT)
    {
        oflag |= _O_SHORT_LIVED;
        pmode |= (_S_IREAD | _S_IWRITE);
    }

    return _open(filename, oflag, pmode);
}
#define open cw_open

extern const char *cw_get_currentfile(void);
extern void cw_set_currentfile(const char *filename);
static inline int safe_open(const char *filename, int oflag)
{
    cw_set_currentfile(filename);
    return cw_open(filename, oflag);
}

/* FIXME: check if this function works as expected */
static inline ssize_t pread(int fd, void *buf, size_t count, off_t offset)
{
    off_t lastpos = _lseek(fd, offset, SEEK_SET);
    ssize_t res;
    if (lastpos == -1) return -1;
    res = (ssize_t) _read(fd, buf, (unsigned int) count);
    return ((_lseek(fd, lastpos, SEEK_SET) == -1) ? -1 : res);
}

#define CW_CHECKALLOC(var, alloc, ret) \
do \
{ \
    if (!(var = alloc)) \
    { \
        fprintf(stderr, #alloc" failed\n"); \
        ret; \
    } \
} while (0)

#define NORMALIZE_PATH(path, free_src, fail)                        \
{                                                                   \
    char *swap = cw_normalizepath((char *) path);                   \
    if (free_src) free((void *) path);                              \
    if (!swap) { fail; }                                            \
    if (free_src)                                                   \
        CW_CHECKALLOC(path, malloc(strlen(swap) + 1), fail);        \
    else                                                            \
        CW_CHECKALLOC(path, alloca(strlen(swap) + 1), fail);        \
    strcpy((char *) path, swap);                                    \
    free(swap); swap = NULL;                                        \
}

#endif /* _CW_INLINE_H */
