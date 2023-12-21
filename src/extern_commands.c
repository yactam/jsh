#include "extern_commands.h"
#include "debug.h"
#include "global_variables.h"
#include "parser.h"
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

int run_extern_command(char **tokens) {
    size_t l = size_parse_table(tokens);
    lunch_mode mode = FOREGROUND;

    if (strcmp(tokens[l - 1], "&") == 0) {
        mode = BACKGROUND;
        free(tokens[l - 1]);
        tokens[l - 1] = NULL;
    }

    return exec(tokens[0], tokens, mode);
}

int exec(char *cmd, char **args, lunch_mode mode) {
    int pid = fork();
    if (pid == -1) {
        log_error("Erreur fork");
    } else if (pid == 0) {
        setpgid(0, 0);
        execvp(cmd, args);
        dprintf(STDERR_FILENO, "bash: %s: commande inconnue\n", cmd);
        exit(EXIT_FAILURE);
    } else {
        job_node *job = add_job(args);
        job->mode = mode;
        job->pgid = pid;

        if (mode == FOREGROUND) {
            int wstatus = 0;
            int ret = waitpid(pid, &wstatus, WUNTRACED | WCONTINUED);
            if (ret > 0) {
                int fd = open("/dev/null", O_WRONLY);
                int r = update_job(ret, wstatus, STDERR_FILENO);
                close(fd);
                return r;
            }
        } else {
            display_job(job, STDERR_FILENO);
        }
    }
    return EXIT_SUCCESS;
}
