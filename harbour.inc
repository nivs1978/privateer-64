// Harbour trading screen (BASIC lines 1500-1720). The sailing-into-harbour
// mini-game (BASIC 1260-1490) is intentionally skipped: entering a harbour
// tile shows this buy/sell menu directly.
// K\benhavn also pays out any prize-ship crew/rigsdaler that made it home
// safely (BASIC lines 1501-1507, tied to the probability in BASIC line 3019).

harbour_price_crew:    .byte 0
harbour_price_repair:  .byte 0
harbour_price_cannon:  .byte 0
harbour_price_grain:   .byte 0
harbour_price_jewel:   .byte 0

harbour_qty_lo:        .byte 0
harbour_qty_hi:        .byte 0
harbour_qty_digits:    .byte 0
harbour_qty_buf:       .byte 0,0,0

harbour_cost_lo:       .byte 0
harbour_cost_hi:       .byte 0
harbour_sum_lo:        .byte 0
harbour_sum_hi:        .byte 0

price_base:            .byte 0
price_low_adjust:      .byte 0
price_result:          .byte 0
price_rnd:             .byte 0
price_prod_lo:         .byte 0
price_prod_hi:         .byte 0
price_add_lo:          .byte 0
price_add_hi:          .byte 0

mul_price_byte:        .byte 0
mul_a_lo:              .byte 0
mul_a_hi:              .byte 0
mul_overflow:          .byte 0

div_value_lo:          .byte 0
div_value_hi:          .byte 0
div_remainder:         .byte 0

// Buy targets indexed 0=crew,1=cannon,2=grain, with the cap each must stay under.
harbour_resource_ptr_lo: .byte <crew_men, <ship_cannons, <grain_sacks
harbour_resource_ptr_hi: .byte >crew_men, >ship_cannons, >grain_sacks
harbour_resource_cap_lo: .byte <500, <150, <700
harbour_resource_cap_hi: .byte >500, >150, >700

// Sell targets indexed 0=grain,1=cannon,2=jewel.
harbour_sell_resource_ptr_lo: .byte <grain_sacks, <ship_cannons, <jewel_count
harbour_sell_resource_ptr_hi: .byte >grain_sacks, >ship_cannons, >jewel_count

// harbour_generate_prices loop state: base price and output byte address per index.
harbour_price_gen_index: .byte 0
harbour_price_base_table: .byte 10, 8, 100, 5, 50
harbour_price_out_lo: .byte <harbour_price_crew, <harbour_price_repair, <harbour_price_cannon, <harbour_price_grain, <harbour_price_jewel
harbour_price_out_hi: .byte >harbour_price_crew, >harbour_price_repair, >harbour_price_cannon, >harbour_price_grain, >harbour_price_jewel

// harbour_print_menu loop state: prefix/suffix text and price byte per line (0..3).
harbour_menu_print_index: .byte 0
harbour_menu_prefix_lo: .byte <harbour_menu_line1_prefix, <harbour_menu_line2_prefix, <harbour_menu_line3_prefix, <harbour_menu_line4_prefix
harbour_menu_prefix_hi: .byte >harbour_menu_line1_prefix, >harbour_menu_line2_prefix, >harbour_menu_line3_prefix, >harbour_menu_line4_prefix
harbour_menu_suffix_lo: .byte <harbour_menu_line1_suffix, <harbour_menu_line2_suffix, <harbour_menu_line3_suffix, <harbour_menu_line4_suffix
harbour_menu_suffix_hi: .byte >harbour_menu_line1_suffix, >harbour_menu_line2_suffix, >harbour_menu_line3_suffix, >harbour_menu_line4_suffix
harbour_menu_price_lo: .byte <harbour_price_crew, <harbour_price_repair, <harbour_price_cannon, <harbour_price_grain
harbour_menu_price_hi: .byte >harbour_price_crew, >harbour_price_repair, >harbour_price_cannon, >harbour_price_grain

// Entry point called from map.inc's resolve_pixel_move with A = harbour id (2..8).
on_harbour_entered:
        sta last_harbour_id
        jsr hide_ship
        jsr configure_intro_vic_bank
        jsr prepare_text_notice_screen
        jsr harbour_sailing_approach
        jsr hide_ship

        jsr harbour_check_copenhagen_prize
        jsr harbour_generate_prices

harbour_menu_loop:
        jsr clear_text_notice_screen
        jsr harbour_show_welcome
        jsr print_newline
        jsr draw_fight_status_table
        jsr harbour_print_menu

