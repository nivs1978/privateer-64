// Havneindl\bet (Harbour Approach)
//
// This module owns the final harbour-entry minigame.  The presentation below
// is static for now; the sailing renderer and movement loop can replace it.
//
// Visual notes from old MAIN.BAS 12030-12090:
// - light-red harbour walls frame a 38-column by 19-row channel;
// - the obstacle fleet grows by difficulty (IDIF * 5 ships);
// - the entrance marker is a 14-cell opening followed by a 10-cell gap and
//   a 15-cell closing wall;
// - a wind gust can push the ship left or right during the approach.

.const harbour_sailing_help_line_count = 10

// Show the original KAPER.BAS harbour-entry instructions.
// Call this before the sailing renderer when entering a harbour.
show_harbour_sailing_help:
        jsr prepare_text_notice_screen
        // BASIC 1260-1330: print the destination harbour in the opening line.
        lda #<harbour_sailing_line_1
        sta src_ptr
        lda #>harbour_sailing_line_1
        sta src_ptr+1
        jsr print_zero_terminated
        lda last_harbour_id
        sec
        sbc #2
        tax
        lda map_harbour_name_ptr_lo,x
        sta src_ptr
        lda map_harbour_name_ptr_hi,x
        sta src_ptr+1
        jsr print_zero_terminated
        lda #'.'
        jsr $ffd2
        jsr print_newline
        ldx #0
harbour_sailing_help_line_loop:
        lda harbour_sailing_help_line_lo,x
        sta src_ptr
        lda harbour_sailing_help_line_hi,x
        sta src_ptr+1
        jsr print_zero_terminated
        jsr print_newline
        inx
        cpx #harbour_sailing_help_line_count
        bne harbour_sailing_help_line_loop
        jsr print_newline
        lda #<harbour_sailing_wind_prefix_msg
        sta src_ptr
        lda #>harbour_sailing_wind_prefix_msg
        sta src_ptr+1
        jsr print_zero_terminated
        jsr print_newline
        lda $dc04
        and #1
        beq harbour_sailing_wind_left
        lda #<harbour_sailing_wind_right_msg
        sta src_ptr
        lda #>harbour_sailing_wind_right_msg
        sta src_ptr+1
        lda #'^'
        tax
        jmp harbour_sailing_print_wind
harbour_sailing_wind_left:
        lda #<harbour_sailing_wind_left_msg
        sta src_ptr
        lda #>harbour_sailing_wind_left_msg
        sta src_ptr+1
        lda #'_'
        tax
harbour_sailing_print_wind:
        jsr print_zero_terminated
        jsr print_newline
        txa
        jsr $ffd2
        jsr print_newline
        jsr print_newline
        jmp harbour_sailing_wait_key

// Show the instructions, then the harbour and vessels, before trading.
harbour_sailing_approach:
        jsr show_harbour_sailing_help
        jsr harbour_sailing_draw_scene
        jsr harbour_sailing_wait_key
        jsr harbour_sailing_run
        clc
        rts

harbour_sailing_wait_key:
        jmp read_key_blocking

// Twenty ENTER advances. Arrow input is polled continuously between advances.
harbour_sailing_run:
        lda #<160
        sta harbour_sailing_ship_x
        lda #>160
        sta harbour_sailing_ship_x+1
        jsr harbour_sailing_update_ship_x
        ldx #0
        ldy #24
        jsr set_cursor_xy
        lda #17
        sta harbour_sailing_steps
harbour_sailing_auto_step:
        jsr harbour_sailing_delay_between_enters
        lda #13
        jsr $ffd2
        jsr harbour_sailing_check_collision
        dec harbour_sailing_steps
        bne harbour_sailing_auto_step
        bcc harbour_sailing_run_done
        jmp ship_sank
