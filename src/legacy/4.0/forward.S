/*
 * Legacy Windows Compatibility Layer: Windows 9x
 *
 * Copyright (c) 2025 Gianluigi Tiesi <sherpya@gmail.com>
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

#ifndef _WIN64
.section .rdata,"dr"

missing_getaddrinfo:
    .asciz "getaddrinfo is not supported, aborting.\n"
missing_freeaddrinfo:
    .asciz "freeaddrinfo is not supported, aborting.\n"

missing_addveh:
    .asciz "AddVectoredExceptionHandler is not supported, aborting.\n"
missing_createhardlink:
    .asciz "CreateHardLinkW is not supported, aborting.\n"
missing_th32snap:
    .asciz "CreateToolhelp32Snapshot is not supported, aborting.\n"
missing_proc32first:
    .asciz "Process32FirstW is not supported, aborting.\n"
missing_proc32next:
    .asciz "Process32NextW is not supported, aborting.\n"
missing_module32first:
    .asciz "Module32FirstW is not supported, aborting.\n"
missing_module32next:
    .asciz "Module32NextW is not supported, aborting.\n"

.global __imp__RegisterWaitForSingleObject@24
.global __imp__UnregisterWait@4
.global __imp__UnregisterWaitEx@8

.global __imp__MultiByteToWideChar@24
.global _MultiByteToWideChar@24

.global _GetUserProfileDirectoryW@12

.global __imp__CreateThread@24
.global _CreateThread@24

__imp__RegisterWaitForSingleObject@24:
    .long thunk_pRegisterWaitForSingleObject@24
__imp__UnregisterWait@4:
    .long thunk_pUnregisterWait@4
__imp__UnregisterWaitEx@8:
    .long thunk_pUnregisterWaitEx@8

__imp__MultiByteToWideChar@24:
    .long _MultiByteToWideChar_wrapper@24

__imp__CreateThread@24:
    .long _MultiByteToWideChar_wrapper@24

.section .text
.align 4

_MultiByteToWideChar@24:
    jmp _MultiByteToWideChar_wrapper@24

_GetUserProfileDirectoryW@12:
    jmp *_pGetUserProfileDirectoryW

_CreateThread@24:
    jmp _CreateThread_wrapper@24

thunk_pRegisterWaitForSingleObject@24:
    jmp *_pRegisterWaitForSingleObject
thunk_pUnregisterWait@4:
    jmp *_pUnregisterWait
thunk_pUnregisterWaitEx@8:
    jmp *_pUnregisterWaitEx

.section .data

p_getaddrinfo:
    .long 0
p_freeaddrinfo:
    .long 0

.section .text
.align 4

.macro DEF_STUB name, ptr, msg
    .globl \name
\name:
    movl    \ptr, %eax              # Load the function pointer into %eax
    test    %eax, %eax              # Check if the pointer is NULL
    jne     jump_\name              # If not NULL, jump to the function implementation
    movl    $2, (%esp)              # Prepare the parameter (stderr)
    call    *__imp____acrt_iob_func # Retrieve the stream pointer
    movl    $\msg, (%esp)           # Place the error message address on the stack
    movl    %eax, 4(%esp)           # Place the stream pointer in the next stack position
    call    _fputs                  # Call fputs to print the error message
    jmp     _abort                  # Abort the program
jump_\name:
    jmp     *%eax                   # Jump to the function implementation via the pointer
.endm

DEF_STUB _AddVectoredExceptionHandler@8, _pAddVectoredExceptionHandler, missing_addveh
DEF_STUB _CreateHardLinkW@12, _pCreateHardLinkW, missing_createhardlink
DEF_STUB _CreateToolhelp32Snapshot@8, _pCreateToolhelp32Snapshot, missing_th32snap
DEF_STUB _Process32FirstW@8, _pProcess32FirstW, missing_proc32first
DEF_STUB _Process32NextW@8, _pProcess32NextW, missing_proc32next
DEF_STUB _Module32FirstW@8, _pModule32FirstW, missing_module32first
DEF_STUB _Module32NextW@8, _pModule32NextW, missing_module32next

DEF_STUB _getaddrinfo@16, p_getaddrinfo, missing_getaddrinfo
DEF_STUB _freeaddrinfo@4, p_freeaddrinfo, missing_freeaddrinfo
#endif // _WIN64
