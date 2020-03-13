#include "../include/parser.h"
#include "../include/interpreter.h"
#include "../include/racket_built_in.h"
#include "../include/addon.h"
#include "../include/vector.h"
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <stddef.h>

static AST_Node *find_contextable_node(AST_Node *current_node);
static AST_Node *search_binding_value(AST_Node *binding);
static void *context_copy_helper(void *value_addr, size_t index, Vector *original_vector, Vector *new_vector, void *aux_data);
static void operand_free_helper(void *value_addr, size_t index, Vector *vector, void *aux_data); 
static int result_free(Result result);
static void output_result(Result result, void *aux_data);

void generate_context(AST_Node *node, AST_Node *parent, void *aux_data)
{
    node->parent = parent;

    if (node->type == Program)
    {
        if (node->context == NULL)
        {
            node->context = VectorNew(sizeof(AST_Node *));
        }

        // only one Program Node in AST, so the following code will run only once
        Vector *built_in_bindings = generate_built_in_bindings(); // add built-in binding to Program
        for (size_t i = 0; i < VectorLength(built_in_bindings); i++)
        {
            AST_Node *binding = *(AST_Node **)VectorNth(built_in_bindings, i);
            generate_context(binding, node, aux_data); // generate context for built-in bindings
            VectorAppend(node->contents.program.built_in_bindings, &binding);
        }
        free_built_in_bindings(built_in_bindings, NULL); // free 'built_in_bindings' itself only

        Vector *addon_bindings = generate_addon_bindings(); // add addon binding to Program
        for (size_t i = 0; i < VectorLength(addon_bindings); i++)
        {
            AST_Node *binding = *(AST_Node **)VectorNth(addon_bindings, i);
            generate_context(binding, node, aux_data);
            VectorAppend(node->contents.program.addon_bindings, &binding);
        }
        free_addon_bindings(addon_bindings, NULL); // free 'addon_bindings' itself only

        // generate context for body
        Vector *body = node->contents.program.body;
        for (size_t i = 0; i < VectorLength(body); i++)
        {
            AST_Node *sub_node = *(AST_Node **)(VectorNth(body, i));
            generate_context(sub_node, node, aux_data);
        }
    }

    if (node->type == Local_Binding_Form)
    {
        if (node->contents.local_binding_form.type == DEFINE)
        {
            AST_Node *binding = node->contents.local_binding_form.contents.define.binding;
            AST_Node *contextable = find_contextable_node(node); // find the nearly parent contextable node
            if (contextable == NULL)
            {
                fprintf(stderr, "generate_context(): something wrong here, the contextable will not be null forever.\n");
                exit(EXIT_FAILURE);
            }
            else
            {
                VectorAppend(contextable->context, &binding);
            } 
            generate_context(binding, node, aux_data);
        }

        if (node->contents.local_binding_form.type == LET)
        {
            Vector *bindings = node->contents.local_binding_form.contents.lets.bindings;
            Vector *body_exprs = node->contents.local_binding_form.contents.lets.body_exprs;

            for (size_t i = 0; i < VectorLength(bindings); i++)
            {
                AST_Node *binding = *(AST_Node **)VectorNth(bindings, i);
                generate_context(binding, node, aux_data);
            }

            // append bindings to every expr in body_exprs, even a Number_Literal ast node
            for (size_t i = 0; i < VectorLength(body_exprs); i++)
            {
                AST_Node *body_expr = *(AST_Node **)VectorNth(body_exprs, i); 
                if (body_expr->context == NULL)
                {
                    body_expr->context = VectorNew(sizeof(AST_Node *));
                }
                for(size_t j = 0; j < VectorLength(bindings); j++)
                {
                    AST_Node *binding = *(AST_Node **)VectorNth(bindings, j);
                    VectorAppend(body_expr->context, &binding);
                }
            }

            for (size_t i = 0; i < VectorLength(body_exprs); i++)
            {
                AST_Node *body_expr = *(AST_Node **)VectorNth(body_exprs, i);
                generate_context(body_expr, node, aux_data);
            }
        }

        if (node->contents.local_binding_form.type == LET_STAR)
        {
            Vector *bindings = node->contents.local_binding_form.contents.lets.bindings;
            Vector *body_exprs = node->contents.local_binding_form.contents.lets.body_exprs;
            
            for (size_t i = 0; i < VectorLength(bindings) - 1; i++)
            {
                AST_Node *nxt_binding = *(AST_Node **)VectorNth(bindings, i + 1);
                AST_Node *value = nxt_binding->contents.binding.value;
                if (value->context == NULL)
                {
                    value->context = VectorNew(sizeof(AST_Node *));
                }
                for (size_t j = 0; j < i + 1; j++)
                {
                    AST_Node *binding = *(AST_Node **)VectorNth(bindings, j);
                    VectorAppend(value->context, &binding);
                }
            }

            for (size_t i = 0; i < VectorLength(bindings); i++)
            {
                AST_Node *binding = *(AST_Node **)VectorNth(bindings, i);
                generate_context(binding, node, aux_data);
            }
 
            // append bindings to every expr in body_exprs, even a Number_Literal ast node
            for (size_t i = 0; i < VectorLength(body_exprs); i++)
            {
                AST_Node *body_expr = *(AST_Node **)VectorNth(body_exprs, i); 
                if (body_expr->context == NULL)
                {
                    body_expr->context = VectorNew(sizeof(AST_Node *));
                }
                for(size_t j = 0; j < VectorLength(bindings); j++)
                {
                    AST_Node *binding = *(AST_Node **)VectorNth(bindings, j);
                    VectorAppend(body_expr->context, &binding);
                }
            }

            for (size_t i = 0; i < VectorLength(body_exprs); i++)
            {
                AST_Node *body_expr = *(AST_Node **)VectorNth(body_exprs, i);
                generate_context(body_expr, node, aux_data);
            }
        }

        if (node->contents.local_binding_form.type == LETREC)
        {
            Vector *bindings = node->contents.local_binding_form.contents.lets.bindings;
            Vector *body_exprs = node->contents.local_binding_form.contents.lets.body_exprs;

            if (VectorLength(bindings) == 1)
            {
                AST_Node *single_binding = *(AST_Node **)VectorNth(bindings, 0);
                AST_Node *value = single_binding->contents.binding.value;

                if (value->context == NULL)
                {
                    value->context = VectorNew(sizeof(AST_Node *));
                }

                VectorAppend(value->context, &single_binding);
            }

            for (size_t i = 0; i < VectorLength(bindings) - 1; i++)
            {
                AST_Node *nxt_binding = *(AST_Node **)VectorNth(bindings, i + 1);
                AST_Node *value = nxt_binding->contents.binding.value;

                if (value->context == NULL)
                {
                    value->context = VectorNew(sizeof(AST_Node *));
                }
                
                for (size_t j = 0; j <= i + 1; j++)
                {
                    AST_Node *binding = *(AST_Node **)VectorNth(bindings, j);
                    VectorAppend(value->context, &binding);
                }
            }

            for (size_t i = 0; i < VectorLength(bindings); i++)
            {
                AST_Node *binding = *(AST_Node **)VectorNth(bindings, i);
                generate_context(binding, node, aux_data);
            } 

            // append bindings to every expr in body_exprs, even a Number_Literal ast node
            for (size_t i = 0; i < VectorLength(body_exprs); i++)
            {
                AST_Node *body_expr = *(AST_Node **)VectorNth(body_exprs, i); 
                if (body_expr->context == NULL)
                {
                    body_expr->context = VectorNew(sizeof(AST_Node *));
                }
                for(size_t j = 0; j < VectorLength(bindings); j++)
                {
                    AST_Node *binding = *(AST_Node **)VectorNth(bindings, j);
                    VectorAppend(body_expr->context, &binding);
                }
            }

            for (size_t i = 0; i < VectorLength(body_exprs); i++)
            {
                AST_Node *body_expr = *(AST_Node **)VectorNth(body_exprs, i);
                generate_context(body_expr, node, aux_data);
            }
        }
    }

    if (node->type == Set_Form)
    {
        AST_Node *id = node->contents.set_form.id;
        AST_Node *expr = node->contents.set_form.expr;
        generate_context(id, node, aux_data);
        generate_context(expr, node, aux_data);
    }

    if (node->type == Conditional_Form)
    {
        if (node->contents.conditional_form.type == IF)
        {
            AST_Node *test_expr = node->contents.conditional_form.contents.if_expression.test_expr;
            AST_Node *then_expr = node->contents.conditional_form.contents.if_expression.then_expr;
            AST_Node *else_expr = node->contents.conditional_form.contents.if_expression.else_expr;
            generate_context(test_expr, node, aux_data);
            generate_context(then_expr, node, aux_data);
            generate_context(else_expr, node, aux_data);
        }

        if (node->contents.conditional_form.type == AND)
        {
            Vector *exprs = node->contents.conditional_form.contents.and_expression.exprs;
            for (size_t i = 0; i < VectorLength(exprs); i++) 
            {
                AST_Node *expr = *(AST_Node **)VectorNth(exprs, i);
                generate_context(expr, node, aux_data);
            }
        }

        if (node->contents.conditional_form.type == NOT)
        {
            AST_Node *expr = node->contents.conditional_form.contents.not_expression.expr;
            generate_context(expr, node, aux_data);
        }

        if (node->contents.conditional_form.type == OR)
        {
            Vector *exprs = node->contents.conditional_form.contents.or_expression.exprs;
            for (size_t i = 0; i < VectorLength(exprs); i++) 
            {
                AST_Node *expr = *(AST_Node **)VectorNth(exprs, i);
                generate_context(expr, node, aux_data);
            }
        }

        if (node->contents.conditional_form.type == COND)
        {
            Vector *cond_clauses = node->contents.conditional_form.contents.cond_expression.cond_clauses;
            for (size_t i = 0; i < VectorLength(cond_clauses); i++)
            {
                AST_Node *cond_clause = *(AST_Node **)VectorNth(cond_clauses, i);
                generate_context(cond_clause, node, aux_data);
            }
        }
    }

    if (node->type == Cond_Clause)
    {
        Cond_Clause_Type cond_clause_type = node->contents.cond_clause.type;

        if (cond_clause_type == TEST_EXPR_WITH_THENBODY)
        {
            // [test-expr then-body ...+]
            // needs to free test_expr and then_bodies
            AST_Node *test_expr = node->contents.cond_clause.test_expr;
            generate_context(test_expr, node, aux_data);

            Vector *then_bodies = node->contents.cond_clause.then_bodies;
            for (size_t i = 0; i < VectorLength(then_bodies); i++)
            {
                AST_Node *then_body = *(AST_Node **)VectorNth(then_bodies, i);
                generate_context(then_body, node, aux_data);
            }
        }
        else if (cond_clause_type == ELSE_STATEMENT)
        {
            // [else then-body ...+]
            // needs to free then_bodies
            Vector *then_bodies = node->contents.cond_clause.then_bodies;
            for (size_t i = 0; i < VectorLength(then_bodies); i++)
            {
                AST_Node *then_body = *(AST_Node **)VectorNth(then_bodies, i);
                generate_context(then_body, node, aux_data);
            }
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
            fprintf(stderr, "generate_context(): can not handle cond_clause_type: %d\n", cond_clause_type);
            exit(EXIT_FAILURE);
        }
    }

    if (node->type == Call_Expression)
    {
        Vector *params = node->contents.call_expression.params;
        for (size_t i = 0; i < VectorLength(params); i++)
        {
            AST_Node *param = *(AST_Node **)VectorNth(params, i);
            generate_context(param, node, aux_data);
        }
    }

    if (node->type == Binding)
    {
        AST_Node *value = node->contents.binding.value;
        // such call_expression (+ a b), 'a' and 'b' is binding, but dont have value
        // it should be search the value for 'a' in eval()
        if (value != NULL) generate_context(value, node, aux_data); 
    }
    
    if (node->type == Lambda_Form) 
    {
        Vector *params = node->contents.lambda_form.params;
        Vector *body_exprs = node->contents.lambda_form.body_exprs;

        for (size_t i = 0; i < VectorLength(params); i++)
        {
            AST_Node *param = *(AST_Node **)VectorNth(params, i);
            generate_context(param, node, aux_data);
        }

        // append param to every expr in body_exprs, even a Number_Literal ast node
        for (size_t i = 0; i < VectorLength(body_exprs); i++)
        {
            AST_Node *body_expr = *(AST_Node **)VectorNth(body_exprs, i); 
            if (body_expr->context == NULL)
            {
                body_expr->context = VectorNew(sizeof(AST_Node *));
            }
            for(size_t j = 0; j < VectorLength(params); j++)
            {
                AST_Node *param = *(AST_Node **)VectorNth(params, j);
                VectorAppend(body_expr->context, &param);
            }
        }

        for (size_t i = 0; i < VectorLength(body_exprs); i++)
        {
            AST_Node *body_expr = *(AST_Node **)VectorNth(body_exprs, i);
            generate_context(body_expr, node, aux_data);
        }
    }

    if (node->type == Procedure)
    {
        // built-in procedure
        if (node->contents.procedure.c_native_function != NULL)
        {

        }

        // programmer defined procedure
        if (node->contents.procedure.c_native_function == NULL)
        {
            Vector *params = node->contents.procedure.params;
            Vector *body_exprs = node->contents.procedure.body_exprs;

            for (size_t i = 0; i < VectorLength(params); i++)
            {
                AST_Node *param = *(AST_Node **)VectorNth(params, i);
                generate_context(param, node, aux_data);
            }

            // append param to every expr in body_exprs, even a Number_Literal ast node
            for (size_t i = 0; i < VectorLength(body_exprs); i++)
            {
                AST_Node *body_expr = *(AST_Node **)VectorNth(body_exprs, i); 
                if (body_expr->context == NULL)
                {
                    body_expr->context = VectorNew(sizeof(AST_Node *));
                }
                for(size_t j = 0; j < VectorLength(params); j++)
                {
                    AST_Node *param = *(AST_Node **)VectorNth(params, j);
                    VectorAppend(body_expr->context, &param);
                }
            }

            for (size_t i = 0; i < VectorLength(body_exprs); i++)
            {
                AST_Node *body_expr = *(AST_Node **)VectorNth(body_exprs, i);
                generate_context(body_expr, node, aux_data);
            } 
        }
    }
    
    if (node->type == Number_Literal ||
        node->type == String_Literal ||
        node->type == Character_Literal ||
        node->type == Boolean_Literal)
    {
        return;
    }

    if (node->type == NULL_Expression)
    {
        AST_Node *value = node->contents.null_expression.value;
        if (value != NULL)
            generate_context(value, node, aux_data);
    }

    if (node->type == EMPTY_Expression)
    {
        AST_Node *value = node->contents.empty_expression.value;
        if (value != NULL)
            generate_context(value, node, aux_data);
    }

    if (node->type == List_Literal)
    {
        // symbol doesnt impled right now, so '((+ 1 2)) will not be supported
        // only literal will appear in list literal
        // nothing will happend for follow code
        Vector *elems = (Vector *)node->contents.literal.value;
        for (size_t i = 0; i < VectorLength(elems); i++)
        {
            AST_Node *elem = *(AST_Node **)VectorNth(elems, i);
            generate_context(elem, node, aux_data);
        }
    }

    if (node->type == Pair_Literal)
    {
        // same situation with List_Literal
        Vector *elems = (Vector *)node->contents.literal.value;
        for (size_t i = 0; i < VectorLength(elems); i++)
        {
            AST_Node *elem = *(AST_Node **)VectorNth(elems, i);
            generate_context(elem, node, aux_data);
        }
    }
}

