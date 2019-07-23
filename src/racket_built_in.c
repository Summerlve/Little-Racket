#include "./racket_built_in.h"
#include "./interpreter.h"
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <stdarg.h>
#include <string.h>

#define DOUBLE_MAX_DIGIT_LENGTH 1024

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

AST_Node *racket_native_plus(AST_Node *procedure, Vector *operands)
{
    // check arity
    int arity = procedure->contents.procedure.required_params_count;
    int operands_count = VectorLength(operands);
    if (operands_count < arity)
    {
        fprintf(stderr, "%s: arity mismatch;"
                        "the expected number of arguments does not match the given number"
                        "given: %d", procedure->contents.procedure.name, operands_count);
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
    
    AST_Node *ast_node = ast_node_new(Number_Literal, MANUAL_FREE, value);
    free(value);
    return ast_node;
}

Vector *generate_built_in_bindings(void)
{
    Vector *built_in_bindings = VectorNew(sizeof(AST_Node *));
    AST_Node *binding = NULL;
    AST_Node *procedure = NULL;

    procedure = ast_node_new(Procedure, AUTO_FREE, "+", 0, NULL, NULL, (void(*)(void))racket_native_plus); 
    binding = ast_node_new(Binding, AUTO_FREE, "+", procedure);
    VectorAppend(built_in_bindings, &binding);

    return built_in_bindings;
}

int free_built_in_bindings(Vector *built_in_bindings, VectorFreeFunction free_fn)
{
    return VectorFree(built_in_bindings, free_fn, NULL);
}