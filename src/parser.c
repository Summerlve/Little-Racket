#include "../include/global.h"
#include "../include/parser.h"
#include "../include/tokenizer.h"
#include "../include/vector.h"
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <stdbool.h>

static AST_Node *walk(Tokens *tokens, size_t *current_p);
static void visitor_free_helper(void *value_addr, size_t index, Vector *vector, void *aux_data);
static void traverser_helper(AST_Node *node, AST_Node *parent, Visitor visitor, void *aux_data);
static void set_tag_rec_visitor_helper(AST_Node *node, AST_Node *parent, void *aux);

/*
    ast_node_new(tag, Program, body/NULL, built_in_bindings/NULL, addon_bindings/NULL)
    ast_node_new(tag, Call_Expression, name/NULL, anonymous_procedure/NULL, params/NULL)
    ast_node_new(tag, Local_Binding_Form, Local_Binding_Form_Type, ...)
    ast_node_new(tag, Local_Binding_Form, DEFINE, unsigned char *name, AST_Node *value)
    ast_node_new(tag, Local_Binding_Form, LET/LET_STAR/LETREC, bindings/NULL, body_exprs/NULL)
    ast_node_new(tag, Binding, name, AST_Node *value/NULL)
    ast_node_new(tag, List or Pair, Vector *value/NULL)
    ast_node_new(tag, xxx_Literal, value)
    ast_node_new(tag, Procedure, name/NULL, required_params_count, params, body_exprs, c_native_function/NULL)
    ast_node_new(tag, Conditional_Form, Conditional_Form_Type, ...)
    ast_node_new(tag, Conditional_Form, IF, test_expr, then_expr, else_expr)
    ast_node_new(tag, Conditional_Form, AND, Vector *exprs/NULL)
    ast_node_new(tag, Conditional_Form, NOT, AST_Node *expr/NULL)
    ast_node_new(tag, Conditional_Form, OR, Vector *exprs/NULL)
    ast_node_new(tag, Conditional_Form, COND, Vector *cond_clauses/NULL)
    ast_node_new(tag, Cond_Clause, Cond_Clause_Type type, AST_Node *test_expr/NULL, Vector *then_bodies/NULL, AST_Node *proc_expr/NULL)
        ast_node_new(tag, Cond_Clause, TEST_EXPR_WITH_THENBODY, test_expr, then_bodies, NULL)
        ast_node_new(tag, Cond_Clause, ELSE_STATEMENT, NULL, then_bodies, NULL)
    ast_node_new(tag, Lambda_Form, params, body_exprs)
    ast_node_new(tag, Set_Form, id/NULL, expr/NULL)
    ast_node_new(tag, NULL_Expression)
    ast_node_new(tag, EMPTY_Expression)
*/
AST_Node *ast_node_new(AST_Node_Tag tag, AST_Node_Type type, ...)
{
    AST_Node *ast_node = (AST_Node *)malloc(sizeof(AST_Node));
    ast_node->tag = tag;
    ast_node->type = type;
    ast_node->parent = NULL;
    ast_node->context = NULL; 
    bool matched = false;

    // flexible args
    va_list ap;
    va_start(ap, type);

    if (ast_node->type == Program)
    {
        matched = true;
        Vector *body = va_arg(ap, Vector *);
        if (body == NULL) body = VectorNew(sizeof(AST_Node *));
        Vector *built_in_bindings = va_arg(ap, Vector *);
        if (built_in_bindings == NULL) built_in_bindings = VectorNew(sizeof(AST_Node *));
        Vector *addon_bindings = va_arg(ap, Vector *);
        if (addon_bindings == NULL) addon_bindings = VectorNew(sizeof(AST_Node *));
        ast_node->contents.program.body = body;
        ast_node->contents.program.built_in_bindings = built_in_bindings;
        ast_node->contents.program.addon_bindings = addon_bindings;
    }

    if (ast_node->type == Call_Expression)
    {
        matched = true;
        unsigned char *name = va_arg(ap, unsigned char *);
        if (name == NULL)
        {
            ast_node->contents.call_expression.name = name;
        }
        else if (name != NULL)
        {
            ast_node->contents.call_expression.name = (unsigned char *)malloc(strlen((const char *)name) + 1);
            strcpy(TYPECAST(char *, ast_node->contents.call_expression.name), TYPECAST(const char *, name));
        }
        ast_node->contents.call_expression.anonymous_procedure = va_arg(ap, AST_Node *);
        Vector *params = va_arg(ap, Vector *);
        if (params == NULL) params = VectorNew(sizeof(AST_Node *));
        ast_node->contents.call_expression.params = params;
    }

    if (ast_node->type == Procedure)
    {
        matched = true;
        ast_node->contents.procedure.name = NULL;
        const unsigned char *name = va_arg(ap, const unsigned char *);
        if (name != NULL)
        {
            ast_node->contents.procedure.name = (unsigned char *)malloc(strlen((const char *)name) + 1);
            strcpy(TYPECAST(char *, ast_node->contents.procedure.name), TYPECAST(const char *, name));
        }
        ast_node->contents.procedure.required_params_count = va_arg(ap, size_t);
        ast_node->contents.procedure.params = va_arg(ap, Vector *);
        ast_node->contents.procedure.body_exprs = va_arg(ap, Vector *);
        ast_node->contents.procedure.c_native_function = va_arg(ap, Function);
    }

    if (ast_node->type == Lambda_Form)
    {
        matched = true;
        ast_node->contents.lambda_form.params = va_arg(ap, Vector *);
        ast_node->contents.lambda_form.body_exprs = va_arg(ap, Vector *);
    }

    if (ast_node->type == Local_Binding_Form)
    {
        Local_Binding_Form_Type local_binding_form_type = va_arg(ap, Local_Binding_Form_Type);
        ast_node->contents.local_binding_form.type = local_binding_form_type;

        if (local_binding_form_type == LET ||
            local_binding_form_type == LET_STAR ||
            local_binding_form_type == LETREC)
        {
            matched = true;
            Vector *bindings = va_arg(ap, Vector *);
            if (bindings == NULL) bindings = VectorNew(sizeof(AST_Node *));
            Vector *body_exprs = va_arg(ap, Vector *);
            if (body_exprs == NULL) body_exprs = VectorNew(sizeof(AST_Node *));
            ast_node->contents.local_binding_form.contents.lets.bindings = bindings;
            ast_node->contents.local_binding_form.contents.lets.body_exprs = body_exprs;
        }

        if (local_binding_form_type == DEFINE)
        {
            matched = true;
            const unsigned char *name = va_arg(ap, const unsigned char *);
            AST_Node *value = va_arg(ap, AST_Node *);
            ast_node->contents.local_binding_form.contents.define.binding = ast_node_new(ast_node->tag, Binding, name, value);
        }
    }

    if (ast_node->type == Set_Form)
    {
        matched = true;
        ast_node->contents.set_form.id = va_arg(ap, AST_Node *);
        ast_node->contents.set_form.expr = va_arg(ap, AST_Node *);
    }

    if (ast_node->type == Conditional_Form)
    {
        Conditional_Form_Type conditional_form_type = va_arg(ap, Conditional_Form_Type);
        ast_node->contents.conditional_form.type = conditional_form_type;

        if (conditional_form_type == IF)
        {
            matched = true;
            ast_node->contents.conditional_form.contents.if_expression.test_expr = va_arg(ap, AST_Node *);
            ast_node->contents.conditional_form.contents.if_expression.then_expr = va_arg(ap, AST_Node *);
            ast_node->contents.conditional_form.contents.if_expression.else_expr = va_arg(ap, AST_Node *);
        }

        if (conditional_form_type == COND)
        {
            matched = true;
            ast_node->contents.conditional_form.contents.cond_expression.cond_clauses = va_arg(ap, Vector *);
        }

        if (conditional_form_type == AND)
        {
            matched = true;
            ast_node->contents.conditional_form.contents.and_expression.exprs = va_arg(ap, Vector *);
        }

        if (conditional_form_type == NOT)
        {
            matched = true;
            ast_node->contents.conditional_form.contents.not_expression.expr = va_arg(ap, AST_Node *);
        }

        if (conditional_form_type == OR)
        {
            matched = true;
            ast_node->contents.conditional_form.contents.or_expression.exprs = va_arg(ap, Vector *);
        }
    }

    if (ast_node->type == Cond_Clause)
    {
        matched = true;
        ast_node->contents.cond_clause.type = va_arg(ap, Cond_Clause_Type);
        ast_node->contents.cond_clause.test_expr = va_arg(ap, AST_Node *);
        ast_node->contents.cond_clause.then_bodies = va_arg(ap, Vector *);
        ast_node->contents.cond_clause.proc_expr = va_arg(ap, AST_Node *);
    }

    if (ast_node->type == Binding)
    {
        matched = true;
        const unsigned char *name = va_arg(ap, const unsigned char *);
        ast_node->contents.binding.name = (unsigned char *)malloc(strlen((const char *)name) + 1);
        strcpy(TYPECAST(char *, ast_node->contents.binding.name), TYPECAST(const char *, name));
        ast_node->contents.binding.value = va_arg(ap, AST_Node *);
    }

    if (ast_node->type == List_Literal)
    {
        matched = true;
        Vector *value = va_arg(ap, Vector *);
        if (value == NULL) value = VectorNew(sizeof(AST_Node *));
        ast_node->contents.literal.value = value; 
        ast_node->contents.literal.c_native_value = NULL;
    }

    if (ast_node->type == Pair_Literal)
    {
        matched = true;
        Vector *value = va_arg(ap, Vector *);
        if (value == NULL) value = VectorNew(sizeof(AST_Node *));
        ast_node->contents.literal.value = value; 
        ast_node->contents.literal.c_native_value = NULL;
    }

    if (ast_node->type == Number_Literal)
    {
        matched = true;
        const unsigned char *value = va_arg(ap, const unsigned char *);
        ast_node->contents.literal.value = malloc(strlen((const char *)value) + 1);
        strcpy(TYPECAST(char *, ast_node->contents.literal.value), TYPECAST(const char *, value));
        // check '.' to decide use int or double
        if (strchr(ast_node->contents.literal.value, '.') == NULL)
        {
            // convert string to long long int
            long long int c_native_value = strtoll(ast_node->contents.literal.value, (char **)NULL, 10);
            ast_node->contents.literal.c_native_value = malloc(sizeof(long long int));
            memcpy(ast_node->contents.literal.c_native_value, &c_native_value, sizeof(long long int));
        }
        else
        {
            // convert string to double
            double c_native_value = strtod(ast_node->contents.literal.value, (char **)NULL);
            ast_node->contents.literal.c_native_value = malloc(sizeof(double));
            memcpy(ast_node->contents.literal.c_native_value, &c_native_value, sizeof(double));
        }
    }

    if (ast_node->type == String_Literal)
    {
        matched = true;
        const unsigned char *value = va_arg(ap, const unsigned char *);
        ast_node->contents.literal.value = malloc(strlen((const char *)value) + 1);
        strcpy(TYPECAST(char *, ast_node->contents.literal.value), TYPECAST(const char *, value));
        ast_node->contents.literal.c_native_value = NULL;
    }

    if (ast_node->type == Character_Literal)
    {
        matched = true;
        const unsigned char *character = va_arg(ap, const unsigned char *);
        ast_node->contents.literal.value = malloc(sizeof(unsigned char));
        memcpy(ast_node->contents.literal.value, character, sizeof(unsigned char));
        ast_node->contents.literal.c_native_value = NULL;
    }

    if (ast_node->type == Boolean_Literal)
    {
        matched = true;
        Boolean_Type *value = va_arg(ap, Boolean_Type *);
        ast_node->contents.literal.value = malloc(sizeof(Boolean_Type));
        memcpy(ast_node->contents.literal.value, value, sizeof(Boolean_Type));
        ast_node->contents.literal.c_native_value = NULL;
    }

    if (ast_node->type == NULL_Expression)
    {
        matched = true;
        ast_node->contents.null_expression.value = NULL;
    }

    if (ast_node->type == EMPTY_Expression)
    {
        matched = true;
        ast_node->contents.empty_expression.value = NULL;
    }

    va_end(ap);

    if (matched == false)
    {
        // when no matches any AST_Node_Type
        fprintf(stderr, "ast_node_new(): can not handle AST_Node_Type: %d\n", ast_node->type);
        exit(EXIT_FAILURE);
    }
  
    return ast_node;
}

