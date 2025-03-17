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
    str[0] = 0; // initialize as empty string

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

#include <time.h>

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

#define THREAD_NO 16

static uint32_t *text;
static uint32_t *temp;
static size_t text_size;
static hash_table_t *thread_tables[THREAD_NO];
static pthread_t worker_threads[THREAD_NO] = {0};

static pthread_barrier_t barrier;

#define CHUNK_SIZE (64 * 1024)

static size_t next_chunk_index;
static pthread_mutex_t chunk_mutex = PTHREAD_MUTEX_INITIALIZER;

static void *get_freq(void *arg)
{
    size_t thread_idx = (size_t)arg;

    pthread_barrier_wait(&barrier);

    size_t adaptive_chunk_size;
    pthread_mutex_lock(&chunk_mutex);
    if (text_size < CHUNK_SIZE * THREAD_NO)
    {
        size_t per_thread_len = text_size / THREAD_NO;
        size_t start_index = thread_idx * per_thread_len;
        size_t chunk_len = (thread_idx == THREAD_NO - 1) ? (per_thread_len + text_size % THREAD_NO) : per_thread_len;

        if (chunk_len > 0)
        {
            pthread_mutex_unlock(&chunk_mutex);

            for (size_t i = start_index; i < start_index + chunk_len; i++)
            {
                if (i + 1 >= text_size)
                    break;

                pair_t pair = {text[i], text[i + 1]};
                size_t count = 0;

                hash_table_search(thread_tables[thread_idx], &pair, &count);
                count++;
                hash_table_insert(thread_tables[thread_idx], &pair, &count);
            }
        }
        else
        {
            pthread_mutex_unlock(&chunk_mutex);
        }
        pthread_barrier_wait(&barrier);
    }
    else
    {
        adaptive_chunk_size = CHUNK_SIZE;
        pthread_mutex_unlock(&chunk_mutex);

        while (true)
        {
            size_t start_index;
            size_t chunk_len;

            pthread_mutex_lock(&chunk_mutex);
            start_index = next_chunk_index;
            if (start_index >= text_size)
            {
                pthread_mutex_unlock(&chunk_mutex);
                break;
            }

            chunk_len = (start_index + adaptive_chunk_size > text_size) ? (text_size - start_index) : adaptive_chunk_size;

            next_chunk_index += chunk_len;
            pthread_mutex_unlock(&chunk_mutex);

            for (size_t i = start_index; i < start_index + chunk_len; i++)
            {
                if (i + 1 >= text_size)
                    break;

                pair_t pair = {text[i], text[i + 1]};
                size_t count = 0;

                hash_table_search(thread_tables[thread_idx], &pair, &count);
                count++;
                hash_table_insert(thread_tables[thread_idx], &pair, &count);
            }
        }

        pthread_barrier_wait(&barrier);
    }

    return (void *)1;
}

bool val_add(const void *val_one, const void *val_two, const void *result)
{
    if (!val_one || !val_two || !result)
        return false;

    size_t size_one = *(size_t *)val_one;
    size_t size_two = *(size_t *)val_two;

    *(size_t *)result = size_one + size_two;
    return true;
}

