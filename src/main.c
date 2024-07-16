#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <GLFW/glfw3.h>
#include "emulate.h"
#include "display.h"
#include "helper.h"

int main(int argc, char** argv) {
    if (argc != 2) {
        printf("Expected 1 input parameter, got %d\n", argc - 1);
        exit(1);
    }

    // Initialise
    readFile(argv[1]);
    PC = readWord(0xfffc);

    GLFWwindow* window = initDisplay();

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