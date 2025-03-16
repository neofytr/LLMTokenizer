#include "../inc/hash_table.h"

#include <string.h>

static inline unsigned long djb2_hash(const void *key, size_t key_size)
{
    unsigned long hash = 5381;
    uint8_t *data = (uint8_t *)key;

    for (size_t counter = 0; counter < key_size; counter++)
    {
        hash = ((hash << 5) + hash) + data[counter];
    }

    return hash;
}

hash_table_t *hash_table_create(size_t num_of_buckets, size_t key_size, size_t value_size)
{
    hash_table_t *table = malloc(sizeof(hash_table_t));
    if (!table)
        return NULL;

    table->num_of_buckets = num_of_buckets;
    table->buckets = calloc(num_of_buckets, sizeof(node_t *));
    if (!table->buckets)
    {
        free(table);
        return NULL;
    }

    table->value_size = value_size;
    table->key_size = key_size;

    return table;
}

void hash_table_destroy(hash_table_t *table)
{
    if (!table)
        return;

    for (size_t i = 0; i < table->num_of_buckets; i++)
    {
        node_t *current = table->buckets[i];
        while (current)
        {
            node_t *next = current->next;
            free(current->key);
            free(current->value);
            free(current);
            current = next;
        }
    }

    free(table->buckets);
    free(table);
}

hash_table_t *hash_table_merge(hash_table_t **hash_table_arr, size_t len, hash_value_add add_value, size_t key_size, size_t value_size, size_t new_bucket_num)
{
    if (!hash_table_arr)
    {
        return NULL;
    }

    for (size_t index = 0; index < len; index++)
    {
        hash_table_t *table = hash_table_arr[index];
        if (!table || (table->key_size != key_size) || (table->value_size != value_size))
        {
            return NULL;
        }
    }

    hash_table_t *merged_table = hash_table_create(new_bucket_num, key_size, value_size);
    if (!merged_table)
    {
        return NULL;
    }

    uint8_t *value = (uint8_t *)malloc(value_size);
    if (!value)
    {
        hash_table_destroy(merged_table);
        return NULL;
    }

    uint8_t *new_val = (uint8_t *)malloc(value_size);
    if (!new_val)
    {
        hash_table_destroy(merged_table);
        return NULL;
    }

    for (size_t index = 0; index < len; index++)
    {
        hash_table_t *table = hash_table_arr[index];
        for (size_t counter = 0; counter < table->num_of_buckets; counter++)
        {
            node_t *curr = table->buckets[counter];
            while (curr)
            {
                if (!hash_table_search(merged_table, curr->key, (void *)value))
                {
                    if (!hash_table_insert(merged_table, curr->key, curr->value))
                    {
                        hash_table_destroy(merged_table);
                        return NULL;
                    }
                }
                else
                {
                    if (!add_value((void *)value, curr->value, (void *)new_val))
                    {
                        hash_table_destroy(merged_table);
                        return NULL;
                    }

                    if (!hash_table_insert(merged_table, curr->key, new_val))
                    {
                        hash_table_destroy(merged_table);
                        return NULL;
                    }
                }

                curr = curr->next;
            }
        }
    }

    free(value);
    free(new_val);
    return merged_table;
}

bool hash_table_insert(hash_table_t *table, const void *key, const void *value)
{
    if (!table || !key || !value)
        return false;

    unsigned long hash = djb2_hash(key, table->key_size) % table->num_of_buckets;

    node_t *current = table->buckets[hash];
    while (current)
    {
        if (!memcmp(current->key, key, table->key_size))
        {
            memcpy(current->value, value, table->value_size);
            return true;
        }
        current = current->next;
    }

    // before creating a new entry, check to see if we can find an empty entry in the same bucket
    while (current)
    {
        if (current->is_free)
        {
            memcpy(current->value, value, table->value_size);
            memcpy(current->key, key, table->key_size);
            current->is_free = false;
            return true;
        }

        current = current->next;
    }

    node_t *new_node = malloc(sizeof(node_t));
    if (!new_node)
        return false;

    new_node->key = malloc(table->key_size);
    if (!new_node->key)
    {
        free(new_node);
        return false;
    }

    new_node->value = malloc(table->value_size);
    if (!new_node->value)
    {
        free(new_node->key);
        free(new_node);
        return false;
    }

    memcpy(new_node->key, key, table->key_size);
    memcpy(new_node->value, value, table->value_size);

    new_node->next = table->buckets[hash];
    new_node->is_free = false;
    table->buckets[hash] = new_node;

    return true;
}

// marks all the entries in the table as free
bool hash_table_clear(hash_table_t *table)
{
    if (!table)
    {
        return false;
    }

    for (size_t index = 0; index < table->num_of_buckets; index++)
    {
        node_t *curr = (node_t *)table->buckets[index];
        while (curr)
        {
            curr->is_free = true;
            curr = curr->next;
        }
    }

    return true;
}

// doesn't actually delete the entry, just marks it as free
// this is better since we can use the space allocated for some other entry
bool hash_table_delete(hash_table_t *table, const void *key)
{
    if (!table || !key)
        return false;

    unsigned long hash = djb2_hash(key, table->key_size) % table->num_of_buckets;

    node_t *current = table->buckets[hash];
    node_t *prev = NULL;

    while (current)
    {
        if (!memcmp(current->key, key, table->key_size))
        {
            if (prev)
            {
                prev->next = current->next;
            }
            else
            {
                table->buckets[hash] = current->next;
            }

            current->is_free = true;
            return true;
        }

        prev = current;
        current = current->next;
    }

    return false;
}

bool hash_table_search(hash_table_t *table, const void *key, void *value)
{
    if (!table || !key || !value)
        return false;

    unsigned long hash = djb2_hash(key, table->key_size) % table->num_of_buckets;

    node_t *current = table->buckets[hash];
    while (current)
    {
        if (memcmp(current->key, key, table->key_size) == 0)
        {
            memcpy(value, current->value, table->value_size);
            return true;
        }
        current = current->next;
    }

    return false;
}