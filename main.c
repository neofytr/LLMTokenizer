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

char *get_file(const char *path)
{
    FILE *file = fopen(path, "r");
    if (!file)
    {
        perror("fopen");
        return NULL;
    }

    if (fseek(file, 0, SEEK_END) == -1)
    {
        perror("fseek");
        fclose(file);
        return NULL;
    }

    long len = ftell(file);
    if (len == -1)
    {
        perror("ftell");
        fclose(file);
        return NULL;
    }

    rewind(file);

    char *arr = (char *)malloc(len + 1); // +1 for null terminator
    if (!arr)
    {
        perror("malloc");
        fclose(file);
        return NULL;
    }

    size_t read_bytes = fread(arr, 1, len, file);
    if (read_bytes < (size_t)len)
    {
        if (ferror(file))
        {
            perror("fread");
            free(arr);
            fclose(file);
            return NULL;
        }
    }

    arr[len] = '\0';

    fclose(file);
    return arr;
}

int main(int argc, char **argv)
{
    int exit_code = EXIT_SUCCESS;
    const char *text = get_file(argv[1]);
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
    if (text)
    {
        free(text);
    }
    return exit_code;
}