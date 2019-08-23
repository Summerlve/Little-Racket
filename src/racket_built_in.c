#include "./racket_built_in.h"
#include "./interpreter.h"
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <stdarg.h>
#include <string.h>

#define DOUBLE_MAX_DIGIT_LENGTH 512 

static int int_digit_count(int num)
{
    int digit_count = 0;

    if (num > 0)
    {
        digit_count = (int)((ceil(log10(num)) + 1) * sizeof(char));
    }
    else if (num < 0)
    {
        num = -num;
        digit_count = 1 + (int)((ceil(log10(num)) + 1) * sizeof(char));
    }
    else if(num == 0)
    {
        digit_count = 1;
    }
    
    return digit_count;
}

AST_Node *racket_native_addition(AST_Node *procedure, Vector *operands)
{
    // check arity
    int arity = procedure->contents.procedure.required_params_count;
    int operands_count = VectorLength(operands);
    if (operands_count < arity)
    {
        fprintf(stderr, "%s: arity mismatch;\n"
                        "the expected number of arguments does not match the given number\n"
                        "expected: at least %d\n"
                        "given: %d\n", procedure->contents.procedure.name, arity, operands_count);
        exit(EXIT_FAILURE); 
    }

    struct {
        bool is_int;
        union {
            int iv;
            double dv;
        } value;
    } result = {
        .is_int = true,
        .value.iv = 0
    };

    for (int i = 0; i < operands_count; i++)
    {
        const AST_Node *operand = *(AST_Node **)VectorNth(operands, i);

        if (operand->type != Number_Literal)
        {
            fprintf(stderr, "#<procedure:%s>: operands must be number\n", procedure->contents.procedure.name);
            exit(EXIT_FAILURE); 
        }

        bool cur_operand_is_int = false;
        if (strchr(operand->contents.literal.value, '.') == NULL) cur_operand_is_int = true;

        if (cur_operand_is_int == true)
        {
            int c_native_value = *(int *)(operand->contents.literal.c_native_value);
            if (result.is_int == true)
            {
                result.value.iv += c_native_value;
            }
            else
            {
                result.value.dv += c_native_value;
            }
        }
        else
        {
            double c_native_value = *(double *)(operand->contents.literal.c_native_value);
            if (result.is_int == true)
            {
                result.is_int = false;
                double tmp = (double)result.value.iv;
                result.value.dv = tmp + c_native_value;
            }
            else
            {
                result.value.dv += c_native_value;
            }
        }
    }

    char *value = NULL;
    if (result.is_int == true)
    {
        // convert int to string.
        value = malloc(int_digit_count(result.value.iv) + 1);
        sprintf(value, "%d", result.value.iv);
    }
    else
    {
        // convert double to string
        value = malloc(DOUBLE_MAX_DIGIT_LENGTH + 1);
        sprintf(value, "%lf", result.value.dv);
    }
    
    AST_Node *ast_node = ast_node_new(NOT_IN_AST, Number_Literal, value);
    free(value);
    return ast_node;
}

