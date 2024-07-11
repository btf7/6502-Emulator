CC = gcc
CFLAGS = -std=c17 -Wall -O0 -g3
CFILES = src/main.c
HFILES = src/main.h

Emulator: $(CFILES) $(HFILES)
	$(CC) $(CFILES) -o Emulator $(CFLAGS)