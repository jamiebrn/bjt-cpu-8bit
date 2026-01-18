
.main:
    imm     ra      0x00
    imm     rb      0x00
.loop:
    iadd    rb      rb      0x1
    addc    ra      ra      ra
    jmp     .loop