/*
 * Legacy Windows Compatibility Layer: fiber API import pointers
 *
 * Copyright (c) 2026 Gianluigi Tiesi <sherpya@gmail.com>
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

.section .rdata,"dr"

#ifdef _WIN64
.align 8

.global __imp_FlsAlloc
.global __imp_FlsFree
.global __imp_FlsSetValue
.global __imp_IsThreadAFiber

__imp_FlsAlloc:
    .quad FlsAlloc
__imp_FlsFree:
    .quad FlsFree
__imp_FlsSetValue:
    .quad FlsSetValue
__imp_IsThreadAFiber:
    .quad IsThreadAFiber

#else
.align 4

.global __imp__FlsAlloc@4
.global __imp__FlsFree@4
.global __imp__FlsSetValue@8
.global __imp__IsThreadAFiber@0

__imp__FlsAlloc@4:
    .long _FlsAlloc@4
__imp__FlsFree@4:
    .long _FlsFree@4
__imp__FlsSetValue@8:
    .long _FlsSetValue@8
__imp__IsThreadAFiber@0:
    .long _IsThreadAFiber@0

#endif
