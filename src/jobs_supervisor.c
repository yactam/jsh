#include "jobs_supervisor.h"
#include "debug.h"
#include "global_variables.h"
#include "parser.h"
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/wait.h>
#include <unistd.h>

JobList *jobs_supervisor;

void init_jobs_supervisor() {
    debug("call to init the jobs supervisor");
    jobs_supervisor = malloc(sizeof(JobList));
    jobs_supervisor->head = NULL;
    jobs_supervisor->nb_jobs = 0;
}

job_node *add_job(char **args) {
    debug("call to add a new job");
    size_t l = size_parse_table(args);
    lunch_mode mode = FOREGROUND;

    if (strcmp(args[l - 1], "&") == 0) {
        mode = BACKGROUND;
        free(args[l - 1]);
        args[l - 1] = NULL;
    }

    job_node *job = (job_node *)malloc(sizeof(job_node));
    check_mem(job);

    job->next = NULL;
    job->pgid = 0;
    job->num_processes = 0;
    job->processes = NULL;

    size_t i = 0, j = 0;
    while (args[i] != NULL && j < COMMAND_MAXSIZE) {
        for (size_t k = 0; k < strlen(args[i]); k++) {
            job->command[j++] = args[i][k];
        }
        if (args[i + 1] != NULL)
            job->command[j++] = ' ';
        else
            job->command[j] = 0;
        i++;
    }

    job->job_status = RUNNING;
    job->mode = mode;

    if (jobs_supervisor->head == NULL) {
        job->job_id = 1;
        jobs_supervisor->head = job;
    } else {
        job_node *curr = jobs_supervisor->head;
        while (curr->next != NULL) {
            curr = curr->next;
        }
        job->job_id = curr->job_id + 1;
        curr->next = job;
    }

    jobs_supervisor->nb_jobs++;
    debug("job %d added successfully", job->job_id);
    return job;

error:
    log_error("Failing to allocate memory");
    return NULL;
}

void add_process(job_node *job, pid_t pid) {
    process_node *process = (process_node *)malloc(sizeof(process_node));
    check_mem(process);

    process->pid = pid;
    process->process_status = RUNNING;
    process->next = job->processes;
    job->processes = process;
    job->num_processes++;
    return;

error:
    log_error("Failing to allocate memory");
}

job_node *get_job(unsigned int id) {
    debug("call to get the job %d", id);
    job_node *curr = jobs_supervisor->head;
    while (curr != NULL && curr->job_id != id) {
        curr = curr->next;
    }

    if (curr == NULL)
        debug("job not found");
    else
        debug("job was found");

    return curr;
}

job_node *get_job_of_process(pid_t pid) {
    job_node *curr_job = jobs_supervisor->head;

    while (curr_job != NULL) {
        process_node *curr_process = curr_job->processes;

        while (curr_process != NULL) {
            if (curr_process->pid == pid) {
                return curr_job;
            }

            curr_process = curr_process->next;
        }

        curr_job = curr_job->next;
    }

    return NULL;
}

void remove_job(unsigned int id) {
    debug("call to remove the job %d", id);
    job_node *curr = jobs_supervisor->head;
    job_node *prev = NULL;

    while (curr != NULL && curr->job_id != id) {
        prev = curr;
        curr = curr->next;
    }

    if (curr == NULL) {
        log_info("The job %d wasn't found", id);
        return;
    }

    if (prev == NULL) {
        jobs_supervisor->head = curr->next;
    } else {
        prev->next = curr->next;
    }

    free_processes_list(curr->processes, curr);
    free(curr);

    jobs_supervisor->nb_jobs--;
}

void free_processes_list(process_node *processes, job_node *job) {
    process_node *curr = processes;
    process_node *next = NULL;

    while (curr != NULL) {
        next = curr->next;
        free(curr);
        curr = next;
        job->num_processes--;
    }
}

void free_jobs_supervisor() {
    job_node *curr = jobs_supervisor->head;
    job_node *next = NULL;

    while (curr != NULL) {
        next = curr->next;
        free_processes_list(curr->processes, curr);
        free(curr);
        curr = next;
    }

    jobs_supervisor->head = NULL;
    free(jobs_supervisor);
}

void display_job(job_node *job, int fd) {

    char *status_str;

    switch (job->job_status) {
    case DONE:
        status_str = "Done";
        break;
    case KILLED:
        status_str = "Killed";
        break;
    case DETACHED:
        status_str = "Detached";
        break;
    case STOPPED:
        status_str = "Stopped";
        break;
    case RUNNING:
        status_str = "Running";
        break;
    default:
        status_str = "Unknown";
        break;
    }

    dprintf(fd, "[%d] %d %s %s\n", job->job_id, (int)job->pgid, status_str,
            job->command);
}

int update_job(job_node *job, pid_t pid, int wstatus, int out_fd) {
    if (job == NULL)
        return -1;
    process_node *curr = job->processes;
    int ret = get_return();
    while (curr != NULL) {
        if (curr->pid == pid) {
            if (WIFEXITED(wstatus)) {
                curr->process_status = DONE;
                ret = WEXITSTATUS(wstatus);
            } else if (WIFSIGNALED(wstatus)) {
                curr->process_status = KILLED;
            } else if (WIFSTOPPED(wstatus)) {
                curr->process_status = STOPPED;
            } else if (WIFCONTINUED(wstatus)) {
                curr->process_status = RUNNING;
                job->mode = BACKGROUND;
            }
            break;
        }
        curr = curr->next;
    }

    int nb_done = 0, nb_stopped = 0, nb_killed = 0;
    process_node *curr_process = job->processes;
    while (curr_process != NULL) {
        if (curr_process->process_status == DONE) {
            nb_done++;
        } else if (curr_process->process_status == KILLED) {
            nb_killed++;
        } else if (curr_process->process_status == STOPPED) {
            nb_stopped++;
        }
        curr_process = curr_process->next;
    }

    debug("nb_processes = %zu, nb_done = %d, nb_killed = %d, nb_stopped = %d",
          job->num_processes, nb_done, nb_killed, nb_stopped);

    status last_status = job->job_status;

    if (job->num_processes == nb_done) {
        if (kill(-job->pgid, 0) < 0)
            job->job_status = DONE;
        else
            job->job_status = DETACHED;
    } else if ((job->num_processes == nb_done + nb_killed) && nb_killed > 0)
        job->job_status = KILLED;
    else if (job->num_processes - nb_done == nb_stopped)
        job->job_status = STOPPED;
    else
        job->job_status = RUNNING;

    if (job->mode == BACKGROUND ||
        (job->job_status != DONE && job->job_status != KILLED &&
         job->job_status != DETACHED)) {
        if (last_status != job->job_status) {
            display_job(job, out_fd);
            set_return(ret);
        }
    }

    return ret;
}

void check_jobs() {
    job_node *job = jobs_supervisor->head;
    int wstatus;
    pid_t pid;
    int ret = get_return();
    while (job != NULL) {
        job_node *next = job->next;
        while ((pid = waitpid(-job->pgid, &wstatus,
                              WNOHANG | WUNTRACED | WCONTINUED)) > 0) {
            debug("job with pid %d changed status", pid);
            ret = update_job(job, pid, wstatus, STDERR_FILENO);
            set_return(ret);
        }
        if (pid < 0 && errno == ECHILD) {
            if (kill(-job->pgid, 0) == 0)
                job->job_status = DETACHED;
        }

        if (job->job_status == DONE || job->job_status == KILLED ||
            job->job_status == DETACHED) {
            remove_job(job->job_id);
        }
        job = next;
    }
}
