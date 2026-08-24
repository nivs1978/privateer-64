// Interrupt-driven QBASIC PLAY string player.
//
// Public API:
//   audio_init                 Install the VIC raster IRQ.
//   audio_play                 A=low byte, X=high byte of a zero-terminated PLAY string.
//   audio_stop                 Stop playback and release SID voice 1.
//
// P rest, A-G notes, #/+/-. and dotted notes. M is accepted and ignored because
// (audio_note_ptr),y needs true zero page; $a7/$a8 is RS-232 workspace the game never uses.
.label audio_note_ptr = $a7
.const AUDIO_TIMER_LO = <19705
.const AUDIO_TIMER_HI = >19705
.const AUDIO_SID_GATE = $41
// Durations are counted in 2-jiffy ticks, matching the intro player's rate.
.const AUDIO_TICKS_PER_QUARTER = 7200
.const AUDIO_DEFAULT_TEMPO = 120
.const AUDIO_DEFAULT_LENGTH = 4
.const AUDIO_DEFAULT_OCTAVE = 4

// Install once during startup. The KERNAL IRQ remains chained after our tick.
audio_init:
        lda audio_installed
        beq audio_init_install
        rts
audio_init_install:
        sei
        lda #1
        sta audio_installed
        lda #15
        sta $d418
        lda #0
        sta $d415
        sta $d416
        sta $d417
        sta $d402
        lda #8
        sta $d403
        lda #9
        sta $d405
        lda #240
        sta $d406
        lda $0314
        sta audio_old_irq
        lda $0315
        sta audio_old_irq_hi
        lda #<audio_irq
        sta $0314
        lda #>audio_irq
        sta $0315
        cli
        rts

audio_start:
        sei
        lda #1
        sta audio_active
        lda #0
        sta audio_parser_state
        sta audio_note_remaining
        sta audio_note_rest
        sta audio_frame_toggle
        sta audio_command_value
        sta audio_note_length
        lda #AUDIO_DEFAULT_TEMPO
        sta audio_tempo
        lda #AUDIO_DEFAULT_LENGTH
        sta audio_default_length
        lda #AUDIO_DEFAULT_OCTAVE
        sta audio_octave
        lda audio_string_ptr
        sta audio_note_ptr
        lda audio_string_ptr+1
        sta audio_note_ptr+1
        lda #0
        sta $d404
        jsr audio_tick
        cli
        rts

// A=low byte, X=high byte. Starts the tune unless F2 has muted the game.
audio_play:
        sta audio_string_ptr
        stx audio_string_ptr+1
        lda sound_enabled
        beq audio_stop
        jmp audio_start

audio_stop:
        sei
        lda #0
        sta audio_active
        sta $d404
        cli
        rts

// Raster IRQ wrapper. The KERNAL IRQ continues to own CIA1 keyboard scanning
// and the system clock; this player only adds one music tick per raster frame.
audio_irq:
        pha
        txa
        pha
        tya
        pha
        lda audio_active
        beq audio_irq_done_tick
        lda audio_frame_toggle
        eor #1
        sta audio_frame_toggle
        bne audio_irq_done_tick
        jsr audio_tick
audio_irq_done_tick:
        pla
        tay
        pla
        tax
        pla
        jmp (audio_old_irq)

audio_tick:
        lda audio_note_remaining
        beq audio_next_event
        dec audio_note_remaining
        beq audio_next_event
        lda audio_note_remaining
        cmp audio_note_rest
        bne audio_tick_done
        lda #0
        sta $d404
audio_tick_done:
        rts

audio_next_event:
        jsr audio_read_command
        lda audio_active
        bne audio_tick_done
        rts

// Read commands until a note or rest is produced. This work occurs only once
// per event, while the actual note duration is counted by the IRQ tick.
audio_read_command:
        ldy #0
        lda (audio_note_ptr),y
        bne audio_command_nonempty
        jmp audio_end
audio_command_nonempty:
        inc audio_note_ptr
        bne audio_command_ready
        inc audio_note_ptr+1
audio_command_ready:
        // Lowercase encodes to $61-$7a; fold it onto the uppercase codes.
        cmp #$61
        bcc audio_command_case_done
        cmp #$7b
        bcs audio_command_case_done
        and #$df
