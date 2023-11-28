#ifndef __GLOBAL_VARIABLES_H__
#define __GLOBAL_VARIABLES_H__

#define PATH_MAXSIZE 1024

extern int last_return;
extern char oldwd[PATH_MAXSIZE];

extern void setReturn(int);
extern void setOldwd(char *);
extern int getReturn();
extern char *getOldwd();

#endif
