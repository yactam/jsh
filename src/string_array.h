#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct {
    char **data;
    int length;
} string_array;

void initialize_string_array(string_array *arr);
void add_string(string_array *arr, const char *str);
const char *get_string(const string_array *arr, int index);
void remove_string(string_array *arr, int index);
void free_string_array(string_array *arr);