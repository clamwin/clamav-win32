/*===-- include/System/DataTypes.h - Define fixed size types -----*- C -*-===*\
|*                                                                            *|
|*                     The LLVM Compiler Infrastructure                       *|
|*                                                                            *|
|* This file is distributed under the University of Illinois Open Source      *|
|* License. See LICENSE.TXT for details.                                      *|
|*                                                                            *|
|*===----------------------------------------------------------------------===*|
|*                                                                            *|
|* This file contains definitions to figure out the size of _HOST_ data types.*|
|* This file is important because different host OS's define different macros,*|
|* which makes portability tough.  This file exports the following            *|
|* definitions:                                                               *|
|*                                                                            *|
|*   [u]int(32|64)_t : typedefs for signed and unsigned 32/64 bit system types*|
|*   [U]INT(8|16|32|64)_(MIN|MAX) : Constants for the min and max values.     *|
|*                                                                            *|
|* No library is required when using these functinons.                        *|
|*                                                                            *|
|*===----------------------------------------------------------------------===*/

/* Please leave this file C-compatible. */

#ifndef SUPPORT_DATATYPES_H
#define SUPPORT_DATATYPES_H

#define HAVE_SYS_TYPES_H 1
#undef HAVE_INTTYPES_H
#undef HAVE_STDINT_H
#define HAVE_UINT64_T 1
#undef HAVE_U_INT64_T

#if defined(_MSC_VER)
typedef unsigned __int64 uint64_t;
typedef signed __int64 int64_t;
typedef unsigned __int32 uint32_t;
typedef signed __int32 int32_t;
typedef unsigned __int16 uint16_t;
typedef signed __int16 int16_t;
typedef unsigned __int8 uint8_t;
typedef signed __int8 int8_t;

#ifndef _DEBUG
// Looks like I need to include first stdlib.h on vc rel
// stdlib.h(477) : error C2365: '_byteswap_ushort' : redefinition; previous definition was 'formerly unknown identifier'
#include <stdlib.h>
#endif

#ifdef  _WIN64
typedef __int64 ssize_t;
#else
typedef int ssize_t;
#endif

#define INT8_MAX 127
#define INT8_MIN -128
#define UINT8_MAX 255
#define INT16_MAX 32767
#define INT16_MIN -32768
#define UINT16_MAX 65535
#define INT32_MAX 2147483647
#define INT32_MIN -2147483648
#define UINT32_MAX 4294967295U

/* Certain compatibility updates to VC++ introduce the `cstdint'
 * header, which defines the INT*_C macros. On default installs they
 * are absent. */
#ifndef INT8_C
# define INT8_C(C)   C
#endif
#ifndef UINT8_C
# define UINT8_C(C)  C
#endif
#ifndef INT16_C
# define INT16_C(C)  C
#endif
#ifndef UINT16_C
# define UINT16_C(C) C
#endif
#ifndef INT32_C
# define INT32_C(C)  C
#endif
#ifndef UINT32_C
# define UINT32_C(C) C ## U
#endif
#ifndef INT64_C
# define INT64_C(C)  ((int64_t) C ## LL)
#endif
#ifndef UINT64_C
# define UINT64_C(C) ((uint64_t) C ## ULL)
#endif

#elif defined(__MINGW32__)
#define HAVE_INTTYPES_H 1
#define HAVE_STDINT_H 1
#endif

#ifdef __cplusplus
#include <cmath>
#else
#include <math.h>
#endif

/* Note that this header's correct operation depends on __STDC_LIMIT_MACROS
   being defined.  We would define it here, but in order to prevent Bad Things
   happening when system headers or C++ STL headers include stdint.h before we
   define it here, we define it on the g++ command line (in Makefile.rules). */
#if !defined(__STDC_LIMIT_MACROS)
# error "Must #define __STDC_LIMIT_MACROS before #including System/DataTypes.h"
#endif

#if !defined(__STDC_CONSTANT_MACROS)
# error "Must #define __STDC_CONSTANT_MACROS before " \
        "#including System/DataTypes.h"
#endif

/* Note that <inttypes.h> includes <stdint.h>, if this is a C99 system. */
#ifdef HAVE_SYS_TYPES_H
#include <sys/types.h>
#endif

#ifdef HAVE_INTTYPES_H
#include <inttypes.h>
#endif

#ifdef HAVE_STDINT_H
#include <stdint.h>
#endif

#ifdef _AIX
#include "llvm/System/AIXDataTypesFix.h"
#endif

/* Handle incorrect definition of uint64_t as u_int64_t */
#ifndef HAVE_UINT64_T
#ifdef HAVE_U_INT64_T
typedef u_int64_t uint64_t;
#else
# error "Don't have a definition for uint64_t on this platform"
#endif
#endif

#ifdef _OpenBSD_
#define INT8_MAX 127
#define INT8_MIN -128
#define UINT8_MAX 255
#define INT16_MAX 32767
#define INT16_MIN -32768
#define UINT16_MAX 65535
#define INT32_MAX 2147483647
#define INT32_MIN -2147483648
#define UINT32_MAX 4294967295U
#endif

/* Set defaults for constants which we cannot find. */
#if !defined(INT64_MAX)
# define INT64_MAX 9223372036854775807LL
#endif
#if !defined(INT64_MIN)
# define INT64_MIN ((-INT64_MAX)-1)
#endif
#if !defined(UINT64_MAX)
# define UINT64_MAX 0xffffffffffffffffULL
#endif

#if __GNUC__ > 3
#define END_WITH_NULL __attribute__((sentinel))
#else
#define END_WITH_NULL
#endif

#ifndef HUGE_VALF
#define HUGE_VALF (float)HUGE_VAL
#endif

#endif  /* SUPPORT_DATATYPES_H */
