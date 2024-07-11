# 6502 Emulator

## Usage

Run `.\Emulator inputfilename`.

You must pass a 64KiB file as input -
this file will be loaded into processor memory before the processor is started.

## Memory Layout

There is 64KiB of memory, broken up as shown:

0x0000 - 0x00FF - Zero page\
0x0100 - 0x01FF - Stack\
0x0200 - 0xDFFF - Free\
0xE000 - 0xEFFF - Video output\
0xF000 - 0xFFF8 - Free\
0xFFF9 - Keyboard input\
0xFFFA - Console output\
0xFFFB - Delay output (milliseconds)\
0xFFFC - 0xFFFD - Start location\
0xFFFE - 0xFFFF - BRK location

## Video Output

**NOTE** - This is not yet added

The video output is a 64 x 64 display being rendered at 640 x 640.

The screen is rendered in horizontal rows starting from the top-left at 0xE000.

Each pixel is 1 byte, storing colour information as RRRGGGBB.

## Delay Output

**NOTE** - This is not yet added

The emulator will run as fast as it can, unless 0xFFFB is written to.

When any value above 0 is written here, the value is treated as a uint_8t and the emulator will wait that number of milliseconds, before clearing 0xFFFB back to 0.
This is also when the emulator will poll the OS for any new keyboard input.

## Keyboard Input

**NOTE** - This is not yet added

When 0xFFFB is written to and a delay is added, the emulator will poll the OS for any new keyboard input.
The most recent key pressed will then be written to 0xFFF9.

## Console Output

When 0xFFFA is written to with any value above 0, it is treated as a char and added to stdout,
before clearing 0xFFFA back to 0.

## Useful Sources

https://www.masswerk.at/6502/6502_instruction_set.html

https://www.nesdev.org/obelisk-6502-guide/reference.html

https://www.nesdev.org/wiki/Status_flags