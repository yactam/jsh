#include "extern_commands.h"
#include "debug.h"
#include "global_variables.h"
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

int run_extern_command(char **tokens) {
    return exec(tokens[0], tokens);
}

int exec(char *cmd, char **args) {
    int pid = fork();
    if (pid == -1) {
        log_error("Erreur fork");
    } else if (pid == 0) {
        execvp(cmd, args);
        char error[ERROR_MAXSIZE];
        snprintf(error, ERROR_MAXSIZE - 2, "bash: %s: commande inconnue\n",
                 cmd);
        check(write(STDERR_FILENO, error, strlen(error)) != -1,
              "Erreur dans l'écriture sur la sortie d'erreur standard.");
        exit(EXIT_FAILURE);
    } else {
        int wstatus = 0;
        waitpid(pid, &wstatus, 0);
        if (WIFEXITED(wstatus)) {
            int exit_status = WEXITSTATUS(wstatus);
            return exit_status;
        }
    }
    return EXIT_FAILURE;
error:
    return EXIT_FAILURE;
}