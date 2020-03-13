#ifndef PARSER
#define PARSER

#include "tokenizer.h"
#include "vector.h"
#include <stddef.h>

// parser parts
typedef enum _z_ast_node_tag{
    IN_AST, // this kind of ast_node will be freed in ast_free() in main.c
    NOT_IN_AST, // fresh data, not in ast
    BUILT_IN_PROCEDURE, // built-in procedure
    BUILT_IN_BINDING, // built-in binding
    ADDON_PROCEDURE, // addon procedure
    ADDON_BINDING, // addon binding
    IMMUTABLE // read only value, can not be changed
} AST_Node_Tag;
typedef void (*Function)(void); // Function points to any type of function
typedef enum _z_ast_node_type {
    Number_Literal, String_Literal, Character_Literal,
    List_Literal, Pair_Literal, Boolean_Literal,
    Local_Binding_Form, Set_Form, Conditional_Form, Lambda_Form,
    Call_Expression, Binding, Procedure, Program, Cond_Clause,
    NULL_Expression, EMPTY_Expression
} AST_Node_Type;
typedef enum _z_local_binding_form_type {
    DEFINE, LET, LET_STAR, LETREC
} Local_Binding_Form_Type;
typedef enum _z_conditional_form_type {
    IF, COND, AND, NOT, OR
} Conditional_Form_Type;
typedef enum _z_boolean_type {
    R_FALSE, R_TRUE // any value other than #f counts as true
} Boolean_Type;
typedef enum _z_cond_clause_type {
    TEST_EXPR_WITH_THENBODY, ELSE_STATEMENT, TEST_EXPR_WITH_PROC, SINGLE_TEST_EXPR
} Cond_Clause_Type;
typedef struct _z_ast_node AST_Node;
typedef struct _z_ast_node {
    AST_Node *parent;
    Vector *context; // optional
    AST_Node_Type type;
    /*
        context: AST_Node *[] type: binding
        if contextable AST_Node, it will have this, if not contextable, set this to null
        initially, the AST_Node with type Program will have context, and other cases will be generated in generate_context() function
        such as let's body_exprs will have context, even Number_Literal
    */ 
    AST_Node_Tag tag;
    union {
        struct {
            /*  
              value filed:
               unsigned char * - normally literal value, such as "123.999", and set the c_native_value to 123.999(double) 
               Boolean_Type * - such as #f or #t, set c_native_value to null
               Vector * - list or pair literal, store the contents into elements(AST_Node *[]), and c_native_value set to null
            */
            void *value; 
            // convert normally literal value to c_native_value, such as double: 123.999 or long long int: 87178291200, when list, pair, boolean, character, string set this field to null
            void *c_native_value; 
        } literal;
        struct { // local binding form: define, let, let*, letrec
            Local_Binding_Form_Type type;
            union {
                struct { // let let* letrec
                    Vector *bindings; // AST_Node *(type: Binding)[]
                    Vector *body_exprs; // AST_Node *[]
                } lets;
                struct {
                    AST_Node *binding;
                } define;
            } contents;
        } local_binding_form;
        struct {
            AST_Node *id; // binding with no value
            AST_Node *expr;
        } set_form;
        struct { // conditional form: if, cond, and, or, not
            Conditional_Form_Type type;
            union {
                // if
                struct {
                    AST_Node *test_expr;
                    AST_Node *then_expr;
                    AST_Node *else_expr;
                } if_expression;
                struct { // cond
                    Vector *cond_clauses; // Cond_Clause *[]
                } cond_expression;
                struct { // and
                    Vector *exprs; // AST_Node *[]
                } and_expression;
                struct { // or
                    Vector *exprs; // AST_Node *[]
                } or_expression;
                struct { // not
                    AST_Node *expr;
                } not_expression;
            } contents;
        } conditional_form;
        struct {
            Cond_Clause_Type type;
            AST_Node *test_expr; // set to null when type is ELSE_STATEMENT
            Vector *then_bodies; // AST_Node *[]
            AST_Node *proc_expr; // when TEST_EXPR_WITH_PROC
        } cond_clause;
        struct { // case: let ... [a 1] 'value' field will have a value, case: a (single variable identifier) 'value' field set to null
            unsigned char *name; // binding's name
            AST_Node *value; // binding's value, pointes to a AST_Node
        } binding;
        struct { // call_expression: (+ 1 2) etc, excludes loacl bingding form or other special form such as let define if etc, just simple function call
            // if a procedure has name, set anonymous_procedure to NULL
            // if a procedure has no name, set name to NULL
            unsigned char *name; // search procedure by name
            AST_Node *anonymous_procedure; // anonymous function call, can not found fn by name
            Vector *params; // params is AST_Node *[]
        } call_expression; // call expression
        struct  {
            unsigned char *name; // copy from initial binding's name
            size_t required_params_count; // only impl required-args right now
            Vector *params; // AST_Node *[] type: binding, set binding.value to null when define a function, just record the variable's name
            Vector *body_exprs; // AST_Node *[]
            Function c_native_function;
        } procedure;
        struct {
            Vector *params; // AST_Node *[] type: binding, set binding.value to null when define a function, just record the variable's name
            Vector *body_exprs; // AST_Node *[]
        } lambda_form;
        struct {
            Vector *body; // program's body is AST_Node *[]
            Vector *built_in_bindings; // like context in AST_Node, store the built-in bindings
            Vector *addon_bindings; // like context in AST_Node, store the addon bindings
        } program;
        struct {
            AST_Node *value; // '()
        } null_expression;
        struct {
            AST_Node *value; // '()
        } empty_expression;
    } contents;
} AST_Node;
typedef AST_Node *AST; // AST_Node whos type is Program, the AST is a pointer to this kind of AST_Node, AST is an abstract of 'abstract syntax tree'
AST_Node *ast_node_new(AST_Node_Tag tag, AST_Node_Type type, ...);
int ast_node_free(AST_Node *ast_node);
AST_Node *ast_node_deep_copy(AST_Node *ast_node, void *aux_data);
void ast_node_set_tag(AST_Node *ast_node, AST_Node_Tag tag);
AST_Node_Tag ast_node_get_tag(AST_Node *ast_node);
AST parser(Tokens *tokens); // retrun AST
int ast_free(AST ast);
typedef Vector *Visitor; // AST_Node_Handler *[]
typedef void (*VisitorFunction)(AST_Node *node, AST_Node *parent, void *aux_data);
typedef struct _z_ast_node_handler {
    AST_Node_Type type;
    VisitorFunction enter; // or NULL
    VisitorFunction exit; // or NULL
} AST_Node_Handler;
Visitor visitor_new();
int visitor_free(Visitor visitor);
AST_Node_Handler *ast_node_handler_new(AST_Node_Type type, VisitorFunction enter, VisitorFunction exit);
int ast_node_handler_free(AST_Node_Handler *handler);
int ast_node_handler_append(Visitor visitor, AST_Node_Handler *handler);
AST_Node_Handler *find_ast_node_handler(Visitor visitor, AST_Node_Type type);
Visitor get_defult_visitor(void);
void traverser(AST ast, Visitor visitor, void *aux_data); // left-sub-tree-first dfs algo

#endif