// return null when work out no value
// this function will make some ast_node not in ast, should be free manually
Result eval(AST_Node *ast_node, void *aux_data)
{
    if (ast_node == NULL) return NULL;
    Result result = NULL;
    bool matched = false;

    if (ast_node->type == Call_Expression)
    {
        matched = true;
        
        const unsigned char *name = ast_node->contents.call_expression.name;
        AST_Node *anonymous_procedure = ast_node->contents.call_expression.anonymous_procedure;
        AST_Node *procedure = NULL;

        // anonymous procedure call or ((lambda (x) x) 1)
        if (name == NULL && anonymous_procedure != NULL)
        {
            procedure = anonymous_procedure;
            if (procedure->type == Lambda_Form)
                procedure = eval(procedure, aux_data);
        }
        // named procedure call
        else if (name != NULL && anonymous_procedure == NULL)
        {
            AST_Node *binding = ast_node_new(NOT_IN_AST, Binding, name, NULL);
            generate_context(binding, ast_node, NULL);
            AST_Node *binding_contains_value = search_binding_value(binding);
            ast_node_free(binding); // free the tmp binding

            if (binding_contains_value == NULL)
            {
                fprintf(stderr, "eval(): unbound identifier: %s\n", name);
                exit(EXIT_FAILURE);
            }

            procedure = binding_contains_value->contents.binding.value;
        }
        else {
            fprintf(stderr, "eval(): call expression error\n");
            exit(EXIT_FAILURE);
        }

        if (procedure->type != Procedure)
        {
            fprintf(stderr, "eval(): not a procedure: %s\n", name);
            exit(EXIT_FAILURE); 
        }

        // built-in procedure
        if (procedure->contents.procedure.c_native_function != NULL)
        {
            Vector *params = ast_node->contents.call_expression.params;
            Vector *operands = VectorNew(sizeof(AST_Node *));

            // eval out operands
            for (size_t i = 0; i < VectorLength(params); i++)
            {
                AST_Node *param = *(AST_Node **)VectorNth(params, i);
                AST_Node *operand = eval(param, aux_data);
                VectorAppend(operands, &operand);
            }

            Function c_native_function = procedure->contents.procedure.c_native_function;
            result = ((AST_Node *(*)(AST_Node *procedure, Vector *operands))c_native_function)(procedure, operands);

            // double here.
            // VectorFree(operands, operand_free_helper, NULL);

            VectorFree(operands, NULL, NULL);
        }

        // programmer defined procedure
        if (procedure->contents.procedure.c_native_function == NULL)
        {
            Vector *params = ast_node->contents.call_expression.params;
            Vector *operands = VectorNew(sizeof(AST_Node *)); 

            // eval out operands
            for (size_t i = 0; i < VectorLength(params); i++)
            {
                AST_Node *param = *(AST_Node **)VectorNth(params, i);
                AST_Node *operand = eval(param, aux_data);
                VectorAppend(operands, &operand);
            }

            // check arity
            int required_params_count = procedure->contents.procedure.required_params_count;
            size_t operands_count = VectorLength(operands);
            if (operands_count != required_params_count)
            {
                if (procedure->contents.procedure.name == NULL)
                {
                    fprintf(stderr, "anomyous procedure: arity mismatch;\n"
                                    "the expected number of arguments does not match the given number\n"
                                    "expected: %d\n"
                                    "given: %zu\n", required_params_count, operands_count);
                    exit(EXIT_FAILURE); 
                }
                else if (procedure->contents.procedure.name != NULL)
                {
                    fprintf(stderr, "%s: arity mismatch;\n"
                                    "the expected number of arguments does not match the given number\n"
                                    "expected: %d\n"
                                    "given: %zu\n", procedure->contents.procedure.name, required_params_count, operands_count);
                    exit(EXIT_FAILURE); 
                }
            }

            // generate a environment for every function call
            Vector *virtual_params = procedure->contents.procedure.params;
            Vector *body_exprs = procedure->contents.procedure.body_exprs;

            Vector *virtual_params_copy = VectorNew(sizeof(AST_Node *));
            Vector *body_exprs_copy = VectorNew(sizeof(AST_Node *));
            
            for (size_t i = 0; i < VectorLength(virtual_params); i++)
            {
                AST_Node *node = *(AST_Node **)VectorNth(virtual_params, i);
                AST_Node *node_copy = ast_node_deep_copy(node, aux_data);

                if (node->context != NULL)
                {
                    node_copy->context = VectorCopy(node->context, context_copy_helper, NULL);
                }

                generate_context(node_copy, node->parent, NULL);
                VectorAppend(virtual_params_copy, &node_copy);
            }

            for (size_t i = 0; i < VectorLength(body_exprs); i++)
            {
                AST_Node *node = *(AST_Node **)VectorNth(body_exprs, i);
                AST_Node *node_copy = ast_node_deep_copy(node, aux_data);

                if (node->context != NULL)
                {
                    node_copy->context = VectorNew(sizeof(AST_Node *));
                    for(size_t j = 0; j < VectorLength(virtual_params_copy); j++)
                    {
                        AST_Node *virtual_param_copy = *(AST_Node **)VectorNth(virtual_params_copy, j);
                        VectorAppend(node_copy->context, &virtual_param_copy);
                    }
                }

                generate_context(node_copy, node->parent, NULL);
                VectorAppend(body_exprs_copy, &node_copy);
            }

            // binding virtual params copy to actual params
            for (size_t i = 0; i < VectorLength(virtual_params_copy); i++)
            {
                AST_Node *virtual_param_copy = *(AST_Node **)VectorNth(virtual_params_copy, i);
                AST_Node *operand = *(AST_Node **)VectorNth(operands, i);
                virtual_param_copy->contents.binding.value = operand;
            }

            // eval
            size_t last = VectorLength(body_exprs_copy) - 1;
            for (size_t i = 0; i < VectorLength(body_exprs_copy); i++)
            {
                AST_Node *body_expr_copy = *(AST_Node **)VectorNth(body_exprs_copy, i);
                result = eval(body_expr_copy, aux_data);
                if (i != last && result != NULL && ast_node_get_tag(result) == NOT_IN_AST)
                {
                    ast_node_free(result);
                }
            }

            // set virtual_param_copy's value to null
            for (size_t i = 0; i < VectorLength(virtual_params_copy); i++)
            {
                AST_Node *virtual_param_copy = *(AST_Node **)VectorNth(virtual_params_copy, i);
                virtual_param_copy->contents.binding.value = NULL; 
            }

            // free operands
            VectorFree(operands, NULL, NULL);

            // free virtual_params_copy
            for (size_t i = 0; i < VectorLength(virtual_params_copy); i++)
            {
                AST_Node *virtual_param_copy = *(AST_Node **)VectorNth(virtual_params_copy, i);
                ast_node_free(virtual_param_copy);
            }
            
            VectorFree(virtual_params_copy, NULL, NULL);

            // free body_exprs_copy
            // TO-DO solve problem here !
            VectorFree(body_exprs_copy, NULL, NULL);
        }
    }
    
    if (ast_node->type == Local_Binding_Form)
    {
        Local_Binding_Form_Type local_binding_form_type = ast_node->contents.local_binding_form.type;

        if (local_binding_form_type == DEFINE)
        {
            // define works out no value
            // eval the init value of binding into a new ast_node, and instead of the init value node by new ast_node
            matched = true;
            AST_Node *binding = ast_node->contents.local_binding_form.contents.define.binding;
            AST_Node *init_value = binding->contents.binding.value;
            AST_Node *eval_value = eval(init_value, aux_data);
            binding->contents.binding.value = eval_value;

            if (eval_value != init_value)
            {
                if (eval_value != NULL && ast_node_get_tag(eval_value) == IN_AST)
                {
                    binding->contents.binding.value = ast_node_deep_copy(eval_value, NULL);
                }
                else if (eval_value != NULL && ast_node_get_tag(eval_value) == NOT_IN_AST)
                {
                    ast_node_set_tag(binding->contents.binding.value, IN_AST);
                    // maybe double free here
                    ast_node_free(init_value);
                }
                generate_context(binding->contents.binding.value, binding, NULL);
            }

            if (binding->contents.binding.value->type == Procedure)
            {
                AST_Node *procedure = binding->contents.binding.value;
                procedure->contents.procedure.name = malloc(strlen((const char *)(binding->contents.binding.name)) + 1);
                strcpy((char *)(procedure->contents.procedure.name), (const char *)(binding->contents.binding.name));
            }
        }
        
        if (local_binding_form_type == LET ||
            local_binding_form_type == LET_STAR ||
            local_binding_form_type == LETREC)
        {
            matched = true;
            Vector *bindings = ast_node->contents.local_binding_form.contents.lets.bindings;
            Vector *body_exprs = ast_node->contents.local_binding_form.contents.lets.body_exprs;

            // eval bindings
            for (size_t i = 0; i < VectorLength(bindings); i++)
            {
                AST_Node *binding = *(AST_Node **)VectorNth(bindings, i);
                AST_Node *init_value = binding->contents.binding.value;
                AST_Node *eval_value = eval(init_value, aux_data);
                binding->contents.binding.value = eval_value;

                if (eval_value != init_value)
                {
                    if (eval_value != NULL && ast_node_get_tag(eval_value) == IN_AST)
                    {
                        binding->contents.binding.value = ast_node_deep_copy(eval_value, NULL);
                    }
                    else if (eval_value != NULL && ast_node_get_tag(eval_value) == NOT_IN_AST)
                    {
                        ast_node_set_tag(binding->contents.binding.value, IN_AST);
                        // maybe double free here
                        ast_node_free(init_value);
                    }
                    generate_context(binding->contents.binding.value, binding, NULL);
                }

                if (binding->contents.binding.value->type == Procedure)
                {
                    AST_Node *procedure = binding->contents.binding.value;
                    procedure->contents.procedure.name = malloc(strlen((const char *)(binding->contents.binding.name)) + 1);
                    strcpy((char *)(procedure->contents.procedure.name), (const char *)(binding->contents.binding.name));
                }
            }

            // eval body_exprs
            size_t last = VectorLength(body_exprs) - 1;
            for (size_t i = 0; i < VectorLength(body_exprs); i++)
            {
                AST_Node *body_expr = *(AST_Node **)VectorNth(body_exprs, i);
                result = eval(body_expr, aux_data); // the last body_expr's result will be returned
                if (i != last) 
                {
                    if (result != NULL && ast_node_get_tag(result) == NOT_IN_AST)
                    {
                        // maybe double free here
                        ast_node_free(result);
                    }
                }
            }
        }
    }

    if (ast_node->type == Set_Form)
    {
        matched = true;
        AST_Node *id = ast_node->contents.set_form.id;
        AST_Node *expr = ast_node->contents.set_form.expr;

        AST_Node *binding = search_binding_value(id);
        AST_Node *expr_val = eval(expr, aux_data);

        AST_Node *old_value = binding->contents.binding.value;
        ast_node_free(old_value);

        binding->contents.binding.value = ast_node_deep_copy(expr_val, aux_data);
        binding->contents.binding.value->tag = IN_AST;
        if (binding->contents.binding.value->type == Procedure)
        {
            AST_Node *procedure = binding->contents.binding.value;
            procedure->contents.procedure.name = malloc(strlen((const char *)(binding->contents.binding.name)) + 1);
            strcpy((char *)(procedure->contents.procedure.name), (const char *)(binding->contents.binding.name));
        }
        generate_context(binding->contents.binding.value, ast_node, aux_data);

        result = NULL;
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

            AST_Node *test_val = eval(test_expr, aux_data);
            if (test_val == NULL)
            {
                fprintf(stderr, "eval(): if: bad syntax\n");
                exit(EXIT_FAILURE); 
            }

            Boolean_Type val = R_TRUE; // true by default
            if (test_val->type == Boolean_Literal &&
               *(Boolean_Type *)(test_val->contents.literal.value) == R_FALSE)
            {
                val = R_FALSE;
            }

            if (ast_node_get_tag(test_val) == NOT_IN_AST)
            {
                ast_node_free(test_val);
            }

            if (val == R_TRUE)
            {
                // excute then expr
                // only (define) will work out NULL, and (define) can not be in (if)
                result = eval(then_expr, aux_data);
                if (result == NULL)
                {
                    fprintf(stderr, "eval(): if: bad syntax\n");
                    exit(EXIT_FAILURE);
                }
            }

            if (val == R_FALSE)
            {
                // excute else expr
                // only (define) will work out NULL, and (define) can not be in (if)
                result = eval(else_expr, aux_data);
                if (result == NULL)
                {
                    fprintf(stderr, "eval(): if: bad syntax\n");
                    exit(EXIT_FAILURE);
                }
            }
        }

        if (conditional_form_type == COND)
        {
            matched = true;
            Vector *cond_clauses = ast_node->contents.conditional_form.contents.cond_expression.cond_clauses;

            for (size_t i = 0; i < VectorLength(cond_clauses); i++)
            {
                AST_Node *cond_clause = *(AST_Node **)VectorNth(cond_clauses, i);

                if (cond_clause->contents.cond_clause.type == TEST_EXPR_WITH_THENBODY)
                {
                    // [test-expr then-body ...+]
                    AST_Node *test_expr = cond_clause->contents.cond_clause.test_expr;

                    AST_Node *test_val = eval(test_expr, aux_data);
                    if (test_val == NULL)
                    {
                        fprintf(stderr, "eval(): cond: bad syntax\n");
                        exit(EXIT_FAILURE); 
                    }

                    if (test_val->type != Boolean_Literal ||
                        (test_val->type == Boolean_Literal && (*(Boolean_Type *)(test_val->contents.literal.value) == R_TRUE)))

                    {
                        Vector *then_bodies = cond_clause->contents.cond_clause.then_bodies;
                        for (size_t j = 0; j < VectorLength(then_bodies); j++)
                        {
                            AST_Node *then_body = *(AST_Node **)VectorNth(then_bodies, j);
                            result = eval(then_body, aux_data);
                        }
                        break;
                    }

                    if (ast_node_get_tag(test_val) == NOT_IN_AST)
                    {
                        ast_node_free(test_val);
                    } 
                }
                else if (cond_clause->contents.cond_clause.type == ELSE_STATEMENT)
                {
                    // [else then-body ...+]
                    Vector *then_bodies = cond_clause->contents.cond_clause.then_bodies;
                    for (size_t j = 0; j < VectorLength(then_bodies); j++)
                    {
                        AST_Node *then_body = *(AST_Node **)VectorNth(then_bodies, j);
                        result = eval(then_body, aux_data);
                    }
                    break;
                }
                else if (cond_clause->contents.cond_clause.type == TEST_EXPR_WITH_PROC)
                {
                }
                else if (cond_clause->contents.cond_clause.type == SINGLE_TEST_EXPR)
                {
                }
                else
                {
                    // something wrong here
                    fprintf(stderr, "eval(): can not handle Cond_Clause_Type: %d\n", cond_clause->contents.cond_clause.type);
                    exit(EXIT_FAILURE);
                }
            }
        }

        if (conditional_form_type == AND)
        {
            matched = true;
            Vector *exprs = ast_node->contents.conditional_form.contents.and_expression.exprs;

            if (VectorLength(exprs) == 0)
            {
                Boolean_Type *value = malloc(sizeof(Boolean_Type)); 
                *value = R_TRUE;
                result = ast_node_new(NOT_IN_AST, Boolean_Literal, value);
            }

            if (VectorLength(exprs) > 0)
            {
                for (size_t i = 0; i < VectorLength(exprs); i++)
                {
                    AST_Node *expr = *(AST_Node **)VectorNth(exprs, i);
                    AST_Node *expr_val = eval(expr, aux_data);

                    if (expr_val->type == Boolean_Literal &&
                        *(Boolean_Type *)(expr_val->contents.literal.value) == R_FALSE)
                    {
                        Boolean_Type *value = malloc(sizeof(Boolean_Type)); 
                        *value = R_FALSE;
                        result = ast_node_new(NOT_IN_AST, Boolean_Literal, value);
                        break;
                    }

                    result = expr_val;
                }
            }
        }

        if (conditional_form_type == NOT)
        {
            matched = true;

            AST_Node *expr = ast_node->contents.conditional_form.contents.not_expression.expr;
            AST_Node *expr_val = eval(expr, aux_data);            

            Boolean_Type *value = malloc(sizeof(Boolean_Type));

            if (expr_val->type == Boolean_Literal && *(Boolean_Type *)(expr_val->contents.literal.value) == R_FALSE)
                *value = R_TRUE; 
            else
                *value = R_FALSE; 

            result = ast_node_new(NOT_IN_AST, Boolean_Literal, value);
        }

        if (conditional_form_type == OR)
        {
            matched = true;
            Vector *exprs = ast_node->contents.conditional_form.contents.or_expression.exprs;
            
            if (VectorLength(exprs) == 0)
            {
                Boolean_Type *value = malloc(sizeof(Boolean_Type));
                *value = R_FALSE;
                result = ast_node_new(NOT_IN_AST, Boolean_Literal, value);
            }
            else if (VectorLength(exprs) == 1)
            {
                AST_Node *expr = *(AST_Node **)VectorNth(exprs, 0);
                AST_Node *expr_val = eval(expr, aux_data);
                result = expr_val;
            }
            else if (VectorLength(exprs) > 1)
            {
                for (size_t i = 0; i < VectorLength(exprs); i++)
                {
                    AST_Node *expr = *(AST_Node **)VectorNth(exprs, i);
                    AST_Node *expr_val = eval(expr, aux_data);

                    if (expr_val->type != Boolean_Literal ||
                        (expr_val->type == Boolean_Literal && *(Boolean_Type *)(expr_val->contents.literal.value) == R_TRUE))
                    {
                        result = expr_val;
                        break;
                    } 

                    result = expr_val;
                }
            }
        }
    }

    if (ast_node->type == Binding)
    { 
        // return the value of Binding
        matched = true;
        AST_Node *value = ast_node->contents.binding.value;
        if (value == NULL)
        {
            AST_Node *binding_contains_value = search_binding_value(ast_node);
            if (binding_contains_value == NULL)
            {
                fprintf(stderr, "eval(): unbound identifier: %s\n", ast_node->contents.binding.name);
                exit(EXIT_FAILURE);
            }
            value = binding_contains_value->contents.binding.value;
        }
        result = value;
    }

    // these kind of AST_Node_Type works out them self
    if (ast_node->type == Number_Literal ||
        ast_node->type == String_Literal ||
        ast_node->type == Character_Literal ||
        ast_node->type == Boolean_Literal)
    {
        matched = true;
        result = ast_node;
    }

    if (ast_node->type == NULL_Expression ||
        ast_node->type == EMPTY_Expression)
    {
        // return '()
        // the element's size in list means nothing, so use 1 byte(sizeof(unsigned char))
        Vector *empty_vector = VectorNew(sizeof(unsigned char));
        AST_Node *empty_list = ast_node_new(NOT_IN_AST, List_Literal, empty_vector);
        return empty_list;
    }

    // List_Literal works out itself
    if (ast_node->type == List_Literal)
    {
        matched = true;
        result = ast_node;
    }

    // Pair_Literal works out itself
    if (ast_node->type == Pair_Literal)
    {
        matched = true;
        result = ast_node;
    }

    // Procedure works out itself
    if (ast_node->type == Procedure)
    {
        matched = true;
        result = ast_node;
    }

    if (ast_node->type == Lambda_Form)
    {
        matched = true;
        Vector *params = ast_node->contents.lambda_form.params;
        Vector *body_exprs = ast_node->contents.lambda_form.body_exprs;

        result = ast_node_new(NOT_IN_AST, Procedure, NULL, -1, NULL, NULL, NULL);

        Vector *params_copy = VectorNew(sizeof(AST_Node *));
        Vector *body_exprs_copy = VectorNew(sizeof(AST_Node *));

        for (size_t i = 0; i < VectorLength(params); i++)
        {
            AST_Node *node = *(AST_Node **)VectorNth(params, i);
            AST_Node *node_copy = ast_node_deep_copy(node, aux_data);
            if (node->context != NULL)
            {
                Vector *context_copy = VectorCopy(node->context, context_copy_helper, NULL);
                node_copy->context = context_copy;
            }
            generate_context(node_copy, result, NULL);
            VectorAppend(params_copy, &node_copy);
        }

        for (size_t i = 0; i < VectorLength(body_exprs); i++)
        {
            AST_Node *node = *(AST_Node **)VectorNth(body_exprs, i);
            AST_Node *node_copy = ast_node_deep_copy(node, aux_data);
            if (node_copy->context == NULL)
            {
                node_copy->context = VectorNew(sizeof(AST_Node *));
            }
            for(size_t j = 0; j < VectorLength(params_copy); j++)
            {
                AST_Node *param_copy = *(AST_Node **)VectorNth(params_copy, j);
                VectorAppend(node_copy->context, &param_copy);
            }
            generate_context(node_copy, result, NULL);
            VectorAppend(body_exprs_copy, &node_copy);
        }

        result->contents.procedure.required_params_count = VectorLength(params_copy);
        result->contents.procedure.params = params_copy;
        result->contents.procedure.body_exprs = body_exprs_copy;

        // if lambda_form itself has context, it should be inherit to the procedure
        if (ast_node->context != NULL)
        {
            result->context = VectorCopy(ast_node->context, context_copy_helper, NULL);
        }

        generate_context(result, ast_node->parent, NULL);
    }

    if (matched == false)
    {
        // when no matches any AST_Node_Type
        fprintf(stderr, "eval(): can not eval AST_Node_Type: %d\n", ast_node->type);
        exit(EXIT_FAILURE);
    }

    return result;
}

