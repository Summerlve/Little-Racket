#include "../include/global.h"
#include "../include/load_racket_file.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>
#include <unistd.h>
#include <errno.h>
#include <stdbool.h>

static bool is_absolute_path(const unsigned char *path);
static unsigned char *generate_racket_file_absolute_path(const unsigned char *path);
static FILE *open_racket_file(const unsigned char *path);
static int close_racket_file(FILE *fp);

Raw_Code *raw_code_new(const unsigned char *path)
{
    Raw_Code *raw_code = (Raw_Code *)malloc(sizeof(Raw_Code));
    // generate absolute path of a racket file 
    raw_code->absolute_path = generate_racket_file_absolute_path(path); 
    // open racket file
    raw_code->fp = open_racket_file(raw_code->absolute_path);
    raw_code->allocated_length = 4; // init 4 lines space to store
    raw_code->line_number = 0;
    raw_code->contents = (unsigned char **)malloc(raw_code->allocated_length * sizeof(unsigned char *));
    if (raw_code->contents == NULL)
    {
        perror("Raw_Code::contents malloc failed");
        exit(EXIT_FAILURE);
    }

    return raw_code;
}

int raw_code_free(Raw_Code *raw_code)
{
    // release FILE *fp
    int result = close_racket_file(raw_code->fp);
    if (result != 0)
    {
        perror("fclose() failed");
        return 1;
    }

    // release const char *absolute_path
    free(raw_code->absolute_path);

    // release char **contents
    for (size_t i = 0; i < raw_code->line_number; i++)
    {
        unsigned char *line = raw_code_contents_nth(raw_code, i);
        free(line);
    }

    free(raw_code->contents);

    //release raw_code itself
    free(raw_code);

    return 0;
}

int add_line(Raw_Code *raw_code, const unsigned char *line)
{
    // expand the Raw_Code::contents
    if (raw_code->line_number == raw_code->allocated_length)
    {
        raw_code->contents = realloc(raw_code->contents, raw_code->allocated_length * 2 * sizeof(unsigned char *));
        if (raw_code->contents == NULL)
        {
            printf("errorno is: %d\n", errno);
            perror("Raw_Code:contents expand failed");
            return 1;
        }
        raw_code->allocated_length *= 2;
    }

    raw_code->contents[raw_code->line_number] = (unsigned char *)malloc(LINE_MAX);
    strcpy(TYPECAST(char *, raw_code->contents[raw_code->line_number]), TYPECAST(const char *, line));
    raw_code->line_number ++;

    return 0;
}

unsigned char *raw_code_contents_nth(Raw_Code *raw_code, size_t index)
{
    return raw_code->contents[index];
}

void raw_code_contents_map(Raw_Code *raw_code, RacketFileMapFunction map, void *aux_data)
{
    size_t length = raw_code->line_number;

    for (size_t i = 0; i < length; i++)
    {
        const unsigned char *line = raw_code_contents_nth(raw_code, i);
        map(line, aux_data);
    }
}

Raw_Code *racket_file_load(const unsigned char *path)
{
    // initialize Raw_Code
    Raw_Code *raw_code = raw_code_new(path);
    
    // copy racket file to Raw_Code::contents
    unsigned char *line = (unsigned char *)malloc(LINE_MAX); // line buffer

    while (fgets(TYPECAST(char *, line), LINE_MAX, raw_code->fp) != NULL)
    {
        add_line(raw_code, line);
        // remove newline character in each line
        size_t index = strcspn(TYPECAST(const char *, raw_code->contents[raw_code->line_number - 1]), "\r\n");
        if (index == 0)
        {
            if (strlen(TYPECAST(const char *, raw_code->contents[raw_code->line_number - 1])) != 0)
            {
                raw_code->contents[raw_code->line_number - 1][index] = '\0'; 
            }
        }
        else
        {
            if (strlen(TYPECAST(const char *, raw_code->contents[raw_code->line_number -1])) != index)
            {
                raw_code->contents[raw_code->line_number - 1][index] = '\0';
            }
        }
    }  

    // free the line buffer
    free(line);

    if (feof(raw_code->fp))
    {
        // load racket file completed
    }

    if (ferror(raw_code->fp))
    {
        perror(TYPECAST(const char *, path));
        exit(EXIT_FAILURE);
    }

    return raw_code;
}

int racket_file_free(Raw_Code *raw_code)
{
    return raw_code_free(raw_code);
}

unsigned char *racket_file_nth(Raw_Code *raw_code, size_t index)
{
    return raw_code_contents_nth(raw_code, index);
}

void racket_file_map(Raw_Code *raw_code, RacketFileMapFunction map, void *aux_data)
{
    raw_code_contents_map(raw_code, map, aux_data);
}

static bool is_absolute_path(const unsigned char *path)
{
    // Absolute paths tend to start with the / character
    if (path[0] == '/')
    {
        return true;
    }
    else
    {
        return false;
    }
}

// remember release the memory
static unsigned char *generate_racket_file_absolute_path(const unsigned char *path)
{
    if (is_absolute_path(path) == true)
    {
        unsigned char *absolute_path = (unsigned char *)malloc(strlen(TYPECAST(const char *, path) + 1));
        strcpy(TYPECAST(char *, absolute_path), TYPECAST(const char *, path));
        
        return absolute_path;
    }

    unsigned char *temp = (unsigned char *)malloc(PATH_MAX);
    unsigned char *absolute_path = (unsigned char *)malloc(PATH_MAX);
    getcwd(TYPECAST(char *, temp), PATH_MAX);
    // append path_from_input to the tail of temp
    strcat(TYPECAST(char *, temp), "/");
    strcat(TYPECAST(char *, temp), TYPECAST(const char *, path));
    // get absolute_path 
    realpath(TYPECAST(const char *, temp), TYPECAST(char *, absolute_path));
    free(temp);

    return absolute_path;
}

static FILE *open_racket_file(const unsigned char *path)
{
    if (strstr(TYPECAST(const char *, path), ".rkt") == NULL)
    {
        // if the path dont includes '.rkt'
        // print error to console and exit program with failure
        perror("load .rkt file please");
        exit(EXIT_FAILURE);
    }

    FILE *fp = fopen(TYPECAST(const char *, path), "r");
    if (fp == NULL)
    {  
        // load .rkt file failed, exit program with failure
        perror(TYPECAST(const char *, path));
        exit(EXIT_FAILURE);
    }

    return fp;
}

static int close_racket_file(FILE *fp)
{
    return fclose(fp);
}