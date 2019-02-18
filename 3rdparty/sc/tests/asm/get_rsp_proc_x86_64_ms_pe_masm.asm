; ---
;  Copyright (c) 2016 Johan Sköld
;  License: https://opensource.org/licenses/ISC
; ---

EXTERN  sc_main_context:PROC
EXTERN  sc_yield:PROC

.code

get_rsp_proc PROC FRAME
    .endprolog

    ; First argument = uintptr_t*
    mov  [rcx], rsp

    ; Get the main context pointer
    call sc_main_context

    ; Yield to the main context
    xor  rdx, rdx
    mov  rcx, rax
    call sc_yield

    ret

get_rsp_proc ENDP

END
