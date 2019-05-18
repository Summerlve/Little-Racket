#ifndef LOAD_RACKET_FILE
#define LOAD_RACKET_FILE

#include <stdio.h>
#include <stdbool.h>

#define LINE_MAX 233 // the most characters of single line in a racket file. 
typedef struct _z_Raw_Code {
    FILE *fp; // store file poniter to racket file stream.
    char **content;
/*
 *  the architecture of field content 'char **'.
 *  every single char * in char **(char *[]) pointed to every single line of racket file without newline character.
 *  and every single char * was created by malloc, and char ** also be, dont forget free it. 
 */
    int line_number; // line number.
    int allocated_length; // allocated number.
} Raw_Code;

Raw_Code *load_racket_file(const char *path); // load racket file into memory
int free_racket_file(Raw_Code *raw_code);

#endif