#include "pipeline.h"
#include "debug.h"
#include "jsh.h"
#include "parser.h"
#include "process_substitution.h"
#include "redirections.h"
#include "signal.h"
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>
#include <unistd.h>
#define BUF_SIZE 512

char ***parse_pipe(char **tokens, int num_cmds) {
    int i = 0;
    char ***cmds = malloc((num_cmds + 1) * sizeof(char **));
    if (!cmds) {
        perror("Memory allocation failed");
        exit(EXIT_FAILURE);
    }

    int cmd_index = 0;
    int cmd_size = 0;

    for (i = 0; tokens[i] != NULL; i++) {
        if (strcmp(tokens[i], "|") == 0) {
            cmds[cmd_index] = malloc((cmd_size + 1) * sizeof(char *));
            if (!cmds[cmd_index]) {
                perror("Memory allocation failed");
                exit(EXIT_FAILURE);
            }

            for (int j = 0; j < cmd_size; j++) {
                cmds[cmd_index][j] = strdup(tokens[i - cmd_size + j]);
                if (cmds[cmd_index][j] == NULL) {
                    perror("Memory allocation failed");
                    exit(EXIT_FAILURE);
                }
            }
            cmds[cmd_index][cmd_size] = NULL;
            cmd_index++;
            cmd_size = 0;
        } else {
            cmd_size++;
        }
    }

    // Handle the last command
    cmds[cmd_index] = malloc((cmd_size + 1) * sizeof(char *));
    if (!cmds[cmd_index]) {
        perror("Memory allocation failed");
        exit(EXIT_FAILURE);
    }

    for (int j = 0; j < cmd_size; j++) {
        cmds[cmd_index][j] = strdup(tokens[(i - 1) - cmd_size + 1 + j]);
        if (cmds[cmd_index][j] == NULL) {
            perror("Memory allocation failed");
            exit(EXIT_FAILURE);
        }
    }
    cmds[cmd_index][cmd_size] = NULL; // Null-terminate the array
    cmd_index++;

    // Null-terminate the cmds array
    cmds[cmd_index] = NULL;
    return cmds;
}

void free_commands(char ***cmds) {
    for (int i = 0; cmds[i] != NULL; i++) {
        for (int j = 0; cmds[i][j] != NULL; j++) {
            free(cmds[i][j]);
        }
        free(cmds[i]);
    }
    free(cmds);
}

int run_process_substitution_aux(char **tokens, job_node *job) {
    size_t l = size_parse_table(tokens);
    int nb_cmds = 0;
    for (size_t i = 0; tokens[i] != NULL; ++i) {
        if (strcmp(tokens[i], "<(") == 0)
            nb_cmds++;
    }

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
        // wait(NULL);
    }

    char f[nb_cmds][BUF_SIZE];
    for (size_t i = 0; i < nb_cmds; ++i) {
        int n = snprintf(f[i], sizeof(f[i]), "/dev/fd/%d", pipes[i][0]);
        f[i][n] = 0;
    }
    // Construct the args table
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

