#ifndef __STRING_PARSER_H__
#define __STRING_PARSER_H__
#include <stddef.h>

typedef enum {
    INTERN_COMMAND,
    EXTERN_COMMAND,
    IO_REDIRECTION,
    PIPE,
    PROCESSUS_SUBSTITUTION,
    ERROR
} command_type;

char **parse_line(char *, char);
size_t size_parse_table(char **);
void free_parse_table(char **);
command_type get_command_type(char **);

#endif
