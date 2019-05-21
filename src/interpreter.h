#ifndef INTERPRETER
#define INTERPRETER

#include "./load_racket_file.h"

// tokenizer part
typedef enum _z_token_type {
    LANGUAGE, /* whcih language are used, supports only: #lang racket */
    COMMENT, /* supports ';' single line comment */ 
    IDENTIFIER,
    PAREN, /* ( ) */
    SQUARE_BRACKET, /* [ ] */
    NUMBER,
    STRING,
    CHARACTER,
    APOSTROPHE, /* ' such as '(1 2 3) list */
    DOT /* . such as '(1 . 2) pair */ 
} Token_Type;
typedef struct _z_token {
    Token_Type type;
    char *value;
} Token;
typedef struct _z_tokens {
    Token *contents; // store all tokens here, Token [].
    int logical_length; // logical length.
    int allocated_length; // allocated length.
} Tokens;
typedef void (*TokensMapFunction)(const Token *token, void *aux_data); // tokens map function.
Tokens *tokenizer(Raw_Code *raw_code); // return tokens here, remember free the memory.
int free_tokens(Tokens *tokens);
void tokens_map(Tokens *tokens, TokensMapFunction map, void *aux_data);

// parser

// calculator

#endif