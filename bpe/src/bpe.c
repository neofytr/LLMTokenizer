#include "../inc/bpe.h"
#include <pthread.h>

bool is_less(const void *a, const void *b)
{
    pair_freq_t *freq_a = (pair_freq_t *)a;
    pair_freq_t *freq_b = (pair_freq_t *)b;

    return freq_a->freq < freq_b->freq;
}

hash_table_t *create_mem_table()
{
    hash_table_t *table = hash_table_create(256, sizeof(uint32_t), sizeof(char *));
    // the hash table stores the pointer to the character array
    if (!table)
    {
        return NULL;
    }
    return table;
}

char *resolve_pair(uint32_t pair_index, dyn_arr_t *pair_arr, hash_table_t *memoization_table)
{
    if (!pair_arr || !memoization_table)
    {
        return NULL;
    }

    char *cached_result;
    // get the pointer to the character array from the table
    if (hash_table_search(memoization_table, (const void *)&pair_index, (void *)&cached_result))
    {
        // duplicate the character array so that we don't destroy the table's copy of it (the table contains the pointer to its copy)
        char *result_copy = strdup(cached_result);
        return result_copy;
    }

    pair_t pair;
    if (!dyn_arr_get(pair_arr, pair_index, (void *)&pair))
    {
        return NULL;
    }

    char *result;

    if (pair.a == pair_index)
    {
        result = (char *)malloc(2);
        if (!result)
            return NULL;

        snprintf(result, 2, "%c", (char)pair.a);
    }
    else
    {
        char *a_text = resolve_pair(pair.a, pair_arr, memoization_table);
        if (!a_text)
            return NULL;

        char *b_text = resolve_pair(pair.b, pair_arr, memoization_table);
        if (!b_text)
        {
            free(a_text);
            return NULL;
        }

        result = (char *)malloc(strlen(a_text) + strlen(b_text) + 1);
        if (!result)
        {
            free(a_text);
            free(b_text);
            return NULL;
        }

        strcpy(result, a_text);
        strcat(result, b_text);

        free(a_text);
        free(b_text);
    }

    char *result_to_store = strdup(result);
    // make a copy of the result and store it's pointer into the table
    // this is done so that the table's copy is preserved when we free this result in a recursive step
    if (result_to_store)
    {
        hash_table_insert(memoization_table, (const void *)&pair_index, (const void *)&result_to_store);
    }

    return result;
}

