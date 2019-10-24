#include "custom_handler.h"
#include "interpreter.h"

void print_raw_code(const char *line, void *aux_data)
{
    printf("%s\n", line);
}

void print_tokens(const Token *token, void *aux_data)
{
    printf("type: %d, value: %s\n", token->type, token->value);
}

static void program_enter(AST_Node *node, AST_Node *parent, void *aux_data)
{
    printf("--- Program starts ---\n");
}

static void program_exit(AST_Node *node, AST_Node *parent, void *aux_data)
{
    printf("\n--- Program ends ---\n");
}

static void call_expression_enter(AST_Node *node, AST_Node *parent, void *aux_data)
{
    printf("(%s ", node->contents.call_expression.name);
}

static void call_expression_exit(AST_Node *node, AST_Node *parent, void *aux_data)
{
    printf(") ");
}

static void procedure_enter(AST_Node *node, AST_Node *parent, void *aux_data)
{
    const char *name = node->contents.procedure.name;
    if (name != NULL) printf("#<procedure:%s>\n", name);
    else printf("#<procedure:anonymous>\n");
}

static void list_enter(AST_Node *node, AST_Node *parent, void *aux_data)
{
    printf(" '(");
}

static void list_exit(AST_Node *node, AST_Node *parent, void *aux_data)
{
    printf(") ");
}

static void pair_enter(AST_Node *node, AST_Node *parent, void *aux_data)
{
    printf(" '(");
}

static void pair_exit(AST_Node *node, AST_Node *parent, void *aux_data)
{
    printf(") ");
}

static void number_enter(AST_Node *node, AST_Node *parent, void *aux_data)
{
    printf(" %s ", (char *)(node->contents.literal.value));
    if (parent != NULL && parent->type == Pair_Literal)
    {
        AST_Node *car = *(AST_Node **)VectorNth((Vector *)(parent->contents.literal.value), 0);    
        if (node == car)
        {
            printf(" . ");
        }
    }
}

static void character_enter(AST_Node *node, AST_Node *parent, void *aux_data)
{
    printf("#\\%c ", *(char *)(node->contents.literal.value));
    if (parent != NULL && parent->type == Pair_Literal)
    {
        AST_Node *car = *(AST_Node **)VectorNth((Vector *)(parent->contents.literal.value), 0);    
        if (node == car)
        {
            printf(" . ");
        }
    }
}

static void string_enter(AST_Node *node, AST_Node *parent, void *aux_data)
{
    printf("\"%s\" ", (char *)(node->contents.literal.value));
    if (parent != NULL && parent->type == Pair_Literal)
    {
        AST_Node *car = *(AST_Node **)VectorNth((Vector *)(parent->contents.literal.value), 0);    
        if (node == car)
        {
            printf(" . ");
        }
    }
}

static void binding_enter(AST_Node *node, AST_Node *parent, void *aux_data)
{
    if (parent == NULL && node->contents.binding.value == NULL)
    {
        // just single variable identifier here. print variable name only.
        printf(" %s ", (char *)(node->contents.binding.name));
        return;
    }

    // question here is: does it exists AST_Node:Binding with parent isnt AST_Node:Local_Binding_Form ?
    if (parent != NULL && parent->type == Local_Binding_Form)
    {
        if (parent->contents.local_binding_form.type == LET ||
            parent->contents.local_binding_form.type == LET_STAR ||
            parent->contents.local_binding_form.type == LETREC)
        {
            Vector *bindings = parent->contents.local_binding_form.contents.lets.bindings;
            AST_Node *first = *(AST_Node **)VectorNth(bindings, 0);
            if (node == first) 
            {
                printf(" (");
            }
            printf("[ ");
        }
    }

    if (parent != NULL & parent->type == Lambda_Form) 
    {
        Vector *params = parent->contents.lambda_form.params;
        AST_Node *first = *(AST_Node **)VectorNth(params, 0);
        if (node == first) 
        {
            printf(" (");
        }
    }

    printf(" %s ", (char *)(node->contents.binding.name));
}

static void binding_exit(AST_Node *node, AST_Node *parent, void *aux_data)
{
    if (parent == NULL && node->contents.binding.value == NULL)
    {
        // just single variable identifier here. do nothing here.
        return;
    }

    if (parent != NULL && parent->type == Local_Binding_Form)
    {
        if (parent->contents.local_binding_form.type == LET ||
            parent->contents.local_binding_form.type == LET_STAR ||
            parent->contents.local_binding_form.type == LETREC)
        {
            printf("] ");
            Vector *bindings = parent->contents.local_binding_form.contents.lets.bindings;
            AST_Node *last = *(AST_Node **)VectorNth(bindings, VectorLength(bindings) - 1);
            if (node == last)
            {
                printf(") ");
            }
        }
    }

    if (parent != NULL & parent->type == Lambda_Form) 
    {
        Vector *params = parent->contents.lambda_form.params;
        AST_Node *last = *(AST_Node **)VectorNth(params, VectorLength(params) - 1);
        if (node == last)
        {
            printf(") ");
        }
    }
}

static void local_binding_form_enter(AST_Node *node, AST_Node *parent, void *aux_data)
{
    // let
    if (node->contents.local_binding_form.type == LET)
    {
        printf("(let ");
    }

    // let*
    if (node->contents.local_binding_form.type == LET_STAR)
    {
        printf("(let* ");
    }

    // letrec
    if (node->contents.local_binding_form.type == LETREC)
    {
        printf("(letrec ");
    }

    // define
    if (node->contents.local_binding_form.type == DEFINE)
    {
        printf("(define ");
    }
}

