#include <stdlib.h>
#include <stdbool.h>

#define DATA void *
#define MAX_NODE_SIZE (1U << 8)

typedef struct
{
    size_t len;
    DATA **nodes;
} dyn_arr_t;

typedef bool (*dyn_iss_less_than_t)(const DATA a, const DATA b); // checks whether a < b

// returning a < b will sort in ascending order; a > b will sort in descending order
typedef bool (*dyn_compare_t)(const DATA a, const DATA b);

dyn_arr_t *dyn_arr_create(size_t min_size);
void dyn_arr_free(dyn_arr_t *dyn_arr);

bool dyn_arr_set(dyn_arr_t *dyn_arr, size_t index, DATA item);
DATA dyn_arr_get(dyn_arr_t *dyn_arr, size_t index);

bool dyn_arr_sort(dyn_arr_t *dyn_arr, size_t start_index, size_t end_index, dyn_compare_t compare); // supply dyn_isLessThan before using this

DATA dyn_arr_max(dyn_arr_t *dyn_arr, size_t start_index, size_t end_index, dyn_iss_less_than_t is_less);
DATA dyn_arr_min(dyn_arr_t *dyn_arr, size_t start_index, size_t end_index, dyn_iss_less_than_t is_less);
