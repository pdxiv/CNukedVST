#!/usr/bin/make -f
# Makefile for OPL3 FM VST
# Designed for static compilation without external dependencies

# Plugin name
NAME = CNukedVST

# Output file name
TARGET = $(NAME).so

# Default compiler 
CXX ?= g++
CC ?= gcc

# Compiler flags
CXXFLAGS += -Wall -Wextra -Wno-unused-parameter -fpermissive
CXXFLAGS += -std=c++11 -fvisibility=hidden
CXXFLAGS += -O3 -ffast-math -mtune=generic -msse -msse2
CXXFLAGS += -fdata-sections -ffunction-sections
# Static compilation flags
CXXFLAGS += -static-libgcc -static-libstdc++

# C compiler flags
CFLAGS += -Wall -Wextra -Wno-unused-parameter
CFLAGS += -O3 -ffast-math -mtune=generic -msse -msse2
CFLAGS += -fdata-sections -ffunction-sections

# Additional flags for building shared library
LDFLAGS += -shared
LDFLAGS += -Wl,--no-undefined
LDFLAGS += -Wl,--strip-all
LDFLAGS += -Wl,--gc-sections
LDFLAGS += -fPIC

# Compiler defines
DEFINES = -D__cdecl="" -DNDEBUG

# Source files
SOURCES = CNukedVST.cpp
C_SOURCES = opl3.c

# Object files
OBJECTS = $(SOURCES:.cpp=.o) $(C_SOURCES:.c=.o)

# Rules
all: $(TARGET)

# Compile rule for C++
%.o: %.cpp
	@echo "Compiling $<..."
	@$(CXX) $(CXXFLAGS) $(DEFINES) -fPIC -c $< -o $@

# Compile rule for C
%.o: %.c
	@echo "Compiling $<..."
	@$(CC) $(CFLAGS) $(DEFINES) -fPIC -c $< -o $@

# Link rule
$(TARGET): $(OBJECTS)
	@echo "Linking $@..."
	@$(CXX) $(LDFLAGS) $(OBJECTS) -o $@
	@echo "Build complete!"

# VST install directories
VST_SYSTEM_DIR = /usr/lib/vst
VST_USER_DIR = $(HOME)/.vst

# Install rule - installs to user's .vst directory
install: $(TARGET)
	@echo "Installing to $(VST_USER_DIR)..."
	@mkdir -p $(VST_USER_DIR)
	@cp $(TARGET) $(VST_USER_DIR)/
	@echo "Installation complete!"

# Clean rule
clean:
	@echo "Cleaning..."
	@rm -f $(OBJECTS) $(TARGET)
	@echo "Clean complete!"

# Check static linking
check-static: $(TARGET)
	@echo "Checking static linking..."
	@ldd $(TARGET)

# Default target
.PHONY: all clean install check-static