static void local_binding_form_exit(AST_Node *node, AST_Node *parent, void *aux_data)
{
    // let let* letrec
    if (node->contents.local_binding_form.type == LET ||
        node->contents.local_binding_form.type == LET_STAR ||
        node->contents.local_binding_form.type == LETREC)
    {
        printf(") ");
    }

    // define
    if (node->contents.local_binding_form.type == DEFINE)
    {
        printf(") ");
    }
}

static void boolean_enter(AST_Node *node, AST_Node *parent, void *aux_data)
{
    Boolean_Type value = *(Boolean_Type *)(node->contents.literal.value);
    if (value == R_TRUE)
    {
        printf(" #t ");
    }
    else if(value == R_FALSE)
    {
        printf(" #f ");
    }
}

static void conditional_form_enter(AST_Node *node, AST_Node *parent, void *aux_data)
{
    if (node->contents.conditional_form.type == IF)
    {
        printf(" (if ");
    }

    if (node->contents.conditional_form.type == COND)
    {
        printf(" (cond ");
    }

    if (node->contents.conditional_form.type == AND)
    {
        printf(" (and ");
    }

    if (node->contents.conditional_form.type == NOT)
    {
        printf(" (not ");
    }

    if (node->contents.conditional_form.type == OR)
    {
        printf(" (or ");
    }
}

static void conditional_form_exit(AST_Node *node, AST_Node *parent, void *aux_data)
{
    if (node->contents.conditional_form.type == IF)
    {
        printf(" ) ");
    }

    if (node->contents.conditional_form.type == COND)
    {
        printf(" ) ");
    }

    if (node->contents.conditional_form.type == AND)
    {
        printf(" ) ");
    }

    if (node->contents.conditional_form.type == NOT)
    {
        printf(" ) ");
    }

    if (node->contents.conditional_form.type == OR)
    {
        printf(" ) ");
    }
}

static void cond_clause_enter(AST_Node *node, AST_Node *parent, void *aux_data)
{
    if (node->contents.cond_clause.type == TEST_EXPR_WITH_THENBODY)
    {
        printf(" [ ");
    }

    if (node->contents.cond_clause.type == ELSE_STATEMENT)
    {
        printf(" [else ");
    }
}

static void cond_clause_exit(AST_Node *node, AST_Node *parent, void *aux_data)
{
    if (node->contents.cond_clause.type == TEST_EXPR_WITH_THENBODY)
    {
        printf(" ] ");
    }

    if (node->contents.cond_clause.type == ELSE_STATEMENT)
    {
        printf(" ] ");
    }
}

static void lambda_form_enter(AST_Node *node, AST_Node *parent, void *aux_data)
{
    printf("(lambda ");
}

static void lambda_form_exit(AST_Node *node, AST_Node *parent, void *aux_data)
{
    printf(" ) ");
}

static void set_form_enter(AST_Node *node, AST_Node *parent, void *aux_data)
{
    printf("(set! ");
}

static void set_form_exit(AST_Node *node, AST_Node *parent, void *aux_data)
{
    printf(" ) ");
}

Visitor get_custom_visitor(void)
{
    Visitor visitor = visitor_new();
    AST_Node_Handler *handler = NULL;

    handler = ast_node_handler_new(Program, program_enter, program_exit);
    ast_node_handler_append(visitor, handler);
    handler = ast_node_handler_new(Call_Expression, call_expression_enter, call_expression_exit);
    ast_node_handler_append(visitor, handler);
    handler = ast_node_handler_new(Procedure, procedure_enter, NULL);
    ast_node_handler_append(visitor, handler);
    handler = ast_node_handler_new(List_Literal, list_enter, list_exit);
    ast_node_handler_append(visitor, handler);
    handler = ast_node_handler_new(Pair_Literal, pair_enter, pair_exit);
    ast_node_handler_append(visitor, handler);
    handler = ast_node_handler_new(Number_Literal, number_enter, NULL);
    ast_node_handler_append(visitor, handler);
    handler = ast_node_handler_new(Character_Literal, character_enter, NULL);
    ast_node_handler_append(visitor, handler);
    handler = ast_node_handler_new(String_Literal, string_enter, NULL);
    ast_node_handler_append(visitor, handler);
    handler = ast_node_handler_new(Binding, binding_enter, binding_exit);
    ast_node_handler_append(visitor, handler);
    handler = ast_node_handler_new(Local_Binding_Form, local_binding_form_enter, local_binding_form_exit);
    ast_node_handler_append(visitor, handler);
    handler = ast_node_handler_new(Boolean_Literal, boolean_enter, NULL);
    ast_node_handler_append(visitor, handler);
    handler = ast_node_handler_new(Conditional_Form, conditional_form_enter, conditional_form_exit);
    ast_node_handler_append(visitor, handler);
    handler = ast_node_handler_new(Lambda_Form, lambda_form_enter, lambda_form_exit);
    ast_node_handler_append(visitor, handler);
    handler = ast_node_handler_new(Set_Form, set_form_enter, set_form_exit);
    ast_node_handler_append(visitor, handler);
    handler = ast_node_handler_new(Cond_Clause, cond_clause_enter, cond_clause_exit);
    ast_node_handler_append(visitor, handler);
    
    return visitor;
}