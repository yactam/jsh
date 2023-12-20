#include "jobs_supervisor.h"
#include "debug.h"
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

JobList *jobs_supervisor;

void init_jobs_supervisor() {
	debug("call to init the jobs supervisor");
	jobs_supervisor = malloc(sizeof(JobList));
	jobs_supervisor->head = NULL;
	jobs_supervisor->nb_jobs = 0;
}

job_node *add_job(char *cmd) {
	debug("call to add a new job to the jobs list with cmd %s", cmd);
	job_node *job = (job_node *) malloc(sizeof(job_node));
	check_mem(job);

	job->next = NULL;
	job->pgid = 0;
	strcpy(job->command, cmd);
	job->status = RUNNING;
	job->mode = BACKGROUND;

	if(jobs_supervisor->head == NULL) {
		job->job_id = 1;
		jobs_supervisor->head = job;
	} else {
		job_node *curr = jobs_supervisor->head;
		while(curr->next != NULL) {
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

job_node *get_job(unsigned int id) {
	debug("call to get the job %d", id);
	job_node *curr = jobs_supervisor->head;
	while(curr != NULL && curr->job_id != id) {
		curr = curr->next;
	}

	if(curr == NULL) debug("job not found"); else debug("job was found");
	
	return curr;
}

void remove_job(unsigned int id) {
	debug("call to remove the job %d", id);
	job_node *curr = jobs_supervisor->head;
	job_node *prev = NULL;

	while(curr != NULL && curr->job_id != id) {
		prev = curr;
		curr = curr->next;
	}

	if(curr == NULL) {
		log_info("The job %d wasn't found", id);
		return;
	}

	if(prev == NULL) {
		jobs_supervisor->head = curr->next;
	} else {
		prev->next = curr->next;
	}

	free(curr);

	jobs_supervisor->nb_jobs--;
}

void free_jobs_supervisor() {
	job_node *curr = jobs_supervisor->head;
	job_node *next = NULL;

	while(curr != NULL) {
		next = curr->next;
		free(curr);
		curr = next;
	}

	jobs_supervisor->head = NULL;
	free(jobs_supervisor);
}

void display_job(job_node *job, int fd) {
	char *status_str;

	 switch (job->status) {
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