int ast_node_free(AST_Node *ast_node)
{
    if (ast_node == NULL) return 1;

    bool matched = false;

    // free context itself only
    Vector *context = ast_node->context;
    if (context != NULL) VectorFree(ast_node->context, NULL, NULL);
    
    if (ast_node->type == Program)
    {
        matched = true;
        Vector *body = ast_node->contents.program.body;
        for (size_t i = 0; i < VectorLength(body); i++)
        {
            AST_Node *sub_node = *(AST_Node **)VectorNth(body, i);
            ast_node_free(sub_node);
        }
        VectorFree(body, NULL, NULL);

        Vector *built_in_bindings = ast_node->contents.program.built_in_bindings;
        for (size_t i = 0; i < VectorLength(built_in_bindings); i++)
        {
            AST_Node *binding = *(AST_Node **)VectorNth(built_in_bindings, i);
            ast_node_free(binding);
        }
        VectorFree(built_in_bindings, NULL, NULL);

        Vector *addon_bindings = ast_node->contents.program.addon_bindings;
        for (size_t i = 0; i < VectorLength(addon_bindings); i++)
        {
            AST_Node *binding = *(AST_Node **)VectorNth(addon_bindings, i);
            ast_node_free(binding);
        }
        VectorFree(addon_bindings, NULL, NULL);
    }

    if (ast_node->type == Call_Expression)
    {
        matched = true;
        Vector *params = ast_node->contents.call_expression.params;
        for (size_t i = 0; i < VectorLength(params); i++)
        {
            AST_Node *sub_node = *(AST_Node **)VectorNth(params, i);
            ast_node_free(sub_node);
        }
        VectorFree(params, NULL, NULL);
        free(ast_node->contents.call_expression.name);
    }

    if (ast_node->type == Procedure)
    {
        matched = true;
        unsigned char *name = ast_node->contents.procedure.name;
        if (name != NULL) free(ast_node->contents.procedure.name);
        Vector *params = ast_node->contents.procedure.params;
        if (params != NULL)
        {
            for (size_t i = 0; i < VectorLength(params); i++)
            {
                AST_Node *param = *(AST_Node **)VectorNth(params, i);
                ast_node_free(param);
            }
            VectorFree(params, NULL, NULL);
        }
        Vector *body_exprs = ast_node->contents.procedure.body_exprs;
        if (body_exprs != NULL)
        {
            for (size_t i = 0; i < VectorLength(body_exprs); i++)
            {
                AST_Node *body_expr = *(AST_Node **)VectorNth(body_exprs, i);
                ast_node_free(body_expr);
            }
            VectorFree(body_exprs, NULL, NULL);
        }
    }

    if (ast_node->type == Lambda_Form)
    {
        matched = true;

        Vector *params = ast_node->contents.lambda_form.params;
        Vector *body_exprs = ast_node->contents.lambda_form.body_exprs;

        for (size_t i = 0; i < VectorLength(params); i++)
        {
            AST_Node *param = *(AST_Node **)VectorNth(params, i);
            ast_node_free(param);
        }
        VectorFree(params, NULL, NULL);

        for (size_t i = 0; i < VectorLength(body_exprs); i++)
        {
            AST_Node *body_expr = *(AST_Node **)VectorNth(body_exprs, i);
            ast_node_free(body_expr);
        }
        VectorFree(body_exprs, NULL, NULL);
    }

    if (ast_node->type == Local_Binding_Form)
    {
        Local_Binding_Form_Type Local_binding_form_type = ast_node->contents.local_binding_form.type;

        if (Local_binding_form_type == LET ||
            Local_binding_form_type == LET_STAR ||
            Local_binding_form_type == LETREC)
        {
            matched = true;
            Vector *bindings = ast_node->contents.local_binding_form.contents.lets.bindings;
            Vector *body_exprs = ast_node->contents.local_binding_form.contents.lets.body_exprs;
            for (size_t i = 0; i < VectorLength(bindings); i++)
            {
                AST_Node *binding = *(AST_Node **)VectorNth(bindings, i);
                ast_node_free(binding);
            }
            for (size_t i = 0; i < VectorLength(body_exprs); i++)
            {
                AST_Node *body_expr = *(AST_Node **)VectorNth(body_exprs, i);
                ast_node_free(body_expr);
            }
            VectorFree(bindings, NULL, NULL);
            VectorFree(body_exprs, NULL, NULL);
        }
       
        if (Local_binding_form_type == DEFINE) 
        {
            matched = true;
            ast_node_free(ast_node->contents.local_binding_form.contents.define.binding);
        }
    }

    if (ast_node->type == Set_Form)
    {
        matched = true;
        AST_Node *id = ast_node->contents.set_form.id;
        AST_Node *expr = ast_node->contents.set_form.expr;
        ast_node_free(id);
        ast_node_free(expr);
    }

    if (ast_node->type == Conditional_Form)
    {
        Conditional_Form_Type conditional_form_type = ast_node->contents.conditional_form.type;

        if (conditional_form_type == IF)
        {
            matched = true;
            AST_Node *test_expr = ast_node->contents.conditional_form.contents.if_expression.test_expr;
            AST_Node *then_expr = ast_node->contents.conditional_form.contents.if_expression.then_expr;
            AST_Node *else_expr = ast_node->contents.conditional_form.contents.if_expression.else_expr;
            ast_node_free(test_expr);
            ast_node_free(then_expr);
            ast_node_free(else_expr);
        }

        if (conditional_form_type == COND)
        {
            matched = true;
            Vector *cond_clauses = ast_node->contents.conditional_form.contents.cond_expression.cond_clauses;

            for (size_t i = 0; i < VectorLength(cond_clauses); i++)
            {
                // free each cond_clause
                AST_Node *cond_clause = *(AST_Node **)VectorNth(cond_clauses, i);
                ast_node_free(cond_clause);
            }

            VectorFree(cond_clauses, NULL, NULL);
        }

        if (conditional_form_type == AND)
        {
            matched = true;
            Vector *exprs = ast_node->contents.conditional_form.contents.and_expression.exprs;

            for (size_t i = 0; i < VectorLength(exprs); i++)
            {
                AST_Node *expr = *(AST_Node **)VectorNth(exprs, i);
                ast_node_free(expr);
            }

            VectorFree(exprs, NULL, NULL);
        }

        if (conditional_form_type == NOT)
        {
            matched = true;
            AST_Node *expr = ast_node->contents.conditional_form.contents.not_expression.expr;
            ast_node_free(expr);
        }

        if (conditional_form_type == OR)
        {
            matched = true;
            Vector *exprs = ast_node->contents.conditional_form.contents.or_expression.exprs;

            for (size_t i = 0; i < VectorLength(exprs); i++)
            {
                AST_Node *expr = *(AST_Node **)VectorNth(exprs, i);
                ast_node_free(expr);
            }

            VectorFree(exprs, NULL, NULL);
        }

        // ...
    }

    if (ast_node->type == Cond_Clause)
    {
        Cond_Clause_Type cond_clause_type = ast_node->contents.cond_clause.type;

        if (cond_clause_type == TEST_EXPR_WITH_THENBODY)
        {
            matched = true;
            // [test-expr then-body ...+]
            // needs to free test_expr and then_bodies
            AST_Node *test_expr = ast_node->contents.cond_clause.test_expr;
            ast_node_free(test_expr);

            Vector *then_bodies = ast_node->contents.cond_clause.then_bodies;
            for (size_t i = 0; i < VectorLength(then_bodies); i++)
            {
                AST_Node *then_body = *(AST_Node **)VectorNth(then_bodies, i);
                ast_node_free(then_body);
            }
            VectorFree(then_bodies, NULL, NULL);
        }
        else if (cond_clause_type == ELSE_STATEMENT)
        {
            matched = true;
            // [else then-body ...+]
            // needs to free then_bodies
            Vector *then_bodies = ast_node->contents.cond_clause.then_bodies;
            for (size_t i = 0; i < VectorLength(then_bodies); i++)
            {
                AST_Node *then_body = *(AST_Node **)VectorNth(then_bodies, i);
                ast_node_free(then_body);
            }
            VectorFree(then_bodies, NULL, NULL);
        }
        else if (cond_clause_type == TEST_EXPR_WITH_PROC)
        {
        }
        else if (cond_clause_type == SINGLE_TEST_EXPR)
        {
        }
        else
        {
            // something wrong here
            fprintf(stderr, "ast_node_free(): can not handle cond_clause_type: %d\n", cond_clause_type);
            exit(EXIT_FAILURE);
        }
    }

    if (ast_node->type == Binding)
    {
        matched = true;
        unsigned char *name = ast_node->contents.binding.name;
        if (name != NULL) free(name);
        AST_Node *value = ast_node->contents.binding.value;
        if (value != NULL) ast_node_free(value);
    }

    if (ast_node->type == List_Literal)
    {
        matched = true;
        Vector *elements = TYPECAST(Vector *, ast_node->contents.literal.value);
        for (size_t i = 0; i < VectorLength(elements); i++)
        {
            AST_Node *element = *(AST_Node **)VectorNth(elements, i);
            ast_node_free(element);
        }
        VectorFree(elements, NULL, NULL);
    }

    if (ast_node->type == Pair_Literal)
    {
        matched = true;
        Vector *elements = TYPECAST(Vector *, ast_node->contents.literal.value);
        for (size_t i = 0; i < VectorLength(elements); i++)
        {
            AST_Node *element = *(AST_Node **)VectorNth(elements, i);
            ast_node_free(element);
        }
        VectorFree(elements, NULL, NULL);
    }

    if (ast_node->type == Number_Literal)
    {
        matched = true;
        free(ast_node->contents.literal.value);
        free(ast_node->contents.literal.c_native_value);
    }

    if (ast_node->type == String_Literal)
    {
        matched = true;
        free(ast_node->contents.literal.value);
    }

    if (ast_node->type == Character_Literal)
    {
        matched = true;
        free(ast_node->contents.literal.value);
    }

    if (ast_node->type == Boolean_Literal)
    {
        matched = true;
        free(ast_node->contents.literal.value);
    }

    if (ast_node->type == NULL_Expression)
    {
        matched = true;
        if (ast_node->contents.null_expression.value != NULL)
            ast_node_free(ast_node->contents.null_expression.value);
    }

    if (ast_node->type == EMPTY_Expression)
    {
        matched = true;
        if (ast_node->contents.empty_expression.value != NULL)
            ast_node_free(ast_node->contents.empty_expression.value);
    }

    if (matched == false)
    {
        // when no matches any AST_Node_Type
        fprintf(stderr, "ast_node_free(): can not handle AST_Node_Type: %d\n", ast_node->type);
        exit(EXIT_FAILURE);
    }

    // free ast_node itself
    free(ast_node);

    return 0;
}