// return: Vector *(Result)
Vector *calculator(AST ast, void *aux_data)
{
    generate_context(ast, NULL, NULL); // generate context 

    Vector *body = ast->contents.program.body;
    Vector *results = VectorNew(sizeof(AST_Node *));

    for (size_t i = 0; i < VectorLength(body); i++)
    {
        AST_Node *sub_node = *(AST_Node **)VectorNth(body, i);
        Result result = eval(sub_node, aux_data);
        if (result != NULL)
        {
            if (ast_node_get_tag(result) == IN_AST)
            {
                AST_Node *result_copy = ast_node_deep_copy(result, NULL);
                result_copy->tag = NOT_IN_AST;
                VectorAppend(results, &result_copy);
            }
            else if (ast_node_get_tag(result) == NOT_IN_AST)
            {
                VectorAppend(results, &result);
            }
        }
    }

    return results;
}

int results_free(Vector *results)
{
    int error = 0;

    for (size_t i = 0; i < VectorLength(results); i++)
    {
        AST_Node *result = *(AST_Node **)VectorNth(results, i);
        error = error | result_free(result);
    }

    return error;
}

void output_results(Vector *results, void *aux_data)
{
    for (size_t i = 0; i < VectorLength(results); i++)
    {
        Result result = *(AST_Node **)VectorNth(results, i);
        output_result(result, aux_data);
        fprintf(stdout, "\n");
    }
}

