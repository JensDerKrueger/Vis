CC=g++
OSTYPE := $(shell uname)

ifeq ($(OSTYPE),Linux)
	CFLAGS=-c -Wall -std=c++17 -Wunreachable-code
	LFLAGS=-lglfw -lGLEW -lGL -L../Utils -lutils
	LIBS=
	INCLUDES=-I. -I../Utils 
else
	CFLAGS=-c -Wall -std=c++17 -Wunreachable-code -Xclang
	LFLAGS=-lglfw -lGLEW -framework OpenGL -L../Utils -lutils
	LIBS=-L /opt/homebrew/lib
	INCLUDES=-I. -I../Utils -I /opt/homebrew/include
endif

SRC = main.cpp MS.cpp
OBJ = $(SRC:.cpp=.o)
TARGET = ms

all: $(TARGET)

release: CFLAGS += -O3 -DNDEBUG
release: $(TARGET)

../Utils/libutils.a:
	cd ../Utils && make $(MAKECMDGOALS)

$(TARGET): $(OBJ) ../Utils/libutils.a
	$(CC) $(INCLUDES) $^ $(LFLAGS) $(LIBS) -o $@

%.o: %.cpp
	$(CC) $(CFLAGS) $(INCLUDES) $^ -o $@

clean:
	-rm -rf $(OBJ) $(TARGET) core Solution.html Solution.wasm Solution.data Solution.js

mrproper: clean
	cd ../Utils && make clean

emscripten:
	em++ $(SRC) ../Utils/bmp.cpp ../Utils/GLApp.cpp ../Utils/GLArray.cpp ../Utils/GLBuffer.cpp ../Utils/GLDebug.cpp ../Utils/GLEnv.cpp ../Utils/GLProgram.cpp ../Utils/GLTexture2D.cpp ../Utils/Grid2D.cpp ../Utils/Image.cpp   -o Solution.html -s ALLOW_MEMORY_GROWTH=1 -D__EMSCRIPTEN__=1 -s USE_WEBGL2=1 -s USE_GLFW=3 -s FULL_ES3=1 -O2 -I. -I../Utils --preload-file image.bmp --preload-file image_small.bmp
