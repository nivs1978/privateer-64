        BasicUpstart2(start)
        .encoding "petscii_upper"
                .import source "constants.inc"

        * = $0810
start:
        // Intro installs a RAM charset at $1000 after screen/color data has been copied.

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

        jsr run_intro
        jmp done

done:
        // ask for name - MAIN.BAS lines 2010-2160
        lda #$93
        jsr $ffd2
        lda #$05        // white text
        jsr $ffd2
        jsr print_double_newline

        pstr(askstr)
        jsr print_double_newline

        // Build-time diagnostic: bytes still free before code would reach into
        // $4400 (VIC bank 1 screen RAM), so an overlap regression is easy to spot.
        pstr(freememstr)
        lda #<($4400 - code_segment_end)
        sta num_print_lo
        lda #>($4400 - code_segment_end)
        sta num_print_hi
        jsr print_inline_word_decimal
        jsr print_double_newline

        // CHROUT clear can overwrite pointer bytes in the active screen's pointer table.
        jsr set_sprite_pointers_intro

        jsr position_ship
        jsr show_ship

        pstr(hvadstr)

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
        jsr load_ship_sprites
        jsr init_map_navigation
        jsr show_ship
        jsr game_loop
        rts

load_ship_sprites:
        jsr set_sprite_pointers_map
        jsr set_sprite_pointers_intro

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

        // Force a known sprite state: only sprites 0-2 are used by the ship.
        lda #$00
        sta $d010
        sta $d01c
        sta $d017
        sta $d01d
        sta $d01b

        ldy #0
shipcopy:
        lda ship_sprites,y
        sta SPRITE_DATA_BANK1,y
        iny
        cpy #192
        bne shipcopy
        rts

set_sprite_pointers_map:
        lda #SPRITE_PTR_BANK1_BASE
        sta SPRITE_PTR_TAB_BANK1_MAP
        lda #SPRITE_PTR_BANK1_BASE+1
        sta SPRITE_PTR_TAB_BANK1_MAP+1
        lda #SPRITE_PTR_BANK1_BASE+2
        sta SPRITE_PTR_TAB_BANK1_MAP+2
        rts

set_sprite_pointers_intro:
        lda #SPRITE_PTR_BANK1_BASE
        sta SPRITE_PTR_TAB_BANK1_INTRO
        lda #SPRITE_PTR_BANK1_BASE+1
        sta SPRITE_PTR_TAB_BANK1_INTRO+1
        lda #SPRITE_PTR_BANK1_BASE+2
        sta SPRITE_PTR_TAB_BANK1_INTRO+2
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
        lda #$07
        sta $d015
        rts

hide_ship:
        lda #$00
        sta $d015
        rts

askstr:  .byte 17
         .text "S]DAN SER DIT SKIB UD:"
         .byte 0
freememstr:.text "FRI HUKOMMELSE F\R KODE:"
         .byte 0
hvadstr: .text "HVAD ER DIT NAVN? "
         .byte 0
kaperstr:.text "KAPER"
         .byte 0
namebuf: .byte 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0
ship_x:  .byte 0
ship_x_hi:.byte 12
ship_y:  .byte 72

// Global player state (shared across include files).
crew_men:           .word 200
ship_repair_points: .word 200
treasury_rigsdaler: .word 600
grain_sacks:        .word 30
ship_cannons:       .word 20
jewel_count:        .word 0
player_points:      .word 0
// voyage_turn_counter must stay a full word: it is read/incremented as one
// everywhere (game_loop, victory checks, status displays). A stray .byte
// declaration here let its high byte alias whatever variable followed it,
// so writes to that variable (e.g. a prize payout) looked like a huge turn count.
voyage_turn_counter:.word 0
// Prize crew/rigsdaler in transit to K\benhavn (BASIC MANDPRISE/IRGSDPRI), paid out on arrival there.
pending_prize_crew_lo:       .byte 0
pending_prize_crew_hi:       .byte 0
pending_prize_rigsdaler_lo:  .byte 0
pending_prize_rigsdaler_hi:  .byte 0
// BASIC line 102: IPOINTLIM = IPOINT!+500+250*IDIF, ITURLIM = ITUR+325-12.5*IDIF (IDIF=2 at start)
point_limit:        .word 1000
turn_limit:         .word 300

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

        .import source "intro.inc"
        .import source "shooting.inc"
        .import source "harbour_sailing.inc"
        // harbour.inc must be imported before map.inc: map.inc ends by relocating
        // the program counter to $9000 (the embedded MapArt bitmap, inside the
        // BASIC ROM window), and anything imported after it would be linked into
        // that ROM-shadowed range and fail to execute at runtime.
        .import source "harbour.inc"
        .import source "map.inc"

        // Imported last: it relocates the PC to the free $8000-$8fff gap, so it
        // must not be followed by anything that belongs in the code segment.
        .import source "intro_screen.inc"

