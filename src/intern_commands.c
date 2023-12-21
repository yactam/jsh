#include "intern_commands.h"
#include "debug.h"
#include "global_variables.h"
#include "parser.h"
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

int run_intern_command(char **input) {
    char *cmd = input[0];
    debug("call to run command '%s'", cmd);

    if (strcmp(cmd, "pwd") == 0) {
        check(pwd() != EXIT_FAILURE, "Erreur d'écriture sur la sortie standard");
        return EXIT_SUCCESS;

    } else if (strcmp(cmd, "cd") == 0) {
        return cd(input);

    } else if (strcmp(cmd, "?") == 0) {
        char buf[4069];
        int n = snprintf(buf, sizeof(buf) - 2, "%d\n", getReturn());
        check(write(STDOUT_FILENO, buf, n) != -1,
              "Erreur d'écriture sur la sortie standard");
        return EXIT_SUCCESS;

    } else if (strcmp(cmd, "exit") == 0) {
        if (input[1] == NULL) {
            free_parse_table(input);
            jsh_exit();
        } else {
            if (input[2] == NULL) {
                int val = atoi(input[1]);
                free_parse_table(input);
                jsh_exit_val(val);
            } else {
                char *err = "jsh: exit: trop d'arguments\n";
                check(write(STDERR_FILENO, err, strlen(err)),
                      "Erreur d'écriture sur la sortie d'erreurs standard");
            }
        }

    } else if (strcmp(cmd, "jobs") == 0) {
        check(write(STDOUT_FILENO, "nope\n", 5) != -1,
              "Erreur d'écriture sur la sortie standard");

    } else if (strcmp(cmd, "bg") == 0) {
        check(write(STDOUT_FILENO, "nope\n", 5) != -1,
              "Erreur d'écriture sur la sortie standard");

    } else if (strcmp(cmd, "fg") == 0) {
        check(write(STDOUT_FILENO, "nope\n", 5) != -1,
              "Erreur d'écriture sur la sortie standard");

    } else if (strcmp(cmd, "kill") == 0) {
        check(write(STDOUT_FILENO, "nope\n", 5) != -1,
              "Erreur d'écriture sur la sortie standard");
    } 

    return EXIT_FAILURE;

error:
    return EXIT_FAILURE;
}

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
        return EXIT_FAILURE;
    return EXIT_SUCCESS;
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
