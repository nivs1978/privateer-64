.label src_ptr = $fb
.label dst_ptr = $fd
.label len_lo = $f9
.label len_hi = $fa
.label bmp_char_ptr = $f7   // charset read pointer for draw_bmp_char (2 bytes: $f7=lo, $f8=hi)

// Normalize one-color cells by duplicating the nonzero nibble into the zero nibble.

draw_map:
        sei
        jsr hide_ship

        // MapArt spans into the BASIC ROM window; expose RAM there while copying it.
        lda $01
        pha
        and #$fe
        sta $01

        lda map_border_color
        sta $d020

        // Copy 8000 bitmap bytes to VIC bitmap RAM while still in text mode.
        lda #<map_bitmap_data
        sta src_ptr
        lda #>map_bitmap_data
        sta src_ptr+1
        lda #<$6000
        sta dst_ptr
        lda #>$6000
        sta dst_ptr+1
        lda #<$1f40
        sta len_lo
        lda #>$1f40
        sta len_hi
        jsr copy_len_bytes

        // Verify the copied bitmap matches the source before enabling bitmap mode.
        lda #<map_bitmap_data
        sta src_ptr
        lda #>map_bitmap_data
        sta src_ptr+1
        lda #<$6000
        sta dst_ptr
        lda #>$6000
        sta dst_ptr+1
        lda #<$1f40
        sta len_lo
        lda #>$1f40
        sta len_hi
        jsr verify_len_bytes

        // Copy 1000 screen color bytes with one-color cell normalization.
        lda #<map_screen_data
        sta map_screen_src
        lda #>map_screen_data
        sta map_screen_src+1
        lda #<$4400
        sta map_screen_dst
        lda #>$4400
        sta map_screen_dst+1

        ldx #25
draw_map_row_loop:
        lda map_screen_src
        sta src_ptr
        lda map_screen_src+1
        sta src_ptr+1
        lda map_screen_dst
        sta dst_ptr
        lda map_screen_dst+1
        sta dst_ptr+1
        jsr copy_screen_row_40
        lda src_ptr
        sta map_screen_src
        lda src_ptr+1
        sta map_screen_src+1
        lda dst_ptr
        sta map_screen_dst
        lda dst_ptr+1
        sta map_screen_dst+1

        dex
        bne draw_map_row_loop

        pla
        sta $01

        // Switch to hires bitmap mode only after the map is fully loaded.
        lda #$3b
        sta $d011

        lda #$08
        sta $d016

        lda $dd00
        and #$fc
        ora #VIC_BANK1_SELECT
        sta $dd00

        // Bitmap at $6000 and screen RAM at $4400 within VIC bank 1.
        lda #$18
        sta $d018

        jsr hide_ship
        jsr draw_map_status_table
        cli
        rts

game_loop:
game_loop_wait:
        jsr $ffe4
        beq game_loop_wait
        cmp #17
        beq game_try_move_down
        cmp #145
        beq game_try_move_up
        cmp #29
        beq game_try_move_right
        cmp #157
        beq game_try_move_left
        jmp game_loop_wait

game_try_move_up:
        jsr begin_pixel_move
        lda ship_y
        cmp #MAP_PIXEL_MIN_Y
        beq game_try_move_up_blocked
        bcc game_try_move_up_blocked
        sec
        sbc #MAP_GRID_STEP
        sta ship_y
        jmp resolve_pixel_move
game_try_move_up_blocked:
        jmp game_loop_wait

game_try_move_down:
        jsr begin_pixel_move
        lda ship_y
        cmp #MAP_PIXEL_MAX_Y
        bcs game_try_move_down_blocked
        clc
        adc #MAP_GRID_STEP
        sta ship_y
        jmp resolve_pixel_move
game_try_move_down_blocked:
        jmp game_loop_wait

game_try_move_left:
        jsr begin_pixel_move
        lda ship_x_hi
        cmp #>MAP_PIXEL_MIN_X
        bcc game_try_move_left_blocked
        bne game_try_move_left_apply
        lda ship_x
        cmp #<MAP_PIXEL_MIN_X
        beq game_try_move_left_blocked
        bcc game_try_move_left_blocked
game_try_move_left_apply:
        sec
        lda ship_x
        sbc #MAP_GRID_STEP
        sta ship_x
        lda ship_x_hi
        sbc #0
        sta ship_x_hi
        jmp resolve_pixel_move
game_try_move_left_blocked:
        jmp game_loop_wait

game_try_move_right:
        jsr begin_pixel_move
        lda ship_x_hi
        cmp #>MAP_PIXEL_MAX_X
        bcc game_try_move_right_apply
        bne game_try_move_right_blocked
        lda ship_x
        cmp #<MAP_PIXEL_MAX_X
        bcs game_try_move_right_blocked
game_try_move_right_apply:
        clc
        lda ship_x
        adc #MAP_GRID_STEP
        sta ship_x
        lda ship_x_hi
        adc #0
        sta ship_x_hi
        jmp resolve_pixel_move
game_try_move_right_blocked:
        jmp game_loop_wait

begin_pixel_move:
        lda ship_x
        sta ship_prev_x
        lda ship_x_hi
        sta ship_prev_x_hi
        lda ship_y
        sta ship_prev_y
        rts

resolve_pixel_move:
        inc voyage_turn_counter
        bne resolve_pixel_move_turn_incremented
        inc voyage_turn_counter+1
resolve_pixel_move_turn_incremented:
        jsr check_victory_conditions
        jsr derive_map_cell_from_ship_position

        ldx map_ship_col
        ldy map_ship_row
        jsr get_map_cell_type
        sta current_map_cell_type

        cmp #MAP_CELL_LAND
        beq resolve_pixel_move_land
        cmp #MAP_CELL_SEA
        beq resolve_pixel_move_sea

        jsr position_ship
        lda current_map_cell_type
        jsr on_harbour_entered
        jmp game_loop_wait

resolve_pixel_move_sea:
        jsr position_ship
        // 25% random sea event chance, matching the BASIC game flow.
        lda $dc04
        and #$03
        beq resolve_pixel_move_random_event
        jsr update_map_move_values
        jmp game_loop_wait
resolve_pixel_move_random_event:
        jsr on_random_sea_event
        jmp game_loop_wait

resolve_pixel_move_land:
        lda ship_prev_x
        sta ship_x
        lda ship_prev_x_hi
        sta ship_x_hi
        lda ship_prev_y
        sta ship_y
        jsr position_ship
        jsr on_land_collision
        jmp game_loop_wait

init_map_navigation:
        // Keep pixel coordinate as source of truth, then derive logical grid cell from it.
        lda #<MAP_START_PIXEL_X
        sta ship_x
        lda #>MAP_START_PIXEL_X
        sta ship_x_hi
        lda #MAP_START_PIXEL_Y
        sta ship_y
        lda #0
        sta voyage_turn_counter
        sta voyage_turn_counter+1
        sta ship_collision_counter
        lda #200
        sta ship_repair_points
        lda #0
        sta last_harbour_id
        lda #1
        sta player_naval_title_index
        sta player_noble_title_index
        jsr derive_map_cell_from_ship_position
        jsr position_ship
        rts

sync_ship_to_grid_position:
        jsr derive_map_cell_from_ship_position
        jsr position_ship
        rts

derive_map_cell_from_ship_position:
        // map_ship_col = ((ship_x - MAP_GRID_ORIGIN_X) / MAP_GRID_STEP) + 1
        lda ship_x
        sec
        sbc #<MAP_GRID_ORIGIN_X
        sta map_rel_lo
        lda ship_x_hi
        sbc #>MAP_GRID_ORIGIN_X
        sta map_rel_hi
        lda #1
        sta map_ship_col
derive_col_loop:
        lda map_rel_hi
        bne derive_col_subtract
        lda map_rel_lo
        cmp #MAP_GRID_STEP
        bcc derive_col_done
derive_col_subtract:
        sec
        lda map_rel_lo
        sbc #MAP_GRID_STEP
        sta map_rel_lo
        lda map_rel_hi
        sbc #0
        sta map_rel_hi
        inc map_ship_col
        jmp derive_col_loop
derive_col_done:

        // map_ship_row = ((ship_y - MAP_GRID_ORIGIN_Y) / MAP_GRID_STEP) + 1
        lda ship_y
        sec
        sbc #MAP_GRID_ORIGIN_Y
        sta map_rel_lo
        lda #1
        sta map_ship_row
derive_row_loop:
        lda map_rel_lo
        cmp #MAP_GRID_STEP
        bcc derive_row_done
        sec
        sbc #MAP_GRID_STEP
        sta map_rel_lo
        inc map_ship_row
        jmp derive_row_loop
derive_row_done:
        rts

get_map_cell_type:
        // Input: X=column (1..29), Y=row (1..14)
        // Output: A=0 sea, 1 land, 2..8 harbour id.
        stx map_query_col
        sty map_query_row

        ldx #0
map_harbour_scan:
        cpx #MAP_HARBOUR_COUNT
        beq map_land_check
        lda map_harbour_col,x
        cmp map_query_col
        bne map_harbour_next
        lda map_harbour_row,x
        cmp map_query_row
        bne map_harbour_next
        lda map_harbour_id,x
        rts
map_harbour_next:
        inx
        bne map_harbour_scan

map_land_check:
        ldx map_query_col
        dex
        lda map_col_bit_mask,x
        sta map_query_bit_mask
        lda map_col_byte_index,x
        sta map_query_byte_index

        lda #<map_land_row_masks
        sta src_ptr
        lda #>map_land_row_masks
        sta src_ptr+1

        ldy map_query_row
        dey
        tya
        asl
        asl
        clc
        adc src_ptr
        sta src_ptr
        bcc map_land_row_ptr_ok
        inc src_ptr+1
map_land_row_ptr_ok:

        ldy map_query_byte_index
        lda (src_ptr),y
        and map_query_bit_mask
        beq map_cell_is_sea
        lda #MAP_CELL_LAND
        rts

map_cell_is_sea:
        lda #MAP_CELL_SEA
        rts

on_land_collision:
        inc ship_collision_counter
        lda ship_repair_points
        beq on_land_collision_done
        cmp #3
        bcc on_land_collision_zero
        sec
        sbc #3
        sta ship_repair_points
        jmp on_land_collision_done
on_land_collision_zero:
        lda #0
        sta ship_repair_points
on_land_collision_done:
        jsr check_ship_survival
        rts

on_random_sea_event:
        jsr hide_ship
        jsr configure_intro_vic_bank
        jsr prepare_text_notice_screen

        // BASIC 1810 branch: I=1..10, where 1..8 are ships and 9..10 are fog events.
        jsr random_mod_10
        clc
        adc #1
        cmp #9
        bcs random_event_fog_branch
        sta enemy_index
        jsr run_enemy_encounter_text
        jmp random_event_done

random_event_fog_branch:
        jsr run_fog_event_text

random_event_done:
        jsr draw_map
        jsr load_ship_sprites
        jsr sync_ship_to_grid_position
        jsr show_ship
        rts

