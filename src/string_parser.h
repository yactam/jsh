#ifndef __STRING_PARSER_H__
#define __STRING_PARSER_H__
#include <stddef.h>

char **parse_line(char *, char);
size_t size_parse_table(char **);
void free_parse_table(char **);
int nombreElements(char **);
int is_a_redirection(const char *);

#endif
