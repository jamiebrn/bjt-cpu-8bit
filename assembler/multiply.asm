
multiply:
    push    rbnk                    ; retain reg
    push    radr
    push    rc
    imm     rbnk    0xFF            ; point in stack space
    imm     rc      0x00
    push    rc                      ; accumulator
    isub    radr    rsp     0x01    ; point to accumulator
multiplyloop:
    lda     rc                      ; load accumulator and add
    add     rc      rc      ra
    sto     rc
    isub    rb      rb      0x01    ; dec counter
    imm     rc      0x00
    cmp     rb      rc
    jmpz    multiplyend             ; end counter == 0
    jmp     multiplyloop
multiplyend:
    pop     ra                      ; return accumulator
    pop     rc
    pop     radr
    pop     rbnk                    ; restore reg
    ret

main:
    imm     ra      0x12
    imm     rb      0x05
    call    multiply
    stop

