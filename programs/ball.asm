
    ; super epic ball bouncing program

#include "stdlib/stdlib.asm"
#include "stdlib/stdvideo.asm"

#define BALL_START_X 30
#define BALL_START_Y 42


ball_update:                        ; (pos, vel)
    push    rc

    imm     rc      0x01
    cmp     rb      rc
    jmpz    .ballvel_pos            ; if ball vel == 1
    jmp     .ballvel_neg

.ballvel_pos:
    imm     rc      0x3F
    cmp     ra      rc
    jmpz    .ballpos_posedge        ; if ball pos == 63 (edge)
    jmp     .ball_move

.ballpos_posedge:
    imm     rb      0xFF            ; ball vel = -1
    jmp     .ball_move

.ballvel_neg:
    imm     rc      0x00
    cmp     ra      rc
    jmpz    .ballpos_negedge        ; if ball pos == 0 (edge)
    jmp     .ball_move

.ballpos_negedge:
    imm     rb      0x01            ; ball vel = 1

.ball_move:
    add     ra      ra      rb

    pop     rc
    ret


main:
    imm     rbnk    0xFF

    imm     ra      BALL_START_X
    push    ra                      ; ball x pos (rbp + 0)
    imm     ra      BALL_START_Y
    push    ra                      ; ball y pos (rbp + 1)

    imm     ra      0x01
    push    ra                      ; ball x vel (rbp + 2)
    push    ra                      ; ball y vel (rbp + 3)

.loop:
    imm     ra      0
    ldrl    ra      rbp     ra      ; load ball x pos

    imm     rb      2
    ldrl    rb      rbp     rb      ; load ball x vel

    call    ball_update             ; update x

    imm     rc      0
    strla   rbp     rc              ; store updated ball x pos

    cpy     ra      rb
    imm     rc      2
    strla   rbp     rc              ; store updated ball x vel
    
    
    imm     ra      1
    ldrl    ra      rbp     ra      ; load ball y pos

    imm     rb      3
    ldrl    rb      rbp     rb      ; load ball y vel

    call    ball_update             ; update y

    imm     rc      1
    strla   rbp     rc              ; store updated ball y pos

    cpy     ra      rb
    imm     rc      3
    strla   rbp     rc              ; store updated ball y vel


    imm     rdis    0x01            ; clear display

    imm     ra      0
    ldrl    ra      rbp     ra      ; load ball x pos

    imm     rb      1
    ldrl    rb      rbp     rb      ; load ball y pos

    imm     rc      0xFF            ; set colour white

    call    stdv_draw_pixel

    jmp     .loop