AST_Node *racket_native_subtraction(AST_Node *procedure, Vector *operands)
{
    // check arity
    int arity = procedure->contents.procedure.required_params_count;
    int operands_count = VectorLength(operands);
    if (operands_count < arity)
    {
        fprintf(stderr, "%s: arity mismatch;\n"
                        "the expected number of arguments does not match the given number\n"
                        "expected: at least %d\n"
                        "given: %d\n", procedure->contents.procedure.name, arity, operands_count);
        exit(EXIT_FAILURE); 
    }

    const AST_Node *minuend = *(AST_Node **)VectorNth(operands, 0);
    if (minuend->type != Number_Literal)
    {
        fprintf(stderr, "#<procedure:%s>: operands must be number\n", procedure->contents.procedure.name);
        exit(EXIT_FAILURE); 
    }

    struct {
        bool is_int;
        union {
            int iv;
            double dv;
        } value;
    } result = {
        .is_int = true,
        .value.iv = 0
    };

    if (strchr(minuend->contents.literal.value, '.') == NULL)
    {
        // minuend is int.
        int c_native_value = *(int *)(minuend->contents.literal.c_native_value);
        if (operands_count != 1)
        {
            result.value.iv = c_native_value;
        }
        else
        {
            result.value.iv = 0 - c_native_value;
        }
    }
    else
    {
        // minuend is double.
        result.is_int = false;
        double c_native_value = *(double *)(minuend->contents.literal.c_native_value);
        if (operands_count != 1)
        {
            result.value.dv = c_native_value;
        }
        else
        {
            result.value.dv = -c_native_value;
        }
    }

    for (int i = 1; i < operands_count; i++)
    {
        const AST_Node *subtrahend = *(AST_Node **)VectorNth(operands, i); 

        if (subtrahend->type != Number_Literal)
        {
            fprintf(stderr, "#<procedure:%s>: operands must be number\n", procedure->contents.procedure.name);
            exit(EXIT_FAILURE); 
        }

        bool cur_operand_is_int = false;
        if (strchr(subtrahend->contents.literal.value, '.') == NULL) cur_operand_is_int = true;

        if (cur_operand_is_int == true)
        {
            int c_native_value = *(int *)(subtrahend->contents.literal.c_native_value);
            if (result.is_int == true)
            {
                result.value.iv -= c_native_value;
            }
            else
            {
                result.value.dv -= c_native_value;
            }
        }
        else
        {
            double c_native_value = *(double *)(subtrahend->contents.literal.c_native_value);
            if (result.is_int == true)
            {
                result.is_int = false;
                double tmp = (double)result.value.iv;
                result.value.dv = tmp - c_native_value;
            }
            else
            {
                result.value.dv -= c_native_value;
            }
        }
    }

    char *value = NULL;
    if (result.is_int == true)
    {
        // convert int to string.
        value = malloc(int_digit_count(result.value.iv) + 1);
        sprintf(value, "%d", result.value.iv);
    }
    else
    {
        // convert double to string
        value = malloc(DOUBLE_MAX_DIGIT_LENGTH + 1);
        sprintf(value, "%lf", result.value.dv);
    }
    
    AST_Node *ast_node = ast_node_new(NOT_IN_AST, Number_Literal, value);
    free(value);
    return ast_node;
}

AST_Node *racket_native_multiplication(AST_Node *procedure, Vector *operands)
{
    // check arity
    int arity = procedure->contents.procedure.required_params_count;
    int operands_count = VectorLength(operands);
    if (operands_count < arity)
    {
        fprintf(stderr, "%s: arity mismatch;\n"
                        "the expected number of arguments does not match the given number\n"
                        "expected: at least %d\n"
                        "given: %d\n", procedure->contents.procedure.name, arity, operands_count);
        exit(EXIT_FAILURE); 
    }

    struct {
        bool is_int;
        union {
            int iv;
            double dv;
        } value;
    } result = {
        .is_int = true,
        .value.iv = 1 
    };

    for (int i = 0; i < operands_count; i++)
    {
        const AST_Node *operand = *(AST_Node **)VectorNth(operands, i);

        if (operand->type != Number_Literal)
        {
            fprintf(stderr, "#<procedure:%s>: operands must be number\n", procedure->contents.procedure.name);
            exit(EXIT_FAILURE); 
        }

        bool cur_operand_is_int = false;
        if (strchr(operand->contents.literal.value, '.') == NULL) cur_operand_is_int = true;

        if (cur_operand_is_int == true)
        {
            int c_native_value = *(int *)(operand->contents.literal.c_native_value);
            if (result.is_int == true)
            {
                result.value.iv *= c_native_value;
            }
            else
            {
                result.value.dv *= c_native_value;
            }
        }
        else
        {
            double c_native_value = *(double *)(operand->contents.literal.c_native_value);
            if (result.is_int == true)
            {
                result.is_int = false;
                double tmp = (double)result.value.iv;
                result.value.dv = tmp * c_native_value;
            }
            else
            {
                result.value.dv *= c_native_value;
            }
        }
    }

    char *value = NULL;
    if (result.is_int == true)
    {
        // convert int to string.
        value = malloc(int_digit_count(result.value.iv) + 1);
        sprintf(value, "%d", result.value.iv);
    }
    else
    {
        // convert double to string
        value = malloc(DOUBLE_MAX_DIGIT_LENGTH + 1);
        sprintf(value, "%lf", result.value.dv);
    }
    
    AST_Node *ast_node = ast_node_new(NOT_IN_AST, Number_Literal, value);
    free(value);
    return ast_node;
}

