.label src_ptr = $fb
.label dst_ptr = $fd
.label len_lo = $f9
.label len_hi = $fa

// Normalize one-color cells by duplicating the nonzero nibble into the zero nibble.

draw_map:
        sei
        jsr hide_ship

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
        lda #<$4000
        sta map_screen_dst
        lda #>$4000
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

        // Switch to hires bitmap mode only after the map is fully loaded.
        lda #$3b
        sta $d011

        lda #$08
        sta $d016

        lda $dd00
        and #$fc
        ora #$02
        sta $dd00

        lda #$08
        sta $d018

        jsr hide_ship
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
        sta ship_collision_counter
        lda #200
        sta ship_repair_points
        lda #0
        sta last_harbour_id
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
        rts

on_random_sea_event:
        lda $dc05
        and #$01
        beq random_event_enemy_ship
        lda #<random_event_fog_msg
        sta src_ptr
        lda #>random_event_fog_msg
        sta src_ptr+1
        jmp show_text_message_and_restore_map

random_event_enemy_ship:
        lda #<random_event_ship_msg
        sta src_ptr
        lda #>random_event_ship_msg
        sta src_ptr+1
        jmp show_text_message_and_restore_map

on_harbour_entered:
        sta last_harbour_id
        sec
        sbc #2
        tax
        lda map_harbour_name_ptr_lo,x
        sta src_ptr
        lda map_harbour_name_ptr_hi,x
        sta src_ptr+1
        jmp show_text_message_and_restore_map

show_text_message_and_restore_map:
        jsr hide_ship
        jsr restore_main_vic_bank

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

        jsr print_zero_terminated
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
        jsr set_sprite_pointers_map
        jsr sync_ship_to_grid_position
        jsr show_ship
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

press_any_key_msg:      .text "TRYK EN TAST"
                        .byte 0
random_event_fog_msg:   .text "UDKIGGEN MELDER: T]GEBANKE I SIGTE."
                        .byte 0
random_event_ship_msg:  .text "UDKIGGEN MELDER: FJENDTLIGT SKIB I SIGTE."
                        .byte 0
harbour_copenhagen_msg: .text "DU ER P] VEJ IND I HAVNEN I K[BENHAVN."
                        .byte 0
harbour_helsingor_msg:  .text "DU ER P] VEJ IND I HAVNEN I HELSING\R."
                        .byte 0
harbour_hundested_msg:  .text "DU ER P] VEJ IND I HAVNEN I HUNDESTED."
                        .byte 0
harbour_grenaa_msg:     .text "DU ER P] VEJ IND I HAVNEN I GREN]."
                        .byte 0
harbour_ebeltoft_msg:   .text "DU ER P] VEJ IND I HAVNEN I EBELTOFT."
                        .byte 0
harbour_kalundborg_msg: .text "DU ER P] VEJ IND I HAVNEN I KALUNDBORG."
                        .byte 0
harbour_molle_msg:      .text "DU ER P] VEJ IND I HAVNEN I M\LLE."
                        .byte 0

map_art_data:
        .import source "map_art_bytes.inc"

// map.art layout: 2-byte load address + 8000 bitmap bytes + 1000 screen bytes + trailing metadata
.label map_bitmap_data  = map_art_data + $0002
.label map_screen_data  = map_art_data + $1f42
.label map_border_color = map_art_data + $232a
