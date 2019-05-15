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
   
    // load racket file
    FILE *fp = load_racket_file(racket_file_path);

    int c;
    if (fp) {
    while ((c = getc(fp)) != EOF)
        putchar(c);
    } 

    // TO-DO tokenizer 
    // TO-DO parser
    // TO-DO calculator 

    // close file
    close_racket_file(fp);
    free(racket_file_path);

    return 0;
}