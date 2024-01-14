#include "process_substitution.h"
#include "debug.h"
#include "jsh.h"
#include "parser.h"
#include "pipeline.h"
#include "redirections.h"
#include "signal.h"
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>
#include <unistd.h>
#define BUF_SIZE 514

int run_process_substitution(char **tokens) {
    job_node *job = add_job(tokens);
    debug("call to run a process substitution");
    size_t l = size_parse_table(tokens);

    // Parsing step
    int num_pipes = 0;
    for (size_t i = 0; tokens[i] != NULL; ++i) {
        if (strcmp(tokens[i], "|") == 0)
            num_pipes++;
    }
    if (num_pipes > 0) {
        dprintf(STDOUT_FILENO, "123\n");
        remove_job(job->job_id);
        return EXIT_SUCCESS;
    } else {
        // Parsing step
        int nb_cmds = 0;
        for (size_t i = 0; tokens[i] != NULL; ++i) {
            if (strcmp(tokens[i], "<(") == 0)
                nb_cmds++;
        }

        debug("the number of commands is %d", nb_cmds);

        int index[nb_cmds];
        int num_cmd = 0;
        for (size_t i = 0; num_cmd < nb_cmds; i++) {
            if (strcmp(tokens[i], "<(") == 0) {
                index[num_cmd] = i + 1;
            } else if (strcmp(tokens[i], ")") == 0) {
                num_cmd++;
                tokens[i] = NULL;
            }
        }

        // Execution step
        int pipes[nb_cmds][2];
        for (int i = 0; i < nb_cmds; i++) {
            if (pipe(pipes[i]) == -1) {
                perror("Memory allocation failed");
                exit(EXIT_FAILURE);
            }
        }

        for (size_t i = 0; i < nb_cmds; i++) {
            pid_t pid = fork();
            if (pid == -1) {
                perror("Fork failed");
                exit(EXIT_FAILURE);
            } else if (pid == 0) {
                reset_signals();
                // Handle pipelines
                int num_pipes = 0, p = index[i];
                while (tokens[p] != NULL) {
                    if (strcmp(tokens[p], "|") == 0)
                        num_pipes++;
                    p++;
                }

                if (num_pipes > 0) {
                    dprintf(STDERR_FILENO,
                            "pipes in process substitutions are not handled\n");
                    exit(EXIT_FAILURE);
                } else {
                    int cmd_index = index[i];

                    int j = cmd_index;
                    while (tokens[j] != NULL) {
                        if (strcmp(tokens[j], "<") == 0 ||
                            strcmp(tokens[j], ">") == 0 ||
                            strcmp(tokens[j], ">|") == 0 ||
                            strcmp(tokens[j], ">>") == 0 ||
                            strcmp(tokens[j], "2>") == 0 ||
                            strcmp(tokens[j], "2>|") == 0 ||
                            strcmp(tokens[j], "2>>") == 0) {
                            char *redirection = tokens[j];
                            char *fic = tokens[j + 1];
                            handle_redirection(redirection, fic);
                            tokens[j] = NULL;
                            j++;
                        }

                        j++;
                    }
                    close(pipes[i][0]);
                    dup2(pipes[i][1], STDOUT_FILENO);
                    close(pipes[i][1]);

                    int r = execvp(tokens[cmd_index], &tokens[index[i]]);
                    if (r == EXIT_FAILURE) {
                        return r;
                    }
                }
            } else {
                setpgid(pid, job->pgid);
                add_process(job, pid);
            }
        }

        for (size_t i = 0; i < nb_cmds; ++i) {
            close(pipes[i][1]);
        }

        if (job->mode == FOREGROUND) {
            int wstatus;
            for (size_t i = 0; i < nb_cmds; ++i) {
                pid_t pid = wait(&wstatus);
                update_job(job, pid, wstatus, STDERR_FILENO);
            }
            remove_job(job->job_id);
        }

        debug("nb cmds = %d", nb_cmds);

        char f[nb_cmds][BUF_SIZE];
        for (size_t i = 0; i < nb_cmds; ++i) {
            int n = snprintf(f[i], sizeof(f[i]), "/dev/fd/%d", pipes[i][0]);
            f[i][n] = 0;
        }

        // Construct the args table
        debug("l = %ld", l);
        size_t size_args = 0;
        for (size_t i = 0; i < l; i++) {
            if (tokens[i] == NULL)
                continue;
            else if (strcmp(tokens[i], "<(") == 0) {
                size_args++; // for /dev/fd/%d
                while (tokens[i] != NULL)
                    i++;
            } else {
                size_args++; // for a token
            }
        }

        size_args++; // for NULL

        char **args = (char **)malloc(size_args * sizeof(char *));
        if (args == NULL) {
            perror("malloc");
            exit(EXIT_FAILURE);
        }

        size_t next_command_index = 0, next_arg_index = 0;
        for (size_t k = 0; k < l; k++) {
            debug("tokens = %s", tokens[k]);
            if (tokens[k] == NULL) {
                continue;
            } else if (strcmp(tokens[k], "<(") == 0) {
                args[next_arg_index++] = strdup(f[next_command_index++]);
                while (tokens[k] != NULL) {
                    k++;
                }
            } else {
                args[next_arg_index++] = strdup(tokens[k]);
            }
        }

        args[size_args - 1] = NULL;

        pid_t pid = fork();
        if (pid == -1) {
            perror("fork");
            exit(EXIT_FAILURE);
        }

        if (pid == 0) {
            execvp(args[0], args);
            perror("execvp");
            return EXIT_FAILURE;
        } else {
            wait(NULL);
            for (size_t k = 0; k < size_args; k++) {
                free(args[k]);
            }
            free(args);
        }

        return EXIT_SUCCESS;
    }
}