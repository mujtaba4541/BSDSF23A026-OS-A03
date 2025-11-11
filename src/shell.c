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

// UPDATED: Enhanced tokenize to handle background operator (&)
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

        // Handle quoted strings
        if (*cp == '"') {
            in_quotes = !in_quotes;
            cp++;
            continue;
        }

        // Handle special operators outside quotes
        if (!in_quotes && (*cp == '<' || *cp == '>' || *cp == '|' || *cp == '&' || *cp == ';')) {
            arglist[argnum][0] = *cp;
            arglist[argnum][1] = '\0';
            argnum++;
            cp++;
            continue;
        }

        start = cp;
        len = 1;
        
        // Read until special character or whitespace (unless in quotes)
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

// NEW: Handle command chaining with semicolons
int handle_chain_commands(char* cmdline) {
    if (strchr(cmdline, ';') == NULL) {
        return 0; // No chaining needed
    }
    
    char* saveptr;
    char* command = strtok_r(cmdline, ";", &saveptr);
    int command_count = 0;
    char* commands[MAXARGS] = {0};
    
    // Split commands by semicolon
    while (command != NULL && command_count < MAXARGS) {
        // Trim leading/trailing whitespace
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
    
    // Execute each command sequentially
    for (int i = 0; i < command_count; i++) {
        char** arglist = tokenize(commands[i]);
        if (arglist != NULL) {
            if (!handle_builtin(arglist)) {
                execute(arglist);
            }
            
            // Free the memory allocated by tokenize()
            for (int j = 0; arglist[j] != NULL; j++) {
                free(arglist[j]);
            }
            free(arglist);
        }
    }
    
    return 1; // Chaining was handled
}
