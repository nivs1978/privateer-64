// Shooting minigame screen display.
// Screen data from C:\source\Kaper64 BASIC\shooting.asm (petmate export, 40x25 C64 upper).
// Border 14 (light blue), background 6 (dark blue).

run_shooting_minigame:
        // outer on_random_sea_event already set VIC bank 1 text mode at $4400
        lda $d011
        and #$df        // ensure BMM cleared (text mode)
        sta $d011

        lda #14
        sta $d020
        lda #6
        sta $d021

        lda #$00
        sta $d015       // hide all sprites while displaying screen

        lda #<shooting_screen_data
        sta intro_src_ptr
        lda #>shooting_screen_data
        sta intro_src_ptr+1
        lda #<INTRO_SCREEN_RAM
        sta intro_dst_ptr
        lda #>INTRO_SCREEN_RAM
        sta intro_dst_ptr+1
        jsr copy_1000_bytes

        lda #<shooting_color_data
        sta intro_src_ptr
        lda #>shooting_color_data
        sta intro_src_ptr+1
        lda #<$d800
        sta intro_dst_ptr
        lda #>$d800
        sta intro_dst_ptr+1
        jsr copy_1000_bytes

        jsr init_shooting_state
        lda #0
        sta battle_return_to_main_choice

        lda #0
        sta shoot_elev
        sta shoot_side

        lda #<crosshair_sprite
        sta intro_src_ptr
        lda #>crosshair_sprite
        sta intro_src_ptr+1
        lda #<$50c0
        sta intro_dst_ptr
        lda #>$50c0
        sta intro_dst_ptr+1
        ldy #63
xhair_copy:
        lda (intro_src_ptr),y
        sta (intro_dst_ptr),y
        dey
        bpl xhair_copy

        lda #SPRITE_PTR_BANK1_BASE+3
        sta SPRITE_PTR_TAB_BANK1_INTRO
        lda #10
        sta $d027

        jsr redraw_shooting_view
        lda #$01
        sta $d015

        lda #0
        sta $c6
xhair_loop:
        jsr $ffe4
        beq xhair_loop

        cmp #$30
        beq xhair_abort
        cmp #$46
        beq xhair_done
        cmp #$66
        beq xhair_done

        cmp #$11            // cursor down
        bne xhair_chk_cursor_up
        jmp xhair_down
xhair_chk_cursor_up:
        cmp #$91            // cursor up
        bne xhair_chk_cursor_left
        jmp xhair_up
xhair_chk_cursor_left:
        cmp #$9d            // cursor left
        bne xhair_chk_cursor_right
        jmp xhair_left
xhair_chk_cursor_right:
        cmp #$1d            // cursor right
        bne xhair_chk_keypad
        jmp xhair_right
xhair_chk_keypad:

        cmp #$32
        bne xhair_chk_up
        jmp xhair_down
xhair_chk_up:
        cmp #$38
        bne xhair_chk_left
        jmp xhair_up
xhair_chk_left:
        cmp #$34
        bne xhair_chk_right
        jmp xhair_left
xhair_chk_right:
        cmp #$36
        bne xhair_chk_down_left
        jmp xhair_right
xhair_chk_down_left:
        cmp #$31
        bne xhair_chk_down_right
        jmp xhair_down_left
xhair_chk_down_right:
        cmp #$33
        bne xhair_chk_up_left
        jmp xhair_down_right
xhair_chk_up_left:
        cmp #$37
        bne xhair_chk_up_right
        jmp xhair_up_left
xhair_chk_up_right:
        cmp #$39
        bne xhair_loop
        jmp xhair_up_right

xhair_abort:
        lda #$00
        sta $d015
        jsr clear_text_notice_screen
        lda #0
        sta battle_return_to_main_choice
        rts

xhair_done:
        lda #$00
        sta $d015
        jsr resolve_shot_round
        lda battle_return_to_main_choice
        cmp #2
        beq xhair_exit_battle
        jsr update_shooting_screen
        jsr draw_shot_markers
        jsr show_shot_prompt
fire_wait_key:
        jsr $ffe4
        beq fire_wait_key
        jsr redraw_shooting_view
        jmp xhair_loop

