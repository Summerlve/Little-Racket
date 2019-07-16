#ifndef RACKET_BUILT_IN
#define RACKET_BUILT_IN

#include "./interpreter.h"

// AST_Node *[] type: binding.
Vector *generate_built_in_bindings(void);
// operands: AST_Node *[], type: binding, value's type: Number_Literal. 
AST_Node *racket_native_plus(int n, Vector *operands);

#endif