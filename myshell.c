#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <fcntl.h>
#include <signal.h>
#include <dirent.h>
#include <termios.h>
#include <ctype.h>

#define MAX_INPUT 1024
#define MAX_ARGS 64
#define MAX_PIPES 10
#define MAX_COMPLETIONS 256
#define MAX_HISTORY 1000

// Terminal settings
struct termios orig_termios;

// Command history
char* history[MAX_HISTORY];
int history_count = 0;
int history_index = 0;

// Function prototypes
void display_prompt();
char* read_input_with_completion();
char** parse_input(char* input);
int execute_command(char** args);
int execute_piped_commands(char** args);
void handle_redirection(char** args);
int builtin_cd(char** args);
int builtin_exit(char** args);
int builtin_help(char** args);
int is_arithmetic_expression(char* str);
double evaluate_expression(char* expr);
char** get_completions(char* partial, int* count);
char** get_command_completions(char* partial, int* count);
char** get_file_completions(char* partial, int* count);
void enable_raw_mode();
void disable_raw_mode();
void add_to_history(char* cmd);
void save_history_to_file();
void load_history_from_file();

// Built-in command names
char* builtin_names[] = {
    "cd",
    "exit",
    "help"
};

// Built-in command functions
int (*builtin_funcs[])(char**) = {
    &builtin_cd,
    &builtin_exit,
    &builtin_help
};

int num_builtins() {
    return sizeof(builtin_names) / sizeof(char*);
}

// Built-in: cd
int builtin_cd(char** args) {
    if (args[1] == NULL) {
        fprintf(stderr, "myshell: expected argument to \"cd\"\n");
    } else {
        if (chdir(args[1]) != 0) {
            perror("myshell");
        }
    }
    return 1;
}

// Built-in: exit
int builtin_exit(char** args) {
    return 0;
}

// Built-in: help
int builtin_help(char** args) {
    printf("MyShell - A Simple Unix Shell\n");
    printf("Type program names and arguments, then press enter.\n");
    printf("Built-in commands:\n");
    for (int i = 0; i < num_builtins(); i++) {
        printf("  %s\n", builtin_names[i]);
    }
    printf("\nFeatures:\n");
    printf("  - Command execution\n");
    printf("  - Input/Output redirection (< and >)\n");
    printf("  - Piping (|)\n");
    printf("  - Tab completion for commands and files\n");
    printf("  - Command history with UP/DOWN arrows\n");
    printf("  - Arithmetic evaluation (e.g., 2+3, 10*5, 100/4)\n");
    printf("  - Cursor navigation with LEFT/RIGHT arrows\n");
    printf("  - Word-by-word navigation with CTRL+LEFT/RIGHT\n");
    printf("\nKeyboard Shortcuts:\n");
    printf("  - TAB: Auto-completion\n");
    printf("  - UP/DOWN: Navigate history\n");
    printf("  - LEFT/RIGHT: Move cursor character by character\n");
    printf("  - CTRL+LEFT/RIGHT: Move cursor word by word\n");
    printf("  - BACKSPACE: Delete character before cursor\n");
    printf("  - CTRL+D: Exit shell\n");
    return 1;
}

// Check if string is an arithmetic expression
int is_arithmetic_expression(char* str) {
    if (str == NULL || strlen(str) == 0) return 0;
    
    int has_digit = 0;
    int has_operator = 0;
    
    for (int i = 0; str[i] != '\0'; i++) {
        if (isdigit(str[i]) || str[i] == '.') {
            has_digit = 1;
        } else if (str[i] == '+' || str[i] == '-' || str[i] == '*' || 
                   str[i] == '/' || str[i] == '%' || str[i] == '(' || 
                   str[i] == ')' || str[i] == ' ' || str[i] == '^') {
            if (str[i] != ' ' && str[i] != '(' && str[i] != ')') {
                has_operator = 1;
            }
        } else {
            return 0; // Invalid character for arithmetic
        }
    }
    
    return has_digit && has_operator;
}

