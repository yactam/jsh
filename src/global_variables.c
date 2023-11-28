#include "global_variables.h"
#include <stdio.h>
#include <string.h>

int last_return = 0;
char oldwd[PATH_MAXSIZE] = {0};

void setReturn(int val) {
    last_return = val;
}

int getReturn() {
    return last_return;
}

void setOldwd(char *wd) {
    strncpy(oldwd, wd, PATH_MAXSIZE - 1);
}

char *getOldwd() {
    if (oldwd[0] == 0)
        return NULL;
    return oldwd;
}
