#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
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

    glfwMakeContextCurrent(window);

    glClear(GL_COLOR_BUFFER_BIT);
    double prevDrawTime = glfwGetTime();
    glFinish(); // Flush the glClear() - Is this necessary?

    while (!glfwWindowShouldClose(window)) {
        runInstruction();

        const double nowTime = glfwGetTime();
        if (nowTime - prevDrawTime > 0.001) {
            prevDrawTime = nowTime;
            if (drawQueued) {
                glFinish();
                drawQueued = false;
            }
        }
    }

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

    glfwMakeContextCurrent(NULL);
    pthread_t emulateThread;
    pthread_create(&emulateThread, NULL, &emulate, NULL);

    while (!glfwWindowShouldClose(window)) {
        glfwPollEvents();
    }

    pthread_join(emulateThread, NULL);

    glfwTerminate();
    return 0;
}