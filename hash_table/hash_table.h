#ifndef HASH_TABLE_H
#define HASH_TABLE_H

#include <stdlib.h>
#include <stdbool.h>
#include <stdint.h>
#include <string.h>

typedef struct node
{
    void *key;
    void *value;
    struct node *next;
} node_t;

typedef struct
{
    size_t num_of_buckets; // number of buckets you want in the hashtable
    size_t key_size;
    size_t value_size;
    node_t **buckets; // each bucket is a linked list of nodes
} hash_table_t;

hash_table_t *hash_table_create(size_t num_of_buckets, size_t key_size, size_t value_size); // number of buckets you want in the hashtable
                                                                                            // each bucket is a linked list of nodes
void hash_table_destroy(hash_table_t *table);

bool hash_table_insert(hash_table_t *table, const void *key, const void *value);
bool hash_table_delete(hash_table_t *table, const void *key);
bool hash_table_search(hash_table_t *table, const void *key, void *value);

#endif

#ifdef HASH_TABLE_IMPLEMENTATION

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
    table->buckets[hash] = new_node;

    return true;
}

bool hash_table_delete(hash_table_t *table, const void *key)
{
    if (!table || !key)
        return false;

    unsigned long hash = djb2_hash(key, table->key_size) % table->num_of_buckets;

    node_t *current = table->buckets[hash];
    node_t *prev = NULL;

    while (current)
    {
        if (memcmp(current->key, key, table->key_size) == 0)
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
            free(current->value);
            free(current);
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

#endif