#ifndef ZADDON
#define ZADDON

#include "tokenizer.h"
#include "parser.h"
#include "vector.h"

AST_Node *racket_addon_string_sha256(AST_Node *procedure, Vector *operands);
Vector *generate_addon_bindings(void);
int free_addon_bindings(Vector *addon_bindings, VectorFreeFunction free_fn);

#endif