run_enemy_encounter_text:
        jsr init_enemy_from_index

        lda #<encounter_lookout_msg
        sta src_ptr
        lda #>encounter_lookout_msg
        sta src_ptr+1
        jsr print_zero_terminated
        jsr print_newline

        lda #<encounter_ship_ahoy_msg
        sta src_ptr
        lda #>encounter_ship_ahoy_msg
        sta src_ptr+1
        jsr print_zero_terminated
        jsr print_newline

        lda enemy_index
        cmp #8
        beq encounter_print_enemy_name
        lda #<encounter_english_msg
        sta src_ptr
        lda #>encounter_english_msg
        sta src_ptr+1
        jsr print_zero_terminated

encounter_print_enemy_name:
        lda enemy_name_ptr_lo
        sta src_ptr
        lda enemy_name_ptr_hi
        sta src_ptr+1
        jsr print_zero_terminated

        lda #<encounter_in_sight_msg
        sta src_ptr
        lda #>encounter_in_sight_msg
        sta src_ptr+1
        jsr print_zero_terminated
        jsr print_double_newline

        lda #<encounter_title_prefix_msg
        sta src_ptr
        lda #>encounter_title_prefix_msg
        sta src_ptr+1
        jsr print_zero_terminated
        jsr print_player_naval_title
        lda #' '
        jsr $ffd2
        jsr print_namebuf_zero_terminated
        lda #','
        jsr $ffd2
        jsr print_newline

        lda #<encounter_choose_msg
        sta src_ptr
        lda #>encounter_choose_msg
        sta src_ptr+1
        jsr print_zero_terminated
        jsr print_newline

encounter_choice_1:
        jsr print_newline
        lda #<encounter_prompt_fight_flee
        sta src_ptr
        lda #>encounter_prompt_fight_flee
        sta src_ptr+1
        jsr print_zero_terminated

encounter_choice_1_key:
        jsr read_key_blocking
        cmp #'A'
        beq encounter_attack_selected
        cmp #'a'
        beq encounter_attack_selected
        cmp #'F'
        beq encounter_flee_selected
        cmp #'f'
        beq encounter_flee_selected
        jmp encounter_choice_1_key

encounter_flee_selected:
        rts

encounter_attack_selected:
        jsr clear_text_notice_screen
        jsr print_newline
        lda #<encounter_prompt_fire_board_1
        sta src_ptr
        lda #>encounter_prompt_fire_board_1
        sta src_ptr+1
        jsr print_zero_terminated
        jsr print_newline
        jsr print_newline
        lda #<encounter_prompt_fire_board_2
        sta src_ptr
        lda #>encounter_prompt_fire_board_2
        sta src_ptr+1
        jsr print_zero_terminated
        jsr print_newline
        jsr print_newline
        jsr print_newline
        jsr print_newline
        jsr print_newline

        lda ship_cannons
        ora ship_cannons+1
        bne encounter_choice_2_key
        jmp encounter_board_selected

encounter_choice_2_key:
        jsr read_key_blocking
        cmp #'B'
        beq encounter_board_selected
        cmp #'b'
        beq encounter_board_selected
        cmp #'S'
        beq encounter_fire_selected
        cmp #'s'
        beq encounter_fire_selected
        jmp encounter_choice_2_key

encounter_board_selected:
        jsr run_boarding_fight_text
        lda battle_return_to_main_choice
        bne encounter_board_done
        jmp encounter_choice_1
encounter_board_done:
        rts

encounter_fire_selected:
        jsr run_shooting_minigame
        lda battle_return_to_main_choice
        beq encounter_fire_abort
        rts
encounter_fire_abort:
        jmp encounter_attack_selected
encounter_fire_done:
        rts

run_fog_event_text:
        lda #<fog_line_1
        sta src_ptr
        lda #>fog_line_1
        sta src_ptr+1
        jsr print_zero_terminated
        jsr print_newline
        lda #<fog_line_2
        sta src_ptr
        lda #>fog_line_2
        sta src_ptr+1
        jsr print_zero_terminated
        jsr print_newline
        lda #<fog_line_3
        sta src_ptr
        lda #>fog_line_3
        sta src_ptr+1
        jsr print_zero_terminated
        jsr print_newline
        lda #<fog_line_4
        sta src_ptr
        lda #>fog_line_4
        sta src_ptr+1
        jsr print_zero_terminated
        jsr print_newline
        lda #<fog_line_5
        sta src_ptr
        lda #>fog_line_5
        sta src_ptr+1
        jsr print_zero_terminated
        jsr print_double_newline

        lda #<encounter_title_prefix_msg
        sta src_ptr
        lda #>encounter_title_prefix_msg
        sta src_ptr+1
        jsr print_zero_terminated
        jsr print_player_naval_title
        lda #' '
        jsr $ffd2
        jsr print_namebuf_zero_terminated
        lda #','
        jsr $ffd2
        jsr print_newline

        lda #<encounter_choose_msg
        sta src_ptr
        lda #>encounter_choose_msg
        sta src_ptr+1
        jsr print_zero_terminated
        jsr print_newline
        jsr print_newline

        lda #<fog_prompt_msg
        sta src_ptr
        lda #>fog_prompt_msg
        sta src_ptr+1
        jsr print_zero_terminated

fog_prompt_key:
        jsr read_key_blocking
        cmp #'J'
        beq fog_investigate_yes
        cmp #'j'
        beq fog_investigate_yes
        cmp #'N'
        beq fog_investigate_no
        cmp #'n'
        beq fog_investigate_no
        jmp fog_prompt_key

fog_investigate_no:
        rts

fog_investigate_yes:
        jsr random_mod_10
        cmp #2
        bcc fog_outcome_enemy
        beq fog_outcome_treasure
fog_check_1:
        cmp #3
        bne fog_check_2
        jmp fog_outcome_jewel
fog_check_2:
        cmp #4
        bne fog_check_3
        jmp fog_outcome_cannon
fog_check_3:
        cmp #5
        bne fog_check_4
        jmp fog_outcome_crew
fog_check_4:
        cmp #8
        bne fog_check_5
        jmp fog_outcome_grain
fog_check_5:
        cmp #9
        bne fog_outcome_default
        jmp fog_outcome_plague
fog_outcome_default:

fog_outcome_nothing:
        jsr clear_text_notice_screen
        lda #<fog_nothing_line_1
        sta src_ptr
        lda #>fog_nothing_line_1
        sta src_ptr+1
        jsr print_zero_terminated
        jsr print_newline
        lda #<fog_nothing_line_2
        sta src_ptr
        lda #>fog_nothing_line_2
        sta src_ptr+1
        jsr print_zero_terminated
        jsr print_newline
        jsr show_press_any_key
        rts

fog_outcome_enemy:
        jsr clear_text_notice_screen
        lda #<fog_enemy_line_1
        sta src_ptr
        lda #>fog_enemy_line_1
        sta src_ptr+1
        jsr print_zero_terminated
        jsr print_newline
        lda #<fog_enemy_line_2
        sta src_ptr
        lda #>fog_enemy_line_2
        sta src_ptr+1
        jsr print_zero_terminated
        jsr print_newline
        jsr show_press_any_key
        jsr print_newline
        jsr random_enemy_roll
        sta enemy_index
        jsr run_enemy_encounter_text
        rts

fog_outcome_treasure:
        jsr clear_text_notice_screen
        lda #<fog_found_island_msg
        sta src_ptr
        lda #>fog_found_island_msg
        sta src_ptr+1
        jsr print_zero_terminated
        jsr print_newline
        lda #<fog_treasure_line_2
        sta src_ptr
        lda #>fog_treasure_line_2
        sta src_ptr+1
        jsr print_zero_terminated
        jsr print_newline
        lda #<600
        sta damage_lo
        lda #>600
        sta damage_hi
        lda #<fog_treasure_line_3
        sta src_ptr
        lda #>fog_treasure_line_3
        sta src_ptr+1
        jsr print_zero_terminated
        lda damage_lo
        sta num_print_lo
        lda damage_hi
        sta num_print_hi
        jsr print_inline_word_decimal
        lda #<fog_treasure_line_4
        sta src_ptr
        lda #>fog_treasure_line_4
        sta src_ptr+1
        jsr print_zero_terminated
        jsr print_newline
        lda damage_lo
        clc
        adc treasury_rigsdaler
        sta treasury_rigsdaler
        lda damage_hi
        adc treasury_rigsdaler+1
        sta treasury_rigsdaler+1
        jsr show_press_any_key
        rts

fog_outcome_jewel:
        jsr clear_text_notice_screen
        jsr random_mod_6
        clc
        adc #2
        sta damage_lo
        lda #0
        sta damage_hi
        lda damage_lo
        clc
        adc jewel_count
        sta jewel_count
        lda damage_hi
        adc jewel_count+1
        sta jewel_count+1
fog_outcome_jewel_msg:
        lda #<fog_found_island_msg
        sta src_ptr
        lda #>fog_found_island_msg
        sta src_ptr+1
        jsr print_zero_terminated
        jsr print_newline
        lda #<fog_jewel_line_2
        sta src_ptr
        lda #>fog_jewel_line_2
        sta src_ptr+1
        jsr print_zero_terminated
        lda damage_lo
        sta num_print_lo
        lda damage_hi
        sta num_print_hi
        jsr print_inline_word_decimal
        lda #<fog_jewel_line_3
        sta src_ptr
        lda #>fog_jewel_line_3
        sta src_ptr+1
        jsr print_zero_terminated
        jsr print_newline
        jsr show_press_any_key
        rts

fog_outcome_cannon:
        jsr clear_text_notice_screen
        inc ship_cannons
        bne fog_outcome_cannon_msg
        inc ship_cannons+1
fog_outcome_cannon_msg:
        lda #<fog_found_island_msg
        sta src_ptr
        lda #>fog_found_island_msg
        sta src_ptr+1
        jsr print_zero_terminated
        jsr print_newline
        lda #<fog_cannon_line_2
        sta src_ptr
        lda #>fog_cannon_line_2
        sta src_ptr+1
        jsr print_zero_terminated
        jsr print_newline
        jsr show_press_any_key
        rts

fog_outcome_crew:
        jsr clear_text_notice_screen
        jsr random_mod_40
        clc
        adc #9
        sta damage_lo
        lda #0
        sta damage_hi
        lda #<fog_crew_line_2
        sta src_ptr
        lda #>fog_crew_line_2
        sta src_ptr+1
        jsr print_zero_terminated
        lda damage_lo
        sta num_print_lo
        lda damage_hi
        sta num_print_hi
        jsr print_inline_word_decimal
        lda #<fog_crew_line_3
        sta src_ptr
        lda #>fog_crew_line_3
        sta src_ptr+1
        jsr print_zero_terminated
        jsr print_newline
        lda damage_lo
        clc
        adc crew_men
        sta crew_men
        lda damage_hi
        adc crew_men+1
        sta crew_men+1
fog_outcome_crew_msg:
        lda #<fog_found_island_msg
        sta src_ptr
        lda #>fog_found_island_msg
        sta src_ptr+1
        jsr print_zero_terminated
        jsr print_newline
        jsr show_press_any_key
        rts

fog_outcome_grain:
        jsr clear_text_notice_screen
        jsr random_mod_6
        clc
        adc #2
        sta damage_lo
        lda #0
        sta damage_hi
        lda damage_lo
        clc
        adc grain_sacks
        sta grain_sacks
        lda damage_hi
        adc grain_sacks+1
        sta grain_sacks+1