audio_command_case_done:
        cmp #'T'
        beq audio_cmd_tempo
        cmp #'L'
        beq audio_cmd_length
        cmp #'O'
        beq audio_cmd_octave
        cmp #'M'
        beq audio_cmd_mode
        cmp #'P'
        beq audio_cmd_rest_far
        cmp #'N'
        beq audio_cmd_numeric
        cmp #'A'
        bcc audio_read_command
        cmp #'G'+1
        bcs audio_read_command
        jmp audio_cmd_note

audio_cmd_rest_far:
        jmp audio_cmd_rest

// MB/MS/MN/ML/MF carry a trailing letter that must not become a note.
audio_cmd_mode:
        jsr audio_advance_ptr
        jmp audio_read_command

audio_cmd_tempo:
        jsr audio_read_number
        lda audio_command_value
        bne audio_store_tempo
        lda #1
audio_store_tempo:
        sta audio_tempo
        jmp audio_read_command

audio_cmd_length:
        jsr audio_read_number
        lda audio_command_value
        bne audio_store_length
        lda #AUDIO_DEFAULT_LENGTH
audio_store_length:
        sta audio_default_length
        jmp audio_read_command

audio_cmd_octave:
        jsr audio_read_number
        lda audio_command_value
        cmp #1
        bcs audio_octave_min_ok
        jmp audio_read_command
audio_octave_min_ok:
        cmp #9
        bcc audio_octave_range_ok
        jmp audio_read_command
audio_octave_range_ok:
        sta audio_octave
        jmp audio_read_command

audio_cmd_numeric:
        jsr audio_read_number
        lda audio_command_value
        bne audio_numeric_value_ready
        jmp audio_read_command
audio_numeric_value_ready:
        cmp #85
        bcc audio_numeric_range_ok
        jmp audio_read_command
audio_numeric_range_ok:
        sec
        sbc #1
        sta audio_note_index
        jsr audio_read_length_and_dot
        lda audio_note_index
        jsr audio_play_index_a
        rts

audio_cmd_note:
        sec
        sbc #'A'
        tax
        lda audio_note_semitone,x
        sta audio_note_index
        jsr audio_read_accidental
        jsr audio_play_note_index
        rts

audio_cmd_rest:
        jsr audio_read_length_and_dot
        lda #0
        sta $d404
        rts

audio_read_accidental:
        ldy #0
        lda (audio_note_ptr),y
        cmp #'#'
        beq audio_accidental_up
        cmp #'+'
        beq audio_accidental_up
        cmp #'-'
        bne audio_read_length_and_dot
        dec audio_note_index
        jsr audio_advance_ptr
        jmp audio_read_length_and_dot
audio_accidental_up:
        inc audio_note_index
        jsr audio_advance_ptr

audio_read_length_and_dot:
        jsr audio_read_number_optional
        lda audio_command_value
        bne audio_have_length
        lda audio_default_length
audio_have_length:
        sta audio_note_length
        lda #0
        sta audio_dotted
        ldy #0
        lda (audio_note_ptr),y
        cmp #'.'
        bne audio_duration_from_length
        inc audio_note_ptr
        bne audio_set_dotted
        inc audio_note_ptr+1
audio_set_dotted:
        lda #1
        sta audio_dotted

audio_duration_from_length:
        // divisor = tempo * note length, kept 16-bit so it never wraps to zero.
        lda #0
        sta audio_divisor
        sta audio_divisor+1
        ldx audio_note_length
        beq audio_divisor_ready
audio_mul_loop:
        clc
        lda audio_divisor
        adc audio_tempo
        sta audio_divisor
        bcc audio_mul_no_carry
        inc audio_divisor+1
audio_mul_no_carry:
        dex
        bne audio_mul_loop
audio_divisor_ready:
        lda audio_divisor
        ora audio_divisor+1
        bne audio_divisor_ok
        lda #1
        sta audio_divisor
audio_divisor_ok:
        lda #<AUDIO_TICKS_PER_QUARTER
        sta audio_dividend
        lda #>AUDIO_TICKS_PER_QUARTER
        sta audio_dividend+1
        // Round to nearest tick: dividend += divisor / 2.
        lda audio_divisor+1
        lsr
        sta audio_length_temp
        lda audio_divisor
        ror
        clc
        adc audio_dividend
        sta audio_dividend
        lda audio_length_temp
        adc audio_dividend+1
        sta audio_dividend+1
        jsr audio_divide_16
        lda audio_quotient+1
        beq audio_duration_fits
        lda #255
        sta audio_quotient
