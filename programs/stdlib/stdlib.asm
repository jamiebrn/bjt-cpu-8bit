    ; --- standard library ---

std_multiply:
    push    rbnk                    ; retain reg
    push    radr
    push    rc
    imm     rbnk    0xFF            ; point in stack space
    imm     rc      0x00
    push    rc                      ; accumulator
    isub    radr    rsp     0x01    ; point to accumulator
.loop:
    lda     rc                      ; load accumulator and add
    add     rc      rc      ra
    sto     rc
    isub    rb      rb      0x01    ; dec counter
    imm     rc      0x00
    cmp     rb      rc
    jmpz    .end                    ; end counter == 0
    jmp     .loop
.end:
    pop     ra                      ; return accumulator
    pop     rc
    pop     radr
    pop     rbnk                    ; restore reg
    ret


std_memset:                         ; memset of ra, rb bytes from addr
    push    radr
    push    rc
    imm     rc      0x00
.loop:
    sto     ra
    iadd    radr    radr    0x01
    isub    rb      rb      0x01
    cmp     rb      rc
    jmpz    .end
    jmp     .loop
.end:
    pop     rc
    pop     radr
    ret


std_memcpy:                         ; memcpy of ra bytes, from addr to rb
    push    radr
    push    rc
    imm     rc      0x00            ; counter
.loop:
    push    ra
    ldrl    ra      radr    rc      ; load at radr + counter
    strla   rb      rc              ; store at rb + counter
    pop     ra
    iadd    rc      rc      0x01
    cmp     ra      rc
    jmpz    .end
    jmp     .loop
.end:
    pop     rc
    pop     radr
    ret
