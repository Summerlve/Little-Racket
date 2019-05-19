#ifndef INTERPRETER
#define INTERPRETER

#include "./load_racket_file.h"

// tokenizer part
typedef enum _z_token_type {
    LANGUAGE, /* whcih language are used, supports only: #lang racket */
    COMMENT, /* supports ';' single line comments */ 
    IDENTIFIER,
    PAREN,
    SQUAER_BRACKET,
    NUMBER,
    STRING,
    CHARACTER,
    LIST,
    PAIR
} Token_Type;
typedef struct _z_token {
    Token_Type type;
    const char *value;
} Token;
typedef struct _z_tokens {
    Token *content; // store all tokens here, Token [].
    int logical_length; // logical length.
    int allocated_length; // allocated length.
} Tokens;
typedef void (*TokensMapFunction)(const Token *token, void *aux_data); // racket file lines map function.
void tokens_map(Tokens *tokens, TokensMapFunction map, void *aux_data);

// return tokens here, remember free the memory.
Tokens *tokenizer(Raw_Code *raw_code);

// parser

// calculator

#endif