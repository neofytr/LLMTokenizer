#ifndef BPE_H
#define BPE_H

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdint.h>
#include <errno.h>
#include <stdbool.h>

#include "../../dyn_arr/inc/dyn_arr.h"
#include "../../hash_table/inc/hash_table.h"

typedef struct
{
    uint32_t a, b;
} pair_t;

typedef struct
{
    pair_t pair;
    uint32_t freq;
} pair_freq_t;

char *get_file(const char *path);
bool dump_pairs(const char *path, dyn_arr_t *pair_arr);
dyn_arr_t *read_pairs(const char *path);

void print_text(const uint32_t *text, int length);
void print_graph(dyn_arr_t *pair_arr, const char *png_name, bool add_ascii);

dyn_arr_t *compress(const char *path, uint32_t **encoding, size_t *len);
char *decompress(uint32_t *encoding, size_t len, dyn_arr_t *pair_arr);
void render_pairs(dyn_arr_t *pair_arr);
char *resolve_pair(uint32_t pair_index, dyn_arr_t *pair_arr, hash_table_t *memoization_table);

bool is_less(const void *a, const void *b);

#endif