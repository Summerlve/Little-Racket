#include "./load_racket_file.h"
#include <stdio.h>
#include <string.h>

FILE *load_racket_file(const char *path)
{
    if (strstr(path, ".rkt") == NULL)
    {
        // can not find .rkt in path.
        // print error to console and exit program.
    }

    FILE *fp;
    fp = fopen(path, "r");
    return fp;
}

int close_racket_file(FILE *fp)
{
    return fclose(fp);
}