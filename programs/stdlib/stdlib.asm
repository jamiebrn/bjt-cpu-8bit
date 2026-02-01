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


std_shl:                            ; left shift of ra by rb bits
    push    rc
    imm     rc      0x00

.loop:
    add     ra      ra      ra

    isub    rb      rb      0x01

    cmp     rb      rc

    jmpz    .end
    jmp     .loop

.end:
    pop     rc
    ret


std_shlr:                           ; left rotate of ra by rb bits
    push    rc
    imm     rc      0x00

.loop:
    add     ra      ra      ra

    jmpc    .addbit
    jmp     .subcounter

.addbit:
    iadd    ra      ra      0x01    ; add rotated bit

.subcounter:
    isub    rb      rb      0x01
    cmp     rb      rc

    jmpz    .end
    jmp     .loop

.end:
    pop     rc
    ret


std_shr:                            ; right shift of ra by rb bits
    push    rc

    imm     rc      0x08
    sub     rc      rc      rb
    cpy     rb      rc

    call    std_shlr                ; rotate left by 8 - rb bits
    
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
    push    rc

    imm     rc      0x00
    
    push    rc                      ; counter (rbp + 2)
    push    ra                      ; byte count (rbp + 3)

.loop:
    ldrl    ra      radr    rc      ; load at radr + counter
    
    ldrl    rc      rb      rc      ; load at rb + counter

    cmp     ra      rc
    jmpz    .equal

    imm     ra      0x00            ; not equal, return 0
    jmp     .end

.equal:
    push    rbnk
    imm     rbnk    0xFF            ; point in stack

    imm     ra      2
    ldrl    ra      rbp     ra      ; load counter from stack
    
    iadd    ra      ra      0x01
    
    imm     rc      3
    ldrl    rc      rbp     rc      ; load byte count from stack

    cmp     rc      ra
    jmpz    .allequal               ; if counter == byte count

    imm     rc      2
    strla   rbp     rc              ; store counter

    pop     rbnk

    jmp     .loop

.allequal:
    imm     ra      0x01            ; return 1

    pop     rbnk

.end:
    pop     rc
    pop     rc                      ; clean up
    
    pop     rc                      ; restore

    ret