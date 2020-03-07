#include "../include/vector.h"
#include "../include/interpreter.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

Vector *VectorNew(size_t elem_size)
{
    Vector *v = (Vector *)malloc(sizeof(Vector));
    v->elem_size = elem_size;
    v->logicl_length = 0;
    v->allocated_length = 4;
    v->elems = malloc(v->elem_size * v->allocated_length);
    
    if (v->elems == NULL)
    {
        perror("Vector::elems malloc failed");
        exit(EXIT_FAILURE);
    }

    return v;
}

static void VectorExpand(Vector *v)
{
    v->allocated_length *= 2;
    v->elems = realloc(v->elems, v->elem_size * v->allocated_length);
    if (v->elems == NULL)
    {
        perror("Vector::elems realloc failed");
        exit(EXIT_FAILURE);
    }
}

size_t VectorLength(Vector *v)
{
    return v->logicl_length;
}

void *VectorNth(Vector *v, size_t index)
{
    return (char *)v->elems + v->elem_size * index;
}

void VectorAppend(Vector *v, const void *value_addr)
{
    if (v->logicl_length == v->allocated_length)
        VectorExpand(v);

    void *dest = VectorNth(v, VectorLength(v));
    memcpy(dest, value_addr, v->elem_size);
    v->logicl_length ++;
}

void VectorMap(Vector *v, VectorMapFunction map, void *aux_data)
{
    size_t length = VectorLength(v);

    for (size_t i = 0; i < length; i++)
    {
        void *value_addr = VectorNth(v, i);
        map(value_addr, i, v, aux_data);
    }
}

Vector *VectorCopy(Vector *v, VectorCopyFunction copy_fn, void *aux_data)
{
    Vector *new_vector = VectorNew(v->elem_size);
    size_t length = VectorLength(v);

    if (copy_fn != NULL)
    {
        for (size_t i = 0; i < length; i++)
        {
            void *value_addr = VectorNth(v, i);
            AST_Node *binding = *(AST_Node **)value_addr;
            void *copy_val_addr = copy_fn(value_addr, i, v, new_vector, aux_data);
            // you can return null to ignore some value.
            if (copy_val_addr != NULL) VectorAppend(new_vector, copy_val_addr);
        }
    }
    else if (copy_fn == NULL)
    {
        memcpy(new_vector, v, sizeof(Vector));
        size_t elems_size = v->allocated_length * v->elem_size;
        void *elems = malloc(elems_size);
        memcpy(elems, v->elems, elems_size);
        new_vector->elems = elems;
    }
   
    return new_vector;
}

int VectorFree(Vector *v, VectorFreeFunction free_fn, void *aux_data)
{
    if (v == NULL) return 1;
    
    if (free_fn != NULL)
    {
        size_t length = VectorLength(v);
        for (size_t i = 0; i < length; i++)
        {
            void *value_addr = VectorNth(v, i);
            free_fn(value_addr, i, v, aux_data);
        }
    }

    free(v->elems);
    free(v);
    return 0;
}