AST_Node *racket_native_division(AST_Node *procedure, Vector *operands)
{
    // check arity
    int arity = procedure->contents.procedure.required_params_count;
    int operands_count = VectorLength(operands);
    if (operands_count < arity)
    {
        fprintf(stderr, "%s: arity mismatch;\n"
                        "the expected number of arguments does not match the given number\n"
                        "expected: at least %d\n"
                        "given: %d\n", procedure->contents.procedure.name, arity, operands_count);
        exit(EXIT_FAILURE); 
    }

    const AST_Node *dividend = *(AST_Node **)VectorNth(operands, 0);
    if (dividend->type != Number_Literal)
    {
        fprintf(stderr, "#<procedure:%s>: operands must be number\n", procedure->contents.procedure.name);
        exit(EXIT_FAILURE); 
    }
    
    double result = 0.0;

    double dividend_value = 0.0;
    if (strchr(dividend->contents.literal.value, '.') == NULL)
    {
        dividend_value = *(int *)(dividend->contents.literal.c_native_value);
    }
    else
    {
        dividend_value = *(double *)(dividend->contents.literal.c_native_value);
    }

    if (operands_count == 1)
    {
        if (dividend_value == 0)
        {
            fprintf(stderr, "/: division by zero\n");
            exit(EXIT_FAILURE); 
        }

        result = 1 / dividend_value;
    }
    else
    {
        result = dividend_value;
    }
    

    for (int i = 1; i < operands_count; i++)
    {
        const AST_Node *divisor = *(AST_Node **)VectorNth(operands, i); 

        if (divisor->type != Number_Literal)
        {
            fprintf(stderr, "#<procedure:%s>: operands must be number\n", procedure->contents.procedure.name);
            exit(EXIT_FAILURE); 
        }

        double c_native_value = 0.0;
        if (strchr(divisor->contents.literal.value, '.') == NULL)
        {
            c_native_value = *(int *)(divisor->contents.literal.c_native_value);
        }
        else
        {
            c_native_value = *(double *)(divisor->contents.literal.c_native_value);
        }

        if (c_native_value == 0)
        {
            fprintf(stderr, "/: division by zero\n");
            exit(EXIT_FAILURE); 
        }

        result /= c_native_value;
    }

    char *value = NULL;
    // convert double to string
    value = malloc(DOUBLE_MAX_DIGIT_LENGTH + 1);
    sprintf(value, "%lf", result);
    
    AST_Node *ast_node = ast_node_new(NOT_IN_AST, Number_Literal, value);
    free(value);
    return ast_node;
}