xhair_exit_battle:
        rts

xhair_up:
        dec shoot_elev
        jsr clamp_elev
        jsr update_crosshair_pos
        jmp xhair_loop

xhair_down:
        inc shoot_elev
        jsr clamp_elev
        jsr update_crosshair_pos
        jmp xhair_loop

xhair_left:
        dec shoot_side
        jsr clamp_side
        jsr update_crosshair_pos
        jmp xhair_loop

xhair_right:
        inc shoot_side
        jsr clamp_side
        jsr update_crosshair_pos
        jmp xhair_loop

xhair_up_left:
        dec shoot_elev
        dec shoot_side
        jsr clamp_elev
        jsr clamp_side
        jsr update_crosshair_pos
        jmp xhair_loop

xhair_up_right:
        dec shoot_elev
        inc shoot_side
        jsr clamp_elev
        jsr clamp_side
        jsr update_crosshair_pos
        jmp xhair_loop

xhair_down_left:
        inc shoot_elev
        dec shoot_side
        jsr clamp_elev
        jsr clamp_side
        jsr update_crosshair_pos
        jmp xhair_loop

xhair_down_right:
        inc shoot_elev
        inc shoot_side
        jsr clamp_elev
        jsr clamp_side
        jsr update_crosshair_pos
        jmp xhair_loop

update_shooting_screen:
        ldx #0
upd_fld_clr:
        lda shoot_fld_clr_lo,x
        sta intro_dst_ptr
        lda shoot_fld_clr_hi,x
        sta intro_dst_ptr+1
        ldy #5
        lda #15
upd_fld_clr_loop:
        sta (intro_dst_ptr),y
        dey
        bpl upd_fld_clr_loop
        inx
        cpx #7
        bne upd_fld_clr

        ldx #6
        ldy #17
        fnum(shoot_dist_lo)

        lda ship_cannons
        ldx #14
        ldy #19
        jsr print_fight_byte_stat

        ldx #14
        ldy #21
        fnum(crew_men)

        ldx #14
        ldy #23
        fnum(ship_repair_points)

        lda enemy_cannons_lo
        ldx #27
        ldy #19
        jsr print_fight_byte_stat

        ldx #27
        ldy #21
        fnum(enemy_crew_lo)

        ldx #27
        ldy #23
        fnum(enemy_ship_hp_lo)

        ldx #3
        ldy #20
        jsr set_screen_ptr_xy
        ldy #0
        lda #$20
        sta (dst_ptr),y
        iny
        sta (dst_ptr),y
        lda shoot_wind_str
        sta num_print_lo
        lda #0
        sta num_print_hi
        jsr print_word_decimal

        ldx #0
upd_wind_reset:
        lda shoot_wind_clr_lo,x
        sta intro_dst_ptr
        lda #>$db4a
        sta intro_dst_ptr+1
        ldy #0
        lda #15
        sta (intro_dst_ptr),y
        iny
        sta (intro_dst_ptr),y
        iny
        sta (intro_dst_ptr),y
        inx
        cpx #3
        bne upd_wind_reset

        ldx shoot_wind_ver
        lda shoot_wind_clr_lo,x
        sta intro_dst_ptr
        lda #>$db4a
        sta intro_dst_ptr+1
        ldy shoot_wind_hor
        lda #4
        sta (intro_dst_ptr),y
        rts

init_shooting_state:
        lda #0
        sta shot_roll
        sta shot_power
        sta shot_penalty

        // BASIC line 1850: IAFST = 500 + INT(RND * 500).
        lda $dc04
        asl
        clc
        adc #<500
        sta shoot_dist_lo
        lda #>500
        adc #0
        sta shoot_dist_hi
        cmp #>1000
        bcc init_shoot_dist_done
        bne init_shoot_dist_wrap
        lda shoot_dist_lo
        cmp #<1000
        bcc init_shoot_dist_done
init_shoot_dist_wrap:
        sec
        lda shoot_dist_lo
        sbc #11
        sta shoot_dist_lo
        lda shoot_dist_hi
        sbc #0
        sta shoot_dist_hi