harbour_sailing_run_done:
        rts

        harbour_sailing_move_left:
                lda harbour_sailing_ship_x+1
                bne harbour_sailing_left_apply
                lda harbour_sailing_ship_x
                cmp #32
                bcc harbour_sailing_move_done
        harbour_sailing_left_apply:
                lda harbour_sailing_ship_x
                sec
                sbc #8
                sta harbour_sailing_ship_x
                lda harbour_sailing_ship_x+1
                sbc #0
                sta harbour_sailing_ship_x+1
                jsr harbour_sailing_update_ship_x
                jsr harbour_sailing_check_collision
        harbour_sailing_move_done:
                rts

        harbour_sailing_move_right:
                lda harbour_sailing_ship_x
                clc
                adc #8
                sta harbour_sailing_ship_x
                lda harbour_sailing_ship_x+1
                adc #0
                sta harbour_sailing_ship_x+1
                cmp #1
                bcc harbour_sailing_store_ship_x
                bne harbour_sailing_set_max_x
                lda harbour_sailing_ship_x
                cmp #73
                bcc harbour_sailing_store_ship_x
        harbour_sailing_set_max_x:
                lda #72
                sta harbour_sailing_ship_x
                lda #1
                sta harbour_sailing_ship_x+1
        harbour_sailing_store_ship_x:
                jsr harbour_sailing_update_ship_x
                jsr harbour_sailing_check_collision
                rts

        harbour_sailing_update_ship_x:
                jsr harbour_sailing_clamp_ship_x
                lda harbour_sailing_ship_x
                sta $d000
                sta $d002
                sta $d004
                lda harbour_sailing_ship_x+1
                beq harbour_sailing_clear_x_high
                lda $d010
                ora #$07
                sta $d010
                rts
        harbour_sailing_clear_x_high:
                lda $d010
                and #$f8
                sta $d010
                rts

        harbour_sailing_clamp_ship_x:
                lda harbour_sailing_ship_x+1
                beq harbour_sailing_clamp_low
                cmp #1
                bcc harbour_sailing_clamp_done
                bne harbour_sailing_clamp_max
                lda harbour_sailing_ship_x
                cmp #73
                bcc harbour_sailing_clamp_done
        harbour_sailing_clamp_max:
                lda #72
                sta harbour_sailing_ship_x
                lda #1
                sta harbour_sailing_ship_x+1
                rts
        harbour_sailing_clamp_low:
                lda harbour_sailing_ship_x
                cmp #32
                bcs harbour_sailing_clamp_done
                lda #32
                sta harbour_sailing_ship_x
                lda #0
                sta harbour_sailing_ship_x+1
        harbour_sailing_clamp_done:
                rts

        harbour_sailing_delay_between_enters:
                lda #12
                sta harbour_sailing_delay_frames
        harbour_sailing_delay_loop:
                jsr harbour_sailing_wait_frame_raw
                jsr $ffe4
                beq harbour_sailing_delay_next
                cmp #$9d
                beq harbour_sailing_delay_left
                cmp #$1d
                beq harbour_sailing_delay_right
                jmp harbour_sailing_delay_next
        harbour_sailing_delay_left:
                jsr harbour_sailing_move_left
                jmp harbour_sailing_delay_next
        harbour_sailing_delay_right:
                jsr harbour_sailing_move_right
        harbour_sailing_delay_next:
                dec harbour_sailing_delay_frames
                bne harbour_sailing_delay_loop
                rts

        harbour_sailing_wait_frame_raw:
                lda $d012
                cmp #250
                bne harbour_sailing_wait_frame_raw
        harbour_sailing_delay_release:
                lda $d012
                cmp #250
                beq harbour_sailing_delay_release
                rts

        harbour_sailing_check_collision:
                lda $d01f
                and #$07
                beq harbour_sailing_no_collision
                lda #2
                sta $d020
                jsr harbour_sailing_apply_damage
                sec
                rts
        harbour_sailing_apply_damage:
                lda #0
                ldx difficulty_level
        harbour_sailing_damage_loop:
                clc
                adc #15
                dex
                bne harbour_sailing_damage_loop
                sta damage_lo
                sec
                lda ship_repair_points
                sbc damage_lo
                sta ship_repair_points
                lda ship_repair_points+1
                sbc #0
                sta ship_repair_points+1
                bcc harbour_sailing_collision_clamp
                jsr check_ship_survival
                rts
