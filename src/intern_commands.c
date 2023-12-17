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
    debug("call to exit with val %d", val);
    exit(val);
}

int pwd() {
    char cwd[PATH_MAXSIZE + 2];
    getcwd(cwd, PATH_MAXSIZE);
    strcat(cwd, "\n");
    size_t cwd_len = strlen(cwd);
    if (write(STDOUT_FILENO, cwd, cwd_len) == -1)
        return 1;
    return 0;
}

int cd(char **args) {
    char path[PATH_MAXSIZE + 1];
    if (args[1] == NULL) {
        strcpy(path, getenv("HOME"));
    } else if (strcmp(args[1], "-") == 0 && args[2] == NULL) {
        char *oldwd = getenv("OLDPWD");
        if (strcmp(oldwd, "") == 0) {
            char err[ERROR_MAXSIZE + 2];
            int err_len =
                snprintf(err, ERROR_MAXSIZE,
                         "bash: %s: « OLDPWD » non défini\n", args[0]);
            check(write(STDERR_FILENO, err, err_len) != -1,
                  "Erreur dans l'écriture sur la sortie d'erreur standard");
            return 1;
        }
        strcpy(path, oldwd);
    } else if (args[1] != NULL && args[2] == NULL) {
        strcpy(path, args[1]);
    } else {
        char err[ERROR_MAXSIZE + 2];
        int err_len = snprintf(err, ERROR_MAXSIZE,
                               "bash: %s: Trop d'arguments\n", args[0]);
        check(write(STDERR_FILENO, err, err_len) != -1,
              "Erreur dans l'écriture sur la sortie d'erreur standard");
    }

    char pwd[PATH_MAXSIZE + 1];
    getcwd(pwd, PATH_MAXSIZE);

    int error_code = chdir(path);
    if (error_code != 0) {
        char err[ERROR_MAXSIZE + 2];
        int err_len =
            snprintf(err, ERROR_MAXSIZE,
                     "bash: %s: %s: Aucun fichier ou dossier de ce type\n",
                     args[0], args[1]);
        check(write(STDERR_FILENO, err, err_len) != -1,
              "Erreur dans l'écriture sur la sortie d'erreur standard");
        return 1;
    }
    setenv("OLDPWD", pwd, 1);
    return 0;
error:
    return 1;
}
