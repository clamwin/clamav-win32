/*
 *  KernelEx
 *  Copyright (C) 2010, Xeno86
 *
 *  This file is part of KernelEx source code.
 *
 *  KernelEx is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published
 *  by the Free Software Foundation; version 2 of the License.
 *
 *  KernelEx is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with GNU Make; see the file COPYING.  If not, write to
 *  the Free Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA.
 *
 */

.equ CONTEXT_CONTEXTFLAGS, 0x00
.equ CONTEXT_SEGGS, 0x8c
.equ CONTEXT_SEGFS, 0x90
.equ CONTEXT_SEGES, 0x94
.equ CONTEXT_SEGDS, 0x98
.equ CONTEXT_EDI, 0x9c
.equ CONTEXT_ESI, 0xa0
.equ CONTEXT_EBX, 0xa4
.equ CONTEXT_EDX, 0xa8
.equ CONTEXT_ECX, 0xac
.equ CONTEXT_EAX, 0xb0
.equ CONTEXT_EBP, 0xb4
.equ CONTEXT_EIP, 0xb8
.equ CONTEXT_SEGCS, 0xbc
.equ CONTEXT_EFLAGS, 0xc0
.equ CONTEXT_ESP, 0xc4
.equ CONTEXT_SEGSS, 0xc8

.equ CONTEXT_FULL, 0x10007

.global _RtlCaptureContext@4

.section .text
.align 4

_RtlCaptureContext@4:
    pushl %ebx
    # Load cr parameter into ebx
    mov 8(%esp), %ebx

    mov %eax, CONTEXT_EAX(%ebx)
    # Get pushed ebx value
    mov (%esp), %eax
    mov %eax, CONTEXT_EBX(%ebx)
    mov %ecx, CONTEXT_ECX(%ebx)
    mov %edx, CONTEXT_EDX(%ebx)
    mov %esi, CONTEXT_ESI(%ebx)
    mov %edi, CONTEXT_EDI(%ebx)

    # Save segment registers
    data16 mov %cs, CONTEXT_SEGCS(%ebx)
    data16 mov %ds, CONTEXT_SEGDS(%ebx)
    data16 mov %es, CONTEXT_SEGES(%ebx)
    data16 mov %fs, CONTEXT_SEGFS(%ebx)
    data16 mov %gs, CONTEXT_SEGGS(%ebx)
    data16 mov %ss, CONTEXT_SEGSS(%ebx)

    # Save flags
    pushfw
    popw CONTEXT_EFLAGS(%ebx)

    # Save EBP, EIP, and ESP
    mov (%ebp), %eax
    mov %eax, CONTEXT_EBP(%ebx)
    mov 4(%ebp), %eax
    mov %eax, CONTEXT_EIP(%ebx)
    lea 8(%ebp), %eax
    mov %eax, CONTEXT_ESP(%ebx)

    # Set context flags
    movl $CONTEXT_FULL, CONTEXT_CONTEXTFLAGS(%ebx)

    # Function epilogue
    popl %ebx
    ret $4
