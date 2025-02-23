/*
 * Clamav Native Windows Port: memory related stuff
 *
 * Copyright (c) 2005-2025 Gianluigi Tiesi <sherpya@gmail.com>
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

#ifndef _CRASHDUMP_H_
#define _CRASHDUMP_H_

//#include <tlhelp32.h>
//#include <psapi.h>

/* dbghelp32 */
#ifdef _MSC_VER
#pragma warning (push)
#pragma warning (disable:4091)
#include <dbghelp.h>
#pragma warning (pop)
#define MINDUMP_FLAGS (MINIDUMP_TYPE) \
    (MiniDumpWithDataSegs  | MiniDumpWithIndirectlyReferencedMemory | MiniDumpFilterModulePaths)
typedef BOOL (WINAPI *pMiniDumpWriteDumpFunc)(HANDLE, DWORD, HANDLE, MINIDUMP_TYPE,
                                              CONST PMINIDUMP_EXCEPTION_INFORMATION,
                                              CONST PMINIDUMP_USER_STREAM_INFORMATION,
                                              CONST PMINIDUMP_CALLBACK_INFORMATION);

extern LONG __stdcall CrashHandlerExceptionFilter(EXCEPTION_POINTERS *pExPtrs);
#endif

//typedef int (*proc_callback)(PROCESSENTRY32 ProcStruct, MODULEENTRY32 me32, void *data);

#endif /* _CRASHDUMP_H_ */
