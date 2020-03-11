#include "../include/addon.h"
#include "../include/interpreter.h"
#include <sodium.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

#define SHA256_HASH_STRING_LEN ((size_t)64)

AST_Node *racket_addon_string_sha256(AST_Node *procedure, Vector *operands)
{
    // check arity
    size_t arity = procedure->contents.procedure.required_params_count;
    size_t operands_count = VectorLength(operands);
    if (operands_count != arity)
    {
        fprintf(stderr, "%s: arity mismatch;\n"
                        "the expected number of arguments does not match the given number\n"
                        "expected: at least %zu\n"
                        "given: %zu\n", procedure->contents.procedure.name, arity, operands_count);
        exit(EXIT_FAILURE); 
    }

    const AST_Node *operand = *(AST_Node **)VectorNth(operands, 0);

    if (operand->type != String_Literal)
    {
        fprintf(stderr, "#<procedure:%s>: operands must be string\n", procedure->contents.procedure.name);
        exit(EXIT_FAILURE);  
    }

    unsigned char *value = (unsigned char *)(operand->contents.literal.value);
    unsigned char hash[crypto_hash_sha256_BYTES];
    crypto_hash_sha256(hash, value, strlen((const char *)value));

    char tmp[3]; tmp[0] = '\0';
    char *result = malloc(SHA256_HASH_STRING_LEN + 1); 

    for(unsigned int i = 0; i < crypto_hash_sha256_BYTES; i++)
    {
        sprintf(tmp, "%02x", hash[i]);
        memcpy(&result[i * 2], tmp, 2);
    }

    result[SHA256_HASH_STRING_LEN] = '\0';

    AST_Node *ast_node = ast_node_new(NOT_IN_AST, String_Literal, result);
    return ast_node;
}

Vector *generate_addon_bindings(void)
{
    Vector *addon_bindings = VectorNew(sizeof(AST_Node *));
    AST_Node *binding = NULL;
    AST_Node *procedure = NULL;

    procedure = ast_node_new(ADDON_PROCEDURE, Procedure, "string-sha256", 1, NULL, NULL, (void(*)(void))racket_addon_string_sha256);
    binding = ast_node_new(ADDON_BINDING, Binding, "string-sha256", procedure);
    VectorAppend(addon_bindings, &binding);

    return addon_bindings;
}

int free_addon_bindings(Vector *addon_bindings, VectorFreeFunction free_fn)
{
    return VectorFree(addon_bindings, free_fn, NULL);
}