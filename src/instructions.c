#include <stdio.h>
#include <stdint.h>
#include <GLEW/glew.h>
#include <GLFW/glfw3.h>
#include <pthread.h>
#include "instructions.h"
#include "emulate.h"
#include "display.h"

uint16_t readWord(const uint16_t pointer) {
    const uint16_t hi = mem[pointer + 1] << 8;
    const uint8_t lo = mem[pointer];
    return hi + lo;
}

static inline void setZeroFlag(const uint8_t val) {
    zeroFlag = val == 0;
}

static inline void setNegativeFlag(const uint8_t val) {
    negativeFlag = val & 0x80;
}

static inline void pushStack(const uint8_t val) {
    mem[0x100 + SP--] = val;
}

static inline uint8_t pullStack(void) {
    return mem[0x100 + (++SP)];
}

static void pushStatus(void) {
    uint8_t status = 0;
    status |= (uint8_t)(negativeFlag) << 7;
    status |= (uint8_t)(overflowFlag) << 6;
    status |= 0x30; // Bit 5 is always 1, bit 4 is 1 when pushed from BRK or PHP (always)
    status |= (uint8_t)(decimalFlag) << 3;
    status |= (uint8_t)(interruptFlag) << 2;
    status |= (uint8_t)(zeroFlag) << 1;
    status |= (uint8_t)(carryFlag);
    pushStack(status);
}

static void pullStatus(void) {
    const uint8_t status = pullStack();
    negativeFlag = status & 0x80;
    overflowFlag = status & 0x40;
    decimalFlag = status & 0x08;
    interruptFlag = status & 0x04;
    zeroFlag = status & 0x02;
    carryFlag = status & 0x01;
}

static void writeByte(const uint16_t pointer, const uint8_t byte) {
    if (pointer == 0xfffa) {
        putchar(byte);
    } else if (pointer == 0xfffb) {
        const double endTime = glfwGetTime() + ((double)byte) / 1000.0;
        // Flush drawings since there's a delay anyway
        glFinish();
        drawQueued = false;
        while (!glfwWindowShouldClose(window) && glfwGetTime() < endTime) {}
    } else if (pointer >= 0xe000 && pointer <= 0xefff) {
        glUniform4f(colourUniform, (float)((byte & 0xe0) >> 5) / 7.0f, (float)((byte & 0x1c) >> 2) / 7.0f, (float)(byte & 0x03) / 3.0f, 1.0f);
        glUniform2f(positionUniform, (float)((pointer - 0xe000) & 0x3f), (float)((pointer - 0xe000) >> 6));
        glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_BYTE, NULL);
        drawQueued = true;
    }

    mem[pointer] = byte;
}

// Instructions

void ADC(const uint16_t pointer) {
    const uint8_t binaryResult = AC + mem[pointer] + carryFlag;

    if (decimalFlag) {
        // Implemented according to http://www.6502.org/tutorials/decimal_mode.html APPENDIX A

        const uint8_t oldACValue = AC;

        uint16_t resultLow = (AC & 0xf) + (mem[pointer] & 0xf) + carryFlag;
        if (resultLow >= 0xa) {
            resultLow = ((resultLow + 6) & 0xf) + 0x10;
        }

        int16_t result = (AC & 0xf0) + (mem[pointer] & 0xf0) + resultLow;
        if (result >= 0xa0) {
            result += 0x60;
        }

        AC = result & 0xff;

        carryFlag = result > 0xff;

        result = ((int8_t)oldACValue & 0xf0) + ((int8_t)mem[pointer] & 0xf0) + resultLow;
        setNegativeFlag(result);
        overflowFlag = result < -128 || result > 127;
    } else {
        const uint16_t carryCheck = AC + mem[pointer] + carryFlag;
        const int16_t overflowCheck = (int8_t)AC + (int8_t)mem[pointer] + carryFlag;

        AC = binaryResult;
        carryFlag = carryCheck > 0xff;
        overflowFlag = overflowCheck > 127 || overflowCheck < -128;
        setNegativeFlag(AC);
    }

    setZeroFlag(binaryResult);

    PC++;
}

void AND(const uint16_t pointer) {
    AC &= mem[pointer];
    setZeroFlag(AC);
    setNegativeFlag(AC);
    PC++;
}

void ASL(const uint16_t pointer) {
    carryFlag = mem[pointer] & 0x80;
    writeByte(pointer, mem[pointer] << 1);
    setZeroFlag(mem[pointer]);
    setNegativeFlag(mem[pointer]);
    PC++;
}

