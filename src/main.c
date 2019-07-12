#include "./load_racket_file.h"
#include "./interpreter.h"
#include "./custom_handler.h"
#include <stdio.h>
#include <stdlib.h>

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

    // parser
    AST ast = parser(tokens);

    // traverser
    Visitor visitor = get_custom_visitor();
    traverser(ast, visitor, NULL);

    // ----- working on calculator -----

    // release memory
    racket_file_free(raw_code);
    tokens_free(tokens);
    ast_free(ast);
    visitor_free(visitor);
    
    return 0;
}