harbour_menu_key:
        jsr read_key_blocking
        cmp #'1'
        bne harbour_menu_key_not_1
        jmp harbour_menu_buy_crew
harbour_menu_key_not_1:
        cmp #'2'
        bne harbour_menu_key_not_2
        jmp harbour_menu_buy_repair
harbour_menu_key_not_2:
        cmp #'3'
        bne harbour_menu_key_not_3
        jmp harbour_menu_buy_cannon
harbour_menu_key_not_3:
        cmp #'4'
        bne harbour_menu_key_not_4
        jmp harbour_menu_buy_grain
harbour_menu_key_not_4:
        cmp #'5'
        bne harbour_menu_key_not_5
        jmp harbour_menu_sell
harbour_menu_key_not_5:
        cmp #'6'
        beq harbour_menu_exit
        jmp harbour_menu_key

harbour_menu_exit:
        jsr draw_map
        jsr load_ship_sprites
        jsr sync_ship_to_grid_position
        jsr show_ship
        rts

// ---------------------------------------------------------------
// Buying: crew, repair points, cannons, grain.
// Crew/cannon/grain share harbour_buy_generic (X = resource index 0..2);
// repair has no cap and clamps a byte instead, so it stays separate.

harbour_menu_buy_crew:
        jsr harbour_prompt_qty
        lda harbour_price_crew
        jsr multiply_qty_price
        ldx #0
        jmp harbour_buy_generic

harbour_menu_buy_repair:
        jsr harbour_prompt_qty
        lda harbour_price_repair
        jsr multiply_qty_price
        jsr harbour_check_afford
        bcc harbour_buy_repair_fail

        jsr harbour_charge_cost
        clc
        lda ship_repair_points
        adc harbour_qty_lo
        sta ship_repair_points
        lda ship_repair_points+1
        adc harbour_qty_hi
        sta ship_repair_points+1
        jmp harbour_menu_loop
harbour_buy_repair_fail:
        jsr harbour_show_cant_afford
        jmp harbour_menu_loop

harbour_menu_buy_cannon:
        jsr harbour_prompt_qty
        lda harbour_price_cannon
        jsr multiply_qty_price
        ldx #1
        jmp harbour_buy_generic

harbour_menu_buy_grain:
        jsr harbour_prompt_qty
        lda harbour_price_grain
        jsr multiply_qty_price
        ldx #2
        jmp harbour_buy_generic

// X = resource index into harbour_resource_ptr/cap tables (0=crew,1=cannon,2=grain).
// harbour_cost_lo/hi (from multiply_qty_price) and harbour_qty_lo/hi must be set.
harbour_buy_generic:
        jsr harbour_check_afford
        bcs harbour_buy_generic_afford_ok
        jsr harbour_show_cant_afford
        jmp harbour_menu_loop
harbour_buy_generic_afford_ok:
        lda harbour_resource_ptr_lo,x
        sta len_lo
        lda harbour_resource_ptr_hi,x
        sta len_hi
        ldy #0
        clc
        lda (len_lo),y
        adc harbour_qty_lo
        sta harbour_sum_lo
        iny
        lda (len_lo),y
        adc harbour_qty_hi
        sta harbour_sum_hi

        lda harbour_sum_hi
        cmp harbour_resource_cap_hi,x
        bcc harbour_buy_generic_apply
        bne harbour_buy_generic_toomany
        lda harbour_sum_lo
        cmp harbour_resource_cap_lo,x
        bcc harbour_buy_generic_apply
harbour_buy_generic_toomany:
        jsr harbour_show_too_many
        jmp harbour_menu_loop
harbour_buy_generic_apply:
        jsr harbour_charge_cost
        ldy #0
        lda harbour_sum_lo
        sta (len_lo),y
        iny
        lda harbour_sum_hi
        sta (len_lo),y
        jmp harbour_menu_loop

// ---------------------------------------------------------------
// Selling: grain, cannons, jewels (share harbour_sell_generic, X = resource index 0..2).

harbour_menu_sell:
        lda #<harbour_sell_prompt_msg
        sta src_ptr
        lda #>harbour_sell_prompt_msg
        sta src_ptr+1
        jsr print_zero_terminated

harbour_sell_key:
        jsr read_key_blocking
        cmp #'K'
        beq harbour_sell_grain
        cmp #'k'
        beq harbour_sell_grain
        cmp #'S'
        bne harbour_sell_key_not_s
        jmp harbour_sell_cannon
