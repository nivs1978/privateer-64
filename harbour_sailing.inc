// Havneindl\bet (Harbour Approach)
//
// This module owns the final harbour-entry minigame.
//
// Visual notes from old MAIN.BAS 12030-12090:
// - light-red harbour walls frame a 38-column by 19-row channel;
// - the obstacle fleet grows by difficulty (IDIF * 5 ships);
// - the entrance marker is a 14-cell opening followed by a 10-cell gap and
//   a 15-cell closing wall; hitting it on the final step is fatal.
// - a wind gust can push the ship left or right during the approach.
//   (rolled and shown in the briefing, but not applied yet)

.const harbour_sailing_help_line_count = 10

// Show the original KAPER.BAS harbour-entry instructions.
// Call this before the sailing renderer when entering a harbour.
show_harbour_sailing_help:
        jsr prepare_text_notice_screen
        // BASIC 1260-1330: print the destination harbour in the opening line.
        pstr(harbour_sailing_line_1)
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
        pstrnl(harbour_sailing_wind_prefix_msg)
        lda $dc04
        and #1
        beq harbour_sailing_wind_from_left
        // "Fra h\jre" blows the ship to port, so the arrow shown is the one the
        // gust will push towards (BASIC 1330/1460: AV pairs with (IVIND-2)*10).
        lda #0
        sta harbour_sailing_wind_push_right
        lda #<harbour_sailing_wind_right_msg
        sta src_ptr
        lda #>harbour_sailing_wind_right_msg
        sta src_ptr+1
        lda #'_'
        tax
        jmp harbour_sailing_print_wind
harbour_sailing_wind_from_left:
        lda #1
        sta harbour_sailing_wind_push_right
        lda #<harbour_sailing_wind_left_msg
        sta src_ptr
        lda #>harbour_sailing_wind_left_msg
        sta src_ptr+1
        lda #'^'
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
        // $d01f latches collisions until read, and it has been accumulating since
        // the intro/map screens - discard those or the first step always "hits".
        lda $d01f
        lda #14
        sta $d020
        lda #17
        sta harbour_sailing_steps
harbour_sailing_auto_step:
        jsr harbour_sailing_delay_between_enters
        lda #13
        jsr $ffd2
        jsr harbour_sailing_maybe_gust
        // Carry from here must reach the bcc below - it decides the fatal
        // entrance miss on the final step, and dec leaves carry alone.
        jsr harbour_sailing_check_collision
        dec harbour_sailing_steps
        bne harbour_sailing_auto_step
        bcc harbour_sailing_run_done
        jmp harbour_entrance_lost
harbour_sailing_run_done:
        lda #14
        sta $d020
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
                jsr harbour_sailing_check_collision_after_move
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
                jsr harbour_sailing_check_collision_after_move
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
                // BASIC 1410 shortens the inter-step delay as IDIF climbs; IDIF is
                // 2 at the start and reaches 8 before the final promotion ends the game.
                lda #14
                sec
                sbc difficulty_level
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

        // BASIC 1440-1460: a gust fires on RND<=IDIF/18 and shoves the ship one
        // cell downwind. random_mod_16 is close enough to the 1-in-18 scale.
        harbour_sailing_maybe_gust:
                jsr random_mod_16
                cmp difficulty_level
                bcs harbour_sailing_gust_none
                jsr harbour_sailing_draw_gust
                lda harbour_sailing_wind_push_right
                bne harbour_sailing_gust_right
                jmp harbour_sailing_move_left
        harbour_sailing_gust_right:
                jmp harbour_sailing_move_right
        harbour_sailing_gust_none:
                rts

        // Three arrows on the row above the ship, scrolled away by the next step.
        harbour_sailing_draw_gust:
                lda harbour_sailing_ship_x
                sec
                sbc #24                     // sprite X 24 is screen column 0
                sta harbour_sailing_gust_col
                lda harbour_sailing_ship_x+1
                sbc #0
                lsr
                ror harbour_sailing_gust_col
                lsr
                ror harbour_sailing_gust_col
                lsr
                ror harbour_sailing_gust_col
                lda harbour_sailing_gust_col
                cmp #37                     // keep all three glyphs on the row
                bcc harbour_sailing_gust_col_ok
                lda #36
        harbour_sailing_gust_col_ok:
                tax
                lda #31                     // screen code of the left arrow
                ldy harbour_sailing_wind_push_right
                beq harbour_sailing_gust_glyph_ok
                lda #30                     // screen code of the right arrow
        harbour_sailing_gust_glyph_ok:
                sta INTRO_SCREEN_RAM+40,x
                sta INTRO_SCREEN_RAM+41,x
                sta INTRO_SCREEN_RAM+42,x
                rts

        // Steering writes $d000-$d004, but the VIC has not drawn the new position
        // yet - drop the bits latched at the old one and sample the next frame.
        harbour_sailing_check_collision_after_move:
                lda $d01f
                jsr harbour_sailing_wait_frame_raw
                jmp harbour_sailing_check_collision

        harbour_sailing_check_collision:
                lda $d01f
                and #$07
                beq harbour_sailing_no_collision
                lda #2
                sta $d020
                jsr sound_harbour_collision
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
                lda #14
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

        // BASIC 1390: the gap narrows as IDIF rises (9-IDIF/2 cells) and starts at
        // a random column, IB = (12-IDIF/2) + INT(1+RND*(6+IDIF)), 1-based.
        lda difficulty_level
        lsr
        sta harbour_sailing_entrance_half
        lda #9
        sec
        sbc harbour_sailing_entrance_half
        sta harbour_sailing_entrance_width
        jsr harbour_sailing_random_offset
        clc
        adc #12
        sec
        sbc harbour_sailing_entrance_half
        sta harbour_sailing_entrance_col

        ldx #0
harbour_sailing_entrance_wall:
        lda #160
        sta INTRO_SCREEN_RAM+$2f8,x
        inx
        cpx #39
        bne harbour_sailing_entrance_wall

        ldx harbour_sailing_entrance_col
        ldy harbour_sailing_entrance_width
harbour_sailing_entrance_gap:
        lda #32
        sta INTRO_SCREEN_RAM+$2f8,x
        inx
        dey
        bne harbour_sailing_entrance_gap
        rts

// 0..(5+IDIF), i.e. BASIC's INT(RND*(6+IDIF)). The span is at most 14, so a
// 4-bit sample with rejection is enough; $dc04 moves every cycle so it settles.
harbour_sailing_random_offset:
        lda difficulty_level
        clc
        adc #6
        sta harbour_sailing_entrance_span
harbour_sailing_random_offset_retry:
        lda $dc04
        and #$0f
        cmp harbour_sailing_entrance_span
        bcs harbour_sailing_random_offset_retry
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
harbour_sailing_wind_push_right: .byte 0
harbour_sailing_gust_col:   .byte 0
harbour_sailing_entrance_half:  .byte 0
harbour_sailing_entrance_width: .byte 0
harbour_sailing_entrance_col:   .byte 0
harbour_sailing_entrance_span:  .byte 0
