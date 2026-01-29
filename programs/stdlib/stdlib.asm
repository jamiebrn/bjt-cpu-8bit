    ; --- standard library ---

std_multiply:
    push    rc
    imm     rc      0x00            ; accumulator
.loop:
    add     rc      rc      ra      ; add to accumulator
    isub    rb      rb      0x01    ; dec counter
    push    rc
    imm     rc      0x00
    cmp     rb      rc
    pop     rc
    jmpz    .end                    ; end counter == 0
    jmp     .loop
.end:
    cpy     ra      rc              ; return accumulator
    pop     rc
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

std_memcmp:                         ; memcmp of ra bytes, starting at addr and rb
    push    radr
    push    rc
    imm     rc      0x00            ; counter
.loop:
    push    ra
    ldrl    ra      radr    rc      ; load at radr + counter
    push    ra
    ldrl    ra      rb      rc      ; load at rb + counter
    
    pop     ra
    iadd    rc      rc      0x01
    cmp     ra      rc
    jmpz    .end
    jmp     .loop
.end:
    pop     rc
    pop     radr
    ret