audio_duration_fits:
        lda audio_quotient
        bne audio_duration_ready
        lda #1
audio_duration_ready:
        sta audio_note_remaining
        lda audio_dotted
        beq audio_duration_done
        lda audio_note_remaining
        lsr
        clc
        adc audio_note_remaining
        sta audio_note_remaining
audio_duration_done:
        // Intro split: tone is 3/4 of the note (min one tick), rest is what is left.
        lda audio_note_remaining
        lsr
        clc
        adc audio_note_remaining
        ror
        bne audio_tone_ok
        lda #1
audio_tone_ok:
        sta audio_length_temp
        lda audio_note_remaining
        sec
        sbc audio_length_temp
        sta audio_note_rest
        rts

audio_divide_16:
        lda #0
        sta audio_quotient
        sta audio_quotient+1
        sta audio_remainder
        sta audio_remainder+1
        ldx #16
audio_div_loop:
        asl audio_dividend
        rol audio_dividend+1
        rol audio_remainder
        rol audio_remainder+1
        lda audio_remainder+1
        cmp audio_divisor+1
        bcc audio_div_no_subtract
        bne audio_div_subtract
        lda audio_remainder
        cmp audio_divisor
        bcc audio_div_no_subtract
audio_div_subtract:
        sec
        lda audio_remainder
        sbc audio_divisor
        sta audio_remainder
        lda audio_remainder+1
        sbc audio_divisor+1
        sta audio_remainder+1
        sec
        rol audio_quotient
        rol audio_quotient+1
        jmp audio_div_next
audio_div_no_subtract:
        clc
        rol audio_quotient
        rol audio_quotient+1
audio_div_next:
        dex
        bne audio_div_loop
        rts

audio_play_note_index:
        lda audio_octave
        asl
        asl
        sta audio_octave_offset
        asl
        clc
        adc audio_octave_offset
        sta audio_octave_offset
        lda audio_note_index
        clc
        adc audio_octave_offset
audio_play_index_a:
        cmp #72
        bcc audio_note_index_ok
        lda #71
audio_note_index_ok:
        tax
        // Same F*4 SID conversion the intro player uses.
        lda audio_sid_freq_lo,x
        sta audio_freq_tmp
        lda audio_sid_freq_hi,x
        sta audio_freq_tmp+1
        asl audio_freq_tmp
        rol audio_freq_tmp+1
        asl audio_freq_tmp
        rol audio_freq_tmp+1
        lda audio_freq_tmp
        sta $d400
        lda audio_freq_tmp+1
        sta $d401
        lda #15
        sta $d418
        lda #AUDIO_SID_GATE
        sta $d404
        rts

audio_read_number:
        jsr audio_read_number_optional
        rts

audio_read_number_optional:
        lda #0
        sta audio_command_value
audio_number_loop:
        ldy #0
        lda (audio_note_ptr),y
        cmp #'0'
        bcc audio_number_done
        cmp #'9'+1
        bcs audio_number_done
        sec
        sbc #'0'
        sta audio_digit
        lda audio_command_value
        asl
        sta audio_length_temp
        asl
        asl
        clc
        adc audio_length_temp
        clc
        adc audio_digit
        sta audio_command_value
        inc audio_note_ptr
        bne audio_number_loop
        inc audio_note_ptr+1
        jmp audio_number_loop
audio_number_done:
        rts

audio_advance_ptr:
        inc audio_note_ptr
        bne audio_advance_ptr_done
        inc audio_note_ptr+1
audio_advance_ptr_done:
        rts

audio_end:
        lda #0
        sta audio_active
        sta $d404
        rts

// Note offsets are C=0, C#=1 ... B=11. Octave is added as 12 semitones.
audio_note_semitone:
        .byte 9,11,0,2,4,5,7