// Simple expression parser
double parse_number(char** expr) {
    double result = 0;
    double decimal = 0;
    int decimal_places = 0;
    
    while (isdigit(**expr) || **expr == '.') {
        if (**expr == '.') {
            decimal_places = 1;
        } else if (decimal_places > 0) {
            decimal = decimal * 10 + (**expr - '0');
            decimal_places++;
        } else {
            result = result * 10 + (**expr - '0');
        }
        (*expr)++;
    }
    
    if (decimal_places > 1) {
        for (int i = 1; i < decimal_places; i++) {
            decimal /= 10.0;
        }
        result += decimal;
    }
    
    return result;
}

double parse_factor(char** expr);
double parse_term(char** expr);
double parse_expr(char** expr);

double parse_factor(char** expr) {
    while (**expr == ' ') (*expr)++;
    
    if (**expr == '(') {
        (*expr)++;
        double result = parse_expr(expr);
        if (**expr == ')') (*expr)++;
        return result;
    }
    
    if (**expr == '-') {
        (*expr)++;
        return -parse_factor(expr);
    }
    
    if (**expr == '+') {
        (*expr)++;
        return parse_factor(expr);
    }
    
    return parse_number(expr);
}

double parse_power(char** expr) {
    double result = parse_factor(expr);
    
    while (**expr == ' ') (*expr)++;
    
    if (**expr == '^') {
        (*expr)++;
        double exponent = parse_power(expr);
        double base = result;
        result = 1;
        for (int i = 0; i < (int)exponent; i++) {
            result *= base;
        }
    }
    
    return result;
}

double parse_term(char** expr) {
    double result = parse_power(expr);
    
    while (1) {
        while (**expr == ' ') (*expr)++;
        
        if (**expr == '*') {
            (*expr)++;
            result *= parse_power(expr);
        } else if (**expr == '/') {
            (*expr)++;
            double divisor = parse_power(expr);
            if (divisor != 0) {
                result /= divisor;
            } else {
                fprintf(stderr, "Error: Division by zero\n");
                return 0;
            }
        } else if (**expr == '%') {
            (*expr)++;
            int divisor = (int)parse_power(expr);
            if (divisor != 0) {
                result = (int)result % divisor;
            } else {
                fprintf(stderr, "Error: Modulo by zero\n");
                return 0;
            }
        } else {
            break;
        }
    }
    
    return result;
}

double parse_expr(char** expr) {
    double result = parse_term(expr);
    
    while (1) {
        while (**expr == ' ') (*expr)++;
        
        if (**expr == '+') {
            (*expr)++;
            result += parse_term(expr);
        } else if (**expr == '-') {
            (*expr)++;
            result -= parse_term(expr);
        } else {
            break;
        }
    }
    
    return result;
}

// Evaluate arithmetic expression
double evaluate_expression(char* expr) {
    char* ptr = expr;
    return parse_expr(&ptr);
}

// Add command to history
void add_to_history(char* cmd) {
    if (cmd == NULL || strlen(cmd) == 0) return;
    
    // Don't add duplicate of last command
    if (history_count > 0 && strcmp(history[history_count - 1], cmd) == 0) {
        return;
    }
    
    if (history_count < MAX_HISTORY) {
        history[history_count] = strdup(cmd);
        history_count++;
    } else {
        // Shift history and add new command
        free(history[0]);
        for (int i = 0; i < MAX_HISTORY - 1; i++) {
            history[i] = history[i + 1];
        }
        history[MAX_HISTORY - 1] = strdup(cmd);
    }
    history_index = history_count;
}

// Save history to file
void save_history_to_file() {
    char* home = getenv("HOME");
    if (!home) return;
    
    char filepath[1024];
    snprintf(filepath, sizeof(filepath), "%s/.myshell_history", home);
    
    FILE* f = fopen(filepath, "w");
    if (!f) return;
    
    for (int i = 0; i < history_count; i++) {
        fprintf(f, "%s\n", history[i]);
    }
    
    fclose(f);
}

// Load history from file
void load_history_from_file() {
    char* home = getenv("HOME");
    if (!home) return;
    
    char filepath[1024];
    snprintf(filepath, sizeof(filepath), "%s/.myshell_history", home);
    
    FILE* f = fopen(filepath, "r");
    if (!f) return;
    
    char line[MAX_INPUT];
    while (fgets(line, sizeof(line), f) && history_count < MAX_HISTORY) {
        // Remove newline
        line[strcspn(line, "\n")] = 0;
        if (strlen(line) > 0) {
            history[history_count] = strdup(line);
            history_count++;
        }
    }
    
    history_index = history_count;
    fclose(f);
}

