/*
 * Clamav Native Windows Port
 *
 * Copyright (c) 2005-2008 Gianluigi Tiesi <sherpya@netfarm.it>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this software; if not, write to the
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA
 */

#ifndef _CWDEFS_H_
#define _CWDEFS_H_

#ifdef _MSC_VER
/* warnings */
#pragma warning(disable: 4996)              /* Deprecated stuff */
#pragma warning(disable: 4018)              /* Signed/Unsigned mismatch */
#pragma warning(disable: 4244 4267)         /* Conversion, possible loss of data */
#pragma warning(disable: 4146)              /* Minus operator applied to unsigned */
#pragma warning(disable: 4101 4102)         /* Unreferenced local variable / label */
#pragma warning(disable: 4090)              /* 'function' : different 'const' qualifiers */

/* #pragma warning(disable: 4715) */        /* not all control paths return a value */
/* #pragma warning(disable: 4005) */        /* Macro redefinition: 'NDEBUG' not "onto" standard */
/* #pragma warning(disable: 4129) */        /* Unrecognized character escape seq % in PDF */
/* #pragma warning(disable: 4026) */        /* Function declared with formal parameter list */
/* #pragma warning(disable: 4311) */        /* 'type cast' pointer truncation */
/* #pragma warning(disable: 4013) */        /* Function undefined - disable with care */

#define _CRT_SECURE_NO_DEPRECATE 1

#endif /* _MSC_VER */

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef WINVER
#define WINVER 0x0501
#endif

#ifndef __cplusplus
#define inline __inline
#endif

#endif /* _CWDEFS_H_ */
