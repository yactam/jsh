#ifndef __JOBS_SUPERVISOR_H__
#define __JOBS_SUPERVISOR_H__

#include <stddef.h>
#include <sys/types.h>

#define COMMAND_MAXSIZE 4069

typedef enum {
    BACKGROUND,
    FOREGROUND
} lunch_mode;

typedef enum {
    DONE,
    KILLED,
    DETACHED,
    STOPPED,
    RUNNING
} status;

typedef struct process_node {
    struct process_node *next;
    pid_t pid;
    status process_status;
} process_node;

typedef struct job_node {
    struct job_node *next;
    unsigned int job_id;
    pid_t pgid;
    char command[COMMAND_MAXSIZE];
    status job_status;
    lunch_mode mode;
    size_t num_processes;
    process_node *processes;
} job_node;

typedef struct {
    job_node *head;
    size_t nb_jobs;
} JobList;

extern JobList *jobs_supervisor;

void init_jobs_supervisor();
job_node *add_job(char **args);
void add_process(job_node *job, pid_t pid);
void remove_job(unsigned int job_id);
void free_processes_list(process_node *processes, job_node *job);
job_node *get_job(unsigned int job_id);
job_node *get_job_of_process(pid_t pid);
void free_jobs_supervisor();
int update_job(job_node *, pid_t, int, int);
void check_jobs();
void display_job(job_node *, int fd);

#endif
