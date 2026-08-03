        BasicUpstart2(start)
        .encoding "petscii_upper"

.label intro_src_ptr = $fb
.label intro_dst_ptr = $fd

        * = $0810
start:
        // Keep default ROM charset; avoids overwriting assembled data at $3800-$3fff.

        jsr load_ship_sprites

        lda $dd00
        and #$fc
        ora #$03
        sta $dd00

        lda $d011
        and #$9f        // clear ECM+BMM; preserve DEN
        sta $d011

        lda $d016
        and #$ef
        sta $d016

        // clear screen then print each custom char on its own line
        lda #$93
        jsr $ffd2

        jsr run_intro
        jmp done

done:
        // ask for name - MAIN.BAS lines 2010-2160
        lda #$93
        jsr $ffd2
        lda #$05        // white text
        jsr $ffd2
        lda #13
        jsr $ffd2
        lda #13
        jsr $ffd2
        
        ldx #0
prtask: lda askstr,x
        beq prtask_done
        jsr $ffd2
        inx
        bne prtask
prtask_done:
        lda #13
        jsr $ffd2
        lda #13
        jsr $ffd2

        jsr position_ship
        jsr show_ship

        ldx #0
prthvad:lda hvadstr,x
        beq prthvad_done
        jsr $ffd2
        inx
        bne prthvad
prthvad_done:

        lda #0
        sta $cc         // enable cursor blink
        ldx #0
inloop: jsr $ffe4
        beq inloop
        cmp #13
        beq indone
        cmp #20         // DEL
        beq indel
        cpx #15
        bcs inloop
        sta namebuf,x
        inx
        jsr $ffd2
        jmp inloop
indel:  cpx #0
        beq inloop
        dex
        lda #20
        jsr $ffd2
        jmp inloop
indone: lda #0
        sta namebuf,x
        lda #$ff
        sta $cc         // disable cursor blink
        cpx #0
        bne nameok
        ldx #0          // default to "KAPER"
defloop:lda kaperstr,x
        sta namebuf,x
        beq nameok
        inx
        bne defloop
nameok: lda #13
        jsr $ffd2
        jsr draw_map
        jsr position_ship
        jsr show_ship
        jsr game_loop
        rts

load_ship_sprites:
        lda #240
        sta $07f8
        lda #241
        sta $07f9
        lda #242
        sta $07fa
        lda #16
        sta $43f8
        lda #17
        sta $43f9
        lda #18
        sta $43fa

        lda #$0a
        sta $d025
        lda #$0f
        sta $d026

        lda #$02
        sta $d027
        lda #$0b
        sta $d028
        lda #$05
        sta $d029

        // Force a known sprite state: only sprites 0-2 are used by the ship.
        lda #$00
        sta $d010
        sta $d01c
        sta $d017
        sta $d01d
        sta $d01b

        ldy #0
shipcopy:
        lda ship_sprites,y
        sta $3c00,y
        lda ship_sprites,y
        sta $4400,y
        iny
        cpy #192
        bne shipcopy
        rts

position_ship:
        lda $d010
        and #$f8
        sta $d010
        lda ship_x_hi
        beq ship_x_low
        lda $d010
        ora #7
        sta $d010
ship_x_low:
        lda ship_x
        sta $d000
        sta $d002
        sta $d004
        lda ship_y
        sta $d001
        sta $d003
        sta $d005
        rts

show_ship:
        lda #$07
        sta $d015
        rts

hide_ship:
        lda #$00
        sta $d015
        rts

game_loop:
game_loop_wait:
        jsr $ffe4
        beq game_loop_wait
        cmp #17
        beq move_ship_down
        cmp #145
        beq move_ship_up
        cmp #29
        beq move_ship_right
        cmp #157
        beq move_ship_left
        jmp game_loop_wait

move_ship_up:
        lda ship_y
        cmp #10
        bcc move_ship_up_clamp
        sec
        sbc #10
        sta ship_y
        jsr position_ship
        jmp game_loop_wait
move_ship_up_clamp:
        lda #0
        sta ship_y
        jsr position_ship
        jmp game_loop_wait

move_ship_down:
        lda ship_y
        clc
        adc #10
        bcc move_ship_down_store
        lda #255
move_ship_down_store:
        sta ship_y
        jsr position_ship
        jmp game_loop_wait

move_ship_left:
        lda ship_x_hi
        bne move_ship_left_wide
        lda ship_x
        cmp #10
        bcc move_ship_left_clamp
        sec
        sbc #10
        sta ship_x
        jsr position_ship
        jmp game_loop_wait
move_ship_left_wide:
        lda ship_x
        sec
        sbc #10
        sta ship_x
        lda ship_x_hi
        sbc #0
        sta ship_x_hi
        jsr position_ship
        jmp game_loop_wait
move_ship_left_clamp:
        lda #0
        sta ship_x
        sta ship_x_hi
        jsr position_ship
        jmp game_loop_wait

move_ship_right:
        lda ship_x
        clc
        adc #10
        sta ship_x
        lda ship_x_hi
        adc #0
        cmp #2
        bcc move_ship_right_store
        bne move_ship_right_clamp
        lda ship_x
        cmp #54
        bcc move_ship_right_store
