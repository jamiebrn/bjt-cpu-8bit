
.main:
    imm     ra      0x00
    imm     0xE     0x00
    imm     0xF     0x00
.loop:
    iadd    ra      ra      0x1
    iadd    0xF     ra      0x0
    sto     ra
    jmp     .loop

