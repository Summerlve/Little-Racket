#include "./custom_handler.h"
#include "./interpreter.h"

void print_raw_code(const char *line, void *aux_data)
{
    printf("%s", line);
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
    if (parent->type == Pair_Literal)
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
    if (parent->type == Pair_Literal)
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
    if (parent->type == Pair_Literal)
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
    if (node->contents.binding.value == NULL)
    {
        // just single variable identifier here. print variable name only.
        printf(" %s ", (char *)(node->contents.binding.name));
        return;
    }

    // question here is: does it exists AST_Node:Binding with parent isnt AST_Node:Local_Binding_Form ?
    if (parent->type == Local_Binding_Form)
    {
        if (parent->contents.local_binding_form.type == LET)
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
    printf(" %s ", (char *)(node->contents.binding.name));
}

static void binding_exit(AST_Node *node, AST_Node *parent, void *aux_data)
{
    if (node->contents.binding.value == NULL)
    {
        // just single variable identifier here. do nothing here.
        return;
    }

    if (parent->type == Local_Binding_Form)
    {
        if (parent->contents.local_binding_form.type == LET)
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
}

static void local_binding_form_enter(AST_Node *node, AST_Node *parent, void *aux_data)
{
    // let
    if (node->contents.local_binding_form.type == LET)
    {
        printf(" (let ");
    }

    // let*

    // ...
}

static void local_binding_form_exit(AST_Node *node, AST_Node *parent, void *aux_data)
{
    // let
    if (node->contents.local_binding_form.type == LET)
    {
        printf(" )");
    }

    // let*

    // ...
}

Visitor get_custom_visitor(void)
{
    Visitor visitor = visitor_new();
    append_ast_node_handler(visitor, Program, program_enter, program_exit);
    append_ast_node_handler(visitor, Call_Expression, call_expression_enter, call_expression_exit);
    append_ast_node_handler(visitor, List_literal, list_enter, list_exit);
    append_ast_node_handler(visitor, Pair_Literal, pair_enter, pair_exit);
    append_ast_node_handler(visitor, Number_Literal, number_enter, NULL);
    append_ast_node_handler(visitor, Character_Literal, character_enter, NULL);
    append_ast_node_handler(visitor, String_Literal, string_enter, NULL);
    append_ast_node_handler(visitor, Binding, binding_enter, binding_exit);
    append_ast_node_handler(visitor, Local_Binding_Form, local_binding_form_enter, local_binding_form_exit);
    return visitor;
}