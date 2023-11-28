#ifndef __INTERN_COMMANDS_H__
#define __INTERN_COMMANDS_H__

#include <stdlib.h>
#include <sys/types.h>

int pwd();
int cd(const char *);
void jsh_exit();
void jsh_exit_val(int);
int jobs();
int jobs_num(unsigned);
int job_t();
int job_t_num(unsigned);
int bg(unsigned);
int fg(unsigned);
int kill_job(unsigned);
int kill_job_sig(int, unsigned);
int kill_process(pid_t);
int kill_process_sig(int, pid_t);

#endif
