#include "extern_commands.h"
#include "debug.h"
#include "global_variables.h"
#include "jobs_supervisor.h"
#include "parser.h"
#include "signal.h"
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

int run_extern_command(char **args) {
    job_node *job = add_job(args);

    int pid = fork();
    if (pid == -1) {
        log_error("Erreur fork");
        return EXIT_FAILURE;
    } else if (pid == 0) {
        reset_signals();
        execvp(args[0], args);
        dprintf(STDERR_FILENO, "bash: %s: commande inconnue\n", args[0]);
        exit(EXIT_FAILURE);
    } else {
        if (job->pgid == 0) {
            job->pgid = pid; // initialiser le leader pour le job
            if (job->mode == FOREGROUND) {
                tcsetpgrp(STDIN_FILENO, pid);
                tcsetpgrp(STDOUT_FILENO, pid);
                tcsetpgrp(STDERR_FILENO, pid);
            }
        }
        setpgid(pid, job->pgid); // ajouter chaque fils au meme groupe
        add_process(job, pid);
        if (job->mode == FOREGROUND) {
            int wstatus = 0;
            int ret = waitpid(pid, &wstatus, WUNTRACED | WCONTINUED);
            tcsetpgrp(STDIN_FILENO, getpgrp());
            tcsetpgrp(STDOUT_FILENO, getpgrp());
            tcsetpgrp(STDERR_FILENO, getpgrp());
            if (ret > 0) {
                debug("command %s ended", args[0]);
                int r = update_job(job, ret, wstatus, STDERR_FILENO);
                return r;
            }
        } else {
            display_job(job, STDERR_FILENO);
        }
    }
    return EXIT_SUCCESS;
}
