#include "./load_racket_file.h"
#include "./interpreter.h"
#include <stdio.h>
#include <stdlib.h>

void print_raw_code(const char *line, void *aux_data)
{
    printf("%s", line);
}

void print_tokens(const Token *token, void *aux_data)
{
    printf("type: %d, value: %s\n", token->type, token->value);
}

int main(int argc, char *argv[])
{
    // get the path from command arg.
    const char *path = argv[1];

    // load racket file content into memory.
    Raw_Code *raw_code = load_racket_file(path);

    // show the raw_code in gaint single string.
    racket_file_map(raw_code, print_raw_code, NULL);
    printf("\n");

    // tokenizer 
    Tokens *tokens = tokenizer(raw_code); 

    // show the tokens.
    tokens_map(tokens, print_tokens, NULL);

    // --- Working on --- parser
    AST ast = parser(tokens);
    
    // TO-DO calculator 

    // release memory
    free_racket_file(raw_code);
    free_tokens(tokens);
    
    return 0;
}