#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <GLEW/glew.h>
#include <GLFW/glfw3.h>

unsigned int compileShader(const char* string, unsigned int type) {
    unsigned int id = glCreateShader(type);
    glShaderSource(id, 1, &string, NULL);
    glCompileShader(id);

    int result;
    glGetShaderiv(id, GL_COMPILE_STATUS, &result);
    if (result == GL_FALSE) {
        int len;
        glGetShaderiv(id, GL_INFO_LOG_LENGTH, &len);
        char* msg = (char*)alloca(len);
        glGetShaderInfoLog(id, len, NULL, msg);
        printf("Failed to compile shader:\n");
        printf("%s\n", msg);
        glDeleteShader(id);
        glfwTerminate();
        exit(1);
    }

    return id;
}

unsigned int createProgram(const char* vShaderSource, const char* fShaderSource) {
    unsigned int program = glCreateProgram();
    unsigned int vShader = compileShader(vShaderSource, GL_VERTEX_SHADER);
    unsigned int fs = compileShader(fShaderSource, GL_FRAGMENT_SHADER);
    
    glAttachShader(program, vShader);
    glAttachShader(program, fs);
    glLinkProgram(program);
    glValidateProgram(program); // Is this needed?

    glDeleteShader(vShader);
    glDeleteShader(fs);

    return program;
}

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

int colourUniform;
int positionUniform;

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

uint8_t getStatus() {
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
    } else if (pointer >= 0xe000 && pointer <= 0xefff){// && (byte == 0xe0 || byte == 0x1c || byte == 0x03)) {
        glUniform4f(colourUniform, (float)((byte & 0xe0) >> 5) / 7.0f, (float)((byte & 0x1c) >> 2) / 7.0f, (float)(byte & 0x03) / 3.0f, 1.0f);
        glUniform2f(positionUniform, (float)((pointer - 0xe000) & 0x3f), (float)((pointer - 0xe000) >> 6));
        glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_BYTE, NULL);
        drawQueued = true;
    }

    mem[pointer] = byte;
}

int main(int argc, char** argv) {
    if (argc != 2) {
        printf("Expected 1 input parameter, got %d\n", argc - 1);
        exit(1);
    }

    // Initialise
    readFile(argv[1]);
    PC = readWord(0xfffc);

    if (!glfwInit()){
        printf("Failed to initialise GLFW\n");
        exit(1);
    }

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);

    glfwWindowHint(GLFW_RED_BITS, 3);
    glfwWindowHint(GLFW_GREEN_BITS, 3);
    glfwWindowHint(GLFW_BLUE_BITS, 2);
    glfwWindowHint(GLFW_ALPHA_BITS, 0);

    glfwWindowHint(GLFW_DOUBLEBUFFER, GLFW_FALSE);

    GLFWwindow* window = glfwCreateWindow(640, 640, "6502 Emulator", NULL, NULL);
    if (!window) {
        printf("Failed to create window\n");
        glfwTerminate();
        exit(1);
    }

    glfwMakeContextCurrent(window);

    if (glewInit() != GLEW_OK){
        printf("Failed to initialise GLEW\n");
        glfwTerminate();
        exit(1);
    }

    float tileVertices[] = {
        -1.0f,          31.0f / 32.0f,
        -31.0f / 32.0f, 31.0f / 32.0f,
        -31.0f / 32.0f, 1.0f,
        -1.0f,          1.0f
    };

    uint8_t tileVertexIndices[] = {
        0, 1, 2,
        2, 3, 0
    };

    unsigned int vertexArr;
    glGenVertexArrays(1, &vertexArr);
    glBindVertexArray(vertexArr);

    unsigned int arrBuf;
    glGenBuffers(1, &arrBuf);
    glBindBuffer(GL_ARRAY_BUFFER, arrBuf);
    glBufferData(GL_ARRAY_BUFFER, sizeof tileVertices, tileVertices, GL_STATIC_DRAW);

    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 2 * sizeof (GLfloat), 0);
    glEnableVertexAttribArray(0);

    unsigned int indexBuf;
    glGenBuffers(1, &indexBuf);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, indexBuf);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof tileVertexIndices, tileVertexIndices, GL_STATIC_DRAW);

    const char* vShaderSource = "\
    #version 330 core\n\
    layout(location = 0) in vec4 vertexPositions;\n\
    uniform vec2 uPosition;\n\
    void main() {gl_Position = vertexPositions + vec4(uPosition.x / 32, uPosition.y / -32, 0, 0);}";

    const char* fShaderSource = "\
    #version 330 core\n\
    layout(location = 0) out vec4 colour;\n\
    uniform vec4 uColour;\n\
    void main() {colour = uColour;}";

    unsigned int program = createProgram(vShaderSource, fShaderSource);
    glUseProgram(program);

    colourUniform = glGetUniformLocation(program, "uColour");
    if (colourUniform == -1) {
        printf("Couldn't find uniform uColour\n");
        glfwTerminate();
        exit(1);
    }

    positionUniform = glGetUniformLocation(program, "uPosition");
    if (positionUniform == -1) {
        printf("Couldn't find uniform uPosition\n");
        glfwTerminate();
        exit(1);
    }

    glClear(GL_COLOR_BUFFER_BIT);
    double prevPoll = glfwGetTime();
    glFinish(); // Flush the glClear() - Is this necessary?

    // Main loop
    while (!glfwWindowShouldClose(window)) {
        // TODO most instructions aren't setting flags
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
            AC += mem[PC + 1] + carryFlag;
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
            PC += 2;
            break;

            case 0xa2: // LDX #
            X = mem[PC + 1];
            PC += 2;
            break;

            case 0xa5: // LDA zp
            AC = mem[mem[PC + 1]];
            PC += 2;
            break;

            case 0xa6: // LDX zp
            X = mem[mem[PC + 1]];
            PC += 2;
            break;

            case 0xa9: // LDA #
            AC = mem[PC + 1];
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
            PC += 2;
            break;

            case 0xe8: // INX
            X++;
            PC++;
            break;

            default:
            printf("Invalid opcode at 0x%.4x: 0x%.2x\n", PC, mem[PC]);
            glfwTerminate();
            exit(1);
        }

        const double newTime = glfwGetTime();
        if (newTime - prevPoll > 0.001) {
            glfwPollEvents();
            prevPoll = newTime;
            if (drawQueued) {
                glFinish();
                drawQueued = false;
            }
        }
    }

    glDeleteProgram(program);

    glfwTerminate();
    return 0;
}