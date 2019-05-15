#include "./load_racket_file.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>
#include <unistd.h>

static bool is_absolute_path(const char *path)
{
    // Absolute paths tend to start with the / character.
    if (path[0] == '/')
    {
        return true;
    }
    else
    {
        return false;
    }
}

char *generate_racket_file_absolute_path(const char *path_from_input)
{
    if (is_absolute_path(path_from_input) == true)
    {
        char *absolute_path = (char *)malloc(strlen(path_from_input) + 1);
        strcpy(absolute_path, path_from_input);
        
        return absolute_path;
    }

    char *temp = (char *)malloc(PATH_MAX);
    char *absolute_path = (char *)malloc(PATH_MAX);
    getcwd(temp, PATH_MAX);
    // append path_from_input to the tail of temp.
    strcat(temp, "/");
    strcat(temp, path_from_input);
    // get absolute_path 
    realpath(temp, absolute_path);
    free(temp);

    return absolute_path;
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