// SID values for C0 through B6. The parser indexes this table by semitone + octave*12.
audio_sid_freq_lo:
        .byte $16,$27,$39,$4b,$5f,$74,$8a,$a1,$ba,$d4,$f0,$0e
        .byte $2d,$4e,$71,$96,$be,$e7,$14,$42,$74,$a9,$e0,$1b
        .byte $5a,$9c,$e2,$2d,$7b,$cf,$27,$85,$e8,$51,$c1,$37
        .byte $b4,$38,$c4,$59,$f7,$9d,$4e,$0a,$d0,$a2,$81,$6d
        .byte $67,$70,$89,$b2,$ed,$3b,$9c,$13,$a0,$45,$02,$da
        .byte $ce,$e0,$11,$64,$da,$76,$39,$26,$40,$89,$04,$b4
        .byte $9c,$c0,$23,$c8,$b4,$eb,$72,$4c,$80,$12,$08,$68
audio_sid_freq_hi:
        .byte $01,$01,$01,$01,$01,$01,$01,$01,$01,$01,$01,$02
        .byte $02,$02,$02,$02,$02,$02,$03,$03,$03,$03,$03,$04
        .byte $04,$04,$04,$05,$05,$05,$06,$06,$06,$07,$07,$08
        .byte $08,$09,$09,$0a,$0a,$0b,$0c,$0d,$0d,$0e,$0f,$10
        .byte $11,$12,$13,$14,$15,$17,$18,$1a,$1b,$1d,$1f,$20
        .byte $22,$24,$27,$29,$2b,$2e,$31,$34,$37,$3a,$3e,$41
        .byte $45,$49,$4e,$52,$57,$5c,$62,$68,$6e,$75,$7c,$83

audio_active: .byte 0
audio_string_ptr: .word 0
audio_old_irq: .byte 0
audio_old_irq_hi: .byte 0
audio_parser_state: .byte 0
audio_tempo: .byte AUDIO_DEFAULT_TEMPO
audio_default_length: .byte AUDIO_DEFAULT_LENGTH
audio_octave: .byte AUDIO_DEFAULT_OCTAVE
audio_octave_offset: .byte 0
audio_note_remaining: .byte 0
audio_note_length: .byte 0
audio_note_index: .byte 0
audio_dotted: .byte 0
audio_command_value: .byte 0
audio_digit: .byte 0
audio_divisor: .word 1
audio_dividend: .word 0
audio_remainder: .word 0
audio_quotient: .word 0
audio_length_temp: .byte 0
audio_note_rest: .byte 0
audio_frame_toggle: .byte 0
sound_enabled: .byte 0
audio_freq_tmp: .word 0
audio_installed: .byte 0

// Extracted QBASIC PLAY strings.
play_intro_theme: .text "T230L8MBMSCO3BO4C4O3CP8C4GFEGO4CP8CEDCD4O3D4D4O4DCO3BDGDG4ABO4C32D32C16O3BAGA32B32A16GFEF32G32F16EDCDCO2BAGO3CO2BO3DCEDFE16F16EC4C4T230L8MBMSO3GFEGO4CO3GEGO4CO3GA16B16AF4F4P16T230L8MBMSO3AGF+AO4DO3AF+AO4DO3AB16O4C16O3BG4G4P16O3FGA32B32A16GFEF32G32F16EDCD32E32D16CO2BO3CDCO2BAGO3CO2BO3DCEDFE16F16EC4C4"
               .byte 0
play_ship_sinking: .text "O2T85MBMNL8B.A16G4D4G.B16A4.P8B.O3C16D4E4D.C16O2B4P4O3E.D16DCO2BAG2BAG2"
               .byte 0
play_fog_bank_warning: .text "MBT200O2L8GGGE-2.P8FFFD2."
               .byte 0
play_boarding_victory: .text "T180MBO2L4G.G8O3C1P4O2G.O3C8E1P4O2G.O3C8E2O2G.O3C8E2O2G.O3C8E1P2C.O3E8G1.O2G.G8O3C1"
               .byte 0
play_boarding_fight: .text "MBT200L16O3CEGO4C..O3GO4C4"
               .byte 0
play_promotion: .text "O3CEGO4CO3GEC2"
               .byte 0

// Short effects; the originals were SOUND/OUT sweeps rather than PLAY strings.
play_grounded: .text "MBT180O2L16GDC"
               .byte 0
play_harbour_collision: .text "MBT180O1L8G"
               .byte 0
play_flee: .text "MBT180O3L16GECO2G"
               .byte 0
play_cannon: .text "MBT200O1L16CG"
               .byte 0
play_splash: .text "MBT200O5L32CO4L32G"
               .byte 0
play_toggle_beep: .text "MBT200O4L16C"
               .byte 0
