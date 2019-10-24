
GL_LIBS = `pkg-config --static --libs glfw3` -lGLEW 
EXT = 
CPPFLAGS = `pkg-config --cflags glfw3`

CC = g++
EXE = assign3_part2
OBJS = main.o shader.o Viewer.o Physics.o

.PHONY: all clean

all: $(EXE)

$(EXE): $(OBJS)
	$(CC) -o $(EXE) $(OBJS) $(GL_LIBS)

main.o: main.cpp InputState.h 	
	$(CC) $(CPPFLAGS) -c main.cpp

shader.o : shader.cpp shader.hpp
	$(CC) $(CPPFLAGS) -c shader.cpp

Viewer.o: Viewer.h Viewer.cpp InputState.h
	$(CC) $(CPPFLAGS) -c Viewer.cpp

Physics.o: Physics.h Physics.cpp InputState.h
	$(CC) $(CPPFLAGS) -c Physics.cpp

clean:
	rm -f *.o $(EXE)$(EXT)
