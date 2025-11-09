# Gamba Card Game Client - Makefile
# Cross-platform support for Windows and Linux

# Detect which venv structure exists (Windows: Scripts/, Linux: bin/)
ifeq ($(wildcard .venv/Scripts/python.exe),)
    # Linux/Unix style venv
    PYTHON = .venv/bin/python3
    PIP = .venv/bin/pip3
else
    # Windows style venv (works on both Windows and WSL)
    PYTHON = .venv/Scripts/python.exe
    PIP = .venv/Scripts/pip.exe
endif

# Detect OS for shell commands (clean uses different syntax on Windows vs Linux)
ifeq ($(OS),Windows_NT)
    # Windows cmd syntax
    CLEAN_PYCACHE = if exist src\__pycache__ rmdir /s /q src\__pycache__ & \
                    if exist src\gui\__pycache__ rmdir /s /q src\gui\__pycache__ & \
                    if exist src\game\__pycache__ rmdir /s /q src\game\__pycache__ & \
                    if exist src\network\__pycache__ rmdir /s /q src\network\__pycache__ & \
                    if exist src\message\__pycache__ rmdir /s /q src\message\__pycache__ & \
                    if exist src\utils\__pycache__ rmdir /s /q src\utils\__pycache__
else
    # Linux/Unix bash syntax
    CLEAN_PYCACHE = find . -type d -name __pycache__ -exec rm -rf {} + 2>/dev/null || true
endif

MAIN = main.py
REQUIREMENTS = requirements.txt

.PHONY: all build install run clean help

# Default target - build the client
all: build

# Build target - install dependencies
build:
	@echo "Building Gamba Client..."
	@echo "Installing dependencies..."
	@$(PIP) install -r $(REQUIREMENTS)
	@echo "Build complete!"

# Install dependencies only
install:
	@echo "Installing dependencies..."
	@$(PIP) install -r $(REQUIREMENTS)

# Run the client
run:
	@echo "Starting Gamba Client..."
	@$(PYTHON) $(MAIN)

# Clean generated files and caches
clean:
	@echo "Cleaning up..."
	@$(CLEAN_PYCACHE)
	@echo "Cleanup complete!"

# Show help
help:
	@echo "Gamba Client Makefile"
	@echo ""
	@echo "Available targets:"
	@echo "  make all     - Build the client"
	@echo "  make build   - Install dependencies"
	@echo "  make install - Install/update dependencies"
	@echo "  make run     - Run the client"
	@echo "  make clean   - Remove cache files"
	@echo "  make help    - Show this help message"
