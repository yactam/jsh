#include "global_variables.h"
#include <stdio.h>
#include <string.h>

int last_return = 0;

void set_return(int val) {
    last_return = val;
}

int get_return() {
    return last_return;
}
