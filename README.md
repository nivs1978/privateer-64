# Kaper 64
A Commodore 64 port of the Privateer game (Kaptajn Kaper i Kattegat)

## Build Source Of Truth
Assembler builds use `kaper.asm` and the included `.inc` files directly.

The project uses Kick Assembler. From PowerShell, build the program with:

```powershell
java -jar C:\apps\KickAssembler\KickAss.jar .\kaper.asm -o .\bin\kaper.prg
```

To create the disk image and run the game in VICE, use:

```powershell
.\run-vice-with-disk.cmd
```

## Modules
- `kaper.asm` is the main entry point and imports the program modules.
- `constants.inc` contains shared memory addresses, VIC configuration, and gameplay constants.
- `intro.inc` contains the intro screen, text-screen setup, VIC bank switching, and shared display helpers.
- `intro.dat` is the hires intro bitmap, written to the disk image and loaded at runtime
  (load address `$5c00`: 1000 bytes of screen colour data, 24 pad bytes, then the 8000-byte bitmap at `$6000`).
  It is converted from `intro.art` (OCP Art Studio hires) in the `Kaper64 BASIC\intro` folder.
- `shooting.inc` contains the cannon-battle minigame and shooting screen data.
- `harbour_sailing.inc` contains the harbour-approach minigame.
- `harbour.inc` contains harbour trading and resource management.
- `map.inc` contains map movement, random events, end-of-battle flags, and the map screen renderer.
- `map_art_bytes.inc` contains the raw high-resolution bitmap data for the map.
- `KAPER.BAS` is the original BASIC source used as a behavioral reference.

## Memory Layout
- The intro uses a 320x200 hires bitmap loaded from `intro.dat` on disk (screen `$5c00`, bitmap `$6000`).
- The map uses a high-resolution bitmap with screen/color data in VIC bank 1.
- Intro and event screens are prepared while the VIC display is hidden to avoid
	showing partially drawn screen contents during transitions.
