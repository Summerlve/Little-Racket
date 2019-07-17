#ifndef RACKET_BUILT_IN
#define RACKET_BUILT_IN

#include "./interpreter.h"

// AST_Node *[] type: binding.
Vector *generate_built_in_bindings(void);
int free_built_in_bindings(Vector *built_in_bindings, VectorFreeFunction free_fn);
// operands: AST_Node *[], type: binding, value's type: Number_Literal. 
AST_Node *racket_native_plus(int n, Vector *operands);

#endif