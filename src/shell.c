#include "shell.h"

char* read_cmd(char* prompt, FILE* fp) {
    (void)fp; // Mark parameter as unused
    
    char* cmdline = readline(prompt);
    
    if (cmdline == NULL) {
        return NULL;
    }
    
    if (cmdline && *cmdline) {
        add_history(cmdline);
    }
    
    return cmdline;
}

// UPDATED: Enhanced tokenize to handle redirection and pipe operators
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
        if (!in_quotes && (*cp == '<' || *cp == '>' || *cp == '|')) {
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
                if (*cp == ' ' || *cp == '\t' || *cp == '<' || *cp == '>' || *cp == '|') {
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
