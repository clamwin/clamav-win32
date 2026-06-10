/*
 * Legacy Windows Compatibility Layer: common
 *
 * Copyright (c) 2025-2026 Gianluigi Tiesi <sherpya@gmail.com>
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

.global __imp_WakeByAddressAll
.global __imp_WaitOnAddress
.global __imp_WakeByAddressSingle
.global __imp_ProcessPrng

.global __imp_InitOnceExecuteOnce
.global __imp__InitOnceExecuteOnce@16

.global GetHostNameW
.global _GetHostNameW@8

.global __imp_ReOpenFile
.global __imp_GetTimeZoneInformationForYear

.global __imp__wassert
.global __imp__freopen_s
.global __imp__mbstowcs_s
.global __imp__wcscpy_s
.global __imp__wcsncpy_s
.global __imp__wcstombs_s

.section .rdata,"dr"

.align 4

__imp_WakeByAddressAll:
    .long _WakeByAddressAll@4
__imp_WaitOnAddress:
    .long _WaitOnAddress@16
__imp_WakeByAddressSingle:
    .long _WakeByAddressSingle@4
__imp_ProcessPrng:
    .long _ProcessPrng@8
__imp__InitOnceExecuteOnce@16:
    .long _InitOnceExecuteOnce@16

__imp__freopen_s:
    .long _freopen_s_compat
__imp__mbstowcs_s:
    .long _mbstowcs_s_compat
__imp__wcscpy_s:
    .long _wcscpy_s_compat
__imp__wcsncpy_s:
    .long _wcsncpy_s_compat
__imp__wcstombs_s:
    .long _wcstombs_s_compat

.section .text

.align 4

_GetHostNameW@8:
    jmp *_pGetHostNameW

__imp_ReOpenFile:
    .long _ReOpenFile@16

__imp_GetTimeZoneInformationForYear:
    .long _GetTimeZoneInformationForYear@12
