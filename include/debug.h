#ifndef ZDEBUG 
#define ZDEBUG

#include "interpreter.h"

void print_raw_code(const unsigned char *line, void *aux_data);
void print_tokens(const Token *token, void *aux_data);
Visitor get_custom_visitor(void);

#endif