AST_Node *racket_native_number_euqal(AST_Node *procedure, Vector *operands)
{
    // check arity
    int arity = procedure->contents.procedure.required_params_count;
    int operands_count = VectorLength(operands);
    if (operands_count < arity)
    {
        fprintf(stderr, "%s: arity mismatch;\n"
                        "the expected number of arguments does not match the given number\n"
                        "expected: at least %d\n"
                        "given: %d\n", procedure->contents.procedure.name, arity, operands_count);
        exit(EXIT_FAILURE); 
    }

    const AST_Node *pre_number = *(AST_Node **)VectorNth(operands, 0); 
    if (pre_number->type != Number_Literal)
    {
        fprintf(stderr, "#<procedure:%s>: operands must be number\n", procedure->contents.procedure.name);
        exit(EXIT_FAILURE); 
    }

    struct {
        union {
            int int_val;
            double double_val;
        } contents;
        bool is_int;
    } pre, cur;
    
    if (strchr(pre_number->contents.literal.value, '.') == NULL)
    {
        pre.contents.int_val = *(int *)(pre_number->contents.literal.c_native_value);
        pre.is_int = true;
    }
    else
    {
        pre.contents.double_val = *(double *)(pre_number->contents.literal.c_native_value);
        pre.is_int = false;
    }

    Boolean_Type *result = malloc(sizeof(Boolean_Type));
    *result = R_TRUE; // true by default.

    for (int i = 1; i < VectorLength(operands); i++)
    {
        AST_Node *cur_number = *(AST_Node **)VectorNth(operands, i); 

        if (cur_number->type != Number_Literal)
        {
            fprintf(stderr, "#<procedure:%s>: operands must be number\n", procedure->contents.procedure.name);
            exit(EXIT_FAILURE); 
        }

        if (strchr(cur_number->contents.literal.value, '.') == NULL)
        {
            cur.contents.int_val = *(int *)(cur_number->contents.literal.c_native_value);
            cur.is_int = true;
        }
        else
        {
            cur.contents.double_val = *(double *)(cur_number->contents.literal.c_native_value);
            cur.is_int = false;
        }

        if (cur.is_int == true && pre.is_int == true)
        {
            if (cur.contents.int_val != pre.contents.int_val) *result = R_FALSE;
            #ifdef DEBUG_MODE
            // printf("pre: %d, cur: %d\n", pre.contents.int_val, cur.contents.int_val);
            #endif
            pre.contents.int_val = cur.contents.int_val;
        }
        else if (cur.is_int == true && pre.is_int == false)
        {
            if (cur.contents.int_val != pre.contents.double_val) *result = R_FALSE;
            #ifdef DEBUG_MODE
            // printf("pre: %f, cur: %d\n", pre.contents.double_val, cur.contents.int_val);
            #endif
            pre.contents.int_val = cur.contents.int_val;
        }
        else if (cur.is_int == false && pre.is_int == true)
        {
            if (cur.contents.double_val != pre.contents.int_val) *result = R_FALSE;
            #ifdef DEBUG_MODE
            // printf("pre: %d, cur: %f\n", pre.contents.int_val, cur.contents.double_val);
            #endif
            pre.contents.double_val = cur.contents.double_val;
        }
        else if (cur.is_int == false && pre.is_int == false)
        {
            if (cur.contents.double_val != pre.contents.double_val) *result = R_FALSE;
            #ifdef DEBUG_MODE
            // printf("pre: %f, cur: %f\n", pre.contents.double_val, cur.contents.double_val);
            #endif
            pre.contents.double_val = cur.contents.double_val;
        }
        pre.is_int = cur.is_int;
    }

    AST_Node *ast_node = ast_node_new(NOT_IN_AST, Boolean_Literal, result);
    return ast_node;
}

