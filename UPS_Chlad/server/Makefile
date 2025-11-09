# Makefile for Gamba Game Server
# Compiler settings
CXX = g++
CXXFLAGS = -std=c++17 -Wall -Wextra -pthread -O2
INCLUDES = -Iinclude -Iinclude/core -Iinclude/game -Iinclude/network -Iinclude/protocol -Isrc
LDFLAGS = -pthread

# Directories
SRC_DIR = src
BUILD_DIR = build
INCLUDE_DIR = include

# Target executable
TARGET = gamba_server

# Source files - automatically find all .cpp files in src/
SOURCES = $(shell find $(SRC_DIR) -name '*.cpp')

# Object files - convert src/path/file.cpp to build/path/file.o
OBJECTS = $(patsubst $(SRC_DIR)/%.cpp,$(BUILD_DIR)/%.o,$(SOURCES))

# Dependency files
DEPS = $(OBJECTS:.o=.d)

# Default target
all: $(TARGET)

# Link the executable
$(TARGET): $(OBJECTS)
	@echo "Linking $(TARGET)..."
	$(CXX) $(OBJECTS) -o $(TARGET) $(LDFLAGS)
	@echo "Build complete: $(TARGET)"

# Compile source files to object files
$(BUILD_DIR)/%.o: $(SRC_DIR)/%.cpp
	@mkdir -p $(dir $@)
	@echo "Compiling $<..."
	$(CXX) $(CXXFLAGS) $(INCLUDES) -MMD -MP -c $< -o $@

# Include dependency files
-include $(DEPS)

# Clean build files
clean:
	@echo "Cleaning build files..."
	rm -rf $(BUILD_DIR)
	rm -f $(TARGET)
	@echo "Clean complete"

# Rebuild everything
rebuild: clean all

# Run the server
run: $(TARGET)
	./$(TARGET)

# Debug build
debug: CXXFLAGS += -g -DDEBUG -O0
debug: clean all

# Help target
help:
	@echo "Gamba Game Server Makefile"
	@echo "Available targets:"
	@echo "  all      - Build the server (default)"
	@echo "  clean    - Remove build files"
	@echo "  rebuild  - Clean and build"
	@echo "  run      - Build and run the server"
	@echo "  debug    - Build with debug symbols"
	@echo "  help     - Show this help message"

.PHONY: all clean rebuild run debug help