AST_Node *ast_node_deep_copy(AST_Node *ast_node, void *aux_data)
{
    /*
        AST_Node::parent: can not copy, you should use generate_context() or xxx->parent = yyy to set parent of an ast copy
        AST_Node::context: can not copy, you should use generate_context() to generate context of an ast copy
        AST_Node::tag: copy tag
    **/

    if (ast_node == NULL)
    {
        fprintf(stderr, "ast_node_deep_copy(): can not copy NULL\n");
        exit(EXIT_FAILURE); 
    }

    AST_Node *copy = NULL;
    bool matched = false;

    if (ast_node->type == Number_Literal)
    {
        matched = true;
        copy = ast_node_new(ast_node->tag, Number_Literal, ast_node->contents.literal.value);
    }

    if (ast_node->type == String_Literal)
    {
        matched = true;
        copy = ast_node_new(ast_node->tag, String_Literal, ast_node->contents.literal.value);
    }

    if (ast_node->type == Character_Literal)
    {
        matched = true;
        copy = ast_node_new(ast_node->tag, Character_Literal, ast_node->contents.literal.value);
    }

    if (ast_node->type == List_Literal)
    {
        matched = true;
        Vector *value = ast_node->contents.literal.value;
        Vector *value_copy = VectorNew(sizeof(AST_Node *));

        for (size_t i = 0; i < VectorLength(value); i++)
        {
            AST_Node *node = *(AST_Node **)VectorNth(value, i);
            AST_Node *node_copy = ast_node_deep_copy(node, aux_data);
            VectorAppend(value_copy, &node_copy);
        }

        copy = ast_node_new(ast_node->tag, List_Literal, value_copy);
    }

    if (ast_node->type == Pair_Literal)
    {
        matched = true;
        Vector *value = ast_node->contents.literal.value;
        Vector *value_copy = VectorNew(sizeof(AST_Node *));

        for (size_t i = 0; i < VectorLength(value); i++)
        {
            AST_Node *node = *(AST_Node **)VectorNth(value, i);
            AST_Node *node_copy = ast_node_deep_copy(node, aux_data);
            VectorAppend(value_copy, &node_copy);
        }

        copy = ast_node_new(ast_node->tag, Pair_Literal, value_copy);
    }

    if (ast_node->type == Boolean_Literal)
    {
        matched = true;
        copy = ast_node_new(ast_node->tag, Boolean_Literal, ast_node->contents.literal.value);
    }

    if (ast_node->type == NULL_Expression)
    {
        matched = true;
        copy = ast_node_new(ast_node->tag, NULL_Expression);
        AST_Node *value = ast_node->contents.null_expression.value;
        if (value != NULL)
            copy->contents.null_expression.value = ast_node_deep_copy(value, aux_data);
    }

    if (ast_node->type == EMPTY_Expression)
    {
        matched = true;
        copy = ast_node_new(ast_node->tag, EMPTY_Expression);
        AST_Node *value = ast_node->contents.empty_expression.value;
        if (value != NULL)
            copy->contents.empty_expression.value = ast_node_deep_copy(value, aux_data);
    }
    
    if (ast_node->type == Local_Binding_Form)
    {
        Local_Binding_Form_Type local_binding_form_type = ast_node->contents.local_binding_form.type;

        if (local_binding_form_type == DEFINE)
        {
            matched = true;
            AST_Node *binding = ast_node->contents.local_binding_form.contents.define.binding;
            unsigned char *name = binding->contents.binding.name;
            AST_Node *value = binding->contents.binding.value;

            AST_Node *value_copy = ast_node_deep_copy(value, aux_data);
            copy = ast_node_new(ast_node->tag, Local_Binding_Form, DEFINE, name, value_copy);
        }

        if (local_binding_form_type == LET ||
            local_binding_form_type == LET_STAR ||
            local_binding_form_type == LETREC)
        {
            matched = true;
            Vector *bindings = ast_node->contents.local_binding_form.contents.lets.bindings;
            Vector *body_exprs = ast_node->contents.local_binding_form.contents.lets.body_exprs;

            Vector *bindings_copy = VectorNew(sizeof(AST_Node *));
            Vector *body_exprs_copy = VectorNew(sizeof(AST_Node *));

            for (size_t i = 0; i < VectorLength(bindings); i++)
            {
                AST_Node *node = *(AST_Node **)VectorNth(bindings, i);
                AST_Node *node_copy = ast_node_deep_copy(node, aux_data);
                VectorAppend(bindings_copy, &node_copy);
            }

            for (size_t i = 0; i < VectorLength(body_exprs); i++)
            {
                AST_Node *node = *(AST_Node **)VectorNth(body_exprs, i);
                AST_Node *node_copy = ast_node_deep_copy(node, aux_data);
                VectorAppend(body_exprs_copy, &node_copy);
            }

            if (local_binding_form_type == LET)
                copy = ast_node_new(ast_node->tag, Local_Binding_Form, LET, bindings_copy, body_exprs_copy);
            if (local_binding_form_type == LET_STAR)
                copy = ast_node_new(ast_node->tag, Local_Binding_Form, LET_STAR, bindings_copy, body_exprs_copy);
            if (local_binding_form_type == LETREC)
                copy = ast_node_new(ast_node->tag, Local_Binding_Form, LETREC, bindings_copy, body_exprs_copy);
        }
    }

    if (ast_node->type == Set_Form)
    {
        matched = true;
        AST_Node *id = ast_node->contents.set_form.id;
        AST_Node *expr = ast_node->contents.set_form.expr;

        AST_Node *id_copy = ast_node_deep_copy(id, aux_data);
        AST_Node *expr_copy = ast_node_deep_copy(expr, aux_data);

        copy = ast_node_new(ast_node->tag, Set_Form, id_copy, expr_copy);
    }

    if (ast_node->type == Conditional_Form)
    {
        Conditional_Form_Type conditional_form_type = ast_node->contents.conditional_form.type;

        if (conditional_form_type == IF)
        {
            matched = true;
            AST_Node *test_expr = ast_node->contents.conditional_form.contents.if_expression.test_expr;
            AST_Node *then_expr = ast_node->contents.conditional_form.contents.if_expression.then_expr;
            AST_Node *else_expr = ast_node->contents.conditional_form.contents.if_expression.else_expr; 

            AST_Node *test_expr_copy = ast_node_deep_copy(test_expr, aux_data);
            AST_Node *then_expr_copy = ast_node_deep_copy(then_expr, aux_data);
            AST_Node *else_expr_copy = ast_node_deep_copy(else_expr, aux_data);

            copy = ast_node_new(ast_node->tag, Conditional_Form, IF, test_expr_copy, then_expr_copy, else_expr_copy);
        }

        if (conditional_form_type == COND)
        {
            matched = true;
            Vector *cond_clauses = ast_node->contents.conditional_form.contents.cond_expression.cond_clauses;
            Vector *cond_clauses_copy = VectorNew(sizeof(AST_Node *));

            for (size_t i = 0; i < VectorLength(cond_clauses); i++)
            {
                AST_Node *cond_clause = *(AST_Node **)VectorNth(cond_clauses, i);
                AST_Node *cond_clause_copy = ast_node_deep_copy(cond_clause, aux_data);
                VectorAppend(cond_clauses_copy, &cond_clause_copy);
            }

            copy = ast_node_new(ast_node->tag, Conditional_Form, COND, cond_clauses_copy);
        }

        if (conditional_form_type == AND)
        {
            matched = true;
            Vector *exprs = ast_node->contents.conditional_form.contents.and_expression.exprs;
            Vector *exprs_copy = VectorNew(sizeof(AST_Node *));

            for (size_t i = 0; i < VectorLength(exprs); i++)
            {
                AST_Node *node = *(AST_Node **)VectorNth(exprs, i);
                AST_Node *node_copy = ast_node_deep_copy(node, aux_data);
                VectorAppend(exprs_copy, &node_copy);
            }

            copy = ast_node_new(ast_node->tag, Conditional_Form, AND, exprs_copy);
        }

        if (conditional_form_type == NOT)
        {
            matched = true;
            AST_Node *expr = ast_node->contents.conditional_form.contents.not_expression.expr;
            AST_Node *expr_copy = ast_node_deep_copy(expr, aux_data);
            copy = ast_node_new(ast_node->tag, Conditional_Form, NOT, expr_copy);
        }

        if (conditional_form_type == OR)
        {
            matched = true;
            Vector *exprs = ast_node->contents.conditional_form.contents.or_expression.exprs;
            Vector *exprs_copy = VectorNew(sizeof(AST_Node *));

            for (size_t i = 0; i < VectorLength(exprs); i++)
            {
                AST_Node *node = *(AST_Node **)VectorNth(exprs, i);
                AST_Node *node_copy = ast_node_deep_copy(node, aux_data);
                VectorAppend(exprs_copy, &node_copy);
            }

            copy = ast_node_new(ast_node->tag, Conditional_Form, OR, exprs_copy);
        }
    }

    if (ast_node->type == Cond_Clause)
    {
        matched = true;
        
        if (ast_node->contents.cond_clause.type == TEST_EXPR_WITH_THENBODY)
        {
            // [test-expr then-body ...+]
            AST_Node *test_expr = ast_node->contents.cond_clause.test_expr;
            AST_Node *test_expr_copy = ast_node_deep_copy(test_expr, aux_data);

            Vector *then_bodies = ast_node->contents.cond_clause.then_bodies;
            Vector *then_bodies_copy = VectorNew(sizeof(AST_Node *));
            
            for (size_t i = 0; i < VectorLength(then_bodies); i++)
            {
                AST_Node *then_body = *(AST_Node **)VectorNth(then_bodies, i);
                AST_Node *then_body_copy = ast_node_deep_copy(then_body, aux_data);
                VectorAppend(then_bodies_copy, &then_body_copy);
            }

            copy = ast_node_new(ast_node->tag, Cond_Clause, TEST_EXPR_WITH_THENBODY, test_expr_copy, then_bodies_copy, NULL);
        }
        else if (ast_node->contents.cond_clause.type == ELSE_STATEMENT)
        {
            // [else then-body ...+]
            Vector *then_bodies = ast_node->contents.cond_clause.then_bodies;
            Vector *then_bodies_copy = VectorNew(sizeof(AST_Node *));
            
            for (size_t i = 0; i < VectorLength(then_bodies); i++)
            {
                AST_Node *then_body = *(AST_Node **)VectorNth(then_bodies, i);
                AST_Node *then_body_copy = ast_node_deep_copy(then_body, aux_data);
                VectorAppend(then_bodies_copy, &then_body_copy);
            }

            copy = ast_node_new(ast_node->tag, Cond_Clause, ELSE_STATEMENT, NULL, then_bodies_copy, NULL);
        }
        else if (ast_node->contents.cond_clause.type == TEST_EXPR_WITH_PROC)
        {
        }
        else if (ast_node->contents.cond_clause.type == SINGLE_TEST_EXPR)
        {
        }
        else
        {
            // something wrong here
            fprintf(stderr, "ast_node_deep_copy(): can not copy Cond_Clause_Type: %d\n", ast_node->contents.cond_clause.type);
            exit(EXIT_FAILURE); 
        }
    }
    
    if (ast_node->type == Lambda_Form)
    {
        matched = true;
        Vector *params = ast_node->contents.lambda_form.params;
        Vector *body_exprs = ast_node->contents.lambda_form.body_exprs; 

        Vector *params_copy = VectorNew(sizeof(AST_Node *));
        Vector *body_exprs_copy = VectorNew(sizeof(AST_Node *));

        for (size_t i = 0; i < VectorLength(params); i++)
        {
            AST_Node *node = *(AST_Node **)VectorNth(params, i);
            AST_Node *node_copy = ast_node_deep_copy(node, aux_data);
            VectorAppend(params_copy, &node_copy);
        }

        for (size_t i = 0; i < VectorLength(body_exprs); i++)
        {
            AST_Node *node = *(AST_Node **)VectorNth(body_exprs, i);
            AST_Node *node_copy = ast_node_deep_copy(node, aux_data);
            VectorAppend(body_exprs_copy, &node_copy);
        }

        copy = ast_node_new(ast_node->tag, Lambda_Form, params_copy, body_exprs_copy);
    }

    if (ast_node->type == Call_Expression)
    {
        matched = true;
        const unsigned char *name = ast_node->contents.call_expression.name;
        AST_Node *anonymous_procedure = ast_node->contents.call_expression.anonymous_procedure;
        Vector *params = ast_node->contents.call_expression.params;

        Vector *params_copy = VectorNew(sizeof(AST_Node *));

        for (size_t i = 0; i < VectorLength(params); i++)
        {
            AST_Node *node = *(AST_Node **)VectorNth(params, i);
            AST_Node *node_copy = ast_node_deep_copy(node, aux_data);
            VectorAppend(params_copy, &node_copy);
        }

        copy = ast_node_new(ast_node->tag, Call_Expression, name, anonymous_procedure, params_copy);
    }

    if (ast_node->type == Binding)
    {
        matched = true;
        unsigned char *name = ast_node->contents.binding.name;
        AST_Node *value = ast_node->contents.binding.value;
        AST_Node *value_copy = NULL;
        if (value != NULL) value_copy = ast_node_deep_copy(value, aux_data);

        copy = ast_node_new(ast_node->tag, Binding, name, value_copy);
    }

    if (ast_node->type == Procedure)
    {
        matched = true;
        unsigned char *name = ast_node->contents.procedure.name;
        int required_params_count = ast_node->contents.procedure.required_params_count; 
        Vector *params = ast_node->contents.procedure.params; 
        Vector *body_exprs = ast_node->contents.procedure.body_exprs; 
        Function c_native_function = ast_node->contents.procedure.c_native_function;

        if (params == NULL && body_exprs == NULL && c_native_function != NULL)
        {
            // built-in or addon procedure
            copy = ast_node_new(ast_node->tag, Procedure, name, required_params_count, NULL, NULL, c_native_function);
        }
        else if (params != NULL && body_exprs != NULL && c_native_function == NULL)
        {
            // user defined procedure
            Vector *params_copy = VectorNew(sizeof(AST_Node *));
            Vector *body_exprs_copy = VectorNew(sizeof(AST_Node *));
            
            for (size_t i = 0; i < VectorLength(params); i++)
            {
                AST_Node *node = *(AST_Node **)VectorNth(params, i);
                AST_Node *node_copy = ast_node_deep_copy(node, aux_data);
                VectorAppend(params_copy, &node_copy);
            }
    
            for (size_t i = 0; i < VectorLength(body_exprs); i++)
            {
                AST_Node *node = *(AST_Node **)VectorNth(body_exprs, i);
                AST_Node *node_copy = ast_node_deep_copy(node, aux_data);
                VectorAppend(body_exprs_copy, &node_copy);
            }
    
            copy = ast_node_new(ast_node->tag, Procedure, name, required_params_count, params_copy, body_exprs_copy, NULL);
        }
        else
        {
            // something wrong here
            fprintf(stderr, "ast_node_deep_copy(): can not copy procedure\n");
            exit(EXIT_FAILURE); 
        }
    }

    if (ast_node->type == Program)
    {
        matched = true;
        Vector *body = ast_node->contents.program.body;
        Vector *built_in_bindings = ast_node->contents.program.built_in_bindings;
        Vector *addon_bindings = ast_node->contents.program.addon_bindings;

        Vector *body_copy = VectorNew(sizeof(AST_Node *));
        Vector *built_in_bindings_copy = VectorNew(sizeof(AST_Node *));
        Vector *addon_bindings_copy = VectorNew(sizeof(AST_Node *));

        for (size_t i = 0; i < VectorLength(body); i++)
        {
            AST_Node *node = *(AST_Node **)VectorNth(body, i);
            AST_Node *node_copy = ast_node_deep_copy(node, aux_data);
            VectorAppend(body_copy, &node_copy);
        }

        for (size_t i = 0; i < VectorLength(built_in_bindings); i++)
        {
            AST_Node *node = *(AST_Node **)VectorNth(built_in_bindings, i);
            VectorAppend(built_in_bindings_copy, &node);
        }

        for (size_t i = 0; i < VectorLength(addon_bindings); i++)
        {
            AST_Node *node = *(AST_Node **)VectorNth(addon_bindings, i);
            VectorAppend(addon_bindings_copy, &node);
        }

        copy = ast_node_new(ast_node->tag, Program, body_copy, built_in_bindings_copy, addon_bindings_copy);
    }

    if (matched == false)
    {
        // when no matches any AST_Node_Type
        fprintf(stderr, "ast_node_deep_copy(): can not copy AST_Node_Type: %d\n", ast_node->type);
        exit(EXIT_FAILURE);
    }

    // copy AST_Node::tag
    copy->tag = ast_node->tag;
    
    return copy;
}

