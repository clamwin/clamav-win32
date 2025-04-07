/*
 * Legacy Windows Compatibility Layer: Windows 7 (GCC)
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


#ifdef _WIN64
.section .text

.align 8
thunk_GetSystemTimePreciseAsFileTime:
    jmp *pGetSystemTimePreciseAsFileTime(%rip)

.section .rdata,"dr"

.global GetSystemTimePreciseAsFileTime
.align 8

GetSystemTimePreciseAsFileTime:
    .quad thunk_GetSystemTimePreciseAsFileTime

#else // x86

.section .text

.align 4
thunk_GetSystemTimePreciseAsFileTime@4:
    jmp *_pGetSystemTimePreciseAsFileTime

.section .rdata,"dr"

.global _GetSystemTimePreciseAsFileTime@4
.align 4

_GetSystemTimePreciseAsFileTime@4:
    .long thunk_GetSystemTimePreciseAsFileTime@4

#endif
