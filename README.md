# 6502 Emulator

## Build

Run `make` to build in debug mode.\
Run `make release` to build in release mode\
Run `make clean` to delete build directories and executable

This project is set up to build on Windows 10 with MinGW-W64 - just run the makefile.

If you are building on a different architecture, you'll have to
manually update the makefile with new libraries.
You should also then delete libglfw3.a and libglew32.a from the dependencies folder.

## Usage

Run `.\emulator inputfilename`.

You must pass a 64KiB file as input -
this file will be loaded into processor memory before the processor is started.

This emulator supports all official 6502 instructions. If any opcode other than the official ones is read, the emulator will crash.\
Decimal mode is not yet supported.

If you move the window or grab the title bar, the emulator will freeze until you let go - this is a Windows feature I can't easily work around

## Memory Layout

There is 64KiB of memory, broken up as shown:

0x0000 - 0x00FF - Zero page\
0x0100 - 0x01FF - Stack\
0x0200 - 0xDFFF - Free\
0xE000 - 0xEFFF - Video output\
0xF000 - 0xFFF7 - Free\
0xFFF8 - 0xFFF9 - Keyboard input\
0xFFFA - Console output\
0xFFFB - Delay output (milliseconds)\
0xFFFC - 0xFFFD - Start location\
0xFFFE - 0xFFFF - BRK location

## Video Output

The video output is a 64 x 64 display being rendered at 640 x 640.

The screen is rendered in horizontal rows starting from the top-left at 0xE000.

Each pixel is 1 byte, storing colour information as RRRGGGBB.

Whenever any address from 0xE000 - 0xEFFF is written to or modified, the display will immediately (<= 1ms) reflect the change.

## Delay Output

The emulator will run as fast as it can, unless 0xFFFB is written to.

Whenever 0xFFFB is written to or modified, the new value is read as a uint8_t and the emulator will sleep for that number of milliseconds.

## Keyboard Input

The most recent key pressed will be written as 2 bytes to 0xFFF8 at intervals of ~1ms.

0xFFF8 will receive the low byte of the key, and 0xFFF9 will receive the high byte. The high byte is generally either 0 or 1 -
0 for normal keys, 1 for special keys (SHIFT, ESC, PG UP, numpad keys etc.).
Note that some keys have identical low bytes.
For example, 'U' = 0x55 and CTRL = 0x155.

## Console Output

Whenever 0xFFFA is written to or modified, the new value is also written to stdout.

## Useful Sources

https://www.masswerk.at/6502/6502_instruction_set.html

https://www.nesdev.org/obelisk-6502-guide/reference.html