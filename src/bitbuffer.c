#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>

#include "bitbuffer.h"
#include "util.h"

bool bitbuffer_next(bitbuffer * b){
    b->head_offset++;
    while (b->head_offset >= 8) {
        b->head_offset -= 8;
        b->buffer++;
        b->remaining_bytes--;
        if (b->remaining_bytes < 0) {
            printf("error trying to advance through empty buffer");
            exit(1);
        }
    }

    return (b->buffer[0] >> (8 - b->head_offset)) & 1;
}

void bitbuffer_init(bitbuffer * b, size_t bufsize) {
    b->buffer_origin = (char *) malloc(sizeof(char) * bufsize);
    b->buffer = b->buffer + bufsize;

    b->head_offset = 0;

    b->buflen_max = bufsize;
    b->remaining_bytes = 0;
}

void bitbuffer_pop(char * target, bitbuffer * source, size_t bits) {
    size_t bytes = bits2bytes(bits/8);
    for(size_t i=0; i<bytes; i++) {
        target[i] = 0;
        target[i] = target[i] | source->buffer[i] << source->head_offset;
        target[i] = target[i] | source->buffer[i+1] >> source->head_offset;
    }

}

void bitbuffer_print(bitbuffer *b) {
    unsigned int cur_offset = b->head_offset;
    char * head = b->buffer;
    while(head != b->buffer + b->remaining_bytes) {
        printf("%d", *head >> (7 - cur_offset) & 1);
        if (++cur_offset >= 8) {
            cur_offset = 0;
            head++;
            printf(" ");
        }
    }
    printf("\n");
}
