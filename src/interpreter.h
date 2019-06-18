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
        APOSTROPHE: ' such as '(1 2 3) list, or pair '(1 . 2) 
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
    Token **contents; // store all tokens here, Token [].
    int logical_length; // logical length.
    int allocated_length; // allocated length.
} Tokens;
typedef void (*TokensMapFunction)(const Token *token, void *aux_data); // tokens map function.
Tokens *tokenizer(Raw_Code *raw_code); // return tokens here, remember free the memory.
int tokens_free(Tokens *tokens);
void tokens_map(Tokens *tokens, TokensMapFunction map, void *aux_data);

// racket value mapping to c native value.
#define racket_null NULL // null is equal to empty is equal to '(), both of them are list not a pair.
#define racket_empty NULL
typedef struct _z_racket_value_box {
    void *value;
    char *signature;
    int elem_size;
} RacketValueBox; // single value wrapper.
typedef struct _z_racket_pair {
    RacketValueBox car;
    RacketValueBox cdr;
} Racket_Pair;

// racket built-in procedure mapping to c native function.

// parser parts
typedef void (*Function)(void); // Function points to any type of function.
typedef enum _z_ast_node_type {
    Number_Literal, String_Literal, Character_Literal,
    List_literal, Pair_Literal,
    Call_Expression, Binding, Program
} AST_Node_Type;
typedef struct _z_ast_node {
    AST_Node_Type type; // 4 bytes
    union _z_ast_node_contents {
        struct _z_ast_node_literal {
            char *value; // literal
            void *c_native_value; // convert value to c_native_value.
        } literal;
        char *name; // binding
        struct _z_ast_node_call_expression {
            char *name;
            Vector *params; // params is AST_Node *[]
            Function c_native_function; // function ponits to any type of function.
        } call_expression; // call expression
        Vector *body; // program's body is AST_Node *[]
    } contents; // 24 bytes
} AST_Node; // 28 bytes
typedef AST_Node *AST; // AST_Node whos type is Program, the AST is a pointer to this kind of AST_Node.
typedef void (*Visitor)(AST_Node *node, AST_Node *root, void *aux_data);
AST parser(Tokens *tokens); // retrun AST.
int ast_free(AST ast);
void traverser(AST ast, Visitor visitor, void *aux_data); // left-sub-tree-first dfs algo. 

// calculator parts

#endif