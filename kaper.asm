        BasicUpstart2(start)
        .encoding "petscii_upper"

        * = $0810
start:
        // copy ROM charset ($d000-$d7ff) to RAM at $3800
        sei
        lda $01
        pha
        and #$fb        // CHAREN=0: exposes char ROM at $d000
        sta $01
        ldx #0
copyloop:
        lda $d000,x
        sta $3800,x
        lda $d100,x
        sta $3900,x
        lda $d200,x
        sta $3a00,x
        lda $d300,x
        sta $3b00,x
        lda $d400,x
        sta $3c00,x
        lda $d500,x
        sta $3d00,x
        lda $d600,x
        sta $3e00,x
        lda $d700,x
        sta $3f00,x
        inx
        bne copyloop
        pla
        sta $01
        cli

        // point VIC charset to $3800: bits 3-1 = 111
        lda $d018
        and #$f0
        ora #$0e
        sta $d018

        // patch 5 custom chars (MAIN.BAS lines 152-165)
        ldx #7
pae:    lda char_ae,x
        sta $38d8,x     // slot 27  = petscii 91  ([ = ae)
        dex
        bpl pae

        ldx #7
poe:    lda char_oe,x
        sta $38e0,x     // slot 28  = petscii 92  (\ = oe)
        dex
        bpl poe

        ldx #7
paa:    lda char_aa,x
        sta $38e8,x     // slot 29  = petscii 93  (] = aa)
        dex
        bpl paa

        ldx #7
pan:    lda char_ship,x
        sta $38f0,x     // slot 30  = petscii 94  (^ = ship)
        dex
        bpl pan

        jsr load_ship_sprites

        lda $dd00
        and #$fc
        ora #$03
        sta $dd00

        lda $d011
        and #$9f        // clear ECM+BMM; preserve DEN
        sta $d011

        lda $d016
        and #$ef
        sta $d016

        // clear screen then print each custom char on its own line
        lda #$93
        jsr $ffd2

        ldx #0
prloop: lda teststr,x
        beq done
        jsr $ffd2
        inx
        bne prloop
done:
        // ask for name - MAIN.BAS lines 2010-2160
        lda #$93
        jsr $ffd2
        lda #$05        // white text
        jsr $ffd2
        lda #13
        jsr $ffd2
        lda #13
        jsr $ffd2
        
        ldx #0
prtask: lda askstr,x
        beq prtask_done
        jsr $ffd2
        inx
        bne prtask
prtask_done:
        lda #13
        jsr $ffd2
        lda #13
        jsr $ffd2

        jsr position_ship
        jsr show_ship

        ldx #0
prthvad:lda hvadstr,x
        beq prthvad_done
        jsr $ffd2
        inx
        bne prthvad
prthvad_done:

        lda #0
        sta $cc         // enable cursor blink
        ldx #0
inloop: jsr $ffe4
        beq inloop
        cmp #13
        beq indone
        cmp #20         // DEL
        beq indel
        cpx #15
        bcs inloop
        sta namebuf,x
        inx
        jsr $ffd2
        jmp inloop
indel:  cpx #0
        beq inloop
        dex
        lda #20
        jsr $ffd2
        jmp inloop
indone: lda #0
        sta namebuf,x
        lda #$ff
        sta $cc         // disable cursor blink
        cpx #0
        bne nameok
        ldx #0          // default to "KAPER"
defloop:lda kaperstr,x
        sta namebuf,x
        beq nameok
        inx
        bne defloop
nameok: lda #13
        jsr $ffd2
        jsr draw_map
        rts

load_ship_sprites:
        lda #220
        sta $07f8
        lda #221
        sta $07f9
        lda #222
        sta $07fa

        lda #$0a
        sta $d025
        lda #$0f
        sta $d026

        lda #$02
        sta $d027
        lda #$0b
        sta $d028
        lda #$05
        sta $d029

        lda $d01c
        and #$f8
        sta $d01c
        lda $d017
        and #$f8
        sta $d017
        lda $d01d
        and #$f8
        sta $d01d

        ldy #0
shipcopy:
        lda ship_sprites,y
        sta $3700,y
        iny
        cpy #192
        bne shipcopy
        rts

position_ship:
        lda $d010
        and #$f8
        sta $d010
        lda ship_x_hi
        beq ship_x_low
        lda $d010
        ora #7
        sta $d010
ship_x_low:
        lda ship_x
        sta $d000
        sta $d002
        sta $d004
        lda ship_y
        sta $d001
        sta $d003
        sta $d005
        rts

show_ship:
        lda $d015
        ora #7
        sta $d015
        rts

hide_ship:
        lda $d015
        and #$f8
        sta $d015
        rts

        // petscii values of the 4 patched chars, each followed by cr
teststr:
        .byte 91, 13            // [ = Æ
        .byte 92, 13            // \ = oe
        .byte 93, 13            // ] = aa
        .byte 94, 13            // ^ = ship
        .byte 0

askstr:  .byte 17
         .text "S]DAN SER DIT SKIB UD:"
         .byte 0
hvadstr: .text "HVAD ER DIT NAVN? "
         .byte 0
kaperstr:.text "KAPER"
         .byte 0
namebuf: .byte 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0
ship_x:  .byte 240
ship_x_hi:.byte 0
ship_y:  .byte 72

        // glyph data from MAIN.BAS lines 122-128
char_ae:     .byte 63,108,108,127,108,108,111,0
char_oe:     .byte 60,102,110,126,118,102,60,0
char_aa:     .byte 24,36,24,60,102,126,102,0
char_ship:   .byte 36,36,36,36,36,255,127,62

        // 3 sprites generated with spritemate on 01/08/2026, 12:48:00
        // Byte 64 of each sprite contains multicolor (high nibble) & color (low nibble) information
ship_sprites:
        // Red sails
        .byte $00,$00,$00,$2E,$00,$00,$6E,$00
        .byte $00,$6E,$00,$00,$E0,$00,$00,$00
        .byte $00,$00,$00,$00,$00,$00,$00,$00
        .byte $00,$00,$00,$00,$00,$00,$00,$00
        .byte $00,$00,$00,$00,$00,$00,$00,$00
        .byte $00,$00,$00,$00,$00,$00,$00,$00
        .byte $00,$00,$00,$00,$00,$00,$00,$00
        .byte $00,$00,$00,$00,$00,$00,$00,$0A
        // Grey mast
        .byte $10,$00,$00,$10,$00,$00,$10,$00
        .byte $00,$10,$00,$00,$10,$00,$00,$10
        .byte $00,$00,$00,$00,$00,$00,$00,$00
        .byte $00,$00,$00,$00,$00,$00,$00,$00
        .byte $00,$00,$00,$00,$00,$00,$00,$00
        .byte $00,$00,$00,$00,$00,$00,$00,$00
        .byte $00,$00,$00,$00,$00,$00,$00,$00
        .byte $00,$00,$00,$00,$00,$00,$00,$0F
        // Orange hull
        .byte $00,$00,$00,$00,$00,$00,$00,$00
        .byte $00,$00,$00,$00,$03,$80,$00,$EF
        .byte $80,$00,$7F,$80,$00,$7F,$00,$00
        .byte $00,$00,$00,$00,$00,$00,$00,$00
        .byte $00,$00,$00,$00,$00,$00,$00,$00
        .byte $00,$00,$00,$00,$00,$00,$00,$00
        .byte $00,$00,$00,$00,$00,$00,$00,$00
        .byte $00,$00,$00,$00,$00,$00,$00,$03

        .import source "map.inc"

