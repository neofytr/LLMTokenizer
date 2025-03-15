#include "dyn_arr.h"

dyn_arr_t *dyn_arr_create(size_t min_size)
{
    dyn_arr_t *dyn_arr = (dyn_arr_t *)malloc(sizeof(dyn_arr_t));
    if (!dyn_arr)
    {
        return NULL;
    }

    if (!min_size)
    {
        return dyn_arr;
    }

    size_t num_of_nodes = min_size / MAX_NODE_SIZE + 1;
    DATA **nodes = (DATA **)malloc(sizeof(DATA *) * num_of_nodes);
    if (!nodes)
    {
        free(dyn_arr);
        return NULL;
    }

    for (int counter = 0; counter < num_of_nodes; counter++)
    {
        nodes[counter] = (DATA *)malloc(MAX_NODE_SIZE * sizeof(DATA));
        if (!nodes[counter])
        {
            free(dyn_arr);
            for (int index = 0; index < counter; index++)
            {
                free(nodes[index]);
            }
            return NULL;
        }
    }

    dyn_arr->len = num_of_nodes;
    dyn_arr->nodes = nodes;

    return dyn_arr;
}

void dyn_arr_free(dyn_arr_t *dyn_arr)
{
    if (!dyn_arr)
    {
        return;
    }

    for (size_t counter = 0; counter < dyn_arr->len; counter++)
    {
        free(dyn_arr->nodes[counter]);
    }

    free(dyn_arr);
}