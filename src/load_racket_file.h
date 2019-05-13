#ifndef LOAD_RACKET_FILE
#define LOAD_RACKET_FILE

#include <stdio.h>

FILE *load_racket_file(const char *path);
int close_racket_file(FILE *fp);

#endif