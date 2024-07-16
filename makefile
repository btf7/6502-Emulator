CC = gcc
CCWARNINGS = -Wall -Wextra -pedantic -Wmissing-prototypes -Wstrict-prototypes -Wredundant-decls -Wshadow
INCLUDEDIR = -IDependencies
LINKDIRS = -LDependencies\GLFW -LDependencies\GLEW
LIBS = -lglfw3 -lglew32 -lgdi32 -lopengl32
DEBUGFLAGS = -std=c17 -O0 -g3 $(CCWARNINGS)
RELEASEFLAGS = -std=c17 -O3 -g0 $(CCWARNINGS)

# Note - If on windows, and either mkdir or rm isn't found, make sure Git\usr\bin is in PATH and restart terminal if neccessary

dev: build emulatordev

release: releasebuild emulatorrelease

clean:
	-rm -r build
	-rm -r releasebuild
	-rm emulator.exe

build:
	mkdir build

emulatordev: build/main.o build/emulate.o build/display.o build/helper.o
	gcc $^ -o emulator $(DEBUGFLAGS) $(LINKDIRS) $(LIBS)

build/main.o: src/main.c src/emulate.h src/display.h src/helper.h
	gcc -c $< -o $@ $(DEBUGFLAGS) $(INCLUDEDIR)

build/emulate.o: src/emulate.c src/emulate.h src/helper.h
	gcc -c $< -o $@ $(DEBUGFLAGS) $(INCLUDEDIR)

build/display.o: src/display.c src/display.h
	gcc -c $< -o $@ $(DEBUGFLAGS) $(INCLUDEDIR) -DGLEW_STATIC

build/helper.o: src/helper.c src/emulate.h src/display.h src/helper.h
	gcc -c $< -o $@ $(DEBUGFLAGS) $(INCLUDEDIR) -DGLEW_STATIC

releasebuild:
	mkdir releasebuild

emulatorrelease: releasebuild/main.o releasebuild/emulate.o releasebuild/display.o releasebuild/helper.o
	gcc $^ -o emulator $(RELEASEFLAGS) $(LINKDIRS) $(LIBS)

releasebuild/main.o: src/main.c src/emulate.h src/display.h src/helper.h
	gcc -c $< -o $@ $(RELEASEFLAGS) $(INCLUDEDIR)

releasebuild/emulate.o: src/emulate.c src/emulate.h src/helper.h
	gcc -c $< -o $@ $(RELEASEFLAGS) $(INCLUDEDIR)

releasebuild/display.o: src/display.c src/display.h
	gcc -c $< -o $@ $(RELEASEFLAGS) $(INCLUDEDIR) -DGLEW_STATIC

releasebuild/helper.o: src/helper.c src/emulate.h src/display.h src/helper.h
	gcc -c $< -o $@ $(RELEASEFLAGS) $(INCLUDEDIR) -DGLEW_STATIC