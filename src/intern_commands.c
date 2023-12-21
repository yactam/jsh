#include "intern_commands.h"
#include "debug.h"
#include "global_variables.h"
#include "jobs_supervisor.h"
#include "parser.h"
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

int run_intern_command(char **input) {
    char *cmd = input[0];
    debug("call to run command '%s'", cmd);

    if (strcmp(cmd, "pwd") == 0) {
        check(pwd() != EXIT_FAILURE,
              "Erreur d'écriture sur la sortie standard");
        return EXIT_SUCCESS;

    } else if (strcmp(cmd, "cd") == 0) {
        return cd(input);

    } else if (strcmp(cmd, "?") == 0) {
        int ret = getReturn();
        dprintf(STDOUT_FILENO, "%d\n", ret);
        return EXIT_SUCCESS;

    } else if (strcmp(cmd, "exit") == 0) {
        if (jobs_supervisor->nb_jobs > 0) {
            dprintf(STDERR_FILENO, "Il y a des tâches en cours\n");
            return EXIT_FAILURE;
        } else if (input[1] == NULL) {
            free_parse_table(input);
            jsh_exit();
        } else {
            if (input[2] == NULL) {
                int val = atoi(input[1]);
                free_parse_table(input);
                jsh_exit_val(val);
            } else {
                dprintf(STDERR_FILENO, "bash: kill: trop d'arguments\n");
                return EXIT_FAILURE;
            }
        }

    } else if (strcmp(cmd, "jobs") == 0) {
        if (input[1] == NULL) {
            return jobs();
        } else {
            if (input[2] == NULL) {
                char *arg = input[1] + 1; // on élimine le %
                int job_num = atoi(arg);
                jobs_num(job_num);
            } else {
                dprintf(STDERR_FILENO, "bash: kill: trop d'arguments\n");
                return EXIT_FAILURE;
            }
        }

    } else if (strcmp(cmd, "bg") == 0) {
        check(write(STDOUT_FILENO, "nope\n", 5) != -1,
              "Erreur d'écriture sur la sortie standard");

    } else if (strcmp(cmd, "fg") == 0) {
        check(write(STDOUT_FILENO, "nope\n", 5) != -1,
              "Erreur d'écriture sur la sortie standard");

    } else if (strcmp(cmd, "kill") == 0) {
        if (input[2] == NULL) {
            if (input[1][0] == '%') {     // c'est un job
                char *arg = input[1] + 1; // on élimine le %
                int job_num = atoi(arg);
                return kill_job(job_num);
            } else {
                pid_t pid = atoi(input[1]);
                if (pid == 0) {
                    dprintf(STDERR_FILENO,
                            "kill : utilisation :kill [-signal] %%job ou"
                            "kill [-signal] pid\n");
                    return EXIT_FAILURE;
                } else {
                    return kill_process(pid);
                }
            }
        } else if (input[3] == NULL) {
            if (input[2][0] == '%') {     // c'est un job
                char *arg = input[1] + 1; // on élimine le -
                int sig = atoi(arg);
                arg = input[2] + 1; // on élimine le %
                int job_num = atoi(arg);
                return kill_job_sig(sig, job_num);
            } else {
                pid_t pid = atoi(input[2]);
                char *arg = input[1] + 1; // on élimine le -
                int sig = atoi(arg);
                return kill_process_sig(sig, pid);
            }
        } else {
            dprintf(STDERR_FILENO, "bash: kill: trop d'arguments\n");
            return EXIT_FAILURE;
        }
    }

    return EXIT_FAILURE;

error:
    return EXIT_FAILURE;
}

void jsh_exit() {
    jsh_exit_val(getReturn());
}

void jsh_exit_val(int val) {
    debug("call to exit with val %d", val);
    if (jobs_supervisor->nb_jobs > 0) {
        dprintf(STDERR_FILENO, "Il y a des tâches en cours\n");
        setReturn(EXIT_FAILURE);
        return;
    }
    free_jobs_supervisor();
    exit(val);
}

int pwd() {
    char cwd[PATH_MAXSIZE + 2];
    getcwd(cwd, PATH_MAXSIZE);
    strcat(cwd, "\n");
    size_t cwd_len = strlen(cwd);
    if (write(STDOUT_FILENO, cwd, cwd_len) == -1)
        return EXIT_FAILURE;
    return EXIT_SUCCESS;
}

int cd(char **args) {
    char path[PATH_MAXSIZE + 1];
    if (args[1] == NULL) {
        strcpy(path, getenv("HOME"));
    } else if (strcmp(args[1], "-") == 0 && args[2] == NULL) {
        char *oldwd = getenv("OLDPWD");
        if (strcmp(oldwd, "") == 0) {
            dprintf(STDERR_FILENO, "bash: %s: « OLDPWD » non défini\n",
                    args[0]);
            return EXIT_FAILURE;
        }
        strcpy(path, oldwd);
    } else if (args[1] != NULL && args[2] == NULL) {
        strcpy(path, args[1]);
    } else {
        dprintf(STDERR_FILENO, "bash: %s: Trop d'arguments\n", args[0]);
        return EXIT_FAILURE;
    }

    char pwd[PATH_MAXSIZE + 1];
    getcwd(pwd, PATH_MAXSIZE);

    int error_code = chdir(path);
    if (error_code != 0) {
        dprintf(STDERR_FILENO,
                "bash: %s: %s: Aucun fichier ou dossier de ce type\n", args[0],
                args[1]);
        return EXIT_FAILURE;
    }
    setenv("OLDPWD", pwd, 1);
    return EXIT_SUCCESS;
}

int jobs() {
    int wstatus;
    pid_t pid;
    while ((pid = waitpid(-1, &wstatus, WNOHANG | WUNTRACED | WCONTINUED)) >
           0) {
        update_job(pid, wstatus, STDOUT_FILENO);
    }

    job_node *job = jobs_supervisor->head;
    while (job != NULL) {
        display_job(job, STDOUT_FILENO);
        job = job->next;
    }
    return EXIT_SUCCESS;
}

int jobs_num(unsigned int job_id) {
    job_node *job = get_job(job_id);
    if (!job) {
        dprintf(STDERR_FILENO, "bash: jobs: %d : tâche inexistante\n", job_id);
        return EXIT_FAILURE;
    }
    int wstatus;
    pid_t ret = waitpid(job->pgid, &wstatus, WNOHANG | WUNTRACED | WCONTINUED);
    if (ret > 0) {
        update_job(ret, wstatus, STDOUT_FILENO);
    }

    if (job->status == DONE) {
        remove_job(job->job_id);
    }

    return EXIT_SUCCESS;
}

int kill_job(unsigned int job_id) {
    return kill_job_sig(SIGTERM, job_id);
}

int kill_job_sig(int sig, unsigned int job_id) {
    job_node *job = get_job(job_id);
    if (job == NULL) {
        log_info("job %d not found", job_id);
        return EXIT_FAILURE;
    }
    debug("send signal %d to group %d", sig, -job->pgid);
    if (kill(-job->pgid, sig) < 0)
        return EXIT_FAILURE;
    return EXIT_SUCCESS;
}

int kill_process(pid_t pid) {
    return kill(pid, SIGTERM);
}

int kill_process_sig(int sig, pid_t pid) {
    return kill(pid, sig);
}
