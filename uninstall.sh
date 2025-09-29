#!/bin/bash

# MyShell Uninstallation Script

set -e

SHELL_NAME="myshell"
INSTALL_DIR="/usr/local/bin"

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
        print_error "This script requires sudo privileges to uninstall from $INSTALL_DIR"
        print_info "Please run with: sudo $0"
        exit 1
    fi
}

# Remove from /etc/shells
remove_from_shells() {
    if grep -q "$INSTALL_DIR/$SHELL_NAME" /etc/shells; then
        # Create backup and remove
        cp /etc/shells /etc/shells.backup.$(date +%Y%m%d)
        grep -v "$INSTALL_DIR/$SHELL_NAME" /etc/shells > /etc/shells.tmp
        mv /etc/shells.tmp /etc/shells
        print_success "Removed MyShell from /etc/shells"
    else
        print_info "MyShell not found in /etc/shells"
    fi
}

# Check if user is using myshell as current shell
check_current_shell() {
    local current_shell=$(getent passwd $(whoami) | cut -d: -f7)
    if [[ "$current_shell" == "$INSTALL_DIR/$SHELL_NAME" ]]; then
        print_warning "⚠️  WARNING: You are currently using MyShell as your login shell!"
        print_warning "Changing your shell back to bash before uninstallation is recommended."
        echo ""
        print_info "To change back to bash, run:"
        print_info "  chsh -s /bin/bash"
        echo ""
        read -p "Do you want to continue with uninstallation? (y/N): " -n 1 -r
        echo
        if [[ ! $REPLY =~ ^[Yy]$ ]]; then
            print_info "Uninstallation cancelled"
            exit 0
        fi
    fi
}

# Main uninstallation
main() {
    echo "=========================================="
    echo "    MyShell Uninstallation Script"
    echo "=========================================="
    echo ""
    
    check_root
    check_current_shell
    
    print_info "Starting MyShell uninstallation..."
    
    # Remove binary
    if [[ -f "$INSTALL_DIR/$SHELL_NAME" ]]; then
        rm -f "$INSTALL_DIR/$SHELL_NAME"
        print_success "Removed $INSTALL_DIR/$SHELL_NAME"
    else
        print_warning "MyShell not found in $INSTALL_DIR"
    fi
    
    # Remove from /etc/shells
    remove_from_shells
    
    # Inform about history files
    echo ""
    print_info "Note: User history files (~/.myshell_history) were not removed"
    print_info "To remove your personal history file, run:"
    print_info "  rm -f ~/.myshell_history"
    
    echo ""
    print_success "🎉 MyShell uninstalled successfully!"
    
    # Final warning if shell was changed
    local current_shell=$(getent passwd $(whoami) | cut -d: -f7)
    if [[ "$current_shell" == "$INSTALL_DIR/$SHELL_NAME" ]]; then
        echo ""
        print_warning "⚠️  IMPORTANT: Your login shell is still set to MyShell!"
        print_warning "You won't be able to login until you change it back."
        echo ""
        print_info "To change back to bash, run:"
        print_info "  chsh -s /bin/bash"
    fi
}

# Show usage
show_usage() {
    echo "MyShell Uninstaller"
    echo ""
    echo "Usage: sudo $0"
    echo ""
    echo "This will:"
    echo "  - Remove MyShell binary from $INSTALL_DIR"
    echo "  - Remove MyShell from /etc/shells"
    echo "  - Preserve your history files (optional manual removal)"
    echo ""
    echo "Note: If MyShell is your current login shell,"
    echo "      change it back before uninstalling:"
    echo "      chsh -s /bin/bash"
}

# Parse arguments
case "${1:-}" in
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