void ASLA(void) {
    carryFlag = AC & 0x80;
    AC = AC << 1;
    setZeroFlag(AC);
    setNegativeFlag(AC);
    PC++;
}

void BCC(const uint16_t pointer) {
    if (!carryFlag) {
        PC += ((int8_t)mem[pointer]);
    }
    PC++;
}

void BCS(const uint16_t pointer) {
    if (carryFlag) {
        PC += ((int8_t)mem[pointer]);
    }
    PC++;
}

void BEQ(const uint16_t pointer) {
    if (zeroFlag) {
        PC += ((int8_t)mem[pointer]);
    }
    PC++;
}

void BIT(const uint16_t pointer) {
    setZeroFlag(AC & mem[pointer]);
    overflowFlag = mem[pointer] & 0x40;
    setNegativeFlag(mem[pointer]);
    PC++;
}

void BMI(const uint16_t pointer) {
    if (negativeFlag) {
        PC += ((int8_t)mem[pointer]);
    }
    PC++;
}

void BNE(const uint16_t pointer) {
    if (!zeroFlag) {
        PC += ((int8_t)mem[pointer]);
    }
    PC++;
}

void BPL(const uint16_t pointer) {
    if (!negativeFlag) {
        PC += ((int8_t)mem[pointer]);
    }
    PC++;
}

void BRK(void) {
    PC += 2;
    pushStack(PC >> 8);
    pushStack(PC & 0xff);
    pushStatus();
    interruptFlag = true;
    PC = readWord(0xfffe);
}

void BVC(const uint16_t pointer) {
    if (!overflowFlag) {
        PC += ((int8_t)mem[pointer]);
    }
    PC++;
}

void BVS(const uint16_t pointer) {
    if (overflowFlag) {
        PC += ((int8_t)mem[pointer]);
    }
    PC++;
}

void CLC(void) {
    carryFlag = false;
    PC++;
}

void CLD(void) {
    decimalFlag = false;
    PC++;
}

void CLI(void) {
    interruptFlag = false;
    PC++;
}

void CLV(void) {
    overflowFlag = false;
    PC++;
}

void CMP(const uint16_t pointer) {
    carryFlag = AC >= mem[pointer];
    zeroFlag = AC == mem[pointer];
    setNegativeFlag(AC - mem[pointer]);
    PC++;
}

void CPX(const uint16_t pointer) {
    carryFlag = X >= mem[pointer];
    zeroFlag = X == mem[pointer];
    setNegativeFlag(X - mem[pointer]);
    PC++;
}

void CPY(const uint16_t pointer) {
    carryFlag = Y >= mem[pointer];
    zeroFlag = Y == mem[pointer];
    setNegativeFlag(Y - mem[pointer]);
    PC++;
}

void DEC(const uint16_t pointer) {
    writeByte(pointer, mem[pointer] - 1);
    setZeroFlag(mem[pointer]);
    setNegativeFlag(mem[pointer]);
    PC++;
}

void DEX(void) {
    X--;
    setZeroFlag(X);
    setNegativeFlag(X);
    PC++;
}

void DEY(void) {
    Y--;
    setZeroFlag(Y);
    setNegativeFlag(Y);
    PC++;
}

void EOR(const uint16_t pointer) {
    AC ^= mem[pointer];
    setZeroFlag(AC);
    setNegativeFlag(AC);
    PC++;
}

void INC(const uint16_t pointer) {
    writeByte(pointer, mem[pointer] + 1);
    setZeroFlag(mem[pointer]);
    setNegativeFlag(mem[pointer]);
    PC++;
}

void INX(void) {
    X++;
    setZeroFlag(X);
    setNegativeFlag(X);
    PC++;
}

void INY(void) {
    Y++;
    setZeroFlag(Y);
    setNegativeFlag(Y);
    PC++;
}

void JMP(const uint16_t pointer) {
    PC = pointer;
}

void JSR(const uint16_t pointer) {
    pushStack(PC >> 8);
    pushStack(PC & 0xff);
    PC = pointer;
}

void LDA(const uint16_t pointer) {
    AC = mem[pointer];
    setZeroFlag(AC);
    setNegativeFlag(AC);
    PC++;
}

void LDX(const uint16_t pointer) {
    X = mem[pointer];
    setZeroFlag(X);
    setNegativeFlag(X);
    PC++;
}

void LDY(const uint16_t pointer) {
    Y = mem[pointer];
    setZeroFlag(Y);
    setNegativeFlag(Y);
    PC++;
}

