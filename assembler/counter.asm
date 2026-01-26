
.main:
    imm     ra      0x10
.initloop:
    iadd    0xF     0xF      0x01
    sto     0xF
    cmp     0xF     ra
    jmpz    .end
    jmp     .initloop
.end:
    stop
