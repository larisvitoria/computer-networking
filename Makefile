# ─────────────────────────────────────────────────────────────
# Compiler and project configuration
# ─────────────────────────────────────────────────────────────

CXX      := g++
TARGET   := pici_hunt
SRC      := main.cpp

# C++ standard
STD      := -std=c++17

# Compiler flags
WARNINGS      := -Wall -Wextra
RELEASE_FLAGS := -O2
DEBUG_FLAGS   := -O0 -g -DDEV_MODE

# SFML libraries
LIBS := -lsfml-graphics -lsfml-window -lsfml-system -lsfml-audio


# ─────────────────────────────────────────────────────────────
# Default target: release build
# ─────────────────────────────────────────────────────────────

all: release


# ─────────────────────────────────────────────────────────────
# Release build
# Produces the final optimized executable
# ─────────────────────────────────────────────────────────────

release:
	$(CXX) $(STD) $(RELEASE_FLAGS) $(WARNINGS) $(SRC) -o $(TARGET) $(LIBS)


# ─────────────────────────────────────────────────────────────
# Development build
# Enables debug symbols and DEV_MODE
# DEV_MODE allows loopback connections for local testing
# ─────────────────────────────────────────────────────────────

dev:
	$(CXX) $(STD) $(DEBUG_FLAGS) $(WARNINGS) $(SRC) -o $(TARGET)_dev $(LIBS)
	@echo "✓ Development build created: ./$(TARGET)_dev"
	@echo "  Loopback 127.0.0.1 is enabled in DEV_MODE"
	@echo ""
	@echo "  Terminal 1: ./$(TARGET)_dev → H"
	@echo "  Terminal 2: ./$(TARGET)_dev → C → 127.0.0.1"


# ─────────────────────────────────────────────────────────────
# Help menu
# Shows available commands and how to run the game
# ─────────────────────────────────────────────────────────────

help:
	@echo ""
	@echo "Pici Hunt - Makefile Help"
	@echo "─────────────────────────"
	@echo ""
	@echo "Build commands:"
	@echo "  make          Build the release version"
	@echo "  make release  Build the optimized release version"
	@echo "  make dev      Build the development version with DEV_MODE enabled"
	@echo "  make clean    Remove generated binaries"
	@echo "  make help     Show this help message"
	@echo ""
	@echo "How to run:"
	@echo "  Release:"
	@echo "    ./$(TARGET)"
	@echo ""
	@echo "  Development mode:"
	@echo "    ./$(TARGET)_dev"
	@echo ""


# ─────────────────────────────────────────────────────────────
# Remove generated binaries
# ─────────────────────────────────────────────────────────────

clean:
	rm -f $(TARGET) $(TARGET)_dev


# ─────────────────────────────────────────────────────────────
# Phony targets
# These targets are not files
# ─────────────────────────────────────────────────────────────

.PHONY: all release dev clean help