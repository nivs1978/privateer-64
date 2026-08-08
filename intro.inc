.label intro_src_ptr = $fb
.label intro_dst_ptr = $fd
.const intro_note_count = 63
.const intro_sp = 8
// BASIC used SR=5, but interpreted BASIC overhead made effective playback slower.
// In machine code we step much faster, so use 1 tick/jiffy for comparable duration.
.const intro_sr = 1
.const intro_tick_jiffies = 2

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
        jsr configure_intro_vic_bank
        jsr draw_intro_screen
        jsr prepare_intro_charset
        jsr init_intro_music
        lda #0
        sta $c6
        lda $d011
        ora #$10
        sta $d011
        jsr draw_intro_prompt
        lda $a2
        sta intro_last_ti

intro_wait_release:
        jsr $ff9f
        jsr $ffe4
        bne intro_wait_release

intro_wait_key:
        jsr $ff9f
        jsr $ffe4
        bne intro_done
        lda $a2
        cmp intro_last_ti
        beq intro_wait_key
        sta intro_last_ti
        inc intro_tick_acc
        lda intro_tick_acc
        cmp #intro_tick_jiffies
        bcc intro_wait_key
        lda #0
        sta intro_tick_acc
        ldy #intro_sr
intro_tick_loop:
        jsr advance_intro_music
        dey
        bne intro_tick_loop
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
        lda #<INTRO_SCREEN_RAM
        sta intro_dst_ptr
        lda #>INTRO_SCREEN_RAM
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

draw_intro_prompt:
        // Print prompt from .text at row 24, col 0 in intro screen.
        lda #19
        jsr $ffd2
        ldx #24
draw_intro_prompt_down:
        lda #17
        jsr $ffd2
        dex
        bne draw_intro_prompt_down
        lda #152
        jsr $ffd2
        ldx #0
draw_intro_prompt_text:
        lda intro_prompt_text,x
        beq draw_intro_prompt_done
        jsr $ffd2
        inx
        bne draw_intro_prompt_text
draw_intro_prompt_done:
        lda #19
        jsr $ffd2
        rts

configure_intro_vic_bank:
        // VIC bank 1 ($4000-$7fff), screen $4400, charset $4800.
        lda $dd00
        and #$fc
        ora #VIC_BANK1_SELECT
        sta $dd00
        lda #VIC_D018_INTRO
        sta $d018
        // KERNAL screen editor should also target $4400 (high byte $44).
        lda #KERNAL_SCREEN_PAGE_INTRO
        sta KERNAL_SCREEN_PAGE_ADDR
        rts

restore_main_vic_bank:
        // Restore default VIC bank 0 and screen/charset pointer.
        lda $dd00
        and #$fc
        ora #VIC_BANK0_SELECT
        sta $dd00
        lda #VIC_D018_MAIN
        sta $d018
        lda #KERNAL_SCREEN_PAGE_MAIN
        sta KERNAL_SCREEN_PAGE_ADDR
        rts

prepare_intro_charset:
        // Copy uppercase ROM charset ($D000-$D7FF) to RAM at $4800.
        sei
        lda $01
        pha
        and #$fb
        sta $01

        lda #$00
        sta intro_src_ptr
        sta intro_dst_ptr
        lda #$d0
        sta intro_src_ptr+1
        lda #>INTRO_CHARSET_RAM
        sta intro_dst_ptr+1

        ldx #8
        ldy #0
charset_copy_page:
        lda (intro_src_ptr),y
        sta (intro_dst_ptr),y
        iny
        bne charset_copy_page
        inc intro_src_ptr+1
        inc intro_dst_ptr+1
        dex
        bne charset_copy_page

        pla
        sta $01
        cli

        // Patch custom Danish glyphs: [=AE, \=OE, ]=AA (Danish AE/OE/AA; screen codes 27/28/29).
        ldy #0
charset_patch_ae:
        lda char_ae,y
        sta CHAR_SLOT_AE_ADDR,y
        iny
        cpy #8
        bne charset_patch_ae

        ldy #0
charset_patch_oe:
        lda char_oe,y
        sta CHAR_SLOT_OE_ADDR,y
        iny
        cpy #8
        bne charset_patch_oe

        ldy #0
charset_patch_aa:
        lda char_aa,y
        sta CHAR_SLOT_AA_ADDR,y
        iny
        cpy #8
        bne charset_patch_aa
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
        // BASIC 5000-5080
        lda #15
        sta $d418
        lda #0
        sta $d415
        sta $d416
        sta $d417
        lda #9
        sta $d405
        lda #240
        sta $d406
        lda #0
        sta $d402
        lda #8
        sta $d403
        lda #0
        sta intro_mi
        sta intro_mt
        sta intro_mp
        lda #1
        sta intro_mg
        sta intro_ph
        lda #0
        sta $d404
        sta intro_tick_acc
        rts

