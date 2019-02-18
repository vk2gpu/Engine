; ---
;  Copyright (c) 2016 Johan Sköld
;  License: https://opensource.org/licenses/ISC
; ---

.code

get_xmm_register PROC FRAME
    .endprolog

    ; First argument (rcx) = register to get

    cmp  rcx,  6
    je  get_6
    cmp  rcx,  7
    je  get_7
    cmp  rcx,  8
    je  get_8
    cmp  rcx,  9
    je  get_9
    cmp  rcx,  10
    je  get_10
    cmp  rcx,  11
    je  get_11
    cmp  rcx,  12
    je  get_12
    cmp  rcx,  13
    je  get_13
    cmp  rcx,  14
    je  get_14
    jmp  get_15

get_6:
    movdqa  xmm0,  xmm6
    ret

get_7:
    movdqa  xmm0,  xmm7
    ret

get_8:
    movdqa  xmm0,  xmm8
    ret

get_9:
    movdqa  xmm0,  xmm9
    ret

get_10:
    movdqa  xmm0,  xmm10
    ret

get_11:
    movdqa  xmm0,  xmm11
    ret

get_12:
    movdqa  xmm0,  xmm12
    ret

get_13:
    movdqa  xmm0,  xmm13
    ret

get_14:
    movdqa  xmm0,  xmm14
    ret

get_15:
    movdqa  xmm0,  xmm15
    ret

get_xmm_register ENDP

END