// Enable raw mode for terminal
void enable_raw_mode() {
    tcgetattr(STDIN_FILENO, &orig_termios);
    struct termios raw = orig_termios;
    raw.c_lflag &= ~(ECHO | ICANON);
    tcsetattr(STDIN_FILENO, TCSAFLUSH, &raw);
}

// Disable raw mode
void disable_raw_mode() {
    tcsetattr(STDIN_FILENO, TCSAFLUSH, &orig_termios);
}

// Get command completions from PATH
char** get_command_completions(char* partial, int* count) {
    char** completions = malloc(MAX_COMPLETIONS * sizeof(char*));
    *count = 0;
    
    // Check built-in commands first
    for (int i = 0; i < num_builtins(); i++) {
        if (strncmp(builtin_names[i], partial, strlen(partial)) == 0) {
            completions[*count] = strdup(builtin_names[i]);
            (*count)++;
        }
    }
    
    // Get PATH
    char* path_env = getenv("PATH");
    if (!path_env) return completions;
    
    char* path = strdup(path_env);
    char* dir = strtok(path, ":");
    
    while (dir && *count < MAX_COMPLETIONS) {
        DIR* d = opendir(dir);
        if (!d) {
            dir = strtok(NULL, ":");
            continue;
        }
        
        struct dirent* entry;
        while ((entry = readdir(d)) && *count < MAX_COMPLETIONS) {
            if (strncmp(entry->d_name, partial, strlen(partial)) == 0) {
                // Check if already in list
                int duplicate = 0;
                for (int i = 0; i < *count; i++) {
                    if (strcmp(completions[i], entry->d_name) == 0) {
                        duplicate = 1;
                        break;
                    }
                }
                if (!duplicate) {
                    completions[*count] = strdup(entry->d_name);
                    (*count)++;
                }
            }
        }
        closedir(d);
        dir = strtok(NULL, ":");
    }
    
    free(path);
    return completions;
}

// Get file/directory completions
char** get_file_completions(char* partial, int* count) {
    char** completions = malloc(MAX_COMPLETIONS * sizeof(char*));
    *count = 0;
    
    char* dir_path = ".";
    char* prefix = partial;
    char temp_path[1024];
    
    // Check if partial contains a path
    char* last_slash = strrchr(partial, '/');
    if (last_slash) {
        strncpy(temp_path, partial, last_slash - partial + 1);
        temp_path[last_slash - partial + 1] = '\0';
        dir_path = temp_path;
        prefix = last_slash + 1;
    }
    
    DIR* d = opendir(dir_path);
    if (!d) return completions;
    
    struct dirent* entry;
    while ((entry = readdir(d)) && *count < MAX_COMPLETIONS) {
        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0) {
            continue;
        }
        
        if (strncmp(entry->d_name, prefix, strlen(prefix)) == 0) {
            char full_path[2048];
            if (last_slash) {
                snprintf(full_path, sizeof(full_path), "%s%s", dir_path, entry->d_name);
            } else {
                snprintf(full_path, sizeof(full_path), "%s", entry->d_name);
            }
            
            // Add trailing slash for directories
            char path_check[2048];
            snprintf(path_check, sizeof(path_check), "%s/%s", dir_path, entry->d_name);
            DIR* test = opendir(path_check);
            if (test) {
                strcat(full_path, "/");
                closedir(test);
            }
            
            completions[*count] = strdup(full_path);
            (*count)++;
        }
    }
    
    closedir(d);
    return completions;
}

// Get appropriate completions
char** get_completions(char* partial, int* count) {
    // If partial is empty or starts with special chars, get file completions
    if (strlen(partial) == 0 || partial[0] == '.' || partial[0] == '/' || partial[0] == '~') {
        return get_file_completions(partial, count);
    }
    
    // Check if this is the first word (command)
    char input_copy[MAX_INPUT];
    strncpy(input_copy, partial, MAX_INPUT);
    
    // Simple heuristic: if no spaces before cursor position, it's a command
    int has_space = 0;
    for (int i = 0; i < strlen(partial); i++) {
        if (isspace(partial[i])) {
            has_space = 1;
            break;
        }
    }
    
    if (!has_space) {
        return get_command_completions(partial, count);
    } else {
        return get_file_completions(partial, count);
    }
}

