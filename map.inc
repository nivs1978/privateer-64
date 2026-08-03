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

map_art_data:
        .import source "map_art_bytes.inc"

// map.art layout: 2-byte load address + 8000 bitmap bytes + 1000 screen bytes + trailing metadata
.label map_bitmap_data  = map_art_data + $0002
.label map_screen_data  = map_art_data + $1f42
.label map_border_color = map_art_data + $232a
