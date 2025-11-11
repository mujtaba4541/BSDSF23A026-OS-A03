#include "shell.h"

char* read_cmd(char* prompt, FILE* fp) {
    (void)fp;
    
    char* cmdline = readline(prompt);
    
    if (cmdline == NULL) {
        return NULL;
    }
    
    if (cmdline && *cmdline) {
        add_history(cmdline);
    }
    
    return cmdline;
}

// UPDATED: Improved multiline block reading
char* read_multiline_block(const char* prompt) {
    (void)prompt;
    
    static char block_buffer[MAX_LEN * MAX_BLOCK_LINES] = "";
    block_buffer[0] = '\0';
    
    char* line;
    int line_count = 0;
    int then_found = 0;
    int else_found = 0;
    int fi_found = 0;
    
    printf("Enter if-then-else block (end with 'fi'):\n");
    
    while (line_count < MAX_BLOCK_LINES) {
        const char* current_prompt;
        if (!then_found) {
            current_prompt = "if> ";
        } else if (!else_found) {
            current_prompt = "then> ";
        } else {
            current_prompt = "else> ";
        }
        
        line = readline(current_prompt);
        if (line == NULL) {
            break;
        }
        
        // Add to readline history
        if (line && *line) {
            add_history(line);
        }
        
        // Trim the line
        char* trimmed_line = line;
        while (*trimmed_line == ' ' || *trimmed_line == '\t') trimmed_line++;
        char* end = trimmed_line + strlen(trimmed_line) - 1;
        while (end > trimmed_line && (*end == ' ' || *end == '\t' || *end == '\n')) {
            *end = '\0';
            end--;
        }
        
        // Check for control keywords
        if (strcmp(trimmed_line, "then") == 0) {
            then_found = 1;
            free(line);
            continue;
        } else if (strcmp(trimmed_line, "else") == 0) {
            else_found = 1;
            free(line);
            continue;
        } else if (strcmp(trimmed_line, "fi") == 0) {
            fi_found = 1;
            free(line);
            break;
        }
        
        // Only add non-empty, non-keyword lines to the block
        if (strlen(trimmed_line) > 0) {
            if (block_buffer[0] != '\0') {
                strcat(block_buffer, "\n");
            }
            strcat(block_buffer, trimmed_line);
        }
        
        free(line);
        line_count++;
    }
    
    if (!fi_found) {
        printf("Error: 'fi' not found to close if statement\n");
        block_buffer[0] = '\0';
        return NULL;
    }
    
    if (block_buffer[0] == '\0') {
        return strdup(""); // Empty block
    }
    
    return strdup(block_buffer);
}

// UPDATED: Improved command block execution
int execute_command_block(char** commands, int count) {
    for (int i = 0; i < count; i++) {
        if (commands[i] == NULL || strlen(commands[i]) == 0) {
            continue;
        }
        
        // Skip empty lines
        char* cmd = commands[i];
        while (*cmd == ' ' || *cmd == '\t') cmd++;
        if (*cmd == '\0') continue;
        
        char** arglist = tokenize(commands[i]);
        if (arglist != NULL) {
            if (!handle_builtin(arglist)) {
                execute(arglist);
            }
            
            for (int j = 0; arglist[j] != NULL; j++) {
                free(arglist[j]);
            }
            free(arglist);
        }
    }
    return 0;
}

