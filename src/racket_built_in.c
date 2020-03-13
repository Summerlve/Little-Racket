#include "../include/racket_built_in.h"
#include "../include/interpreter.h"
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>
#include <stdbool.h>

#define DOUBLE_MAX_DIGIT_LENGTH ((size_t)512)

static size_t int_digit_count(int num)
{
    size_t digit_count = 0;

    if (num > 0)
    {
        digit_count = (size_t)((ceil(log10(num)) + 1) * sizeof(unsigned char));
    }
    else if (num < 0)
    {
        num = -num;
        digit_count = 1 + (size_t)((ceil(log10(num)) + 1) * sizeof(unsigned char));
    }
    else if(num == 0)
    {
        digit_count = 1;
    }
    
    return digit_count;
}

static AST_Node *racket_native_addition(AST_Node *procedure, Vector *operands)
{
    // check arity
    size_t arity = procedure->contents.procedure.required_params_count;
    size_t operands_count = VectorLength(operands);
    if (operands_count < arity)
    {
        fprintf(stderr, "%s: arity mismatch;\n"
                        "the expected number of arguments does not match the given number\n"
                        "expected: at least %zu\n"
                        "given: %zu\n", procedure->contents.procedure.name, arity, operands_count);
        exit(EXIT_FAILURE); 
    }

    struct {
        bool is_int;
        union {
            long long int iv;
            double dv;
        } value;
    } result = {
        .is_int = true,
        .value.iv = 0
    };

    for (size_t i = 0; i < operands_count; i++)
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
            long long int c_native_value = *(long long int *)(operand->contents.literal.c_native_value);
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

    unsigned char *value = NULL;
    if (result.is_int == true)
    {
        // convert int to string.
        value = malloc(int_digit_count(result.value.iv) + 1);
        sprintf((char *)value, "%lld", result.value.iv);
    }
    else
    {
        // convert double to string
        value = malloc(DOUBLE_MAX_DIGIT_LENGTH + 1);
        sprintf((char *)value, "%lf", result.value.dv);
    }
    
    AST_Node *ast_node = ast_node_new(NOT_IN_AST, Number_Literal, value);
    free(value);
    return ast_node;
}

static AST_Node *racket_native_subtraction(AST_Node *procedure, Vector *operands)
{
    // check arity
    size_t arity = procedure->contents.procedure.required_params_count;
    size_t operands_count = VectorLength(operands);
    if (operands_count < arity)
    {
        fprintf(stderr, "%s: arity mismatch;\n"
                        "the expected number of arguments does not match the given number\n"
                        "expected: at least %zu\n"
                        "given: %zu\n", procedure->contents.procedure.name, arity, operands_count);
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
            long long int iv;
            double dv;
        } value;
    } result = {
        .is_int = true,
        .value.iv = 0
    };

    if (strchr(minuend->contents.literal.value, '.') == NULL)
    {
        // minuend is int.
        long long int c_native_value = *(long long int *)(minuend->contents.literal.c_native_value);
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

    for (size_t i = 1; i < operands_count; i++)
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
            long long int c_native_value = *(long long int *)(subtrahend->contents.literal.c_native_value);
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

    unsigned char *value = NULL;
    if (result.is_int == true)
    {
        // convert int to string.
        value = malloc(int_digit_count(result.value.iv) + 1);
        sprintf((char *)value, "%lld", result.value.iv);
    }
    else
    {
        // convert double to string
        value = malloc(DOUBLE_MAX_DIGIT_LENGTH + 1);
        sprintf((char *)value, "%lf", result.value.dv);
    }
    
    AST_Node *ast_node = ast_node_new(NOT_IN_AST, Number_Literal, value);
    free(value);
    return ast_node;
}

static AST_Node *racket_native_multiplication(AST_Node *procedure, Vector *operands)
{
    // check arity
    size_t arity = procedure->contents.procedure.required_params_count;
    size_t operands_count = VectorLength(operands);
    if (operands_count < arity)
    {
        fprintf(stderr, "%s: arity mismatch;\n"
                        "the expected number of arguments does not match the given number\n"
                        "expected: at least %zu\n"
                        "given: %zu\n", procedure->contents.procedure.name, arity, operands_count);
        exit(EXIT_FAILURE); 
    }

    struct {
        bool is_int;
        union {
            long long int iv;
            double dv;
        } value;
    } result = {
        .is_int = true,
        .value.iv = 1 
    };

    for (size_t i = 0; i < operands_count; i++)
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
            long long int c_native_value = *(long long int *)(operand->contents.literal.c_native_value);
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

    unsigned char *value = NULL;
    if (result.is_int == true)
    {
        // convert int to string.
        value = malloc(int_digit_count(result.value.iv) + 1);
        sprintf((char *)value, "%lld", result.value.iv);
    }
    else
    {
        // convert double to string
        value = malloc(DOUBLE_MAX_DIGIT_LENGTH + 1);
        sprintf((char *)value, "%lf", result.value.dv);
    }
    
    AST_Node *ast_node = ast_node_new(NOT_IN_AST, Number_Literal, value);
    free(value);
    return ast_node;
}

