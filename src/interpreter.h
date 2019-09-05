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
    CHARACTER, /* #\a */
    BOOLEAN /* #t #f */
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

// parser parts
typedef enum _z_ast_node_tag{
    IN_AST, // this kind of ast_node will be freed in ast_free() in main.c.
    NOT_IN_AST, // fresh data, not in ast.
    BUILT_IN_PROCEDURE, // built-in procedure.
    BUILT_IN_BINDING // built-in binding.
} AST_Node_Tag;
typedef void (*Function)(void); // Function points to any type of function.
typedef enum _z_ast_node_type {
    Number_Literal, String_Literal, Character_Literal,
    List_Literal, Pair_Literal, Boolean_Literal,
    Local_Binding_Form, Conditional_Form, Lambda_Form,
    Call_Expression, Binding, Procedure, Program
} AST_Node_Type;
typedef enum _z_local_binding_form_type {
    DEFINE, LET, LET_STAR, LETREC
} Local_Binding_Form_Type;
typedef enum _z_conditional_form_type {
    IF, COND, AND, NOT, OR
} Conditional_Form_Type;
typedef enum _z_boolean_type {
    R_FALSE, R_TRUE // any value other than #f counts as true.
} Boolean_Type;
typedef struct _z_ast_node {
    struct _z_ast_node *parent;
    Vector *context; // optional
    AST_Node_Type type;
    /*
        context: AST_Node *[] type: binding.
        if contextable AST_Node, it will have this, if not contextable, set this to null.
        initially, the AST_Node with type Program will have context, and other cases will be generated in generate_context() function.
        such as let's body_exprs will have context, even Number_Literal.
    */ 
    AST_Node_Tag tag;
    union {
        struct {
            /*  
              value filed:
               char * - normally literal value, such as "123.999", and set the c_native_value to 123.999(double). 
               Boolean_Type * - such as #f or #t, set c_native_value to null.
               Vector * - list or pair literal, store the contents into elements(AST_Node *[]), and c_native_value set to null.
            */
            void *value; 
            // convert normally literal value to c_native_value, such as double: 123.999, when list, pair, boolean, character, string set this field to null.
            void *c_native_value; 
        } literal;
        struct { // local binding form: define, let, let*, letrec.
            Local_Binding_Form_Type type;
            union {
                struct { // let let* letrec
                    Vector *bindings; // AST_Node *(type: Binding)[]
                    Vector *body_exprs; // AST_Node *[]
                } lets;
                struct {
                    struct _z_ast_node *binding;
                } define;
            } contents;
        } local_binding_form;
        struct { // conditionals form: if, cond, and, or.
            Conditional_Form_Type type;
            union {
                // if
                struct {
                    struct _z_ast_node *test_expr;
                    struct _z_ast_node *then_expr;
                    struct _z_ast_node *else_expr;
                } if_expression;
                // cond
                struct { // and
                    Vector *exprs; // AST_Node *[]
                } and_expression;
                struct { // or
                    Vector *exprs; // AST_Node *[]
                } or_expression;
                struct { // not
                    struct _z_ast_node *expr;
                } not_expression;
            } contents;
        } conditional_form;
        struct { // case: let ... [a 1] 'value' field will have a value, case: a (single variable identifier) 'value' field set to null.
            char *name; // binding's name.
            struct _z_ast_node *value; // binding's value, pointes to a AST_Node.
        } binding;
        struct { // call_expression: (+ 1 2) etc, excludes loacl bingding form or other special form such as let define if etc, just simple function call.
            // if a procedure has name, set anonymous_procedure to NULL
            // if a procedure has no name, set name to NULL
            char *name; // search procedure by name.
            struct _z_ast_node *anonymous_procedure; // anonymous function call, can not found fn by name.
            Vector *params; // params is AST_Node *[]
        } call_expression; // call expression
        struct  {
            char *name; // copy from initial binding's name.
            int required_params_count; // only impl required-args right now.
            Vector *params; // AST_Node *[] type: binding, set binding.value to null when define a function, just record the variable's name.
            Vector *body_exprs; // AST_Node *[]
            Function c_native_function;
        } procedure;
        struct {
            Vector *params; // AST_Node *[] type: binding, set binding.value to null when define a function, just record the variable's name.
            Vector *body_exprs; // AST_Node *[]
        } lambda_form;
        struct {
            Vector *body; // program's body is AST_Node *[]
            Vector *built_in_bindings; // like context in AST_Node, store the built-in bindings.
        } program;
    } contents;
} AST_Node;
typedef AST_Node *AST; // AST_Node whos type is Program, the AST is a pointer to this kind of AST_Node, AST is an abstract of 'abstract syntax tree'.
AST_Node *ast_node_new(AST_Node_Tag tag, AST_Node_Type type, ...);
int ast_node_free(AST_Node *ast_node);
AST_Node *ast_node_deep_copy(AST_Node *ast_node, void *aux_data);
void ast_node_set_tag(AST_Node *ast_node, AST_Node_Tag tag);
AST_Node_Tag ast_node_get_tag(AST_Node *ast_node);
AST parser(Tokens *tokens); // retrun AST.
int ast_free(AST ast);
typedef Vector *Visitor; // AST_Node_Handler *[]
typedef void (*VisitorFunction)(AST_Node *node, AST_Node *parent, void *aux_data);
typedef struct _z_ast_node_handler {
    AST_Node_Type type;
    VisitorFunction enter;
    VisitorFunction exit;
} AST_Node_Handler;
Visitor visitor_new();
int visitor_free(Visitor visitor);
AST_Node_Handler *ast_node_handler_new(AST_Node_Type type, VisitorFunction enter, VisitorFunction exit);
int ast_node_handler_free(AST_Node_Handler *handler);
int ast_node_handler_append(Visitor visitor, AST_Node_Handler *handler);
AST_Node_Handler *find_ast_node_handler(Visitor visitor, AST_Node_Type type);
Visitor get_defult_visitor(void);
void traverser(AST ast, Visitor visitor, void *aux_data); // left-sub-tree-first dfs algo. 

// calculator parts
typedef AST_Node *Result; // the result of whole racket code.
void generate_context(AST_Node *node, AST_Node *parent, void *aux_data);
Vector *calculator(AST ast, void *aux_data);
int results_free(Vector *results);
Result eval(AST_Node *ast_node, void *aux_data);
void output_results(Vector *results);

#endif