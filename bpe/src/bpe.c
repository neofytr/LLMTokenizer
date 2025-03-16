#include "../inc/bpe.h"

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

#define PROFILE_BEGIN(beg) (beg) = clock()
#define PROFILE_END(beg, label) fprintf(stdout, "%s: %lf seconds\n", (label), (double)(clock() - (beg)) / CLOCKS_PER_SEC)

dyn_arr_t *compress(const char *path, uint32_t **encoding, size_t *len)
{

#include <time.h>

    clock_t begin;

    if (!path)
    {
        return NULL;
    }

    char *text_buffer = get_file(path);
    if (!text_buffer)
    {
        return NULL;
    }

    int original_text_size = strlen(text_buffer);
    int text_size = original_text_size;

    if (text_size < 2)
    {
        printf("Error: File contains less than 2 characters\n");
        return NULL;
    }

    // convert char array to uint32_t array
    uint32_t *text = (uint32_t *)malloc(text_size * sizeof(uint32_t));
    uint32_t *temp = (uint32_t *)malloc(text_size * sizeof(uint32_t));
    if (!text || !temp)
    {
        perror("malloc");
        free(text_buffer);
        return NULL;
    }

    for (int i = 0; i < text_size; i++)
    {
        text[i] = (uint32_t)(uint8_t)text_buffer[i];
        temp[i] = (uint32_t)(uint8_t)text_buffer[i];
    }

    free(text_buffer);

    uint32_t next_symbol = 256;

    dyn_arr_t *pair_arr = dyn_arr_create(512, sizeof(pair_t));
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
        printf("\n\n");
        printf("Iteration: %zu\n", iteration);

        hash_table_t *table = hash_table_create(1024, sizeof(pair_t), sizeof(size_t));
        if (!table)
        {
            perror("hash_table_create");
            goto error_handling;
        }

        PROFILE_BEGIN(begin);
        // prepare frequency of each pair
        for (int i = 0; i < text_size - 1; i++)
        {
            pair_t pair = {text[i], text[i + 1]};
            size_t count = 0;

            hash_table_search(table, &pair, &count);
            count++;
            if (!hash_table_insert(table, &pair, &count))
            {
                perror("hash_table_insert");
                hash_table_destroy(table);
                goto error_handling;
            }
        }
        PROFILE_END(begin, "Frequency preparation time");

        // create a dynamic array to store pair-frequency data retrived from text
        dyn_arr_t *node_arr = dyn_arr_create(0, sizeof(pair_freq_t));
        if (!node_arr)
        {
            perror("dyn_arr_create");
            hash_table_destroy(table);
            goto error_handling;
        }

        // feed the pair-frequency data into a dynamic array
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
                    dyn_arr_free(node_arr);
                    hash_table_destroy(table);
                    goto error_handling;
                }

                curr = curr->next;
            }
        }

        // no pairs found; text with 0 or 1 element
        if (!index)
        {
            dyn_arr_free(node_arr);
            hash_table_destroy(table);
            break;
        }

        // find the pair-freq node with the maximum frequency
        pair_freq_t max;
        if (!dyn_arr_max(node_arr, 0, index - 1, is_less, &max))
        {
            perror("dyn_arr_max");
            dyn_arr_free(node_arr);
            hash_table_destroy(table);
            goto error_handling;
        }

        // stop the process if the max frequency of all nodes is 1
        if (max.freq <= 1)
        {
            dyn_arr_free(node_arr);
            hash_table_destroy(table);
            break;
        }

        // add the max frequency pair to the pair array
        // the index of this max frequency pair is it's pair ID
        pair_t new_pair = max.pair;
        if (!dyn_arr_set(pair_arr, next_symbol, &new_pair))
        {
            perror("dyn_arr_set");
            dyn_arr_free(node_arr);
            hash_table_destroy(table);
            goto error_handling;
        }

        PROFILE_BEGIN(begin);
        // replace the max frequency pair in the text with it's pair ID
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

        PROFILE_END(begin, "Replacement time");

        dyn_arr_free(node_arr);
        hash_table_destroy(table);

        printf("\n");
        printf("iteration %zu finished!\n", iteration);
        printf("pair array length: %zu\n", pair_arr->last_index + 1);
        printf("encoded text size: %d\n", text_size);
    }

    *encoding = text;
    *len = text_size;
    free(temp);

    // resize the encoded text into it's requisite size
    *encoding = realloc(*encoding, *len);
    if (!encoding)
    {
        goto error_handling;
    }
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

    return NULL;
}
