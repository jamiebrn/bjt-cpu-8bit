#include "stdlib/stdlib.asm"

main:
    imm     ra      0x03
    imm     rc      0x08
.loop:
    call    std_shr
    sto     ra
    iadd    rb      rb      0x01
    iadd    radr    radr    0x01

    cmp     rb      rc
    jmpz    .resetcount
    jmp     .loop

.resetcount:
    imm     rb      0x00
    jmp     .loop