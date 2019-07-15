#ifndef RACKET_BUILT_IN
#define RACKET_BUILT_IN

#include "./interpreter.h"

AST_Node *racket_native_plus(int n, ...); // ... must be AST_Node *

#endif