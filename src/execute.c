#include "shell.h"

// Global history variables
char* history[HISTORY_SIZE] = {0};
int history_count = 0;

// Global job control variables
pid_t background_jobs[MAX_JOBS] = {0};
char* job_commands[MAX_JOBS] = {0};
int job_count = 0;

// Global variable storage
char* var_names[MAX_VARS] = {0};
char* var_values[MAX_VARS] = {0};
int var_count = 0;

// Check if command is a variable assignment
int is_variable_assignment(char** arglist) {
    if (arglist[0] == NULL) {
        return 0;
    }
    
    char* cmd = arglist[0];
    char* equal_sign = strchr(cmd, '=');
    
    // Must have = sign and it shouldn't be the first character
    if (equal_sign == NULL || equal_sign == cmd) {
        return 0;
    }
    
    // Check that the part before = is a valid variable name
    char* var_name = cmd;
    int name_len = equal_sign - cmd;
    
    for (int i = 0; i < name_len; i++) {
        if (!isalnum(var_name[i]) && var_name[i] != '_') {
            return 0; // Invalid character in variable name
        }
    }
    
    // Variable name cannot start with a number
    if (isdigit(var_name[0])) {
        return 0;
    }
    
    return 1; // Valid variable assignment
}

// Handle variable assignment
void handle_variables(char** arglist) {
    if (arglist[0] == NULL) return;
    
    char* assignment = arglist[0];
    char* equal_sign = strchr(assignment, '=');
    if (equal_sign == NULL) return;
    
    *equal_sign = '\0';
    char* var_name = assignment;
    char* var_value = equal_sign + 1;
    
    // Proper quote handling - remove surrounding quotes only
    char* final_value;
    if (var_value[0] == '"' && var_value[strlen(var_value)-1] == '"' && strlen(var_value) > 1) {
        // Create a new string without the quotes
        final_value = malloc(strlen(var_value) - 1);
        if (final_value) {
            strncpy(final_value, var_value + 1, strlen(var_value) - 2);
            final_value[strlen(var_value) - 2] = '\0';
        } else {
            final_value = strdup(""); // Fallback if malloc fails
        }
    } else {
        final_value = strdup(var_value); // Make a copy
    }
    
    if (!final_value) return; // Memory allocation failed
    
    // Update existing or add new
    for (int i = 0; i < var_count; i++) {
        if (strcmp(var_names[i], var_name) == 0) {
            free(var_values[i]);
            var_values[i] = final_value;
            return;
        }
    }
    
    // Add new variable
    if (var_count < MAX_VARS) {
        var_names[var_count] = strdup(var_name);
        var_values[var_count] = final_value;
        var_count++;
    } else {
        printf("Maximum variables reached (%d)\n", MAX_VARS);
        free(final_value);
    }
}

// Expand variables in arguments
void expand_variables(char** arglist) {
    for (int i = 0; arglist[i] != NULL; i++) {
        char* arg = arglist[i];
        
        // Check if this argument starts with $ and has more characters
        if (arg[0] == '$' && strlen(arg) > 1) {
            char* var_name = arg + 1; // Skip the $
            
            // Extract clean variable name (alphanumeric and underscore only)
            char clean_var_name[ARGLEN] = {0};
            int j = 0;
            while (var_name[j] != '\0' && j < ARGLEN - 1) {
                if (isalnum(var_name[j]) || var_name[j] == '_') {
                    clean_var_name[j] = var_name[j];
                    j++;
                } else {
                    break; // Stop at first non-alphanumeric character
                }
            }
            clean_var_name[j] = '\0';
            
            // Skip if variable name is empty
            if (strlen(clean_var_name) == 0) {
                continue;
            }
            
            char* new_value = NULL;
            
            // Check environment variables first
            char* env_value = getenv(clean_var_name);
            if (env_value != NULL) {
                new_value = strdup(env_value);
            } else {
                // Check shell variables
                for (int k = 0; k < var_count; k++) {
                    if (strcmp(var_names[k], clean_var_name) == 0) {
                        new_value = strdup(var_values[k]);
                        break;
                    }
                }
            }
            
            // If we found a value, replace the argument
            if (new_value != NULL) {
                free(arglist[i]);
                arglist[i] = new_value;
            }
            // If variable not found, leave it as $VAR (don't replace)
        }
    }
}

// Print variables
void print_variables() {
    if (var_count == 0) {
        printf("No shell variables defined\n");
    } else {
        printf("Shell variables:\n");
        for (int i = 0; i < var_count; i++) {
            printf("  %s=%s\n", var_names[i], var_values[i]);
        }
    }
    
    // Also show some important environment variables
    printf("\nEnvironment variables:\n");
    char* important_vars[] = {"HOME", "PATH", "USER", "SHELL", "PWD", NULL};
    for (int i = 0; important_vars[i] != NULL; i++) {
        char* value = getenv(important_vars[i]);
        if (value != NULL) {
            printf("  %s=%s\n", important_vars[i], value);
        }
    }
}

