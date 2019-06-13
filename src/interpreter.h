#ifndef INTERPRETER
#define INTERPRETER

#include "./load_racket_file.h"
#include "./vector.h"

// tokenizer parts
// number type
typedef struct _z_number {
    char *contents; // use a dynamic null-terminated string to store a number.
    int logical_length; // logical length.
    int allocated_length; // allocated length.
} Number_Type;
typedef enum _z_token_type {
    LANGUAGE, /* whcih language are used, supports only: #lang racket */
    IDENTIFIER, /* except for the sequences of characters that make number constants, means can not full of numbers. */
    COMMENT, /* supports ';' single line comment */
    PUNCTUATION,
    /*
        PAREN: ( )
        SQUARE_BRACKET: [ ]
        APOSTROPHE: ' such as '(1 2 3) list, or symbol such as: 'a
        DOT: . such as '(1 . 2) pair, or decimal fraction such as: 1.456
    */
    NUMBER,
    STRING, /* "xxx", racket supports multilines string */
    CHARACTER /* #\a */
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

// parser parts
typedef enum _z_ast_node_type {
    Number_Literal, String_Literal, Character_Literal,
    Call_Expression, List_literal, Pair_Literal,
    Program
} AST_Node_Type;
typedef struct _z_ast_node {
    AST_Node_Type type; // 4 bytes
    Vector *body;
    Vector *params;
    const char *name;
    const char *value;
} AST_Node;
typedef AST_Node *AST; // AST_Node whos type is Program, the AST is a pointer to this kind of AST_Node.
typedef void (*AST_NodesMapFunction)(const AST_Node *ast_node, void *aux_data);
typedef void (*Visitor)(void);
AST parser(Tokens *tokens); // retrun AST.
int free_ast(AST ast);
void traverser(AST ast, Visitor visitor, void *aux_data); // left-sub-tree-first dfs algo. 

// calculator

#endif