init_shoot_dist_done:

        // BASIC line 1950: each wind axis is -10, 0, or +10.
        jsr random_mod_8
        and #$03
        cmp #3
        bne init_shoot_wind_hor_ok
        lda #1
init_shoot_wind_hor_ok:
        sta shoot_wind_hor

        jsr random_mod_8
        and #$03
        cmp #3
        bne init_shoot_wind_ver_ok
        lda #1
init_shoot_wind_ver_ok:
        sta shoot_wind_ver

        // BASIC line 1960: strength is 1..10, unless both axes are calm.
        jsr random_mod_16
        cmp #10
        bcc init_shoot_wind_strength_ok
        sec
        sbc #10
init_shoot_wind_strength_ok:
        clc
        adc #1
        sta shoot_wind_str
        lda shoot_wind_hor
        cmp #1
        bne init_shoot_state_done
        lda shoot_wind_ver
        cmp #1
        bne init_shoot_state_done
        lda #0
        sta shoot_wind_str
init_shoot_state_done:
        rts

// color RAM low bytes for wind compass rows 21/22/23 at col 2
// high byte is $DB for all three ($DB4A, $DB72, $DB9A)
shoot_wind_clr_lo:
        .byte <($db4a + 0*40)
        .byte <($db4a + 1*40)
        .byte <($db4a + 2*40)

shoot_dist_lo:  .byte 0
shoot_dist_hi:  .byte 0
shoot_wind_str: .byte 0
shoot_wind_hor: .byte 0     // 0=left 1=center 2=right
shoot_wind_ver: .byte 0     // 0=up 1=center 2=down

// color RAM start addresses for the 7 stat value fields ($D800 + row*40 + col)
shoot_fld_clr_lo:
        .byte <$daae    // AFST     row 17 col 6
        .byte <$db06    // own SKYTS row 19 col 14
        .byte <$db56    // own MAND  row 21 col 14
        .byte <$dba6    // own REP   row 23 col 14
        .byte <$db13    // foe SKYTS row 19 col 27
        .byte <$db63    // foe MAND  row 21 col 27
        .byte <$dbb3    // foe REP   row 23 col 27
shoot_fld_clr_hi:
        .byte >$daae,>$db06,>$db56,>$dba6,>$db13,>$db63,>$dbb3

// Clamp shoot_elev to -30..+30 (IELEVA bounds, BASIC lines 2170-2180)
clamp_elev:
        lda shoot_elev
        bmi clamp_elev_neg
        cmp #31
        bcc clamp_elev_done
        lda #30
        sta shoot_elev
        rts
clamp_elev_neg:
        cmp #$e2            // $E2 = -30 in two's complement
        bcs clamp_elev_done
        lda #$e2
        sta shoot_elev
clamp_elev_done:
        rts

// Clamp shoot_side to -50..+50 (ISIDE bounds, BASIC lines 2190-2200)
clamp_side:
        lda shoot_side
        bmi clamp_side_neg
        cmp #51
        bcc clamp_side_done
        lda #50
        sta shoot_side
        rts
clamp_side_neg:
        cmp #$ce            // $CE = -50 in two's complement
        bcs clamp_side_done
        lda #$ce
        sta shoot_side
clamp_side_done:
        rts

// Update sprite 0 X/Y from shoot_side/elev.
// X = 244 + shoot_side (signed), Y = 90 + shoot_elev (signed).
update_crosshair_pos:
        lda shoot_side
        bmi ucp_neg_side
        clc
        adc #244            // positive side: carry set if result > 255
        sta $d000
        bcc ucp_clear_xhi
        lda $d010
        ora #$01            // set X_hi for sprite 0
        sta $d010
        jmp ucp_y
ucp_neg_side:
        clc
        adc #244            // negative side: result always < 256
        sta $d000
ucp_clear_xhi:
        lda $d010
        and #$fe
        sta $d010
ucp_y:
        lda #90
        clc
        adc shoot_elev      // range 60..120; ignore carry
        sta $d001
        rts

