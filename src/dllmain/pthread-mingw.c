#if defined(__MINGW32__) && defined(PTW32_STATIC_LIB)
#define NEED_FTIME
#include <pthread.c>
#endif
