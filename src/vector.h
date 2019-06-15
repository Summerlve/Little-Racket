#ifndef VECTOR
#define VECTOR

typedef struct _z_vector {
    void *elems; // created by malloc, needs to be released.
    int elem_size;
    int logicl_length;
    int allocated_length;
} Vector;
typedef void (*VectorFreeFunction)(void *v, void *aux_data);
typedef void (*VectorMapFunction)(void *value_addr, void *aux_data);
Vector *VectorNew(int elem_size);
int VectorLength(Vector *v);
void *VectorNth(Vector *v, int index);
void VectorAppend(Vector *v, const void *value_addr);
void VectorMap(Vector *v, VectorMapFunction map, void *aux_data);
void VectorFree(Vector *v, VectorFreeFunction free_fn, void *aux_data);

#endif