// (map fn list ...)
AST_Node *racket_native_map(AST_Node *procedure, Vector *operands)
{
    // check arity
    int arity = procedure->contents.procedure.required_params_count;
    int operands_count = VectorLength(operands);
    if (operands_count < arity)
    {
        fprintf(stderr, "%s: arity mismatch;\n"
                        "the expected number of arguments does not match the given number\n"
                        "expected: at least %d\n"
                        "given: %d\n", procedure->contents.procedure.name, arity, operands_count);
        exit(EXIT_FAILURE); 
    }

    // check procedure
    AST_Node *fn = *(AST_Node **)VectorNth(operands, 0);
    if (fn->type != Procedure)
    {
        fprintf(stderr, "%s: parameter's type is incorrecly\n", procedure->contents.procedure.name);
        exit(EXIT_FAILURE); 
    }

    // the rest of operands must be list
    AST_Node *first_list = *(AST_Node **)VectorNth(operands, 1); 
    if (first_list->type != List_Literal)
    {
        fprintf(stderr, "%s: parameter's type is incorrecly\n", procedure->contents.procedure.name);
        exit(EXIT_FAILURE); 
    }

    int list_length = VectorLength((Vector *)(first_list->contents.literal.value));

    for (int i = 1; i < VectorLength(operands); i++)
    {
        // check list
        AST_Node *list = *(AST_Node **)VectorNth(operands, i);
        if (list->type != List_Literal)
        {
            fprintf(stderr, "%s: parameter's type is incorrecly\n", procedure->contents.procedure.name);
            exit(EXIT_FAILURE); 
        }

        // check list size
        int cur_list_length = VectorLength((Vector *)(list->contents.literal.value));
        if (list_length != cur_list_length)
        {
            fprintf(stderr, "%s: all lists must have same size\n", procedure->contents.procedure.name);
            exit(EXIT_FAILURE); 
        }
    }

    Vector *results = VectorNew(sizeof(AST_Node *));

    for (int i = 0; i < list_length; i++)
    {
        Vector *column = VectorNew(sizeof(AST_Node *));

        for (int j = 1; j < VectorLength(operands); j++)
        {
            AST_Node *list = *(AST_Node **)VectorNth(operands, j);
            Vector *value = (Vector *)(list->contents.literal.value);
            AST_Node *item = *(AST_Node **)VectorNth(value, i);
            VectorAppend(column, &item);
        }

        // execute fn
        // constructe a call expression and call the eval().
        AST_Node *call_expression = NULL;
        // named procedure
        if (fn->contents.procedure.name != NULL)
        {
            call_expression = ast_node_new(NOT_IN_AST, Call_Expression, fn->contents.procedure.name, NULL, column);
        }
        // anonymous procedure
        else if (fn->contents.procedure.name == NULL)
        {
            call_expression = ast_node_new(NOT_IN_AST, Call_Expression, NULL, fn, column);
        }

        generate_context(call_expression, fn, NULL);
        Result result = eval(call_expression, NULL);

        // append to results
        if (ast_node_get_tag(result) == IN_AST)
        {
            result = ast_node_deep_copy(result, NULL);
        }
        VectorAppend(results, &result);

        VectorFree(column, NULL, NULL);
    }

    AST_Node *ast_node = ast_node_new(NOT_IN_AST, List_Literal, results);
    return ast_node;
}

AST_Node *racket_native_is_list(AST_Node *procedure, Vector *operands)
{
    return NULL;
}

Vector *generate_built_in_bindings(void)
{
    Vector *built_in_bindings = VectorNew(sizeof(AST_Node *));
    AST_Node *binding = NULL;
    AST_Node *procedure = NULL;

    procedure = ast_node_new(BUILT_IN_PROCEDURE, Procedure, "+", 0, NULL, NULL, (void(*)(void))racket_native_addition); 
    binding = ast_node_new(BUILT_IN_BINDING, Binding, "+", procedure);
    VectorAppend(built_in_bindings, &binding);

    procedure = ast_node_new(BUILT_IN_PROCEDURE,Procedure,  "-", 1, NULL, NULL, (void(*)(void))racket_native_subtraction); 
    binding = ast_node_new(BUILT_IN_BINDING, Binding, "-", procedure);
    VectorAppend(built_in_bindings, &binding);

    procedure = ast_node_new(BUILT_IN_PROCEDURE, Procedure, "*", 0, NULL, NULL, (void(*)(void))racket_native_multiplication); 
    binding = ast_node_new(BUILT_IN_BINDING, Binding, "*", procedure);
    VectorAppend(built_in_bindings, &binding);

    procedure = ast_node_new(BUILT_IN_PROCEDURE, Procedure, "/", 1, NULL, NULL, (void(*)(void))racket_native_division); 
    binding = ast_node_new(BUILT_IN_BINDING, Binding, "/", procedure);
    VectorAppend(built_in_bindings, &binding);

    procedure = ast_node_new(BUILT_IN_PROCEDURE, Procedure, "=", 1, NULL, NULL, (void(*)(void))racket_native_number_euqal); 
    binding = ast_node_new(BUILT_IN_BINDING, Binding, "=", procedure);
    VectorAppend(built_in_bindings, &binding);

    procedure = ast_node_new(BUILT_IN_PROCEDURE, Procedure, "map", 2, NULL, NULL, (void(*)(void))racket_native_map); 
    binding = ast_node_new(BUILT_IN_BINDING, Binding, "map", procedure);
    VectorAppend(built_in_bindings, &binding);

    return built_in_bindings;
}

int free_built_in_bindings(Vector *built_in_bindings, VectorFreeFunction free_fn)
{
    return VectorFree(built_in_bindings, free_fn, NULL);
}