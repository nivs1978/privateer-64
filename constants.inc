// Centralized memory layout/constants for VIC/screen/charset/sprite setup.

.const VIC_BANK0_SELECT = $03
.const VIC_BANK1_SELECT = $02

.const VIC_D018_MAIN = $15      // screen $0400, charset $1000 (bank 0)
.const VIC_D018_INTRO = $12     // screen $4400, charset $4800 (bank 1)

.const KERNAL_SCREEN_PAGE_ADDR = $0288
.const KERNAL_SCREEN_PAGE_MAIN = $04
.const KERNAL_SCREEN_PAGE_INTRO = $44

.const INTRO_SCREEN_RAM = $4400
// NOTE: charset must stay below $6000 - the map bitmap destination spans
// $6000-$7f3f and draw_map would overwrite anything placed inside it.
.const INTRO_CHARSET_RAM = $4800

.const SPRITE_PTR_TAB_BANK1_MAP = $47f8    // screen base $4400 + $03f8
.const SPRITE_PTR_TAB_BANK1_INTRO = $47f8  // screen base $4400 + $03f8

.const SPRITE_DATA_BANK1 = $5000

.const SPRITE_PTR_BANK1_BASE = 64

.const CHAR_SLOT_AE_ADDR = INTRO_CHARSET_RAM + $00d8
.const CHAR_SLOT_OE_ADDR = INTRO_CHARSET_RAM + $00e0
.const CHAR_SLOT_AA_ADDR = INTRO_CHARSET_RAM + $00e8
.const CHAR_SLOT_RIGHT_ARROW_ADDR = INTRO_CHARSET_RAM + $00f0
.const CHAR_SLOT_HARBOUR_SHIP_ADDR = INTRO_CHARSET_RAM + ($5e * 8)

// Shared zero-page pointers (declared here, first import, so every file that
// uses them for (zp),y indirect addressing gets true zero-page opcodes).
.label src_ptr = $fb
.label dst_ptr = $fd
.label len_lo = $f9
.label len_hi = $fa
.label bmp_char_ptr = $f7   // charset read pointer for draw_bmp_char (2 bytes: $f7=lo, $f8=hi)

.const MAP_GRID_COL_MIN = 1
.const MAP_GRID_COL_MAX = 29
.const MAP_GRID_ROW_MIN = 1
// Original BASIC clamps IY to 140 (rows 1..14 in 10-pixel steps).
.const MAP_GRID_ROW_MAX = 14
.const MAP_START_COL = 10
.const MAP_START_ROW = 10
.const MAP_GRID_STEP = 10
.const MAP_GRID_ORIGIN_X = 35
.const MAP_GRID_ORIGIN_Y = 59
.const MAP_START_PIXEL_X = 125
.const MAP_START_PIXEL_Y = 149
.const MAP_PIXEL_MIN_X = MAP_GRID_ORIGIN_X + ((MAP_GRID_COL_MIN - 1) * MAP_GRID_STEP)
.const MAP_PIXEL_MAX_X = MAP_GRID_ORIGIN_X + ((MAP_GRID_COL_MAX - 1) * MAP_GRID_STEP)
.const MAP_PIXEL_MIN_Y = MAP_GRID_ORIGIN_Y + ((MAP_GRID_ROW_MIN - 1) * MAP_GRID_STEP)
.const MAP_PIXEL_MAX_Y = MAP_GRID_ORIGIN_Y + ((MAP_GRID_ROW_MAX - 1) * MAP_GRID_STEP)

.const MAP_CELL_SEA = 0
.const MAP_CELL_LAND = 1
.const MAP_HARBOUR_COUNT = 7
