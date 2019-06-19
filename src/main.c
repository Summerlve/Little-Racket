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
    Raw_Code *raw_code = racket_file_load(path);

    // show the raw_code in gaint single string.
    racket_file_map(raw_code, print_raw_code, NULL);
    printf("\n");

    // tokenizer 
    Tokens *tokens = tokenizer(raw_code); 

    // show the tokens.
    tokens_map(tokens, print_tokens, NULL);

    // --- Working on --- parser
    AST ast = parser(tokens);
    printf("Program's body's size: %d\n", VectorLength(ast->contents.body));

    // TO-DO calculator 

    // release memory
    racket_file_free(raw_code);
    tokens_free(tokens);
    ast_free(ast);
    
    return 0;
}