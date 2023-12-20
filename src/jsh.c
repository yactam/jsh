#include "jsh.h"
#include "debug.h"
#include "extern_commands.h"
#include "intern_commands.h"
#include "string_array.h"
#include "string_parser.h"
#include <fcntl.h>
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

    size_t prompt_size =
        PROMPT_MAXSIZE + strlen(BLUE) + strlen(YELLOW) + strlen(DEFAULT) + 1;
    char *prompt = malloc(prompt_size * sizeof(char));
    check_mem(prompt);

    int cwd_len = strlen(cwd);
    debug("cwd_len = %d", cwd_len);
    int nb_jobs = 0;
    char jobs[JOBS_MAXSIZE + 3];
    snprintf(jobs, JOBS_MAXSIZE + 2, "[%d]", nb_jobs);
    strcpy(prompt, BLUE);
    strcat(prompt, jobs);
    strcat(prompt, YELLOW);
    size_t size_left_to_ref = PROMPT_MAXSIZE - strlen(jobs) - 2;
    debug("size left to prompt = %ld", size_left_to_ref);
    if (cwd_len <= size_left_to_ref) {
        strcat(prompt, cwd);
    } else {
        strcat(prompt, "...");
        strcat(prompt, &(cwd[cwd_len - size_left_to_ref + 3]));
    }
    strcat(prompt, DEFAULT);
    strcat(prompt, "$ ");
    return prompt;
error:
    return NULL;
}

int run_command(char **input) {
    char *cmd = input[0];
    debug("call to run command '%s'", cmd);
    if (strcmp(cmd, "pwd") == 0) {
        check(pwd() != -1, "Erreur d'écriture sur la sortie standard");
    } else if (strcmp(cmd, "cd") == 0) {
        return cd(input);
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

int run_red(string_array sa) {
    int sin = dup(STDIN_FILENO);
    int sout = dup(STDOUT_FILENO);
    int serr = dup(STDERR_FILENO);
    if (strcmp(sa.data[sa.length - 2], ">") == 0) {
        int write_fd;
        write_fd =
            open(sa.data[sa.length - 1], O_CREAT | O_WRONLY | O_TRUNC, 0770);
        dup2(write_fd, STDOUT_FILENO);
        close(write_fd);
    } else if (strcmp(sa.data[sa.length - 2], ">>") == 0) {
        int write_fd;
        write_fd =
            open(sa.data[sa.length - 1], O_CREAT | O_WRONLY | O_APPEND, 0770);
        dup2(write_fd, STDOUT_FILENO);
        close(write_fd);
    } else if (strcmp(sa.data[sa.length - 2], ">|") == 0) {
        int write_fd;
        write_fd = open(sa.data[sa.length - 1], O_CREAT | O_WRONLY, 0770);
        dup2(write_fd, STDOUT_FILENO);
        close(write_fd);
    } else if (strcmp(sa.data[sa.length - 2], "2>") == 0) {
        int write_fd;
        write_fd =
            open(sa.data[sa.length - 1], O_CREAT | O_WRONLY | O_TRUNC, 0770);
        dup2(write_fd, STDERR_FILENO);
        close(write_fd);
    } else if (strcmp(sa.data[sa.length - 2], "2>>") == 0) {
        int write_fd;
        write_fd =
            open(sa.data[sa.length - 1], O_CREAT | O_WRONLY | O_APPEND, 0770);
        dup2(write_fd, STDERR_FILENO);
        close(write_fd);
    } else if (strcmp(sa.data[sa.length - 2], "2>|") == 0) {
        int write_fd;
        write_fd = open(sa.data[sa.length - 1], O_CREAT | O_WRONLY, 0770);
        dup2(write_fd, STDERR_FILENO);
        close(write_fd);
    } else if (strcmp(sa.data[sa.length - 2], "<") == 0) {
        int input_fd;
        input_fd = open(sa.data[sa.length - 1], O_RDONLY);
        dup2(input_fd, STDIN_FILENO);
        close(input_fd);
    }
    remove_string(&sa, sa.length - 1);
    remove_string(&sa, sa.length - 1);
    int res = run_command(sa.data);
    dup2(sin, STDIN_FILENO);
    dup2(sout, STDOUT_FILENO);
    dup2(serr, STDERR_FILENO);
    return res;
}

int start() {
    debug("call to start the jsh");
    setenv("OLDPWD", "", 1);
    rl_outstream = stderr;
    string_array sa;
    while (1) {
        char *prompt = getPrompt();
        debug("current prompt: %s", prompt);
        char *line = readline(prompt);
        if (line == NULL) {
            jsh_exit_val(getReturn());
        }
        debug("line read: %s", line);
        int ret;
        if (strcmp(line, "") != 0) {
            add_history(line);
            char **input = parse_line(line, ' ');
            int len_input = nombreElements(input);
            initialize_string_array(&sa);
            for (int i = 0; i < len_input; i++) {
                add_string(&sa, input[i]);
            }
            const char *maybe_red = get_string(&sa, sa.length - 2);
            printf("%i\n", strcmp(maybe_red, ">"));
            printf("%s\n", maybe_red);
            if (is_a_redirection(maybe_red)) {
                printf("ici ?\n");
                ret = run_red(sa);
            } else {
                ret = run_command(input);
            }
            free(line);
            debug("the last command returned: %d", ret);
            setReturn(ret);
            free_parse_table(input);
            free_string_array(&sa);
        }
        free(prompt);
    }
    return 0;
}
