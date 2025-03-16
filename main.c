#include "bpe/inc/bpe.h"

int main(int argc, char **argv)
{
    if (argc < 2)
    {
        fprintf(stderr, "Usage: %s <file_path>\n", argv[0]);
        return EXIT_FAILURE;
    }

    uint32_t *text;
    size_t text_len;

    dyn_arr_t *pair_arr = compress(argv[1], &text, &text_len);
    if (!pair_arr)
    {
        return EXIT_FAILURE;
    }

    print_text(text, text_len);

    free(text);
    dyn_arr_free(pair_arr);
    return EXIT_SUCCESS;
}