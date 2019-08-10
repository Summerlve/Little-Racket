#ifndef VECTOR
#define VECTOR

typedef struct _z_vector {
    void *elems; // created by malloc, needs to be released.
    int elem_size;
    int logicl_length;
    int allocated_length;
} Vector;
typedef void (*VectorFreeFunction)(void *value_addr, int index, Vector *vector, void *aux_data);
typedef void (*VectorMapFunction)(void *value_addr, int index, Vector *vector, void *aux_data);
typedef void *(*VectorCopyFunction)(void *value_addr, int index, Vector *original_vector, Vector *new_vector, void *aux_data);
Vector *VectorNew(int elem_size);
int VectorLength(Vector *v);
void *VectorNth(Vector *v, int index);
void VectorAppend(Vector *v, const void *value_addr);
Vector *VectorCopy(Vector *v, VectorCopyFunction copy_fn, void *aux_data);
void VectorMap(Vector *v, VectorMapFunction map, void *aux_data);
int VectorFree(Vector *v, VectorFreeFunction free_fn, void *aux_data);

#endif