move_ship_right_clamp:
        lda #53
        sta ship_x
        lda #1
        sta ship_x_hi
        jsr position_ship
        jmp game_loop_wait
move_ship_right_store:
        sta ship_x_hi
        jsr position_ship
        jmp game_loop_wait

run_intro:
        // BASIC line 5: screen off while intro is prepared
        lda $d011
        and #$ef
        sta $d011

        // BASIC lines 20/30: intro colors
        lda #14
        sta $d020
        lda #6
        sta $d021

        lda #$93
        jsr $ffd2
        jsr draw_intro_screen
        jsr init_intro_music
        lda #0
        sta $c6
        lda $d011
        ora #$10
        sta $d011

intro_wait_key:
        jsr $ffe4
        bne intro_done

        ldx #5
intro_tick_loop:
        jsr advance_intro_music
        dex
        bne intro_tick_loop
        jsr wait_frame
        jmp intro_wait_key

intro_done:
        lda #0
        sta $d404
        lda #0
        sta $c6
        lda $d011
        ora #$10
        sta $d011
        rts

draw_intro_screen:
        lda #<intro_screen_data
        sta intro_src_ptr
        lda #>intro_screen_data
        sta intro_src_ptr+1
        lda #<$0400
        sta intro_dst_ptr
        lda #>$0400
        sta intro_dst_ptr+1
        jsr copy_1000_bytes

        lda #<intro_color_data
        sta intro_src_ptr
        lda #>intro_color_data
        sta intro_src_ptr+1
        lda #<$d800
        sta intro_dst_ptr
        lda #>$d800
        sta intro_dst_ptr+1
        jsr copy_1000_bytes
        rts

copy_1000_bytes:
        ldy #0
        ldx #3
copy_full_page:
        lda (intro_src_ptr),y
        sta (intro_dst_ptr),y
        iny
        bne copy_full_page
        inc intro_src_ptr+1
        inc intro_dst_ptr+1
        dex
        bne copy_full_page

        ldy #0
copy_tail_232:
        lda (intro_src_ptr),y
        sta (intro_dst_ptr),y
        iny
        cpy #232
        bne copy_tail_232
        rts

wait_frame:
        bit $d011
wait_frame_1:
        bit $d011
        bpl wait_frame_1
wait_frame_2:
        bit $d011
        bmi wait_frame_2
        rts

init_intro_music:
        lda #15
        sta $d418
        lda #9
        sta $d405
        lda #240
        sta $d406
        lda #0
        sta $d402
        lda #8
        sta $d403
        sta $d404
        sta intro_mi
        sta intro_mt
        sta intro_mp
        lda #1
        sta intro_mg
        sta intro_ph
        rts

advance_intro_music:
        lda intro_mg
        bne intro_mg_ok
        rts

intro_mg_ok:
        lda intro_mt
        beq intro_load_note
        dec intro_mt
        bne intro_tick_return
        lda intro_ph
        bne intro_tick_return
        lda intro_mp
        beq intro_tick_return
        lda #0
        sta $d404
        lda intro_mp
        sta intro_mt
        lda #1
        sta intro_ph
        jmp intro_tick_done

intro_tick_return:
        rts

intro_load_note:
        ldx intro_mi
        cpx #63
        bcc intro_have_note
        lda #0
        sta intro_mg
        sta $d404
        rts

intro_have_note:
        lda intro_freq_lo,x
        sta intro_f
        lda intro_freq_hi,x
        sta intro_f+1
        lda intro_dur,x
        clc
        adc #7
        lsr
        lsr
        lsr
        bne intro_tt_ok
        lda #1
intro_tt_ok:
        sta intro_tt

        inx
        stx intro_mi

        lda intro_f
        ora intro_f+1
        bne intro_play_note

        lda #0
        sta $d404
        lda intro_tt
        sta intro_mt
        lda #0
        sta intro_mp
        lda #1
        sta intro_ph
        rts

intro_play_note:
        lda intro_tt
        sta intro_mt
        lsr
        clc
        adc intro_mt
        lsr
        sta intro_mt
        bne intro_mt_ok
        lda #1
        sta intro_mt
intro_mt_ok:
        lda intro_tt
        sec
        sbc intro_mt
        sta intro_mp

        asl intro_f
        rol intro_f+1
        asl intro_f
        rol intro_f+1
        lda intro_f
        sta $d400
        lda intro_f+1
        sta $d401
        lda #$41
        sta $d404
        lda #0
        sta intro_ph

intro_tick_done:
        rts

askstr:  .byte 17
         .text "S]DAN SER DIT SKIB UD:"
         .byte 0
hvadstr: .text "HVAD ER DIT NAVN? "
         .byte 0
kaperstr:.text "KAPER"
         .byte 0
namebuf: .byte 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0
ship_x:  .byte 0
ship_x_hi:.byte 12
ship_y:  .byte 72

