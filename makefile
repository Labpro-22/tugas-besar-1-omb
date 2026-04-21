# Makefile for Nimonspoli — IF2010 Tugas Besar 1

# Compiler settings
CXX      := g++
CXXFLAGS := -Wall -Wextra -std=c++17 -I include

# Directories
SRC_DIR     := src
OBJ_DIR     := build
BIN_DIR     := bin
INCLUDE_DIR := include
DATA_DIR    := data
CONFIG_DIR  := config

# Target executable
TARGET := $(BIN_DIR)/nimonspoli

# Recursive source finding — automatically discovers all .cpp in src/
SRCS := $(shell find $(SRC_DIR) -name '*.cpp')

# Map src/xxx/yyy.cpp → build/xxx/yyy.o
OBJS := $(patsubst $(SRC_DIR)/%.cpp, $(OBJ_DIR)/%.o, $(SRCS))

# =============================================================================
# Targets
# =============================================================================

.PHONY: all clean rebuild run directories

all: directories $(TARGET)

# Create output directories
directories:
	@mkdir -p $(OBJ_DIR) $(BIN_DIR) $(DATA_DIR) $(CONFIG_DIR)

# Link object files into executable
$(TARGET): $(OBJS)
	$(CXX) $(CXXFLAGS) $^ -o $@
	@echo ""
	@echo "========================================="
	@echo "  Build successful!"
	@echo "  Executable: $(TARGET)"
	@echo "========================================="
	@echo ""

# Compile each .cpp to .o (auto-creates subdirectories)
$(OBJ_DIR)/%.o: $(SRC_DIR)/%.cpp
	@mkdir -p $(dir $@)
	$(CXX) $(CXXFLAGS) -c $< -o $@

# Run the game
run: all
	./$(TARGET)

# Clean build artifacts
clean:
	rm -rf $(OBJ_DIR) $(BIN_DIR)
	@echo "Cleaned $(OBJ_DIR)/ and $(BIN_DIR)/"

# Full rebuild
rebuild: clean all