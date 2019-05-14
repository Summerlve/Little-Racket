#include "./load_racket_file.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>
#include <unistd.h>

char *generate_racket_file_absolute_path(const char *path_from_input)
{
    char *absolute_path = (char *)malloc(PATH_MAX);
    

}

FILE *load_racket_file(const char *path)
{
    if (strstr(path, ".rkt") == NULL)
    {
        // if can not find .rkt in path.
        // print error to console and exit program with failure.
        perror("load .rkt file plz.");
        exit(EXIT_FAILURE);
    }

    FILE *fp;
    fp = fopen(path, "r");
    if (fp == NULL)
    {  
        perror(path);
    }

    return fp;
}

int close_racket_file(FILE *fp)
{
    return fclose(fp);
}