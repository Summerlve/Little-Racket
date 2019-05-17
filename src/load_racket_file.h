#ifndef LOAD_RACKET_FILE
#define LOAD_RACKET_FILE

#include <stdio.h>
#include <stdbool.h>

#define LINE_MAX 233 // the most characters of single line in a racket file. 
typedef struct _z_Raw_Code {
    char **content;
    long line_number;
} Raw_Code;
int raw_code_new(Raw_Code *raw_code);
int expand(Raw_Code *raw_code);
int add_line(Raw_Code *raw_code);
int raw_code_free(Raw_Code *raw_code);
FILE *load_racket_file(const char *path);
int close_racket_file(FILE *fp);
char *generate_racket_file_absolute_path(const char *path_from_input); // remember release the memory.
Raw_Code *load_racket_file_into_mem(FILE *fp); // load racket file into memory
/*
 *  the architecture of return value 'char **'.
 *  every single char * in char **(char *[]) pointed to every single line of racket file without newline character.
 *  and every single char * was created by malloc, and char ** also be, dont forget free it. 
 */
void free_racket_file(char **raw_code);

#endif