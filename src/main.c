#include "./load_racket_file.h"
#include "./interpreter.h"
#include <stdio.h>
#include <stdlib.h>

int main(int argc, char *argv[])
{
    // get the path from command arg.
    const char *path_from_input = argv[1];

    // generate absolute path of a racket file. 
    char *racket_file_path = generate_racket_file_absolute_path(path_from_input); 
   
    // load file stream to racket file. 
    FILE *fp = load_racket_file(racket_file_path);

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