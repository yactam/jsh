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
} job_status;

typedef struct job_node {
    struct job_node *next;
    unsigned int job_id;
    pid_t pgid;
    char command[COMMAND_MAXSIZE];
    job_status status;
    lunch_mode mode;
} job_node;

typedef struct {
    job_node *head;
    size_t nb_jobs;
} JobList;

extern JobList *jobs_supervisor;

void init_jobs_supervisor();
job_node *add_job(char **args);
void remove_job(unsigned int job_id);
job_node *get_job(unsigned int job_id);
void free_jobs_supervisor();
int update_job(pid_t, int, int);
void check_jobs();
void display_job(job_node *, int fd);

#endif
