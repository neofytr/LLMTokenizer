#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdint.h>
#include <errno.h>
#include <stdbool.h>

#include "./dyn_arr/inc/dyn_arr.h"
#include "hash_table/inc/hash_table.h"

typedef struct
{
    uint32_t a, b;
} pair_t;

typedef struct
{
    pair_t pair;
    uint32_t freq;
} pair_freq_t;

bool is_less(const void *a, const void *b)
{
    pair_freq_t *freq_a = (pair_freq_t *)a;
    pair_freq_t *freq_b = (pair_freq_t *)b;

    return freq_a->freq < freq_b->freq;
}

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

int replace_pairs(uint8_t *text, int text_size, pair_t pair, uint8_t new_symbol)
{
    uint8_t *new_text = (uint8_t *)malloc(text_size + 1);
    if (!new_text)
    {
        perror("malloc");
        return -1;
    }

    int new_index = 0;
    for (int i = 0; i < text_size; i++)
    {
        if (i < text_size - 1 && text[i] == pair.a && text[i + 1] == pair.b)
        {
            new_text[new_index++] = new_symbol;
            i++; // skip the next character as it's part of the pair
        }
        else
        {
            new_text[new_index++] = text[i];
        }
    }

    new_text[new_index] = '\0';
    memcpy(text, new_text, new_index + 1);
    free(new_text);

    return new_index;
}

int main(int argc, char **argv)
{
    if (argc < 2)
    {
        fprintf(stderr, "Usage: %s <file_path> [max_iterations]\n", argv[0]);
        return EXIT_FAILURE;
    }

    int exit_code = EXIT_SUCCESS;
    char *text_buffer = get_file(argv[1]);
    if (!text_buffer)
    {
        return EXIT_FAILURE;
    }

    int text_size = strlen(text_buffer);
    uint8_t *text = (uint8_t *)text_buffer;

    int max_iterations = 1000;
    if (argc > 2)
    {
        max_iterations = atoi(argv[2]);
        if (max_iterations <= 0)
        {
            fprintf(stderr, "Invalid max iterations: %s\n", argv[2]);
            free(text_buffer);
            return EXIT_FAILURE;
        }
    }

    uint32_t next_symbol = 256;

    dyn_arr_t *pair_arr = dyn_arr_create(512, sizeof(pair_t));
    if (!pair_arr)
    {
        perror("dyn_arr_create");
        free(text_buffer);
        return EXIT_FAILURE;
    }

    for (uint32_t i = 0; i < 256; i++)
    {
        pair_t pair = {i, 0};
        if (!dyn_arr_set(pair_arr, i, &pair))
        {
            perror("dyn_arr_set");
            dyn_arr_free(pair_arr);
            free(text_buffer);
            return EXIT_FAILURE;
        }
    }

    for (int iteration = 0; iteration < max_iterations && text_size > 1; iteration++)
    {
        hash_table_t *table = hash_table_create(1024, sizeof(pair_t), sizeof(size_t));
        if (!table)
        {
            perror("hash_table_create");
            exit_code = EXIT_FAILURE;
            goto error_handling;
        }

        for (int i = 0; i < text_size - 1; i++)
        {
            pair_t pair = {text[i], text[i + 1]};
            size_t count = 0;

            hash_table_search(table, &pair, &count);
            count++;
            if (!hash_table_insert(table, &pair, &count))
            {
                perror("hash_table_insert");
                exit_code = EXIT_FAILURE;
                hash_table_destroy(table);
                goto error_handling;
            }
        }

        dyn_arr_t *node_arr = dyn_arr_create(0, sizeof(pair_freq_t));
        if (!node_arr)
        {
            perror("dyn_arr_create");
            exit_code = EXIT_FAILURE;
            hash_table_destroy(table);
            goto error_handling;
        }

        size_t index = 0;
        for (size_t i = 0; i < table->num_of_buckets; i++)
        {
            node_t *curr = table->buckets[i];
            while (curr)
            {
                pair_freq_t temp;
                pair_t *pair = curr->key;
                size_t *freq = curr->value;

                temp.pair.a = pair->a;
                temp.pair.b = pair->b;
                temp.freq = *freq;

                if (!dyn_arr_set(node_arr, index++, (void *)&temp))
                {
                    perror("dyn_arr_set");
                    exit_code = EXIT_FAILURE;
                    dyn_arr_free(node_arr);
                    hash_table_destroy(table);
                    goto error_handling;
                }

                curr = curr->next;
            }
        }

        if (!index)
        {
            dyn_arr_free(node_arr);
            hash_table_destroy(table);
            break;
        }

        pair_freq_t max;
        if (!dyn_arr_max(node_arr, 0, index - 1, is_less, &max))
        {
            perror("dyn_arr_max");
            exit_code = EXIT_FAILURE;
            dyn_arr_free(node_arr);
            hash_table_destroy(table);
            goto error_handling;
        }

        if (max.freq <= 1)
        {
            dyn_arr_free(node_arr);
            hash_table_destroy(table);
            break;
        }

        pair_t new_pair = max.pair;
        if (!dyn_arr_set(pair_arr, next_symbol, &new_pair))
        {
            perror("dyn_arr_set");
            exit_code = EXIT_FAILURE;
            dyn_arr_free(node_arr);
            hash_table_destroy(table);
            goto error_handling;
        }

        printf("Iteration %d: Replacing pair (%u, %u) with symbol %u, frequency %u\n",
               iteration, new_pair.a, new_pair.b, next_symbol, max.freq);

        text_size = replace_pairs(text, text_size, new_pair, next_symbol);
        if (text_size == -1)
        {
            exit_code = EXIT_FAILURE;
            dyn_arr_free(node_arr);
            hash_table_destroy(table);
            goto error_handling;
        }

        next_symbol++;
        dyn_arr_free(node_arr);
        hash_table_destroy(table);
    }

    FILE *output = fopen("compressed.bin", "wb");
    if (!output)
    {
        perror("fopen");
        exit_code = EXIT_FAILURE;
        goto error_handling;
    }

    uint32_t dict_size = next_symbol;
    fwrite(&dict_size, sizeof(uint32_t), 1, output);

    for (uint32_t i = 256; i < dict_size; i++)
    {
        pair_t pair;
        if (!dyn_arr_get(pair_arr, i, &pair))
        {
            perror("dyn_arr_get");
            fclose(output);
            exit_code = EXIT_FAILURE;
            goto error_handling;
        }
        fwrite(&pair, sizeof(pair_t), 1, output);
    }

    fwrite(&text_size, sizeof(int), 1, output);

    fwrite(text, 1, text_size, output);

    fclose(output);

    printf("Compression complete");

error_handling:
    if (pair_arr)
    {
        dyn_arr_free(pair_arr);
    }
    if (text_buffer)
    {
        free((void *)text_buffer);
    }
    return exit_code;
}