// Display shell prompt
void display_prompt() {
    char cwd[1024];
    if (getcwd(cwd, sizeof(cwd)) != NULL) {
        printf("\033[1;32m%s\033[0m $ ", cwd);
    } else {
        printf("myshell $ ");
    }
    fflush(stdout);
}

// Read input with tab completion
char* read_input_with_completion() {
    char* input = malloc(MAX_INPUT);
    int cursor = 0;  // Current cursor position
    int len = 0;     // Total length of input
    int temp_history_index = history_index;
    
    memset(input, 0, MAX_INPUT);
    enable_raw_mode();
    
    while (1) {
        char c;
        read(STDIN_FILENO, &c, 1);
        
        if (c == '\n') {
            // Enter pressed
            printf("\n");
            break;
        } else if (c == 27) {
            // Escape sequence (arrow keys, etc.)
            char seq[5];
            if (read(STDIN_FILENO, &seq[0], 1) != 1) continue;
            if (read(STDIN_FILENO, &seq[1], 1) != 1) continue;
            
            if (seq[0] == '[') {
                if (seq[1] == 'A') {
                    // Up arrow - navigate history
                    if (temp_history_index > 0) {
                        temp_history_index--;
                        
                        // Clear current line
                        printf("\r\033[K");
                        display_prompt();
                        
                        // Copy history command to input
                        strcpy(input, history[temp_history_index]);
                        len = strlen(input);
                        cursor = len;
                        printf("%s", input);
                        fflush(stdout);
                    }
                } else if (seq[1] == 'B') {
                    // Down arrow - navigate history
                    if (temp_history_index < history_count - 1) {
                        temp_history_index++;
                        
                        // Clear current line
                        printf("\r\033[K");
                        display_prompt();
                        
                        // Copy history command to input
                        strcpy(input, history[temp_history_index]);
                        len = strlen(input);
                        cursor = len;
                        printf("%s", input);
                        fflush(stdout);
                    } else if (temp_history_index == history_count - 1) {
                        // Go to empty line
                        temp_history_index = history_count;
                        
                        // Clear current line
                        printf("\r\033[K");
                        display_prompt();
                        
                        memset(input, 0, MAX_INPUT);
                        len = 0;
                        cursor = 0;
                        fflush(stdout);
                    }
                } else if (seq[1] == 'C') {
                    // Right arrow - move cursor right
                    if (cursor < len) {
                        cursor++;
                        printf("\033[C");  // Move cursor right
                        fflush(stdout);
                    }
                } else if (seq[1] == 'D') {
                    // Left arrow - move cursor left
                    if (cursor > 0) {
                        cursor--;
                        printf("\033[D");  // Move cursor left
                        fflush(stdout);
                    }
                } else if (seq[1] == '1') {
                    // Check for Ctrl+Arrow (extended sequences)
                    if (read(STDIN_FILENO, &seq[2], 1) == 1) {
                        if (seq[2] == ';') {
                            if (read(STDIN_FILENO, &seq[3], 1) == 1 && 
                                read(STDIN_FILENO, &seq[4], 1) == 1) {
                                if (seq[3] == '5') {
                                    if (seq[4] == 'C') {
                                        // Ctrl+Right - move word forward
                                        while (cursor < len && input[cursor] != ' ') cursor++;
                                        while (cursor < len && input[cursor] == ' ') cursor++;
                                        
                                        // Redraw line with cursor at new position
                                        printf("\r\033[K");
                                        display_prompt();
                                        printf("%s", input);
                                        for (int i = len; i > cursor; i--) {
                                            printf("\033[D");
                                        }
                                        fflush(stdout);
                                    } else if (seq[4] == 'D') {
                                        // Ctrl+Left - move word backward
                                        if (cursor > 0) cursor--;
                                        while (cursor > 0 && input[cursor] == ' ') cursor--;
                                        while (cursor > 0 && input[cursor - 1] != ' ') cursor--;
                                        
                                        // Redraw line with cursor at new position
                                        printf("\r\033[K");
                                        display_prompt();
                                        printf("%s", input);
                                        for (int i = len; i > cursor; i--) {
                                            printf("\033[D");
                                        }
                                        fflush(stdout);
                                    }
                                }
                            }
                        }
                    }
                }
            }
        } else if (c == '\t') {
            // Tab completion
            input[len] = '\0';
            
            // Find the word to complete
            int word_start = cursor - 1;
            while (word_start >= 0 && !isspace(input[word_start])) {
                word_start--;
            }
            word_start++;
            
            char partial[MAX_INPUT];
            strncpy(partial, input + word_start, cursor - word_start);
            partial[cursor - word_start] = '\0';
            
            int count;
            char** completions = get_completions(partial, &count);
            
            if (count == 1) {
                // Single completion - auto-complete
                int partial_len = strlen(partial);
                int completion_len = strlen(completions[0]);
                
                // Shift remaining text if needed
                if (cursor < len) {
                    memmove(input + word_start + completion_len, 
                           input + cursor, 
                           len - cursor);
                }
                
                // Insert completion
                memcpy(input + word_start, completions[0], completion_len);
                len = len - partial_len + completion_len;
                cursor = word_start + completion_len;
                
                // Add space after completion
                if (cursor == len && len < MAX_INPUT - 1) {
                    input[len++] = ' ';
                    cursor++;
                }
                
                // Redraw line
                printf("\r\033[K");
                display_prompt();
                input[len] = '\0';
                printf("%s", input);
                for (int i = len; i > cursor; i--) {
                    printf("\033[D");
                }
            } else if (count > 1) {
                // Multiple completions - show them
                printf("\n");
                for (int i = 0; i < count; i++) {
                    printf("%s  ", completions[i]);
                    if ((i + 1) % 5 == 0) printf("\n");
                }
                printf("\n");
                
                // Re-display prompt and current input
                display_prompt();
                input[len] = '\0';
                printf("%s", input);
                for (int i = len; i > cursor; i--) {
                    printf("\033[D");
                }
            }
            
            // Free completions
            for (int i = 0; i < count; i++) {
                free(completions[i]);
            }
            free(completions);
            
            fflush(stdout);
        } else if (c == 127 || c == 8) {
            // Backspace
            if (cursor > 0) {
                // Shift text left
                memmove(input + cursor - 1, input + cursor, len - cursor);
                len--;
                cursor--;
                
                // Redraw line
                printf("\r\033[K");
                display_prompt();
                input[len] = '\0';
                printf("%s", input);
                for (int i = len; i > cursor; i--) {
                    printf("\033[D");
                }
                fflush(stdout);
            }
        } else if (c == 4) {
            // Ctrl+D (EOF)
            if (len == 0) {
                disable_raw_mode();
                free(input);
                exit(0);
            }
        } else if (c >= 32 && c <= 126) {
            // Printable character - insert at cursor position
            if (len < MAX_INPUT - 1) {
                // Shift text right to make room
                memmove(input + cursor + 1, input + cursor, len - cursor);
                input[cursor] = c;
                len++;
                cursor++;
                
                // Redraw from cursor position
                printf("\r\033[K");
                display_prompt();
                input[len] = '\0';
                printf("%s", input);
                for (int i = len; i > cursor; i--) {
                    printf("\033[D");
                }
                fflush(stdout);
            }
        }
    }
    
    disable_raw_mode();
    input[len] = '\0';
    
    // Add to history if not empty
    if (strlen(input) > 0) {
        add_to_history(input);
    }
    
    return input;
}

