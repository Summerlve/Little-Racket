#ifndef LOAD_RACKET_FILE
#define LOAD_RACKET_FILE

#include <stdio.h>
#include <stdbool.h>

FILE *load_racket_file(const char *path);
int close_racket_file(FILE *fp);
char *generate_racket_file_absolute_path(const char *path_from_input); // remember release the memory.

#endif