# Virtual Memory Visualization Tool - C Backend
# Makefile for building the CLI tool and library

CC = gcc
CFLAGS = -Wall -Wextra -g -O2
LDFLAGS = 

# Directories
SRCDIR = src
OBJDIR = obj
BINDIR = bin

# Source files
SRCS = $(wildcard $(SRCDIR)/*.c)
OBJS = $(patsubst $(SRCDIR)/%.c,$(OBJDIR)/%.o,$(SRCS))

# Main executable
TARGET = $(BINDIR)/vmem_shell

# Object files excluding main (for potential library use)
LIB_OBJS = $(filter-out $(OBJDIR)/vmem_shell.o,$(OBJS))

.PHONY: all clean debug release

all: directories $(TARGET)

# Create output directories
directories:
	@mkdir -p $(OBJDIR) $(BINDIR)

# Link the executable
$(TARGET): $(OBJS)
	$(CC) $(OBJS) -o $@ $(LDFLAGS)
	@echo "Build complete: $@"

# Compile source files
$(OBJDIR)/%.o: $(SRCDIR)/%.c
	$(CC) $(CFLAGS) -c $< -o $@

# Debug build with extra debugging info
debug: CFLAGS += -DDEBUG -g3
debug: all

# Release build with optimizations
release: CFLAGS += -O3 -DNDEBUG
release: all

# Clean build artifacts
clean:
	rm -rf $(OBJDIR) $(BINDIR)
	@echo "Clean complete"

# Run the shell (requires sudo for full functionality)
run: all
	@echo "Note: Run with sudo for full pagemap access"
	$(TARGET)

# Run with sudo
sudo-run: all
	sudo $(TARGET)

# Install to /usr/local/bin
install: all
	sudo cp $(TARGET) /usr/local/bin/vmem_shell
	@echo "Installed to /usr/local/bin/vmem_shell"

# Uninstall
uninstall:
	sudo rm -f /usr/local/bin/vmem_shell
	@echo "Uninstalled"

# Show help
help:
	@echo "Virtual Memory Visualization Tool - Build System"
	@echo ""
	@echo "Targets:"
	@echo "  all       - Build the project (default)"
	@echo "  debug     - Build with debugging symbols"
	@echo "  release   - Build with optimizations"
	@echo "  clean     - Remove build artifacts"
	@echo "  run       - Build and run the shell"
	@echo "  sudo-run  - Build and run with root privileges"
	@echo "  install   - Install to /usr/local/bin"
	@echo "  uninstall - Remove from /usr/local/bin"
	@echo "  help      - Show this help message"
