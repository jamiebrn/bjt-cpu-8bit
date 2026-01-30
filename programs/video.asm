#include "stdlib/stdlib.asm"

main:
    imm     ra      0x00
    imm     rb      0x00
.loop:
    iadd    ra      ra      0x01
    iadd    rb      rb      0x01

    imm     rdis    0x01            ; clear display

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

    imm     rdis    0xCF            ; set pixel white

    jmp     .loop