int run_pipe(char **tokens) {
    job_node *job = add_job(tokens);
    debug("call to execute a pipeline");

    int num_pipes = 0, i = 0, is_in_substitution = 0;
    while (tokens[i] != NULL) {
        if (strcmp(tokens[i], "<(") == 0) {
            is_in_substitution = 1;
        } else if (strcmp(tokens[i], ")") == 0) {
            is_in_substitution = 0;
        } else if (strcmp(tokens[i], "|") == 0) {
            if (is_in_substitution) {
                dprintf(STDOUT_FILENO, "123\n");
                remove_job(job->job_id);
                return EXIT_SUCCESS;
            } else {
                num_pipes++;
            }
        }
        i++;
    }

    debug("The number of pipes is %d", num_pipes);
    int pipes[num_pipes][2];
    for (size_t i = 0; i < num_pipes; i++) {
        if (pipe(pipes[i]) == -1) {
            perror("Pipe creation failed");
            exit(EXIT_FAILURE);
        }
    }

    // Parsing des commandes
    int num_cmds = num_pipes + 1;
    char ***cmds = parse_pipe(tokens, num_cmds);

    // Début de traitement
    for (int i = 0; i < num_cmds; i++) {
        pid_t pid = fork();

        if (pid == -1) {
            perror("Fork failed");
            exit(EXIT_FAILURE);
        } else if (pid == 0) {
            reset_signals();
            if (i > 0) {
                close(pipes[i - 1][1]);
                dup2(pipes[i - 1][0], STDIN_FILENO);
                close(pipes[i - 1][0]);
            }

            // Redirect output for all commands except the last
            if (i < num_cmds - 1) {
                close(pipes[i][0]);
                dup2(pipes[i][1], STDOUT_FILENO);
                close(pipes[i][1]);
            }

            // Handle redirections
            int sin = dup(STDIN_FILENO);
            int sout = dup(STDOUT_FILENO);
            int serr = dup(STDERR_FILENO);
            int j = 0;
            while (cmds[i][j] != NULL) {
                if (strcmp(cmds[i][j], "<") == 0 ||
                    strcmp(cmds[i][j], ">") == 0 ||
                    strcmp(cmds[i][j], ">|") == 0 ||
                    strcmp(cmds[i][j], ">>") == 0 ||
                    strcmp(cmds[i][j], "2>") == 0 ||
                    strcmp(cmds[i][j], "2>|") == 0 ||
                    strcmp(cmds[i][j], "2>>") == 0) {
                    char *redirection = cmds[i][j];
                    char *fic = cmds[i][j + 1];
                    if (handle_redirection(redirection, fic) == EXIT_FAILURE) {
                        dprintf(STDERR_FILENO, "Invalid redirection\n");
                        dup2(sin, STDIN_FILENO);
                        dup2(sout, STDOUT_FILENO);
                        dup2(serr, STDERR_FILENO);
                        remove_job(job->job_id);
                        exit(EXIT_FAILURE);
                    }
                    cmds[i][j] = NULL;
                    j++;
                }

                j++;
            }

            // Get the command type
            switch (get_command_type(cmds[i])) {
            case PROCESSUS_SUBSTITUTION:
                debug("call to run process substitution %s", cmds[i][0]);
                run_process_substitution_aux(cmds[i], job);
                exit(EXIT_SUCCESS);

            default:
                execvp(cmds[i][0], cmds[i]);
                perror("Command execution failed");
                exit(EXIT_FAILURE);
            }

        } else {
            if (i == 0) {
                job->pgid = pid;
                if (job->mode == FOREGROUND) {
                    tcsetpgrp(STDIN_FILENO, pid);
                    tcsetpgrp(STDOUT_FILENO, pid);
                    tcsetpgrp(STDERR_FILENO, pid);
                }
            }
            setpgid(pid, job->pgid);
            add_process(job, pid);

            if (i > 0) {
                close(pipes[i - 1][0]);
                close(pipes[i - 1][1]);
            }
        }
    }

    for (int i = 0; i < num_cmds - 1; i++) {
        close(pipes[i][0]);
        close(pipes[i][1]);
    }

    free_commands(cmds);
    int r = EXIT_SUCCESS;
    if (job->mode == FOREGROUND) {
        int wstatus, ret = get_return();
        pid_t pid;
        while ((pid = waitpid(-job->pgid, &wstatus, WUNTRACED | WCONTINUED)) >
               0) {
            debug("here");
            ret = update_job(job, pid, wstatus, STDERR_FILENO);
            set_return(ret);
            if (job->job_status == STOPPED)
                break;
        }
        tcsetpgrp(STDIN_FILENO, getpgrp());
        tcsetpgrp(STDOUT_FILENO, getpgrp());
        tcsetpgrp(STDERR_FILENO, getpgrp());
    } else {
        display_job(job, STDERR_FILENO);
    }

    return r;
}