redraw_shooting_view:
        lda #<shooting_screen_data
        sta intro_src_ptr
        lda #>shooting_screen_data
        sta intro_src_ptr+1
        lda #<INTRO_SCREEN_RAM
        sta intro_dst_ptr
        lda #>INTRO_SCREEN_RAM
        sta intro_dst_ptr+1
        jsr copy_1000_bytes

        lda #<shooting_color_data
        sta intro_src_ptr
        lda #>shooting_color_data
        sta intro_src_ptr+1
        lda #<$d800
        sta intro_dst_ptr
        lda #>$d800
        sta intro_dst_ptr+1
        jsr copy_1000_bytes

        jsr update_shooting_screen
        jsr update_crosshair_pos
        lda #$01
        sta $d015
        rts

draw_shot_markers:
        // Original pixel effects: own (IXPL,IYPL), enemy (IAKTX,IAKTY).
        // These coordinates are their character-cell equivalents.
        lda #<shot_marker_msg
        sta src_ptr
        lda #>shot_marker_msg
        sta src_ptr+1
        ldx shot_own_marker_x
        ldy shot_own_marker_y
        jsr draw_shot_marker_at
        ldx shot_enemy_marker_x
        ldy shot_enemy_marker_y
draw_shot_marker_at:
        jsr write_zero_terminated_screen
        rts

resolve_shot_round:
        jsr random_mod_16
        sta shot_roll

        lda ship_cannons
        lsr
        clc
        adc shot_roll
        sta shot_power

        lda shoot_side
        bpl rsr_side_abs_done
        eor #$ff
        clc
        adc #1
rsr_side_abs_done:
        lsr
        lsr
        lsr
        sta shot_penalty

        lda shoot_elev
        bpl rsr_elev_abs_done
        eor #$ff
        clc
        adc #1
rsr_elev_abs_done:
        lsr
        lsr
        lsr
        clc
        adc shot_penalty
        adc shoot_wind_str
        sta shot_penalty

        lda shot_power
        sec
        sbc shot_penalty
        sta shot_power

        cmp #8
        bcs rsr_hit

        // User-facing panel coordinates are one-based. Screen coordinates are zero-based:
        // own: columns 4..13, rows 2..5; enemy: columns 4..14, rows 8..11.
        lda shoot_side
        bmi rsr_miss_left
        lda #13
        sta shot_own_marker_x
        lda #14
        sta shot_enemy_marker_x
        jmp rsr_miss_x_done
rsr_miss_left:
        lda #4
        sta shot_own_marker_x
        lda #4
        sta shot_enemy_marker_x
rsr_miss_x_done:
        lda shoot_elev
        bmi rsr_miss_high
        lda #5
        sta shot_own_marker_y
        lda #11
        sta shot_enemy_marker_y
        jmp rsr_damage_player
rsr_miss_high:
        lda #2
        sta shot_own_marker_y
        lda #8
        sta shot_enemy_marker_y
        jmp rsr_damage_player

rsr_hit:
        // Centered impacts for the character-cell vessel panels.
        lda #9
        sta shot_own_marker_x
        lda #3
        sta shot_own_marker_y
        lda #9
        sta shot_enemy_marker_x
        lda #9
        sta shot_enemy_marker_y

        lda shot_power
        clc
        adc #4
        sta damage_lo
        lda #0
        sta damage_hi

        sec
        lda enemy_ship_hp_lo
        sbc damage_lo
        sta enemy_ship_hp_lo
        lda enemy_ship_hp_hi
        sbc damage_hi
        bcs rsr_enemy_hp_ok
        lda #0
        sta enemy_ship_hp_lo
        sta enemy_ship_hp_hi
rsr_enemy_hp_ok:

        lda shot_power
        lsr
        clc
        adc #1
        sta damage_lo
        lda #0
        sta damage_hi

        sec
        lda enemy_crew_lo
        sbc damage_lo
        sta enemy_crew_lo
        lda enemy_crew_hi
        sbc damage_hi
        bcs rsr_enemy_crew_ok
        lda #0
        sta enemy_crew_lo
        sta enemy_crew_hi
rsr_enemy_crew_ok:

        lda shot_power
        lsr
        lsr
        cmp #3
        bcc rsr_damage_player
        lda enemy_cannons_lo
        beq rsr_damage_player
        dec enemy_cannons_lo
        jmp rsr_check_enemy_crew

