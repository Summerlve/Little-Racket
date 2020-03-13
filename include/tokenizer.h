#ifndef TOKENIZER
#define TOKENIZER

#include "load_racket_file.h"
#include <stddef.h>

/* define ascii character here */
#define LANGUAGE_SIGN "lang"
#define RACKET_SIGN "racket"
#define WHITE_SPACE 0x20 // ' '
#define POUND 0x23 // '#'
#define LEFT_PAREN 0x28 // '('
#define RIGHT_PAREN 0x29 // ')'
#define LEFT_SQUARE_BRACKET 0x5b // '['
#define RIGHT_SQUARE_BRACKET 0x5d // ']'
#define APOSTROPHE 0x27 // '\''
#define DOT 0x2e // '.'
#define SEMICOLON 0x3b // ';'
#define DOUBLE_QUOTE 0x22 // '\"'
#define BACK_SLASH 0x5c // '\'
#define BAR 0x2d // '-'

// number type
typedef struct _z_number {
    unsigned char *contents; // use a dynamic null-terminated string to store a number
    size_t logical_length; // logical length
    size_t allocated_length; // allocated length
} Number_Type;
Number_Type *number_type_new(void);
int number_type_free(Number_Type *number);
int number_type_append(Number_Type *number, const unsigned char ch);
unsigned char *get_number_type_contents(Number_Type *number);

// token type
typedef enum _z_token_type {
    LANGUAGE, /* whcih language are used, supports only: #lang racket */
    IDENTIFIER, /* except for the sequences of characters that make number constants, means can not full of numbers */
    COMMENT, /* supports ';' single line comment */
    PUNCTUATION,
    /*
        PAREN: ( )
        SQUARE_BRACKET: [ ]
        APOSTROPHE: ' such as '(1 2 3) list, or pair '(1 . 2) 
        DOT: . such as '(1 . 2) pair, or decimal fraction such as: 1.456
    */
    NUMBER,
    STRING, /* "xxx", racket supports multilines string */
    CHARACTER, /* #\a */
    BOOLEAN /* #t #f */
} Token_Type;
typedef struct _z_token {
    Token_Type type;
    unsigned char *value;
} Token;
typedef struct _z_tokens {
    Token **contents; // store all tokens here, Token []
    size_t logical_length; // logical length
    size_t allocated_length; // allocated length
} Tokens;
Token *token_new(Token_Type type, const unsigned char *value);
int token_free(Token *token);
typedef void (*TokensMapFunction)(const Token *token, void *aux_data); // tokens map function
Tokens *tokens_new();
int tokens_free(Tokens *tokens);
int add_token(Tokens *tokens, Token *token);
size_t tokens_length(Tokens *tokens);
Token *tokens_nth(Tokens *tokens, size_t index);
void tokens_map(Tokens *tokens, TokensMapFunction map, void *aux_data);
Tokens *tokenizer(Raw_Code *raw_code); // return tokens here, remember free the memory

#endif