#include "../headers/jsh.h"
#include "../headers/debug.h"
#include "../headers/extern_commands.h"
#include "../headers/intern_commands.h"
#include "../headers/string_parser.h"
#include <readline/history.h>
#include <readline/readline.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

char *getPrompt() {
    debug("call to get the prompt");

    char cwd[PATH_MAXSIZE + 1];
    check(getcwd(cwd, PATH_MAXSIZE) != NULL,
          "can't get current working directory!");

    size_t prompt_size = PROMPT_MAXSIZE + strlen(YELLOW) + strlen(DEFAULT) + 1;
    char *prompt = malloc(prompt_size * sizeof(char));
    check_mem(prompt);

    int cwd_len = strlen(cwd);
    strcpy(prompt, YELLOW);
    if (cwd_len < 30) {
        strcat(prompt, cwd);
    } else {
        strcat(prompt, "...");
        strcat(prompt, &(cwd[cwd_len - 27]));
    }
    strcat(prompt, "$ ");
    strcat(prompt, DEFAULT);
    return prompt;
error:
    return NULL;
}

void clearScreen() {
    printf(CLEAR_SCREEN);
}

int run_command(char **input) {
    char *cmd = input[0];
    debug("call to run command '%s'", cmd);
    if (strcmp(cmd, "pwd") == 0) {
        check(write(STDOUT_FILENO, "nope\n", 5) != -1,
              "Erreur d'écriture sur la sortie standard");
    } else if (strcmp(cmd, "cd") == 0) {
        check(write(STDOUT_FILENO, "nope\n", 5) != -1,
              "Erreur d'écriture sur la sortie standard");
    } else if (strcmp(cmd, "?") == 0) {
        char buf[4069];
        int n = snprintf(buf, sizeof(buf) - 2, "%d\n", getReturn());
        check(write(STDOUT_FILENO, buf, n) != -1,
              "Erreur d'écriture sur la sortie standard");
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
    } else {
        return exec(cmd, input);
    }
    return 0;
error:
    return -1;
}

int start() {
    clearScreen();
    debug("call to start the jsh");
    while (1) {
        char *prompt = getPrompt();
        debug("current prompt: %s", prompt);
        char *line = readline(prompt);
        if (line == NULL) {
            jsh_exit_val(getReturn());
        }
        debug("line read: %s", line);
        if (strcmp(line, "") != 0) {
            add_history(line);
            char **input = parse_line(line, ' ');
            free(line);
            int ret = run_command(input);
            setReturn(ret);
            free_parse_table(input);
        }
    }
    return 0;
}
