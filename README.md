
# MyShell - A Feature-Rich Unix Shell

MyShell is a custom Unix-like command-line interpreter (shell) written in C. It supports standard command execution, command history, piping, input/output redirection, and includes advanced features like **Tab Completion** and a built-in **Arithmetic Evaluator**.

<img width="1854" height="1048" alt="Image" src="https://github.com/user-attachments/assets/85f5bbe8-776c-4218-88fd-c2954721403c" />

## 🌟 Features

MyShell is designed to be a powerful and user-friendly alternative shell, incorporating features typically found in modern shells:

### Core Shell Functionality

  * **Command Execution**: Executes external programs and system commands.
  * **Piping (`|`)**: Supports connecting the output of one command to the input of another.
  * **Input/Output Redirection (`<`, `>`)**: Allows redirecting command input from a file or output to a file.

### Advanced Interaction

  * **Tab Completion**: Provides auto-completion for both **external commands** (by searching the `$PATH`) and **local files/directories**.
      * Pressing **`TAB`** once will complete a single match or, if multiple matches exist, pressing **`TAB`** again will list all possibilities.
  * **Command History**: Stores and manages command history, allowing easy navigation2].
      * Use the **`UP`** and **`DOWN`** arrow keys to scroll through previously executed commands.
      * History is **persisted** between sessions by saving it to a file at `~/.myshell_history`.

### Arithmetic Evaluation

  * **Built-in Calculator**: MyShell can evaluate arithmetic expressions directly typed into the prompt.
      * Supports integers and floating-point numbers (printed to two decimal places if not a whole number).
      * Supported operators include **addition (`+`), subtraction (`-`), multiplication (`*`), division (`/`), modulo (`%`), and exponentiation (`^`)**.
      * Supports **parentheses** for defining order of operations35].

### Built-in Commands

MyShell includes its own set of essential built-in commands for core shell functionality:
| Command | Description | Source |
| :--- | :--- | :--- |
| **`cd <directory>`** | Changes the current working directory. | Built-in |
| **`exit`** | Closes MyShell and returns to the calling shell[cite: 12]. | Built-in |
| **`help`** | Displays this comprehensive help message listing all features and built-in commands[cite: 13, 14]. | Built-in |

-----

## ⚙️ Installation

The installation process compiles the MyShell source code from the GitHub repository and installs the resulting binary to `/usr/local/bin`. **You must have `git` and `gcc` installed.**

### Prerequisites

  * **`git`**: For cloning the repository.
  * **`gcc` (GNU Compiler Collection)**: For compiling the C source code.
  * **`sudo` privileges**: Required to install the binary to `/usr/local/bin`.

### Installation Steps (Using `install.sh`)

1.  **Download the Installation Script:**
    ```bash
    wget https://raw.githubusercontent.com/Bimbok/myshell/main/install.sh -O install.sh
    chmod +x install.sh
    ```
2.  **Run the Installer with `sudo`:**
    ```bash
    sudo ./install.sh
    ```
    This script will perform the following actions:
      * Check for `sudo` privileges.
      * Clone the source code from `https://github.com/Bimbok/myshell.git` into a temporary directory (`/tmp/myshell-install`).
      * Compile `myshell.c` using `gcc`.
      * Copy the compiled binary (`myshell`) to `/usr/local/bin`.
      * Clean up the temporary files.

### Post-Installation

After installation, you can run MyShell by simply typing:

```bash
myshell
```

-----

## 🔧 Uninstalling MyShell

The uninstallation script removes the binary and cleans up the system-wide configuration, but intentionally **preserves your personal history file** for safety.

### Uninstallation Steps (Using `uninstall.sh`)

1.  **Download the Uninstallation Script:**
    ```bash
    wget https://raw.githubusercontent.com/Bimbok/myshell/main/uninstall.sh -O uninstall.sh
    chmod +x uninstall.sh
    ```
2.  **Run the Uninstaller with `sudo`:**
    ```bash
    sudo ./uninstall.sh
    ```
    This script will perform the following actions:
      * Check for `sudo` privileges.
      * **Check Login Shell**: It checks if the user running the script is currently using MyShell as their login shell. If so, it issues a **critical warning** and prompts for confirmation to prevent the user from being locked out of their system.
      * Remove the MyShell binary from `/usr/local/bin/myshell`.
      * Remove the MyShell path (`/usr/local/bin/myshell`) from the system-wide `/etc/shells` file, creating a backup first.
      * Inform the user that the personal history file (`~/.myshell_history`) was **not** removed.

### Manual Cleanup

If you wish to completely remove all traces of MyShell, manually delete the history file:

```bash
rm -f ~/.myshell_history
```

-----

## 🛠️ Installation Parameters (`install.sh`)

The `install.sh` script supports a specific parameter for system configuration:

| Parameter | What it Does |
| :--- | :--- |
| **`--set-login`** | This option only modifies the system shell list[cite: 216]. It **adds** the MyShell binary path (`/usr/local/bin/myshell`) to the `/etc/shells` file. |
| **No Parameter** | Runs the primary installation process: clones, compiles, and installs the binary to `/usr/local/bin`. |

### How to Use `--set-login`

Run the command below after the binary has been successfully installed. This step is necessary if you intend to use MyShell as your default login shell (e.g., via `chsh`):

```bash
sudo ./install.sh --set-login
```

**To then make MyShell your default login shell**, use `chsh`:

```bash
chsh -s /usr/local/bin/myshell
```

-----

## 📝 Usage

MyShell operates like any standard Unix shell.

### Executing Commands

Type the command and its arguments, then press **Enter**:

```bash
myshell $ ls -l | grep txt > output.txt
```

### Arithmetic Evaluation

Type the full expression and press **Enter**. MyShell detects the arithmetic pattern and evaluates it instead of executing a program[cite: 186].

```bash
myshell $ 10 + 5 * (8/4)
30
myshell $ 2^8 - 1
255
```

### Tab Completion

1.  **Command Completion**: Type the start of a command and press `TAB`:

    ```bash
    myshell $ help  # Press TAB after 'h'
    ```

    (This would complete to `help` or show options like `history`, etc., if they existed.)

2.  **File/Directory Completion**: Type the start of a file path (or a path starting with `./`, `/`, or `~`) and press `TAB`[cite: 103].

    ```bash
    myshell $ cat outpu  # Press TAB after 'outpu' to complete to 'output.txt'
    ```
