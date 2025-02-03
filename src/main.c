#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <GLEW/glew.h>
#include <GLFW/glfw3.h>
#include <pthread.h>
#include "emulate.h"
#include "display.h"
#include "instructions.h"

static void keyCallback(GLFWwindow* callbackWindow, int key, int scancode, int action, int mods) {
    // Compiler warns about unused parameters
    // Cast to void to ignore them
    (void)callbackWindow;
    (void)scancode;
    (void)mods;

    if (action == GLFW_PRESS) {
        mem[0xfff8] = key & 0xff;
        mem[0xfff9] = (key >> 8) & 0xff;
    }
}

static void* emulate(void* args) {
    (void)args;

    while (!glfwWindowShouldClose(window)) runInstruction();

    return NULL;
}

int main(int argc, char** argv) {
    if (argc != 2) {
        printf("Expected 1 input parameter, got %d\n", argc - 1);
        exit(1);
    }

    // Initialise
    readFile(argv[1]);
    PC = readWord(0xfffc);

    initDisplay();
    glfwSetKeyCallback(window, keyCallback);

    glClear(GL_COLOR_BUFFER_BIT);
    glfwSwapBuffers(window);

    // Windows freezes the main thread when the window is grabbed
    // Run the emulation on another thread to circumvent this
    pthread_t emulateThread;
    pthread_create(&emulateThread, NULL, &emulate, NULL);

    while (!glfwWindowShouldClose(window)) {
        // Render screen

        for (uint16_t i = 0xe000; i < 0xf000; i++) {
            const uint8_t byte = mem[i];
            glUniform4f(colourUniform, (float)((byte & 0xe0) >> 5) / 7.0f, (float)((byte & 0x1c) >> 2) / 7.0f, (float)(byte & 0x03) / 3.0f, 1.0f);
            glUniform2f(positionUniform, (float)((i - 0xe000) & 0x3f), (float)((i - 0xe000) >> 6));
            glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_BYTE, NULL);
        }

        glfwSwapBuffers(window);

        glfwPollEvents();
    }

    pthread_join(emulateThread, NULL);

    glfwTerminate();
    return 0;
}