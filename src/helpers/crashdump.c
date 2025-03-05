/*
 * Clamav Native Windows Port: Crash Dumper Helper
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

#ifdef _MSC_VER

#include "platform.h"
#include "osdeps.h"

#include <dbghelp.h>

#define MINDUMP_FLAGS (MINIDUMP_TYPE) \
    (MiniDumpWithDataSegs  | MiniDumpWithIndirectlyReferencedMemory | MiniDumpFilterModulePaths)
typedef BOOL(WINAPI* pMiniDumpWriteDumpFunc)(HANDLE, DWORD, HANDLE, MINIDUMP_TYPE,
	CONST PMINIDUMP_EXCEPTION_INFORMATION,
	CONST PMINIDUMP_USER_STREAM_INFORMATION,
	CONST PMINIDUMP_CALLBACK_INFORMATION);

DWORD WINAPI CrashMiniDumpWriteDumpProc(LPVOID lpParam)
{
	PEXCEPTION_POINTERS pExPtrs = (PEXCEPTION_POINTERS)lpParam;
	LONG retval = 1;
	MINIDUMP_EXCEPTION_INFORMATION ExInfo;
	pMiniDumpWriteDumpFunc pMiniDumpWriteDump = NULL;
	HANDLE hFile = INVALID_HANDLE_VALUE;
	HANDLE hMapFile = NULL;
	LPBYTE lpMapAddress = NULL;
	BY_HANDLE_FILE_INFORMATION FileInformation;
	HMODULE hDll = NULL;
	wchar_t dumpfile[MAX_PATH + 1] = { 0 };
	wchar_t executable[MAX_PATH + 1] = L"Unknown module";
	wchar_t* lSlash;

	GetModuleFileNameW(NULL, executable, MAX_PATH);
	GetTempPathW(MAX_PATH, dumpfile);
	dumpfile[MAX_PATH] = L'\0';

	lSlash = wcsrchr(executable, L'\\');
	size_t size = wcslen(dumpfile);
	_snwprintf(&dumpfile[size], MAX_PATH - size, L"%s.%08lx.dmp", lSlash ? lSlash + 1 : L"Unknown", GetCurrentProcessId());
	dumpfile[MAX_PATH - size - 1] = L'\0';

	fwprintf(stderr, L"*** ClamWinDumper ***\n"
		L"*** %ls Crashed\n"
		L"    ExpCode   : 0x%8.8x\n"
		L"    ExpAddress: 0x%p\n",
		executable,
		pExPtrs->ExceptionRecord->ExceptionCode,
		pExPtrs->ExceptionRecord->ExceptionAddress);

	/* Load the version provided by the environment */
	if (!hDll)
		hDll = LoadLibraryW(L"dbghelp.dll");

	if (!hDll)
	{
		fwprintf(stderr, L"[crashdump] Cannot find dbghelp.dll, I cannot produce a crash dump without\n");
		return retval;
	}

	pMiniDumpWriteDump = (pMiniDumpWriteDumpFunc)GetProcAddress(hDll, "MiniDumpWriteDump");
	if (!pMiniDumpWriteDump)
	{
		fwprintf(stderr, L"[crashdump] Your dbghelp.dll does not export MiniDumpWriteDump\n");
		goto cleanup;
	}

	hFile = CreateFileW(dumpfile, GENERIC_READ | GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);

	if (hFile == INVALID_HANDLE_VALUE)
	{
		fwprintf(stderr, L"[crashdump] Cannot open file for writing (LE: %d)\n", GetLastError());
		goto cleanup;
	}

	ExInfo.ThreadId = GetCurrentThreadId();
	ExInfo.ExceptionPointers = pExPtrs;
	ExInfo.ClientPointers = FALSE;

	if (!pMiniDumpWriteDump(GetCurrentProcess(), GetCurrentProcessId(), hFile, MINDUMP_FLAGS, &ExInfo, NULL, NULL))
	{
		fwprintf(stderr, L"[crashdump] MiniDumpWriteDump() failed (LE: %d)\n", GetLastError());
		goto cleanup;
	}

#ifdef SCRAMBLE_CRASH_DUMP
	/* Now scramble it by xor-ing with 42, to avoid false positives on the dump file */
	hMapFile = CreateFileMappingW(hFile, NULL, PAGE_READWRITE, 0, 0, L"libClamAVDumper");

	if (!hMapFile)
	{
		fwprintf(stderr, L"[crashdump] CreateFileMappingW() failed (LE: %d)\n", GetLastError());
		goto cleanup;
	}

	lpMapAddress = (LPBYTE)MapViewOfFile(hMapFile, FILE_MAP_ALL_ACCESS, 0, 0, 0);
	if (!lpMapAddress)
	{
		fwprintf(stderr, L"[crashdump] MapViewOfFile() failed (LE: %d)\n", GetLastError());
		goto cleanup;
	}

	if (!GetFileInformationByHandle(hFile, &FileInformation))
	{
		fwprintf(stderr, L"[crashdump] GetFileInformationByHandle() failed (LE: %d)\n", GetLastError());
		goto cleanup;
	}

	for (int i = 0; i < FileInformation.nFileSizeLow; i++)
		lpMapAddress[i] ^= 42;

	FlushViewOfFile(lpMapAddress, 0);
#endif
	fwprintf(stderr, L"[crashdump] Crash Dump saved as %ls, please report\n", dumpfile);
	retval = 0;

cleanup:
	if (lpMapAddress) UnmapViewOfFile(lpMapAddress);
	if (hMapFile) CloseHandle(hMapFile);
	if (hFile != INVALID_HANDLE_VALUE) CloseHandle(hFile);
	FreeLibrary(hDll);
	return retval;
}

LONG __stdcall CrashHandlerExceptionFilter(PEXCEPTION_POINTERS pExPtrs)
{
	/* Spawn a new thread this should improve the dump */
	HANDLE cProc;
	DWORD tid = 0, res = -1;

	cProc = CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)CrashMiniDumpWriteDumpProc, (LPVOID)pExPtrs, 0, &tid);

	if (!cProc)
	{
		fwprintf(stderr, L"[crashdump] W00ps!! Cannot spawn crash dumper thread (LE: %d)\n", GetLastError());
		abort();
	}

	if (WaitForSingleObject(cProc, INFINITE) == WAIT_OBJECT_0)
		GetExitCodeThread(cProc, &res);

	CloseHandle(cProc);
	fwprintf(stderr, L"[crashdump] Thread Done (Result: %d)\n", res);
	return EXCEPTION_EXECUTE_HANDLER;
}

#endif /* _MSC_VER */
