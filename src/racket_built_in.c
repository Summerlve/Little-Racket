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

AST_Node *racket_native_map(AST_Node *procedure, Vector *operands)
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

    procedure = ast_node_new(BUILT_IN_PROCEDURE, Procedure, "map", 2, NULL, NULL, (void(*)(void))racket_native_map); 
    binding = ast_node_new(BUILT_IN_BINDING, Binding, "map", procedure);
    VectorAppend(built_in_bindings, &binding);

    return built_in_bindings;
}

int free_built_in_bindings(Vector *built_in_bindings, VectorFreeFunction free_fn)
{
    return VectorFree(built_in_bindings, free_fn, NULL);
}