// DEBUG: Print all variables for debugging
void debug_print_variables() {
    printf("DEBUG - Current variables (%d):\n", var_count);
    for (int i = 0; i < var_count; i++) {
        printf("  [%d] %s='%s' (len=%zu)\n", i, var_names[i], var_values[i], strlen(var_values[i]));
    }
}

void cleanup_background_jobs() {
    int status;
    pid_t pid;
    
    while ((pid = waitpid(-1, &status, WNOHANG)) > 0) {
        for (int i = 0; i < job_count; i++) {
            if (background_jobs[i] == pid) {
                printf("[%d] Done    %d %s\n", i+1, pid, job_commands[i]);
                
                free(job_commands[i]);
                
                for (int j = i; j < job_count - 1; j++) {
                    background_jobs[j] = background_jobs[j+1];
                    job_commands[j] = job_commands[j+1];
                }
                job_count--;
                background_jobs[job_count] = 0;
                job_commands[job_count] = NULL;
                break;
            }
        }
    }
}

void print_jobs() {
    if (job_count == 0) {
        printf("No background jobs\n");
        return;
    }
    
    for (int i = 0; i < job_count; i++) {
        int status;
        pid_t result = waitpid(background_jobs[i], &status, WNOHANG);
        if (result == 0) {
            printf("[%d] Running %d %s\n", i+1, background_jobs[i], job_commands[i]);
        } else if (result > 0) {
            printf("[%d] Done    %d %s\n", i+1, background_jobs[i], job_commands[i]);
        } else {
            printf("[%d] Unknown %d %s\n", i+1, background_jobs[i], job_commands[i]);
        }
    }
}

int handle_background(char** arglist) {
    int background = 0;
    int last_index = 0;
    
    for (int i = 0; arglist[i] != NULL; i++) {
        last_index = i;
    }
    
    if (last_index >= 0 && strcmp(arglist[last_index], "&") == 0) {
        background = 1;
        arglist[last_index] = NULL;
    }
    
    return background;
}

int parse_redirection(char** arglist, char** input_file, char** output_file) {
    *input_file = NULL;
    *output_file = NULL;
    
    for (int i = 0; arglist[i] != NULL; i++) {
        if (strcmp(arglist[i], "<") == 0) {
            *input_file = arglist[i+1];
            arglist[i] = NULL;
            if (arglist[i+1]) arglist[i+1] = NULL;
        }
        else if (strcmp(arglist[i], ">") == 0) {
            *output_file = arglist[i+1];
            arglist[i] = NULL;
            if (arglist[i+1]) arglist[i+1] = NULL;
        }
    }
    return 0;
}

int handle_redirection(char** arglist) {
    char* input_file = NULL;
    char* output_file = NULL;
    
    parse_redirection(arglist, &input_file, &output_file);
    
    if (input_file != NULL) {
        int fd = open(input_file, O_RDONLY);
        if (fd == -1) {
            perror("Input redirection failed");
            return -1;
        }
        if (dup2(fd, STDIN_FILENO) == -1) {
            perror("dup2 input failed");
            close(fd);
            return -1;
        }
        close(fd);
    }
    
    if (output_file != NULL) {
        int fd = open(output_file, O_WRONLY | O_CREAT | O_TRUNC, 0644);
        if (fd == -1) {
            perror("Output redirection failed");
            return -1;
        }
        if (dup2(fd, STDOUT_FILENO) == -1) {
            perror("dup2 output failed");
            close(fd);
            return -1;
        }
        close(fd);
    }
    
    return 0;
}

int handle_pipe(char** arglist) {
    int pipe_pos = -1;
    
    for (int i = 0; arglist[i] != NULL; i++) {
        if (strcmp(arglist[i], "|") == 0) {
            pipe_pos = i;
            break;
        }
    }
    
    if (pipe_pos == -1) {
        return 0;
    }
    
    arglist[pipe_pos] = NULL;
    char** left_cmd = arglist;
    char** right_cmd = &arglist[pipe_pos + 1];
    
    if (right_cmd[0] == NULL) {
        fprintf(stderr, "Syntax error: no command after pipe\n");
        return -1;
    }
    
    int pipefd[2];
    if (pipe(pipefd) == -1) {
        perror("pipe failed");
        return -1;
    }
    
    pid_t left_pid = fork();
    if (left_pid == 0) {
        close(pipefd[0]);
        dup2(pipefd[1], STDOUT_FILENO);
        close(pipefd[1]);
        
        handle_redirection(left_cmd);
        
        execvp(left_cmd[0], left_cmd);
        perror("Left command failed");
        exit(1);
    }
    
    pid_t right_pid = fork();
    if (right_pid == 0) {
        close(pipefd[1]);
        dup2(pipefd[0], STDIN_FILENO);
        close(pipefd[0]);
        
        handle_redirection(right_cmd);
        
        execvp(right_cmd[0], right_cmd);
        perror("Right command failed");
        exit(1);
    }
    
    close(pipefd[0]);
    close(pipefd[1]);
    
    int status;
    waitpid(left_pid, &status, 0);
    waitpid(right_pid, &status, 0);
    
    return 1;
}

