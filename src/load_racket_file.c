#include "./load_racket_file.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>
#include <unistd.h>
#include <errno.h>

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
static char *generate_racket_file_absolute_path(const char *path)
{
    if (is_absolute_path(path) == true)
    {
        char *absolute_path = (char *)malloc(strlen(path) + 1);
        strcpy(absolute_path, path);
        
        return absolute_path;
    }

    char *temp = (char *)malloc(PATH_MAX);
    char *absolute_path = (char *)malloc(PATH_MAX);
    getcwd(temp, PATH_MAX);
    // append path_from_input to the tail of temp.
    strcat(temp, "/");
    strcat(temp, path);
    // get absolute_path 
    realpath(temp, absolute_path);
    free(temp);

    return absolute_path;
}

static int raw_code_new(Raw_Code *raw_code, const char *path)
{
    // generate absolute path of a racket file. 
    char *absolute_path = generate_racket_file_absolute_path(path); 
    raw_code->absolute_path = absolute_path;
    raw_code->fp = NULL;
    raw_code->allocated_length = 4; // init 4 lines space to store.
    raw_code->content = (char **)malloc(raw_code->allocated_length * sizeof(char *));
    if (raw_code->content == NULL)
    {
        perror("Raw_Code::content malloc failure");
        return 1;
    }
    raw_code->line_number = 0;

    return 0;
}

static int raw_code_free(Raw_Code *raw_code)
{
     return 0;
}

static int add_line(Raw_Code *raw_code, const char *line)
{
    // expand the Raw_Code::content
    if (raw_code->line_number == raw_code->allocated_length)
    {
        raw_code->content = realloc(raw_code->content, raw_code->allocated_length * 2 * sizeof(char *));
        if (raw_code->content == NULL)
        {
            printf("errorno is: %d\n", errno);
            perror("Raw_Code:content expand failure");
            return 1;
        }
        raw_code->allocated_length *= 2;
    }

    raw_code->content[raw_code->line_number] = (char *)malloc(LINE_MAX);
    strcpy(raw_code->content[raw_code->line_number], line);
    raw_code->line_number ++;

    return 0;
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
    // initialize Raw_Code
    Raw_Code *raw_code = malloc(sizeof(Raw_Code));
    raw_code_new(raw_code, path);

    // open racket file
    raw_code->fp = open_racket_file(raw_code->absolute_path);
    
    // copy racket file to Raw_Code::content
    char *line = (char *)malloc(LINE_MAX);

    while (fgets(line, LINE_MAX, raw_code->fp) != NULL)
    {
        add_line(raw_code, line);
        // remove newline character in each line.
        int index = strcspn(raw_code->content[raw_code->line_number - 1], "\r\n");
        if (index == 0)
        {
            if (strlen(raw_code->content[raw_code->line_number - 1]) != 0)
            {
                raw_code->content[raw_code->line_number - 1][index] = '\0'; 
            }
        }
        else
        {
            if (strlen(raw_code->content[raw_code->line_number -1]) != index)
            {
                raw_code->content[raw_code->line_number - 1][index] = '\0';
            }
        }

        line = (char *)malloc(LINE_MAX);
    }  

    if (feof(raw_code->fp))
    {
        // load racket file completed.
    }

    if (ferror(raw_code->fp))
    {
        perror(path);
        exit(EXIT_FAILURE);
    }

    return raw_code;
}

int free_racket_file(Raw_Code *raw_code)
{
    return 0;
}