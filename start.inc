// Title screen shown before the bitmap intro.

show_start_screen:
        jsr configure_intro_vic_bank
        jsr prepare_intro_charset

        lda #$93
        jsr $ffd2
        lda #$05
        jsr $ffd2

        // Centered on a 40-column screen: (40 - length) / 2.
        ldx #8
        ldy #0
        jsr start_plot
        pstrnl(start_title)

        ldx #11
        ldy #1
        jsr start_plot
        pstrnl(start_author)

        ldx #2
        ldy #2
        jsr start_plot
        pstrnl(start_port_author)

        ldx #10
        ldy #4
        jsr start_plot
        pstrnl(start_version)

        ldx #0
        ldy #24
        jsr start_plot
        pstr(start_press_key)   // no newline: printing past row 24 would scroll the screen

        // configure_intro_vic_bank blanks the screen; turn it back on once drawn.
        jsr show_vic_screen

start_wait_release:
        jsr $ff9f
        jsr $ffe4
        bne start_wait_release
start_wait_key:
        jsr $ff9f
        jsr $ffe4
        beq start_wait_key
        rts

start_plot:
        jmp set_cursor_xy       // X=column, Y=row

start_title:
        .text "KAPTAJN KAPER I KATTEGAT"
        .byte 0
start_author:
        .text "AF P.O.FREDERIKSEN"
        .byte 0
start_port_author:
        .text "COMMODORE 64 VERSION AF HANS MILLING"
        .byte 0
start_version:
        .text "VERSION 1 RELEASE 4"
        .byte 0
start_press_key:
        .text "TRYK P] EN TAST FOR AT FORTS[TTE!"
        .byte 0
