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

    #ifdef DEBUG_MODE
    // show the raw_code in gaint single string.
    printf("Raw code:\n");
    racket_file_map(raw_code, print_raw_code, NULL);
    printf("\n");
    #endif

    // tokenizer 
    Tokens *tokens = tokenizer(raw_code); 

    // show the tokens.
    #ifdef DEBUG_MODE
    printf("Tokens:\n");
    tokens_map(tokens, print_tokens, NULL);
    printf("\n");
    #endif

    // parser
    AST ast = parser(tokens);

    #ifdef DEBUG_MODE
    // show ast by traverser
    Visitor custom_visitor = get_custom_visitor();
    traverser(ast, custom_visitor, NULL);

    // ast copy and show it
    AST ast_copy = ast_node_deep_copy(ast, NULL);
    traverser(ast_copy, custom_visitor, NULL);
    printf("\n");
    #endif

    // calculator
    Result result = calculator(ast, NULL);

    #ifdef DEBUG_MODE
    // show result by traverser
    printf("Result:\n");
    traverser(result, custom_visitor, NULL);
    printf("\n");
    #endif

    // release memory
    racket_file_free(raw_code);
    tokens_free(tokens);
    ast_free(ast);
    result_free(result);
    #ifdef DEBUG_MODE
    visitor_free(custom_visitor);
    #endif 
    
    return 0;
}