// Parse input into arguments
char** parse_input(char* input) {
    int bufsize = MAX_ARGS;
    int position = 0;
    char** tokens = malloc(bufsize * sizeof(char*));
    char* token;
    
    if (!tokens) {
        fprintf(stderr, "myshell: allocation error\n");
        exit(1);
    }
    
    token = strtok(input, " \t\r\n");
    while (token != NULL) {
        tokens[position] = token;
        position++;
        
        if (position >= bufsize) {
            bufsize += MAX_ARGS;
            tokens = realloc(tokens, bufsize * sizeof(char*));
            if (!tokens) {
                fprintf(stderr, "myshell: allocation error\n");
                exit(1);
            }
        }
        
        token = strtok(NULL, " \t\r\n");
    }
    tokens[position] = NULL;
    return tokens;
}

// Handle input/output redirection
void handle_redirection(char** args) {
    for (int i = 0; args[i] != NULL; i++) {
        if (strcmp(args[i], "<") == 0) {
            int fd = open(args[i + 1], O_RDONLY);
            if (fd < 0) {
                perror("myshell");
                exit(1);
            }
            dup2(fd, STDIN_FILENO);
            close(fd);
            args[i] = NULL;
        } else if (strcmp(args[i], ">") == 0) {
            int fd = open(args[i + 1], O_WRONLY | O_CREAT | O_TRUNC, 0644);
            if (fd < 0) {
                perror("myshell");
                exit(1);
            }
            dup2(fd, STDOUT_FILENO);
            close(fd);
            args[i] = NULL;
        }
    }
}