// find the nearly parent contextable node
static AST_Node *find_contextable_node(AST_Node *current_node)
{
    if (current_node == NULL) return NULL;

    AST_Node *contextable = NULL;

    if (current_node->context == NULL)
    {
        contextable = current_node->parent;
        while (contextable != NULL &&
               contextable->context == NULL) 
        {
            contextable = contextable->parent;
        }
    }
    else
    {
        contextable = current_node;
    }

    return contextable;
}

// param: AST_Node *(type: Binding) has just a name
// return: AST_Node *(type: Binding) contains value
static AST_Node *search_binding_value(AST_Node *binding)
{
    if (binding == NULL)
    {
        fprintf(stderr, "can not search binding value for NULL\n");
        exit(EXIT_FAILURE);
    }

    // if current binding node has value
    if (binding->contents.binding.value != NULL) return binding;

    AST_Node *binding_contains_value = NULL;
    AST_Node *contextable = find_contextable_node(binding);

    // search binding's value by 'name'
    while (contextable != NULL)
    {
        Vector *context = contextable->context;
        Vector *built_in_bindings = NULL;
        Vector *addon_bindings = NULL;

        if (contextable->type == Program) built_in_bindings = contextable->contents.program.built_in_bindings;
        if (contextable->type == Program) addon_bindings = contextable->contents.program.addon_bindings; 
        
        for (size_t i = VectorLength(context); i > 0; i--)
        {
            AST_Node *node = *(AST_Node **)VectorNth(context, i - 1);
            #ifdef DEBUG_MODE
            printf("searching for name: %s, cur node's name: %s\n", binding->contents.binding.name, node->contents.binding.name);
            #endif
            if (strcmp((const char *)(binding->contents.binding.name), (const char *)(node->contents.binding.name)) == 0)
            {
                binding_contains_value = node;

                // check value
                AST_Node *value = binding_contains_value->contents.binding.value;
                if (value == NULL)
                {
                    fprintf(stderr, "search_binding_value(): unbound identifier: %s\n", binding->contents.binding.name);
                    exit(EXIT_FAILURE);
                }

                goto found;
            }
        }

        if (built_in_bindings != NULL)
        {
            for (size_t i = VectorLength(built_in_bindings); i > 0; i--)
            {
                AST_Node *node = *(AST_Node **)VectorNth(built_in_bindings, i - 1);
                #ifdef DEBUG_MODE
                printf("searching for name: %s, cur node's name: %s\n", binding->contents.binding.name, node->contents.binding.name);
                #endif
                if (strcmp((const char *)(binding->contents.binding.name), (const char *)(node->contents.binding.name)) == 0)
                {
                    binding_contains_value = node;

                    // check value
                    AST_Node *value = binding_contains_value->contents.binding.value;
                    if (value == NULL)
                    {
                        fprintf(stderr, "search_binding_value(): unbound identifier: %s\n", binding->contents.binding.name);
                        exit(EXIT_FAILURE);
                    }

                    goto found;
                }
            }
        }

        if (addon_bindings != NULL)
        {
            for (size_t i = VectorLength(addon_bindings); i > 0; i--)
            {
                AST_Node *node = *(AST_Node **)VectorNth(addon_bindings, i - 1);
                #ifdef DEBUG_MODE
                printf("searching for name: %s, cur node's name: %s\n", binding->contents.binding.name, node->contents.binding.name);
                #endif
                if (strcmp((const char *)(binding->contents.binding.name), (const char *)(node->contents.binding.name)) == 0)
                {
                    binding_contains_value = node;

                    // check value
                    AST_Node *value = binding_contains_value->contents.binding.value;
                    if (value == NULL)
                    {
                        fprintf(stderr, "search_binding_value(): unbound identifier: %s\n", binding->contents.binding.name);
                        exit(EXIT_FAILURE);
                    }

                    goto found;
                }
            }
        }

        contextable = find_contextable_node(contextable->parent);
    }

    if (binding_contains_value == NULL)
    {
        fprintf(stderr, "search_binding_value(): unbound identifier: %s\n", binding->contents.binding.name);
        exit(EXIT_FAILURE);
    }

    found: return binding_contains_value;
}

