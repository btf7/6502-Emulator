#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <GLFW/glfw3.h>
#include "emulate.h"
#include "helper.h"

uint8_t mem[0x10000];
uint16_t PC;
uint8_t SP = 0xff; // Grows down
uint8_t AC = 0;
uint8_t X = 0;
uint8_t Y = 0;

bool negativeFlag = false;
bool overflowFlag = false;
bool decimalFlag = false;
bool interruptFlag = false;
bool zeroFlag = false;
bool carryFlag = false;

bool drawQueued = false;

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

void runInstruction(void) {
    int16_t tmp;

    switch (mem[PC]) {
        case 0x00: // BRK
        PC += 2;
        pushStack(PC >> 8);
        pushStack(PC & 0xff);
        pushStack(getStatus());
        PC = readWord(0xfffe);
        break;

        case 0x18: // CLC
        carryFlag = false;
        PC++;
        break;

        case 0x29: // AND #
        AC &= mem[PC + 1];
        zeroFlag = AC == 0;
        negativeFlag = AC & 0x80;
        PC += 2;
        break;

        case 0x4c: // JMP abs
        PC = readWord(PC + 1);
        break;

        case 0x69: // ADC #
        tmp = (int16_t)AC + (int16_t)mem[PC + 1] + carryFlag;
        AC = tmp;
        carryFlag = tmp > 0xff;
        zeroFlag = AC == 0;
        overflowFlag = tmp > 127 || tmp < -128;
        negativeFlag = AC & 0x80;
        PC += 2;
        break;

        case 0x85: // STA zp
        writeByte(mem[PC + 1], AC);
        PC += 2;
        break;

        case 0x86: // STX zp
        writeByte(mem[PC + 1], X);
        PC += 2;
        break;

        case 0x8d: // STA abs
        writeByte(readWord(PC + 1), AC);
        PC += 3;
        break;

        case 0x91: // STA (),y
        writeByte(readWord(mem[PC + 1]) + Y, AC);
        PC += 2;
        break;

        case 0x9d: // STA abs,x
        writeByte(readWord(PC + 1) + X, AC);
        PC += 3;
        break;

        case 0xa0: // LDY #
        Y = mem[PC + 1];
        zeroFlag = Y == 0;
        negativeFlag = Y & 0x80;
        PC += 2;
        break;

        case 0xa2: // LDX #
        X = mem[PC + 1];
        zeroFlag = X == 0;
        negativeFlag = X & 0x80;
        PC += 2;
        break;

        case 0xa5: // LDA zp
        AC = mem[mem[PC + 1]];
        zeroFlag = AC == 0;
        negativeFlag = AC & 0x80;
        PC += 2;
        break;

        case 0xa6: // LDX zp
        X = mem[mem[PC + 1]];
        zeroFlag = X == 0;
        negativeFlag = X & 0x80;
        PC += 2;
        break;

        case 0xa9: // LDA #
        AC = mem[PC + 1];
        zeroFlag = AC == 0;
        negativeFlag = AC & 0x80;
        PC += 2;
        break;

        case 0xc0: // CPY #
        carryFlag = Y >= mem[PC + 1];
        zeroFlag = Y == mem[PC + 1];
        negativeFlag = (Y - mem[PC + 1]) & 0x80;
        PC += 2;
        break;

        case 0xc8: // INY
        Y++;
        zeroFlag = Y == 0;
        negativeFlag = Y & 0x80;
        PC++;
        break;

        case 0xc9: // CMP #
        carryFlag = AC >= mem[PC + 1];
        zeroFlag = AC == mem[PC + 1];
        negativeFlag = (AC - mem[PC + 1]) & 0x80;
        PC += 2;
        break;

        case 0xd0: // BNE
        if (!zeroFlag) {
            PC += (int8_t)mem[PC + 1];
        }
        PC += 2;
        break;

        case 0xe0: // CPX #
        carryFlag = X >= mem[PC + 1];
        zeroFlag = X == mem[PC + 1];
        negativeFlag = (X - mem[PC + 1]) & 0x80;
        PC += 2;
        break;

        case 0xe6: // INC zp
        writeByte(mem[PC + 1], mem[mem[PC + 1]] + 1);
        zeroFlag = mem[mem[PC + 1]] == 0;
        negativeFlag = mem[mem[PC + 1]] & 0x80;
        PC += 2;
        break;

        case 0xe8: // INX
        X++;
        zeroFlag = X == 0;
        negativeFlag = X & 0x80;
        PC++;
        break;

        default:
        printf("Invalid opcode at 0x%.4x: 0x%.2x\n", PC, mem[PC]);
        glfwTerminate();
        exit(1);
    }
}