#include "intern_commands.h"
#include "debug.h"
#include "global_variables.h"
#include "jobs_supervisor.h"
#include "parser.h"
#include <dirent.h>
#include <fcntl.h>
#include <readline/history.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#define MAX_PATH_LEN 4096
#define MAX_STAT_LEN 4096

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
        int ret = get_return();
        dprintf(STDOUT_FILENO, "%d\n", ret);
        return EXIT_SUCCESS;

    } else if (strcmp(cmd, "exit") == 0) {
        if (jobs_supervisor->nb_jobs > 0) {
            dprintf(STDERR_FILENO, "Il y a des tâches en cours\n");
            return EXIT_FAILURE;
        } else if (input[1] == NULL) {
            free_parse_table(input);
            clear_history();
            jsh_exit();
        } else {
            if (input[2] == NULL) {
                int val = atoi(input[1]);
                free_parse_table(input);
                clear_history();
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
            if (strcmp(input[1], "-t") == 0) {
                if (input[2] == NULL) {
                    return job_t();
                } else {
                    if (input[3] == NULL) {
                        char *arg = input[2] + 1; // on élimine le %
                        int job_num = atoi(arg);
                        job_t_num(job_num);
                    } else {
                        dprintf(STDERR_FILENO,
                                "bash: jobs: trop d'arguments\n");
                        return EXIT_FAILURE;
                    }
                }
            } else {
                if (input[2] == NULL) {
                    char *arg = input[1] + 1; // on élimine le %
                    int job_num = atoi(arg);
                    jobs_num(job_num);
                } else {
                    dprintf(STDERR_FILENO, "bash: jobs: trop d'arguments\n");
                    return EXIT_FAILURE;
                }
            }
        }

    } else if (strcmp(cmd, "bg") == 0) {
        if (input[1] == NULL) {
            return bg(jobs_supervisor->nb_jobs);
        } else if (input[2] == NULL) {
            char *arg = input[1] + 1;
            int job_num = atoi(arg);
            return bg(job_num);
        } else {
            dprintf(STDERR_FILENO, "bash: bg: trop d'arguments\n");
            return EXIT_FAILURE;
        }
    } else if (strcmp(cmd, "fg") == 0) {
        if (input[1] == NULL) {
            return fg(jobs_supervisor->nb_jobs);
        } else if (input[2] == NULL) {
            char *arg = input[1] + 1;
            int job_num = atoi(arg);
            return fg(job_num);
        } else {
            dprintf(STDERR_FILENO, "bash: bg: trop d'arguments\n");
            return EXIT_FAILURE;
        }
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
                            "kill : utilisation :kill [-signal] %%job ou "
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
    jsh_exit_val(get_return());
}

void jsh_exit_val(int val) {
    debug("call to exit with val %d", val);

    if (jobs_supervisor->nb_jobs > 0) {
        dprintf(STDERR_FILENO, "Il y a des tâches en cours\n");
        set_return(EXIT_FAILURE);
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

    job_node *job = jobs_supervisor->head;
    int ret = get_return();
    while (job != NULL) {
        job_node *next = job->next;
        while ((pid = waitpid(-job->pgid, &wstatus,
                              WNOHANG | WUNTRACED | WCONTINUED)) > 0) {
            ret = update_job(job, pid, wstatus, STDOUT_FILENO);
            set_return(ret);
            debug("update job ended");
        }
        if (job->job_status == DONE || job->job_status == KILLED) {
            remove_job(job->job_id);
        }
        job = next;
    }

    job = jobs_supervisor->head;
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
    pid_t pid;
    int ret = get_return();
    while ((pid = waitpid(-job->pgid, &wstatus,
                          WNOHANG | WUNTRACED | WCONTINUED)) > 0) {
        ret = update_job(job, pid, wstatus, STDOUT_FILENO);
        set_return(ret);
        debug("update job ended");
    }
    if (job->job_status == DONE || job->job_status == KILLED) {
        remove_job(job->job_id);
    }

    return EXIT_SUCCESS;
}

typedef struct {
    pid_t pid;
    pid_t ppid;
    pid_t pgid;
    char status;
    char command[MAX_PATH_LEN];
} ProcessInfo;

int get_process_info(pid_t pid, ProcessInfo *info) {
    char proc_path[MAX_PATH_LEN];
    snprintf(proc_path, sizeof(proc_path), "/proc/%d/stat", pid);
    int fd = open(proc_path, O_RDONLY);
    if (fd == -1) {
        perror("Error opening stat file");
        return -1;
    }
    char stat_buf[MAX_STAT_LEN];
    ssize_t read_count = read(fd, stat_buf, sizeof(stat_buf) - 1);
    close(fd);

    if (read_count == -1) {
        perror("Error reading stat file");
        return -1;
    }

    stat_buf[read_count] = '\0';

    char **tokens = parse_line(stat_buf, ' ');
    if (tokens == NULL)
        return -1;
    info->pid = atoi(tokens[0]);
    for (size_t i = 0; i < strlen(tokens[1]); ++i) {
        info->command[i] = stat_buf[i];
    }
    info->status = tokens[2][0];
    info->ppid = atoi(tokens[3]);
    info->pgid = atoi(tokens[4]);

    return 0;
}

void display_process_tree(pid_t leader) {
    ProcessInfo leader_info;
    ProcessInfo current_info;
    if (get_process_info(leader, &leader_info) == 0) {
        char str_status[512];
        switch (current_info.status) {
        case 'T':
            strcpy(str_status, "STOPPED");
            break;
        case 'R':
            strcpy(str_status, "RUNNING");
            break;
        default:
            strcpy(str_status, "UNKNOWN");
            break;
        }
        printf("\t| %d %s %s\n", leader_info.pid, str_status,
               leader_info.command);
    }

    DIR *proc_dir = opendir("/proc");
    if (!proc_dir) {
        perror("Error opening /proc directory");
        return;
    }

    struct dirent *entry;
    while ((entry = readdir(proc_dir)) != NULL) {
        if (entry->d_type == DT_DIR) {
            pid_t pid = atoi(entry->d_name);

            if (pid > 0) {
                if (get_process_info(pid, &current_info) == 0 &&
                    current_info.pgid == leader && current_info.pid != leader) {
                    char str_status[512];
                    switch (current_info.status) {
                    case 'T':
                        strcpy(str_status, "STOPPED");
                        break;
                    case 'R':
                        strcpy(str_status, "RUNNING");
                        break;
                    default:
                        strcpy(str_status, "UNKNOWN");
                        break;
                    }
                    printf("\t%d %s %s\n", current_info.pid, str_status,
                           current_info.command);
                }
            }
        }
    }
    closedir(proc_dir);
}

int job_t() {
    job_node *job = jobs_supervisor->head;
    while (job != NULL) {
        job_t_num(job->job_id);
        job = job->next;
    }
    return EXIT_SUCCESS;
}

int job_t_num(unsigned job_id) {
    job_node *job = get_job(job_id);
    if (!job) {
        return EXIT_FAILURE;
    }
    int wstatus;
    pid_t pid;
    int ret = get_return();
    while ((pid = waitpid(-job->pgid, &wstatus,
                          WNOHANG | WUNTRACED | WCONTINUED)) > 0) {
        ret = update_job(job, pid, wstatus, STDERR_FILENO);
        set_return(ret);
    }

    pid_t leader = job->pgid;
    display_job(job, STDOUT_FILENO);
    display_process_tree(leader);
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

int bg(unsigned job_id) {
    job_node *job = get_job(job_id);
    if (job == NULL) {
        log_info("job %d not found", job_id);
        return EXIT_FAILURE;
    }
    if (kill(-job->pgid, SIGCONT) < 0) {
        return EXIT_FAILURE;
    }
    job->mode = BACKGROUND;
    return EXIT_SUCCESS;
}

int fg(unsigned job_id) {
    job_node *job = get_job(job_id);
    if (job == NULL) {
        log_info("job %d not found", job_id);
        return EXIT_FAILURE;
    }

    if (kill(-job->pgid, SIGCONT) < 0) {
        return EXIT_FAILURE;
    }
    job->mode = FOREGROUND;
    job->job_status = RUNNING;
    tcsetpgrp(STDIN_FILENO, job->pgid);
    tcsetpgrp(STDOUT_FILENO, job->pgid);
    tcsetpgrp(STDERR_FILENO, job->pgid);
    int wstatus, ret = get_return();
    int fd = open("/dev/null", O_WRONLY);
    pid_t pid;
    while ((pid = waitpid(-job->pgid, &wstatus, WUNTRACED | WCONTINUED)) > 0) {
        ret = update_job(job, pid, wstatus, fd);
        set_return(ret);
        if (job->job_status == STOPPED) {
            display_job(job, STDERR_FILENO);
            break;
        }
    }
    tcsetpgrp(STDIN_FILENO, getpgrp());
    tcsetpgrp(STDOUT_FILENO, getpgrp());
    tcsetpgrp(STDERR_FILENO, getpgrp());

    close(fd);
    return EXIT_SUCCESS;
}
