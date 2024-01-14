#include "signal.h"
#include "jobs_supervisor.h"
#include <fcntl.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

void sigchld_handler(int sig) {
    if (sig == SIGCHLD) {
        int wstatus;
        pid_t pid = waitpid(-1, &wstatus, WNOHANG | WUNTRACED);
        job_node *job = get_job_of_process(pid);
        if (job != NULL) {
            update_job(job, pid, wstatus, STDERR_FILENO);
        }
    }
}

void ignore_signals() {
    struct sigaction sa;
    memset(&sa, 0, sizeof(struct sigaction));
    sa.sa_handler = SIG_IGN;

    // Ignore SIGINT
    if (sigaction(SIGINT, &sa, NULL) == -1) {
        perror("sigaction for SIGINT");
        exit(EXIT_FAILURE);
    }

    // Ignore SIGTERM
    if (sigaction(SIGTERM, &sa, NULL) == -1) {
        perror("sigaction for SIGTERM");
        exit(EXIT_FAILURE);
    }

    // Ignore SIGTTIN
    if (sigaction(SIGTTIN, &sa, NULL) == -1) {
        perror("sigaction for SIGTTIN");
        exit(EXIT_FAILURE);
    }

    // Ignore SIGQUIT
    if (sigaction(SIGQUIT, &sa, NULL) == -1) {
        perror("sigaction for SIGQUIT");
        exit(EXIT_FAILURE);
    }

    // Ignore SIGTTOU
    if (sigaction(SIGTTOU, &sa, NULL) == -1) {
        perror("sigaction for SIGTTOU");
        exit(EXIT_FAILURE);
    }

    // Ignore SIGTSTP
    if (sigaction(SIGTSTP, &sa, NULL) == -1) {
        perror("sigaction for SIGTSTP");
        exit(EXIT_FAILURE);
    }

    // Set SIGCHLD signal
    /*sa.sa_handler = sigchld_handler;
    if (sigaction(SIGCHLD, &sa, NULL) == -1) {
        perror("sigaction for SIGCHLD");
        exit(EXIT_FAILURE);
    }*/
}

void reset_signals() {
    struct sigaction sa;
    memset(&sa, 0, sizeof(struct sigaction));
    sa.sa_handler = SIG_DFL;

    // Reset SIGINT
    if (sigaction(SIGINT, &sa, NULL) == -1) {
        perror("sigaction for SIGINT");
        exit(EXIT_FAILURE);
    }

    // Reset SIGTERM
    if (sigaction(SIGTERM, &sa, NULL) == -1) {
        perror("sigaction for SIGTERM");
        exit(EXIT_FAILURE);
    }

    // Reset SIGTTIN
    if (sigaction(SIGTTIN, &sa, NULL) == -1) {
        perror("sigaction for SIGTTIN");
        exit(EXIT_FAILURE);
    }

    // Reset SIGQUIT
    if (sigaction(SIGQUIT, &sa, NULL) == -1) {
        perror("sigaction for SIGQUIT");
        exit(EXIT_FAILURE);
    }

    // Reset SIGTTOU
    if (sigaction(SIGTTOU, &sa, NULL) == -1) {
        perror("sigaction for SIGTTOU");
        exit(EXIT_FAILURE);
    }

    // Reset SIGTSTP
    if (sigaction(SIGTSTP, &sa, NULL) == -1) {
        perror("sigaction for SIGTSTP");
        exit(EXIT_FAILURE);
    }

    // Reset SIGCHLD
    /*if (sigaction(SIGCHLD, &sa, NULL) == -1) {
        perror("sigaction for SIGCHLD");
        exit(EXIT_FAILURE);
    }*/
}