
main:
    imm     ra      0x15
    imm     rb      0x00
initadr:
    imm     radr    0x00
initloop:
    iadd    rb       rb     0x01
    iadd    radr     radr   0x01
    sto     rb
    cmp     radr     ra
    jmpz    end
    jmp     initloop
end:
    jmp     initadr