rsr_check_enemy_crew:
        lda enemy_crew_hi
        ora enemy_crew_lo
        bne rsr_check_enemy_hp
        jsr handle_enemy_surrender
        lda #2
        sta battle_return_to_main_choice
        rts

rsr_check_enemy_hp:
        lda enemy_ship_hp_hi
        ora enemy_ship_hp_lo
        bne rsr_damage_player
        jsr handle_enemy_surrender
        lda #2
        sta battle_return_to_main_choice
        rts

rsr_damage_player:
        jsr random_mod_8
        clc
        adc #1
        adc shoot_wind_str
        sta damage_lo
        lda #0
        sta damage_hi

        sec
        lda ship_repair_points
        sbc damage_lo
        sta ship_repair_points
        lda ship_repair_points+1
        sbc damage_hi
        sta ship_repair_points+1
        bcs rsr_repair_ok
        lda #0
        sta ship_repair_points
        sta ship_repair_points+1
rsr_repair_ok:

        lda damage_lo
        lsr
        clc
        adc #1
        sta damage_lo
        lda #0
        sta damage_hi

        sec
        lda crew_men
        sbc damage_lo
        sta crew_men
        lda crew_men+1
        sbc damage_hi
        bcs rsr_player_crew_ok
        lda #0
        sta crew_men
        sta crew_men+1
rsr_player_crew_ok:
        jsr check_ship_survival
        rts

show_shot_prompt:
        ldx #17
        ldy #15
        wstr(shooting_press_key_msg)
        rts

// Crosshair sprite: vertical line at pixel 11, horizontal at row 10 (24x21 singlecolor).
crosshair_sprite:
        .byte $00,$10,$00, $00,$10,$00, $00,$10,$00, $00,$10,$00, $00,$10,$00
        .byte $00,$10,$00, $00,$10,$00, $00,$10,$00, $00,$10,$00, $00,$10,$00
        .byte $FF,$FF,$FE
        .byte $00,$10,$00, $00,$10,$00, $00,$10,$00, $00,$10,$00, $00,$10,$00
        .byte $00,$10,$00, $00,$10,$00, $00,$10,$00, $00,$10,$00, $00,$10,$00
        .byte $0A           // unused padding (VIC reads color from $D027)

shoot_elev: .byte 0         // -30..+30  (IELEVA equivalent)
shoot_side: .byte 0         // -50..+50  (ISIDE equivalent)
shot_own_marker_x: .byte 0
shot_own_marker_y: .byte 0
shot_enemy_marker_x: .byte 0
shot_enemy_marker_y: .byte 0
shot_roll: .byte 0
shot_power: .byte 0
shot_penalty: .byte 0

shot_marker_msg:
        .text "X"
        .byte 0

shooting_press_key_msg:
        .text "TRYK EN TAST          "
        .byte 0

// 2000 bytes of pure screen/colour data, relocated to the free $c000 block so it
// does not eat into the code segment (which must end before $4400).
// CPU-copied at runtime, so it needs no VIC visibility.
shooting_code_end:
* = $c000 "ShootingScreenData"

