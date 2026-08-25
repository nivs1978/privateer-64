.label intro_src_ptr = $fb
.label intro_dst_ptr = $fd

run_intro:
        // BASIC line 5: screen off while intro is prepared
        jsr hide_vic_screen

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
        jsr audio_init
        lda #0
        sta $c6
        jsr show_vic_screen
        jsr draw_intro_prompt

        lda #<play_intro_theme
        ldx #>play_intro_theme
        jsr audio_play

intro_wait_release:
        jsr $ff9f
        jsr $ffe4
        bne intro_wait_release

intro_wait_key:
        jsr $ff9f
        jsr $ffe4
        beq intro_wait_key

intro_done:
        jsr audio_stop
        lda #0
        sta $c6
        jsr show_vic_screen
        rts

// ---------------------------------------------------------------------------
// Gameplay SID sounds. Voice 1 is reserved for these short, blocking effects;
// the intro music is stopped before gameplay begins.

sound_toggle:
        lda sound_enabled
        eor #1
        sta sound_enabled
        beq sound_toggle_off
        lda #<play_toggle_beep
        ldx #>play_toggle_beep
        jmp audio_play
sound_toggle_off:
        jmp audio_stop

sound_grounded:
        lda #<play_grounded
        ldx #>play_grounded
        jmp audio_play

sound_harbour_collision:
        lda #<play_harbour_collision
        ldx #>play_harbour_collision
        jmp audio_play

sound_flee:
        lda #<play_flee
        ldx #>play_flee
        jmp audio_play

sound_cannon:
        lda #<play_cannon
        ldx #>play_cannon
        jmp audio_play

sound_splash:
        lda #<play_splash
        ldx #>play_splash
        jmp audio_play

sound_fog:
        lda #<play_fog_bank_warning
        ldx #>play_fog_bank_warning
        jmp audio_play

sound_surrender:
        lda #<play_boarding_victory
        ldx #>play_boarding_victory
        jmp audio_play

sound_sink:
        lda #<play_ship_sinking
        ldx #>play_ship_sinking
        jmp audio_play

sound_boarding:
        lda #<play_boarding_fight
        ldx #>play_boarding_fight
        jmp audio_play

sound_promotion:
        lda #<play_promotion
        ldx #>play_promotion
        jmp audio_play

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
        lda press_any_key_msg,x
        beq draw_intro_prompt_done
        jsr $ffd2
        inx
        bne draw_intro_prompt_text
draw_intro_prompt_done:
        lda #19
        jsr $ffd2
        rts

hide_vic_screen:
        lda $d011
        and #$ef
        sta $d011
        rts

show_vic_screen:
        lda $d011
        ora #$10
        sta $d011
        rts

configure_intro_vic_bank:
        // VIC bank 1 ($4000-$7fff), screen $4400, charset $4800.
        jsr hide_vic_screen
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
        // Also patch ^=right arrow (screen code 30); _ keeps its default left-arrow glyph.
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

        ldy #0
charset_patch_right_arrow:
        lda char_right_arrow,y
        sta CHAR_SLOT_RIGHT_ARROW_ADDR,y
        iny
        cpy #8
        bne charset_patch_right_arrow

        ldy #0
charset_patch_harbour_ship:
        lda char_harbour_ship,y
        sta CHAR_SLOT_HARBOUR_SHIP_ADDR,y
        iny
        cpy #8
        bne charset_patch_harbour_ship
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

// Fallback labels so intro code can stay assembled while intro_screen include is disabled.
//.label intro_screen_data = 0
//.label intro_color_data  = 0

// glyph data from MAIN.BAS lines 122-128
char_ae:     .byte 63,108,108,127,108,108,111,0
char_oe:     .byte 60,102,110,126,118,102,60,0
char_aa:     .byte 24,36,24,60,102,126,102,0
char_right_arrow: .byte 0,4,6,127,127,6,4,0
char_harbour_ship: .byte 36,36,36,36,36,255,126,126