void render_pairs(dyn_arr_t *pair_arr)
{
    hash_table_t *mem_table = create_mem_table();
    if (!mem_table)
    {
        return;
    }

    for (size_t index = 256; index <= pair_arr->last_index; index++)
    {
        pair_t pair;
        if (!dyn_arr_get(pair_arr, index, (void *)&pair))
        {
            return;
        }

        if (pair.a == index)
        {
            fprintf(stdout, "%zu => %c\n", index, (char)pair.a);
        }
        else
        {
            char *str = resolve_pair(index, pair_arr, mem_table);
            if (!str)
            {
                return;
            }

            fprintf(stdout, "%zu => %s\n", index, str);
            free(str);
        }
    }

    hash_table_destroy(mem_table);
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

void print_graph(dyn_arr_t *pair_arr, const char *png_name, bool add_ascii)
{
    FILE *temp_file = fopen("temp_graph.dot", "w");
    if (!temp_file)
    {
        perror("Failed to create temp file");
        return;
    }

    fprintf(temp_file, "digraph Pairs {\n");
    if (add_ascii)
    {
        for (uint32_t index = 0; index < pair_arr->last_index; index++)
        {
            pair_t pair;
            dyn_arr_get(pair_arr, index, (void *)&pair);
            fprintf(temp_file, "%u -> %u;\n", index, pair.a);
            fprintf(temp_file, "%u -> %u;\n", index, pair.b);
        }
    }
    else
    {
        for (uint32_t index = 256; index < pair_arr->last_index; index++)
        {
            pair_t pair;
            dyn_arr_get(pair_arr, index, (void *)&pair);
            fprintf(temp_file, "%u -> %u;\n", index, pair.a);
            fprintf(temp_file, "%u -> %u;\n", index, pair.b);
        }
    }

    fprintf(temp_file, "}\n");
    fclose(temp_file);

    char command[256];
    snprintf(command, sizeof(command), "dot -Tpng temp_graph.dot -o %s", png_name);
    int result = system(command);
    if (result != 0)
    {
        fprintf(stderr, "Failed to generate PNG\n");
    }

    int rv = system("rm temp_graph.dot");
}

bool dump_pairs(const char *path, dyn_arr_t *pair_arr)
{
    if (!path || !pair_arr)
    {
        fprintf(stderr, "Invalid arguments to dump_pairs\n");
        return false;
    }

    FILE *dump = fopen(path, "wb");
    if (!dump)
    {
        perror("fopen");
        return false;
    }

    for (uint16_t index = 256; index < pair_arr->last_index; index++)
    {
        pair_t pair;
        if (!dyn_arr_get(pair_arr, index, &pair))
        {
            fprintf(stderr, "Error retrieving element at index %u\n", index);
            fclose(dump);
            return false;
        }

        if (fwrite(&pair, sizeof(pair_t), 1, dump) != 1)
        {
            perror("fwrite");
            fclose(dump);
            return false;
        }
    }

    fclose(dump);
    return true;
}

dyn_arr_t *read_pairs(const char *path)
{
    if (!path)
    {
        fprintf(stderr, "Invalid file path\n");
        return NULL;
    }

    FILE *file = fopen(path, "rb");
    if (!file)
    {
        perror("fopen");
        return NULL;
    }

    dyn_arr_t *pair_arr = dyn_arr_create(512, sizeof(pair_t));
    if (!pair_arr)
    {
        fprintf(stderr, "Failed to create dynamic array\n");
        fclose(file);
        return NULL;
    }

    for (uint32_t i = 0; i < 256; i++)
    {
        pair_t pair = {i, 0};
        if (!dyn_arr_set(pair_arr, i, &pair))
        {
            fprintf(stderr, "dyn_arr_set failed at index %u\n", i);
            dyn_arr_free(pair_arr);
            fclose(file);
            return NULL;
        }
    }

    pair_t pair;
    size_t index = 256;
    while (fread(&pair, sizeof(pair_t), 1, file) == 1)
    {
        if (!dyn_arr_set(pair_arr, index, &pair))
        {
            fprintf(stderr, "dyn_arr_set failed at index %zu\n", index);
            dyn_arr_free(pair_arr);
            fclose(file);
            return NULL;
        }
        index++;
    }

    if (ferror(file))
    {
        perror("fread");
        dyn_arr_free(pair_arr);
        fclose(file);
        return NULL;
    }

    fclose(file);
    return pair_arr;
}

char *decompress(uint32_t *encoding, size_t len, dyn_arr_t *pair_arr)
{
#define INIT_LEN (4096)
    hash_table_t *mem_table = create_mem_table();
    if (!mem_table)
    {
        return NULL;
    }

    char *str = (char *)malloc(sizeof(char) * INIT_LEN);
    if (!str)
    {
        return NULL;
    }

    size_t str_pos = 0;
    size_t str_capacity = INIT_LEN;
    str[0] = '\0'; // initialize as empty string

    for (size_t index = 0; index < len; index++)
    {
        char *res_pair = resolve_pair(encoding[index], pair_arr, mem_table);
        if (!res_pair)
        {
            free(str);
            return NULL;
        }

        size_t pair_len = strlen(res_pair);
        if (str_pos + pair_len + 1 >= str_capacity) // +1 for null terminator
        {
            size_t new_capacity = 2 * (str_capacity + pair_len);
            char *new_str = realloc(str, new_capacity);
            if (!new_str)
            {
                free(res_pair);
                free(str);
                return NULL;
            }
            str = new_str;
            str_capacity = new_capacity;
        }

        // copy the pair to the end of str
        strcpy(str + str_pos, res_pair);
        str_pos += pair_len;

        free(res_pair);
    }

    hash_table_destroy(mem_table);
    return str;
#undef INIT_LEN
}

#define PROFILE_BEGIN_TS(beg) clock_gettime(CLOCK_MONOTONIC, &(beg))
#define PROFILE_END_TS(beg, label)                              \
    do                                                          \
    {                                                           \
        struct timespec end;                                    \
        clock_gettime(CLOCK_MONOTONIC, &end);                   \
        double elapsed = (end.tv_sec - (beg).tv_sec) +          \
                         (end.tv_nsec - (beg).tv_nsec) / 1e9;   \
        fprintf(stdout, "%s: %lf seconds\n", (label), elapsed); \
    } while (0)

typedef struct
{
    uint32_t *text_arr;
    hash_table_t *text_table;
    size_t len;
} text_t;

static void *get_freq(void *arg)
{
    if (!arg)
    {
        return NULL;
    }

    text_t *text_struct = (text_t *)arg;
    uint32_t *text_arr = text_struct->text_arr;
    hash_table_t *text_table = text_struct->text_table;
    size_t text_size = text_struct->len;

    if (!text_arr || !text_table)
    {
        free(text_struct);
        return NULL;
    }

    for (size_t i = 0; i < text_size - 1; i++)
    {
        pair_t pair = {text_arr[i], text_arr[i + 1]};
        size_t count = 0;

        hash_table_search(text_table, &pair, &count);
        count++;
        if (!hash_table_insert(text_table, &pair, &count))
        {
            perror("hash_table_insert");
        }
    }

    free(text_arr);
    free(text_struct);

    return (void *)1;
}

bool val_add(const void *val_one, const void *val_two, const void *result)
{
    if (!val_one || !val_two || !result)
    {
        return false;
    }

    size_t size_one = *(size_t *)val_one;
    size_t size_two = *(size_t *)val_two;

    size_t res = size_one + size_two;
    *(size_t *)result = res;

    return true;
}

dyn_arr_t *compress(const char *path, uint32_t **encoding, size_t *len)
{
    if (!path || !encoding || !len)
    {
        return NULL;
    }

    char *text_buffer = get_file(path);
    if (!text_buffer)
    {
        return NULL;
    }

    size_t original_text_size = strlen(text_buffer);
    size_t text_size = original_text_size;

    if (text_size < 2)
    {
        printf("Error: File contains less than 2 characters\n");
        free(text_buffer);
        return NULL;
    }

    uint32_t *text = (uint32_t *)malloc(text_size * sizeof(uint32_t));
    uint32_t *temp = (uint32_t *)malloc(text_size * sizeof(uint32_t));
    if (!text || !temp)
    {
        perror("malloc");
        free(text_buffer);
        free(text); // Free if allocated
        free(temp); // Free if allocated
        return NULL;
    }

    for (size_t i = 0; i < text_size; i++)
    {
        text[i] = (uint32_t)(uint8_t)text_buffer[i];
        temp[i] = (uint32_t)(uint8_t)text_buffer[i];
    }

    free(text_buffer);

    uint32_t next_symbol = 256;
    dyn_arr_t *pair_arr = NULL;

    pair_arr = dyn_arr_create(512, sizeof(pair_t));
    if (!pair_arr)
    {
        perror("dyn_arr_create");
        free(text);
        free(temp);
        return NULL;
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
            return NULL;
        }
    }

    for (size_t iteration = 0;; iteration++)
    {

        printf("\n\niteration: %zu\n", iteration);
#define THREAD_NO (16)

        clock_t begin;

        PROFILE_BEGIN(begin);
        pthread_t worker_threads[THREAD_NO];
        pthread_attr_t attr;

        pthread_attr_init(&attr);
        pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_JOINABLE);

        size_t per_thread_text_len = text_size / THREAD_NO;
        size_t last_thread_excess = text_size % THREAD_NO;

        hash_table_t *table_arr[THREAD_NO];
        memset(table_arr, 0, sizeof(table_arr));

        for (size_t index = 0; index < THREAD_NO; index++)
        {
            text_t *text_struct = (text_t *)malloc(sizeof(text_t));
            if (!text_struct)
            {
                perror("malloc text_struct");
                for (size_t i = 0; i < index; i++)
                {
                    if (table_arr[i])
                    {
                        hash_table_destroy(table_arr[i]);
                    }
                }
                goto error_handling;
            }

            size_t offset = index * per_thread_text_len;

            if (index == THREAD_NO - 1)
            {
                text_struct->len = last_thread_excess + per_thread_text_len;
            }
            else
            {
                text_struct->len = per_thread_text_len;
            }

            text_struct->text_arr = (uint32_t *)malloc(sizeof(uint32_t) * text_struct->len);
            if (!text_struct->text_arr)
            {
                perror("malloc text_arr");
                free(text_struct);
                for (size_t i = 0; i < index; i++)
                {
                    if (table_arr[i])
                    {
                        hash_table_destroy(table_arr[i]);
                    }
                }
                goto error_handling;
            }

            memcpy(text_struct->text_arr, text + offset, text_struct->len * sizeof(uint32_t));

            text_struct->text_table = hash_table_create(1024, sizeof(pair_t), sizeof(size_t));
            if (!text_struct->text_table)
            {
                perror("hash_table_create");
                free(text_struct->text_arr);
                free(text_struct);
                for (size_t i = 0; i < index; i++)
                {
                    if (table_arr[i])
                    {
                        hash_table_destroy(table_arr[i]);
                    }
                }
                goto error_handling;
            }

            table_arr[index] = text_struct->text_table;
            if (pthread_create(&worker_threads[index], &attr, get_freq, (void *)text_struct) != 0)
            {
                perror("pthread_create");
                hash_table_destroy(text_struct->text_table);
                free(text_struct->text_arr);
                free(text_struct);
                for (size_t i = 0; i <= index; i++)
                {
                    if (table_arr[i])
                    {
                        hash_table_destroy(table_arr[i]);
                    }
                }
                goto error_handling;
            }
        }

        pthread_attr_destroy(&attr);

        for (size_t index = 0; index < THREAD_NO; index++)
        {
            void *status;
            pthread_join(worker_threads[index], &status);
        }

        hash_table_t *table = hash_table_merge(table_arr, THREAD_NO, val_add, sizeof(pair_t), sizeof(size_t), 2048);
        if (!table)
        {
            for (size_t i = 0; i < THREAD_NO; i++)
            {
                hash_table_destroy(table_arr[i]);
                table_arr[i] = NULL;
            }

            goto error_handling;
        }

        for (size_t i = 0; i < THREAD_NO; i++)
        {
            hash_table_destroy(table_arr[i]);
            table_arr[i] = NULL;
        }

        PROFILE_END(begin, "Frequency calculation time:");

        dyn_arr_t *node_arr = dyn_arr_create(0, sizeof(pair_freq_t));
        if (!node_arr)
        {
            perror("dyn_arr_create");
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
                pair_t *pair = (pair_t *)curr->key;
                size_t *freq = (size_t *)curr->value;

                temp.pair.a = pair->a;
                temp.pair.b = pair->b;
                temp.freq = *freq;

                if (!dyn_arr_set(node_arr, index++, (void *)&temp))
                {
                    perror("dyn_arr_set");
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
            dyn_arr_free(node_arr);
            hash_table_destroy(table);
            goto error_handling;
        }

        begin = clock();
        size_t new_text_size = 0;
        for (size_t i = 0; i < text_size; i++)
        {
            if (i < text_size - 1 && text[i] == new_pair.a && text[i + 1] == new_pair.b)
            {
                temp[new_text_size++] = next_symbol;
                i++;
            }
            else
            {
                temp[new_text_size++] = text[i];
            }
        }

        uint32_t *swap = text;
        text = temp;
        temp = swap;
        text_size = new_text_size;

        next_symbol++;

        dyn_arr_free(node_arr);
        hash_table_destroy(table);
    }

    *encoding = text;
    *len = text_size;
    free(temp);

    uint32_t *new_encoding = realloc(*encoding, *len * sizeof(uint32_t));
    if (!new_encoding)
    {
        goto error_handling;
    }
    *encoding = new_encoding;

    return pair_arr;

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
    *encoding = NULL;
    *len = 0;
    return NULL;
}

#undef THREAD_NO