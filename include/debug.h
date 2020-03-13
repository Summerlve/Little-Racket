#ifndef ZDEBUG 
#define ZDEBUG

#include "tokenizer.h"
#include "parser.h"

void print_raw_code(const unsigned char *line, void *aux_data);
void print_tokens(const Token *token, void *aux_data);
Visitor get_custom_visitor(void);

#endif