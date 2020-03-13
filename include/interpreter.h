#ifndef INTERPRETER
#define INTERPRETER

#include "parser.h"
#include "vector.h"

// calculator parts
typedef AST_Node *Result; // the result of whole racket code
void generate_context(AST_Node *node, AST_Node *parent, void *aux_data);
Result eval(AST_Node *ast_node, void *aux_data);
Vector *calculator(AST ast, void *aux_data);
int results_free(Vector *results);
void output_results(Vector *results, void *aux_data);

#endif