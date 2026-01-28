#include "stdlib/stdlib.asm"

#define START_VAL   0x12
#define NUM_BYTES   0x08

main:
    imm     radr    0x04
    imm     ra      START_VAL
.loop:
    imm     rb      NUM_BYTES
    call    std_memset
    iadd    ra      ra      0x02
    jmp     .loop