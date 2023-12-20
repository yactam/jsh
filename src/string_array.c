#include "string_array.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

void initialize_string_array(string_array *arr) {
    arr->data = NULL;
    arr->length = 0;
}

void add_string(string_array *arr, const char *str) {
    arr->data = realloc(arr->data, (arr->length + 1) * sizeof(char *));
    arr->data[arr->length] = strdup(str);
    arr->length++;
}

const char *get_string(const string_array *arr, int index) {
    if (index >= 0 && index < arr->length) {
        return arr->data[index];
    } else {
        return NULL;
    }
}

void remove_string(string_array *arr, int index) {
    if (index >= 0 && index < arr->length) {
        arr->data[index] = NULL;
        free(arr->data[index]);

        for (int i = index; i < arr->length - 1; ++i) {
            arr->data[i] = arr->data[i + 1];
        }

        arr->data = realloc(arr->data, (arr->length - 1) * sizeof(char *));
        arr->length--;
    }
}

void free_string_array(string_array *arr) {
    for (int i = 0; i < arr->length; ++i) {
        free(arr->data[i]);
    }
    free(arr->data);
    arr->length = 0;
}