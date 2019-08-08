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
    printf("Raw code:\n");
    racket_file_map(raw_code, print_raw_code, NULL);
    printf("\n");

    // tokenizer 
    Tokens *tokens = tokenizer(raw_code); 

    // show the tokens.
    printf("Tokens:\n");
    tokens_map(tokens, print_tokens, NULL);

    // parser
    AST ast = parser(tokens);

    // show ast by traverser
    Visitor custom_visitor = get_custom_visitor();
    traverser(ast, custom_visitor, NULL);

    // ast copy
    AST ast_copy = ast_node_deep_copy(ast, NULL);
    printf("--- ast copy starts ---\n");
    traverser(ast_copy, custom_visitor, NULL);
    printf("--- ast copy ends ---\n");

    // calculator
    Result result = calculator(ast_copy, NULL);

    // show result by traverser
    printf("Result:\n");
    traverser(result, custom_visitor, NULL);

    // release memory
    racket_file_free(raw_code);
    tokens_free(tokens);
    ast_free(ast);
    result_free(result);
    visitor_free(custom_visitor);
    
    return 0;
}