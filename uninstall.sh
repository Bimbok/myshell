#!/bin/bash

# MyShell Uninstallation Script - Fixed login shell check

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
    local full_path="$INSTALL_DIR/$SHELL_NAME"
    if grep -q "^$full_path$" /etc/shells; then # Added anchors for exact match
        # Create backup and remove
        cp /etc/shells "/etc/shells.backup.$(date +%Y%m%d_%H%M%S)" # More precise backup name
        grep -v "^$full_path$" /etc/shells > /etc/shells.tmp # Added anchors for exact match
        mv /etc/shells.tmp /etc/shells
        print_success "Removed MyShell from /etc/shells"
    else
        print_info "MyShell not found in /etc/shells"
    fi
}

# Check if the *original* user is using myshell as login shell
check_current_shell() {
    # Check the shell for the user who called sudo (if any)
    local target_user="${SUDO_USER:-$(whoami)}" # Use SUDO_USER if available, otherwise current user
    local login_shell=$(getent passwd "$target_user" | cut -d: -f7) # Get the 7th field (shell) from passwd entry
    local myshell_path="$INSTALL_DIR/$SHELL_NAME"

    if [[ "$login_shell" == "$myshell_path" ]]; then
        print_warning "⚠️  WARNING: The user '$target_user' is currently using MyShell as their login shell!"
        print_warning "This script will remove the binary, which will prevent '$target_user' from logging in."
        echo ""
        print_info "To change back to a default shell (e.g., bash), '$target_user' should run:"
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
    check_current_shell # Check the actual user's login shell
    
    print_info "Starting MyShell uninstallation..."
    
    # Remove binary
    local full_path="$INSTALL_DIR/$SHELL_NAME"
    if [[ -f "$full_path" ]]; then
        rm -f "$full_path"
        print_success "Removed $full_path"
    else
        print_warning "MyShell binary not found at $full_path"
    fi
    
    # Remove from /etc/shells
    remove_from_shells
    
    # Inform about history files
    echo ""
    print_info "Note: User history files (~/.myshell_history) were not removed."
    print_info "To remove your personal history file, run:"
    print_info "  rm -f ~/.myshell_history"
    
    echo ""
    print_success "🎉 MyShell uninstalled successfully!"
    
    # Final warning if shell was changed (using the same robust check)
    local target_user="${SUDO_USER:-$(whoami)}"
    local login_shell=$(getent passwd "$target_user" | cut -d: -f7)
    local myshell_path="$INSTALL_DIR/$SHELL_NAME"
    
    if [[ "$login_shell" == "$myshell_path" ]]; then
        echo ""
        print_warning "⚠️  IMPORTANT: The user '$target_user's login shell is still set to the old MyShell path!"
        print_warning "The user won't be able to login until the shell is changed."
        echo ""
        print_info "To change back, the user '$target_user' (or root) should run:"
        print_info "  chsh -s /bin/bash $target_user"
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
    echo "  - Preserve user history files (optional manual removal)"
    echo ""
    echo "Note: If MyShell is your current login shell, you will be warned."
    echo "      Change it back before uninstalling to avoid login issues:"
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