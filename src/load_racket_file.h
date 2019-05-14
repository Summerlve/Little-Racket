#ifndef LOAD_RACKET_FILE
#define LOAD_RACKET_FILE

#define PATH_MAX 4096
#include <stdio.h>

FILE *load_racket_file(const char *path);
int close_racket_file(FILE *fp);

#endif