int execute(char* arglist[]) {
    // Expand variables before execution
    expand_variables(arglist);
    
    if (handle_pipe(arglist) == 1) {
        return 0;
    }
    
    int background = handle_background(arglist);
    
    int status;
    int cpid = fork();

    switch (cpid) {
        case -1:
            perror("fork failed");
            exit(1);
        case 0: // Child process
            if (handle_redirection(arglist) == -1) {
                exit(1);
            }
            execvp(arglist[0], arglist);
            perror("Command not found");
            exit(1);
        default: // Parent process
            if (background) {
                if (job_count < MAX_JOBS) {
                    background_jobs[job_count] = cpid;
                    
                    char cmd_buf[MAX_LEN] = "";
                    for (int i = 0; arglist[i] != NULL && i < 10; i++) {
                        if (i > 0) strcat(cmd_buf, " ");
                        strcat(cmd_buf, arglist[i]);
                    }
                    job_commands[job_count] = strdup(cmd_buf);
                    
                    printf("[%d] %d\n", job_count + 1, cpid);
                    job_count++;
                } else {
                    printf("Maximum background jobs reached (%d)\n", MAX_JOBS);
                    waitpid(cpid, &status, 0);
                }
            } else {
                waitpid(cpid, &status, 0);
            }
            return 0;
    }
}

void add_to_history(const char* cmdline) {
    if (cmdline == NULL || strlen(cmdline) == 0 || cmdline[0] == '!') {
        return;
    }
    
    if (history_count > 0 && strcmp(history[history_count-1], cmdline) == 0) {
        return;
    }
    
    if (history_count < HISTORY_SIZE) {
        history[history_count] = strdup(cmdline);
        history_count++;
    } else {
        free(history[0]);
        for (int i = 1; i < HISTORY_SIZE; i++) {
            history[i-1] = history[i];
        }
        history[HISTORY_SIZE-1] = strdup(cmdline);
    }
    
    if (cmdline && *cmdline) {
        add_history(cmdline);
    }
}

void print_history() {
    printf("Shell command history:\n");
    for (int i = 0; i < history_count; i++) {
        printf("%d %s\n", i+1, history[i]);
    }
}

int execute_from_history(int n) {
    if (n < 1 || n > history_count) {
        printf("No such command in history\n");
        return -1;
    }
    return n-1;
}

int handle_builtin(char** arglist) {
    if (arglist[0] == NULL) {
        return 0;
    }
    
    // Handle variable assignment
    if (is_variable_assignment(arglist)) {
        handle_variables(arglist);
        return 1;
    }
    
    if (strcmp(arglist[0], "exit") == 0) {
        if (job_count > 0) {
            printf("Waiting for background jobs to finish...\n");
            for (int i = 0; i < job_count; i++) {
                int status;
                waitpid(background_jobs[i], &status, 0);
                free(job_commands[i]);
            }
        }
        
        // Clean up variables before exit
        for (int i = 0; i < var_count; i++) {
            free(var_names[i]);
            free(var_values[i]);
        }
        
        printf("Shell exited.\n");
        rl_clear_history();
        exit(0);
        return 1;
    }
    
    else if (strcmp(arglist[0], "cd") == 0) {
        char* path = arglist[1];
        if (path == NULL) {
            path = getenv("HOME");
            if (path == NULL) {
                fprintf(stderr, "cd: HOME environment variable not set\n");
                return 1;
            }
        }
        
        if (chdir(path) != 0) {
            perror("cd failed");
        }
        return 1;
    }
    
    else if (strcmp(arglist[0], "help") == 0) {
        printf("Built-in commands:\n");
        printf("  cd <directory>    - Change current working directory\n");
        printf("  exit              - Exit the shell\n");
        printf("  help              - Display this help message\n");
        printf("  jobs              - Display background jobs\n");
        printf("  history           - Display command history\n");
        printf("  set               - Display all variables\n");
        printf("\n");
        printf("Variable usage:\n");
        printf("  NAME=value        - Set variable (no spaces around =)\n");
        printf("  NAME=\"value\"     - Set variable with spaces\n");
        printf("  echo $NAME        - Use variable in commands\n");
        printf("  set               - Display all shell variables and important environment variables\n");
        printf("\n");
        printf("Advanced features:\n");
        printf("  Tab completion    - Press Tab to complete commands and filenames\n");
        printf("  History navigation - Use Up/Down arrows to browse command history\n");
        printf("  I/O Redirection   - Use < for input, > for output redirection\n");
        printf("  Pipes             - Use | to connect commands (e.g., cmd1 | cmd2)\n");
        printf("  Command chaining  - Use ; to run multiple commands sequentially\n");
        printf("  Background jobs   - Use & to run commands in background\n");
        printf("  If-then-else     - Use if-then-else-fi for conditional execution\n");
        return 1;
    }
    
    else if (strcmp(arglist[0], "jobs") == 0) {
        print_jobs();
        return 1;
    }
    
    else if (strcmp(arglist[0], "history") == 0) {
        print_history();
        return 1;
    }
    
    // set command to display variables
    else if (strcmp(arglist[0], "set") == 0) {
        print_variables();
        // debug_print_variables(); // Uncomment for debugging
        return 1;
    }
    
    return 0;
}
