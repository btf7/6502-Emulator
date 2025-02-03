#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <GLFW/glfw3.h>
#include <pthread.h>
#include "emulate.h"
#include "instructions.h"
#include "display.h"

uint8_t mem[0x10000];
uint16_t PC;
static uint16_t prevPC;
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

// Functions for getting the value's address in different addressing modes
// These should also increment PC by the number of bytes they read

static inline uint16_t readAdrImmediate(void) {
    return ++PC;
}

#define readAdrRel readAdrImmediate

static inline uint16_t readAdrZP(void) {
    return mem[++PC];
}

static inline uint16_t readAdrZPX(void) {
    return (mem[++PC] + X) & 0xff;
}

static inline uint16_t readAdrZPY(void) {
    return (mem[++PC] + Y) & 0xff;
}

static inline uint16_t readAdrAbs(void) {
    PC++; return readWord(PC++);
}

static inline uint16_t readAdrAbsX(void) {
    PC++; return readWord(PC++) + X;
}

static inline uint16_t readAdrAbsY(void) {
    PC++; return readWord(PC++) + Y;
}

static inline uint16_t readAdrInd(void) {
    // In the original 6502 chips, indirect addressing has a bug
    // where if the indirect vector is xxFF,
    // it will read the high byte from xx00 instead of (xx+1)00
    // This bug is fixed in some later chips, but we will emulate it here
    PC++;
    const uint16_t indirectVector = readWord(PC++);
    if ((indirectVector & 0xff) == 0xff) {
        const uint16_t hi = mem[indirectVector & 0xff00] << 8;
        const uint8_t lo = mem[indirectVector];
        return hi + lo;
    } else {
        return readWord(indirectVector);
    }
}

static inline uint16_t readAdrIndX(void) {
    return readWord(readAdrZPX());
}

