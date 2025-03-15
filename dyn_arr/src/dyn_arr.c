#include "../inc/dyn_arr.h"

#include <math.h>

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

    // allocate the min number of nodes
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

bool dyn_arr_insert(dyn_arr_t *dyn_arr, size_t index, DATA item)
{
    if (!dyn_arr)
        return false;

    size_t node_index = index & (MAX_NODE_SIZE - 1);
    size_t node_no = index / MAX_NODE_SIZE;

    if (node_no >= dyn_arr->len)
    {
        // node number out of bounds, expand the node array
        size_t new_len = 1U << ((size_t)log2(node_no) + 1U);

        void *temp = realloc(dyn_arr->nodes, new_len * sizeof(DATA *));
        if (!temp)
            return false;
        dyn_arr->nodes = temp;

        // fill the resized node array with NULL pointers
        for (size_t i = dyn_arr->len; i < new_len; i++)
            dyn_arr->nodes[i] = NULL;

        dyn_arr->len = new_len;
    }

    if (!dyn_arr->nodes[node_no])
    {
        // if the node ptr is NULL, allocate it
        dyn_arr->nodes[node_no] = malloc(MAX_NODE_SIZE * sizeof(DATA));
        if (!dyn_arr->nodes[node_no])
            return false;
    }

    dyn_arr->nodes[node_no][node_index] = item;
    return true;
}

DATA dyn_arr_get(dyn_arr_t *dyn_arr, size_t index)
{
    size_t node_no = index / MAX_NODE_SIZE;
    size_t node_index = index & (MAX_NODE_SIZE - 1);

    // this return will cause segfault if the array index is out of bounds
    return dyn_arr->nodes[node_no][node_index];
}