// Execute piped commands
int execute_piped_commands(char** args) {
    int pipe_positions[MAX_PIPES];
    int pipe_count = 0;
    
    // Find all pipe positions
    for (int i = 0; args[i] != NULL; i++) {
        if (strcmp(args[i], "|") == 0) {
            pipe_positions[pipe_count++] = i;
            args[i] = NULL;
        }
    }
    
    if (pipe_count == 0) {
        return -1; // No pipes found
    }
    
    int pipefds[2 * pipe_count];
    for (int i = 0; i < pipe_count; i++) {
        if (pipe(pipefds + i * 2) < 0) {
            perror("pipe");
            return 1;
        }
    }
    
    int cmd_start = 0;
    for (int i = 0; i <= pipe_count; i++) {
        pid_t pid = fork();
        
        if (pid == 0) {
            // Child process
            if (i > 0) {
                dup2(pipefds[(i - 1) * 2], STDIN_FILENO);
            }
            if (i < pipe_count) {
                dup2(pipefds[i * 2 + 1], STDOUT_FILENO);
            }
            
            // Close all pipe fds
            for (int j = 0; j < 2 * pipe_count; j++) {
                close(pipefds[j]);
            }
            
            handle_redirection(&args[cmd_start]);
            execvp(args[cmd_start], &args[cmd_start]);
            perror("myshell");
            exit(1);
        }
        
        cmd_start = pipe_positions[i] + 1;
    }
    
    // Close all pipe fds in parent
    for (int i = 0; i < 2 * pipe_count; i++) {
        close(pipefds[i]);
    }
    
    // Wait for all children
    for (int i = 0; i <= pipe_count; i++) {
        wait(NULL);
    }
    
    return 1;
}

// Execute command
int execute_command(char** args) {
    if (args[0] == NULL) {
        return 1;
    }
    
    // Check for built-in commands
    for (int i = 0; i < num_builtins(); i++) {
        if (strcmp(args[0], builtin_names[i]) == 0) {
            return (*builtin_funcs[i])(args);
        }
    }
    
    // Check for pipes
    if (execute_piped_commands(args) != -1) {
        return 1;
    }
    
    // Execute external command
    pid_t pid = fork();
    
    if (pid == 0) {
        // Child process
        handle_redirection(args);
        execvp(args[0], args);
        perror("myshell");
        exit(1);
    } else if (pid < 0) {
        perror("myshell");
    } else {
        // Parent process
        int status;
        waitpid(pid, &status, 0);
    }
    
    return 1;
}

// Main shell loop
void shell_loop() {
    char* input;
    char** args;
    int status;
    
    do {
        display_prompt();
        input = read_input_with_completion();
        
        // Check if input is an arithmetic expression
        if (is_arithmetic_expression(input)) {
            double result = evaluate_expression(input);
            // Check if result is an integer
            if (result == (int)result) {
                printf("%d\n", (int)result);
            } else {
                printf("%.2f\n", result);
            }
            status = 1;
        } else {
            args = parse_input(input);
            status = execute_command(args);
            free(args);
        }
        
        free(input);
    } while (status);
}

int main(int argc, char** argv) {
    // Ignore Ctrl+C in parent
    signal(SIGINT, SIG_IGN);
    
    // Load command history
    load_history_from_file();
    
    printf("MyShell v2.0 with Tab Completion & History\n");
    printf("Type 'help' for more information.\n");
    printf("Press TAB for auto-completion, UP/DOWN for history.\n");
    printf("Use LEFT/RIGHT arrows to move cursor, CTRL+LEFT/RIGHT to jump words.\n");
    printf("You can also evaluate arithmetic expressions (e.g., 2+3, 10*5).\n\n");
    
    shell_loop();
    
    // Save history before exit
    save_history_to_file();
    
    return 0;
}