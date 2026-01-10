# Compiler
CXX = g++
CXX_FLAGS = -g -Wall -Wextra

# Linker flags
LIBRARIES = -lsfml-graphics -lsfml-window -lsfml-system -lsfml-audio -lX11 -l Xext
##LDFLAGS_LINUX = -lX11 -lXext

# Source files
SRC = src
INCLUDE = include
BIN = bin
LIB = lib
ASSETS = assets
TARGET = $(BIN)/lofi-buddy

# Build rules

.PHONY: all clean

all: $(TARGET)

# Build executable
$(TARGET): $(SRC)/*.cpp
	$(CXX) $(CXX_FLAGS) -I$(INCLUDE) -I$(LIB) -L$(LIB) $(LDFLAGS) $^ -o $@ $(LIBRARIES)  
	cp $(ASSETS)/* $(BIN)

linux:
	$(CXX) $(SRC) -o $(TARGET) $(LDFLAGS) $(LDFLAGS_LINUX)

windows:
	$(CXX) $(SRC) -o $(TARGET) $(LDFLAGS)

clean:
	rm -rf $(BIN)/*

-include $(DEPS)