static void *context_copy_helper(void *value_addr, size_t index, Vector *original_vector, Vector *new_vector, void *aux_data)
{
    AST_Node *binding = *(AST_Node **)value_addr;
    return value_addr;
}

static void operand_free_helper(void *value_addr, size_t index, Vector *vector, void *aux_data) 
{
    AST_Node *operand = *(AST_Node **)value_addr;
    if (operand->tag == NOT_IN_AST) ast_node_free(operand);
}

static int result_free(Result result)
{
    if (result == NULL) return 1;
    return ast_node_free(result);
}

static void output_result(Result result, void *aux_data)
{
    bool matched = false;

    if (result->type == Number_Literal)
    {
        matched = true;
        fprintf(stdout, "%s", (unsigned char *)(result->contents.literal.value));
    }

    if (result->type ==  String_Literal)
    {
        matched = true;
        fprintf(stdout, "\"%s\"", (unsigned char *)(result->contents.literal.value));
    }

    if (result->type ==  Character_Literal)
    {
        matched = true;
        fprintf(stdout, "#\\%c", *(unsigned char *)(result->contents.literal.value));
    }

    if (result->type ==  List_Literal)
    {
        matched = true;

        Vector *value = (Vector *)(result->contents.literal.value);

        size_t length = VectorLength(value);
        size_t last = length - 1;

        if (aux_data != NULL && strcmp(aux_data, "in_list_or_in_pair") == 0)
        {
            fprintf(stdout, "(");
        }
        else
        {
            fprintf(stdout, "'(");
        }
        
        for (size_t i = 0; i < length; i++)
        {
            AST_Node *node = *(AST_Node **)VectorNth(value, i);
            output_result(node, "in_list_or_in_pair");
            if (i != last) fprintf(stdout, " ");
        }
        fprintf(stdout, ")");
    }

    if (result->type == Pair_Literal)
    {
        matched = true;

        Vector *value = (Vector *)(result->contents.literal.value);
        AST_Node *car = *(AST_Node **)VectorNth(value, 0);
        AST_Node *cdr = *(AST_Node **)VectorNth(value, 1);

        int length = 2; // a dot pair always has two elements

        if (aux_data != NULL && strcmp(aux_data, "in_list_or_in_pair") == 0)
        {
            fprintf(stdout, "(");
        }
        else
        {
            fprintf(stdout, "'(");
        }
        output_result(car, "in_list_or_in_pair");
        fprintf(stdout, " . ");
        output_result(cdr, "in_list_or_in_pair");
        fprintf(stdout, ")");
    }

    if (result->type == Boolean_Literal)
    {
        matched = true;

        Boolean_Type *value = (Boolean_Type *)(result->contents.literal.value);

        if (*value == R_TRUE)
            fprintf(stdout, "#t");
        else if (*value == R_FALSE)
            fprintf(stdout, "#f");
    }

    if (result->type == Procedure)
    {
        matched = true;

        unsigned char *name = result->contents.procedure.name;

        if (name == NULL)
            fprintf(stdout, "#<procedure>");
        else if (name != NULL)
            fprintf(stdout, "#<procedure:%s>", name);
    }

    if (matched == false)
    {
        // when no matches any AST_Node_Type
        fprintf(stderr, "output_result(): can not output AST_Node_Type: %d\n", result->type);
        exit(EXIT_FAILURE);
    }
}