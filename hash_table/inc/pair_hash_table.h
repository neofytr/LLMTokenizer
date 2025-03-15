#ifndef PAIR_HASH_TABLE_H
#define PAIR_HASH_TABLE_H

#include <stdlib.h>
#include <stdbool.h>
#include <stdint.h>

typedef struct node
{
    uint32_t key[2];
    size_t value;
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

bool hash_table_insert(hash_table_t *table, const uint32_t key_one, const uint32_t key_two, size_t value);
bool hash_table_delete(hash_table_t *table, const uint32_t key_one, const uint32_t key_two);
bool hash_table_search(hash_table_t *table, const uint32_t key_one, const uint32_t key_two, size_t *value);

#endif