harbour_sell_key_not_s:
        cmp #'s'
        bne harbour_sell_key_not_s2
        jmp harbour_sell_cannon
harbour_sell_key_not_s2:
        cmp #'J'
        bne harbour_sell_key_not_j
        jmp harbour_sell_jewel
harbour_sell_key_not_j:
        cmp #'j'
        bne harbour_sell_key_not_j2
        jmp harbour_sell_jewel
harbour_sell_key_not_j2:
        jmp harbour_sell_key

harbour_sell_grain:
        jsr print_newline
        jsr harbour_prompt_qty
        lda harbour_price_grain
        jsr multiply_qty_price
        ldx #0
        jmp harbour_sell_generic

harbour_sell_cannon:
        jsr print_newline
        jsr harbour_prompt_qty
        lda harbour_price_cannon
        jsr multiply_qty_price
        ldx #1
        jmp harbour_sell_generic

harbour_sell_jewel:
        jsr print_newline
        jsr harbour_prompt_qty
        lda harbour_price_jewel
        jsr multiply_qty_price
        ldx #2
        jmp harbour_sell_generic

// X = resource index into harbour_sell_resource_ptr table (0=grain,1=cannon,2=jewel).
// harbour_cost_lo/hi (from multiply_qty_price) and harbour_qty_lo/hi must be set.
harbour_sell_generic:
        lda harbour_sell_resource_ptr_lo,x
        sta len_lo
        lda harbour_sell_resource_ptr_hi,x
        sta len_hi
        ldy #0
        lda (len_lo),y
        sta harbour_sum_lo
        iny
        lda (len_lo),y
        sta harbour_sum_hi

        lda harbour_qty_hi
        cmp harbour_sum_hi
        bcc harbour_sell_generic_ok
        bne harbour_sell_generic_toomuch
        lda harbour_qty_lo
        cmp harbour_sum_lo
        bcc harbour_sell_generic_ok
        beq harbour_sell_generic_ok
harbour_sell_generic_toomuch:
        jsr harbour_show_dont_have
        jmp harbour_menu_loop
harbour_sell_generic_ok:
        jsr harbour_pay_proceeds
        sec
        lda harbour_sum_lo
        sbc harbour_qty_lo
        sta harbour_sum_lo
        lda harbour_sum_hi
        sbc harbour_qty_hi
        sta harbour_sum_hi
        ldy #0
        lda harbour_sum_lo
        sta (len_lo),y
        iny
        lda harbour_sum_hi
        sta (len_lo),y
        jmp harbour_menu_loop

// ---------------------------------------------------------------
// Shared helpers.

harbour_prompt_qty:
        lda #<harbour_prompt_qty_msg
        sta src_ptr
        lda #>harbour_prompt_qty_msg
        sta src_ptr+1
        jsr print_zero_terminated
        jsr read_qty_input
        jsr print_newline
        lda harbour_qty_lo
        ora harbour_qty_hi
        bne harbour_prompt_qty_have_value
        // Cancelled (qty=0): discard our own return address and go straight to the menu.
        pla
        pla
        jmp harbour_menu_loop
harbour_prompt_qty_have_value:
        rts

// A = carry set if harbour_cost <= treasury_rigsdaler.
harbour_check_afford:
        lda harbour_cost_hi
        cmp treasury_rigsdaler+1
        bcc harbour_check_afford_yes
        bne harbour_check_afford_no
        lda harbour_cost_lo
        cmp treasury_rigsdaler
        beq harbour_check_afford_yes
        bcc harbour_check_afford_yes
harbour_check_afford_no:
        clc
        rts
harbour_check_afford_yes:
        sec
        rts

harbour_charge_cost:
        sec
        lda treasury_rigsdaler
        sbc harbour_cost_lo
        sta treasury_rigsdaler
        lda treasury_rigsdaler+1
        sbc harbour_cost_hi
        sta treasury_rigsdaler+1
        rts

harbour_pay_proceeds:
        clc
        lda treasury_rigsdaler
        adc harbour_cost_lo
        sta treasury_rigsdaler
        lda treasury_rigsdaler+1
        adc harbour_cost_hi
        sta treasury_rigsdaler+1
        rts

// A/Y = low/high of the zero-terminated message to show, then wait for a key.
harbour_show_message:
        sta src_ptr
        sty src_ptr+1
        jsr print_zero_terminated
        jsr print_newline
        jmp show_press_any_key