void ast_node_set_tag(AST_Node *ast_node, AST_Node_Tag tag)
{
    if (ast_node == NULL)
    {
        fprintf(stderr, "ast_node_set_tag(): ast_node is NULL\n");
        exit(EXIT_FAILURE);
    }

    ast_node->tag = tag;
}

void ast_node_set_tag_recursive(AST_Node *ast_node, AST_Node_Tag tag)
{
    ast_node_set_tag(ast_node, tag);
    Visitor visitor = visitor_new();

    // generate handler for all type
    for (AST_Node_Type type = Number_Literal; type != LAST; type++)
    {
        AST_Node_Handler *handler = ast_node_handler_new(type, set_tag_rec_visitor_helper, NULL);
        ast_node_handler_append(visitor, handler);
    }

    traverser(ast_node, visitor, (void *)(&tag));
    visitor_free(visitor);
}

AST_Node_Tag ast_node_get_tag(AST_Node *ast_node)
{
    if (ast_node == NULL)
    {
        fprintf(stderr, "ast_node_get_tag(): ast_node is NULL\n");
        exit(EXIT_FAILURE);
    }

    return ast_node->tag;
}

AST parser(Tokens *tokens)
{
    AST ast = ast_node_new(IN_AST, Program, NULL, NULL, NULL);
    size_t current = 0;

    while (current < tokens_length(tokens))
    // the value of 'current' was changed in walk() by current_p
    {
        AST_Node *ast_node = walk(tokens, &current);
        if (ast_node != NULL) VectorAppend(ast->contents.program.body, &ast_node);
        else continue;
    }
    
    return ast;
}

