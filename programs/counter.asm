
.main:
    imm     ra      0x10
.initloop:
    iadd    0xF     0xF      0x0
    sto     0xF
    cmp     0xF     ra
    jmpz    .double
    jmp     .initloop
.double:
    imm     0xF     0x00