// 25 rows x 40 bytes of screen RAM codes (direct copy to $4400).
shooting_screen_data:
        .byte 160,160,160,160,160,160,160,160,160,160,160,160,160,160,160,160,160,160,160,160,160,160,160,160,160,160,160,160,160,160,160,160,160,160,160,160,160,160,160,160
        .byte 160,6,10,5,14,4,5,32,32,32,32,32,32,32,32,32,32,32,32,160,32,32,32,32,32,32,32,32,32,32,32,32,32,32,32,32,32,32,32,160
        .byte 160,32,32,32,32,32,32,32,32,32,32,32,32,32,32,32,32,32,32,160,32,32,32,32,32,32,32,32,32,32,32,32,32,32,32,32,32,32,32,160
        .byte 160,32,32,32,32,100,121,248,247,160,160,247,98,32,32,32,32,32,32,160,32,32,32,32,32,32,32,32,32,118,219,32,32,32,32,32,32,32,32,160
        .byte 160,32,32,32,32,99,120,226,239,160,160,239,226,32,32,32,32,32,32,160,32,32,32,32,32,32,118,160,223,118,160,223,32,32,32,32,32,32,32,160
        .byte 160,32,32,32,32,32,32,32,32,32,32,32,32,32,32,32,32,32,32,160,32,32,32,32,32,32,118,64,67,118,160,160,223,32,32,32,32,32,32,160
        .byte 160,160,160,160,160,160,160,160,160,160,160,160,160,160,160,160,160,160,160,160,32,32,32,32,32,32,118,160,160,118,160,160,160,223,32,32,32,32,32,160
        .byte 160,5,7,5,14,32,32,32,32,32,32,32,32,32,32,32,32,32,32,160,32,32,32,32,77,32,118,32,32,118,32,225,251,251,32,32,32,32,32,160
        .byte 160,32,32,32,32,32,32,32,32,32,32,32,32,32,32,32,32,32,32,160,32,32,32,32,32,251,160,239,239,239,239,239,239,236,32,32,32,32,32,160
        .byte 160,32,32,32,32,100,121,248,247,160,160,247,98,32,32,32,32,32,32,160,32,32,32,32,32,32,251,160,160,160,160,160,160,32,32,32,32,32,32,160
        .byte 160,32,32,32,32,99,120,226,239,160,160,239,226,32,32,32,32,32,32,160,32,32,32,32,32,32,32,32,32,32,32,32,32,32,32,32,32,32,32,160
        .byte 160,32,32,32,32,32,32,32,32,32,32,32,32,32,32,32,32,32,32,160,32,32,32,32,32,32,32,32,32,32,32,32,32,32,32,32,32,32,32,160
        .byte 160,32,32,32,32,32,32,32,32,32,32,32,32,32,32,32,32,32,32,160,32,32,32,32,32,32,32,19,9,7,20,5,32,32,32,32,32,32,32,160
        .byte 160,160,160,160,160,160,160,160,160,160,160,160,160,160,160,160,160,160,160,160,160,160,160,160,160,160,160,160,160,160,160,160,160,160,160,160,160,160,160,160
        .byte 32,14,5,4,19,12,1,7,32,32,32,32,32,32,32,32,32,32,32,32,32,32,32,32,32,32,32,32,32,32,32,32,32,32,32,32,32,32,32,32
        .byte 32,32,32,32,32,32,32,32,32,32,32,32,32,32,32,32,32,6,61,6,25,18,44,32,48,61,1,6,2,18,25,4,32,11,1,13,16,5,14,32
        .byte 112,67,67,67,67,67,67,67,67,67,67,67,114,67,67,67,67,67,67,67,67,67,67,67,67,67,114,64,67,67,67,67,67,67,67,67,67,67,67,110
        .byte 93,1,6,19,20,58,32,32,32,32,32,32,66,32,32,32,5,7,5,14,32,32,32,32,32,32,66,32,32,32,32,6,10,5,14,4,5,32,32,93
        .byte 107,67,67,67,67,67,114,67,67,67,67,67,91,67,67,67,67,67,67,67,67,67,67,67,67,67,91,67,67,67,67,67,67,67,67,67,67,67,67,115
        .byte 66,22,9,14,4,32,66,19,11,25,20,19,66,32,32,32,32,32,32,32,32,32,32,32,32,32,66,32,32,32,32,32,32,32,32,32,32,32,32,93
        .byte 66,32,32,32,32,32,107,67,67,67,67,67,91,67,67,67,67,67,67,67,67,67,67,67,67,67,91,64,67,67,67,67,67,67,67,67,67,67,67,115
        .byte 93,32,77,66,78,32,66,13,1,14,4,32,66,32,32,32,32,32,32,32,32,32,32,32,32,32,66,32,32,32,32,32,32,32,32,32,32,32,32,93
        .byte 66,32,64,81,64,32,107,67,67,67,67,67,91,67,67,67,67,67,67,67,67,67,67,67,67,64,91,64,67,67,67,67,67,67,67,67,67,67,67,115
        .byte 93,32,78,66,77,32,66,18,5,16,32,32,66,32,32,32,32,32,32,32,32,32,32,32,32,32,66,32,32,32,32,32,32,32,32,32,32,32,32,93
        .byte 109,67,67,67,67,67,113,67,67,67,67,67,113,67,67,67,67,67,67,67,67,67,67,67,67,67,113,64,67,67,67,67,67,67,67,67,67,67,67,125