int ast_free(AST ast)
{
    return ast_node_free(ast);
}

Visitor visitor_new()
{
    return VectorNew(sizeof(AST_Node_Handler *));
}

int visitor_free(Visitor visitor)
{
    if (visitor == NULL) return 1;
    VectorFree(visitor, visitor_free_helper, NULL);
    return 0;
}

AST_Node_Handler *ast_node_handler_new(AST_Node_Type type, VisitorFunction enter, VisitorFunction exit)
{
    AST_Node_Handler *handler = (AST_Node_Handler *)malloc(sizeof(AST_Node_Handler));
    handler->type = type;
    handler->enter = enter;
    handler->exit = exit;
    return handler;
}

int ast_node_handler_free(AST_Node_Handler *handler)
{
    free(handler);
    return 0;
}

int ast_node_handler_append(Visitor visitor, AST_Node_Handler *handler)
{
    VectorAppend(visitor, &handler);
    return 0;
}

AST_Node_Handler *find_ast_node_handler(Visitor visitor, AST_Node_Type type)
{
    for (size_t i = 0; i < VectorLength(visitor); i++)
    {
        AST_Node_Handler *handler = *(AST_Node_Handler **)VectorNth(visitor, i);
        if (handler->type == type) return handler;
    }
    return NULL;
}

Visitor get_defult_visitor(void)
{
    return NULL;
}

// left-sub-tree-first dfs algo
void traverser(AST ast, Visitor visitor, void *aux_data)
{
    if (visitor == NULL) visitor = get_defult_visitor();
    traverser_helper(ast, NULL, visitor, aux_data);
}