fog_outcome_grain_msg:
        lda #<fog_found_island_msg
        sta src_ptr
        lda #>fog_found_island_msg
        sta src_ptr+1
        jsr print_zero_terminated
        jsr print_newline
        lda #<fog_grain_line_2
        sta src_ptr
        lda #>fog_grain_line_2
        sta src_ptr+1
        jsr print_zero_terminated
        lda damage_lo
        sta num_print_lo
        lda damage_hi
        sta num_print_hi
        jsr print_inline_word_decimal
        lda #<fog_grain_line_3
        sta src_ptr
        lda #>fog_grain_line_3
        sta src_ptr+1
        jsr print_zero_terminated
        jsr print_newline
        jsr show_press_any_key
        rts

fog_outcome_plague:
        jsr clear_text_notice_screen
        jsr apply_plague_to_crew
        lda #<fog_found_island_msg
        sta src_ptr
        lda #>fog_found_island_msg
        sta src_ptr+1
        jsr print_zero_terminated
        lda #' '
        jsr $ffd2
        lda #<fog_plague_line_1_suffix
        sta src_ptr
        lda #>fog_plague_line_1_suffix
        sta src_ptr+1
        jsr print_zero_terminated
        jsr print_newline
        lda #<fog_plague_line_2
        sta src_ptr
        lda #>fog_plague_line_2
        sta src_ptr+1
        jsr print_zero_terminated
        jsr print_newline
        lda #<fog_plague_line_3
        sta src_ptr
        lda #>fog_plague_line_3
        sta src_ptr+1
        jsr print_zero_terminated
        jsr print_newline
        jsr show_press_any_key
        rts

run_cannon_fight_text:
        lda #0
        sta battle_return_to_main_choice
        jsr draw_fight_status_table

run_cannon_fight_round:
        jsr update_fight_status_values
        ldx #15
        ldy #16
        jsr set_cursor_xy
        lda #<fight_fire_prompt
        sta src_ptr
        lda #>fight_fire_prompt
        sta src_ptr+1
        jsr print_zero_terminated

fight_fire_prompt_key:
        jsr read_key_blocking
        cmp #'F'
        beq fight_fire_execute
        cmp #'f'
        beq fight_fire_execute
        cmp #'0'
        beq fight_fire_abort
        jmp fight_fire_prompt_key

fight_fire_abort:
        lda #0
        sta battle_return_to_main_choice
        rts

fight_fire_execute:
        jsr apply_cannon_round
        jsr check_ship_survival
        lda enemy_ship_hp_hi
        bne run_cannon_fight_round
        lda enemy_ship_hp_lo
        bne run_cannon_fight_round
        jsr handle_enemy_surrender
        lda #1
        sta battle_return_to_main_choice
        rts

run_boarding_fight_text:
        lda #0
        sta battle_return_to_main_choice
        jsr clear_text_notice_screen
        jsr draw_fight_status_table

boarding_round:
        jsr apply_boarding_round
        jsr check_ship_survival
        jsr update_fight_status_values
        lda enemy_crew_hi
        bne boarding_continue
        lda enemy_crew_lo
        cmp #20
        bcs boarding_continue
        jmp boarding_enemy_low
boarding_continue:
        jsr clear_boarding_text_area

        ldx #0
        ldy #6
        jsr set_screen_ptr_xy
        lda #<boarding_battle_line_1
        sta src_ptr
        lda #>boarding_battle_line_1
        sta src_ptr+1
        jsr append_zero_terminated_screen

        ldx #0
        ldy #7
        jsr set_screen_ptr_xy
        lda enemy_index
        cmp #8
        beq boarding_print_pirate_ship
        lda #<boarding_enemy_line_prefix
        sta src_ptr
        lda #>boarding_enemy_line_prefix
        sta src_ptr+1
        jsr append_zero_terminated_screen
        lda enemy_name_ptr_lo
        sta src_ptr
        lda enemy_name_ptr_hi
        sta src_ptr+1
        jsr append_zero_terminated_screen
        lda #<boarding_enemy_line_suffix
        sta src_ptr
        lda #>boarding_enemy_line_suffix
        sta src_ptr+1
        jsr append_zero_terminated_screen
        jmp boarding_ship_line_done
boarding_print_pirate_ship:
        lda #<boarding_pirate_line
        sta src_ptr
        lda #>boarding_pirate_line
        sta src_ptr+1
        jsr append_zero_terminated_screen
boarding_ship_line_done:

        ldx #0
        ldy #8
        jsr set_screen_ptr_xy
        lda #<boarding_enemy_losses_prefix
        sta src_ptr
        lda #>boarding_enemy_losses_prefix
        sta src_ptr+1
        jsr append_zero_terminated_screen
        lda boarding_last_enemy_loss
        sta num_print_lo
        lda #0
        sta num_print_hi
        jsr append_word_decimal_screen
        lda #<boarding_enemy_losses_suffix
        sta src_ptr
        lda #>boarding_enemy_losses_suffix
        sta src_ptr+1
        jsr append_zero_terminated_screen

        ldx #0
        ldy #9
        jsr set_screen_ptr_xy
        lda #<boarding_enemy_remaining_prefix
        sta src_ptr
        lda #>boarding_enemy_remaining_prefix
        sta src_ptr+1
        jsr append_zero_terminated_screen
        lda enemy_crew_lo
        sta num_print_lo
        lda enemy_crew_hi
        sta num_print_hi
        jsr append_word_decimal_screen
        lda #<boarding_enemy_remaining_suffix
        sta src_ptr
        lda #>boarding_enemy_remaining_suffix
        sta src_ptr+1
        jsr append_zero_terminated_screen

        ldx #0
        ldy #10
        jsr set_screen_ptr_xy
        lda #<boarding_player_losses_prefix
        sta src_ptr
        lda #>boarding_player_losses_prefix
        sta src_ptr+1
        jsr append_zero_terminated_screen
        lda boarding_last_player_loss
        sta num_print_lo
        lda #0
        sta num_print_hi
        jsr append_word_decimal_screen
        lda #<boarding_player_losses_suffix
        sta src_ptr
        lda #>boarding_player_losses_suffix
        sta src_ptr+1
        jsr append_zero_terminated_screen

        ldx #0
        ldy #12
        jsr set_screen_ptr_xy
        lda #<boarding_prompt_line_1
        sta src_ptr
        lda #>boarding_prompt_line_1
        sta src_ptr+1
        jsr append_zero_terminated_screen
        ldx #0
        ldy #13
        jsr set_screen_ptr_xy
        lda #<boarding_prompt_line_2
        sta src_ptr
        lda #>boarding_prompt_line_2
        sta src_ptr+1
        jsr append_zero_terminated_screen

boarding_prompt_key:
        jsr read_key_blocking
        cmp #'K'
        bne boarding_prompt_key_lower_k
        jmp boarding_round
boarding_prompt_key_lower_k:
        cmp #'k'
        bne boarding_prompt_key_upper_t
        jmp boarding_round
boarding_prompt_key_upper_t:
        cmp #'T'
        beq boarding_back_to_ship
        cmp #'t'
        beq boarding_back_to_ship
        jmp boarding_prompt_key

boarding_back_to_ship:
        lda #0
        sta battle_return_to_main_choice
        rts

boarding_enemy_low:
        jsr handle_enemy_surrender
        lda #1
        sta battle_return_to_main_choice
        rts

apply_cannon_round:
        jsr random_mod_16
        clc
        adc #8
        sta damage_lo
        lda #0
        sta damage_hi

        sec
        lda enemy_ship_hp_lo
        sbc damage_lo
        sta enemy_ship_hp_lo
        lda enemy_ship_hp_hi
        sbc damage_hi
        bcs cannon_enemy_hp_ok
        lda #0
        sta enemy_ship_hp_lo
        sta enemy_ship_hp_hi
cannon_enemy_hp_ok:

        jsr random_mod_8
        clc
        adc #1
        cmp ship_repair_points
        bcc cannon_apply_repair_damage
        lda #0
        sta ship_repair_points
        rts
cannon_apply_repair_damage:
        sta damage_lo
        sec
        lda ship_repair_points
        sbc damage_lo
        sta ship_repair_points
        rts

apply_boarding_round:
        jsr random_mod_8
        clc
        adc #2
        sta damage_lo
        sta boarding_last_player_loss
        lda #0
        sta damage_hi

        sec
        lda crew_men
        sbc damage_lo
        sta crew_men
        lda crew_men+1
        sbc damage_hi
        bcs boarding_player_crew_ok
        lda #0                      // clamp to 0 on underflow
        sta crew_men
        sta crew_men+1
        jmp boarding_player_done
boarding_player_crew_ok:
        sta crew_men+1
boarding_player_done:

        jsr random_mod_16
        clc
        adc #7
        sta damage_lo
        sta boarding_last_enemy_loss
        lda #0
        sta damage_hi

        lda enemy_crew_lo
        sta enemy_crew_prev_lo
        lda enemy_crew_hi
        sta enemy_crew_prev_hi

        sec
        lda enemy_crew_lo
        sbc damage_lo
        sta enemy_crew_lo
        lda enemy_crew_hi
        sbc damage_hi
        bcs boarding_enemy_ok
        lda #0
        sta enemy_crew_lo
        sta enemy_crew_hi
boarding_enemy_ok:
        lda enemy_crew_hi
        cmp enemy_crew_prev_hi
        bcc boarding_enemy_monotonic_ok
        bne boarding_enemy_force_decrease
        lda enemy_crew_lo
        cmp enemy_crew_prev_lo
        bcc boarding_enemy_monotonic_ok
boarding_enemy_force_decrease:
        lda enemy_crew_prev_hi
        ora enemy_crew_prev_lo
        beq boarding_enemy_monotonic_ok
        sec
        lda enemy_crew_prev_lo
        sbc #1
        sta enemy_crew_lo
        lda enemy_crew_prev_hi
        sbc #0
        sta enemy_crew_hi
boarding_enemy_monotonic_ok:
        rts

handle_enemy_surrender:
        jsr clear_text_notice_screen
        lda #<surrender_line_1
        sta src_ptr
        lda #>surrender_line_1
        sta src_ptr+1
        jsr print_zero_terminated
        jsr print_newline

        clc
        lda treasury_rigsdaler
        adc enemy_rgsd_lo
        sta treasury_rigsdaler
        lda treasury_rigsdaler+1
        adc enemy_rgsd_hi
        sta treasury_rigsdaler+1

        clc
        lda grain_sacks
        adc enemy_grain
        sta grain_sacks
        bcc surrender_show_loot
        inc grain_sacks+1