static AST_Node *racket_native_division(AST_Node *procedure, Vector *operands)
{
    // check arity
    size_t arity = procedure->contents.procedure.required_params_count;
    size_t operands_count = VectorLength(operands);
    if (operands_count < arity)
    {
        fprintf(stderr, "%s: arity mismatch;\n"
                        "the expected number of arguments does not match the given number\n"
                        "expected: at least %zu\n"
                        "given: %zu\n", procedure->contents.procedure.name, arity, operands_count);
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
        dividend_value = *(long long int *)(dividend->contents.literal.c_native_value);
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
    

    for (size_t i = 1; i < operands_count; i++)
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
            c_native_value = *(long long int *)(divisor->contents.literal.c_native_value);
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

    unsigned char *value = NULL;
    // convert double to string
    value = malloc(DOUBLE_MAX_DIGIT_LENGTH + 1);
    sprintf((char *)value, "%lf", result);
    
    AST_Node *ast_node = ast_node_new(NOT_IN_AST, Number_Literal, value);
    free(value);
    return ast_node;
}

// (= z w ...) -> boolean?
static AST_Node *racket_native_number_equal(AST_Node *procedure, Vector *operands)
{
    // check arity
    size_t arity = procedure->contents.procedure.required_params_count;
    size_t operands_count = VectorLength(operands);
    if (operands_count < arity)
    {
        fprintf(stderr, "%s: arity mismatch;\n"
                        "the expected number of arguments does not match the given number\n"
                        "expected: at least %zu\n"
                        "given: %zu\n", procedure->contents.procedure.name, arity, operands_count);
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

    for (size_t i = 1; i < VectorLength(operands); i++)
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
static AST_Node *racket_native_map(AST_Node *procedure, Vector *operands)
{
    // check arity
    size_t arity = procedure->contents.procedure.required_params_count;
    size_t operands_count = VectorLength(operands);
    if (operands_count < arity)
    {
        fprintf(stderr, "%s: arity mismatch;\n"
                        "the expected number of arguments does not match the given number\n"
                        "expected: at least %zu\n"
                        "given: %zu\n", procedure->contents.procedure.name, arity, operands_count);
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

    size_t list_length = VectorLength((Vector *)(first_list->contents.literal.value));

    for (size_t i = 1; i < VectorLength(operands); i++)
    {
        // check list
        AST_Node *list = *(AST_Node **)VectorNth(operands, i);
        if (list->type != List_Literal)
        {
            fprintf(stderr, "%s: parameter's type is incorrecly\n", procedure->contents.procedure.name);
            exit(EXIT_FAILURE); 
        }

        // check list size
        size_t cur_list_length = VectorLength((Vector *)(list->contents.literal.value));
        if (list_length != cur_list_length)
        {
            fprintf(stderr, "%s: all lists must have same size\n", procedure->contents.procedure.name);
            exit(EXIT_FAILURE); 
        }
    }

    Vector *results = VectorNew(sizeof(AST_Node *));

    if (list_length == 0)
    {
        size_t list_num = VectorLength(operands) - 1;

        // check arity
        if (list_num != fn->contents.procedure.required_params_count)
        {
            fprintf(stderr, "map: argument mismatch;\n"
                        "the given procedure's expected number of arguments does not match the given number of lists\n"
                        "expected: %zu\n"
                        "given: %zu\n", fn->contents.procedure.required_params_count, list_num);
            exit(EXIT_FAILURE); 
        }
    }

    for (size_t i = 0; i < list_length; i++)
    {
        Vector *column = VectorNew(sizeof(AST_Node *));

        for (size_t j = 1; j < VectorLength(operands); j++)
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

// (list? v) -> boolean?
static AST_Node *racket_native_is_list(AST_Node *procedure, Vector *operands)
{
    // check arity
    size_t arity = procedure->contents.procedure.required_params_count;
    size_t operands_count = VectorLength(operands);
    if (operands_count != arity)
    {
        fprintf(stderr, "%s: arity mismatch;\n"
                        "the expected number of arguments does not match the given number\n"
                        "expected: %zu\n"
                        "given: %zu\n", procedure->contents.procedure.name, arity, operands_count);
        exit(EXIT_FAILURE); 
    }

    // get single v for operands
    AST_Node *v = *(AST_Node **)VectorNth(operands, 0);
    Boolean_Type *value = malloc(sizeof(Boolean_Type)); 

    if (v->type == List_Literal)
        *value = R_TRUE;
    else if (v->type != List_Literal)
        *value = R_FALSE;

    AST_Node *ast_node = ast_node_new(NOT_IN_AST, Boolean_Literal, value);
    return ast_node;
}

// (filter pred lst) -> list?
static AST_Node *racket_native_filter(AST_Node *procedure, Vector *operands)
{
    // check arity
    size_t arity = procedure->contents.procedure.required_params_count;
    size_t operands_count = VectorLength(operands);
    if (operands_count != arity)
    {
        fprintf(stderr, "%s: arity mismatch;\n"
                        "the expected number of arguments does not match the given number\n"
                        "expected: %zu\n"
                        "given: %zu\n", procedure->contents.procedure.name, arity, operands_count);
        exit(EXIT_FAILURE); 
    }

    // check procedure
    AST_Node *pred = *(AST_Node **)VectorNth(operands, 0);
    if (pred->type != Procedure)
    {
        fprintf(stderr, "%s: parameter's type is incorrecly\n", procedure->contents.procedure.name);
        exit(EXIT_FAILURE); 
    }

    // the second item of operands must be list
    AST_Node *list_literal = *(AST_Node **)VectorNth(operands, 1); 
    if (list_literal->type != List_Literal)
    {
        fprintf(stderr, "%s: parameter's type is incorrecly\n", procedure->contents.procedure.name);
        exit(EXIT_FAILURE); 
    }

    Vector *list = (Vector *)(list_literal->contents.literal.value);
    Vector *value = VectorNew(sizeof(AST_Node *));

    for (size_t i = 0; i < VectorLength(list); i++)
    {
        Vector *column = VectorNew(sizeof(AST_Node *));
        AST_Node *item = *(AST_Node **)VectorNth(list, i);
        VectorAppend(column, &item);
        
        // execute fn
        // constructe a call expression and call the eval().
        AST_Node *call_expression = NULL;
        // named procedure
        if (pred->contents.procedure.name != NULL)
        {
            call_expression = ast_node_new(NOT_IN_AST, Call_Expression, pred->contents.procedure.name, NULL, column);
        }
        // anonymous procedure
        else if (pred->contents.procedure.name == NULL)
        {
            call_expression = ast_node_new(NOT_IN_AST, Call_Expression, NULL, pred, column);
        }

        generate_context(call_expression, pred, NULL);
        Result result = eval(call_expression, NULL); 

        VectorFree(column, NULL, NULL);

        // check Boolean_Literal
        if (result->type != Boolean_Literal)
        {
            fprintf(stderr, "%s, racket_native_filter(): something wrong here\n", procedure->contents.procedure.name);
            exit(EXIT_FAILURE); 
        }

        // if #t append item to value
        Boolean_Type *is_true = (Boolean_Type *)(result->contents.literal.value);

        if (*is_true == R_TRUE)
        {
            AST_Node *item_copy = ast_node_deep_copy(item, NULL);
            VectorAppend(value, &item_copy);
        }
    }

    AST_Node *result = ast_node_new(NOT_IN_AST, List_Literal, value);
    return result;
}

// (> x y ...+) -> boolean?
static AST_Node *racket_native_number_more_than(AST_Node *procedure, Vector *operands)
{
    // check arity
    size_t arity = procedure->contents.procedure.required_params_count;
    size_t operands_count = VectorLength(operands);
    if (operands_count < arity)
    {
        fprintf(stderr, "%s: arity mismatch;\n"
                        "the expected number of arguments does not match the given number\n"
                        "expected: at least %zu\n"
                        "given: %zu\n", procedure->contents.procedure.name, arity, operands_count);
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
            long long int int_val;
            double double_val;
        } contents;
        bool is_int;
    } pre, cur;
    
    if (strchr(pre_number->contents.literal.value, '.') == NULL)
    {
        pre.contents.int_val = *(long long int *)(pre_number->contents.literal.c_native_value);
        pre.is_int = true;
    }
    else
    {
        pre.contents.double_val = *(double *)(pre_number->contents.literal.c_native_value);
        pre.is_int = false;
    }

    Boolean_Type *result = malloc(sizeof(Boolean_Type));
    *result = R_TRUE; // true by default.

    for (size_t i = 1; i < VectorLength(operands); i++)
    {
        AST_Node *cur_number = *(AST_Node **)VectorNth(operands, i); 

        if (cur_number->type != Number_Literal)
        {
            fprintf(stderr, "#<procedure:%s>: operands must be number\n", procedure->contents.procedure.name);
            exit(EXIT_FAILURE); 
        }

        if (strchr(cur_number->contents.literal.value, '.') == NULL)
        {
            cur.contents.int_val = *(long long int *)(cur_number->contents.literal.c_native_value);
            cur.is_int = true;
        }
        else
        {
            cur.contents.double_val = *(double *)(cur_number->contents.literal.c_native_value);
            cur.is_int = false;
        }

        if (cur.is_int == true && pre.is_int == true)
        {
            if ((cur.contents.int_val > pre.contents.int_val) || (cur.contents.int_val == pre.contents.int_val)) *result = R_FALSE;
            pre.contents.int_val = cur.contents.int_val;
        }
        else if (cur.is_int == true && pre.is_int == false)
        {
            if ((cur.contents.int_val > pre.contents.double_val) || (cur.contents.int_val == pre.contents.double_val)) *result = R_FALSE;
            pre.contents.int_val = cur.contents.int_val;
        }
        else if (cur.is_int == false && pre.is_int == true)
        {
            if ((cur.contents.double_val > pre.contents.int_val) || (cur.contents.double_val == pre.contents.int_val)) *result = R_FALSE;
            pre.contents.double_val = cur.contents.double_val;
        }
        else if (cur.is_int == false && pre.is_int == false)
        {
            if ((cur.contents.double_val > pre.contents.double_val) || (cur.contents.double_val == pre.contents.double_val)) *result = R_FALSE;
            pre.contents.double_val = cur.contents.double_val;
        }

        pre.is_int = cur.is_int;
    }

    AST_Node *ast_node = ast_node_new(NOT_IN_AST, Boolean_Literal, result);
    return ast_node;
}

// (< x y ...) -> boolean?
static AST_Node *racket_native_number_less_than(AST_Node *procedure, Vector *operands)
{
    // check arity
    size_t arity = procedure->contents.procedure.required_params_count;
    size_t operands_count = VectorLength(operands);
    if (operands_count < arity)
    {
        fprintf(stderr, "%s: arity mismatch;\n"
                        "the expected number of arguments does not match the given number\n"
                        "expected: at least %zu\n"
                        "given: %zu\n", procedure->contents.procedure.name, arity, operands_count);
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
            long long int int_val;
            double double_val;
        } contents;
        bool is_int;
    } pre, cur;
    
    if (strchr(pre_number->contents.literal.value, '.') == NULL)
    {
        pre.contents.int_val = *(long long int *)(pre_number->contents.literal.c_native_value);
        pre.is_int = true;
    }
    else
    {
        pre.contents.double_val = *(double *)(pre_number->contents.literal.c_native_value);
        pre.is_int = false;
    }

    Boolean_Type *result = malloc(sizeof(Boolean_Type));
    *result = R_TRUE; // true by default.

    for (size_t i = 1; i < VectorLength(operands); i++)
    {
        AST_Node *cur_number = *(AST_Node **)VectorNth(operands, i); 

        if (cur_number->type != Number_Literal)
        {
            fprintf(stderr, "#<procedure:%s>: operands must be number\n", procedure->contents.procedure.name);
            exit(EXIT_FAILURE); 
        }

        if (strchr(cur_number->contents.literal.value, '.') == NULL)
        {
            cur.contents.int_val = *(long long int *)(cur_number->contents.literal.c_native_value);
            cur.is_int = true;
        }
        else
        {
            cur.contents.double_val = *(double *)(cur_number->contents.literal.c_native_value);
            cur.is_int = false;
        }

        if (cur.is_int == true && pre.is_int == true)
        {
            if ((cur.contents.int_val < pre.contents.int_val) || (cur.contents.int_val == pre.contents.int_val)) *result = R_FALSE;
            pre.contents.int_val = cur.contents.int_val;
        }
        else if (cur.is_int == true && pre.is_int == false)
        {
            if ((cur.contents.int_val < pre.contents.double_val) || (cur.contents.int_val == pre.contents.double_val)) *result = R_FALSE;
            pre.contents.int_val = cur.contents.int_val;
        }
        else if (cur.is_int == false && pre.is_int == true)
        {
            if ((cur.contents.double_val < pre.contents.int_val) || (cur.contents.double_val == pre.contents.int_val)) *result = R_FALSE;
            pre.contents.double_val = cur.contents.double_val;
        }
        else if (cur.is_int == false && pre.is_int == false)
        {
            if ((cur.contents.double_val < pre.contents.double_val) || (cur.contents.double_val == pre.contents.double_val)) *result = R_FALSE;
            pre.contents.double_val = cur.contents.double_val;
        }

        pre.is_int = cur.is_int;
    }

    AST_Node *ast_node = ast_node_new(NOT_IN_AST, Boolean_Literal, result);
    return ast_node;
}

// (pair? v) -> boolean?
static AST_Node *racket_native_is_pair(AST_Node *procedure, Vector *operands)
{
    // check arity
    size_t arity = procedure->contents.procedure.required_params_count;
    size_t operands_count = VectorLength(operands);
    if (operands_count != arity)
    {
        fprintf(stderr, "%s: arity mismatch;\n"
                        "the expected number of arguments does not match the given number\n"
                        "expected: %zu\n"
                        "given: %zu\n", procedure->contents.procedure.name, arity, operands_count);
        exit(EXIT_FAILURE); 
    }

    AST_Node *v = *(AST_Node **)VectorNth(operands, 0);
    Boolean_Type *value = malloc(sizeof(Boolean_Type));

    if (v->type == Pair_Literal)
        *value = R_TRUE;
    else if (v->type == List_Literal && VectorLength((Vector *)(v->contents.literal.value)) != 0)
        *value = R_TRUE;
    else
        *value = R_FALSE;

    AST_Node *ast_node = ast_node_new(NOT_IN_AST, Boolean_Literal, value);
    return ast_node;
}

// (list v ...) -> list?
static AST_Node *racket_native_list(AST_Node *procedure, Vector *operands)
{
    // check arity
    size_t arity = procedure->contents.procedure.required_params_count;
    size_t operands_count = VectorLength(operands);
    if (operands_count < arity)
    {
        fprintf(stderr, "%s: arity mismatch;\n"
                        "the expected number of arguments does not match the given number\n"
                        "expected: %zu\n"
                        "given: %zu\n", procedure->contents.procedure.name, arity, operands_count);
        exit(EXIT_FAILURE); 
    }

    Vector *value = VectorNew(sizeof(AST_Node *));

    for (size_t i = 0; i < operands_count; i++)
    {
        AST_Node *node = *(AST_Node **)VectorNth(operands, i);
        AST_Node *node_copy = ast_node_deep_copy(node, NULL);
        VectorAppend(value, &node_copy);
    }

    AST_Node *ast_node = ast_node_new(NOT_IN_AST, List_Literal, value);
    return ast_node;
}

// (car pair) -> any/c
static AST_Node *racket_native_car(AST_Node *procedure, Vector *operands)
{
    // check arity
    size_t arity = procedure->contents.procedure.required_params_count;
    size_t operands_count = VectorLength(operands);
    if (operands_count != arity)
    {
        fprintf(stderr, "%s: arity mismatch;\n"
                        "the expected number of arguments does not match the given number\n"
                        "expected: %zu\n"
                        "given: %zu\n", procedure->contents.procedure.name, arity, operands_count);
        exit(EXIT_FAILURE); 
    }

    // check if it is a pair
    AST_Node *ast_node = *(AST_Node **)VectorNth(operands, 0);
    bool is_pair = false;

    if (ast_node->type == Pair_Literal)
        is_pair = true;
    else if (ast_node->type == List_Literal && VectorLength((Vector *)(ast_node->contents.literal.value)) != 0)
        is_pair = true;
    else
        is_pair = false;

    if (is_pair == false)
    {
        fprintf(stderr, "%s: contract violation\n"
                        "expected: pair?\n"
                        "given: %zu\n", procedure->contents.procedure.name, operands_count);
        exit(EXIT_FAILURE); 
    }

    Vector *value = (Vector *)(ast_node->contents.literal.value);
    AST_Node *car = *(AST_Node **)VectorNth(value, 0);
    return ast_node_deep_copy(car, NULL);
}

// (cdr pair) -> any/c
static AST_Node *racket_native_cdr(AST_Node *procedure, Vector *operands)
{
    // check arity
    size_t arity = procedure->contents.procedure.required_params_count;
    size_t operands_count = VectorLength(operands);
    if (operands_count != arity)
    {
        fprintf(stderr, "%s: arity mismatch;\n"
                        "the expected number of arguments does not match the given number\n"
                        "expected: %zu\n"
                        "given: %zu\n", procedure->contents.procedure.name, arity, operands_count);
        exit(EXIT_FAILURE); 
    }
    
    // check if it is a pair
    AST_Node *ast_node = *(AST_Node **)VectorNth(operands, 0);
    bool is_pair = false;

    if (ast_node->type == Pair_Literal)
        is_pair = true;
    else if (ast_node->type == List_Literal && VectorLength((Vector *)(ast_node->contents.literal.value)) != 0)
        is_pair = true;
    else
        is_pair = false;

    if (is_pair == false)
    {
        fprintf(stderr, "%s: contract violation\n"
                        "expected: pair?\n"
                        "given: %zu\n", procedure->contents.procedure.name, operands_count);
        exit(EXIT_FAILURE);
    }

    Vector *value = (Vector *)(ast_node->contents.literal.value);

    if (ast_node->type == Pair_Literal)
    {
        AST_Node *cdr = *(AST_Node **)VectorNth(value, 1);
        return ast_node_deep_copy(cdr, NULL);
    }
    else if (ast_node->type == List_Literal)
    {
        Vector *list = VectorNew(sizeof(AST_Node *));

        for (size_t i = 1; i < VectorLength(value); i++)
        {
            AST_Node *node = *(AST_Node **)VectorNth(value, i);
            AST_Node *node_copy = ast_node_deep_copy(node, NULL);
            VectorAppend(list, &node_copy);
        }

        AST_Node *cdr = ast_node_new(NOT_IN_AST, List_Literal, list);
        return cdr;
    }
    else
    {
        fprintf(stderr, "something wrong in racket_native_cdr\n");
        exit(EXIT_FAILURE);
    }
}

static AST_Node *racket_native_cons(AST_Node *procedure, Vector *operands)
{
    // check arity
    size_t arity = procedure->contents.procedure.required_params_count;
    size_t operands_count = VectorLength(operands);
    if (operands_count != arity)
    {
        fprintf(stderr, "%s: arity mismatch;\n"
                        "the expected number of arguments does not match the given number\n"
                        "expected: %zu\n"
                        "given: %zu\n", procedure->contents.procedure.name, arity, operands_count);
        exit(EXIT_FAILURE); 
    }

    AST_Node *car = *(AST_Node **)VectorNth(operands, 0);
    AST_Node *cdr = *(AST_Node **)VectorNth(operands, 1);
    car = ast_node_deep_copy(car, NULL);
    car->tag = NOT_IN_AST;
    cdr = ast_node_deep_copy(cdr, NULL);
    cdr->tag = NOT_IN_AST;
    
    if (cdr->type == List_Literal)
    {
        Vector *value = VectorNew(sizeof(AST_Node *));
        Vector *cdr_value = (Vector *)(cdr->contents.literal.value);

        VectorAppend(value, &car);
        for (size_t i = 0; i < VectorLength(cdr_value); i++)
        {
            AST_Node *node = *(AST_Node **)VectorNth(cdr_value, i);
            VectorAppend(value, &node);
        }

        AST_Node *ast_node = ast_node_new(NOT_IN_AST, List_Literal, value);
        return ast_node;
    }
    else
    {
        Vector *value = VectorNew(sizeof(AST_Node *));

        VectorAppend(value, &car);
        VectorAppend(value, &cdr);
        
        AST_Node *ast_node = ast_node_new(NOT_IN_AST, Pair_Literal, value);
        return ast_node;
    }
}

// (<= x y ...) -> boolean?
static AST_Node *racket_native_less_or_equal_than(AST_Node *procedure, Vector *operands)
{
    // check arity
    size_t arity = procedure->contents.procedure.required_params_count;
    size_t operands_count = VectorLength(operands);
    if (operands_count < arity)
    {
        fprintf(stderr, "%s: arity mismatch;\n"
                        "the expected number of arguments does not match the given number\n"
                        "expected: at least %zu\n"
                        "given: %zu\n", procedure->contents.procedure.name, arity, operands_count);
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
            long long int int_val;
            double double_val;
        } contents;
        bool is_int;
    } pre, cur;
    
    if (strchr(pre_number->contents.literal.value, '.') == NULL)
    {
        pre.contents.int_val = *(long long int *)(pre_number->contents.literal.c_native_value);
        pre.is_int = true;
    }
    else
    {
        pre.contents.double_val = *(double *)(pre_number->contents.literal.c_native_value);
        pre.is_int = false;
    }

    Boolean_Type *result = malloc(sizeof(Boolean_Type));
    *result = R_TRUE; // true by default.

    for (size_t i = 1; i < VectorLength(operands); i++)
    {
        AST_Node *cur_number = *(AST_Node **)VectorNth(operands, i); 

        if (cur_number->type != Number_Literal)
        {
            fprintf(stderr, "#<procedure:%s>: operands must be number\n", procedure->contents.procedure.name);
            exit(EXIT_FAILURE); 
        }

        if (strchr(cur_number->contents.literal.value, '.') == NULL)
        {
            cur.contents.int_val = *(long long int *)(cur_number->contents.literal.c_native_value);
            cur.is_int = true;
        }
        else
        {
            cur.contents.double_val = *(double *)(cur_number->contents.literal.c_native_value);
            cur.is_int = false;
        }

        if (cur.is_int == true && pre.is_int == true)
        {
            if (cur.contents.int_val < pre.contents.int_val) *result = R_FALSE;
            pre.contents.int_val = cur.contents.int_val;
        }
        else if (cur.is_int == true && pre.is_int == false)
        {
            if (cur.contents.int_val < pre.contents.double_val) *result = R_FALSE;
            pre.contents.int_val = cur.contents.int_val;
        }
        else if (cur.is_int == false && pre.is_int == true)
        {
            if (cur.contents.double_val < pre.contents.int_val) *result = R_FALSE;
            pre.contents.double_val = cur.contents.double_val;
        }
        else if (cur.is_int == false && pre.is_int == false)
        {
            if (cur.contents.double_val < pre.contents.double_val) *result = R_FALSE;
            pre.contents.double_val = cur.contents.double_val;
        }

        pre.is_int = cur.is_int;
    }

    AST_Node *ast_node = ast_node_new(NOT_IN_AST, Boolean_Literal, result);
    return ast_node;
}

// (>= x y ...) -> boolean?
static AST_Node *racket_native_more_or_equal_than(AST_Node *procedure, Vector *operands)
{
    // check arity
    size_t arity = procedure->contents.procedure.required_params_count;
    size_t operands_count = VectorLength(operands);
    if (operands_count < arity)
    {
        fprintf(stderr, "%s: arity mismatch;\n"
                        "the expected number of arguments does not match the given number\n"
                        "expected: at least %zu\n"
                        "given: %zu\n", procedure->contents.procedure.name, arity, operands_count);
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
            long long int int_val;
            double double_val;
        } contents;
        bool is_int;
    } pre, cur;
    
    if (strchr(pre_number->contents.literal.value, '.') == NULL)
    {
        pre.contents.int_val = *(long long int *)(pre_number->contents.literal.c_native_value);
        pre.is_int = true;
    }
    else
    {
        pre.contents.double_val = *(double *)(pre_number->contents.literal.c_native_value);
        pre.is_int = false;
    }

    Boolean_Type *result = malloc(sizeof(Boolean_Type));
    *result = R_TRUE; // true by default.

    for (size_t i = 1; i < VectorLength(operands); i++)
    {
        AST_Node *cur_number = *(AST_Node **)VectorNth(operands, i); 

        if (cur_number->type != Number_Literal)
        {
            fprintf(stderr, "#<procedure:%s>: operands must be number\n", procedure->contents.procedure.name);
            exit(EXIT_FAILURE); 
        }

        if (strchr(cur_number->contents.literal.value, '.') == NULL)
        {
            cur.contents.int_val = *(long long int *)(cur_number->contents.literal.c_native_value);
            cur.is_int = true;
        }
        else
        {
            cur.contents.double_val = *(double *)(cur_number->contents.literal.c_native_value);
            cur.is_int = false;
        }

        if (cur.is_int == true && pre.is_int == true)
        {
            if (cur.contents.int_val > pre.contents.int_val) *result = R_FALSE;
            pre.contents.int_val = cur.contents.int_val;
        }
        else if (cur.is_int == true && pre.is_int == false)
        {
            if (cur.contents.int_val > pre.contents.double_val) *result = R_FALSE;
            pre.contents.int_val = cur.contents.int_val;
        }
        else if (cur.is_int == false && pre.is_int == true)
        {
            if (cur.contents.double_val > pre.contents.int_val) *result = R_FALSE;
            pre.contents.double_val = cur.contents.double_val;
        }
        else if (cur.is_int == false && pre.is_int == false)
        {
            if (cur.contents.double_val > pre.contents.double_val) *result = R_FALSE;
            pre.contents.double_val = cur.contents.double_val;
        }

        pre.is_int = cur.is_int;
    }

    AST_Node *ast_node = ast_node_new(NOT_IN_AST, Boolean_Literal, result);
    return ast_node;
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

    procedure = ast_node_new(BUILT_IN_PROCEDURE, Procedure, "=", 1, NULL, NULL, (void(*)(void))racket_native_number_equal); 
    binding = ast_node_new(BUILT_IN_BINDING, Binding, "=", procedure);
    VectorAppend(built_in_bindings, &binding);

    procedure = ast_node_new(BUILT_IN_PROCEDURE, Procedure, ">", 1, NULL, NULL, (void(*)(void))racket_native_number_more_than); 
    binding = ast_node_new(BUILT_IN_BINDING, Binding, ">", procedure);
    VectorAppend(built_in_bindings, &binding);

    procedure = ast_node_new(BUILT_IN_PROCEDURE, Procedure, "<", 1, NULL, NULL, (void(*)(void))racket_native_number_less_than); 
    binding = ast_node_new(BUILT_IN_BINDING, Binding, "<", procedure);
    VectorAppend(built_in_bindings, &binding);

    procedure = ast_node_new(BUILT_IN_PROCEDURE, Procedure, "map", 2, NULL, NULL, (void(*)(void))racket_native_map); 
    binding = ast_node_new(BUILT_IN_BINDING, Binding, "map", procedure);
    VectorAppend(built_in_bindings, &binding);

    procedure = ast_node_new(BUILT_IN_PROCEDURE, Procedure, "filter", 2, NULL, NULL, (void(*)(void))racket_native_filter); 
    binding = ast_node_new(BUILT_IN_BINDING, Binding, "filter", procedure);
    VectorAppend(built_in_bindings, &binding);

    procedure = ast_node_new(BUILT_IN_PROCEDURE, Procedure, "list?", 1, NULL, NULL, (void(*)(void))racket_native_is_list); 
    binding = ast_node_new(BUILT_IN_BINDING, Binding, "list?", procedure);
    VectorAppend(built_in_bindings, &binding);

    procedure = ast_node_new(BUILT_IN_PROCEDURE, Procedure, "pair?", 1, NULL, NULL, (void(*)(void))racket_native_is_pair); 
    binding = ast_node_new(BUILT_IN_BINDING, Binding, "pair?", procedure);
    VectorAppend(built_in_bindings, &binding);

    procedure = ast_node_new(BUILT_IN_PROCEDURE, Procedure, "car", 1, NULL, NULL, (void(*)(void))racket_native_car); 
    binding = ast_node_new(BUILT_IN_BINDING, Binding, "car", procedure);
    VectorAppend(built_in_bindings, &binding);

    procedure = ast_node_new(BUILT_IN_PROCEDURE, Procedure, "cdr", 1, NULL, NULL, (void(*)(void))racket_native_cdr); 
    binding = ast_node_new(BUILT_IN_BINDING, Binding, "cdr", procedure);
    VectorAppend(built_in_bindings, &binding);

    procedure = ast_node_new(BUILT_IN_PROCEDURE, Procedure, "list", 0, NULL, NULL, (void(*)(void))racket_native_list); 
    binding = ast_node_new(BUILT_IN_BINDING, Binding, "list", procedure);
    VectorAppend(built_in_bindings, &binding);

    procedure = ast_node_new(BUILT_IN_PROCEDURE, Procedure, "cons", 2, NULL, NULL, (void(*)(void))racket_native_cons); 
    binding = ast_node_new(BUILT_IN_BINDING, Binding, "cons", procedure);
    VectorAppend(built_in_bindings, &binding);

    procedure = ast_node_new(BUILT_IN_PROCEDURE, Procedure, "<=", 1, NULL, NULL, (void(*)(void))racket_native_less_or_equal_than); 
    binding = ast_node_new(BUILT_IN_BINDING, Binding, "<=", procedure);
    VectorAppend(built_in_bindings, &binding);

    procedure = ast_node_new(BUILT_IN_PROCEDURE, Procedure, ">=", 1, NULL, NULL, (void(*)(void))racket_native_more_or_equal_than); 
    binding = ast_node_new(BUILT_IN_BINDING, Binding, ">=", procedure);
    VectorAppend(built_in_bindings, &binding);

    return built_in_bindings;
}

int free_built_in_bindings(Vector *built_in_bindings, VectorFreeFunction free_fn)
{
    return VectorFree(built_in_bindings, free_fn, NULL);
}