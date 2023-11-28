#include "../headers/intern_commands.h"
#include "../headers/debug.h"
#include "../headers/global_variables.h"
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