surrender_show_loot:
        lda #<surrender_loot_rgsd_prefix
        sta src_ptr
        lda #>surrender_loot_rgsd_prefix
        sta src_ptr+1
        jsr print_zero_terminated
        lda enemy_rgsd_lo
        sta num_print_lo
        lda enemy_rgsd_hi
        sta num_print_hi
        jsr print_inline_word_decimal
        lda #<surrender_loot_rgsd_suffix
        sta src_ptr
        lda #>surrender_loot_rgsd_suffix
        sta src_ptr+1
        jsr print_zero_terminated
        jsr print_newline

        lda #<surrender_loot_men_prefix
        sta src_ptr
        lda #>surrender_loot_men_prefix
        sta src_ptr+1
        jsr print_zero_terminated
        lda enemy_crew_lo
        sta num_print_lo
        lda enemy_crew_hi
        sta num_print_hi
        jsr print_inline_word_decimal
        lda #<surrender_loot_men_suffix
        sta src_ptr
        lda #>surrender_loot_men_suffix
        sta src_ptr+1
        jsr print_zero_terminated
        jsr print_newline

        lda #<surrender_loot_grain_prefix
        sta src_ptr
        lda #>surrender_loot_grain_prefix
        sta src_ptr+1
        jsr print_zero_terminated
        lda enemy_grain
        sta num_print_lo
        lda #0
        sta num_print_hi
        jsr print_inline_word_decimal
        lda #<surrender_loot_grain_suffix
        sta src_ptr
        lda #>surrender_loot_grain_suffix
        sta src_ptr+1
        jsr print_zero_terminated
        jsr print_newline
        lda #<surrender_loot_take_line
        sta src_ptr
        lda #>surrender_loot_take_line
        sta src_ptr+1
        jsr print_zero_terminated
        jsr print_newline
        jsr print_newline

        // Required prize crew = 5 + floor(enemy crew / 2), matching BASIC flow.
        lda enemy_crew_hi
        lsr
        sta damage_hi
        lda enemy_crew_lo
        ror
        sta damage_lo
        clc
        lda damage_lo
        adc #5
        sta damage_lo
        lda damage_hi
        adc #0
        sta damage_hi

        lda #<surrender_prize_need_line_1
        sta src_ptr
        lda #>surrender_prize_need_line_1
        sta src_ptr+1
        jsr print_zero_terminated
        jsr print_newline
        lda #<surrender_prize_need_line_2_prefix
        sta src_ptr
        lda #>surrender_prize_need_line_2_prefix
        sta src_ptr+1
        jsr print_zero_terminated
        lda damage_lo
        sta num_print_lo
        lda damage_hi
        sta num_print_hi
        jsr print_inline_word_decimal
        lda #<surrender_prize_need_line_2_suffix
        sta src_ptr
        lda #>surrender_prize_need_line_2_suffix
        sta src_ptr+1
        jsr print_zero_terminated
        jsr print_newline
        lda #<surrender_prize_have_prefix
        sta src_ptr
        lda #>surrender_prize_have_prefix
        sta src_ptr+1
        jsr print_zero_terminated
        lda crew_men
        sta num_print_lo
        lda crew_men+1
        sta num_print_hi
        jsr print_inline_word_decimal
        lda #<surrender_prize_have_suffix
        sta src_ptr
        lda #>surrender_prize_have_suffix
        sta src_ptr+1
        jsr print_zero_terminated
        jsr print_newline
        jsr print_newline

        lda #<surrender_line_2
        sta src_ptr
        lda #>surrender_line_2
        sta src_ptr+1
        jsr print_zero_terminated
        jsr print_newline
        lda #<surrender_line_3
        sta src_ptr
        lda #>surrender_line_3
        sta src_ptr+1
        jsr print_zero_terminated

surrender_prize_key:
        jsr read_key_blocking
        cmp #'S'
        beq surrender_sink
        cmp #'s'
        beq surrender_sink
        cmp #'P'
        beq surrender_prize
        cmp #'p'
        beq surrender_prize
        jmp surrender_prize_key

surrender_sink:
        clc
        lda crew_men
        adc enemy_crew_lo
        sta crew_men
        lda crew_men+1
        adc enemy_crew_hi
        sta crew_men+1
        rts

surrender_prize:
        sec
        lda crew_men
        sbc damage_lo
        sta crew_men
        lda crew_men+1
        sbc damage_hi
        sta crew_men+1
        jsr print_newline
        jsr print_newline
        lda #<surrender_prize_sailing_line
        sta src_ptr
        lda #>surrender_prize_sailing_line
        sta src_ptr+1
        jsr print_zero_terminated
        rts

init_enemy_from_roll:
        jsr random_enemy_roll
        sta enemy_index

init_enemy_from_index:
        sec
        sbc #1
        tax

        lda enemy_name_ptr_table_lo,x
        sta enemy_name_ptr_lo
        lda enemy_name_ptr_table_hi,x
        sta enemy_name_ptr_hi

        lda enemy_cannons_table,x
        sta enemy_cannons_lo
        lda #0
        sta enemy_cannons_hi

        lda enemy_crew_table_lo,x
        sta enemy_crew_lo
        lda enemy_crew_table_hi,x
        sta enemy_crew_hi

        lda enemy_rgsd_table_lo,x
        sta enemy_rgsd_lo
        lda enemy_rgsd_table_hi,x
        sta enemy_rgsd_hi

        lda enemy_grain_table,x
        sta enemy_grain

        jsr random_mod_64
        clc
        adc #100
        sta enemy_ship_hp_lo
        lda #0
        sta enemy_ship_hp_hi
        rts

random_enemy_roll:
        jsr random_mod_8
        clc
        adc #1
        rts

random_mod_10:
        lda $dc04
        and #$0f
        cmp #10
        bcc random_mod_10_done
        sec
        sbc #10
random_mod_10_done:
        rts

random_mod_16:
        lda $dc04
        and #$0f
        rts

random_mod_8:
        lda $dc05
        and #$07
        rts

random_mod_6:
        lda $dc05
        and #$07
        cmp #6
        bcs random_mod_6
        rts

random_mod_40:
        lda $dc04
        cmp #240
        bcs random_mod_40
random_mod_40_reduce:
        cmp #40
        bcc random_mod_40_done
        sec
        sbc #40
        jmp random_mod_40_reduce
random_mod_40_done:
        rts

random_mod_64:
        lda $dc04
        and #$3f
        rts

clear_boarding_text_area:
        ldy #8
        sty fight_stat_row
clear_boarding_text_area_row:
        ldx #0
        ldy fight_stat_row
        jsr set_screen_ptr_xy
        ldy #0
        ldx #40
clear_boarding_text_area_col:
        lda #' '
        sta (dst_ptr),y
        iny
        dex
        bne clear_boarding_text_area_col
        inc fight_stat_row
        lda fight_stat_row
        cmp #15
        bcc clear_boarding_text_area_row
        rts

clear_text_notice_screen:
        lda #$93
        jsr $ffd2
        lda #$05
        jsr $ffd2
        rts

print_player_naval_title:
        lda player_naval_title_index
        sec
        sbc #1
        tax
        lda naval_title_ptr_lo,x
        sta src_ptr
        lda naval_title_ptr_hi,x
        sta src_ptr+1
        jmp print_zero_terminated

print_player_noble_title:
        lda player_noble_title_index
        sec
        sbc #1
        tax
        lda noble_title_ptr_lo,x
        sta src_ptr
        lda noble_title_ptr_hi,x
        sta src_ptr+1
        jmp print_zero_terminated

print_namebuf_zero_terminated:
        ldy #0
print_namebuf_loop:
        lda namebuf,y
        beq print_namebuf_done
        jsr $ffd2
        iny
        bne print_namebuf_loop
print_namebuf_done:
        rts

apply_plague_to_crew:
        // BASIC-like outcome: roughly one third of crew is lost.
        lda crew_men
        sta crew_work_lo
        lda crew_men+1
        sta crew_work_hi
        lda #0
        sta crew_loss_lo
        sta crew_loss_hi

crew_loss_loop:
        lda crew_work_hi
        bne crew_loss_subtract
        lda crew_work_lo
        cmp #3
        bcc crew_loss_done
crew_loss_subtract:
        sec
        lda crew_work_lo
        sbc #3
        sta crew_work_lo
        lda crew_work_hi
        sbc #0
        sta crew_work_hi

        inc crew_loss_lo
        bne crew_loss_loop
        inc crew_loss_hi
        jmp crew_loss_loop

crew_loss_done:
        sec
        lda crew_men
        sbc crew_loss_lo
        sta crew_men
        lda crew_men+1
        sbc crew_loss_hi
        sta crew_men+1
        rts

show_press_any_key:
        lda #<press_any_key_msg
        sta src_ptr
        lda #>press_any_key_msg
        sta src_ptr+1
        jsr print_zero_terminated

show_press_any_key_wait:
        jsr $ffe4
        beq show_press_any_key_wait
        rts

// BASIC line 5000: die if crew_men <= 10 or ship_repair_points <= 20.
// Resets the stack and jumps to ship_sank if either threshold is breached.
check_ship_survival:
        lda crew_men+1
        bne check_survival_ok       // hi byte set → > 255, survives
        lda crew_men
        cmp #11
        bcc check_survival_sink     // crew <= 10 → sink
check_survival_ok:
        lda ship_repair_points
        cmp #21
        bcc check_survival_sink     // repair <= 20 → sink
        rts
check_survival_sink:
        jmp ship_sank

// BASIC lines 1170-1171: win when the score reaches its target or the
// current voyage reaches its turn target. This runs before hazards or upkeep.
check_victory_conditions:
        lda player_points+1
        cmp point_limit+1
        bcc check_victory_turn_limit
        bne player_won_stage
        lda player_points
        cmp point_limit
        bcs player_won_stage

check_victory_turn_limit:
        lda voyage_turn_counter+1
        cmp turn_limit+1
        bcc check_victory_not_yet
        bne player_won_stage
        lda voyage_turn_counter
        cmp turn_limit
        bcc check_victory_not_yet
player_won_stage:
        jmp show_victory_and_advance
check_victory_not_yet:
        rts

// BASIC lines 6100-6119 and 102. Advance the difficulty, then calculate
// the next point and turn targets from the current score and voyage count.
show_victory_and_advance:
        jsr hide_ship
        jsr configure_intro_vic_bank
        jsr prepare_text_notice_screen
        lda #<victory_msg
        sta src_ptr
        lda #>victory_msg
        sta src_ptr+1
        jsr print_zero_terminated
        jsr print_newline
        lda #<victory_next_goal_msg
        sta src_ptr
        lda #>victory_next_goal_msg
        sta src_ptr+1
        jsr print_zero_terminated
        jsr print_newline
        jsr show_press_any_key

        inc difficulty_level
        inc difficulty_level
        lda player_naval_title_index
        cmp #5
        bcs victory_title_indices_done
        inc player_naval_title_index
        inc player_noble_title_index
victory_title_indices_done:

        // IPOINTLIM = IPOINT + 500 + 250 * IDIF.
        lda #<500
        sta target_add_lo
        lda #>500
        sta target_add_hi
        ldx difficulty_level
victory_point_add_loop:
        cpx #0
        beq victory_point_target
        clc
        lda target_add_lo
        adc #<250
        sta target_add_lo
        lda target_add_hi
        adc #>250
        sta target_add_hi
        dex
        bne victory_point_add_loop
victory_point_target:
        clc
        lda player_points
        adc target_add_lo
        sta point_limit
        lda player_points+1
        adc target_add_hi
        sta point_limit+1

        // ITURLIM = ITUR + 325 - 12.5 * IDIF; IDIF is always even.
        lda #<325
        sta target_add_lo
        lda #>325
        sta target_add_hi
        ldx difficulty_level
        txa
        lsr
        tax
