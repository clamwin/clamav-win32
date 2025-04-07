COMMENT _
/*
 * Legacy Windows Compatibility Layer: Windows 7 (MSVC)
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
_


ifdef rax

public __imp_GetSystemTimePreciseAsFileTime
extern pGetSystemTimePreciseAsFileTime:PROC

.data
__imp_GetSystemTimePreciseAsFileTime dq OFFSET thunk_GetSystemTimePreciseAsFileTime

.code
thunk_GetSystemTimePreciseAsFileTime:
	jmp qword ptr [pGetSystemTimePreciseAsFileTime]

else

.model flat, stdcall

public _imp__GetSystemTimePreciseAsFileTime@4
extern pGetSystemTimePreciseAsFileTime:PROC

.data
_imp__GetSystemTimePreciseAsFileTime@4 dd OFFSET thunk_GetSystemTimePreciseAsFileTime@4

.code
thunk_GetSystemTimePreciseAsFileTime@4:
	jmp dword ptr [pGetSystemTimePreciseAsFileTime]

endif

end
