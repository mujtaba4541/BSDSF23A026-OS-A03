#include "shell.h"

char* read_cmd(char* prompt, FILE* fp) {
    // NEW: Use readline instead of manual input reading
    // Note: fp parameter is kept for compatibility but not used with readline
    (void)fp; // Explicitly mark parameter as unused to avoid warning
    
    char* cmdline = readline(prompt);
    
    if (cmdline == NULL) {
        return NULL; // Handle Ctrl+D
    }
    
    // NEW: Add non-empty commands to readline's history
    if (cmdline && *cmdline) {
        add_history(cmdline); // Add to readline's history for up/down arrows
    }
    
    return cmdline;
}

// Keep tokenize function unchanged
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

    while (*cp != '\0' && argnum < MAXARGS) {
        while (*cp == ' ' || *cp == '\t') cp++;
        
        if (*cp == '\0') break;

        start = cp;
        len = 1;
        while (*++cp != '\0' && !(*cp == ' ' || *cp == '\t')) {
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
