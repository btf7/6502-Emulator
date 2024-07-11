#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include "main.h"

uint8_t mem[0x10000];
uint16_t PC;
uint8_t SP = 0xff; // Grows down
uint8_t AC = 0;
uint8_t X = 0;
uint8_t Y = 0;
uint8_t SR = FLAG_IGNORE;

void readFile(const char * const fileName) {
    FILE * const file = fopen(fileName, "rb");
    if (!file) {
        printf("Failed to open file: %s\n", fileName);
        exit(1);
    }

    fseek(file, 0, SEEK_END); // Seek to end of file
    const long fileSize = ftell(file); // Get file length
    fseek(file, 0, SEEK_SET); // Seek back to beginning of file

    if (fileSize != 0x10000) {
        printf("Expected input file to be 65536 bytes long, got %ld\n", fileSize);
        exit(1);
    }

    for (uint32_t i = 0; i < 0x10000; i++) {
        const int c = getc(file);
        if (c == EOF) {
            printf("Failed to read character %d\n", i);
            exit(1);
        }
        mem[i] = c;
    }

    fclose(file);
}

void pushStack(const uint8_t val) {
    mem[0x100 + SP] = val;
    SP--;
}

uint16_t readWord(const uint16_t pointer) {
    const uint16_t hi = mem[pointer + 1] << 8;
    const uint8_t lo = mem[pointer];
    return hi + lo;
}

int main(int argc, char** argv) {
    if (argc != 2) {
        printf("Expected 1 input parameter, got %d\n", argc - 1);
        exit(1);
    }

    // Initialise
    readFile(argv[1]);
    PC = readWord(0xfffc);

    // Main loop
    while (true) {
        // Console output
        if (mem[0xfffa]) {
            putc(mem[0xfffa], stdout);
            mem[0xfffa] = 0;
        }

        switch (mem[PC]) {
            case 0x00: // BRK
            PC += 2;
            pushStack(PC >> 8);
            pushStack(PC & 0xff);
            pushStack(SR);
            PC = readWord(0xfffe);
            break;

            case 0x8d: // STA absolute
            mem[readWord(PC + 1)] = AC;
            PC += 3;
            break;

            case 0xa9: // LDA #
            AC = mem[PC + 1];
            PC += 2;
            break;

            default:
            printf("Invalid opcode: 0x%x\n", mem[PC]);
            exit(1);
        }
    }

    return 0;
}