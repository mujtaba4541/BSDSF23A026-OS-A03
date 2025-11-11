#include "shell.h"

// NEW: Global history variables (now accessible from other files)
char* history[HISTORY_SIZE] = {0};
int history_count = 0;

int execute(char* arglist[]) {
    int status;
    int cpid = fork();

    switch (cpid) {
        case -1:
            perror("fork failed");
            exit(1);
        case 0: // Child process
            execvp(arglist[0], arglist);
            perror("Command not found");
            exit(1);
        default: // Parent process
            waitpid(cpid, &status, 0);
            return 0;
    }
}

// NEW: Add command to history
void add_to_history(const char* cmdline) {
    // Skip empty commands and history commands
    if (cmdline == NULL || strlen(cmdline) == 0 || cmdline[0] == '!') {
        return;
    }
    
    // Don't add duplicate consecutive commands
    if (history_count > 0 && strcmp(history[history_count-1], cmdline) == 0) {
        return;
    }
    
    if (history_count < HISTORY_SIZE) {
        history[history_count] = strdup(cmdline);
        history_count++;
    } else {
        // Shift history when full (FIFO)
        free(history[0]);
        for (int i = 1; i < HISTORY_SIZE; i++) {
            history[i-1] = history[i];
        }
        history[HISTORY_SIZE-1] = strdup(cmdline);
    }
}

// NEW: Print command history
void print_history() {
    for (int i = 0; i < history_count; i++) {
        printf("%d %s\n", i+1, history[i]);
    }
}

// NEW: Execute command from history by number
int execute_from_history(int n) {
    if (n < 1 || n > history_count) {
        printf("No such command in history\n");
        return -1;
    }
    return n-1; // Return index in history array
}

int handle_builtin(char** arglist) {
    if (arglist[0] == NULL) {
        return 0;
    }
    
    // exit command
    if (strcmp(arglist[0], "exit") == 0) {
        printf("Shell exited.\n");
        exit(0);
        return 1;
    }
    
    // cd command
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
    
    // help command
    else if (strcmp(arglist[0], "help") == 0) {
        printf("Built-in commands:\n");
        printf("  cd <directory>    - Change current working directory\n");
        printf("  exit              - Exit the shell\n");
        printf("  help              - Display this help message\n");
        printf("  jobs              - Display background jobs (not implemented yet)\n");
        printf("  history           - Display command history\n");
        return 1;
    }
    
    // jobs command
    else if (strcmp(arglist[0], "jobs") == 0) {
        printf("Job control not yet implemented.\n");
        return 1;
    }
    
    // NEW: history command
    else if (strcmp(arglist[0], "history") == 0) {
        print_history();
        return 1;
    }
    
    return 0;
}
