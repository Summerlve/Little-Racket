#include "./load_racket_file.h"
#include "./interpreter.h"
#include <stdio.h>
#include <stdlib.h>

int main(int argc, char *argv[])
{
    // get the path from command arg.
    const char *path_from_input = argv[1];


    // load racket file content into memory.
    char **raw_code = load_racket_file_into_mem(fp);

    // TO-DO tokenizer 
    // TO-DO parser
    // TO-DO calculator 

    // close file
    close_racket_file(fp);

    // release memory.
    free(racket_file_path);
    free_racket_file(raw_code);
    
    return 0;
}