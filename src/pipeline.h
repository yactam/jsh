#ifndef __PIPELINE_H__
#define __PIPELINE_H__

#include "jobs_supervisor.h"

char ***parse_pipe(char **, int);
void free_commands(char ***);
int run_pipe(char **);

#endif