dyn_arr_t *compress(const char *path, uint32_t **encoding, size_t *len)
{
    pthread_attr_t attr;
    dyn_arr_t *pair_arr = NULL;
    hash_table_t *table = NULL;
    hash_table_t *new_table = NULL;
    bool threads_created = false;

    if (!path || !encoding || !len)
        return NULL;

    char *text_buffer = get_file(path);
    if (!text_buffer)
        return NULL;

    size_t original_text_size = strlen(text_buffer);
    text_size = original_text_size;

    if (text_size < 2)
    {
        printf("Error: File contains less than 2 characters\n");
        free(text_buffer);
        return NULL;
    }

    text = (uint32_t *)malloc(text_size * sizeof(uint32_t));
    if (!text)
    {
        free(text_buffer);
        return NULL;
    }

    temp = (uint32_t *)malloc(text_size * sizeof(uint32_t));
    if (!temp)
    {
        free(text_buffer);
        free(text);
        return NULL;
    }

    for (size_t i = 0; i < text_size; i++)
    {
        text[i] = (uint32_t)(uint8_t)text_buffer[i];
        temp[i] = (uint32_t)(uint8_t)text_buffer[i];
    }

    free(text_buffer);

    uint32_t next_symbol = 256;
    pair_arr = dyn_arr_create(512, sizeof(pair_t));

    if (!pair_arr)
    {
        free(text);
        free(temp);
        return NULL;
    }

    for (uint32_t i = 0; i < 256; i++)
    {
        pair_t pair = {i, 0};
        if (!dyn_arr_set(pair_arr, i, &pair))
        {
            dyn_arr_free(pair_arr);
            free(text);
            free(temp);
            return NULL;
        }
    }

#define PER_THREAD_TABLE_BUCKET_NUM (1U << 12)
#define MERGED_TABLE_BUCKET_NUM (1U << 18)

    for (size_t i = 0; i < THREAD_NO; i++)
    {
        thread_tables[i] = hash_table_create(PER_THREAD_TABLE_BUCKET_NUM, sizeof(pair_t), sizeof(size_t));
        if (!thread_tables[i])
        {
            for (size_t j = 0; j < i; j++)
                hash_table_destroy(thread_tables[j]);

            goto error_handling;
        }
    }

    if (pthread_barrier_init(&barrier, NULL, THREAD_NO + 1))
    {
        for (size_t i = 0; i < THREAD_NO; i++)
            hash_table_destroy(thread_tables[i]);
        goto error_handling;
    }

    if (pthread_attr_init(&attr))
    {
        pthread_barrier_destroy(&barrier);
        for (size_t i = 0; i < THREAD_NO; i++)
            hash_table_destroy(thread_tables[i]);
        goto error_handling;
    }

    if (pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_JOINABLE))
    {
        pthread_attr_destroy(&attr);
        pthread_barrier_destroy(&barrier);
        for (size_t i = 0; i < THREAD_NO; i++)
            hash_table_destroy(thread_tables[i]);
        goto error_handling;
    }

    for (size_t i = 0; i < THREAD_NO; i++)
    {
        if (pthread_create(&worker_threads[i], &attr, get_freq, (void *)i))
        {
            pthread_attr_destroy(&attr);
            pthread_barrier_destroy(&barrier);
            for (size_t j = 0; j < i; j++)
            {
                pthread_cancel(worker_threads[j]);
                pthread_join(worker_threads[j], NULL);
            }
            for (size_t i = 0; i < THREAD_NO; i++)
                hash_table_destroy(thread_tables[i]);
            goto error_handling;
        }
    }

    threads_created = true;
    pthread_attr_destroy(&attr);

    pthread_mutex_lock(&chunk_mutex);
    next_chunk_index = 0;
    pthread_mutex_unlock(&chunk_mutex);

    pthread_barrier_wait(&barrier);
    pthread_barrier_wait(&barrier);

    for (size_t i = 0; i < THREAD_NO; i++)
    {
        void *status;
        pthread_join(worker_threads[i], &status);
    }

    table = hash_table_merge(thread_tables, THREAD_NO, val_add,
                             sizeof(pair_t), sizeof(size_t), MERGED_TABLE_BUCKET_NUM);

    if (!table)
    {
        for (size_t i = 0; i < THREAD_NO; i++)
            hash_table_destroy(thread_tables[i]);
        pthread_barrier_destroy(&barrier);
        goto error_handling;
    }

    for (size_t i = 0; i < THREAD_NO; i++)
        hash_table_destroy(thread_tables[i]);

    for (;;)
    {
        dyn_arr_t *node_arr = dyn_arr_create(0, sizeof(pair_freq_t));
        if (!node_arr)
        {
            hash_table_destroy(table);
            pthread_barrier_destroy(&barrier);
            goto error_handling;
        }

        size_t index = 0;
        for (size_t i = 0; i < table->num_of_buckets; i++)
        {
            node_t *curr = table->buckets[i];
            while (curr)
            {
                pair_freq_t pair_freq;
                pair_t *pair = (pair_t *)curr->key;
                size_t *freq = (size_t *)curr->value;

                pair_freq.pair.a = pair->a;
                pair_freq.pair.b = pair->b;
                pair_freq.freq = *freq;

                if (!dyn_arr_set(node_arr, index++, &pair_freq))
                {
                    dyn_arr_free(node_arr);
                    hash_table_destroy(table);
                    pthread_barrier_destroy(&barrier);
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
            dyn_arr_free(node_arr);
            hash_table_destroy(table);
            pthread_barrier_destroy(&barrier);
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
            dyn_arr_free(node_arr);
            hash_table_destroy(table);
            pthread_barrier_destroy(&barrier);
            goto error_handling;
        }

        new_table = hash_table_create(MERGED_TABLE_BUCKET_NUM, sizeof(pair_t), sizeof(size_t));
        if (!new_table)
        {
            dyn_arr_free(node_arr);
            hash_table_destroy(table);
            pthread_barrier_destroy(&barrier);
            goto error_handling;
        }

        size_t new_text_size = 0;
        for (size_t i = 0; i < text_size; i++)
        {
            if (i < text_size - 1 && text[i] == new_pair.a && text[i + 1] == new_pair.b)
            {
                temp[new_text_size++] = next_symbol;

                if (new_text_size > 1)
                {
                    pair_t prev_pair = {temp[new_text_size - 2], next_symbol};
                    size_t count = 0;
                    hash_table_search(new_table, &prev_pair, &count);
                    count++;
                    hash_table_insert(new_table, &prev_pair, &count);
                }

                if (i + 2 < text_size)
                {
                    pair_t next_pair = {next_symbol, text[i + 2]};
                    size_t count = 0;
                    hash_table_search(new_table, &next_pair, &count);
                    count++;
                    hash_table_insert(new_table, &next_pair, &count);
                }

                i++;
            }
            else
            {
                temp[new_text_size] = text[i];

                if (new_text_size > 0)
                {
                    pair_t curr_pair = {temp[new_text_size - 1], temp[new_text_size]};
                    size_t count = 0;
                    hash_table_search(new_table, &curr_pair, &count);
                    count++;
                    hash_table_insert(new_table, &curr_pair, &count);
                }

                new_text_size++;
            }
        }

        uint32_t *swap = text;
        text = temp;
        temp = swap;
        text_size = new_text_size;

        next_symbol++;
        dyn_arr_free(node_arr);
        hash_table_destroy(table);
        table = new_table;
        new_table = NULL;
    }

    pthread_barrier_destroy(&barrier);

    *encoding = text;
    *len = text_size;
    free(temp);
    temp = NULL;

    uint32_t *reallocated_encoding = realloc(*encoding, *len * sizeof(uint32_t));
    if (reallocated_encoding)
    {
        *encoding = reallocated_encoding;
    }

    return pair_arr;

error_handling:
    if (threads_created)
    {
        for (size_t i = 0; i < THREAD_NO; i++)
        {
            if (worker_threads[i])
            {
                void *status;
                pthread_cancel(worker_threads[i]);
                pthread_join(worker_threads[i], &status);
            }
        }
    }

    if (pair_arr)
        dyn_arr_free(pair_arr);
    if (text)
        free(text);
    if (temp)
        free(temp);
    if (table)
        hash_table_destroy(table);
    if (new_table)
        hash_table_destroy(new_table);

    *encoding = NULL;
    *len = 0;
    return NULL;
}