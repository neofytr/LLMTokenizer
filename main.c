#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdint.h>

#include "dyn_arr/inc/dyn_arr.h"
#include "hash_table/inc/hash_table.h"

bool is_less(const DATA a, const DATA b)
{
    return ((node_t *)a)->value < ((node_t *)b)->value;
}

int main()
{
    // taken from wikipedia article for BPE
    const char *text = "The original BPE algorithm operates by iteratively replacing the most common contiguous sequences of characters in a target text with unused 'placeholder' bytes. The iteration ends when no sequences can be found, leaving the target text effectively compressed. Decompression can be performed by reversing this process, querying known placeholder terms against their corresponding denoted sequence, using a lookup table. In the original paper, this lookup table is encoded and stored alongside the compressed text.";
    int text_size = strlen(text);

    hash_table_t *table = hash_table_create(1024);
    if (!table)
    {
        perror("hash_table_create");
        return EXIT_FAILURE;
    }

    for (int counter = 0; counter < text_size - 1; counter++) // prevent underflow for zero text_size by using a signed iterator
    {
        char key[3] = {text[counter], text[counter + 1], 0};

        size_t count = 0;

        // count remains unchanged when key is not found, so, this works
        hash_table_search(table, key, &count);
        if (!hash_table_insert(table, key, ++count))
        {
            perror("hash_table_insert");
            hash_table_destroy(table);
            return EXIT_FAILURE;
        }
    }

    dyn_arr_t *arr = dyn_arr_create(0);
    int index = 0;

    for (size_t counter = 0; counter < table->size; counter++)
    {
        node_t *curr = table->buckets[counter];
        while (curr)
        {
            if (!dyn_arr_set(arr, index++, (void *)curr))
            {
                dyn_arr_free(arr);
                hash_table_destroy(table);
                perror("dyn_arr_set");
                return EXIT_FAILURE;
            }
            curr = curr->next;
        }
    }

    for (size_t counter = 0; counter < index; counter++)
    {
        node_t *node = (node_t *)dyn_arr_get(arr, counter);
        printf("%s => %ld\n", node->key, node->value);
    }

    if (!dyn_arr_sort(arr, 0, index, is_less))
    {
        dyn_arr_free(arr);
        hash_table_destroy(table);
        perror("dyn_arr_sort");
        return EXIT_FAILURE;
    }

    for (size_t counter = 0; counter < index; counter++)
    {
        node_t *node = (node_t *)dyn_arr_get(arr, counter);
        printf("%s => %ld\n", node->key, node->value);
    }

    return EXIT_SUCCESS;
}