#ifndef RACKET_BUILT_IN
#define RACKET_BUILT_IN

#include "./interpreter.h"

// null is equal to empty is equal to '(), both of them are list not a pair.
#define racket_null NULL 
#define racket_empty NULL

// AST_Node *[] type: binding.
Vector *generate_built_in_bindings(void);
int free_built_in_bindings(Vector *built_in_bindings, VectorFreeFunction free_fn);

// standard library procedures
// operands: AST_Node *[], type: binding, value's type: Number_Literal. 
AST_Node *racket_native_addition(AST_Node *procedure, Vector *operands); // +
AST_Node *racket_native_subtraction(AST_Node *procedure, Vector *operands); // -
AST_Node *racket_native_multiplication(AST_Node *procedure, Vector *operands); // *
AST_Node *racket_native_division(AST_Node *procedure, Vector *operands); // / 
AST_Node *racket_native_number_euqal(AST_Node *procedure, Vector *operands); // =

#endif