victory_turn_subtract_loop:
        cpx #0
        beq victory_turn_target
        sec
        lda target_add_lo
        sbc #25
        sta target_add_lo
        lda target_add_hi
        sbc #0
        sta target_add_hi
        dex
        bne victory_turn_subtract_loop
victory_turn_target:
        clc
        lda voyage_turn_counter
        adc target_add_lo
        sta turn_limit
        lda voyage_turn_counter+1
        adc target_add_hi
        sta turn_limit+1

        jsr draw_map
        jsr load_ship_sprites
        jsr sync_ship_to_grid_position
        jsr show_ship
        jmp game_loop_wait

ship_sank:
        ldx #$ff
        txs
        jsr configure_intro_vic_bank
        jsr prepare_text_notice_screen
        lda #<ship_sank_msg
        sta src_ptr
        lda #>ship_sank_msg
        sta src_ptr+1
        jsr print_zero_terminated
        jsr print_newline
        lda crew_men
        sta num_print_lo
        lda crew_men+1
        sta num_print_hi
        lda #<ship_sank_men_prefix
        sta src_ptr
        lda #>ship_sank_men_prefix
        sta src_ptr+1
        jsr print_zero_terminated
        jsr print_inline_word_decimal
        lda #<ship_sank_rep_prefix
        sta src_ptr
        lda #>ship_sank_rep_prefix
        sta src_ptr+1
        jsr print_zero_terminated
        lda ship_repair_points
        sta num_print_lo
        lda #0
        sta num_print_hi
        jsr print_inline_word_decimal
        jsr print_newline
        lda player_points
        sta num_print_lo
        lda player_points+1
        sta num_print_hi
        lda #<ship_sank_score_prefix
        sta src_ptr
        lda #>ship_sank_score_prefix
        sta src_ptr+1
        jsr print_zero_terminated
        jsr print_inline_word_decimal
        jsr print_newline
        jsr show_press_any_key
        rts

read_key_blocking:
        jsr $ffe4
        beq read_key_blocking
        rts

print_newline:
        lda #13
        jsr $ffd2
        rts

print_double_newline:
        jsr print_newline
        jsr print_newline
        rts

set_cursor_xy:
        // Callers pass X=column, Y=row.
        // KERNAL PLOT set-cursor expects C=0, X=row, Y=column.
        txa
        pha
        tya
        tax
        pla
        tay
        clc
        jsr $fff0
        rts

// ---------------------------------------------------------------
// Bitmap text rendering: draw PETSCII characters into the hires
// map bitmap (bank 1, $6000) using the charset at INTRO_CHARSET_RAM.
// ---------------------------------------------------------------

// Draw one PETSCII character at (bmp_char_col, bmp_char_row).
// A = PETSCII code.  Clobbers A, X, Y, bmp_char_ptr ($f7/$f8), dst_ptr, $f9.
draw_bmp_char:
        jsr petscii_to_screen_code
        // bmp_char_ptr = INTRO_CHARSET_RAM + screen_code * 8
        sta $f9
        lda #0
        asl $f9                     // shift 3× collecting carry into A
        rol
        asl $f9
        rol
        asl $f9
        rol
        clc
        adc #>INTRO_CHARSET_RAM
        sta bmp_char_ptr+1
        lda $f9
        sta bmp_char_ptr
        // dst_ptr = bmp_row_base[row] + col * 8
        ldx bmp_char_row
        lda bmp_row_base_lo, x
        sta dst_ptr
        lda bmp_row_base_hi, x
        sta dst_ptr+1
        lda bmp_char_col
        sta $f9
        lda #0
        asl $f9
        rol
        asl $f9
        rol
        asl $f9
        rol
        sta $fa                     // high byte of col*8 (use $fa to keep bmp_char_ptr intact)
        lda $f9
        clc
        adc dst_ptr
        sta dst_ptr
        lda $fa
        adc dst_ptr+1
        sta dst_ptr+1
        // Copy 8 pixel rows from charset to bitmap
        ldy #7
dbc_loop:
        lda (bmp_char_ptr), y
        sta (dst_ptr), y
        dey
        bpl dbc_loop
        // Set screen RAM color byte: white fg (1), black bg (0) = $10
        ldx bmp_char_row
        lda clr_row_base_lo, x
        sta dst_ptr
        lda clr_row_base_hi, x
        sta dst_ptr+1
        ldy bmp_char_col
        lda #$86                    // orange fg (8) on dark blue bg (6)
        sta (dst_ptr), y
        rts

// Print a zero-terminated PETSCII string into the bitmap.
// src_ptr = string address.  bmp_char_col/bmp_char_row must be set.
// Advances bmp_char_col for each character printed.
print_bmp_string:
        ldy #0
pbs_loop:
        lda (src_ptr), y
        beq pbs_done
        sty bmp_str_idx             // save string index across draw_bmp_char
        jsr draw_bmp_char
        inc bmp_char_col
        ldy bmp_str_idx
        iny
        bne pbs_loop
pbs_done:
        rts

// Render num_print_lo/hi as decimal digits at (bmp_char_col, bmp_char_row).
// bmp_char_col is advanced past the last digit.
print_bmp_decimal:
        lda num_print_lo
        ora num_print_hi
        beq pbd_zero
        lda #<bmp_decimal_buf
        sta dst_ptr
        lda #>bmp_decimal_buf
        sta dst_ptr+1
        jsr print_word_decimal
        ldy #0
pbd_digit_loop:
        cpy num_print_out_index
        beq pbd_done
        lda bmp_decimal_buf, y
        sty bmp_str_idx
        jsr draw_bmp_char
        inc bmp_char_col
        ldy bmp_str_idx
        iny
        bne pbd_digit_loop
        rts
pbd_zero:
        lda #<bmp_decimal_buf
        sta dst_ptr
        lda #>bmp_decimal_buf
        sta dst_ptr+1
        jsr print_word_decimal      // writes '0' to bmp_decimal_buf[0]
        lda bmp_decimal_buf
        jsr draw_bmp_char
        inc bmp_char_col
pbd_done:
        rts

// Clear a 6-char field with spaces then print decimal at col+1.
// X = start column, Y = row.  num_print_lo/hi = value.
print_bmp_stat_word:
        stx bmp_char_col
        sty bmp_char_row
        lda #6
        sta bmp_field_cnt
pbsw_clear:
        lda #' '
        jsr draw_bmp_char
        inc bmp_char_col
        dec bmp_field_cnt
        bne pbsw_clear
        // reset to col+1 (started at col, incremented 6 times, go back 5)
        lda bmp_char_col
        sec
        sbc #5
        sta bmp_char_col
        jsr print_bmp_decimal
        rts

// Draw label rows and current stat values into the map bitmap.
draw_map_status_table:
        // Row 21: three label cells, no pipe chars
        lda #1
        sta bmp_char_col
        lda #21
        sta bmp_char_row
        lda #<map_label_trk
        sta src_ptr
        lda #>map_label_trk
        sta src_ptr+1
        jsr print_bmp_string
        lda #14
        sta bmp_char_col
        lda #<map_label_rgsdl
        sta src_ptr
        lda #>map_label_rgsdl
        sta src_ptr+1
        jsr print_bmp_string
        lda #27
        sta bmp_char_col
        lda #<map_label_skyts
        sta src_ptr
        lda #>map_label_skyts
        sta src_ptr+1
        jsr print_bmp_string
        // Row 23: three label cells, no pipe chars
        lda #1
        sta bmp_char_col
        lda #23
        sta bmp_char_row
        lda #<map_label_maend
        sta src_ptr
        lda #>map_label_maend
        sta src_ptr+1
        jsr print_bmp_string
        lda #14
        sta bmp_char_col
        lda #<map_label_point
        sta src_ptr
        lda #>map_label_point
        sta src_ptr+1
        jsr print_bmp_string
        lda #27
        sta bmp_char_col
        lda #<map_label_rep
        sta src_ptr
        lda #>map_label_rep
        sta src_ptr+1
        jsr print_bmp_string
        jsr update_map_bmp_values
        // Row 0, col 2: POINT label + value (load only; points change via events)
        lda #2
        sta bmp_char_col
        lda #0
        sta bmp_char_row
        lda #<map_label_point
        sta src_ptr
        lda #>map_label_point
        sta src_ptr+1
        jsr print_bmp_string
        lda #' '                    // separator space before value
        jsr draw_bmp_char
        inc bmp_char_col
        lda player_points
        sta num_print_lo
        lda player_points+1
        sta num_print_hi
        jsr print_bmp_decimal
        lda #' '                    // one trailing space
        jsr draw_bmp_char
        // Row 0, col 16: goal display (M]L: x P] y TR[K)
        lda #16
        sta bmp_char_col
        lda #<map_goal_prefix
        sta src_ptr
        lda #>map_goal_prefix
        sta src_ptr+1
        jsr print_bmp_string
        lda point_limit
        sta num_print_lo
        lda point_limit+1
        sta num_print_hi
        jsr print_bmp_decimal
        lda #<map_goal_mid
        sta src_ptr
        lda #>map_goal_mid
        sta src_ptr+1
        jsr print_bmp_string
        lda turn_limit
        sta num_print_lo
        lda turn_limit+1
        sta num_print_hi
        jsr print_bmp_decimal
        lda #<map_goal_suffix
        sta src_ptr
        lda #>map_goal_suffix
        sta src_ptr+1
        jsr print_bmp_string
        rts

// Update only the two values that change during navigation.
update_map_move_values:
        ldx #6
        ldy #21
        lda voyage_turn_counter
        sta num_print_lo
        lda voyage_turn_counter+1
        sta num_print_hi
        jsr print_bmp_stat_word
        ldx #33
        ldy #23
        lda ship_repair_points
        sta num_print_lo
        lda #0
        sta num_print_hi
        jsr print_bmp_stat_word
        lda #' '                    // clear cell right of last digit in case REP shrank
        jsr draw_bmp_char
        rts

// Write current game stats as decimal into the map bitmap status area.
update_map_bmp_values:
        // TR[K (voyage_turn_counter)
        ldx #6
        ldy #21
        lda voyage_turn_counter
        sta num_print_lo
        lda voyage_turn_counter+1
        sta num_print_hi
        jsr print_bmp_stat_word
        // RGSDL
        ldx #20
        ldy #21
        lda treasury_rigsdaler
        sta num_print_lo
        lda treasury_rigsdaler+1
        sta num_print_hi
        jsr print_bmp_stat_word
        // SKYTS
        ldx #33
        ldy #21
        lda ship_cannons
        sta num_print_lo
        lda ship_cannons+1
        sta num_print_hi
        jsr print_bmp_stat_word
        // M[ND
        ldx #6
        ldy #23
        lda crew_men
        sta num_print_lo
        lda crew_men+1
        sta num_print_hi
        jsr print_bmp_stat_word
        // POINT
        ldx #20
        ldy #23
        lda player_points
        sta num_print_lo
        lda player_points+1
        sta num_print_hi
        jsr print_bmp_stat_word
        // REP
        ldx #33
        ldy #23
        lda ship_repair_points
        sta num_print_lo
        lda #0
        sta num_print_hi
        jsr print_bmp_stat_word
        rts

