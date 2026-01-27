
main:
    imm     ra      0x10
.initloop:
    iadd    radr     radr      0x01
    sto     radr
    cmp     radr     ra
    jmpz    .end
    jmp     .initloop
.end:
    stop