void LSR(const uint16_t pointer) {
    carryFlag = mem[pointer] & 0x01;
    writeByte(pointer, mem[pointer] >> 1);
    setZeroFlag(mem[pointer]);
    setNegativeFlag(mem[pointer]);
    PC++;
}

void LSRA(void) {
    carryFlag = AC & 0x01;
    AC = AC >> 1;
    setZeroFlag(AC);
    setNegativeFlag(AC);
    PC++;
}

void NOP(uint8_t bytes) {
    PC += bytes; // Illegal versions might not be 1 byte
    return;
}

void ORA(const uint16_t pointer) {
    AC |= mem[pointer];
    setZeroFlag(AC);
    setNegativeFlag(AC);
    PC++;
}

void PHA(void) {
    pushStack(AC);
    PC++;
}

void PHP(void) {
    pushStatus();
    PC++;
}

void PLA(void) {
    AC = pullStack();
    setZeroFlag(AC);
    setNegativeFlag(AC);
    PC++;
}

void PLP(void) {
    pullStatus();
    PC++;
}

void ROL(const uint16_t pointer) {
    const bool tmpCarryFlag = mem[pointer] & 0x80;
    writeByte(pointer, (mem[pointer] << 1) | carryFlag);
    carryFlag = tmpCarryFlag;
    setZeroFlag(mem[pointer]);
    setNegativeFlag(mem[pointer]);
    PC++;
}

void ROLA(void) {
    const bool tmpCarryFlag = AC & 0x80;
    AC = (AC << 1) | carryFlag;
    carryFlag = tmpCarryFlag;
    setZeroFlag(AC);
    setNegativeFlag(AC);
    PC++;
}

void ROR(const uint16_t pointer) {
    const bool tmpCarryFlag = mem[pointer] & 0x01;
    writeByte(pointer, (mem[pointer] >> 1) | (((uint8_t)carryFlag) << 7));
    carryFlag = tmpCarryFlag;
    setZeroFlag(mem[pointer]);
    setNegativeFlag(mem[pointer]);
    PC++;
}

void RORA(void) {
    const bool tmpCarryFlag = AC & 0x01;
    AC = (AC >> 1) | (((uint8_t)carryFlag) << 7);
    carryFlag = tmpCarryFlag;
    setZeroFlag(AC);
    setNegativeFlag(AC);
    PC++;
}

void RTI(void) {
    pullStatus();
    PC = pullStack();
    PC |= pullStack() << 8;
}

void RTS(void) {
    PC = pullStack();
    PC |= pullStack() << 8;
    PC++;
}

void SBC(const uint16_t pointer) {
    const uint16_t carryCheck = AC - mem[pointer] - 1 + carryFlag;
    const int16_t overflowCheck = (int8_t)AC - (int8_t)mem[pointer] - 1 + carryFlag;

    const uint8_t binaryResult = AC - mem[pointer] - 1 + carryFlag;

    if (decimalFlag) {
        // Implemented according to http://www.6502.org/tutorials/decimal_mode.html APPENDIX A

        int16_t resultLow = (AC & 0xf) - (mem[pointer] & 0xf) - 1 + carryFlag;
        if (resultLow < 0) {
            resultLow = ((resultLow - 6) & 0xf) - 0x10;
        }

        int16_t result = (AC & 0xf0) - (mem[pointer] & 0xf0) + resultLow;
        if (result < 0) {
            result -= 0x60;
        }

        AC = result & 0xff;
    } else {
        AC = binaryResult;
    }

    setZeroFlag(binaryResult);
    overflowFlag = overflowCheck > 127 || overflowCheck < -128;
    setNegativeFlag(binaryResult);
    carryFlag = carryCheck <= 0xff;

    PC++;
}

void SEC(void) {
    carryFlag = true;
    PC++;
}

void SED(void) {
    decimalFlag = true;
    PC++;
}

void SEI(void) {
    interruptFlag = true;
    PC++;
}

void STA(const uint16_t pointer) {
    writeByte(pointer, AC);
    PC++;
}

void STX(const uint16_t pointer) {
    writeByte(pointer, X);
    PC++;
}

void STY(const uint16_t pointer) {
    writeByte(pointer, Y);
    PC++;
}

void TAX(void) {
    X = AC;
    setZeroFlag(X);
    setNegativeFlag(X);
    PC++;
}

void TAY(void) {
    Y = AC;
    setZeroFlag(Y);
    setNegativeFlag(Y);
    PC++;
}

void TSX(void) {
    X = SP;
    setZeroFlag(X);
    setNegativeFlag(X);
    PC++;
}

