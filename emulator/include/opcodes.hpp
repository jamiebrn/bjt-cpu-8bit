#pragma once

// Registers

#define REG_A       0x0
#define REG_B       0x1
#define REG_C       0x2
#define REG_SP      0xA
#define REG_BP      0xB
#define REG_BNK     0xE
#define REG_ADDR    0xF

#define REG_DIS     0x9

// Opcodes

#define OP_STOP     0x00
#define OP_RET      0x01
#define OP_PCALL    0x02
#define OP_POP      0x20
#define OP_LDA      0xF0
#define OP_PLDA     0x30

#define OP_ADD      0x40
#define OP_ADDC     0x50
#define OP_SUB      0x70
#define OP_SUBC     0x80
#define OP_IMM      0xA0
#define OP_NAND     0xB0
#define OP_PUSH     0x10
#define OP_STO      0x11
#define OP_CMP      0x12
#define OP_STRLA    0x60
#define OP_LDRL     0x90

#define OP_IADD     0xC0
#define OP_ISUB     0xD0
#define OP_JMP      0xE0
#define OP_JMPZ     0xE1
#define OP_JMPN     0xE2
#define OP_JMPC     0xE4
#define OP_JMPO     0xE8
#define OP_CALL     0xEA

// Mask
#define FLAG_ZBIT   0
#define FLAG_CBIT   1
#define FLAG_OBIT   2
#define FLAG_NBIT   3

#define FLAG_ZMASK(x)   (x & (0x1 << FLAG_ZBIT))
#define FLAG_CMASK(x)   (x & (0x1 << FLAG_CBIT))
#define FLAG_OMASK(x)   (x & (0x1 << FLAG_OBIT))
#define FLAG_NMASK(x)   (x & (0x1 << FLAG_NBIT))