#include "debug.h"
#include "intern_commands.h"
#include "jsh.h"
#include "parser.h"
#include "signal.h"
#include <fcntl.h>
#include <string.h>
#include <sys/wait.h>
#include <unistd.h>

int handle_redirection(char *redirection, char *fic) {
    if (strcmp(redirection, ">") == 0) {
        int write_fd;
        write_fd = open(fic, O_CREAT | O_WRONLY | O_EXCL, 0666);
        if (write_fd < 0) {
            dprintf(STDERR_FILENO, "bash : %s : %s\n", fic, strerror(errno));
            return EXIT_FAILURE;
        }
        dup2(write_fd, STDOUT_FILENO);
        close(write_fd);
    } else if (strcmp(redirection, ">>") == 0) {
        int write_fd;
        write_fd = open(fic, O_CREAT | O_WRONLY | O_APPEND, 0666);
        if (write_fd < 0) {
            dprintf(STDERR_FILENO, "bash : %s : %s\n", fic, strerror(errno));
            return EXIT_FAILURE;
        }
        dup2(write_fd, STDOUT_FILENO);
        close(write_fd);
    } else if (strcmp(redirection, ">|") == 0) {
        int write_fd;
        write_fd = open(fic, O_CREAT | O_WRONLY | O_TRUNC, 0666);
        if (write_fd < 0) {
            dprintf(STDERR_FILENO, "bash : %s : %s\n", fic, strerror(errno));
            return EXIT_FAILURE;
        }
        dup2(write_fd, STDOUT_FILENO);
        close(write_fd);
    } else if (strcmp(redirection, "2>") == 0) {
        int write_fd;
        write_fd = open(fic, O_CREAT | O_WRONLY | O_EXCL, 0666);
        if (write_fd < 0) {
            dprintf(STDERR_FILENO, "bash : %s : %s\n", fic, strerror(errno));
            return EXIT_FAILURE;
        }
        dup2(write_fd, STDERR_FILENO);
        close(write_fd);
    } else if (strcmp(redirection, "2>>") == 0) {
        int write_fd;
        write_fd = open(fic, O_CREAT | O_WRONLY | O_APPEND, 0666);
        if (write_fd < 0) {
            dprintf(STDERR_FILENO, "bash : %s : %s\n", fic, strerror(errno));
            return EXIT_FAILURE;
        }
        dup2(write_fd, STDERR_FILENO);
        close(write_fd);
    } else if (strcmp(redirection, "2>|") == 0) {
        int write_fd;
        write_fd = open(fic, O_CREAT | O_WRONLY | O_TRUNC, 0666);
        if (write_fd < 0) {
            dprintf(STDERR_FILENO, "bash : %s : %s\n", fic, strerror(errno));
            return EXIT_FAILURE;
        }
        dup2(write_fd, STDERR_FILENO);
        close(write_fd);
    } else if (strcmp(redirection, "<") == 0) {
        int input_fd;
        input_fd = open(fic, O_RDONLY);
        if (input_fd < 0) {
            dprintf(STDERR_FILENO, "bash : %s : %s\n", fic, strerror(errno));
            return EXIT_FAILURE;
        }
        dup2(input_fd, STDIN_FILENO);
        close(input_fd);
    }

    return EXIT_SUCCESS;
}

int run_redirection(char **tokens) {

    job_node *job = add_job(tokens);

    int sin = dup(STDIN_FILENO);
    int sout = dup(STDOUT_FILENO);
    int serr = dup(STDERR_FILENO);

    size_t i = 0, end_cmd = 0;
    for (i = 0; tokens[i] != NULL; i++) {
        if (strcmp(tokens[i], "<") == 0 || strcmp(tokens[i], ">") == 0 ||
            strcmp(tokens[i], ">|") == 0 || strcmp(tokens[i], ">>") == 0 ||
            strcmp(tokens[i], "2>") == 0 || strcmp(tokens[i], "2>|") == 0 ||
            strcmp(tokens[i], "2>>") == 0) {
            if (end_cmd == 0)
                end_cmd = i;
            char *redirection = tokens[i];
            char *file = tokens[i + 1];

            if (handle_redirection(redirection, file) == EXIT_FAILURE) {
                dup2(sin, STDIN_FILENO);
                dup2(sout, STDOUT_FILENO);
                dup2(serr, STDERR_FILENO);
                remove_job(job->job_id);
                return EXIT_FAILURE;
            }

            i++;
        }
    }

    size_t j = end_cmd;
    while (tokens[j] != NULL)
        free(tokens[j++]);
    tokens[end_cmd] = NULL;

    command_type type = get_command_type(tokens);
    int res = EXIT_SUCCESS;
    if (type == INTERN_COMMAND) {
        remove_job(job->job_id);
        res = run_intern_command(tokens);
    } else if (type == EXTERN_COMMAND) {
        int pid = fork();
        if (pid == -1) {
            log_error("Erreur fork");
            res = EXIT_FAILURE;
        } else if (pid == 0) {
            reset_signals();
            execvp(tokens[0], tokens);
            dprintf(STDERR_FILENO, "bash: %s: commande inconnue\n", tokens[0]);
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
                    debug("command %s ended", tokens[0]);
                    res = update_job(job, pid, wstatus, STDERR_FILENO);
                } else {
                    res = EXIT_FAILURE;
                }
            } else {
                display_job(job, STDERR_FILENO);
                res = EXIT_SUCCESS;
            }
        }
    }

    dup2(sin, STDIN_FILENO);
    dup2(sout, STDOUT_FILENO);
    dup2(serr, STDERR_FILENO);

    return res;
}