static inline uint16_t readAdrIndY(void) {
    return readWord(mem[++PC]) + Y;
}

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
    if (PC == prevPC){
        printf("\nInfinite loop at 0x%.4x\n", PC);
        glFinish(); // Flush any drawings that haven't been drawn yet
        pthread_exit(NULL);
    }
    prevPC = PC;

    switch (mem[PC]) {
        case 0x00:
        BRK();
        break;

        case 0x01:
        ORA(readAdrIndX());
        break;

        case 0x02:
        // Illegal
        JAM();
        break;

        case 0x03:
        // Illegal
        SLO(readAdrIndX());
        break;

        case 0x04:
        // Illegal
        NOP(2);
        break;

        case 0x05:
        ORA(readAdrZP());
        break;

        case 0x06:
        ASL(readAdrZP());
        break;

        case 0x07:
        // Illegal
        SLO(readAdrZP());
        break;

        case 0x08:
        PHP();
        break;

        case 0x09:
        ORA(readAdrImmediate());
        break;

        case 0x0a:
        ASLA();
        break;

        case 0x0b:
        // Illegal
        ANC(readAdrImmediate());
        break;

        case 0x0c:
        // Illegal
        NOP(3);
        break;

        case 0x0d:
        ORA(readAdrAbs());
        break;

        case 0x0e:
        ASL(readAdrAbs());
        break;

        case 0x0f:
        // Illegal
        SLO(readAdrAbs());
        break;

        case 0x10:
        BPL(readAdrRel());
        break;

        case 0x11:
        ORA(readAdrIndY());
        break;

        case 0x12:
        // Illegal
        JAM();
        break;

        case 0x13:
        // Illegal
        SLO(readAdrIndY());
        break;

        case 0x14:
        // Illegal
        NOP(2);
        break;

        case 0x15:
        ORA(readAdrZPX());
        break;

        case 0x16:
        ASL(readAdrZPX());
        break;

        case 0x17:
        // Illegal
        SLO(readAdrZPX());
        break;

        case 0x18:
        CLC();
        break;

        case 0x19:
        ORA(readAdrAbsY());
        break;

        case 0x1a:
        // Illegal
        NOP(1);
        break;

        case 0x1b:
        // Illegal
        SLO(readAdrAbsY());
        break;

        case 0x1c:
        // Illegal
        NOP(3);
        break;

        case 0x1d:
        ORA(readAdrAbsX());
        break;

        case 0x1e:
        ASL(readAdrAbsX());
        break;

        case 0x1f:
        // Illegal
        SLO(readAdrAbsX());
        break;

        case 0x20:
        JSR(readAdrAbs());
        break;

        case 0x21:
        AND(readAdrIndX());
        break;

        case 0x22:
        // Illegal
        JAM();
        break;

        case 0x23:
        // Illegal
        RLA(readAdrIndX());
        break;

        case 0x24:
        BIT(readAdrZP());
        break;

        case 0x25:
        AND(readAdrZP());
        break;

        case 0x26:
        ROL(readAdrZP());
        break;

        case 0x27:
        // Illegal
        RLA(readAdrZP());
        break;

        case 0x28:
        PLP();
        break;

        case 0x29:
        AND(readAdrImmediate());
        break;

        case 0x2a:
        ROLA();
        break;

        case 0x2b:
        // Illegal
        ANC(readAdrImmediate());
        break;

        case 0x2c:
        BIT(readAdrAbs());
        break;

        case 0x2d:
        AND(readAdrAbs());
        break;

        case 0x2e:
        ROL(readAdrAbs());
        break;

        case 0x2f:
        // Illegal
        RLA(readAdrAbs());
        break;

        case 0x30:
        BMI(readAdrRel());
        break;

        case 0x31:
        AND(readAdrIndY());
        break;

        case 0x32:
        // Illegal
        JAM();
        break;

        case 0x33:
        // Illegal
        RLA(readAdrIndY());
        break;

        case 0x34:
        // Illegal
        NOP(2);
        break;

        case 0x35:
        AND(readAdrZPX());
        break;

        case 0x36:
        ROL(readAdrZPX());
        break;

        case 0x37:
        // Illegal
        RLA(readAdrZPX());
        break;

        case 0x38:
        SEC();
        break;

        case 0x39:
        AND(readAdrAbsY());
        break;

        case 0x3a:
        // Illegal
        NOP(1);
        break;

        case 0x3b:
        // Illegal
        RLA(readAdrAbsY());
        break;

        case 0x3c:
        // Illegal
        NOP(3);
        break;

        case 0x3d:
        AND(readAdrAbsX());
        break;

        case 0x3e:
        ROL(readAdrAbsX());
        break;

        case 0x3f:
        // Illegal
        RLA(readAdrAbsX());
        break;

        case 0x40:
        RTI();
        break;

        case 0x41:
        EOR(readAdrIndX());
        break;

        case 0x42:
        // Illegal
        JAM();
        break;

        case 0x43:
        // Illegal
        SRE(readAdrIndX());
        break;

        case 0x44:
        // Illegal
        NOP(2);
        break;

        case 0x45:
        EOR(readAdrZP());
        break;

        case 0x46:
        LSR(readAdrZP());
        break;

        case 0x47:
        // Illegal
        SRE(readAdrZP());
        break;

        case 0x48:
        PHA();
        break;

        case 0x49:
        EOR(readAdrImmediate());
        break;

        case 0x4a:
        LSRA();
        break;

        case 0x4b:
        // Illegal
        ALR(readAdrImmediate());
        break;

        case 0x4c:
        JMP(readAdrAbs());
        break;

        case 0x4d:
        EOR(readAdrAbs());
        break;

        case 0x4e:
        LSR(readAdrAbs());
        break;

        case 0x4f:
        // Illegal
        SRE(readAdrAbs());
        break;

        case 0x50:
        BVC(readAdrRel());
        break;

        case 0x51:
        EOR(readAdrIndY());
        break;

        case 0x52:
        // Illegal
        JAM();
        break;

        case 0x53:
        // Illegal
        SRE(readAdrIndY());
        break;

        case 0x54:
        // Illegal
        NOP(2);
        break;

        case 0x55:
        EOR(readAdrZPX());
        break;

        case 0x56:
        LSR(readAdrZPX());
        break;

        case 0x57:
        // Illegal
        SRE(readAdrZPX());
        break;

        case 0x58:
        CLI();
        break;

        case 0x59:
        EOR(readAdrAbsY());
        break;

        case 0x5a:
        // Illegal
        NOP(1);
        break;

        case 0x5b:
        // Illegal
        SRE(readAdrAbsY());
        break;

        case 0x5c:
        // Illegal
        NOP(3);
        break;

        case 0x5d:
        EOR(readAdrAbsX());
        break;

        case 0x5e:
        LSR(readAdrAbsX());
        break;

        case 0x5f:
        // Illegal
        SRE(readAdrAbsX());
        break;

        case 0x60:
        RTS();
        break;

        case 0x61:
        ADC(readAdrIndX());
        break;

        case 0x62:
        // Illegal
        JAM();
        break;

        case 0x63:
        // Illegal
        RRA(readAdrIndX());
        break;

        case 0x64:
        // Illegal
        NOP(2);
        break;

        case 0x65:
        ADC(readAdrZP());
        break;

        case 0x66:
        ROR(readAdrZP());
        break;

        case 0x67:
        // Illegal
        RRA(readAdrZP());
        break;

        case 0x68:
        PLA();
        break;

        case 0x69:
        ADC(readAdrImmediate());
        break;

        case 0x6a:
        RORA();
        break;

        case 0x6b:
        // Illegal
        ARR(readAdrImmediate());
        break;

        case 0x6c:
        JMP(readAdrInd());
        break;

        case 0x6d:
        ADC(readAdrAbs());
        break;

        case 0x6e:
        ROR(readAdrAbs());
        break;

        case 0x6f:
        // Illegal
        RRA(readAdrAbs());
        break;

        case 0x70:
        BVS(readAdrRel());
        break;

        case 0x71:
        ADC(readAdrIndY());
        break;

        case 0x72:
        // Illegal
        JAM();
        break;

        case 0x73:
        // Illegal
        RRA(readAdrIndY());
        break;

        case 0x74:
        // Illegal
        NOP(2);
        break;

        case 0x75:
        ADC(readAdrZPX());
        break;

        case 0x76:
        ROR(readAdrZPX());
        break;

        case 0x77:
        // Illegal
        RRA(readAdrZPX());
        break;

        case 0x78:
        SEI();
        break;

        case 0x79:
        ADC(readAdrAbsY());
        break;

        case 0x7a:
        // Illegal
        NOP(1);
        break;

        case 0x7b:
        // Illegal
        RRA(readAdrAbsY());
        break;

        case 0x7c:
        // Illegal
        NOP(3);
        break;

        case 0x7d:
        ADC(readAdrAbsX());
        break;

        case 0x7e:
        ROR(readAdrAbsX());
        break;

        case 0x7f:
        // Illegal
        RRA(readAdrAbsX());
        break;

        case 0x80:
        // Illegal
        NOP(2);
        break;

        case 0x81:
        STA(readAdrIndX());
        break;

        case 0x82:
        // Illegal
        NOP(2);
        break;

        case 0x83:
        // Illegal
        SAX(readAdrIndX());
        break;

        case 0x84:
        STY(readAdrZP());
        break;

        case 0x85:
        STA(readAdrZP());
        break;

        case 0x86:
        STX(readAdrZP());
        break;

        case 0x87:
        // Illegal
        SAX(readAdrZP());
        break;

        case 0x88:
        DEY();
        break;

        case 0x89:
        // Illegal
        NOP(2);
        break;

        case 0x8a:
        TXA();
        break;

        case 0x8b:
        // Illegal
        ANE(readAdrImmediate());
        break;

        case 0x8c:
        STY(readAdrAbs());
        break;

        case 0x8d:
        STA(readAdrAbs());
        break;

        case 0x8e:
        STX(readAdrAbs());
        break;

        case 0x8f:
        // Illegal
        SAX(readAdrAbs());
        break;

        case 0x90:
        BCC(readAdrRel());
        break;

        case 0x91:
        STA(readAdrIndY());
        break;

        case 0x92:
        // Illegal
        JAM();
        break;

        case 0x93:
        // Illegal
        SHA(readAdrIndY());
        break;

        case 0x94:
        STY(readAdrZPX());
        break;

        case 0x95:
        STA(readAdrZPX());
        break;

        case 0x96:
        STX(readAdrZPY());
        break;

        case 0x97:
        // Illegal
        SAX(readAdrZPY());
        break;

        case 0x98:
        TYA();
        break;

        case 0x99:
        STA(readAdrAbsY());
        break;

        case 0x9a:
        TXS();
        break;

        case 0x9b:
        // Illegal
        TAS(readAdrAbsY());
        break;

        case 0x9c:
        // Illegal
        SHY(readAdrAbsX());
        break;

        case 0x9d:
        STA(readAdrAbsX());
        break;

        case 0x9e:
        // Illegal
        SHX(readAdrAbsY());
        break;

        case 0x9f:
        // Illegal
        SHA(readAdrAbsY());
        break;

        case 0xa0:
        LDY(readAdrImmediate());
        break;

        case 0xa1:
        LDA(readAdrIndX());
        break;

        case 0xa2:
        LDX(readAdrImmediate());
        break;

        case 0xa3:
        // Illegal
        LAX(readAdrIndX());
        break;

        case 0xa4:
        LDY(readAdrZP());
        break;

        case 0xa5:
        LDA(readAdrZP());
        break;

        case 0xa6:
        LDX(readAdrZP());
        break;

        case 0xa7:
        // Illegal
        LAX(readAdrZP());
        break;

        case 0xa8:
        TAY();
        break;

        case 0xa9:
        LDA(readAdrImmediate());
        break;

        case 0xaa:
        TAX();
        break;

        case 0xab:
        // Illegal
        LXA(readAdrImmediate());
        break;

        case 0xac:
        LDY(readAdrAbs());
        break;

        case 0xad:
        LDA(readAdrAbs());
        break;

        case 0xae:
        LDX(readAdrAbs());
        break;

        case 0xaf:
        // Illegal
        LAX(readAdrAbs());
        break;

        case 0xb0:
        BCS(readAdrRel());
        break;

        case 0xb1:
        LDA(readAdrIndY());
        break;

        case 0xb2:
        // Illegal
        JAM();
        break;

        case 0xb3:
        // Illegal
        LAX(readAdrIndY());
        break;

        case 0xb4:
        LDY(readAdrZPX());
        break;

        case 0xb5:
        LDA(readAdrZPX());
        break;

        case 0xb6:
        LDX(readAdrZPY());
        break;

        case 0xb7:
        // Illegal
        LAX(readAdrZPY());
        break;

        case 0xb8:
        CLV();
        break;

        case 0xb9:
        LDA(readAdrAbsY());
        break;

        case 0xba:
        TSX();
        break;

        case 0xbb:
        // Illegal
        LAS(readAdrAbsY());
        break;

        case 0xbc:
        LDY(readAdrAbsX());
        break;

        case 0xbd:
        LDA(readAdrAbsX());
        break;

        case 0xbe:
        LDX(readAdrAbsY());
        break;

        case 0xbf:
        // Illegal
        LAX(readAdrAbsY());
        break;

        case 0xc0:
        CPY(readAdrImmediate());
        break;

        case 0xc1:
        CMP(readAdrIndX());
        break;

        case 0xc2:
        // Illegal
        NOP(2);
        break;

        case 0xc3:
        // Illegal
        DCP(readAdrIndX());
        break;

        case 0xc4:
        CPY(readAdrZP());
        break;

        case 0xc5:
        CMP(readAdrZP());
        break;

        case 0xc6:
        DEC(readAdrZP());
        break;

        case 0xc7:
        // Illegal
        DCP(readAdrZP());
        break;

        case 0xc8:
        INY();
        break;

        case 0xc9:
        CMP(readAdrImmediate());
        break;

        case 0xca:
        DEX();
        break;

        case 0xcb:
        // Illegal
        SBX(readAdrImmediate());
        break;

        case 0xcc:
        CPY(readAdrAbs());
        break;

        case 0xcd:
        CMP(readAdrAbs());
        break;

        case 0xce:
        DEC(readAdrAbs());
        break;

        case 0xcf:
        // Illegal
        DCP(readAdrAbs());
        break;

        case 0xd0:
        BNE(readAdrRel());
        break;

        case 0xd1:
        CMP(readAdrIndY());
        break;

        case 0xd2:
        // Illegal
        JAM();
        break;

        case 0xd3:
        // Illegal
        DCP(readAdrIndY());
        break;

        case 0xd4:
        // Illegal
        NOP(2);
        break;

        case 0xd5:
        CMP(readAdrZPX());
        break;

        case 0xd6:
        DEC(readAdrZPX());
        break;

        case 0xd7:
        // Illegal
        DCP(readAdrZPX());
        break;

        case 0xd8:
        CLD();
        break;

        case 0xd9:
        CMP(readAdrAbsY());
        break;

        case 0xda:
        // Illegal
        NOP(1);
        break;

        case 0xdb:
        // Illegal
        DCP(readAdrAbsY());
        break;

        case 0xdc:
        // Illegal
        NOP(3);
        break;

        case 0xdd:
        CMP(readAdrAbsX());
        break;

        case 0xde:
        DEC(readAdrAbsX());
        break;

        case 0xdf:
        // Illegal
        DCP(readAdrAbsX());
        break;

        case 0xe0:
        CPX(readAdrImmediate());
        break;

        case 0xe1:
        SBC(readAdrIndX());
        break;

        case 0xe2:
        // Illegal
        NOP(2);
        break;

        case 0xe3:
        // Illegal
        ISC(readAdrIndX());
        break;

        case 0xe4:
        CPX(readAdrZP());
        break;

        case 0xe5:
        SBC(readAdrZP());
        break;

        case 0xe6:
        INC(readAdrZP());
        break;

        case 0xe7:
        // Illegal
        ISC(readAdrZP());
        break;

        case 0xe8:
        INX();
        break;

        case 0xe9:
        SBC(readAdrImmediate());
        break;

        case 0xea:
        NOP(1);
        break;

        case 0xeb:
        // Illegal
        SBC(readAdrImmediate());
        break;

        case 0xec:
        CPX(readAdrAbs());
        break;

        case 0xed:
        SBC(readAdrAbs());
        break;

        case 0xee:
        INC(readAdrAbs());
        break;

        case 0xef:
        // Illegal
        ISC(readAdrAbs());
        break;

        case 0xf0:
        BEQ(readAdrRel());
        break;

        case 0xf1:
        SBC(readAdrIndY());
        break;

        case 0xf2:
        // Illegal
        JAM();
        break;

        case 0xf3:
        // Illegal
        ISC(readAdrIndY());
        break;

        case 0xf4:
        // Illegal
        NOP(2);
        break;

        case 0xf5:
        SBC(readAdrZPX());
        break;

        case 0xf6:
        INC(readAdrZPX());
        break;

        case 0xf7:
        // Illegal
        ISC(readAdrZPX());
        break;

        case 0xf8:
        SED();
        break;

        case 0xf9:
        SBC(readAdrAbsY());
        break;

        case 0xfa:
        // Illegal
        NOP(1);
        break;

        case 0xfb:
        // Illegal
        ISC(readAdrAbsY());
        break;

        case 0xfc:
        // Illegal
        NOP(3);
        break;

        case 0xfd:
        SBC(readAdrAbsX());
        break;

        case 0xfe:
        INC(readAdrAbsX());
        break;

        case 0xff:
        // Illegal
        ISC(readAdrAbsX());
        break;

        // All possible opcodes covered, don't need default statement
    }
}