#include "./load_racket_file.h"
#include "./interpreter.h"
#include "./custom_handler.h"
#include <stdio.h>
#include <stdlib.h>

int main(int argc, char *argv[])
{
    #ifdef RELEASE_MODE
    // get the path from command arg.
    const char *path = argv[1];
    // load racket file content into memory.
    Raw_Code *raw_code = racket_file_load(path);
    // tokenizer 
    Tokens *tokens = tokenizer(raw_code);
    // parser
    AST ast = parser(tokens);
    // calculator
    Result result = calculator(ast, NULL);
    // release memory
    racket_file_free(raw_code);
    tokens_free(tokens);
    ast_free(ast);
    result_free(result);
    #endif

    #ifdef TEST_MODE 
    // get the path from command arg.
    const char *path = argv[1];
    // load racket file content into memory.
    Raw_Code *raw_code = racket_file_load(path);
    // tokenizer 
    Tokens *tokens = tokenizer(raw_code); 
    // parser
    AST ast = parser(tokens);
    Visitor custom_visitor = get_custom_visitor();
    // calculator
    Result result = calculator(ast, NULL);
    traverser(result, custom_visitor, NULL);
    // release memory
    racket_file_free(raw_code);
    tokens_free(tokens);
    ast_free(ast);
    result_free(result);
    visitor_free(custom_visitor);
    #endif

    #ifdef DEBUG_MODE
    // get the path from command arg.
    const char *path = argv[1];
    // load racket file content into memory.
    Raw_Code *raw_code = racket_file_load(path);

    // show the raw_code in gaint single string.
    printf("Raw code:\n");
    racket_file_map(raw_code, print_raw_code, NULL);
    printf("\n");

    // tokenizer 
    Tokens *tokens = tokenizer(raw_code);
    printf("Tokens:\n");
    tokens_map(tokens, print_tokens, NULL);
    printf("\n");

    // parser
    AST ast = parser(tokens);
    // show ast by traverser
    Visitor custom_visitor = get_custom_visitor();
    traverser(ast, custom_visitor, NULL);
    // ast copy and show it
    AST ast_copy = ast_node_deep_copy(ast, NULL);
    traverser(ast_copy, custom_visitor, NULL);
    printf("\n");

    // calculator
    Result result = calculator(ast, NULL);
    // show result by traverser
    printf("Result:\n");
    traverser(result, custom_visitor, NULL);
    printf("\n");
    
    // release memory
    racket_file_free(raw_code);
    tokens_free(tokens);
    ast_free(ast);
    result_free(result);
    visitor_free(custom_visitor);
    #endif

    return 0;
}