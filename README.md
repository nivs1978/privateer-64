# privateer-64
A Commodore 64 port of the Privateer game (Kaptajn Kaper i Kattegat)

## Build Source Of Truth
Assembler builds use `kaper.asm` plus included `.inc` assets directly.

## Modules
- `constants.inc` contains different memory offsets and other constants used by the code, this is to keep these in a shared place for easy editing
- `intro_screen.inc` contains the PESCII intro image.
not part of the build pipeline.
- `map.inc` contains the code that handles the ship movement on the map, and the random events (but not the mini games)
- `map_art_bytes.inc` contains the raw high res graphics for the map.
