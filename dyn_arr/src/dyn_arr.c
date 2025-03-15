#include "../inc/dyn_arr.h"

#include <math.h>

bool dyn_arr_sort(dyn_arr_t *dyn_arr, size_t start_index, size_t end_index)
{
    // check if array is empty or if indices are invalid
    if (!dyn_arr || start_index > end_index)
    {
        return false;
    }

    if (start_index == end_index)
    {
        // one length array is already sorted by definition
        return true;
    }

    if (start_index < end_index)
    {
        size_t mid = start_index + (end_index - start_index) / 2;

        // divide the array into half and recursively sort the halves
        if (!dyn_arr_sort(dyn_arr, start_index, mid))
        {
            return false;
        }
        if (!dyn_arr_sort(dyn_arr, mid + 1, end_index))
        {
            return false;
        }

        // store the sorted halves into temporary arrays
        size_t left_len = mid - start_index + 1;
        size_t right_len = end_index - mid;

        DATA *left_temp = (DATA *)malloc(sizeof(DATA) * left_len);
        if (!left_temp)
        {
            return false;
        }
        DATA *right_temp = (DATA *)malloc(sizeof(DATA) * right_len);
        if (!right_temp)
        {
            free(left_temp);
            return false;
        }

        for (size_t counter = 0; counter < left_len; counter++)
        {
            left_temp[counter] = dyn_arr_get(dyn_arr, start_index + counter);
        }

        for (size_t counter = 0; counter < right_len; counter++)
        {
            right_temp[counter] = dyn_arr_get(dyn_arr, mid + 1 + counter);
        }

        // merge the sorted halves
        size_t left_index = 0;
        size_t right_index = 0;
        size_t main_index = start_index;

        while (left_index < left_len && right_index < right_len)
        {
            if (left_temp[left_index] < right_temp[right_index])
            {
                if (!dyn_arr_set(dyn_arr, main_index++, left_temp[left_index++]))
                {
                    free(left_temp);
                    free(right_temp);
                    return false;
                }
            }
            else
            {
                if (!dyn_arr_set(dyn_arr, main_index++, right_temp[right_index++]))
                {
                    free(left_temp);
                    free(right_temp);
                    return false;
                }
            }
        }

        while (left_index < left_len)
        {
            if (!dyn_arr_set(dyn_arr, main_index++, left_temp[left_index++]))
            {
                free(left_temp);
                free(right_temp);
                return false;
            }
        }

        while (right_index < right_len)
        {
            if (!dyn_arr_set(dyn_arr, main_index++, right_temp[right_index++]))
            {
                free(left_temp);
                free(right_temp);
                return false;
            }
        }

        free(left_temp);
        free(right_temp);
    }

    return true;
}

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

bool dyn_arr_set(dyn_arr_t *dyn_arr, size_t index, DATA item)
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