advance_intro_music:
        // BASIC 5100-5210
        lda intro_mg
        bne intro_mg_ok
        rts

intro_mg_ok:
        lda intro_mt
        beq intro_after_mt
        dec intro_mt
        bne intro_mt_still_active
        lda intro_ph
        bne intro_after_mt
        lda intro_mp
        beq intro_after_mt
        lda #0
        sta $d404
        lda intro_mp
        sta intro_mt
        lda #1
        sta intro_ph
intro_mt_still_active:
        rts

intro_after_mt:
        lda intro_mt
        beq intro_after_hold_check
        rts

intro_after_hold_check:
        ldx intro_mi
        cpx #intro_note_count
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

        // TT = INT((D + SP - 1) / SP)
        clc
        adc #intro_sp-1
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

        // Rest note
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
        // MT = INT((TT*3)/4)
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

        // MP = TT - MT
        lda intro_tt
        sec
        sbc intro_mt
        sta intro_mp

        // F2 = F*4
        asl intro_f
        rol intro_f+1
        asl intro_f
        rol intro_f+1

        lda intro_f
        sta $d400
        lda intro_f+1
        sta $d401
        lda #15
        sta $d418
        lda #$41
        sta $d404
        lda #0
        sta intro_ph
        rts

intro_freq_lo:
        .byte <$1167,<$106d,<$1167,<$08b4,<$0000,<$08b4,<$0d0a,<$0b9d
        .byte <$0af7,<$0d0a,<$1167,<$0000,<$1167,<$15ed,<$1389,<$1167
        .byte <$1389,<$09c4,<$09c4,<$1389,<$1167,<$106d,<$09c4,<$0d0a
        .byte <$09c4,<$0d0a,<$0ea2,<$106d,<$1167,<$1389,<$1167,<$106d
        .byte <$0ea2,<$0d0a,<$0ea2,<$106d,<$0ea2,<$0d0a,<$0b9d,<$0af7
        .byte <$0b9d,<$0d0a,<$0b9d,<$0af7,<$09c4,<$08b4,<$09c4,<$08b4
        .byte <$0837,<$0751,<$0685,<$08b4,<$0837,<$09c4,<$08b4,<$0af7
        .byte <$09c4,<$0b9d,<$0af7,<$0b9d,<$0af7,<$08b4,<$08b4
intro_freq_hi:
        .byte >$1167,>$106d,>$1167,>$08b4,>$0000,>$08b4,>$0d0a,>$0b9d
        .byte >$0af7,>$0d0a,>$1167,>$0000,>$1167,>$15ed,>$1389,>$1167
        .byte >$1389,>$09c4,>$09c4,>$1389,>$1167,>$106d,>$09c4,>$0d0a
        .byte >$09c4,>$0d0a,>$0ea2,>$106d,>$1167,>$1389,>$1167,>$106d
        .byte >$0ea2,>$0d0a,>$0ea2,>$106d,>$0ea2,>$0d0a,>$0b9d,>$0af7
        .byte >$0b9d,>$0d0a,>$0b9d,>$0af7,>$09c4,>$08b4,>$09c4,>$08b4
        .byte >$0837,>$0751,>$0685,>$08b4,>$0837,>$09c4,>$08b4,>$0af7
        .byte >$09c4,>$0b9d,>$0af7,>$0b9d,>$0af7,>$08b4,>$08b4
intro_dur:
        .byte 29,29,57,29,29,57,29,29
        .byte 29,29,29,29,29,29,29,29
        .byte 57,57,57,29,29,29,29,29
        .byte 29,57,29,29,7,7,14,29
        .byte 29,29,7,7,14,29,29,29
        .byte 7,7,14,29,29,29,29,29
        .byte 29,29,29,29,29,29,29,29
        .byte 29,29,14,14,29,57,57

intro_mi: .byte 0
intro_mt: .byte 0
intro_mp: .byte 0
intro_mg: .byte 0
intro_ph: .byte 0
intro_tt: .byte 0
intro_f:  .word 0
intro_last_ti: .byte 0
intro_tick_acc: .byte 0

intro_prompt_text:
        .text "TRYK P] EN TAST FOR AT FORTS[TTE!"
        .byte 0

// Fallback labels so intro code can stay assembled while intro_screen include is disabled.
//.label intro_screen_data = 0
//.label intro_color_data  = 0

// glyph data from MAIN.BAS lines 122-128
char_ae:     .byte 63,108,108,127,108,108,111,0
char_oe:     .byte 60,102,110,126,118,102,60,0
char_aa:     .byte 24,36,24,60,102,126,102,0
