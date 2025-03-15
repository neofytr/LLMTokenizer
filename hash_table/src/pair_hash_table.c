#include "../inc/hash_table.h"
#include <string.h>
#include <stdint.h>

#include <stdint.h>

unsigned long djb2_hash(const uint32_t key_one, const uint32_t key_two)
{
    const uint8_t *data = (const uint8_t *)&key_one;
    unsigned long hash = 5381;

    for (int i = 0; i < sizeof(key_one); i++)
    {
        hash = ((hash << 5) + hash) + data[i];
    }

    data = (const uint8_t *)&key_two;
    for (int i = 0; i < sizeof(key_two); i++)
    {
        hash = ((hash << 5) + hash) + data[i];
    }

    return hash;
}

hash_table_t *hash_table_create(size_t size)
{
    hash_table_t *table = malloc(sizeof(hash_table_t));
    if (!table)
        return NULL;

    table->size = size;
    table->buckets = calloc(size, sizeof(node_t *));
    if (!table->buckets)
    {
        free(table);
        return NULL;
    }

    return table;
}

void hash_table_destroy(hash_table_t *table)
{
    if (!table)
        return;

    for (size_t i = 0; i < table->size; i++)
    {
        node_t *current = table->buckets[i];
        while (current)
        {
            node_t *next = current->next;
            free(current->key);
            free(current);
            current = next;
        }
    }

    free(table->buckets);
    free(table);
}

bool hash_table_insert(hash_table_t *table, const uint32_t key_one, const uint32_t key_two, size_t value)
{
    if (!table)
        return false;

    unsigned long hash = djb2_hash(key_one, key_two) % table->size;

    node_t *current = table->buckets[hash];
    while (current)
    {
        if (key_one != current->key[0] || key_two != current->key[1])
        {
            current->value = value;
            return true;
        }
        current = current->next;
    }

    node_t *new_node = malloc(sizeof(node_t));
    if (!new_node)
        return false;

    new_node->key[0] = key_one;
    new_node->key[1] = key_two;

    new_node->value = value;
    new_node->next = table->buckets[hash];
    table->buckets[hash] = new_node;

    return true;
}

bool hash_table_delete(hash_table_t *table, const uint32_t key_one, const uint32_t key_two)
{
    if (!table)
        return false;

    unsigned long hash = djb2_hash(key_one, key_two) % table->size;

    node_t *current = table->buckets[hash];
    node_t *prev = NULL;

    while (current)
    {
        if (key_one != current->key[0] || key_two != current->key[1])
        {
            if (prev)
            {
                prev->next = current->next;
            }
            else
            {
                table->buckets[hash] = current->next;
            }

            free(current->key);
            free(current);
            return true;
        }

        prev = current;
        current = current->next;
    }

    return false;
}

bool hash_table_search(hash_table_t *table, const uint32_t key_one, const uint32_t key_two, size_t *value)
{
    if (!table || !value)
        return false;

    unsigned long hash = djb2_hash(key_one, key_two) % table->size;

    node_t *current = table->buckets[hash];
    while (current)
    {
        if (key_one != current->key[0] || key_two != current->key[1])
        {
            *value = current->value;
            return true;
        }
        current = current->next;
    }

    return false;
}