intro_mi: .byte 0
intro_mt: .byte 0
intro_mp: .byte 0
intro_mg: .byte 0
intro_ph: .byte 0
intro_tt: .byte 0
intro_f:  .word 0

intro_freq_lo:
        .byte <$1167,<$106d,<$1167,<$08b4,<$0000,<$08b4,<$0d0a,<$0b9d
        .byte <$0af7,<$0d0a,<$1167,<$0000,<$1167,<$15ed,<$1389,<$1167
        .byte <$1389,<$09c4,<$09c4,<$1389,<$1167,<$106d,<$09c4,<$0d0a
        .byte <$09c4,<$0d0a,<$0ea2,<$106d,<$1167,<$1389,<$1167,<$0ea2
        .byte <$0d0a,<$0d0a,<$0ea2,<$106d,<$0ea2,<$0d0a,<$0b9d,<$0af7
        .byte <$0b9d,<$0d0a,<$0b9d,<$0af7,<$09c4,<$08b4,<$09c4,<$08b4
        .byte <$0837,<$0751,<$0685,<$08b4,<$0837,<$09c4,<$08b4,<$0af7
        .byte <$09c4,<$0b9d,<$0af7,<$0000,<$0000
intro_freq_hi:
        .byte >$1167,>$106d,>$1167,>$08b4,>$0000,>$08b4,>$0d0a,>$0b9d
        .byte >$0af7,>$0d0a,>$1167,>$0000,>$1167,>$15ed,>$1389,>$1167
        .byte >$1389,>$09c4,>$09c4,>$1389,>$1167,>$106d,>$09c4,>$0d0a
        .byte >$09c4,>$0d0a,>$0ea2,>$106d,>$1167,>$1389,>$1167,>$0ea2
        .byte >$0d0a,>$0d0a,>$0ea2,>$106d,>$0ea2,>$0d0a,>$0b9d,>$0af7
        .byte >$0b9d,>$0d0a,>$0b9d,>$0af7,>$09c4,>$08b4,>$09c4,>$08b4
        .byte >$0837,>$0751,>$0685,>$08b4,>$0837,>$09c4,>$08b4,>$0af7
        .byte >$09c4,>$0b9d,>$0af7,>$0000,>$0000
intro_dur:
        .byte 29,29,57,29,29,57,29,29
        .byte 29,29,29,29,29,29,29,29
        .byte 57,57,57,29,29,29,29,29
        .byte 57,29,29,7,7,14,29,29
        .byte 29,7,7,14,29,29,29,7
        .byte 7,14,29,29,29,29,29,29
        .byte 29,29,29,29,29,29,29,29
        .byte 14,14,29,57,57

// Fallback labels so intro code can stay assembled while intro_screen include is disabled.
//.label intro_screen_data = 0
//.label intro_color_data  = 0

        // glyph data from MAIN.BAS lines 122-128
char_ae:     .byte 63,108,108,127,108,108,111,0
char_oe:     .byte 60,102,110,126,118,102,60,0
char_aa:     .byte 24,36,24,60,102,126,102,0
char_ship:   .byte 36,36,36,36,36,255,127,62

        // 3 sprites generated with spritemate on 01/08/2026, 12:48:00
        // Byte 64 of each sprite contains multicolor (high nibble) & color (low nibble) information
ship_sprites:
        // Red sails
        .byte $00,$00,$00,$2E,$00,$00,$6E,$00
        .byte $00,$6E,$00,$00,$E0,$00,$00,$00
        .byte $00,$00,$00,$00,$00,$00,$00,$00
        .byte $00,$00,$00,$00,$00,$00,$00,$00
        .byte $00,$00,$00,$00,$00,$00,$00,$00
        .byte $00,$00,$00,$00,$00,$00,$00,$00
        .byte $00,$00,$00,$00,$00,$00,$00,$00
        .byte $00,$00,$00,$00,$00,$00,$00,$0A
        // Grey mast
        .byte $10,$00,$00,$10,$00,$00,$10,$00
        .byte $00,$10,$00,$00,$10,$00,$00,$10
        .byte $00,$00,$00,$00,$00,$00,$00,$00
        .byte $00,$00,$00,$00,$00,$00,$00,$00
        .byte $00,$00,$00,$00,$00,$00,$00,$00
        .byte $00,$00,$00,$00,$00,$00,$00,$00
        .byte $00,$00,$00,$00,$00,$00,$00,$00
        .byte $00,$00,$00,$00,$00,$00,$00,$0F
        // Orange hull
        .byte $00,$00,$00,$00,$00,$00,$00,$00
        .byte $00,$00,$00,$00,$03,$80,$00,$EF
        .byte $80,$00,$7F,$80,$00,$7F,$00,$00
        .byte $00,$00,$00,$00,$00,$00,$00,$00
        .byte $00,$00,$00,$00,$00,$00,$00,$00
        .byte $00,$00,$00,$00,$00,$00,$00,$00
        .byte $00,$00,$00,$00,$00,$00,$00,$00
        .byte $00,$00,$00,$00,$00,$00,$00,$03

        .import source "intro_screen.inc"

        .import source "map.inc"

