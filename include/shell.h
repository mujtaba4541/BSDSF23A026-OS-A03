#ifndef SHELL_H
#define SHELL_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <readline/readline.h>
#include <readline/history.h>

#define MAX_LEN 1024
#define MAXARGS 64
#define ARGLEN 64
#define PROMPT "myshell> "
#define HISTORY_SIZE 20
#define MAX_JOBS 20
#define MAX_BLOCK_LINES 20        // NEW: Maximum lines in if-then-else block

// Function declarations
char* read_cmd(char* prompt, FILE* fp);
char** tokenize(char* cmdline);
int execute(char* arglist[]);
int handle_builtin(char** arglist);

// History function declarations
void add_to_history(const char* cmdline);
void print_history();
int execute_from_history(int n);

// Redirection and pipe function declarations
int handle_redirection(char** arglist);
int handle_pipe(char** arglist);
int parse_redirection(char** arglist, char** input_file, char** output_file);

// Command chaining and background job declarations
int handle_chain_commands(char* cmdline);
int handle_background(char** arglist);
void cleanup_background_jobs();
void print_jobs();

// NEW: If-then-else-fi control structure declarations
int handle_if_then_else(char* cmdline);
char* read_multiline_block(const char* prompt);
int execute_command_block(char** commands, int count);

// External declaration of history array
extern char* history[HISTORY_SIZE];
extern int history_count;

// External declarations for job control
extern pid_t background_jobs[MAX_JOBS];
extern int job_count;
extern char* job_commands[MAX_JOBS];

#endif
