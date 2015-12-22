#include <stddef.h>
#include <stdbool.h>
#include <stdio.h>
#include <ctype.h>

#include "util.h"

size_t bits2bytes(size_t bits) {
    return (bits / 8) + ((bits % 8)? 1 : 0);
}

char * shiftBuffer(char * buffer, size_t size, size_t amount) {
    printf("buffer in  %d\n", *buffer);
    // start by shifting the characters
    size_t base_shift = amount / 8;
    for (size_t i = 0; i<size; i++) {
        size_t target = i + base_shift;
        if (target >= 0 && target < size) {
            buffer[target] = buffer[i];
        }
    }

    // determine the direction to shift the characters
    size_t remainder = amount % 8;
    size_t start, end, del;
    if (amount > 0 ){
        // right shift, traverse left to right
        start = 0;
        end = size;
        del = 1;
    } else {
        // left shift, traverse right to left
        start = size-1;
        end = -1;
        del = -1;
    }

    // traverse in the order declared above & set values
    char carry = 0;
    for (unsigned int i=start; i<end; i+=del) {
        char shifted = (buffer[i] >> remainder) | carry;
        carry = buffer[i] ^ (shifted << remainder);
        buffer[i] = shifted;
    }

    printf("buffer out %d\n", *buffer);
    return buffer;
}

void print_chars(void *bin, size_t size) {
    char *c = bin;
    int i;
    for (i = 0; i < size; i++) {
        if (isgraph(*c)) {
            printf("%c", *c);
        } else {
            printf(" ");
        }
        c++;
    }
}

void print_hex(void *bin, size_t size) {
    char *c = bin;
    int i;

    for (i = 0; i < size; i++) { 
        printf("%02x ", (unsigned char) (*c));
        c++;
    }
    printf("\n");
}