// recursion function <walk> walk over the tokens array, and generates a ast
static AST_Node *walk(Tokens *tokens, size_t *current_p)
{
    Token *token = tokens_nth(tokens, *current_p);

    if (token->type == LANGUAGE)
    {
        (*current_p)++;
        return NULL;
    } 

    if (token->type == COMMENT)
    {
        (*current_p)++;
        return NULL;
    }

    if (token->type == IDENTIFIER)
    {
        // handle null
        if (strcmp(TYPECAST(const char *, token->value), "null") == 0)
        {
            AST_Node *null_expr = ast_node_new(IN_AST, NULL_Expression);
            (*current_p)++; // skip null itself
            return null_expr;
        }

        // handle empty
        if (strcmp(TYPECAST(const char *, token->value), "empty") == 0)
        {
            AST_Node *empty_expr = ast_node_new(IN_AST, EMPTY_Expression);
            (*current_p)++; // skip empty itself
            return empty_expr;
        }

        // handle normally identifier 
        AST_Node *ast_node = ast_node_new(IN_AST, Binding, token->value, NULL);
        (*current_p)++;
        return ast_node;
    }

    if (token->type == NUMBER)
    {
        AST_Node *ast_node = ast_node_new(IN_AST, Number_Literal, token->value);
        (*current_p)++;
        return ast_node;
    }

    if (token->type == STRING)
    {
        AST_Node *ast_node = ast_node_new(IN_AST, String_Literal, token->value);
        (*current_p)++;
        return ast_node;
    }

    if (token->type == CHARACTER)
    {
        AST_Node *ast_node = ast_node_new(IN_AST, Character_Literal, token->value);
        (*current_p)++;
        return ast_node;
    }

    if (token->type == BOOLEAN)
    {
        Boolean_Type *boolean_type = malloc(sizeof(Boolean_Type));
        if (strchr(TYPECAST(const char *, token->value), 't') != NULL)
        {
            *boolean_type = R_TRUE;
        }
        if (strchr(TYPECAST(const char *, token->value), 'f') != NULL)
        {
            *boolean_type = R_FALSE;
        }
        AST_Node *ast_node = ast_node_new(IN_AST, Boolean_Literal, boolean_type);
        free(boolean_type);
        (*current_p)++;
        return ast_node;
    }

    if (token->type == PUNCTUATION)
    {
        unsigned char token_value = (token->value)[0];

        // '(' and ')' normally function call or each kind of form such as let let* if cond etc
        if (token_value == LEFT_PAREN)
        {
            // point to the function's name
            (*current_p)++;
            token = tokens_nth(tokens, *current_p); 

            // handle Local_Binding_Form
            // handle 'let' 'let*' 'letrec' contains '[' and ']'
            if ((strcmp(TYPECAST(const char *, token->value), "let") == 0) ||
                (strcmp(TYPECAST(const char *, token->value), "let*") == 0) ||
                (strcmp(TYPECAST(const char *, token->value), "letrec") == 0))
            {
                Token *name_token = token;
                Vector *bindings = VectorNew(sizeof(AST_Node *));
                Vector *body_exprs = VectorNew(sizeof(AST_Node *));

                // point to the bindings form starts '('
                (*current_p)++;
                token = tokens_nth(tokens, *current_p);
                if ((token->type != PUNCTUATION) ||
                    (token->type == PUNCTUATION && token->value[0] != LEFT_PAREN))
                {
                    fprintf(stderr, "walk(): let expression here, check the syntax\n");
                    exit(EXIT_FAILURE);
                }

                // move to first binding. '['
                (*current_p)++; 
                token = tokens_nth(tokens, *current_p);

                // collect bindings
                while ((token->type != PUNCTUATION) ||
                       (token->type == PUNCTUATION && (token->value)[0] != RIGHT_PAREN)) 
                {
                    // check '['
                    if ((token->type != PUNCTUATION) ||
                         (token->type == PUNCTUATION && token->value[0] != LEFT_SQUARE_BRACKET))
                    {
                        fprintf(stderr, "walk(): let expression here, check the syntax\n");
                        exit(EXIT_FAILURE);
                    }

                    // move to binding's name
                    (*current_p)++; 

                    AST_Node *binding = walk(tokens, current_p); 
                    VectorAppend(bindings, &binding);
                     
                    AST_Node *value = walk(tokens, current_p);
                    binding->contents.binding.value = value;

                    // check ']'
                    token = tokens_nth(tokens, *current_p);
                    if ((token->type != PUNCTUATION) ||
                        (token->type == PUNCTUATION && token->value[0] != RIGHT_SQUARE_BRACKET))
                    {
                        fprintf(stderr, "walk(): let expression here, check the syntax\n");
                        exit(EXIT_FAILURE);
                    }

                    // move to next '[' or ')' that completeing the binding form
                    (*current_p)++;
                    token = tokens_nth(tokens, *current_p);
                }

                /*
                    collect body_exprs
                */
                // move to next token
                (*current_p)++;
                token = tokens_nth(tokens, *current_p);
                
                while ((token->type != PUNCTUATION) ||
                       (token->type == PUNCTUATION && (token->value)[0] != RIGHT_PAREN)) 
                {
                    AST_Node *body_expr = walk(tokens, current_p);
                    VectorAppend(body_exprs, &body_expr);
                    token = tokens_nth(tokens, *current_p);
                }

                AST_Node *ast_node = NULL;
                if (strcmp(TYPECAST(const char *, name_token->value), "let") == 0) ast_node = ast_node_new(IN_AST, Local_Binding_Form, LET, bindings, body_exprs);
                if (strcmp(TYPECAST(const char *, name_token->value), "let*") == 0) ast_node = ast_node_new(IN_AST, Local_Binding_Form, LET_STAR, bindings, body_exprs);
                if (strcmp(TYPECAST(const char *, name_token->value), "letrec") == 0) ast_node = ast_node_new(IN_AST, Local_Binding_Form, LETREC, bindings, body_exprs); 
                (*current_p)++; // skip ')' of let expression
                return ast_node;
            }

            // handle 'define' 
            if (strcmp(TYPECAST(const char *, token->value), "define") == 0)
            {
                // move to binding's name
                (*current_p)++;
                token = tokens_nth(tokens, *current_p);

                // check token's type if or not identifier
                if (token->type != IDENTIFIER)
                {
                    fprintf(stderr, "walk(): plz: (define xxx xxx), check the syntax\n");
                    exit(EXIT_FAILURE);
                }

                // move to binding's value
                (*current_p)++;
                AST_Node *value = walk(tokens, current_p);

                AST_Node *ast_node = ast_node_new(IN_AST, Local_Binding_Form, DEFINE, token->value, value);
                (*current_p)++; // skip ')' of let expression
                return ast_node;
            }

            // handle 'lambda'
            if (strcmp(TYPECAST(const char *, token->value), "lambda") == 0)
            {
                Vector *params = VectorNew(sizeof(AST_Node *));
                Vector *body_exprs = VectorNew(sizeof(AST_Node *));

                // arguments list
                // move to '('    
                (*current_p)++;
                token = tokens_nth(tokens, *current_p);
                if ((token->value)[0] != LEFT_PAREN)
                {
                    fprintf(stderr, "walk(): lambda: bad syntax\n");
                    exit(EXIT_FAILURE);
                }

                // move to first arg of argument-list 
                (*current_p)++;
                token = tokens_nth(tokens, *current_p);

                // collect arguments
                while ((token->type != PUNCTUATION) ||
                       (token->type == PUNCTUATION && (token->value)[0] != RIGHT_PAREN)) 
                {
                    AST_Node *param = walk(tokens, current_p);
                    VectorAppend(params, &param);
                    token = tokens_nth(tokens, *current_p);
                }

                // check ')' of argument-list
                if ((token->value)[0] != RIGHT_PAREN)
                {
                    fprintf(stderr, "walk(): lambda: bad syntax\n");
                    exit(EXIT_FAILURE);
                }

                // move to first body_expr
                (*current_p)++;
                token = tokens_nth(tokens, *current_p);

                // collect body expressions
                while ((token->type != PUNCTUATION) ||
                       (token->type == PUNCTUATION && (token->value)[0] != RIGHT_PAREN)) 
                {
                    AST_Node *body_expr = walk(tokens, current_p);
                    VectorAppend(body_exprs, &body_expr);
                    token = tokens_nth(tokens, *current_p);
                }

                // check ')' of body_exprs
                if ((token->value)[0] != RIGHT_PAREN)
                {
                    fprintf(stderr, "walk(): lambda: bad syntax\n");
                    exit(EXIT_FAILURE);
                }

                AST_Node *lambda = ast_node_new(IN_AST, Lambda_Form, params, body_exprs);
                (*current_p)++; // skip ')' of lambda expression 
                return lambda;
            }

            // handle 'if'
            if (strcmp(TYPECAST(const char *, token->value), "if") == 0)
            {
                // move to test_expr
                (*current_p)++;

                AST_Node *test_expr = walk(tokens, current_p);
                AST_Node *then_expr = walk(tokens, current_p);
                AST_Node *else_expr = walk(tokens, current_p);

                // check ')'
                token = tokens_nth(tokens, *current_p);
                if ((token->value)[0] != RIGHT_PAREN)
                {
                    fprintf(stderr, "walk(): if: bad syntax\n");
                    exit(EXIT_FAILURE);
                }

                AST_Node *if_expr = ast_node_new(IN_AST, Conditional_Form, IF, test_expr, then_expr, else_expr);
                (*current_p)++; // skip the ')' of if expression
                return if_expr;
            }
            
            // handle 'and'
            if (strcmp(TYPECAST(const char *, token->value), "and") == 0)
            {
                // move to first expr or ')'
                (*current_p)++;
                token = tokens_nth(tokens, *current_p);

                Vector *exprs = VectorNew(sizeof(AST_Node *));

                // check ')'
                // (and) -> #t
                if ((token->value)[0] == RIGHT_PAREN)
                {
                    AST_Node *and_expr = ast_node_new(IN_AST, Conditional_Form, AND, exprs);
                    (*current_p)++; // skip the ')' of and expression
                    return and_expr;
                }

                // when have some exprs
                // (and 1), (and #t #f), ...
                while ((token->type != PUNCTUATION) ||
                       (token->type == PUNCTUATION && (token->value)[0] != RIGHT_PAREN)) 
                {
                    AST_Node *expr = walk(tokens, current_p);
                    VectorAppend(exprs, &expr);
                    token = tokens_nth(tokens, *current_p);
                }

                // check ')' of and expression
                if ((token->value)[0] != RIGHT_PAREN)
                {
                    fprintf(stderr, "walk(): and: bad syntax\n");
                    exit(EXIT_FAILURE);
                }

                AST_Node *and_expr = ast_node_new(IN_AST, Conditional_Form, AND, exprs);
                (*current_p)++; // skip the ')' of and expression
                return and_expr;
            }

            // handle 'not'
            if (strcmp(TYPECAST(const char *, token->value), "not") == 0)
            {
                // move to expr
                (*current_p)++;
                token = tokens_nth(tokens, *current_p);

                AST_Node *expr = walk(tokens, current_p);

                // check ')' of not expression
                token = tokens_nth(tokens, *current_p);
                if ((token->value)[0] != RIGHT_PAREN)
                {
                    fprintf(stderr, "walk(): not: bad syntax\n");
                    exit(EXIT_FAILURE);
                }

                AST_Node *not_expr = ast_node_new(IN_AST, Conditional_Form, NOT, expr);
                (*current_p)++; // skip the ')' of not expression
                return not_expr;
            }

            // handle 'or'
            if (strcmp(TYPECAST(const char *, token->value), "or") == 0)
            {
                // move to first expr or ')'
                (*current_p)++;
                token = tokens_nth(tokens, *current_p);

                Vector *exprs = VectorNew(sizeof(AST_Node *));

                // check ')'
                // (or) -> #f 
                if ((token->value)[0] == RIGHT_PAREN)
                {
                    AST_Node *or_expr = ast_node_new(IN_AST, Conditional_Form, OR, exprs);
                    (*current_p)++; // skip the ')' of and expression
                    return or_expr;
                } 

                // when have some exprs
                // (or 1) -> 1, (or #f ...) -> ...
                while ((token->type != PUNCTUATION) ||
                       (token->type == PUNCTUATION && (token->value)[0] != RIGHT_PAREN)) 
                {
                    AST_Node *expr = walk(tokens, current_p);
                    VectorAppend(exprs, &expr);
                    token = tokens_nth(tokens, *current_p);
                }

                // check ')' of or expression
                if ((token->value)[0] != RIGHT_PAREN)
                {
                    fprintf(stderr, "walk(): or: bad syntax\n");
                    exit(EXIT_FAILURE);
                }

                AST_Node *or_expr = ast_node_new(IN_AST, Conditional_Form, OR, exprs);
                (*current_p)++; // skip the ')' of and expression
                return or_expr;
            }

            // handle cond
            if (strcmp(TYPECAST(const char *, token->value), "cond") == 0)
            {
                Vector *cond_clauses = VectorNew(sizeof(AST_Node *));
                int else_statement_counter = 0;

                // move to first cond clause
                (*current_p)++;
                token = tokens_nth(tokens, *current_p);

                while ((token->type != PUNCTUATION) ||
                       (token->type == PUNCTUATION && (token->value)[0] != RIGHT_PAREN)) 
                {
                    AST_Node *cond_clause = NULL;

                    // check '['
                    if ((token->value)[0] != LEFT_SQUARE_BRACKET)
                    {
                        fprintf(stderr, "walk(): cond: bad syntax\n");
                        exit(EXIT_FAILURE);
                    }

                    // move to test-expr or else
                    (*current_p)++;
                    token = tokens_nth(tokens, *current_p);

                    if (strcmp(TYPECAST(const char *, token->value), "else") == 0)
                    {
                        // else statement
                        Vector *then_bodies = VectorNew(sizeof(AST_Node *));

                        // move to first then_body
                        (*current_p)++;
                        token = tokens_nth(tokens, *current_p);

                        while ((token->type != PUNCTUATION) ||
                               (token->type == PUNCTUATION && (token->value)[0] != RIGHT_SQUARE_BRACKET)) 
                        {
                            AST_Node *then_body = walk(tokens, current_p);
                            VectorAppend(then_bodies, &then_body);
                            token = tokens_nth(tokens, *current_p);
                        }
                        
                        else_statement_counter ++;
                        cond_clause = ast_node_new(IN_AST, Cond_Clause, ELSE_STATEMENT, NULL, then_bodies, NULL);
                    }
                    else
                    {
                        // other, actually TEST_EXPR_WITH_THENBODY only right now
                        Vector *then_bodies = VectorNew(sizeof(AST_Node *));
                        AST_Node *test_expr = walk(tokens, current_p);
                        token = tokens_nth(tokens, *current_p);

                        while ((token->type != PUNCTUATION) ||
                               (token->type == PUNCTUATION && (token->value)[0] != RIGHT_SQUARE_BRACKET)) 
                        {
                            AST_Node *then_body = walk(tokens, current_p);
                            VectorAppend(then_bodies, &then_body);
                            token = tokens_nth(tokens, *current_p);
                        }

                        cond_clause = ast_node_new(IN_AST, Cond_Clause, TEST_EXPR_WITH_THENBODY, test_expr, then_bodies, NULL);
                    }

                    // check ']'
                    if ((token->value)[0] != RIGHT_SQUARE_BRACKET)
                    {
                        fprintf(stderr, "walk(): cond: bad syntax\n");
                        exit(EXIT_FAILURE);
                    }

                    VectorAppend(cond_clauses, &cond_clause);
                    (*current_p)++; // skip ']'
                    token = tokens_nth(tokens, *current_p);
                }

                // check ')' of cond expression
                if ((token->value)[0] != RIGHT_PAREN)
                {
                    fprintf(stderr, "walk(): cond: bad syntax\n");
                    exit(EXIT_FAILURE);
                }

                // check else statement situation 
                if (else_statement_counter == 1)
                {
                    // check the last element of cond_clauses if it is a ELSE_STATEMENT or not
                    AST_Node *else_statment = *(AST_Node **)VectorNth(cond_clauses, VectorLength(cond_clauses) - 1);
                    if (else_statment->contents.cond_clause.test_expr != NULL)
                    {
                        fprintf(stderr, "walk(): cond: bad syntax\n");
                        exit(EXIT_FAILURE);
                    }
                }
                else if (else_statement_counter > 1)
                {
                    fprintf(stderr, "walk(): cond: bad syntax\n");
                    exit(EXIT_FAILURE);
                }

                AST_Node *cond_expr = ast_node_new(IN_AST, Conditional_Form, COND, cond_clauses);
                (*current_p)++; // skip the ')' of cond expression
                return cond_expr;
            }

            // handle set!
            if (strcmp(TYPECAST(const char *, token->value), "set!") == 0)
            {
                // move to id
                (*current_p)++;

                AST_Node *id = walk(tokens, current_p);
                AST_Node *expr = walk(tokens, current_p);

                // check ')'
                token = tokens_nth(tokens, *current_p);
                if ((token->value)[0] != RIGHT_PAREN)
                {
                    fprintf(stderr, "walk(): set!: bad syntax\n");
                    exit(EXIT_FAILURE);
                }

                AST_Node *set_expr = ast_node_new(IN_AST, Set_Form, id, expr);
                (*current_p)++; // skip the ')' of and expression
                return set_expr;
            }

            // handle ... 
            
            // handle normally function call
            struct {
                bool named;
                union {
                    Token *name_token;
                    AST_Node *lambda;
                } value;
            } named_or_lambda = {.named = false, .value = {NULL}};

            // named function call
            if (token->type == IDENTIFIER)
            {
                named_or_lambda.named = true;
                named_or_lambda.value.name_token = token;
            }

            if (token->type != IDENTIFIER)
            {
                // anonymous function call, like: ((lambda (x) x) x) 
                named_or_lambda.named = false;
                named_or_lambda.value.lambda = walk(tokens, current_p); 

                // check lambda form
                if (named_or_lambda.value.lambda->type != Lambda_Form)
                {
                    fprintf(stderr, "walk(): call expression: bad syntax\n");
                    exit(EXIT_FAILURE);
                }
            }

            Vector *params = VectorNew(sizeof(AST_Node *));

            // point to the first argument when named function call only
            if (named_or_lambda.named == true)
            {
                (*current_p)++; 
            }

            // update token
            token = tokens_nth(tokens, *current_p);

            while ((token->type != PUNCTUATION) ||
                   (token->type == PUNCTUATION && (token->value)[0] != RIGHT_PAREN)
            ) 
            {
                AST_Node *param = walk(tokens, current_p);
                if (param != NULL) VectorAppend(params, &param);
                token = tokens_nth(tokens, *current_p);
            }

            AST_Node *ast_node = NULL;

            if (named_or_lambda.named == true) 
            {
                ast_node = ast_node_new(IN_AST, Call_Expression, named_or_lambda.value.name_token->value, NULL, params);
            }

            if (named_or_lambda.named == false) 
            {
                ast_node = ast_node_new(IN_AST, Call_Expression, NULL, named_or_lambda.value.lambda, params);
            }
            
            (*current_p)++; // skip ')'
            return ast_node;
        }

        // '\'' and '.', list or pair
        if (token_value == APOSTROPHE) 
        {
            // check '(
            (*current_p)++;
            token = tokens_nth(tokens, *current_p);
            token_value = (token->value)[0];
            if (token_value != LEFT_PAREN)
            {
                fprintf(stderr, "List or pair literal must be starts with '( \n");
                exit(EXIT_FAILURE);
            }

            // move to first element of list or pair
            // or ) for '() empty list
            (*current_p)++;
            token = tokens_nth(tokens, *current_p);
            if ((token->value)[0] == RIGHT_PAREN)
            {
                // '() empty list here
                Vector *value = VectorNew(sizeof(AST_Node *));
                AST_Node *empty_list = ast_node_new(IN_AST, List_Literal, value);
                (*current_p)++; // skip ')'
                return empty_list;
            }

            // check list or pair '.', dont move current_p, use a tmp value instead
            bool is_pair = false;
            size_t cursor = *current_p + 1;
            Token *tmp = tokens_nth(tokens, cursor);
            unsigned char tmp_value = tmp->value[0];
            if (tmp_value == DOT) is_pair = true;

            AST_Node *ast_node = NULL;
            if (is_pair)
            {
                // pair here
                AST_Node *car = walk(tokens, current_p);
                (*current_p)++; // skip '.'
                AST_Node *cdr = walk(tokens, current_p);
                if (car == NULL || cdr == NULL)
                {
                    fprintf(stderr, "Pair literal should have two values\n");
                    exit(EXIT_FAILURE);
                }

                Vector *value = VectorNew(sizeof(AST_Node *));
                VectorAppend(value, &car);
                VectorAppend(value, &cdr);

                ast_node = ast_node_new(IN_AST, Pair_Literal, value);
            }
            else
            {
                // non-empty list here
                Vector *value = VectorNew(sizeof(AST_Node *));

                while ((token->type != PUNCTUATION) ||
                       (token->type == PUNCTUATION && (token->value)[0] != RIGHT_PAREN)
                )
                {
                    AST_Node *element = walk(tokens, current_p);
                    if (element != NULL) VectorAppend(value, &element);
                    token = tokens_nth(tokens, *current_p);
                }
                
                ast_node = ast_node_new(IN_AST, List_Literal, value);
            }

            (*current_p)++; // skip ')'
            return ast_node;
        }

        // handle PUNCTUATION ...

    }

    // handle ...
    
    // when no matches any Token_Type
    fprintf(stderr, "walk(): can not handle token -> type: %d, value: %s\n", token->type, token->value);
    exit(EXIT_FAILURE);
}

