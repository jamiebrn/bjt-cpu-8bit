    ; --- video / display extension ---

stdv_draw_pixel:                    ; (x, y, colour)
    push    rc

    imm     rc      0x3F

    nand    ra      ra      rc
    nand    ra      ra      ra      ; bitwise and x cursor
    
    nand    rb      rb      rc
    nand    rb      rb      rb      ; bitwise and y cursor

    imm     rc      0x40            ; cursor x opcode
    
    push    ra
    nand    rc      rc      rc
    nand    ra      ra      ra
    nand    rc      ra      rc      ; rc = ra | rc
    pop     ra

    cpy     rdis    rc              ; set cursor x to ra

    imm     rc      0x80            ; cursor y opcode
    
    push    rb
    nand    rc      rc      rc
    nand    rb      rb      rb
    nand    rc      rb      rc      ; rc = rb | rc
    pop     rb

    cpy     rdis    rc              ; set cursor y to rb

    pop     rc
    cpy     rdis    rc              ; set pixel colour

    ret
