#!/bin/bash

# MyShell Installation Script - Fixed to compile from source

set -e

SHELL_NAME="myshell"
INSTALL_DIR="/usr/local/bin"
REPO_URL="https://github.com/Bimbok/myshell.git"
TEMP_DIR="/tmp/myshell-install"
SOURCE_FILE="myshell.c" # Added source file name

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

# Helper functions
print_info() {
    echo -e "${BLUE}[INFO]${NC} $1"
}

print_success() {
    echo -e "${GREEN}[SUCCESS]${NC} $1"
}

print_warning() {
    echo -e "${YELLOW}[WARNING]${NC} $1"
}

print_error() {
    echo -e "${RED}[ERROR]${NC} $1"
}

# Check if running as root
check_root() {
    if [[ $EUID -eq 0 ]]; then
        return 0
    else
        print_error "This script requires sudo privileges to install to $INSTALL_DIR"
        print_info "Please run with: sudo $0"
        exit 1
    fi
}

# Compile from source (Replaced the flawed download function)
compile_and_install() {
    print_info "Cloning repository from GitHub..."
    
    # Check for git
    if ! command -v git &> /dev/null; then
        print_error "git is not available. Please install git to continue."
        exit 1
    fi
    
    # Create and enter temporary directory
    rm -rf "$TEMP_DIR"
    mkdir -p "$TEMP_DIR"
    cd "$TEMP_DIR"
    
    git clone --depth 1 "$REPO_URL" . # Use --depth 1 to speed up clone
    
    # Check if source file exists
    if [[ ! -f "$SOURCE_FILE" ]]; then
        print_error "Source file $SOURCE_FILE not found in repository"
        exit 1
    fi
    
    print_info "Compiling MyShell..."
    
    # Check for GCC
    if ! command -v gcc &> /dev/null; then
        print_error "GCC compiler not found. Please install gcc (e.g., 'sudo apt install build-essential')."
        exit 1
    fi
    
    # Compile the shell with necessary libraries (e.g., -lm for math if used, -lncurses for better TUI, etc. - based on myshell.c, none needed but good practice)
    # The provided myshell.c only needs standard libs
    gcc -o "$SHELL_NAME" "$SOURCE_FILE" -O2 -Wall -Wextra
    
    if [[ $? -ne 0 ]]; then
        print_error "Compilation failed!"
        exit 1
    fi
    
    print_success "Compilation completed successfully"
    
    # Install to /usr/local/bin
    print_info "Installing to $INSTALL_DIR..."
    cp "$SHELL_NAME" "$INSTALL_DIR/"
    chmod +x "$INSTALL_DIR/$SHELL_NAME"
    
    # Clean up
    rm -rf "$TEMP_DIR"
    
    print_success "MyShell installed successfully to $INSTALL_DIR/$SHELL_NAME"
}

# Add to /etc/shells
add_to_shells() {
    local full_path="$INSTALL_DIR/$SHELL_NAME"
    if ! grep -q "^$full_path$" /etc/shells; then # Added anchors for exact match
        echo "$full_path" >> /etc/shells
        print_success "Added MyShell to /etc/shells"
    else
        print_info "MyShell already in /etc/shells"
    fi
}

# Main installation
main() {
    echo "=========================================="
    echo "    MyShell Installation Script"
    echo "=========================================="
    echo ""
    
    check_root
    
    print_info "Starting MyShell installation..."
    compile_and_install # Calls the new compile and install function
 
    echo ""
    print_success "🎉 Installation completed!"
    echo ""
    
    # Test the installation
    if command -v "$SHELL_NAME" &> /dev/null; then
        print_success "MyShell is now available at: $INSTALL_DIR/$SHELL_NAME"
    else
        print_warning "Installation complete but the command may not be immediately available."
        print_info "Run: source ~/.bashrc or restart your terminal"
    fi
    
    echo ""
    print_info "Quick start:"
    print_info "  Run MyShell: $SHELL_NAME"
    print_info "  Setup for login shell: sudo $0 --set-login"
    echo ""
    print_info "For more info: $SHELL_NAME then type 'help'"
}

# Set as login shell
set_login_shell() {
    check_root
    print_info "Setting up MyShell as login shell..."
    add_to_shells
    echo ""
    print_success "MyShell added to /etc/shells"
    echo ""
    print_info "To set as your default shell, run:"
    print_info "  chsh -s $INSTALL_DIR/$SHELL_NAME"
    echo ""
    print_info "Or for a specific user:"
    print_info "  sudo chsh -s $INSTALL_DIR/$SHELL_NAME username"
}

# Show usage
show_usage() {
    echo "MyShell Quick Installer"
    echo ""
    echo "Usage: $0 [option]"
    echo ""
    echo "Options:"
    echo "  --set-login    Add MyShell to /etc/shells for login shell usage"
    echo "  --help         Show this help message"
    echo ""
    echo "Examples:"
    echo "  sudo $0           # Install MyShell (compiles from source)"
    echo "  sudo $0 --set-login # Setup for login shell"
    echo ""
    echo "After installation, run: myshell"
}

# Parse arguments
case "${1:-}" in
    "--set-login")
        set_login_shell
        ;;
    "--help"|"-h")
        show_usage
        ;;
    "")
        main
        ;;
    *)
        print_error "Unknown option: $1"
        echo ""
        show_usage
        exit 1
        ;;
esac