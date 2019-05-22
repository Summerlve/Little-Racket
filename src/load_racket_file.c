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

static Raw_Code *raw_code_new(const char *path)
{
    Raw_Code *raw_code = (Raw_Code *)malloc(sizeof(Raw_Code));
    // generate absolute path of a racket file. 
    raw_code->absolute_path = generate_racket_file_absolute_path(path); 
    // open racket file
    raw_code->fp = open_racket_file(raw_code->absolute_path);
    raw_code->allocated_length = 4; // init 4 lines space to store.
    raw_code->contents = (char **)malloc(raw_code->allocated_length * sizeof(char *));
    if (raw_code->contents == NULL)
    {
        perror("Raw_Code::contents malloc failed");
        exit(EXIT_FAILURE);
    }
    raw_code->line_number = 0;

    return raw_code;
}

static char *raw_code_contents_nth(Raw_Code *raw_code, int index)
{
    return raw_code->contents[index];
}

static int raw_code_free(Raw_Code * raw_code)
{
    // release FILE *fp
    int result = fclose(raw_code->fp);
    if (result != 0)
    {
        perror("fclose() failed");
        return 1;
    }

    // release const char *absolute_path
    free(raw_code->absolute_path);

    // release char **contents
    for (int i = 0; i < raw_code->line_number; i++)
    {
        char *line = raw_code_contents_nth(raw_code, i);
        free(line);
    }

    free(raw_code->contents);

    //release raw_code itself
    free(raw_code);

    return 0;
}

static void raw_code_contents_map(Raw_Code *raw_code, RacketFileMapFunction map, void *aux_data)
{
    int length = raw_code->line_number;

    for (int i = 0; i < length; i++)
    {
        const char *line = raw_code_contents_nth(raw_code, i);
        map(line, aux_data);
    }
}

static int add_line(Raw_Code *raw_code, const char *line)
{
    // expand the Raw_Code::contents
    if (raw_code->line_number == raw_code->allocated_length)
    {
        raw_code->contents = realloc(raw_code->contents, raw_code->allocated_length * 2 * sizeof(char *));
        if (raw_code->contents == NULL)
        {
            printf("errorno is: %d\n", errno);
            perror("Raw_Code:contents expand failed");
            return 1;
        }
        raw_code->allocated_length *= 2;
    }

    raw_code->contents[raw_code->line_number] = (char *)malloc(LINE_MAX);
    strcpy(raw_code->contents[raw_code->line_number], line);
    raw_code->line_number ++;

    return 0;
}

Raw_Code *load_racket_file(const char *path)
{
    // initialize Raw_Code
    Raw_Code *raw_code = raw_code_new(path);
    
    // copy racket file to Raw_Code::contents
    char *line = (char *)malloc(LINE_MAX); // line buffer.

    while (fgets(line, LINE_MAX, raw_code->fp) != NULL)
    {
        add_line(raw_code, line);
        // remove newline character in each line.
        int index = strcspn(raw_code->contents[raw_code->line_number - 1], "\r\n");
        if (index == 0)
        {
            if (strlen(raw_code->contents[raw_code->line_number - 1]) != 0)
            {
                raw_code->contents[raw_code->line_number - 1][index] = '\0'; 
            }
        }
        else
        {
            if (strlen(raw_code->contents[raw_code->line_number -1]) != index)
            {
                raw_code->contents[raw_code->line_number - 1][index] = '\0';
            }
        }
    }  

    // free the line buffer.
    free(line);

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
    return raw_code_free(raw_code);
}

static char *racket_file_nth(Raw_Code *raw_code, int index)
{
    return raw_code_contents_nth(raw_code, index);
}

void racket_file_map(Raw_Code *raw_code, RacketFileMapFunction map, void *aux_data)
{
    raw_code_contents_map(raw_code, map, aux_data);
}