harbour_show_cant_afford:
        lda #<harbour_cant_afford_msg
        ldy #>harbour_cant_afford_msg
        jmp harbour_show_message

harbour_show_too_many:
        lda #<harbour_too_many_msg
        ldy #>harbour_too_many_msg
        jmp harbour_show_message

harbour_show_dont_have:
        lda #<harbour_dont_have_msg
        ldy #>harbour_dont_have_msg
        jmp harbour_show_message

harbour_show_welcome:
        lda #<harbour_welcome_prefix_msg
        sta src_ptr
        lda #>harbour_welcome_prefix_msg
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
        jmp print_newline

// K\benhavn only (harbour id 2): pay out any prize crew/rigsdaler in transit.
harbour_check_copenhagen_prize:
        lda last_harbour_id
        cmp #2
        beq harbour_check_copenhagen_is_kbh
        jmp harbour_check_copenhagen_prize_done
harbour_check_copenhagen_is_kbh:

        lda pending_prize_crew_lo
        ora pending_prize_crew_hi
        ora pending_prize_rigsdaler_lo
        ora pending_prize_rigsdaler_hi
        bne harbour_check_copenhagen_has_pending
        jmp harbour_check_copenhagen_prize_done
harbour_check_copenhagen_has_pending:

        lda #<harbour_prize_line_1
        sta src_ptr
        lda #>harbour_prize_line_1
        sta src_ptr+1
        jsr print_zero_terminated
        jsr print_newline
        lda #<harbour_prize_line_2_prefix
        sta src_ptr
        lda #>harbour_prize_line_2_prefix
        sta src_ptr+1
        jsr print_zero_terminated
        lda pending_prize_crew_lo
        sta num_print_lo
        lda pending_prize_crew_hi
        sta num_print_hi
        jsr print_inline_word_decimal
        jsr print_newline
        lda #<harbour_prize_line_3
        sta src_ptr
        lda #>harbour_prize_line_3
        sta src_ptr+1
        jsr print_zero_terminated
        jsr print_newline
        lda #<harbour_prize_line_4_prefix
        sta src_ptr
        lda #>harbour_prize_line_4_prefix
        sta src_ptr+1
        jsr print_zero_terminated
        lda pending_prize_rigsdaler_lo
        sta num_print_lo
        lda pending_prize_rigsdaler_hi
        sta num_print_hi
        jsr print_inline_word_decimal
        lda #<harbour_prize_line_4_suffix
        sta src_ptr
        lda #>harbour_prize_line_4_suffix
        sta src_ptr+1
        jsr print_zero_terminated
        jsr print_newline
        lda #<harbour_prize_line_5
        sta src_ptr
        lda #>harbour_prize_line_5
        sta src_ptr+1
        jsr print_zero_terminated
        jsr print_double_newline

        // BASIC 1504: IPOINT = IPOINT + INT(IRGSDPRI/10).
        lda pending_prize_rigsdaler_lo
        sta div_value_lo
        lda pending_prize_rigsdaler_hi
        sta div_value_hi
        jsr divide16_by10
        clc
        lda player_points
        adc div_value_lo
        sta player_points
        lda player_points+1
        adc div_value_hi
        sta player_points+1

        clc
        lda crew_men
        adc pending_prize_crew_lo
        sta crew_men
        lda crew_men+1
        adc pending_prize_crew_hi
        sta crew_men+1

        clc
        lda treasury_rigsdaler
        adc pending_prize_rigsdaler_lo
        sta treasury_rigsdaler
        lda treasury_rigsdaler+1
        adc pending_prize_rigsdaler_hi
        sta treasury_rigsdaler+1

        lda #0
        sta pending_prize_crew_lo
        sta pending_prize_crew_hi
        sta pending_prize_rigsdaler_lo
        sta pending_prize_rigsdaler_hi

        jsr show_press_any_key
harbour_check_copenhagen_prize_done:
        rts

harbour_generate_prices:
        lda #0
        sta harbour_price_gen_index
harbour_generate_prices_loop:
        ldx harbour_price_gen_index
        lda harbour_price_base_table,x
        jsr compute_harbour_price
        ldx harbour_price_gen_index
        lda harbour_price_out_lo,x
        sta len_lo
        lda harbour_price_out_hi,x
        sta len_hi
        ldy #0
        lda price_result
        sta (len_lo),y
        inc harbour_price_gen_index
        lda harbour_price_gen_index
        cmp #5
        bne harbour_generate_prices_loop
        rts