harbour_sailing_collision_clamp:
                lda #0
                sta ship_repair_points
                sta ship_repair_points+1
                jsr check_ship_survival
                rts
        harbour_sailing_no_collision:
                lda #6
                sta $d020
                clc
                rts

harbour_sailing_draw_scene:
        jsr prepare_text_notice_screen
        // Shooting uses sprite 0 for its crosshair; restore the ship pointers.
        jsr load_ship_sprites
        lda #14
        sta $d020
        lda #6
        sta $d021
        // C64 sprite Y=50 is the visible screen top; harbour row 3 is 16 pixels down.
        // Therefore use hardware Y=66 rather than Y=16, which is above the display.
        lda #$00
        sta $d010
        lda #160
        sta $d000
        sta $d002
        sta $d004
        lda #66
        sta $d001
        sta $d003
        sta $d005
        lda #$07
        sta $d015

        // Clear the full 40x25 screen before drawing the channel.
        lda #$20
        ldx #0
harbour_sailing_clear_screen:
        sta INTRO_SCREEN_RAM,x
        sta INTRO_SCREEN_RAM+$100,x
        sta INTRO_SCREEN_RAM+$200,x
        sta INTRO_SCREEN_RAM+$300,x
        lda #8
        sta $d800,x
        sta $d900,x
        sta $da00,x
        sta $db00,x
        lda #$20
        inx
        bne harbour_sailing_clear_screen

        // The clear includes $47f8-$47ff, where the VIC sprite pointers live.
        jsr set_sprite_pointers_intro

        // Top row: one reverse block followed by 38 spaces.
        lda #<INTRO_SCREEN_RAM
        sta src_ptr
        lda #>INTRO_SCREEN_RAM
        sta src_ptr+1
        lda #160
        ldy #0
        sta (src_ptr),y
        iny
        lda #$20
harbour_sailing_top_row_spaces:
        sta (src_ptr),y
        iny
        cpy #39
        bne harbour_sailing_top_row_spaces
        lda #160
        sta (src_ptr),y

        // Remaining rows: reverse blocks at both edges with 38 spaces inside.
        clc
        lda src_ptr
        adc #40
        sta src_ptr
        lda src_ptr+1
        adc #0
        sta src_ptr+1
        ldx #1
harbour_sailing_wall_loop:
        lda #160
        ldy #0
        sta (src_ptr),y
        ldy #39
        sta (src_ptr),y
        ldy #1
        lda #$20
harbour_sailing_wall_spaces:
        sta (src_ptr),y
        iny
        cpy #39
        bne harbour_sailing_wall_spaces
        clc
        lda src_ptr
        adc #40
        sta src_ptr
        lda src_ptr+1
        adc #0
        sta src_ptr+1
        inx
        cpx #20
        bne harbour_sailing_wall_loop

        // BASIC 12075: FOR IS=0 TO ID*5, placing screen character 94.
        lda difficulty_level
        asl
        asl
        clc
        adc difficulty_level
        sta harbour_sailing_ship_count
        inc harbour_sailing_ship_count
harbour_sailing_ship_loop:
harbour_sailing_col_retry:
        jsr random_mod_40
        cmp #38
        bcs harbour_sailing_col_retry
        clc
        adc #1
        sta harbour_sailing_ship_col
harbour_sailing_row_retry:
        lda $dc05
        and #$1f
        cmp #19
        bcs harbour_sailing_row_retry
        sta harbour_sailing_ship_row

        // screen = $4400 + row*40 + column
        lda #<INTRO_SCREEN_RAM
        sta src_ptr
        lda #>INTRO_SCREEN_RAM
        sta src_ptr+1
        lda #<$d800
        sta dst_ptr
        lda #>$d800
        sta dst_ptr+1
        ldy harbour_sailing_ship_row
        beq harbour_sailing_ship_row_done
