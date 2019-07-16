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

AST_Node *racket_native_plus(int n, Vector *operands)
{
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
    
    for (int i = 0; i < n; i++)
    {
        AST_Node *operand = *(AST_Node **)VectorNth(operands, i);
        if (strchr(operand->contents.literal.value, '.') == NULL)
        {
            int c_native_value = *(int *)(operand->contents.literal.c_native_value);
            result.value.iv += c_native_value;
        }
        else
        {
            if (result.is_int == true)
            {
                double c_native_value = *(double *)(operand->contents.literal.c_native_value);
                result.is_int = false;
                double tmp = (double)result.value.iv;
                result.value.dv = tmp + c_native_value;
            }
            else
            {
                double c_native_value = *(double *)(operand->contents.literal.c_native_value);
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
        sprintf(value, "%f", result.value.dv);
    }
    
    AST_Node *ast_node = ast_node_new(Number_Literal, value);
    free(value);
    return ast_node;
}

Vector *generate_built_in_bindings(void)
{
    Vector *built_in_context = VectorNew(sizeof(AST_Node *));
    AST_Node *tmp = ast_node_new(Procedure); 
    return built_in_context;
}

int free_built_in_bindings(Vector *built_in_bindings)
{
    return 0;
}