draw_fight_status_table:
        ldx #0
        ldy #20
        lda #<fight_status_border_top
        sta src_ptr
        lda #>fight_status_border_top
        sta src_ptr+1
        jsr write_zero_terminated_screen

        ldx #0
        ldy #21
        lda #<fight_status_row_1
        sta src_ptr
        lda #>fight_status_row_1
        sta src_ptr+1
        jsr write_zero_terminated_screen

        ldx #0
        ldy #22
        lda #<fight_status_border_middle
        sta src_ptr
        lda #>fight_status_border_middle
        sta src_ptr+1
        jsr write_zero_terminated_screen

        ldx #0
        ldy #23
        lda #<fight_status_row_2
        sta src_ptr
        lda #>fight_status_row_2
        sta src_ptr+1
        jsr write_zero_terminated_screen

        ldx #0
        ldy #24
        lda #<fight_status_border_bottom
        sta src_ptr
        lda #>fight_status_border_bottom
        sta src_ptr+1
        jsr write_zero_terminated_screen

        jsr update_fight_status_values
        rts

update_fight_status_values:
        // TR[ K
        ldx #7
        ldy #21
        lda voyage_turn_counter
        sta num_print_lo
        lda voyage_turn_counter+1
        sta num_print_hi
        jsr print_fight_word_stat

        // RGSDL
        lda treasury_rigsdaler
        sta num_print_lo
        lda treasury_rigsdaler+1
        sta num_print_hi
        ldx #20
        ldy #21
        jsr print_fight_word_stat

        // SKYTS
        lda ship_cannons
        sta num_print_lo
        lda ship_cannons+1
        sta num_print_hi
        ldx #33
        ldy #21
        jsr print_fight_word_stat

        // M[ND
        lda crew_men
        sta num_print_lo
        lda crew_men+1
        sta num_print_hi
        ldx #7
        ldy #23
        jsr print_fight_word_stat

        // POINT
        lda player_points
        sta num_print_lo
        lda player_points+1
        sta num_print_hi
        ldx #20
        ldy #23
        jsr print_fight_word_stat

        // REP
        lda ship_repair_points
        ldx #31
        ldy #23
        jsr print_fight_byte_stat
        rts

print_fight_byte_stat:
        sta num_print_lo
        lda #0
        sta num_print_hi
        jmp print_fight_word_stat

print_fight_word_stat:
        stx fight_stat_col
        sty fight_stat_row
        jsr set_screen_ptr_xy

        ldy #0
        lda #' '
clear_fight_value_field_loop:
        sta (dst_ptr),y
        iny
        cpy #6
        bne clear_fight_value_field_loop

        inc dst_ptr
        bne print_fight_word_stat_ptr_ok
        inc dst_ptr+1
print_fight_word_stat_ptr_ok:
        jsr print_word_decimal
        rts

set_screen_ptr_xy:
        lda screen_row_addr_lo,y
        sta dst_ptr
        lda screen_row_addr_hi,y
        sta dst_ptr+1
        txa
        clc
        adc dst_ptr
        sta dst_ptr
        bcc set_screen_ptr_xy_done
        inc dst_ptr+1
set_screen_ptr_xy_done:
        rts

write_zero_terminated_screen:
        jsr set_screen_ptr_xy
        ldy #0
write_zero_terminated_screen_loop:
        lda (src_ptr),y
        beq write_zero_terminated_screen_done
        jsr petscii_to_screen_code
        sta (dst_ptr),y
        iny
        bne write_zero_terminated_screen_loop
write_zero_terminated_screen_done:
        rts

append_zero_terminated_screen:
        ldy #0
append_zero_terminated_screen_loop:
        lda (src_ptr),y
        beq append_zero_terminated_screen_done
        jsr petscii_to_screen_code
        sta (dst_ptr),y
        iny
        bne append_zero_terminated_screen_loop
append_zero_terminated_screen_done:
        tya
        clc
        adc dst_ptr
        sta dst_ptr
        bcc append_zero_terminated_screen_ptr_ok
        inc dst_ptr+1
append_zero_terminated_screen_ptr_ok:
        rts

append_word_decimal_screen:
        lda num_print_lo
        ora num_print_hi
        beq append_word_decimal_screen_zero
        jsr print_word_decimal
        lda num_print_out_index
        jmp append_word_decimal_screen_advance
append_word_decimal_screen_zero:
        jsr print_word_decimal
        lda #1
append_word_decimal_screen_advance:
        clc
        adc dst_ptr
        sta dst_ptr
        bcc append_word_decimal_screen_ptr_ok
        inc dst_ptr+1
append_word_decimal_screen_ptr_ok:
        rts

petscii_to_screen_code:
        // Uppercase/graphics conversion for direct screen RAM writes.
        // $40-$5F  -> $00-$1F
        // $60-$7F  -> $40-$5F
        // $A0-$BF  -> $60-$7F
        cmp #$40
        bcc petscii_to_screen_done
        cmp #$60
        bcc petscii_to_screen_sub_64
        cmp #$80
        bcc petscii_to_screen_sub_32
        cmp #$a0
        bcc petscii_to_screen_done
        cmp #$c0
        bcs petscii_to_screen_done
petscii_to_screen_sub_64:
        sec
        sbc #$40
        jmp petscii_to_screen_done
petscii_to_screen_sub_32:
        sec
        sbc #$20
petscii_to_screen_done:
        rts

print_word_decimal:
        lda num_print_lo
        ora num_print_hi
        bne print_word_decimal_begin
        ldy #0
        lda #'0'
        sta (dst_ptr),y
        rts

print_word_decimal_begin:
        lda #0
        sta num_print_started
        sta num_print_out_index
        ldx #0

print_word_decimal_digit:
        ldy #0
print_word_decimal_subtract_check:
        lda num_print_hi
        cmp decimal_divisor_hi,x
        bcc print_word_decimal_digit_ready
        bne print_word_decimal_subtract
        lda num_print_lo
        cmp decimal_divisor_lo,x
        bcc print_word_decimal_digit_ready
print_word_decimal_subtract:
        sec
        lda num_print_lo
        sbc decimal_divisor_lo,x
        sta num_print_lo
        lda num_print_hi
        sbc decimal_divisor_hi,x
        sta num_print_hi
        iny
        bne print_word_decimal_subtract_check

print_word_decimal_digit_ready:
        tya
        bne print_word_decimal_emit
        lda num_print_started
        bne print_word_decimal_emit
        cpx #4
        bne print_word_decimal_next

print_word_decimal_emit:
        tya
        clc
        adc #'0'
        ldy num_print_out_index
        sta (dst_ptr),y
        inc num_print_out_index
        lda #1
        sta num_print_started

print_word_decimal_next:
        inx
        cpx #5
        bne print_word_decimal_digit
        rts

print_inline_word_decimal:
        lda #' '
        jsr $ffd2
        ldx #0
print_inline_word_decimal_clear:
        lda #0
        sta inline_decimal_buffer,x
        inx
        cpx #6
        bne print_inline_word_decimal_clear
        lda #<inline_decimal_buffer
        sta dst_ptr
        lda #>inline_decimal_buffer
        sta dst_ptr+1
        jsr print_word_decimal
        lda #<inline_decimal_buffer
        sta src_ptr
        lda #>inline_decimal_buffer
        sta src_ptr+1
        jsr print_zero_terminated
        lda #' '
        jsr $ffd2
        rts

on_harbour_entered:
        sta last_harbour_id
        jmp show_text_message_and_restore_map

show_text_message_and_restore_map:
        jsr hide_ship
        jsr configure_intro_vic_bank
        jsr prepare_text_notice_screen

        lda #<harbour_entry_prefix_msg
        sta src_ptr
        lda #>harbour_entry_prefix_msg
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
        lda #13
        jsr $ffd2
        lda #13
        jsr $ffd2

        lda #<press_any_key_msg
        sta src_ptr
        lda #>press_any_key_msg
        sta src_ptr+1
        jsr print_zero_terminated

wait_message_key:
        jsr $ffe4
        beq wait_message_key

        jsr draw_map
        jsr load_ship_sprites
        jsr sync_ship_to_grid_position
        jsr show_ship
        rts

prepare_text_notice_screen:
        // Ensure text mode while showing event/harbour notices.
        lda $d011
        and #$df
        sta $d011
        lda $d016
        and #$ef
        sta $d016

        lda #$93
        jsr $ffd2
        lda #$05
        jsr $ffd2
        rts

print_zero_terminated:
        ldy #0
print_zero_terminated_loop:
        lda (src_ptr),y
        beq print_zero_terminated_done
        jsr $ffd2
        iny
        bne print_zero_terminated_loop
print_zero_terminated_done:
        rts

copy_len_bytes:
copy_len_loop:
        lda len_lo
        ora len_hi
        beq copy_len_done

        ldy #0
        lda (src_ptr),y
        sta (dst_ptr),y

        inc src_ptr
        bne src_ptr_ok
        inc src_ptr+1
src_ptr_ok:
        inc dst_ptr
        bne dst_ptr_ok
        inc dst_ptr+1
dst_ptr_ok:

        lda len_lo
        bne dec_len_lo
        dec len_hi
dec_len_lo:
        dec len_lo
        jmp copy_len_loop

copy_len_done:
        rts

verify_len_bytes:
verify_len_loop:
        lda len_lo
        ora len_hi
        beq verify_len_done

        ldy #0
        lda (src_ptr),y
        cmp (dst_ptr),y
        bne verify_len_fail

        inc src_ptr
        bne verify_src_ok
        inc src_ptr+1
verify_src_ok:
        inc dst_ptr
        bne verify_dst_ok
        inc dst_ptr+1
verify_dst_ok:

        lda len_lo
        bne verify_dec_lo
        dec len_hi
verify_dec_lo:
        dec len_lo
        jmp verify_len_loop

verify_len_fail:
        lda #2
        sta $d020
verify_fail_loop:
        jmp verify_fail_loop

verify_len_done:
        rts

copy_screen_row_40:
        ldy #0
copy_screen_row_40_loop:
        lda (src_ptr),y
        sta screen_tmp
        // Normalize one-color cells: if either nibble is zero, duplicate the nonzero nibble.
        and #$0f
        bne copy_screen_row_check_high_zero
        lda screen_tmp
        lsr
        lsr
        lsr
        lsr
        ora screen_tmp
        sta screen_tmp
        jmp copy_screen_row_fix_zero_store

copy_screen_row_check_high_zero:
        lda screen_tmp
        and #$f0
        bne copy_screen_row_fix_zero_store
        lda screen_tmp
        asl
        asl
        asl
        asl
        ora screen_tmp
        sta screen_tmp

copy_screen_row_fix_zero_store:
        lda screen_tmp
        sta (dst_ptr),y

copy_screen_row_next:
        iny
        cpy #40
        beq copy_screen_row_40_done
        jmp copy_screen_row_40_loop

copy_screen_row_40_done:

        clc
        lda src_ptr
        adc #$28
        sta src_ptr
        bcc copy_row_src_advance_done
        inc src_ptr+1
copy_row_src_advance_done:

        clc
        lda dst_ptr
        adc #$28
        sta dst_ptr
        bcc copy_row_dst_advance_done
        inc dst_ptr+1
copy_row_dst_advance_done:
        rts

map_screen_src: .word 0
map_screen_dst: .word 0
screen_tmp: .byte 0