// UPDATED: Completely rewritten handle_if_then_else function
int handle_if_then_else(char* cmdline) {
    // Check if this is an if statement
    if (strncmp(cmdline, "if ", 3) != 0) {
        return 0;
    }
    
    // Extract the condition command
    char* condition_cmd = cmdline + 3;
    while (*condition_cmd == ' ') condition_cmd++;
    
    // Read the multiline block
    char* block = read_multiline_block("");
    if (block == NULL) {
        return -1;
    }
    
    // If block is empty, return
    if (strlen(block) == 0) {
        free(block);
        return 1;
    }
    
    // Parse block into commands
    char* commands[MAX_BLOCK_LINES] = {0};
    int command_count = 0;
    char* saveptr;
    char* command = strtok_r(block, "\n", &saveptr);
    
    while (command != NULL && command_count < MAX_BLOCK_LINES) {
        // Skip empty commands
        char* trimmed_cmd = command;
        while (*trimmed_cmd == ' ' || *trimmed_cmd == '\t') trimmed_cmd++;
        if (strlen(trimmed_cmd) > 0) {
            commands[command_count++] = strdup(trimmed_cmd);
        }
        command = strtok_r(NULL, "\n", &saveptr);
    }
    
    // Execute the condition command
    char** condition_args = tokenize(condition_cmd);
    if (condition_args == NULL) {
        printf("Error: Invalid condition command\n");
        free(block);
        for (int i = 0; i < command_count; i++) {
            free(commands[i]);
        }
        return -1;
    }
    
    int condition_status = 0;
    pid_t pid = fork();
    
    if (pid == 0) {
        // Child process for condition
        if (handle_redirection(condition_args) == -1) {
            exit(1);
        }
        execvp(condition_args[0], condition_args);
        perror("Condition command failed");
        exit(1);
    } else if (pid > 0) {
        // Parent process
        int status;
        waitpid(pid, &status, 0);
        condition_status = WEXITSTATUS(status);
        
        // Free condition arguments
        for (int i = 0; condition_args[i] != NULL; i++) {
            free(condition_args[i]);
        }
        free(condition_args);
    } else {
        perror("fork failed");
        free(block);
        for (int i = 0; i < command_count; i++) {
            free(commands[i]);
        }
        return -1;
    }
    
    // Execute the appropriate block based on condition
    if (condition_status == 0) {
        // Condition succeeded - execute all commands in the block
        execute_command_block(commands, command_count);
    } else {
        // Condition failed - don't execute the block
        printf("Condition failed, skipping then block\n");
    }
    
    // Cleanup
    free(block);
    for (int i = 0; i < command_count; i++) {
        free(commands[i]);
    }
    
    return 1;
}

// Rest of the file remains the same...
char** tokenize(char* cmdline) {
    if (cmdline == NULL || cmdline[0] == '\0' || cmdline[0] == '\n') {
        return NULL;
    }

    char** arglist = (char**)malloc(sizeof(char*) * (MAXARGS + 1));
    for (int i = 0; i < MAXARGS + 1; i++) {
        arglist[i] = (char*)malloc(sizeof(char) * ARGLEN);
        memset(arglist[i], 0, ARGLEN);
    }

    char* cp = cmdline;
    char* start;
    int len;
    int argnum = 0;
    int in_quotes = 0;

    while (*cp != '\0' && argnum < MAXARGS) {
        while (*cp == ' ' || *cp == '\t') cp++;
        
        if (*cp == '\0') break;

        if (*cp == '"') {
            in_quotes = !in_quotes;
            cp++;
            continue;
        }

        if (!in_quotes && (*cp == '<' || *cp == '>' || *cp == '|' || *cp == '&' || *cp == ';')) {
            arglist[argnum][0] = *cp;
            arglist[argnum][1] = '\0';
            argnum++;
            cp++;
            continue;
        }

        start = cp;
        len = 1;
        
        while (*++cp != '\0') {
            if (!in_quotes) {
                if (*cp == ' ' || *cp == '\t' || *cp == '<' || *cp == '>' || *cp == '|' || *cp == '&' || *cp == ';') {
                    break;
                }
                if (*cp == '"') {
                    in_quotes = 1;
                    continue;
                }
            } else {
                if (*cp == '"') {
                    in_quotes = 0;
                    continue;
                }
            }
            len++;
        }
        
        strncpy(arglist[argnum], start, len);
        arglist[argnum][len] = '\0';
        argnum++;
    }

    if (argnum == 0) {
        for(int i = 0; i < MAXARGS + 1; i++) free(arglist[i]);
        free(arglist);
        return NULL;
    }

    arglist[argnum] = NULL;
    return arglist;
}

int handle_chain_commands(char* cmdline) {
    if (strchr(cmdline, ';') == NULL) {
        return 0;
    }
    
    char* saveptr;
    char* command = strtok_r(cmdline, ";", &saveptr);
    int command_count = 0;
    char* commands[MAXARGS] = {0};
    
    while (command != NULL && command_count < MAXARGS) {
        while (*command == ' ' || *command == '\t') command++;
        char* end = command + strlen(command) - 1;
        while (end > command && (*end == ' ' || *end == '\t')) {
            *end = '\0';
            end--;
        }
        
        if (strlen(command) > 0) {
            commands[command_count++] = command;
        }
        command = strtok_r(NULL, ";", &saveptr);
    }
    
    for (int i = 0; i < command_count; i++) {
        char** arglist = tokenize(commands[i]);
        if (arglist != NULL) {
            if (!handle_builtin(arglist)) {
                execute(arglist);
            }
            
            for (int j = 0; arglist[j] != NULL; j++) {
                free(arglist[j]);
            }
            free(arglist);
        }
    }
    
    return 1;
}
