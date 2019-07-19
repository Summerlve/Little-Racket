#ifndef RACKET_BUILT_IN
#define RACKET_BUILT_IN

#include "./interpreter.h"
#define racket_null NULL // null is equal to empty is equal to '(), both of them are list not a pair.
#define racket_empty NULL
// AST_Node *[] type: binding.
Vector *generate_built_in_bindings(void);
int free_built_in_bindings(Vector *built_in_bindings, VectorFreeFunction free_fn);
// operands: AST_Node *[], type: binding, value's type: Number_Literal. 
AST_Node *racket_native_plus(AST_Node *procedure, Vector *operands);

#endif