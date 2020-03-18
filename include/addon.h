#ifndef ZADDON
#define ZADDON

#include "parser.h"
#include "vector.h"

#define SHA256_HASH_STRING_LEN ((size_t)64)

AST_Node *racket_addon_string_sha256(AST_Node *procedure, Vector *operands);
Vector *generate_addon_bindings(void);
int free_addon_bindings(Vector *addon_bindings, VectorFreeFunction free_fn);

#endif