// Logical map navigation state (map-local).
map_ship_col: .byte MAP_START_COL
map_ship_row: .byte MAP_START_ROW
ship_prev_x: .byte 0
ship_prev_x_hi: .byte 0
ship_prev_y: .byte 0
map_query_col: .byte 0
map_query_row: .byte 0
map_query_bit_mask: .byte 0
map_query_byte_index: .byte 0
current_map_cell_type: .byte 0
last_harbour_id: .byte 0
ship_collision_counter: .byte 0
difficulty_level: .byte 0
target_add_lo: .byte 0
target_add_hi: .byte 0
crew_work_lo: .byte 0
crew_work_hi: .byte 0
crew_loss_lo: .byte 0
crew_loss_hi: .byte 0
enemy_index: .byte 1
enemy_name_ptr_lo: .byte 0
enemy_name_ptr_hi: .byte 0
enemy_cannons_lo: .byte 0
enemy_cannons_hi: .byte 0
enemy_crew_lo: .byte 0
enemy_crew_hi: .byte 0
enemy_crew_prev_lo: .byte 0
enemy_crew_prev_hi: .byte 0
enemy_rgsd_lo: .byte 0
enemy_rgsd_hi: .byte 0
enemy_grain: .byte 0
enemy_ship_hp_lo: .byte 0
enemy_ship_hp_hi: .byte 0
boarding_last_player_loss: .byte 0
boarding_last_enemy_loss: .byte 0
battle_return_to_main_choice: .byte 0
damage_lo: .byte 0
damage_hi: .byte 0
num_print_lo: .byte 0
num_print_hi: .byte 0
num_print_started: .byte 0
num_print_out_index: .byte 0
inline_decimal_buffer: .fill 6, 0
fight_stat_col: .byte 0
fight_stat_row: .byte 0
bmp_char_col:   .byte 0
bmp_char_row:   .byte 0
bmp_str_idx:    .byte 0
bmp_field_cnt:  .byte 0
bmp_decimal_buf:.fill 6, 0
player_naval_title_index: .byte 1
player_noble_title_index: .byte 1

// Temporary state for pixel->cell conversion.
map_rel_lo: .byte 0
map_rel_hi: .byte 0

map_harbour_col: .byte 25,25,18,9,7,11,24
map_harbour_row: .byte 12,7,8,1,4,13,3
map_harbour_id:  .byte 2,3,4,5,6,7,8

map_harbour_name_ptr_lo:
        .byte <harbour_copenhagen_msg
        .byte <harbour_helsingor_msg
        .byte <harbour_hundested_msg
        .byte <harbour_grenaa_msg
        .byte <harbour_ebeltoft_msg
        .byte <harbour_kalundborg_msg
        .byte <harbour_molle_msg
map_harbour_name_ptr_hi:
        .byte >harbour_copenhagen_msg
        .byte >harbour_helsingor_msg
        .byte >harbour_hundested_msg
        .byte >harbour_grenaa_msg
        .byte >harbour_ebeltoft_msg
        .byte >harbour_kalundborg_msg
        .byte >harbour_molle_msg

// For each column 1..29, select which of 4 row-mask bytes to test and which bit.
map_col_byte_index:
        .byte 0,0,0,0,0,0,0,0,1,1,1,1,1,1,1,1,2,2,2,2,2,2,2,2,3,3,3,3,3
map_col_bit_mask:
        .byte $01,$02,$04,$08,$10,$20,$40,$80,$01,$02,$04,$08,$10,$20,$40,$80
        .byte $01,$02,$04,$08,$10,$20,$40,$80,$01,$02,$04,$08,$10

// Land mask rows from original DOS map data (1 bit per column, 29 columns/row).
map_land_row_masks:
        .byte $80,$00,$00,$0C
        .byte $D8,$00,$00,$0C
        .byte $76,$00,$00,$09
        .byte $52,$00,$00,$0E
        .byte $4A,$00,$00,$06
        .byte $02,$00,$70,$04
        .byte $02,$00,$F8,$0C
        .byte $32,$30,$F0,$08
        .byte $23,$C4,$F4,$1A
        .byte $20,$C0,$F4,$10
        .byte $20,$60,$F4,$10
        .byte $04,$FE,$F7,$01
        .byte $00,$98,$FF,$01
        .byte $00,$04,$F0,$01
        .byte $00,$00,$F0,$00

press_any_key_msg:      .text "TRYK P] EN TAST FOR AT FORTS[TTE!"
                        .byte 0
encounter_lookout_msg:  .text "UDKIGGEN MELDER:"
                        .byte 0
encounter_ship_ahoy_msg:.text "SHIP OH\J!"
                        .byte 0
encounter_english_msg:  .text "ENGELSK "
                        .byte 0
encounter_in_sight_msg: .text " I SIGTE"
                        .byte 0
encounter_title_prefix_msg:
                        .text "N] "
                        .byte 0
encounter_choose_msg:   .text "NU M] DU V[LGE:"
                        .byte 0
encounter_prompt_fight_flee:
                        .text "VIL DU ANGRIBE (A) ELLER FLYGTE (F)?"
                        .byte 0
encounter_prompt_fire_board_1:
                        .text "VIL DU BORDE HENDE (B) ELLER"
                        .byte 0
encounter_prompt_fire_board_2:
                        .text "VIL DU SKYDE (S) P] HENDE?"
                        .byte 0
fight_fire_prompt:      .text "F=FYR. 0=AFBRYD KAMPEN"
                        .byte 0
fight_status_border_top:
                        .byte 176
                        .fill 12,96
                        .byte 178
                        .fill 12,96
                        .byte 178
                        .fill 12,96
                        .byte 174,0
fight_status_border_middle:
                        .byte 171
                        .fill 12,96
                        .byte 123
                        .fill 12,96
                        .byte 123
                        .fill 12,96
                        .byte 179,0
fight_status_border_bottom:
                        .byte 173
                        .fill 12,96
                        .byte 177
                        .fill 12,96
                        .byte 177
                        .fill 12,96
                        .byte 189,0
fight_status_row_1:
                        .byte 98
                        .text "TR[K:       "
                        .byte 98
                        .text "RGSDL:      "
                        .byte 98
                        .text "SKYTS:      "
                        .byte 98
                        .byte 0
fight_status_row_2:
                        .byte 98
                        .text "M[ND:       "
                        .byte 98
                        .text "POINT:      "
                        .byte 98
                        .text "REP:        "
                        .byte 98
                        .byte 0
ship_sank_msg:          .text "DIT SKIB GIKK NED MED MAND OG MUS!"
                        .byte 0
ship_sank_men_prefix:   .text "M[ND: "
                        .byte 0
ship_sank_rep_prefix:   .text "  REP: "
                        .byte 0
ship_sank_score_prefix: .text "DU FIK "
                        .byte 0
victory_msg:            .text "TILLYKKE!!!!"
                        .byte 0
victory_next_goal_msg:  .text "DU HAR N]ET M]LET. N[STE M]L VENTER!"
                        .byte 0
boarding_battle_line_1: .text "KAMPEN RASER OMBORD P]"
                        .byte 0
map_label_trk:          .text "TR[K:"
                        .byte 0
map_label_rgsdl:        .text "RGSDL:"
                        .byte 0
map_label_skyts:        .text "SKYTS:"
                        .byte 0
map_label_maend:        .text "M[ND:"
                        .byte 0
map_label_point:        .text "POINT:"
                        .byte 0
map_goal_prefix:        .text "M]L: "
                        .byte 0
map_goal_mid:           .text " P] "
                        .byte 0
map_goal_suffix:        .text " TR[K "
                        .byte 0
map_label_rep:          .text "REP:"
                        .byte 0
boarding_enemy_line_prefix:
                        .text "DEN ENGELSKE "
                        .byte 0
boarding_enemy_line_suffix:
                        .text " !"
                        .byte 0
boarding_pirate_line:   .text "S\R\VERSKIBET!"
                        .byte 0
boarding_enemy_losses_prefix:
                        .text "FJENDEN HAR "
                        .byte 0
boarding_enemy_losses_suffix:
                        .text " FALDNE,"
                        .byte 0
boarding_enemy_remaining_prefix:
                        .text "MEN HAN HAR "
                        .byte 0
boarding_enemy_remaining_suffix:
                        .text " IGEN."
                        .byte 0
boarding_player_losses_prefix:
                        .text "DU HAR MISTET "
                        .byte 0
boarding_player_losses_suffix:
                        .text " AF DINE TAPRE M[ND."
                        .byte 0
boarding_prompt_line_1: .text "VIL DU K[MPE VIDERE (K)"
                        .byte 0
boarding_prompt_line_2: .text "ELLER VENDE TILBAGE TIL DIT SKIB (T)?"
                        .byte 0
surrender_line_1:       .text "DE OVERGIVER SIG!!"
                        .byte 0
surrender_line_2:       .text "VIL DU S[NKE DET, (S)"
                        .byte 0
surrender_line_3:       .text "ELLER TAGE DET SOM PRISE (P) ?"
                        .byte 0
surrender_loot_rgsd_prefix:
                        .text "DER ER"
                        .byte 0
surrender_loot_rgsd_suffix:
                        .text "RIGSDALER OMBORD."
                        .byte 0
surrender_loot_men_prefix:
                        .text "DER ER"
                        .byte 0
surrender_loot_men_suffix:
                        .text "OVERLEVENDE OMBORD."
                        .byte 0
surrender_loot_grain_prefix:
                        .text "DER ER"
                        .byte 0
surrender_loot_grain_suffix:
                        .text "S[KKE KORN OMBORD."
                        .byte 0
surrender_loot_take_line:
                        .text "DEM TAGER DU!"
                        .byte 0
surrender_prize_need_line_1:
                        .text "DETTE SKIB BEH\\VER ET PRISEMANDSKAB"
                        .byte 0
surrender_prize_need_line_2_prefix:
                        .text "P]"
                        .byte 0
surrender_prize_need_line_2_suffix:
                        .text "MAND!"
                        .byte 0
surrender_prize_have_prefix:
                        .text "DU HAR"
                        .byte 0
surrender_prize_have_suffix:
                        .text "MAND."
                        .byte 0
surrender_prize_sailing_line:
                        .text "OK, DE ER P] VEJ TIL K\BENHAVN."
                        .byte 0

fog_line_1:             .text "UDKIGGEN MELDER:"
                        .byte 0
fog_line_2:             .text "OH\J!"
                        .byte 0
fog_line_3:             .text "DER ER EN UNDERLIG T]GEBANKE 2 S\MIL"
                        .byte 0
fog_line_4:             .text "TIL STYRBORD,"
                        .byte 0
fog_line_5:             .text "DET SER UD TIL, AT DER ER NOGET INDE I DEN."
                        .byte 0
fog_prompt_msg:         .text "VIL DU UNDERS\GE DET (J/N)?"
                        .byte 0
fog_enemy_line_1:       .text "DER DUKKER AF DISEN"
                        .byte 0
fog_enemy_line_2:       .text "ET V[MMELIGT SKIB!"
                        .byte 0
fog_found_island_msg:   .text "DU FINDER EN \DE \"
                        .byte 0
fog_treasure_line_2:    .text "MED EN SKATTEKISTE,"
                        .byte 0
