#include <stddef.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <stdio.h>
#include <ctype.h>

#include "util.h"

size_t bits2bytes(size_t bits) { return (bits / 8) + ((bits % 8) ? 1 : 0); }

void print_chars(void *bin, size_t size) {
    unsigned char *c = (unsigned char *)bin;
    size_t i;
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
    unsigned char *c = (unsigned char *)bin;
    size_t i;

    for (i = 0; i < size; i++) {
        printf("%02x ", (unsigned char)(*c));
        c++;
    }
    printf("\n");
}

void print_binary(void *bin, size_t bits) {
    unsigned char *cbin = (unsigned char *)bin;
    for (size_t i = 0; i < bits; i++) {
        printf("%d", (*cbin >> (7 - (i % 8))) & 1);
        if (i % 8 == 7) {
            cbin++;
        }
    }
    printf("\n");
}

void swap_endian_on_field(void *addr, size_t size) {
    size_t i;
    char tmp;
    char *iter = (char *)addr;

    for (i = 0; i < size / 2; i++) {
        tmp = iter[i];
        iter[i] = iter[size - i - 1];
        iter[size - 1 - i] = tmp;
    }
}

int memcmp_bits(void *a, void *b, size_t len) {
    size_t byte = len / 8;
    size_t bit = len % 8;

    int mcp = (byte > 0) ? memcmp(a, b, byte) : 0;
    if (mcp != 0) {
        return mcp;
    } else if (bit != 0) {
        unsigned char filter = 0;
        for (int i = 7; i >= 8 - (int)bit; i--) {
            filter = filter | (1 << i);
        }

        // printf("a: %x, ", filter & ((char *) a) [byte]);
        // printf("b: %x\n", filter & ((char *) b) [byte]);

        return (filter & ((char *)a)[byte]) - (filter & ((char *)b)[byte]);
    }

    return mcp;
}

void free_sequence(void *head, size_t count) {
    // workaround to avoid complaints from type checker
    void **realhead = (void **)head;
    for (size_t i = 0; i < count; i++) {
        free(realhead[i]);
    }
}
