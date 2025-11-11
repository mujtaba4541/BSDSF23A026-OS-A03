#include "shell.h"

// Global history variables
char* history[HISTORY_SIZE] = {0};
int history_count = 0;

// NEW: Parse redirection operators and extract filenames
int parse_redirection(char** arglist, char** input_file, char** output_file) {
    *input_file = NULL;
    *output_file = NULL;
    
    for (int i = 0; arglist[i] != NULL; i++) {
        if (strcmp(arglist[i], "<") == 0) {
            *input_file = arglist[i+1];
            arglist[i] = NULL; // Remove < from arguments
            if (arglist[i+1]) arglist[i+1] = NULL; // Remove filename from arguments
        }
        else if (strcmp(arglist[i], ">") == 0) {
            *output_file = arglist[i+1];
            arglist[i] = NULL; // Remove > from arguments
            if (arglist[i+1]) arglist[i+1] = NULL; // Remove filename from arguments
        }
    }
    return 0;
}

// NEW: Handle I/O redirection
int handle_redirection(char** arglist) {
    char* input_file = NULL;
    char* output_file = NULL;
    
    parse_redirection(arglist, &input_file, &output_file);
    
    // Handle input redirection
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
    
    // Handle output redirection
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

// NEW: Handle pipe between commands
int handle_pipe(char** arglist) {
    int pipe_pos = -1;
    
    // Find pipe position
    for (int i = 0; arglist[i] != NULL; i++) {
        if (strcmp(arglist[i], "|") == 0) {
            pipe_pos = i;
            break;
        }
    }
    
    if (pipe_pos == -1) {
        return 0; // No pipe found
    }
    
    // Split commands at pipe
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
        // Left child process (writer)
        close(pipefd[0]); // Close read end
        dup2(pipefd[1], STDOUT_FILENO); // Connect stdout to pipe write
        close(pipefd[1]);
        
        // Handle redirection for left command
        handle_redirection(left_cmd);
        
        execvp(left_cmd[0], left_cmd);
        perror("Left command failed");
        exit(1);
    }
    
    pid_t right_pid = fork();
    if (right_pid == 0) {
        // Right child process (reader)
        close(pipefd[1]); // Close write end
        dup2(pipefd[0], STDIN_FILENO); // Connect stdin to pipe read
        close(pipefd[0]);
        
        // Handle redirection for right command
        handle_redirection(right_cmd);
        
        execvp(right_cmd[0], right_cmd);
        perror("Right command failed");
        exit(1);
    }
    
    // Parent process
    close(pipefd[0]);
    close(pipefd[1]);
    
    // Wait for both children
    int status;
    waitpid(left_pid, &status, 0);
    waitpid(right_pid, &status, 0);
    
    return 1; // Indicate pipe was handled
}

int execute(char* arglist[]) {
    // NEW: Check for pipes first
    if (handle_pipe(arglist) == 1) {
        return 0; // Pipe was handled, no need for normal execution
    }
    
    int status;
    int cpid = fork();

    switch (cpid) {
        case -1:
            perror("fork failed");
            exit(1);
        case 0: // Child process
            // NEW: Handle redirection before exec
            if (handle_redirection(arglist) == -1) {
                exit(1);
            }
            execvp(arglist[0], arglist);
            perror("Command not found");
            exit(1);
        default: // Parent process
            waitpid(cpid, &status, 0);
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
    
    if (strcmp(arglist[0], "exit") == 0) {
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
        printf("  jobs              - Display background jobs (not implemented yet)\n");
        printf("  history           - Display command history\n");
        printf("\n");
        printf("Advanced features:\n");
        printf("  Tab completion    - Press Tab to complete commands and filenames\n");
        printf("  History navigation - Use Up/Down arrows to browse command history\n");
        printf("  I/O Redirection   - Use < for input, > for output redirection\n");
        printf("  Pipes             - Use | to connect commands (e.g., cmd1 | cmd2)\n");
        return 1;
    }
    
    else if (strcmp(arglist[0], "jobs") == 0) {
        printf("Job control not yet implemented.\n");
        return 1;
    }
    
    else if (strcmp(arglist[0], "history") == 0) {
        print_history();
        return 1;
    }
    
    return 0;
}
