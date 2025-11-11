#ifndef SHELL_H
#define SHELL_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>              // NEW: For file operations
#include <readline/readline.h>
#include <readline/history.h>

#define MAX_LEN 1024
#define MAXARGS 64
#define ARGLEN 64
#define PROMPT "myshell> "
#define HISTORY_SIZE 20

// Function declarations
char* read_cmd(char* prompt, FILE* fp);
char** tokenize(char* cmdline);
int execute(char* arglist[]);
int handle_builtin(char** arglist);

// History function declarations
void add_to_history(const char* cmdline);
void print_history();
int execute_from_history(int n);

// NEW: Redirection and pipe function declarations
int handle_redirection(char** arglist);
int handle_pipe(char** arglist);
int parse_redirection(char** arglist, char** input_file, char** output_file);

// External declaration of history array
extern char* history[HISTORY_SIZE];
extern int history_count;

#endif
