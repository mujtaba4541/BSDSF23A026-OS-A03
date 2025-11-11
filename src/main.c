#include "shell.h"

int main() {
    char* cmdline;
    char** arglist;

    rl_bind_key('\t', rl_complete);
    rl_readline_name = "myshell";
    
    printf("Welcome to MyShell with Command Chaining and Background Jobs!\n");
    printf("Type 'help' for more information.\n\n");

    while (1) {
        // NEW: Clean up completed background jobs before each prompt
        cleanup_background_jobs();
        
        cmdline = read_cmd(PROMPT, stdin);
        
        if (cmdline == NULL) {
            printf("\nShell exited.\n");
            break;
        }
        
        // NEW: Handle command chaining first
        if (strchr(cmdline, ';') != NULL) {
            handle_chain_commands(cmdline);
            free(cmdline);
            continue;
        }
        
        if (cmdline[0] == '!') {
            int hist_num;
            if (sscanf(cmdline + 1, "%d", &hist_num) == 1) {
                int hist_index = execute_from_history(hist_num);
                if (hist_index >= 0) {
                    free(cmdline);
                    cmdline = strdup(history[hist_index]);
                    printf("%s\n", cmdline);
                } else {
                    free(cmdline);
                    continue;
                }
            } else {
                printf("Invalid history command. Use !n where n is history number.\n");
                free(cmdline);
                continue;
            }
        }
        
        if (strlen(cmdline) > 0 && cmdline[0] != '!') {
            add_to_history(cmdline);
        }
        
        if ((arglist = tokenize(cmdline)) != NULL) {
            if (!handle_builtin(arglist)) {
                execute(arglist);
            }

            for (int i = 0; arglist[i] != NULL; i++) {
                free(arglist[i]);
            }
            free(arglist);
        }
        free(cmdline);
    }

    return 0;
}
