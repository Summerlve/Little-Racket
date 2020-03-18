#ifndef VECTOR
#define VECTOR

#include <stddef.h>

typedef struct _z_vector {
    void *elems; // created by malloc, needs to be released.
    size_t elem_size;
    size_t logicl_length;
    size_t allocated_length;
} Vector;
typedef void (*VectorFreeFunction)(void *value_addr, size_t index, Vector *vector, void *aux_data);
typedef void (*VectorMapFunction)(void *value_addr, size_t index, Vector *vector, void *aux_data);
typedef void *(*VectorCopyFunction)(void *value_addr, size_t index, Vector *original_vector, Vector *new_vector, void *aux_data);
Vector *VectorNew(size_t elem_size);
int VectorFree(Vector *v, VectorFreeFunction free_fn, void *aux_data);
size_t VectorLength(Vector *v);
void *VectorNth(Vector *v, size_t index);
void VectorMap(Vector *v, VectorMapFunction map, void *aux_data);
void VectorAppend(Vector *v, const void *value_addr);
Vector *VectorCopy(Vector *v, VectorCopyFunction copy_fn, void *aux_data);

#endif