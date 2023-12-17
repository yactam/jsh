#include "global_variables.h"
#include <stdio.h>
#include <string.h>

int last_return = 0;

void setReturn(int val) {
    last_return = val;
}

int getReturn() {
    return last_return;
}
