CC = gcc
CFLAGS = -std=c17 -Wall -Wextra -pedantic -O0 -g3 -DGLEW_STATIC -IDependencies -LDependencies\GLFW -LDependencies\GLEW -lglfw3 -lglew32 -lgdi32 -lopengl32
CFILES = src/main.c
#HFILES = src/main.h

emulator: $(CFILES)# $(HFILES)
	$(CC) $(CFILES) -o emulator $(CFLAGS)