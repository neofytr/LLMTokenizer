#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "dyn_arr/inc/dyn_arr.h"

int main()
{
    // taken from wikipedia article for BPE
    const char *text = "The original BPE algorithm operates by iteratively replacing the most common contiguous sequences of characters in a target text with unused 'placeholder' bytes. The iteration ends when no sequences can be found, leaving the target text effectively compressed. Decompression can be performed by reversing this process, querying known placeholder terms against their corresponding denoted sequence, using a lookup table. In the original paper, this lookup table is encoded and stored alongside the compressed text.";
    int text_size = strlen(text);

    for (int counter = 0; counter < text_size - 1; counter++) // prevent underflow for zero text_size by using a signed iterator
    {
        char a = text[counter];
        char b = text[counter + 1];
        fprintf(stdout, "%c%c\n", a, b);
    }
    return EXIT_SUCCESS;
}