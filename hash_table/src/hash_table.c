#include "../inc/hash_table.h"
#include <string.h>

unsigned long djb2_hash(const char *str)
{
    unsigned long hash = 5381;
    int c;

    while ((c = *str++))
    {
        hash = ((hash << 5) + hash) + c;
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

bool hash_table_insert(hash_table_t *table, const char *key, size_t value)
{
    if (!table || !key)
        return false;

    unsigned long hash = djb2_hash(key) % table->size;

    node_t *current = table->buckets[hash];
    while (current)
    {
        if (!strcmp(current->key, key))
        {
            current->value = value;
            return true;
        }
        current = current->next;
    }

    node_t *new_node = malloc(sizeof(node_t));
    if (!new_node)
        return false;

    new_node->key = strdup(key);
    if (!new_node->key)
    {
        free(new_node);
        return false;
    }

    new_node->value = value;
    new_node->next = table->buckets[hash];
    table->buckets[hash] = new_node;

    return true;
}

bool hash_table_delete(hash_table_t *table, const char *key)
{
    if (!table || !key)
        return false;

    unsigned long hash = djb2_hash(key) % table->size;

    node_t *current = table->buckets[hash];
    node_t *prev = NULL;

    while (current)
    {
        if (strcmp(current->key, key) == 0)
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

bool hash_table_search(hash_table_t *table, const char *key, size_t *value)
{
    if (!table || !key || !value)
        return false;

    unsigned long hash = djb2_hash(key) % table->size;

    node_t *current = table->buckets[hash];
    while (current)
    {
        if (strcmp(current->key, key) == 0)
        {
            *value = current->value;
            return true;
        }
        current = current->next;
    }

    return false;
}