#include "string_parser.h"
#include "debug.h"
#include <stddef.h>
#include <stdlib.h>
#include <string.h>

size_t nbWords(char *line, char sep) {
    debug("call to get the number of words");
    size_t res = 0;
    char *cursor = line;
    while (*cursor == sep) {
        cursor++;
    }
    while (*cursor != '\0') {
        char *space = strchr(cursor, sep);
        if (space == NULL) {
            return ++res;
        }
        debug("first occurrence of separator = '%s'\n", space);
        cursor = space;
        while (*cursor == sep) {
            cursor++;
            if (*cursor == '\0')
                return res;
        }
        res++;
    }
    return res;
}

char **parse_line(char *line, char sep) {
    debug("start parse line '%s' using separator '%c'", line, sep);
    size_t nb_words = nbWords(line, sep);
    debug("the number of words in the line is %ld", nb_words);

    char **res = malloc((nb_words + 1) * sizeof(char *));
    size_t index = 0;
    char *word = NULL;
    check_mem(res);
    size_t cursor = 0;
    size_t line_len = strlen(line);

    debug("the length of the line is %ld", line_len);

    while (line[cursor] == sep) {
        cursor++;
    }

    while (cursor < line_len) {
        char *space = strchr(line + cursor, sep);
        debug("first occurence of separator = '%s'", space);
        if (space == NULL) {
            debug("no separators left");
            size_t len = strlen(line + cursor);
            word = malloc((len + 1) * sizeof(char));
            check_mem(word);
            check_mem(memmove(word, line + cursor, len));
            word[len] = '\0';
            debug("copy '%s' to res[%ld]", word, index);
            res[index] = malloc((len + 1) * sizeof(char));
            check_mem(res[index]);
            strcpy(res[index++], word);
            cursor += len;
            debug("cursor = %ld", cursor);
            free(word);
        } else {
            size_t word_len = space - (line + cursor);
            char *word = malloc((word_len + 1) * sizeof(char));
            check_mem(word);
            check_mem(memmove(word, line + cursor, word_len));
            word[word_len] = '\0';
            debug("copy '%s' to res[%ld]", word, index);
            res[index] = malloc((word_len + 1) * sizeof(char));
            check_mem(res[index]);
            strcpy(res[index++], word);
            free(word);
            debug("the length of the word whithout '%c' is %ld", sep, word_len);
            cursor = space - line;

            while (line[cursor] != '\0' && line[cursor] == sep) {
                cursor++;
                debug("cursor = %ld", cursor);
            }

            debug("cursor = %ld", cursor);
        }
    }

    res[nb_words] = NULL;

    return res;
error:
    if (word)
        free(word);
    for (size_t i = 0; i < index; ++i)
        free(res[i]);
    if (res)
        free(res);
    return NULL;
}

void free_parse_table(char **words) {
    for (size_t i = 0; words[i] != NULL; ++i) {
        free(words[i]);
    }
    free(words);
}