fog_treasure_line_3:    .text "DER INDEHOLDER"
                        .byte 0
fog_treasure_line_4:    .text "RIGSDALER!"
                        .byte 0
fog_jewel_line_2:       .text "MED"
                        .byte 0
fog_jewel_line_3:       .text "JUVELER!"
                        .byte 0
fog_cannon_line_2:      .text "HVOR DER LIGGER EN KANON!"
                        .byte 0
fog_crew_line_2:        .text "MED"
                        .byte 0
fog_crew_line_3:        .text "SKIBBRUDNE, SOM TAGER HYRE!"
                        .byte 0
fog_nothing_line_1:     .text "N\H!"
                        .byte 0
fog_nothing_line_2:     .text "DER VAR ALLIGEVEL IKKE NOGET."
                        .byte 0
fog_grain_line_2:       .text "MED"
                        .byte 0
fog_grain_line_3:       .text "S[KKE KORN!"
                        .byte 0
fog_plague_line_1_suffix:
                        .text "MED EN PESTSYG SKIBBRUDEN!"
                        .byte 0
fog_plague_line_2:      .text "HAN SMITTER DIN BES[TNING, OG EN TREDIEDEL D\R!"
                        .byte 0
fog_plague_line_3:      .text "RESTEN ER IKKE ALT FOR BEGEJSTREDE FOR DIT LEDERSKAB!"
                        .byte 0

naval_title_ptr_lo:
                        .byte <naval_title_1
                        .byte <naval_title_2
                        .byte <naval_title_3
                        .byte <naval_title_4
                        .byte <naval_title_5
naval_title_ptr_hi:
                        .byte >naval_title_1
                        .byte >naval_title_2
                        .byte >naval_title_3
                        .byte >naval_title_4
                        .byte >naval_title_5
noble_title_ptr_lo:
                        .byte <noble_title_1
                        .byte <noble_title_2
                        .byte <noble_title_3
                        .byte <noble_title_4
                        .byte <noble_title_5
noble_title_ptr_hi:
                        .byte >noble_title_1
                        .byte >noble_title_2
                        .byte >noble_title_3
                        .byte >noble_title_4
                        .byte >noble_title_5

naval_title_1:          .text "SKIBSF\RER"
                        .byte 0
naval_title_2:          .text "KAPTAJN"
                        .byte 0
naval_title_3:          .text "KOMMAND\RKAPTAJN"
                        .byte 0
naval_title_4:          .text "KOMMAND\R"
                        .byte 0
naval_title_5:          .text "ADMIRAL"
                        .byte 0

noble_title_1:          .text "BORGER "
                        .byte 0
noble_title_2:          .text "BARON "
                        .byte 0
noble_title_3:          .text "GREV "
                        .byte 0
noble_title_4:          .text "KAMMERHERRE "
                        .byte 0
noble_title_5:          .text "KRONPRINS "
                        .byte 0

enemy_name_ptr_table_lo:
                        .byte <enemy_name_1
                        .byte <enemy_name_2
                        .byte <enemy_name_3
                        .byte <enemy_name_4
                        .byte <enemy_name_5
                        .byte <enemy_name_6
                        .byte <enemy_name_7
                        .byte <enemy_name_8
enemy_name_ptr_table_hi:
                        .byte >enemy_name_1
                        .byte >enemy_name_2
                        .byte >enemy_name_3
                        .byte >enemy_name_4
                        .byte >enemy_name_5
                        .byte >enemy_name_6
                        .byte >enemy_name_7
                        .byte >enemy_name_8
enemy_cannons_table:
                        .byte 5,10,2,15,6,1,50,70
enemy_crew_table_lo:
                        .byte <80,<480,<40,<140,<50,<10,<140,<200
enemy_crew_table_hi:
                        .byte >80,>480,>40,>140,>50,>10,>140,>200
enemy_rgsd_table_lo:
                        .byte <150,<600,<150,<450,<200,<30,<900,<1500
enemy_rgsd_table_hi:
                        .byte >150,>600,>150,>450,>200,>30,>900,>1500
enemy_grain_table:
                        .byte 30,15,4,5,4,1,20,12

enemy_name_1:           .text "HANDELSMAND"
                        .byte 0
enemy_name_2:           .text "TROPPETRANSPORT"
                        .byte 0
enemy_name_3:           .text "KANONB]D"
                        .byte 0
enemy_name_4:           .text "GALEASE"
                        .byte 0
enemy_name_5:           .text "BRIG"
                        .byte 0
enemy_name_6:           .text "SKONNERT"
                        .byte 0
enemy_name_7:           .text "ORLOGSMAND"
                        .byte 0
enemy_name_8:           .text "S\R\VERSKIB"
                        .byte 0
harbour_copenhagen_msg: .text "K\BENHAVN"
                        .byte 0
harbour_helsingor_msg:  .text "HELSING\R"
                        .byte 0
harbour_hundested_msg:  .text "HUNDESTED"
                        .byte 0
harbour_grenaa_msg:     .text "GREN]"
                        .byte 0
harbour_ebeltoft_msg:   .text "EBELTOFT"
                        .byte 0
harbour_kalundborg_msg: .text "KALUNDBORG"
                        .byte 0
harbour_molle_msg:      .text "M\LLE"
                        .byte 0
harbour_entry_prefix_msg:
                        .text "DU ER P] VEJ IND I HAVNEN I "
                        .byte 0

decimal_divisor_lo:
                        .byte <10000,<1000,<100,<10,<1
decimal_divisor_hi:
                        .byte >10000,>1000,>100,>10,>1

// Bitmap base addresses per text row: $6000 + row * 320
bmp_row_base_lo:
        .byte <($6000+0*320),<($6000+1*320),<($6000+2*320),<($6000+3*320),<($6000+4*320)
        .byte <($6000+5*320),<($6000+6*320),<($6000+7*320),<($6000+8*320),<($6000+9*320)
        .byte <($6000+10*320),<($6000+11*320),<($6000+12*320),<($6000+13*320),<($6000+14*320)
        .byte <($6000+15*320),<($6000+16*320),<($6000+17*320),<($6000+18*320),<($6000+19*320)
        .byte <($6000+20*320),<($6000+21*320),<($6000+22*320),<($6000+23*320),<($6000+24*320)
bmp_row_base_hi:
        .byte >($6000+0*320),>($6000+1*320),>($6000+2*320),>($6000+3*320),>($6000+4*320)
        .byte >($6000+5*320),>($6000+6*320),>($6000+7*320),>($6000+8*320),>($6000+9*320)
        .byte >($6000+10*320),>($6000+11*320),>($6000+12*320),>($6000+13*320),>($6000+14*320)
        .byte >($6000+15*320),>($6000+16*320),>($6000+17*320),>($6000+18*320),>($6000+19*320)
        .byte >($6000+20*320),>($6000+21*320),>($6000+22*320),>($6000+23*320),>($6000+24*320)
// Color RAM base addresses per text row: $4400 + row * 40
clr_row_base_lo:
        .byte <($4400+0*40),<($4400+1*40),<($4400+2*40),<($4400+3*40),<($4400+4*40)
        .byte <($4400+5*40),<($4400+6*40),<($4400+7*40),<($4400+8*40),<($4400+9*40)
        .byte <($4400+10*40),<($4400+11*40),<($4400+12*40),<($4400+13*40),<($4400+14*40)
        .byte <($4400+15*40),<($4400+16*40),<($4400+17*40),<($4400+18*40),<($4400+19*40)
        .byte <($4400+20*40),<($4400+21*40),<($4400+22*40),<($4400+23*40),<($4400+24*40)
clr_row_base_hi:
        .byte >($4400+0*40),>($4400+1*40),>($4400+2*40),>($4400+3*40),>($4400+4*40)
        .byte >($4400+5*40),>($4400+6*40),>($4400+7*40),>($4400+8*40),>($4400+9*40)
        .byte >($4400+10*40),>($4400+11*40),>($4400+12*40),>($4400+13*40),>($4400+14*40)
        .byte >($4400+15*40),>($4400+16*40),>($4400+17*40),>($4400+18*40),>($4400+19*40)
        .byte >($4400+20*40),>($4400+21*40),>($4400+22*40),>($4400+23*40),>($4400+24*40)
screen_row_addr_lo:
                        .byte <INTRO_SCREEN_RAM,<(INTRO_SCREEN_RAM+$28),<(INTRO_SCREEN_RAM+$50),<(INTRO_SCREEN_RAM+$78),<(INTRO_SCREEN_RAM+$a0)
                        .byte <(INTRO_SCREEN_RAM+$c8),<(INTRO_SCREEN_RAM+$f0),<(INTRO_SCREEN_RAM+$118),<(INTRO_SCREEN_RAM+$140),<(INTRO_SCREEN_RAM+$168)
                        .byte <(INTRO_SCREEN_RAM+$190),<(INTRO_SCREEN_RAM+$1b8),<(INTRO_SCREEN_RAM+$1e0),<(INTRO_SCREEN_RAM+$208),<(INTRO_SCREEN_RAM+$230)
                        .byte <(INTRO_SCREEN_RAM+$258),<(INTRO_SCREEN_RAM+$280),<(INTRO_SCREEN_RAM+$2a8),<(INTRO_SCREEN_RAM+$2d0),<(INTRO_SCREEN_RAM+$2f8)
                        .byte <(INTRO_SCREEN_RAM+$320),<(INTRO_SCREEN_RAM+$348),<(INTRO_SCREEN_RAM+$370),<(INTRO_SCREEN_RAM+$398),<(INTRO_SCREEN_RAM+$3c0)
screen_row_addr_hi:
                        .byte >INTRO_SCREEN_RAM,>(INTRO_SCREEN_RAM+$28),>(INTRO_SCREEN_RAM+$50),>(INTRO_SCREEN_RAM+$78),>(INTRO_SCREEN_RAM+$a0)
                        .byte >(INTRO_SCREEN_RAM+$c8),>(INTRO_SCREEN_RAM+$f0),>(INTRO_SCREEN_RAM+$118),>(INTRO_SCREEN_RAM+$140),>(INTRO_SCREEN_RAM+$168)
                        .byte >(INTRO_SCREEN_RAM+$190),>(INTRO_SCREEN_RAM+$1b8),>(INTRO_SCREEN_RAM+$1e0),>(INTRO_SCREEN_RAM+$208),>(INTRO_SCREEN_RAM+$230)
                        .byte >(INTRO_SCREEN_RAM+$258),>(INTRO_SCREEN_RAM+$280),>(INTRO_SCREEN_RAM+$2a8),>(INTRO_SCREEN_RAM+$2d0),>(INTRO_SCREEN_RAM+$2f8)
                        .byte >(INTRO_SCREEN_RAM+$320),>(INTRO_SCREEN_RAM+$348),>(INTRO_SCREEN_RAM+$370),>(INTRO_SCREEN_RAM+$398),>(INTRO_SCREEN_RAM+$3c0)

* = $9000 "MapArt"
map_art_data:
        .import source "map_art_bytes.inc"

// map.art layout: 2-byte load address + 8000 bitmap bytes + 1000 screen bytes + trailing metadata
.label map_bitmap_data  = map_art_data + $0002
.label map_screen_data  = map_art_data + $1f42
.label map_border_color = map_art_data + $232a
