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
    Vector *results = calculator(ast, NULL);

    // output results
    output_results(results);

    // release memory
    racket_file_free(raw_code);
    tokens_free(tokens);
    ast_free(ast);
    results_free(results);
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

    // calculator
    Vector *results = calculator(ast, NULL);

    // output results
    output_results(results);

    // release memory
    racket_file_free(raw_code);
    tokens_free(tokens);
    ast_free(ast);
    results_free(results);
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
    Vector *results = calculator(ast, NULL);

    // show result by traverser
    printf("\nResult:\n");
    for (int i = 0; i < VectorLength(results); i++)
    {
        Result result = *(AST_Node **)VectorNth(results, i);
        traverser(result, custom_visitor, NULL);
        printf("\n");
    }
    
    // release memory
    racket_file_free(raw_code);
    tokens_free(tokens);
    ast_free(ast);
    results_free(results);
    visitor_free(custom_visitor);
    #endif

    return 0;
}