harbour_sailing_ship_row_loop:
        clc
        lda src_ptr
        adc #40
        sta src_ptr
        lda src_ptr+1
        adc #0
        sta src_ptr+1
        clc
        lda dst_ptr
        adc #40
        sta dst_ptr
        lda dst_ptr+1
        adc #0
        sta dst_ptr+1
        dey
        bne harbour_sailing_ship_row_loop
harbour_sailing_ship_row_done:
        ldy harbour_sailing_ship_col
        lda #94
        sta (src_ptr),y
        lda #2
        sta (dst_ptr),y
        dec harbour_sailing_ship_count
        bne harbour_sailing_ship_loop

        // BASIC 12090: 14 reverse cells, a 10-cell opening, then 15 reverse.
        ldx #0
harbour_sailing_entrance_left:
        lda #160
        sta INTRO_SCREEN_RAM+$2f8,x
        inx
        cpx #14
        bne harbour_sailing_entrance_left
        ldx #24
harbour_sailing_entrance_right:
        lda #160
        sta INTRO_SCREEN_RAM+$2f8,x
        inx
        cpx #39
        bne harbour_sailing_entrance_right
        rts

harbour_sailing_line_1:
        .text "DU ER P] VEJ IND I HAVNEN I "
        .byte 0
harbour_sailing_line_2:
        .text "STYR UDEN OM DE ANDRE SKIBE P] REDEN"
        .byte 0
harbour_sailing_line_3:
        .text "IND I HAVNEINDL\BET."
        .byte 0
harbour_sailing_line_4:
        .text "BRUG _ FOR AT FLYTTE TIL VENSTRE"
        .byte 0
harbour_sailing_line_5:
        .text "OG ^ FOR AT FLYTTE TIL H\JRE."
        .byte 0
harbour_sailing_line_blank:
        .byte 0
harbour_sailing_line_6:
        .text "HVIS DU ST\DER IND I ET"
        .byte 0
harbour_sailing_line_7:
        .text "AF DE ANDRE SKIBE VIL DIT SKIB"
        .byte 0
harbour_sailing_line_8:
        .text "BLIVE BESKADIGET, MEN HVIS DU"
        .byte 0
harbour_sailing_line_9:
        .text "RAMMER VED SIDEN AF HAVNEINDL\BET,"
        .byte 0
harbour_sailing_line_10:
        .text "S] OVERLEVER DU DET IKKE!"
        .byte 0
harbour_sailing_wind_prefix_msg:
        .text "PAS P]! DER KAN KOMME VINDST\D FRA"
        .byte 0
harbour_sailing_wind_left_msg:
        .text "VENSTRE!"
        .byte 0
harbour_sailing_wind_right_msg:
        .text "H\JRE!"
        .byte 0

harbour_sailing_help_line_lo:
        .byte <harbour_sailing_line_2, <harbour_sailing_line_3
        .byte <harbour_sailing_line_4, <harbour_sailing_line_5
        .byte <harbour_sailing_line_blank
        .byte <harbour_sailing_line_6, <harbour_sailing_line_7
        .byte <harbour_sailing_line_8, <harbour_sailing_line_9
        .byte <harbour_sailing_line_10
harbour_sailing_help_line_hi:
        .byte >harbour_sailing_line_2, >harbour_sailing_line_3
        .byte >harbour_sailing_line_4, >harbour_sailing_line_5
        .byte >harbour_sailing_line_blank
        .byte >harbour_sailing_line_6, >harbour_sailing_line_7
        .byte >harbour_sailing_line_8, >harbour_sailing_line_9
        .byte >harbour_sailing_line_10

// Static harbour presentation.  The vessels are intentionally simple text
// silhouettes; the future interactive renderer can replace this scene.
harbour_sailing_ship_count: .byte 0
harbour_sailing_ship_col:   .byte 0
harbour_sailing_ship_row:   .byte 0
harbour_sailing_ship_x:     .word 160
harbour_sailing_steps:      .byte 0
harbour_sailing_delay_frames: .byte 0
