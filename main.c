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

void print_text(const uint32_t *text, int length)
{
    for (int i = 0; i < length; i++)
    {
        if (text[i] < 32 || text[i] > 126)
        { // Non-printable ASCII
            printf("[%u]", text[i]);
        }
        else
        {
            printf("%c", (char)text[i]);
        }
    }
    printf("\n");
}

int main(int argc, char **argv)
{
    if (argc < 2)
    {
        fprintf(stderr, "Usage: %s <file_path>\n", argv[0]);
        return EXIT_FAILURE;
    }

    int exit_code = EXIT_SUCCESS;
    char *text_buffer = get_file(argv[1]);
    if (!text_buffer)
    {
        return EXIT_FAILURE;
    }

    int original_text_size = strlen(text_buffer);
    int text_size = original_text_size;

    if (text_size < 2)
    {
        printf("Error: File contains less than 2 characters\n");
        exit_code = EXIT_FAILURE;
        goto error_handling;
    }

    // convert char array to uint32_t array
    uint32_t *text = (uint32_t *)malloc(text_size * sizeof(uint32_t));
    uint32_t *temp = (uint32_t *)malloc(text_size * sizeof(uint32_t));
    if (!text || !temp)
    {
        perror("malloc");
        free(text_buffer);
        return EXIT_FAILURE;
    }

    for (int i = 0; i < text_size; i++)
    {
        text[i] = (uint32_t)(uint8_t)text_buffer[i];
        temp[i] = (uint32_t)(uint8_t)text_buffer[i];
    }

    uint32_t next_symbol = 256;

    dyn_arr_t *pair_arr = dyn_arr_create(512, sizeof(pair_t));
    if (!pair_arr)
    {
        perror("dyn_arr_create");
        free(text);
        free(temp);
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
            free(text);
            free(temp);
            free(text_buffer);
            return EXIT_FAILURE;
        }
    }

    for (;;)
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

        int new_text_size = 0;
        for (int i = 0; i < text_size; i++)
        {
            if (i < text_size - 1 && text[i] == new_pair.a && text[i + 1] == new_pair.b)
            {
                temp[new_text_size++] = next_symbol;
                i++; // skip the next character as it's part of the pair
            }
            else
            {
                temp[new_text_size++] = text[i];
            }
        }

        // swap pointers
        uint32_t *swap = text;
        text = temp;
        temp = swap;
        text_size = new_text_size;

        next_symbol++;

        printf("%d\n", new_text_size);
        dyn_arr_free(node_arr);
        hash_table_destroy(table);
    }

    printf("Compression complete\n");

error_handling:
    if (pair_arr)
    {
        dyn_arr_free(pair_arr);
    }
    if (text)
    {
        free(text);
    }
    if (temp)
    {
        free(temp);
    }
    if (text_buffer)
    {
        free(text_buffer);
    }
    return exit_code;
}