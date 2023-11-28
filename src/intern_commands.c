#include "intern_commands.h"
#include "debug.h"
#include "global_variables.h"
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

void jsh_exit() {
    jsh_exit_val(getReturn());
}

void jsh_exit_val(int val) {
    exit(val);
}

int cd(const char *arg) {
    char path[PATH_MAXSIZE];
    memset(path, '\0', sizeof(path));

    if (arg == NULL) {
        strcpy(path, getenv("HOME"));
    } else if (strcmp(arg, "-") == 0) {
        strcpy(path, getenv("OLDPWD"));
    } else {
        strcpy(path, arg);
    }

    int error_code = chdir(path);
    if (error_code != 0) {
        log_error("Failed to change directory with arg=%s (param=%s)", arg,
                  path);
    }

    return error_code;
}