// 25 rows x 40 bytes of color RAM values (direct copy to $D800).
shooting_color_data:
        .byte 15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15
        .byte 15,15,15,15,15,15,15,14,14,14,14,14,14,14,14,14,14,14,14,15,14,14,14,14,14,14,14,14,14,14,14,14,14,14,14,14,14,14,14,15
        .byte 15,14,14,14,14,14,14,14,14,14,14,14,14,14,14,14,14,14,14,15,14,14,14,14,14,14,14,14,14,14,14,14,14,14,14,14,14,14,14,15
        .byte 15,14,14,14,14,4,4,4,4,4,4,4,4,14,14,14,14,14,14,15,14,14,14,14,14,14,14,14,10,4,3,14,14,14,14,14,14,14,14,15
        .byte 15,14,14,14,14,4,4,4,4,4,4,4,4,14,14,14,14,14,14,15,14,14,14,14,14,14,4,15,15,4,15,15,15,14,14,14,14,14,14,15
        .byte 15,14,14,14,14,14,14,14,15,15,15,15,14,14,14,14,14,14,14,15,14,14,14,14,14,14,4,4,4,4,15,15,15,15,14,14,14,14,14,15
        .byte 15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,14,14,14,14,14,14,4,15,15,4,15,15,15,15,14,14,14,14,14,15
        .byte 15,15,15,15,15,14,14,14,14,14,14,14,14,14,14,14,14,14,14,15,14,14,14,14,4,10,4,10,10,4,14,4,4,4,14,14,14,14,14,15
        .byte 15,14,14,14,15,15,14,14,14,14,14,14,14,14,14,14,14,14,14,15,14,14,14,14,14,4,4,4,4,4,4,4,4,4,14,14,14,14,14,15
        .byte 15,14,14,14,15,3,3,3,3,3,3,3,3,14,14,14,14,14,14,15,14,14,14,14,14,14,4,4,4,4,4,4,4,14,14,14,14,14,14,15
        .byte 15,14,14,14,14,3,3,3,3,3,3,3,3,14,14,14,14,14,14,15,14,14,14,14,14,14,14,14,14,14,14,14,14,14,14,14,14,14,14,15
        .byte 15,14,14,14,14,14,14,14,14,14,14,14,14,14,14,14,14,14,14,15,14,14,14,14,14,14,14,14,14,14,14,14,14,14,14,14,14,14,14,15
        .byte 15,14,14,14,14,14,14,14,14,14,14,15,15,14,14,14,14,14,14,15,14,14,14,14,14,14,15,15,15,15,15,15,14,14,14,14,14,14,14,15
        .byte 15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15
        .byte 14,15,15,15,15,15,15,15,14,14,14,14,14,14,14,14,14,14,14,14,14,14,14,14,14,14,14,14,14,14,14,14,14,14,14,14,14,14,14,14
        .byte 14,14,14,14,14,14,14,14,14,14,14,14,14,15,14,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,14
        .byte 15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15
        .byte 15,15,15,15,15,15,15,15,15,15,15,15,15,14,14,14,15,15,15,15,14,14,14,14,15,15,15,15,15,14,15,15,15,15,15,15,15,14,14,15
        .byte 15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15
        .byte 15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,14,14,14,14,14,14,14,14,14,15,15,15,14,14,14,14,14,15,15,15,15,15,15
        .byte 15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15
        .byte 15,15,15,15,15,15,15,15,15,15,15,14,15,14,14,14,14,14,14,14,14,14,14,14,14,14,15,15,15,14,14,14,14,14,14,15,15,14,14,15
        .byte 15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15
        .byte 15,15,15,15,15,14,15,15,15,15,14,14,15,14,14,14,14,14,14,14,14,14,14,14,14,15,15,15,15,14,14,14,14,14,14,14,14,14,14,15
        .byte 15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15

* = shooting_code_end "ShootingCodeResume"
