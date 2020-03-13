#ifndef RACKET_BUILT_IN
#define RACKET_BUILT_IN

#include "vector.h"

// return: AST_Node *[] type: binding.
Vector *generate_built_in_bindings(void);
int free_built_in_bindings(Vector *built_in_bindings, VectorFreeFunction free_fn);

#endif