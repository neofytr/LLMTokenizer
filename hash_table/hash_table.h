#ifndef HASH_TABLE_H
#define HASH_TABLE_H

#include <stdlib.h>
#include <stdbool.h>
#include <stdint.h>
#include <string.h>

typedef struct node
{
    KEY_TYPE key;
    VALUE_TYPE value;
    struct node *next;
} node_t;

typedef struct
{
    size_t size;      // size is the number of buckets you want in the hashtable
    node_t **buckets; // each bucket is a linked list of nodesI
} hash_table_t;

hash_table_t *hash_table_create(size_t size); // size is the number of buckets you want in the hashtable
                                              // each bucket is a linked list of nodes
void hash_table_destroy(hash_table_t *table);

bool hash_table_insert(hash_table_t *table, const KEY_TYPE *key, VALUE_TYPE value);
bool hash_table_delete(hash_table_t *table, const KEY_TYPE *key);
bool hash_table_search(hash_table_t *table, const KEY_TYPE *key, VALUE_TYPE *value);

#endif

#ifdef HASH_TABLE_IMPLEMENTATION

unsigned long djb2_hash(const KEY_TYPE *key)
{
    unsigned long hash = 5381;
    uint8_t *data = (uint8_t *)key;

    for (size_t counter = 0; counter < sizeof(KEY_TYPE); counter++)
    {
        hash = ((hash << 5) + hash) + data[counter];
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
            free(current);
            current = next;
        }
    }

    free(table->buckets);
    free(table);
}

bool hash_table_insert(hash_table_t *table, const KEY_TYPE *key, VALUE_TYPE value)
{
    if (!table || !key)
        return false;

    unsigned long hash = djb2_hash(key) % table->size;

    node_t *current = table->buckets[hash];
    while (current)
    {
        if (!memcmp(key, &current->key, sizeof(KEY_TYPE)))
        {
            current->value = value;
            return true;
        }
        current = current->next;
    }

    node_t *new_node = malloc(sizeof(node_t));
    if (!new_node)
        return false;

    memcpy(&new_node->key, key, sizeof(KEY_TYPE));

    new_node->value = value;
    new_node->next = table->buckets[hash];
    table->buckets[hash] = new_node;

    return true;
}

bool hash_table_delete(hash_table_t *table, const KEY_TYPE *key)
{
    if (!table || !key)
        return false;

    unsigned long hash = djb2_hash(key) % table->size;

    node_t *current = table->buckets[hash];
    node_t *prev = NULL;

    while (current)
    {
        if (!memcmp(key, &current->key, sizeof(KEY_TYPE)))
        {
            if (prev)
            {
                prev->next = current->next;
            }
            else
            {
                table->buckets[hash] = current->next;
            }

            free(current);
            return true;
        }

        prev = current;
        current = current->next;
    }

    return false;
}

bool hash_table_search(hash_table_t *table, const KEY_TYPE *key, VALUE_TYPE *value)
{
    if (!table || !key || !value)
        return false;

    unsigned long hash = djb2_hash(key) % table->size;

    node_t *current = table->buckets[hash];
    while (current)
    {
        if (!memcmp(key, &current->key, sizeof(KEY_TYPE)))
        {
            *value = current->value;
            return true;
        }
        current = current->next;
    }

    return false;
}

#endif