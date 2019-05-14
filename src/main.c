#include "./load_racket_file.h"
#include "./interpreter.h"
#include <stdio.h>

int main(int argc, char *argv[])
{
    // get the path from command arg.
    const char *path = argv[1];

    // generate absolute path of a racket file. 
    
    // load racket file
    FILE *fp = load_racket_file(path);

    // TO-DO tokenizer 
    // TO-DO parser
    // TO-DO calculator 
    return 0;
}