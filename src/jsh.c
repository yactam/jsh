#include "jsh.h"
#include "debug.h"
#include "extern_commands.h"
#include "intern_commands.h"
#include "redirections.h"
#include "parser.h"
#include "jobs_supervisor.h"
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
    debug("call to run command");
   
    command_type type = get_command_type(input);
    debug("command type is %d", type);
	int ret = 0;
    switch(type) {
        case ERROR:
            dprintf(STDERR_FILENO, "Error in command (NULL)\n");
			ret = EXIT_FAILURE;
            break;
		case INTERN_COMMAND:
            debug("intern command to execute");
			ret = run_intern_command(input);
			break;
		case EXTERN_COMMAND:
            debug("extern command to execute");
			ret = run_extern_command(input);
			break;
		case IO_REDIRECTION:
            debug("redirection command to execute");
			ret = run_rediraction(input);
			break;
		case PIPE:
			ret = 0; // run_pipe(input);
			break;
		case PROCESSUS_SUBSTITUTION:
			ret = 0; // run_process_substitution(input);
			break;
    }

    return ret;
}


int start() {
    debug("call to start the jsh");
    setenv("OLDPWD", "", 1);
    rl_outstream = stderr;
	init_jobs_supervisor();
    while (1) {
        char *prompt = getPrompt();
        debug("current prompt: %s", prompt);
        char *line = readline(prompt);
        free(prompt);
        if (line == NULL) {
            free_jobs_supervisor();
            jsh_exit_val(getReturn());
        }
        debug("line read: %s", line);
        if (strcmp(line, "") != 0) {
            add_history(line);
            char **input = parse_line(line, ' ');
            debug("line parsed");
            int ret = run_command(input);
            debug("the last command returned: %d", ret);
            setReturn(ret);
            free(line);
            free_parse_table(input);
        }
    }
    return 0;
}
