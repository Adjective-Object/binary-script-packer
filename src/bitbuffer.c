#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>

#include "bitbuffer.h"
#include "util.h"

void bitbuffer_advance(bitbuffer *b, size_t bits) {
    b->head_offset = b->head_offset + (bits % 8);
    b->buffer += bits / 8;

    if (b->remaining_bytes < bits / 8) {
        printf("error trying to advance past the end "
               "of a bitbuffer");
        exit(1);
    }

    b->remaining_bytes -= bits / 8;
    while (b->head_offset >= 8) {
        b->head_offset -= 8;
        b->buffer++;
        b->remaining_bytes--;
    }
}

void bitbuffer_init_from_buffer(bitbuffer *b, char *buffer, size_t bufsize) {
    b->buffer_origin = buffer;
    b->buffer = b->buffer_origin;

    b->head_offset = 0;

    b->buflen_max = bufsize;
    b->remaining_bytes = bufsize;
    b->buffer_controlled = false;
}

void bitbuffer_init(bitbuffer *b, size_t bufsize) {
    bitbuffer_init_from_buffer(b, (char *)malloc(sizeof(char) * bufsize),
                               bufsize);
    b->buffer_controlled = true;
}

bool bitbuffer_next(bitbuffer *b) {
    bool toRet = (*(b->buffer) >> (7 - b->head_offset)) & 1;
    bitbuffer_advance(b, 1);
    return toRet;
}

void bitbuffer_pop(void *t, bitbuffer *source, size_t bits) {
    char *target = (char *)t;

    /*
    printf("requesting %ld of %ld tracked (offset %d): ",
            bits,
            source->remaining_bytes * 8 + (8 - source->head_offset),
            source->head_offset);
    bitbuffer_print(source);
    */

    size_t bytes = bits2bytes(bits);
    for (size_t i = 0; i < bytes; i++) {
        if (source->head_offset != 0) {
            target[i] = source->buffer[i] << source->head_offset;
            if ((source->buffer + i + 1) >
                source->buffer_origin + source->buflen_max) {
                target[i] = target[i] | (source->buffer[i + 1] >>
                                         (7 - source->head_offset));
            }
        } else {
            target[i] = source->buffer[i];
        }
    }

    bitbuffer_advance(source, bits);
}

void bitbuffer_print(bitbuffer *b) {
    unsigned int cur_offset = b->head_offset;
    char *head = b->buffer;
    while (head != b->buffer + b->remaining_bytes) {
        printf("%d", *head >> (7 - cur_offset) & 1);
        if (++cur_offset >= 8) {
            cur_offset = 0;
            head++;
            printf(" ");
        }
    }
    printf("\n");
}

size_t bitbuffer_sprintf(void *out, bitbuffer *b) {
    void *origin = out;
    unsigned int cur_offset = b->head_offset;
    char *head = b->buffer;
    while (head != b->buffer + b->remaining_bytes) {
        out += sprintf(out, "%d", *head >> (7 - cur_offset) & 1);
        if (++cur_offset >= 8) {
            cur_offset = 0;
            head++;
            out += sprintf(out, " ");
        }
    }
    return out - origin;
}

size_t bitbuffer_sprintf_hex(void *out, bitbuffer *b) {
    void *origin = out;
    unsigned int cur_offset = b->head_offset;
    char *head = b->buffer;

    if (cur_offset != 0) {
        out += sprintf(out, "0b");
    }

    while (head != b->buffer + b->remaining_bytes) {
        if (cur_offset != 0) {
            out += sprintf(out, "%d", *head >> (7 - cur_offset) & 1);

            if (++cur_offset >= 8) {
                cur_offset = 0;
                head++;
                out += sprintf(out, " ");
            }

        } else {
            out += sprintf(out, "%02x", (unsigned char)*head);
            head++;
            if (head != b->buffer_origin + b->buflen_max) {
                out += sprintf(out, " ");
            }
        }
    }
    return out - origin;
}

void bitbuffer_free(bitbuffer *b) {
    if (b->buffer_controlled)
        free(b->buffer_origin);
}

void bitbuffer_writebit(bitbuffer *b, bool bit) {
    // zero the referenced bit in the buffer
    *(b->buffer) = *(b->buffer) & (~(1 << (7 - b->head_offset)));
    // the set it to bool
    *(b->buffer) = *(b->buffer) | (bit << (7 - b->head_offset));

    // advance the buffer
    bitbuffer_advance(b, 1);
}

void bitbuffer_writeblock(bitbuffer *b, void *block, size_t bits) {
    // TODO set in char blocks to be fast 'n junk
    char *head = block;
    size_t offset = 0;
    for (size_t i = 0; i < bits; i++) {
        bitbuffer_writebit(b, (*head >> (7 - offset)) & 1);

        if (++offset >= 8) {
            offset -= 8;
            head++;
        }
    }
}

void bitbuffer_write_int(bitbuffer *b, unsigned int val, size_t bits) {
    for (int i = bits - 1; i >= 0; i--) {
        bitbuffer_writebit(b, (val >> i) & 1);
    }
}
