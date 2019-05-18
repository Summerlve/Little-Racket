#include "./load_racket_file.h"
#include "./interpreter.h"
#include <stdio.h>
#include <stdlib.h>

void map(const char *line, void *aux_data)
{
    printf("%s", line);
}

int main(int argc, char *argv[])
{
    // get the path from command arg.
    const char *path = argv[1];

    // load racket file content into memory.
    Raw_Code *raw_code = load_racket_file(path);

    // show the raw_code in gaint single string.
    racket_file_map(raw_code, map, NULL);

    // WORKING tokenizer 
    Tokens *tokens = tokenizer(raw_code); 

    // TO-DO parser
    // TO-DO calculator 

    // release memory.
    free_racket_file(raw_code);
    
    return 0;
}