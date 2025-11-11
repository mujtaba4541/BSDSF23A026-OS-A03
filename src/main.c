#include "shell.h"

int main() {
    char* cmdline;
    char** arglist;

    // NEW: Configure readline for better user experience
    rl_bind_key('\t', rl_complete); // Enable tab completion
    
    // Optional: Set readline behavior
    rl_readline_name = "myshell"; // Name for conditional parsing in .inputrc
    
    printf("Welcome to MyShell with Readline support!\n");
    printf("Features: Tab completion, History navigation with Up/Down arrows\n");
    printf("Type 'help' for more information.\n\n");

    while (1) {
        cmdline = read_cmd(PROMPT, stdin);
        
        if (cmdline == NULL) {
            printf("\nShell exited.\n");
            break;
        }
        
        // Handle history re-execution (!n command)
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
        
        // Add to our internal history (except history commands themselves)
        if (strlen(cmdline) > 0 && cmdline[0] != '!') {
            add_to_history(cmdline);
        }
        
        if ((arglist = tokenize(cmdline)) != NULL) {
            if (!handle_builtin(arglist)) {
                execute(arglist);
            }

            // Free the memory allocated by tokenize()
            for (int i = 0; arglist[i] != NULL; i++) {
                free(arglist[i]);
            }
            free(arglist);
        }
        free(cmdline);
    }

    return 0;
}
