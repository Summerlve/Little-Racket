#ifndef LOAD_RACKET_FILE
#define LOAD_RACKET_FILE

#include <stdio.h>
#include <stdbool.h>

#define LINE_MAX 1024 // the most characters of single line in a racket file. 
typedef struct _z_Raw_Code {
    FILE *fp; // store file poniter to racket file stream.
    unsigned char *absolute_path;
    unsigned char **contents;
/*
 *  the architecture of field contents 'unsigned char **'.
 *  every single unsigned char * in unsigned char ** pointed to every single physical line of racket file without newline character.
 *  physcial line is equal to logical line in this project, because of I dont implement which similiar to \ of C in racket.
 *  and every single unsigned char * was created by malloc, and unsigned char ** also be, dont forget free it. 
 */
    size_t line_number; // physical line number.
    size_t allocated_length; // allocated number.
} Raw_Code;
typedef void (*RacketFileMapFunction)(const unsigned char *line, void *aux_data); // racket file lines map function.
Raw_Code *racket_file_load(const unsigned char *path); // load racket file into memory
int racket_file_free(Raw_Code *raw_code);
void racket_file_map(Raw_Code *raw_code, RacketFileMapFunction map, void *aux_data);

#endif