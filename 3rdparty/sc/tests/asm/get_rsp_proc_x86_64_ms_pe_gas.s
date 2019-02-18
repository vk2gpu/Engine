/*
 * Copyright (c) 2016 Johan Sköld
 * License: https://opensource.org/licenses/ISC
 */

.file	"get_rsp_proc_x86_64_ms_pe_gas.s"
.text
.p2align 4,,15
.globl	get_rsp_proc
.def	get_rsp_proc;	.scl	2;	.type	32;	.endef
.seh_proc	get_rsp_proc
get_rsp_proc:
.seh_endprologue

    /* First argument = uintptr_t* */
    movq %rsp, (%rcx)

    /* Get the main context pointer */
    call sc_main_context

    /* Yield to the main context */
    xorq %rdx, %rdx
    movq %rax, %rcx
    call sc_yield

    ret

.seh_endproc

.def	sc_yield;	        .scl	2;	.type	32;	.endef
.def	sc_main_context;	.scl	2;	.type	32;	.endef

.section .drectve
.ascii " -export:\"get_rsp_proc\""
