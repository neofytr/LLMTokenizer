#include <stdlib.h>

#define DATA int
#define MAX_NODE_SIZE 256

typedef struct
{
    size_t len;
    DATA **nodes;
} dyn_arr_t;

dyn_arr_t *dyn_arr_create(size_t min_size);
void dyn_arr_free(dyn_arr_t *dyn_arr);

bool dyn_arr_insert(dyn_arr_t *dyn_arr, size_t index, DATA item);
DATA dyn_arr_get(dyn_arr_t *dyn_arr, size_t index);
