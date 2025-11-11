#include "shell.h"

// Global history variables
char* history[HISTORY_SIZE] = {0};
int history_count = 0;

// NEW: Global job control variables
pid_t background_jobs[MAX_JOBS] = {0};
char* job_commands[MAX_JOBS] = {0};
int job_count = 0;

// NEW: Clean up completed background jobs
void cleanup_background_jobs() {
    int status;
    pid_t pid;
    
    while ((pid = waitpid(-1, &status, WNOHANG)) > 0) {
        // Find and remove the completed job
        for (int i = 0; i < job_count; i++) {
            if (background_jobs[i] == pid) {
                printf("[%d] Done    %d %s\n", i+1, pid, job_commands[i]);
                
                // Free job command memory
                free(job_commands[i]);
                
                // Shift jobs array
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

// NEW: Print current background jobs
void print_jobs() {
    if (job_count == 0) {
        printf("No background jobs\n");
        return;
    }
    
    for (int i = 0; i < job_count; i++) {
        // Check if job is still running
        int status;
        pid_t result = waitpid(background_jobs[i], &status, WNOHANG);
        if (result == 0) {
            // Job is still running
            printf("[%d] Running %d %s\n", i+1, background_jobs[i], job_commands[i]);
        } else if (result > 0) {
            // Job has completed but not yet cleaned up
            printf("[%d] Done    %d %s\n", i+1, background_jobs[i], job_commands[i]);
        } else {
            // Error case
            printf("[%d] Unknown %d %s\n", i+1, background_jobs[i], job_commands[i]);
        }
    }
}

// NEW: Handle background execution
int handle_background(char** arglist) {
    // Check if last argument is &
    int background = 0;
    int last_index = 0;
    
    for (int i = 0; arglist[i] != NULL; i++) {
        last_index = i;
    }
    
    if (last_index >= 0 && strcmp(arglist[last_index], "&") == 0) {
        background = 1;
        arglist[last_index] = NULL; // Remove & from arguments
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
    // Check for pipes first
    if (handle_pipe(arglist) == 1) {
        return 0;
    }
    
    // NEW: Check for background execution
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
                // NEW: Store background job
                if (job_count < MAX_JOBS) {
                    background_jobs[job_count] = cpid;
                    
                    // Store command string for display
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
                    waitpid(cpid, &status, 0); // Wait anyway if no space
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
    
    if (strcmp(arglist[0], "exit") == 0) {
        // NEW: Wait for all background jobs before exiting
        if (job_count > 0) {
            printf("Waiting for background jobs to finish...\n");
            for (int i = 0; i < job_count; i++) {
                int status;
                waitpid(background_jobs[i], &status, 0);
                free(job_commands[i]);
            }
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
        printf("  jobs              - Display background jobs\n");  // UPDATED
        printf("  history           - Display command history\n");
        printf("\n");
        printf("Advanced features:\n");
        printf("  Tab completion    - Press Tab to complete commands and filenames\n");
        printf("  History navigation - Use Up/Down arrows to browse command history\n");
        printf("  I/O Redirection   - Use < for input, > for output redirection\n");
        printf("  Pipes             - Use | to connect commands (e.g., cmd1 | cmd2)\n");
        printf("  Command chaining  - Use ; to run multiple commands sequentially\n");
        printf("  Background jobs   - Use & to run commands in background\n");
        return 1;
    }
    
    // UPDATED: jobs command now shows actual background jobs
    else if (strcmp(arglist[0], "jobs") == 0) {
        print_jobs();
        return 1;
    }
    
    else if (strcmp(arglist[0], "history") == 0) {
        print_history();
        return 1;
    }
    
    return 0;
}