// BASIC line 1500: PRIS(I) = PRISG(I) * (.8 + RND). Approximated with shifts as
// base - base/8 + (base * random_mod_16)/16, giving roughly the same 0.8x-1.8x range.
// Input: A = base price. Output: price_result.
compute_harbour_price:
        sta price_base
        lsr
        lsr
        lsr
        sta price_low_adjust
        lda price_base
        sec
        sbc price_low_adjust
        sta price_result

        jsr random_mod_16
        sta price_rnd
        lda #0
        sta price_prod_lo
        sta price_prod_hi
        lda price_base
        sta price_add_lo
        lda #0
        sta price_add_hi
        ldx #4
compute_harbour_price_mul_loop:
        lsr price_rnd
        bcc compute_harbour_price_mul_skip
        clc
        lda price_prod_lo
        adc price_add_lo
        sta price_prod_lo
        lda price_prod_hi
        adc price_add_hi
        sta price_prod_hi
compute_harbour_price_mul_skip:
        asl price_add_lo
        rol price_add_hi
        dex
        bne compute_harbour_price_mul_loop

        ldx #4
compute_harbour_price_shift_loop:
        lsr price_prod_hi
        ror price_prod_lo
        dex
        bne compute_harbour_price_shift_loop

        lda price_result
        clc
        adc price_prod_lo
        sta price_result
        rts

// 16-bit (harbour_qty) x 8-bit (A) multiply, clamped to $ffff on overflow.
// Output: harbour_cost_lo/hi.
multiply_qty_price:
        sta mul_price_byte
        lda #0
        sta harbour_cost_lo
        sta harbour_cost_hi
        sta mul_overflow
        lda harbour_qty_lo
        sta mul_a_lo
        lda harbour_qty_hi
        sta mul_a_hi
        ldx #8
multiply_qty_price_loop:
        lsr mul_price_byte
        bcc multiply_qty_price_skip
        clc
        lda harbour_cost_lo
        adc mul_a_lo
        sta harbour_cost_lo
        lda harbour_cost_hi
        adc mul_a_hi
        sta harbour_cost_hi
        lda mul_overflow
        adc #0
        sta mul_overflow
multiply_qty_price_skip:
        asl mul_a_lo
        rol mul_a_hi
        rol mul_overflow
        dex
        bne multiply_qty_price_loop
        lda mul_overflow
        beq multiply_qty_price_done
        lda #$ff
        sta harbour_cost_lo
        sta harbour_cost_hi
multiply_qty_price_done:
        rts

// Divides div_value_lo/hi (16-bit) by 10 in place. Remainder in div_remainder.
divide16_by10:
        lda #0
        sta div_remainder
        ldx #16
divide16_by10_loop:
        asl div_value_lo
        rol div_value_hi
        rol div_remainder
        lda div_remainder
        sec
        sbc #10
        bcc divide16_by10_skip
        sta div_remainder
        inc div_value_lo
divide16_by10_skip:
        dex
        bne divide16_by10_loop
        rts

// Reads up to 3 digits terminated by RETURN (DEL supported) into harbour_qty_lo/hi.
read_qty_input:
        lda #0
        sta harbour_qty_digits
        jsr recompute_qty
read_qty_key:
        jsr $ffe4
        beq read_qty_key
        cmp #13
        beq read_qty_done
        cmp #20
        beq read_qty_backspace
        cmp #'0'
        bcc read_qty_key
        cmp #'9'+1
        bcs read_qty_key
        ldx harbour_qty_digits
        cpx #3
        bcs read_qty_key
        pha
        sec
        sbc #'0'
        sta harbour_qty_buf,x
        inc harbour_qty_digits
        jsr recompute_qty
        pla
        jsr $ffd2
        jmp read_qty_key
read_qty_backspace:
        lda harbour_qty_digits
        beq read_qty_key
        dec harbour_qty_digits
        jsr recompute_qty
        lda #20
        jsr $ffd2
        jmp read_qty_key
read_qty_done:
        rts

recompute_qty:
        lda #0
        sta harbour_qty_lo
        sta harbour_qty_hi
        ldy #0
        ldx harbour_qty_digits
        beq recompute_qty_done
recompute_qty_loop:
        jsr multiply_qty_by_10
        lda harbour_qty_buf,y
        clc
        adc harbour_qty_lo
        sta harbour_qty_lo
        lda harbour_qty_hi
        adc #0
        sta harbour_qty_hi
        iny
        dex
        bne recompute_qty_loop
