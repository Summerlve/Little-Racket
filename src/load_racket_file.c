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

// remember release the memory.
static char *generate_racket_file_absolute_path(const char *path_from_input)
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

static int raw_code_new(Raw_Code *raw_code)
{
    
    // generate absolute path of a racket file. 
    char *racket_file_path = generate_racket_file_absolute_path(path_from_input); 
   
    // load file stream to racket file. 
    FILE *fp = load_racket_file(racket_file_path);
}

static int expand(Raw_Code *raw_code)
{

}

static int add_line(Raw_Code *raw_code)
{

}

static int raw_code_free(Raw_Code *raw_code)
{

}

static FILE *open_racket_file(const char *path)
{
    if (strstr(path, ".rkt") == NULL)
    {
        // if the path dont includes '.rkt'.
        // print error to console and exit program with failure.
        perror("load .rkt file please");
        exit(EXIT_FAILURE);
    }

    FILE *fp;
    fp = fopen(path, "r");
    if (fp == NULL)
    {  
        // load .rkt file failed, exit program with failure.
        perror(path);
        exit(EXIT_FAILURE);
    }

    return fp;
}

static int close_racket_file(FILE *fp)
{
    return fclose(fp);
}



Raw_Code *load_racket_file(const char *path)
{
    // TODO open racket file
    // TODO initialize Raw_Code
    // TODO copy racket file to Raw_Code::content
    Raw_Code *raw_code = malloc(siezof(Raw_Code));

    char *line = (char *)malloc(LINE_MAX);

    while (fgets(line, LINE_MAX, fp) != NULL)
    {
        line = (char *)malloc(LINE_MAX);
    }  

    if (feof(fp))
    {

    }

    if (ferror(fp))
    {

    }

    return raw_code;
}

int free_racket_file(Raw_Code *raw_code)
{

}


