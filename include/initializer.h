#ifndef _INITIALIZER_H_
#define _INITIALIZER_H_

#if defined(_MSC_VER) && !defined(__clang__)
#if defined _M_IX86
#define _CRT_LINKER_SYMBOL_PREFIX(f) "_"
#elif defined _M_X64 || defined _M_ARM || defined _M_ARM64
#define _CRT_LINKER_SYMBOL_PREFIX ""
#else
#error Unsupported architecture
#endif
#pragma section(".CRT$XCU", read)
#define INITIALIZER(f)                                       \
    static void f(void);                                     \
    __declspec(allocate(".CRT$XCU")) void (*f##_)(void) = f; \
    __pragma(comment(linker, "/include:" _CRT_LINKER_SYMBOL_PREFIX #f "_")) static void f(void)
#else
#define INITIALIZER(f)                                \
    static void f(void) __attribute__((constructor)); \
    static void f(void)
#endif
#endif // _INITIALIZER_H_
