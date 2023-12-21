#include "debug.h"
#include "jsh.h"
#include "parser.h"
#include <fcntl.h>
#include <string.h>
#include <unistd.h>

int handel_redirection(char *redirection, char *fic) {
    if (strcmp(redirection, ">") == 0) {
        int write_fd;
        write_fd = open(fic, O_CREAT | O_WRONLY | O_EXCL, 0666);
        if (write_fd < 0) {
            dprintf(STDERR_FILENO, "bash : %s : %s\n", fic, strerror(errno));
            return EXIT_FAILURE;
        }
        dup2(write_fd, STDOUT_FILENO);
        close(write_fd);
    } else if (strcmp(redirection, ">>") == 0) {
        int write_fd;
        write_fd = open(fic, O_CREAT | O_WRONLY | O_APPEND, 0666);
        if (write_fd < 0) {
            dprintf(STDERR_FILENO, "bash : %s : %s\n", fic, strerror(errno));
            return EXIT_FAILURE;
        }
        dup2(write_fd, STDOUT_FILENO);
        close(write_fd);
    } else if (strcmp(redirection, ">|") == 0) {
        int write_fd;
        write_fd = open(fic, O_CREAT | O_WRONLY | O_TRUNC, 0666);
        if (write_fd < 0) {
            dprintf(STDERR_FILENO, "bash : %s : %s\n", fic, strerror(errno));
            return EXIT_FAILURE;
        }
        dup2(write_fd, STDOUT_FILENO);
        close(write_fd);
    } else if (strcmp(redirection, "2>") == 0) {
        int write_fd;
        write_fd = open(fic, O_CREAT | O_WRONLY | O_EXCL, 0666);
        if (write_fd < 0) {
            dprintf(STDERR_FILENO, "bash : %s : %s\n", fic, strerror(errno));
            return EXIT_FAILURE;
        }
        dup2(write_fd, STDERR_FILENO);
        close(write_fd);
    } else if (strcmp(redirection, "2>>") == 0) {
        int write_fd;
        write_fd = open(fic, O_CREAT | O_WRONLY | O_APPEND, 0666);
        if (write_fd < 0) {
            dprintf(STDERR_FILENO, "bash : %s : %s\n", fic, strerror(errno));
            return EXIT_FAILURE;
        }
        dup2(write_fd, STDERR_FILENO);
        close(write_fd);
    } else if (strcmp(redirection, "2>|") == 0) {
        int write_fd;
        write_fd = open(fic, O_CREAT | O_WRONLY | O_TRUNC, 0666);
        if (write_fd < 0) {
            dprintf(STDERR_FILENO, "bash : %s : %s\n", fic, strerror(errno));
            return EXIT_FAILURE;
        }
        dup2(write_fd, STDERR_FILENO);
        close(write_fd);
    } else if (strcmp(redirection, "<") == 0) {
        int input_fd;
        input_fd = open(fic, O_RDONLY);
        if (input_fd < 0) {
            dprintf(STDERR_FILENO, "bash : %s : %s\n", fic, strerror(errno));
            return EXIT_FAILURE;
        }
        dup2(input_fd, STDIN_FILENO);
        close(input_fd);
    }

    return EXIT_SUCCESS;
}

int run_redirection(char **tokens) {

    size_t i = 0;

    while (tokens[i] != NULL) {
        if (strcmp(tokens[i], "<") == 0 || strcmp(tokens[i], ">") == 0 ||
            strcmp(tokens[i], ">|") == 0 || strcmp(tokens[i], ">>") == 0 ||
            strcmp(tokens[i], "2>") == 0 || strcmp(tokens[i], "2>|") == 0 ||
            strcmp(tokens[i], "2>>") == 0)
            break;
        i++;
    }

    char **args = (char **)malloc((i + 1) * sizeof(char *));
    if (args == NULL) {
        log_error("Memory allocation error");
        return EXIT_FAILURE;
    }

    for (size_t j = 0; j < i; j++) {
        args[j] = strdup(tokens[j]);
        if (args[j] == NULL) {
            log_error("Memory allocation error");
            for (size_t k = 0; k < j; k++) {
                free(args[k]);
            }
            free(args);
            return EXIT_FAILURE;
        }
    }

    args[i] = NULL;

    int sin = dup(STDIN_FILENO);
    int sout = dup(STDOUT_FILENO);
    int serr = dup(STDERR_FILENO);

    for (i = 0; tokens[i] != NULL; i++) {
        if (strcmp(tokens[i], "<") == 0 || strcmp(tokens[i], ">") == 0 ||
            strcmp(tokens[i], ">|") == 0 || strcmp(tokens[i], ">>") == 0 ||
            strcmp(tokens[i], "2>") == 0 || strcmp(tokens[i], "2>|") == 0 ||
            strcmp(tokens[i], "2>>") == 0) {

            char *redirection = tokens[i];
            char *file = tokens[i + 1];

            if (handel_redirection(redirection, file) == EXIT_FAILURE) {
                dup2(sin, STDIN_FILENO);
                dup2(sout, STDOUT_FILENO);
                dup2(serr, STDERR_FILENO);
                return EXIT_FAILURE;
            }

            i++;
        }
    }

    int res = run_command(args);
    free_parse_table(args);
    dup2(sin, STDIN_FILENO);
    dup2(sout, STDOUT_FILENO);
    dup2(serr, STDERR_FILENO);

    return res;
}