static void visitor_free_helper(void *value_addr, size_t index, Vector *vector, void *aux_data)
{
    AST_Node_Handler *handler = *(AST_Node_Handler **)value_addr;
    ast_node_handler_free(handler);
}

// traverser helper function
static void traverser_helper(AST_Node *node, AST_Node *parent, Visitor visitor, void *aux_data)
{
    if (node == NULL)
    {
        fprintf(stdout, "works out no value.\n");
        return;
    }

    AST_Node_Handler *handler = find_ast_node_handler(visitor, node->type);

    if (handler == NULL)
    {
        fprintf(stderr, "traverser_helper(): can not find handler for AST_Node_Type: %d\n", node->type);
        exit(EXIT_FAILURE);
    }

    // enter
    if (handler->enter != NULL) handler->enter(node, parent, aux_data);

    // left sub-tree first dfs algorithm
    if (node->type == Program)  
    {
        Vector *body = node->contents.program.body;
        for (size_t i = 0; i < VectorLength(body); i++)
        {
            AST_Node *ast_node = *(AST_Node **)VectorNth(body, i);
            traverser_helper(ast_node, node, visitor, aux_data);
        }
    }

    if (node->type == Call_Expression)
    {
        Vector *params = node->contents.call_expression.params;
        for (size_t i = 0; i < VectorLength(params); i++)
        {
            AST_Node *ast_node = *(AST_Node **)VectorNth(params, i);
            traverser_helper(ast_node, node, visitor, aux_data);
        }
    }

    if (node->type == Lambda_Form)
    {
        Vector *params = node->contents.lambda_form.params;
        for (size_t i = 0; i < VectorLength(params); i++)
        {
            AST_Node *ast_node = *(AST_Node **)VectorNth(params, i);
            traverser_helper(ast_node, node, visitor, aux_data);
        }
        Vector *body_exprs = node->contents.lambda_form.body_exprs;
        for (size_t i = 0; i < VectorLength(body_exprs); i++)
        {
            AST_Node *ast_node = *(AST_Node **)VectorNth(body_exprs, i);
            traverser_helper(ast_node, node, visitor, aux_data);
        }
    }

    if (node->type == Local_Binding_Form)
    {
        Local_Binding_Form_Type local_binding_form_type = node->contents.local_binding_form.type;

        if (local_binding_form_type == LET ||
            local_binding_form_type == LET_STAR ||
            local_binding_form_type == LETREC)
        {
            Vector *bindings = node->contents.local_binding_form.contents.lets.bindings;
            Vector *body_exprs = node->contents.local_binding_form.contents.lets.body_exprs;
            for (size_t i = 0; i < VectorLength(bindings); i++)            
            {
                AST_Node *binding = *(AST_Node **)VectorNth(bindings, i);
                traverser_helper(binding, node, visitor, aux_data);
            }
            for (size_t i = 0; i < VectorLength(body_exprs); i++)            
            {
                AST_Node *body_expr = *(AST_Node **)VectorNth(body_exprs, i);
                traverser_helper(body_expr, node, visitor, aux_data);
            }
        }

        if (local_binding_form_type == DEFINE)
        {
            AST_Node *binding = node->contents.local_binding_form.contents.define.binding;
            traverser_helper(binding, node, visitor, aux_data);
        }
    }

    if (node->type == Set_Form)
    {
        AST_Node *id = node->contents.set_form.id;
        AST_Node *expr = node->contents.set_form.expr;
        traverser_helper(id, node, visitor, aux_data);
        traverser_helper(expr, node, visitor, aux_data);
    }

    if (node->type == Conditional_Form)
    {
        Conditional_Form_Type conditional_form_type = node->contents.conditional_form.type;
        
        if (conditional_form_type == IF)
        {
            AST_Node *test_expr = node->contents.conditional_form.contents.if_expression.test_expr;
            AST_Node *then_expr = node->contents.conditional_form.contents.if_expression.then_expr;
            AST_Node *else_expr = node->contents.conditional_form.contents.if_expression.else_expr;
            traverser_helper(test_expr, node, visitor, aux_data);
            traverser_helper(then_expr, node, visitor, aux_data);
            traverser_helper(else_expr, node, visitor, aux_data);
        }

        if (conditional_form_type == AND)
        {
            Vector *exprs = node->contents.conditional_form.contents.and_expression.exprs;
            for (size_t i = 0; i < VectorLength(exprs); i++)
            {
                AST_Node *expr = *(AST_Node **)VectorNth(exprs, i);
                traverser_helper(expr, node, visitor, aux_data);
            }
        }

        if (conditional_form_type == NOT)
        {
            AST_Node *expr = node->contents.conditional_form.contents.not_expression.expr;
            traverser_helper(expr, node, visitor, aux_data);
        }

        if (conditional_form_type == OR)
        {
            Vector *exprs = node->contents.conditional_form.contents.or_expression.exprs;
            for (size_t i = 0; i < VectorLength(exprs); i++)
            {
                AST_Node *expr = *(AST_Node **)VectorNth(exprs, i);
                traverser_helper(expr, node, visitor, aux_data);
            }
        }

        if (conditional_form_type == COND)
        {
            Vector *cond_clauses = node->contents.conditional_form.contents.cond_expression.cond_clauses;
            for (size_t i = 0; i < VectorLength(cond_clauses); i++)
            {
                AST_Node *cond_clause = *(AST_Node **)VectorNth(cond_clauses, i);
                traverser_helper(cond_clause, node, visitor, aux_data);
            }
        }
    }

    if (node->type == Cond_Clause)
    {
        if (node->contents.cond_clause.type == TEST_EXPR_WITH_THENBODY)
        {
            // [test-expr then-body ...+]
            AST_Node *test_expr = node->contents.cond_clause.test_expr;
            Vector *then_bodies = node->contents.cond_clause.then_bodies;
            
            traverser_helper(test_expr, node, visitor, aux_data);

            for (size_t i = 0; i < VectorLength(then_bodies); i++)
            {
                AST_Node *then_body = *(AST_Node **)VectorNth(then_bodies, i);
                traverser_helper(then_body, node, visitor, aux_data);
            }
        }
        else if (node->contents.cond_clause.type == ELSE_STATEMENT)
        {
            // [else then-body ...+]
            Vector *then_bodies = node->contents.cond_clause.then_bodies;

            for (size_t i = 0; i < VectorLength(then_bodies); i++)
            {
                AST_Node *then_body = *(AST_Node **)VectorNth(then_bodies, i);
                traverser_helper(then_body, node, visitor, aux_data);
            }
        }
        else if (node->contents.cond_clause.type == TEST_EXPR_WITH_PROC)
        {
        }
        else if (node->contents.cond_clause.type == SINGLE_TEST_EXPR)
        {
        }
        else
        {
            // something wrong here
            fprintf(stderr, "traverser_helper(): can not handle Cond_Clause_Type: %d\n", node->contents.cond_clause.type);
            exit(EXIT_FAILURE);
        }
    }

    if (node->type == List_Literal)
    {
        Vector *value = TYPECAST(Vector *, node->contents.literal.value);
        for (size_t i = 0; i < VectorLength(value); i++)
        {
            AST_Node *ast_node = *(AST_Node **)VectorNth(value, i);
            traverser_helper(ast_node, node, visitor, aux_data);
        }
    }

    if (node->type == Pair_Literal)
    {
        Vector *value = TYPECAST(Vector *, node->contents.literal.value);
        for (size_t i = 0; i < VectorLength(value); i++)
        {
            AST_Node *ast_node = *(AST_Node **)VectorNth(value, i);
            traverser_helper(ast_node, node, visitor, aux_data);
        }
    }

    if (node->type == Binding)
    {
        AST_Node *value = node->contents.binding.value;
        if (value != NULL)
        {
            traverser_helper(value, node, visitor, aux_data);
        }
    }

    // exit
    if(handler->exit != NULL) handler->exit(node, parent, aux_data);
}

static void set_tag_rec_visitor_helper(AST_Node *node, AST_Node *parent, void *aux)
{
    // inherit tag from parent
    ast_node_set_tag(node, *(AST_Node_Tag *)aux);
}