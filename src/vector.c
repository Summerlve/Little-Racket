#include "./vector.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

Vector *VectorNew(int elem_size)
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

int VectorLength(Vector *v)
{
    return v->logicl_length;
}

void *VectorNth(Vector *v, int index)
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
    int length = VectorLength(v);

    for (int i = 0; i < length; i++)
    {
        void *value_addr = VectorNth(v, i);
        map(value_addr, aux_data);
    }
}

int VectorFree(Vector *v, VectorFreeFunction free_fn, void *aux_data)
{
    if (v == NULL) return 1;
    
    if (free_fn != NULL)
    {
        int length = VectorLength(v);
        for (int i = 0; i < length; i++)
        {
            void *value_addr = VectorNth(v, i);
            free_fn(value_addr, aux_data);
        }
    }

    free(v->elems);
    free(v);
    return 0;
}