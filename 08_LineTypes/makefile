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

SRC = main.cpp Flowfield4D.cpp
OBJ = $(SRC:.cpp=.o)
TARGET = flow

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
	em++ $(SRC) ../Utils/GLEnv.cpp ../Utils/GLApp.cpp ../Utils/GLDebug.cpp  ../Utils/ArcBall.cpp ../Utils/GLProgram.cpp ../Utils/GLArray.cpp ../Utils/GLBuffer.cpp ../Utils/GLTexture2D.cpp ../Utils/Tesselation.cpp ../Utils/Rand.cpp -o Solution.html -s ALLOW_MEMORY_GROWTH=1 -D__EMSCRIPTEN__=1 -s USE_WEBGL2=1 -s USE_GLFW=3 -s FULL_ES3=1 -O2 -I. -I../Utils
