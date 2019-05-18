#include "./load_racket_file.h"
#include "./interpreter.h"
#include <stdio.h>
#include <stdlib.h>

int main(int argc, char *argv[])
{
    // get the path from command arg.
    const char *path = argv[1];

    // load racket file content into memory.
    Raw_Code *raw_code = load_racket_file(path);

    // show the raw_code
    for (int i = 0; i < raw_code->line_number; i++)
    {
        printf("%s", raw_code->content[i]);
    }

    // TO-DO tokenizer 
    // TO-DO parser
    // TO-DO calculator 

    // release memory.
    free_racket_file(raw_code);
    
    return 0;
}