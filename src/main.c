#include "shell.h"

int main() {
    char* cmdline;
    char** arglist;

    while ((cmdline = read_cmd(PROMPT, stdin)) != NULL) {
        // NEW: Handle history re-execution (!n command)
        if (cmdline[0] == '!') {
            int hist_num;
            if (sscanf(cmdline + 1, "%d", &hist_num) == 1) {
                int hist_index = execute_from_history(hist_num);
                if (hist_index >= 0) {
                    // Replace current command with history command
                    free(cmdline);
                    cmdline = strdup(history[hist_index]);
                    printf("%s\n", cmdline); // Echo the command
                } else {
                    free(cmdline);
                    continue; // Skip to next iteration if history command failed
                }
            } else {
                printf("Invalid history command. Use !n where n is history number.\n");
                free(cmdline);
                continue;
            }
        }
        
        // NEW: Add non-empty commands to history (except history commands themselves)
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

    printf("\nShell exited.\n");
    return 0;
}
