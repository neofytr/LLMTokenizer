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

bool is_less(const void *a, const void *b);
char *get_file(const char *path);
void print_text(const uint32_t *text, int length);
void print_graph(dyn_arr_t *pair_arr, size_t len, const char *png_name, bool add_ascii);
bool dump_pairs(const char *path, dyn_arr_t *pair_arr, uint32_t len);
dyn_arr_t *read_pairs(const char *path);

dyn_arr_t *initialize_pair_array();
uint32_t *initialize_text_buffer(const char *file_path, int *text_size);
void free_resources(uint32_t *text, uint32_t *temp, char *text_buffer, dyn_arr_t *pair_arr);
uint32_t *encode(uint32_t *text, int *text_size, dyn_arr_t *pair_arr);

#endif