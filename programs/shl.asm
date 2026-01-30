#include "stdlib/stdlib.asm"

main:
    imm     ra      0x03
.loop:
    imm     rb      0x01
    call    std_shlr
    jmp     .loop