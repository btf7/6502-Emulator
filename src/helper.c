#include <stdio.h>
#include <stdint.h>
#include <GLEW/glew.h>
#include <GLFW/glfw3.h>
#include "helper.h"
#include "emulate.h"
#include "display.h"

void pushStack(const uint8_t val) {
    mem[0x100 + SP] = val;
    SP--;
}

uint8_t getStatus(void) {
    uint8_t status = 0;
    status |= (uint8_t)(negativeFlag) << 7;
    status |= (uint8_t)(overflowFlag) << 6;
    status |= 0x30; // Bit 3 is always 1, bit 4 is 1 when pushed from BRK or PHP (always)
    status |= (uint8_t)(decimalFlag) << 3;
    status |= (uint8_t)(interruptFlag) << 2;
    status |= (uint8_t)(zeroFlag) << 1;
    status |= (uint8_t)(carryFlag);
    return status;
}

uint16_t readWord(const uint16_t pointer) {
    const uint16_t hi = mem[pointer + 1] << 8;
    const uint8_t lo = mem[pointer];
    return hi + lo;
}

void writeByte(const uint16_t pointer, const uint8_t byte) {
    if (pointer == 0xfffa) {
        putc(byte, stdout);
    } else if (pointer >= 0xe000 && pointer <= 0xefff) {
        glUniform4f(colourUniform, (float)((byte & 0xe0) >> 5) / 7.0f, (float)((byte & 0x1c) >> 2) / 7.0f, (float)(byte & 0x03) / 3.0f, 1.0f);
        glUniform2f(positionUniform, (float)((pointer - 0xe000) & 0x3f), (float)((pointer - 0xe000) >> 6));
        glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_BYTE, NULL);
        drawQueued = true;
    }

    mem[pointer] = byte;
}