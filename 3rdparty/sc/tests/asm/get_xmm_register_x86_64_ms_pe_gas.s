/*
 * Copyright (c) 2016 Johan Sköld
 * License: https://opensource.org/licenses/ISC
 */

.file	"get_xmm_register_x86_64_ms_pe_gas.s"
.text
.p2align 4,,15
.globl	get_xmm_register
.def	get_xmm_register;	.scl	2;	.type	32;	.endef
.seh_proc	get_xmm_register
get_xmm_register:
.seh_endprologue

    /* First argument (rcx) = register to get */

    cmpq $6, %rcx
    je   get_6
    cmpq $7, %rcx
    je   get_7
    cmpq $8, %rcx
    je   get_8
    cmpq $9, %rcx
    je   get_9
    cmpq $10, %rcx
    je   get_10
    cmpq $11, %rcx
    je   get_11
    cmpq $12, %rcx
    je   get_12
    cmpq $13, %rcx
    je   get_13
    cmpq $14, %rcx
    je   get_14
    jmp  get_15

get_6:
    movdqa  %xmm6,%xmm0
    ret

get_7:
    movdqa  %xmm7, %xmm0
    ret

get_8:
    movdqa  %xmm8, %xmm0
    ret

get_9:
    movdqa  %xmm9, %xmm0
    ret

get_10:
    movdqa  %xmm10, %xmm0
    ret

get_11:
    movdqa  %xmm11, %xmm0
    ret

get_12:
    movdqa  %xmm12, %xmm0
    ret

get_13:
    movdqa  %xmm13, %xmm0
    ret

get_14:
    movdqa  %xmm14, %xmm0
    ret

get_15:
    movdqa  %xmm15, %xmm0
    ret

.seh_endproc

.section .drectve
.ascii " -export:\"get_xmm_register\""