void TXA(void) {
    AC = X;
    setZeroFlag(AC);
    setNegativeFlag(AC);
    PC++;
}

void TXS(void) {
    SP = X;
    PC++;
}

void TYA(void) {
    AC = Y;
    setZeroFlag(AC);
    setNegativeFlag(AC);
    PC++;
}

// Illegal instructions

void ALR(uint16_t pointer) {
    uint8_t val = AC * mem[pointer];
    carryFlag = val & 1;
    val >>= 1;
    setZeroFlag(val);
    setNegativeFlag(val);
    PC++;
}

void ANC(uint16_t pointer) {
    const uint8_t val = AC & mem[pointer];
    carryFlag = val & 0x80;
    setZeroFlag(val);
    setNegativeFlag(val);
    PC++;
}

void ANE(uint16_t pointer) {
    // The 0xff is (AC | random value) where the random value is recommended to be 0xff
    // In the real chip, the value changes based on chip series, temperature, etc.
    AC = 0xff & X & mem[pointer];
    setZeroFlag(AC);
    setNegativeFlag(AC);
    PC++;
}

void ARR(uint16_t pointer) {
    const uint8_t oldAC = AC;
    const bool oldCarry = carryFlag;
    AC &= mem[pointer];
    ADC(pointer); // To set overflow flag
    ROR(pointer);
    AC = oldAC;
    mem[pointer] |= oldCarry << 7;
    PC--;
}

void DCP(uint16_t pointer) {
    mem[pointer] -= 1;
    CMP(pointer);
}

void ISC(uint16_t pointer) {
    mem[pointer]++;
    SBC(pointer);
}

void JAM(void) {
    printf("\nHit illegal JAM instruction %.2x at %.4x\n", mem[PC], PC);
    glFinish(); // Flush any drawings that haven't been drawn yet
    pthread_exit(NULL);
}

void LAS(uint16_t pointer) {
    AC = mem[pointer] & SP;
    X = AC;
    SP = AC;
    setZeroFlag(AC);
    setNegativeFlag(AC);
    PC++;
}

void LAX(uint16_t pointer) {
    LDA(pointer);
    X = AC;
}

void LXA(uint16_t pointer) {
    // The 0xff is (AC | random value) where the random value is recommended to be 0xff
    // In the real chip, the value changes based on chip series, temperature, etc.
    AC = 0xff & mem[pointer];
    X = AC;
    setZeroFlag(AC);
    setNegativeFlag(AC);
    PC++;
}

void RLA(uint16_t pointer) {
    const uint8_t oldVal = mem[pointer];
    ROL(pointer);
    AC &= oldVal;
    setZeroFlag(AC);
    setNegativeFlag(AC);
}

void RRA(uint16_t pointer) {
    const uint8_t oldVal = mem[pointer];
    ROR(pointer);
    const uint8_t newVal = mem[pointer];
    mem[pointer] = oldVal;
    ADC(pointer);
    mem[pointer] = newVal;
    PC--;
}

void SAX(uint16_t pointer) {
    mem[pointer] = AC & X;
    PC++;
}

void SBX(uint16_t pointer) {
    X = AC & X;
    carryFlag = X >= mem[pointer];
    zeroFlag = X == mem[pointer];
    X -= mem[pointer];
    setNegativeFlag(X);
    PC++;
}

void SHA(uint16_t pointer) {
    // The (& mem[pointer + 1]) may be dropped, or not cross page boundaries
    // This behaviour is not emulated
    mem[pointer] = AC & X & mem[(pointer + 1) & 0xffff];
    PC++;
}

void SHX(uint16_t pointer) {
    // The (& mem[pointer + 1]) may be dropped, or not cross page boundaries
    // This behaviour is not emulated
    mem[pointer] = X & mem[(pointer + 1) & 0xffff];
    PC++;
}

void SHY(uint16_t pointer) {
    // The (& mem[pointer + 1]) may be dropped, or not cross page boundaries
    // This behaviour is not emulated
    mem[pointer] = Y & mem[(pointer + 1) & 0xffff];
    PC++;
}

void SLO(uint16_t pointer) {
    const uint8_t oldVal = mem[pointer];
    ASL(pointer);
    AC |= oldVal;
    setZeroFlag(AC);
    setNegativeFlag(AC);
}

void SRE(uint16_t pointer) {
    const uint8_t oldVal = mem[pointer];
    LSR(pointer);
    AC ^= oldVal;
    setZeroFlag(AC);
    setNegativeFlag(AC);
}

void TAS(uint16_t pointer) {
    SP = AC & X;
    SHA(pointer);
}