#ifndef LOAD_RACKET_FILE
#define LOAD_RACKET_FILE

#include <stdio.h>
#include <stdbool.h>

FILE *load_racket_file(const char *path);
int close_racket_file(FILE *fp);
char *generate_racket_file_absolute_path(const char *path_from_input); // remember release the memory.
char **load_racket_file_into_mem(FILE *fp); // load racket file into memory
/*
 *  the architecture of return value 'char **'.
 *  every single char * in char **(char *[]) pointed to every single line of racket file without newline character.
 *  and every single char * was created by malloc, and char ** also be, dont forget free it. 
 */
void free_racket_file(char **raw_code);

#endif