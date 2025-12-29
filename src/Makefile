# Compiler
CXX = g++

# Linker flags
LDFLAGS = -lsfml-graphics -lsfml-window -lsfml-system -lsfml-audio
LDFLAGS_LINUX = -lX11 -lXext

# Source files
SRC = main.cpp

# Output executable
TARGET = lofi-buddy

# Build rules
linux:
	$(CXX) $(SRC) -o $(TARGET) $(LDFLAGS) $(LDFLAGS_LINUX)

windows:
	$(CXX) $(SRC) -o $(TARGET) $(LDFLAGS)

clean:
	rm -f $(TARGET)

