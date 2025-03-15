#include <stdlib.h>
#include <stdbool.h>

#define DATA int
#define MAX_NODE_SIZE (1U << 8)

typedef struct
{
    size_t len;
    DATA **nodes;
} dyn_arr_t;

typedef bool (*dyn_isLessThan)(const DATA *a, const DATA *b); // checks whether a < b

dyn_arr_t *dyn_arr_create(size_t min_size);
void dyn_arr_free(dyn_arr_t *dyn_arr);

bool dyn_arr_set(dyn_arr_t *dyn_arr, size_t index, DATA item);
DATA dyn_arr_get(dyn_arr_t *dyn_arr, size_t index);

bool dyn_arr_sort(dyn_arr_t *dyn_arr, size_t start_index, size_t end_index, dyn_isLessThan is_less); // supply dyn_isLessThan before using this
