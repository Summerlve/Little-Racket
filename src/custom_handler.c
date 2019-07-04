#include "./custom_handler.h"
#include "./interpreter.h"

// handler function can not be static, it's referenced by AST_Node_Handler or use in other file, so it's must be 'public'
void print_raw_code(const char *line, void *aux_data)
{
    printf("%s", line);
}

void print_tokens(const Token *token, void *aux_data)
{
    printf("type: %d, value: %s\n", token->type, token->value);
}

void program_enter(AST_Node *node, AST_Node *parent, void *aux_data)
{
    printf("--- Program starts ---\n");
}

void program_exit(AST_Node *node, AST_Node *parent, void *aux_data)
{
    printf("\n--- Program ends ---\n");
}

void call_expression_enter(AST_Node *node, AST_Node *parent, void *aux_data)
{
    printf(" (%s ", node->contents.call_expression.name);
}

void call_expression_exit(AST_Node *node, AST_Node *parent, void *aux_data)
{
    printf(" ) ");
}

void list_enter(AST_Node *node, AST_Node *parent, void *aux_data)
{
    printf(" '( ");
}

void list_exit(AST_Node *node, AST_Node *parent, void *aux_data)
{
    printf(" ) ");
}

void pair_enter(AST_Node *node, AST_Node *parent, void *aux_data)
{
    printf(" '( ");
}

void pair_exit(AST_Node *node, AST_Node *parent, void *aux_data)
{
    printf(" ) ");
}

void number_enter(AST_Node *node, AST_Node *parent, void *aux_data)
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

void character_enter(AST_Node *node, AST_Node *parent, void *aux_data)
{
    printf(" #\\%c ", *(char *)(node->contents.literal.value));
    if (parent->type == Pair_Literal)
    {
        AST_Node *car = *(AST_Node **)VectorNth((Vector *)(parent->contents.literal.value), 0);    
        if (node == car)
        {
            printf(" . ");
        }
    }
}

void string_enter(AST_Node *node, AST_Node *parent, void *aux_data)
{
    printf(" \"%s\" ", (char *)(node->contents.literal.value));
    if (parent->type == Pair_Literal)
    {
        AST_Node *car = *(AST_Node **)VectorNth((Vector *)(parent->contents.literal.value), 0);    
        if (node == car)
        {
            printf(" . ");
        }
    }
}

void binding_enter(AST_Node *node, AST_Node *parent, void *aux_data)
{
    printf(" %s ", (char *)(node->contents.binding.name));
}

void let_enter(AST_Node *node, AST_Node *parent, void *aux_data)
{
    
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
    append_ast_node_handler(visitor, Binding, binding_enter, NULL);
    return visitor;
}