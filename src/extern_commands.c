#include "extern_commands.h"
#include "debug.h"
#include "global_variables.h"
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

int exec(char *cmd, char **args) {
    int pid = fork();
    if (pid == -1) {
        log_error("Erreur fork");
    } else if (pid == 0) {
        check(dup2(STDIN_FILENO, STDIN_FILENO) != -1, "Erreur dup2 stdin");
        check(dup2(STDOUT_FILENO, STDOUT_FILENO) != -1, "Erreur dup2 stdout");
        check(dup2(STDERR_FILENO, STDERR_FILENO) != -1, "Erreur dup2 stderr");
        if (execvp(cmd, args) == -1) {
            perror("jsh");
            exit(EXIT_FAILURE);
        }
        return -1;
    } else {
        int wstatus = 0;
        waitpid(pid, &wstatus, 0);
        if (WIFEXITED(wstatus)) {
            int exit_status = WEXITSTATUS(wstatus);
            return exit_status;
        }
    }
error:
    exit(EXIT_FAILURE);
    return -1;
}
