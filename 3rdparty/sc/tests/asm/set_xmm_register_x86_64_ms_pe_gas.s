/*
 * Copyright (c) 2016 Johan Sköld
 * License: https://opensource.org/licenses/ISC
 */

.file	"set_xmm_register_x86_64_ms_pe_gas.s"
.text
.p2align 4,,15
.globl	set_xmm_register
.def	set_xmm_register;	.scl	2;	.type	32;	.endef
.seh_proc	set_xmm_register
set_xmm_register:
.seh_endprologue

    /* First argument (rcx) = register to set */
    /* Second argument (rdx) = value to set it to */

    cmpq $6, %rcx
    je   set_6
    cmpq $7, %rcx
    je   set_7
    cmpq $8, %rcx
    je   set_8
    cmpq $9, %rcx
    je   set_9
    cmpq $10, %rcx
    je   set_10
    cmpq $11, %rcx
    je   set_11
    cmpq $12, %rcx
    je   set_12
    cmpq $13, %rcx
    je   set_13
    cmpq $14, %rcx
    je   set_14
    jmp  set_15

set_6:
    movdqa  (%rdx), %xmm6
    ret

set_7:
    movdqa  (%rdx), %xmm7
    ret

set_8:
    movdqa  (%rdx), %xmm8
    ret

set_9:
    movdqa  (%rdx), %xmm9
    ret

set_10:
    movdqa  (%rdx), %xmm10
    ret

set_11:
    movdqa  (%rdx), %xmm11
    ret

set_12:
    movdqa  (%rdx), %xmm12
    ret

set_13:
    movdqa  (%rdx), %xmm13
    ret

set_14:
    movdqa  (%rdx), %xmm14
    ret

set_15:
    movdqa  (%rdx), %xmm15
    ret

.seh_endproc

.section .drectve
.ascii " -export:\"set_xmm_register\""
