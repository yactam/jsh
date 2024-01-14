#ifndef __GLOBAL_VARIABLES_H__
#define __GLOBAL_VARIABLES_H__

#define PATH_MAXSIZE 1024
#define ERROR_MAXSIZE 2048

#include <sys/types.h>

extern int last_return;
extern pid_t shell_pid;

extern void set_return(int);
extern int get_return();
extern void setShellPid(pid_t pid);
extern pid_t getShellPid();

#endif
