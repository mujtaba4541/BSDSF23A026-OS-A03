#ifndef SHELL_H
#define SHELL_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>

#define MAX_LEN 1024
#define MAXARGS 64
#define ARGLEN 64
#define PROMPT "myshell> "

// Function declarations
char* read_cmd(char* prompt, FILE* fp);
char** tokenize(char* cmdline);
int execute(char* arglist[]);
int handle_builtin(char** arglist);  // NEW: Built-in command handler

#endif
