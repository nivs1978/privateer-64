.label src_ptr = $fb
.label dst_ptr = $fd

draw_map:
        jsr hide_ship

        // Copy bitmap payload to VIC bitmap area ($6000-$7fff).
        lda #<map_bitmap_data
        sta src_ptr
        lda #>map_bitmap_data
        sta src_ptr+1
        lda #<$6000
        sta dst_ptr
        lda #>$6000
        sta dst_ptr+1
        ldx #$20
        jsr copy_full_pages

        // Copy screen RAM using the original MAP.PRG block layout.
        // This preserves the source overlap pattern that produces the final 1000 bytes at $4000-$43e7.
        ldx #0
copy_screen_pages:
        lda map_screen_block0,x
        sta $4000,x
        lda map_screen_block1,x
        sta $4100,x
        lda map_screen_block2,x
        sta $4200,x
        lda map_screen_block3,x
        sta $42e8,x
        dex
        bne copy_screen_pages

        lda map_border_color
        sta $d020

        lda $d011
        and #$df
        ora #$20
        sta $d011

        lda $d016
        and #$ef
        sta $d016

        lda $dd00
        and #$fc
        ora #$02
        sta $dd00

        lda #$08
        sta $d018

        lda #100
        sta ship_x
        sta ship_y
        lda #0
        sta ship_x_hi

        lda #$f0
        sta $43f8
        lda #$f1
        sta $43f9
        lda #$f2
        sta $43fa

        ldy #0
copy_map_ship_sprites:
        lda ship_sprites,y
        sta $7c00,y
        iny
        cpy #192
        bne copy_map_ship_sprites

        jsr position_ship
        jsr show_ship
        rts

copy_full_pages:
copy_page_loop:
        ldy #0
copy_byte_loop:
        lda (src_ptr),y
        sta (dst_ptr),y
        iny
        bne copy_byte_loop

        inc src_ptr+1
        inc dst_ptr+1
        dex
        bne copy_page_loop
        rts

map_prg_data:
        .import source "map_prg_bytes.inc"

.label map_border_color = map_prg_data + $0135
.label map_bitmap_data  = map_prg_data + $0138
.label map_screen_block0 = map_prg_data + $2078
.label map_screen_block1 = map_prg_data + $2178
.label map_screen_block2 = map_prg_data + $2278
.label map_screen_block3 = map_prg_data + $2360