recompute_qty_done:
        rts

// harbour_qty (word) = harbour_qty * 10, via (x2)+(x8).
multiply_qty_by_10:
        lda harbour_qty_lo
        asl
        sta price_add_lo    // price_add = qty*2
        lda harbour_qty_hi
        rol
        sta price_add_hi
        lda price_add_lo
        sta price_prod_lo   // save qty*2 before shifting further to qty*8
        lda price_add_hi
        sta price_prod_hi
        asl price_add_lo
        rol price_add_hi
        asl price_add_lo
        rol price_add_hi
        clc
        lda price_add_lo
        adc price_prod_lo
        sta harbour_qty_lo
        lda price_add_hi
        adc price_prod_hi
        sta harbour_qty_hi
        rts

harbour_print_menu:
        lda #0
        sta harbour_menu_print_index
harbour_print_menu_loop:
        ldx harbour_menu_print_index
        lda harbour_menu_prefix_lo,x
        sta src_ptr
        lda harbour_menu_prefix_hi,x
        sta src_ptr+1
        jsr print_zero_terminated

        lda harbour_menu_price_lo,x
        sta len_lo
        lda harbour_menu_price_hi,x
        sta len_hi
        ldy #0
        lda (len_lo),y
        sta num_print_lo
        lda #0
        sta num_print_hi
        jsr print_inline_word_decimal

        ldx harbour_menu_print_index
        lda harbour_menu_suffix_lo,x
        sta src_ptr
        lda harbour_menu_suffix_hi,x
        sta src_ptr+1
        jsr print_zero_terminated
        jsr print_newline

        inc harbour_menu_print_index
        lda harbour_menu_print_index
        cmp #4
        bne harbour_print_menu_loop

        lda #<harbour_menu_line5
        sta src_ptr
        lda #>harbour_menu_line5
        sta src_ptr+1
        jsr print_zero_terminated
        jsr print_newline

        lda #<harbour_menu_line6
        sta src_ptr
        lda #>harbour_menu_line6
        sta src_ptr+1
        jsr print_zero_terminated
        jsr print_newline
        rts

// ---------------------------------------------------------------
// Text.

harbour_welcome_prefix_msg:
                        .text "VELKOMMEN TIL "
                        .byte 0

harbour_prize_line_1:  .text "HURRA!"
                        .byte 0
harbour_prize_line_2_prefix:
                        .text "HER I K\BENHAVN VENTER DINE"
                        .byte 0
harbour_prize_line_3:   .text "TAPRE MAND P] DIG."
                        .byte 0
harbour_prize_line_4_prefix:
                        .text "DE HAR"
                        .byte 0
harbour_prize_line_4_suffix:
                        .text "RIGSDALER"
                        .byte 0
harbour_prize_line_5:   .text "I PRISEPENGE TIL DIG!"
                        .byte 0

harbour_menu_line1_prefix: .text "1: HYRE FLERE M[ND ("
                            .byte 0
harbour_menu_line1_suffix: .text "RGSD PR. MAND)"
                            .byte 0
harbour_menu_line2_prefix: .text "2: REPARERE SKIBET ("
                            .byte 0
harbour_menu_line2_suffix: .text "RGSD PR. POINT)"
                            .byte 0
harbour_menu_line3_prefix: .text "3: K\BE KANONER ("
                            .byte 0
harbour_menu_line3_suffix: .text "RGSD PR STK)"
                            .byte 0
harbour_menu_line4_prefix: .text "4: K\BE KORN ("
                            .byte 0
harbour_menu_line4_suffix: .text "RGSD PR S[K)"
                            .byte 0
harbour_menu_line5:         .text "5: S[LGE NOGET"
                            .byte 0
harbour_menu_line6:         .text "6: ST] TIL S\S IGEN."
                            .byte 0

harbour_prompt_qty_msg:    .text "HVOR MANGE?"
                            .byte 0
harbour_sell_prompt_msg:   .text "KORN (K), SKYTS (S) ELLER JUVELER (J)?"
                            .byte 0
harbour_cant_afford_msg:   .text "DET HAR DU IKKE R]D TIL!"
                            .byte 0
harbour_too_many_msg:      .text "DET ER FOR MANGE!"
                            .byte 0
harbour_dont_have_msg:     .text "S] MANGE HAR DU IKKE!"
                            .byte 0
