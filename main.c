#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdint.h>
#include <errno.h>

#include "./dyn_arr/inc/dyn_arr.h"
#include "hash_table/inc/hash_table.h"

typedef struct
{
    uint32_t a, b;
} pair_t;

int main()
{
    int exit_code = EXIT_SUCCESS;
    // taken from wikipedia article for BPE
    const char *text = "The original BPE algorithm operates by iteratively replacing the most common contiguous sequences of characters in a target text with unused 'placeholder' bytes. The iteration ends when no sequences can be found, leaving the target text effectively compressed. Decompression can be performed by reversing this process, querying known placeholder terms against their corresponding denoted sequence, using a lookup table. In the original paper, this lookup table is encoded and stored alongside the compressed text.";
    int text_size = strlen(text);

    hash_table_t *table = hash_table_create(1024, sizeof(pair_t), sizeof(size_t));
    if (!table)
    {
        perror("hash_table_create");
        exit_code = EXIT_FAILURE;
        goto error_handling;
    }

    for (int counter = 0; counter < text_size - 1; counter++)
    {
        pair_t pair = {text[counter], text[counter + 1]};
        size_t count = 0;

        // count remains unchanged when key is not found, so, this works
        hash_table_search(table, &pair, &count);
        count++;
        if (!hash_table_insert(table, &pair, &count))
        {
            perror("hash_table_insert");
            exit_code = EXIT_FAILURE;
            goto error_handling;
        }
    }

    dyn_arr_t *pair_arr = dyn_arr_create(0, sizeof(pair_t));
    if (!pair_arr)
    {
        perror("dyn_arr_create");
        exit_code = EXIT_FAILURE;
        goto error_handling;
    }

    for (size_t index = 0; index < 1U << 8U; index++)
    {
        pair_t pair = {.a = index, .b = 0};
        if (!dyn_arr_set(pair_arr, index, &pair))
        {
            perror("dyn_arr_set");
            exit_code = EXIT_FAILURE;
            goto error_handling;
        }
    }

error_handling:
    if (table)
    {
        hash_table_destroy(table);
    }
    return exit_code;
}