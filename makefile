CC = gcc
INCLUDEDIR = -IDependencies
LINKDIRS = -LDependencies\GLFW -LDependencies\GLEW
LIBS = -lglfw3 -lglew32 -lgdi32 -lopengl32
CCWARNINGS = -Wall -Wextra -pedantic -Wmissing-prototypes -Wstrict-prototypes -Wredundant-decls -Wshadow
CFLAGS = -std=c17 -MMD -MP -DGLEW_STATIC $(CCWARNINGS)
DEBUGFLAGS = -O0 -g3 $(CFLAGS)
RELEASEFLAGS = -O3 -g0 $(CFLAGS)

# Note - If on windows, and either mkdir or rm isn't found, make sure Git\usr\bin is in PATH and restart terminal if neccessary

debug: build emulatordebug

release: releasebuild emulatorrelease

clean:
	-rm -r build
	-rm -r releasebuild
	-rm emulator.exe

build:
	mkdir build

emulatordebug: build/main.o build/emulate.o build/display.o build/instructions.o
	$(CC) $^ -o emulator $(DEBUGFLAGS) $(LINKDIRS) $(LIBS)

build/%.o: src/%.c
	$(CC) -c $< -o $@ $(DEBUGFLAGS) $(INCLUDEDIR)

-include $(wildcard build/*.d)

releasebuild:
	mkdir releasebuild

emulatorrelease: releasebuild/main.o releasebuild/emulate.o releasebuild/display.o releasebuild/instructions.o
	$(CC) $^ -o emulator $(RELEASEFLAGS) $(LINKDIRS) $(LIBS)

releasebuild/%.o: src/%.c
	$(CC) -c $< -o $@ $(RELEASEFLAGS) $(INCLUDEDIR)

-include $(wildcard releasebuild/*.d)