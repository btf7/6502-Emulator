#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <GLFW/glfw3.h>
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
    double prevPollTime = glfwGetTime();
    glFinish(); // Flush the glClear() - Is this necessary?

    // Main loop
    while (!glfwWindowShouldClose(window)) {
        runInstruction();

        const double nowTime = glfwGetTime();
        if (nowTime - prevPollTime > 0.001) {
            glfwPollEvents();
            prevPollTime = nowTime;
            if (drawQueued) {
                glFinish();
                drawQueued = false;
            }
        }
    }

    glfwTerminate();
    return 0;
}