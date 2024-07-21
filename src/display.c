#include <stdio.h>
#include <stdlib.h>
#include <GLEW/glew.h>
#include <GLFW/glfw3.h>
#include "display.h"

static const char * const vShaderSource = "\
#version 330 core\n\
layout(location = 0) in vec4 vertexPositions;\n\
uniform vec2 uPosition;\n\
void main() {\n\
gl_Position = vertexPositions + vec4(uPosition.x / 32, uPosition.y / -32, 0, 0);\n\
}";

static const char * const fShaderSource = "\
#version 330 core\n\
layout(location = 0) out vec4 colour;\n\
uniform vec4 uColour;\n\
void main() {\n\
colour = uColour;\n\
}";

int colourUniform;
int positionUniform;
GLFWwindow* window;

static unsigned int compileShader(const char * const string, const unsigned int type) {
    const unsigned int id = glCreateShader(type);
    glShaderSource(id, 1, &string, NULL);
    glCompileShader(id);

    int result;
    glGetShaderiv(id, GL_COMPILE_STATUS, &result);
    if (result == GL_FALSE) {
        int len;
        glGetShaderiv(id, GL_INFO_LOG_LENGTH, &len);

        char * const msg = malloc(len);
        if (!msg) {
            printf("malloc() failed\n");
            glfwTerminate();
            exit(1);
        }
        glGetShaderInfoLog(id, len, NULL, msg);

        printf("Failed to compile shader:\n");
        printf("%s\n", msg);

        free(msg);
        glDeleteShader(id);
        glfwTerminate();
        exit(1);
    }

    return id;
}

static unsigned int createProgram(void) {
    const unsigned int program = glCreateProgram();
    const unsigned int vShader = compileShader(vShaderSource, GL_VERTEX_SHADER);
    const unsigned int fShader = compileShader(fShaderSource, GL_FRAGMENT_SHADER);
    
    glAttachShader(program, vShader);
    glAttachShader(program, fShader);
    glLinkProgram(program);
    glValidateProgram(program); // Is this needed?

    glDetachShader(program, vShader);
    glDetachShader(program, fShader);
    glDeleteShader(vShader);
    glDeleteShader(fShader);

    return program;
}

void initDisplay(void) {
    // Initialise GLFW and GLEW

    if (!glfwInit()){
        printf("Failed to initialise GLFW\n");
        exit(1);
    }

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);

    // The emulator will only ever output RRRGGGBB colours
    glfwWindowHint(GLFW_RED_BITS, 3);
    glfwWindowHint(GLFW_GREEN_BITS, 3);
    glfwWindowHint(GLFW_BLUE_BITS, 2);
    glfwWindowHint(GLFW_ALPHA_BITS, 0);

    // The display updates real-time(ish), don't bother with frame timings, just draw straight to the buffer
    glfwWindowHint(GLFW_DOUBLEBUFFER, GLFW_FALSE);

    window = glfwCreateWindow(640, 640, "6502 Emulator", NULL, NULL);
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

    // Setup vertex array

    unsigned int vertexArr;
    glGenVertexArrays(1, &vertexArr);
    glBindVertexArray(vertexArr);

    // Setup array buffer

    const float tileVertices[] = {
        -1.0f,          31.0f / 32.0f,
        -31.0f / 32.0f, 31.0f / 32.0f,
        -31.0f / 32.0f, 1.0f,
        -1.0f,          1.0f
    };

    unsigned int arrBuf;
    glGenBuffers(1, &arrBuf);
    glBindBuffer(GL_ARRAY_BUFFER, arrBuf);
    glBufferData(GL_ARRAY_BUFFER, sizeof tileVertices, tileVertices, GL_STATIC_DRAW);

    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 2 * sizeof (GLfloat), 0);
    glEnableVertexAttribArray(0);

    // Setup index buffer

    const uint8_t tileVertexIndices[] = {
        0, 1, 2,
        2, 3, 0
    };

    unsigned int indexBuf;
    glGenBuffers(1, &indexBuf);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, indexBuf);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof tileVertexIndices, tileVertexIndices, GL_STATIC_DRAW);

    // Setup shaders

    const unsigned int program = createProgram();
    glUseProgram(program);
    glDeleteProgram(program); // Mark for deletion once it's no longer being used

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
}