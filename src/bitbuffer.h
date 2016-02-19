#ifndef BINSCRIPT_BITBUFFER
#define BINSCRIPT_BITBUFFER

#include <stddef.h>
#include <stdbool.h>

typedef struct bitbuffer {
    char *buffer;
    char *buffer_origin;
    size_t remaining_bytes;
    size_t buflen_max;
    int head_offset;
    bool buffer_controlled;
} bitbuffer;

/**
 * Initializes a bitbuffer from an existing data buffer
 **/
void bitbuffer_init_from_buffer(bitbuffer *buffer, char *data_buffer,
                                size_t len);

/**
 * Initializes a bitbuffer and allocates a data buffer of
 * specified length for the data buffer
 **/
void bitbuffer_init(bitbuffer *buffer, size_t len);

/**
 * Gets the next available bit in a bitbuffer and steps it
 * forward by 1 bit
 **/
bool bitbuffer_next(bitbuffer *buffer);

/**
 * steps a bitbuffer forward by a specified amount
 **/
void bitbuffer_advance(bitbuffer *buffer, size_t bits);

/**
 * copies a block of data from the head of a bitbuffer into
 * a given destination. Advances the bitbuffer past that data.
 *
 * target: the destination fro the data
 * source: the bitbuffer to copy from
 * bits: the number of bits to pop
 *
 **/
void bitbuffer_pop(void *target, bitbuffer *source, size_t bits);

/**
 * prints the remaining contents  of a bitbuffer in blocks of
 * 8 bits, with divisions between the internal bytes of the
 * bitbuffer.
 *
 * b: the bitbuffer to be printed
 **/
void bitbuffer_print(bitbuffer *b);

/**
 * prints the remaining contents  of a bitbuffer in blocks of
 * 8 bits, with divisions between the internal bytes of the
 * bitbuffer.
 *
 * out: the buffer to be printed to;
 * b: the bitbuffer to be printed
 **/
size_t bitbuffer_sprintf(void *out, bitbuffer *b);
size_t bitbuffer_sprintf_hex(void *out, bitbuffer *b);

/**
 * Writes a bit to the head of the bitbuffer, and advances
 * the bitbuffer by 1 bit
 *
 * buffer: the bitbuffer to be written to
 * bit: the bit that will be written in the current position of the bitbuffer
 **/
void bitbuffer_writebit(bitbuffer *buffer, bool bit);

/**
 * Writes a block of data to the head of the bitbuffer, and
 * advances the bitbuffer by the number of bits specified.
 *
 * buffer: the bitbuffer to operate on
 * block: the data to be copied into the bitbuffer
 * bits: the number of bits to be copied into the bitbuffer
 **/
void bitbuffer_writeblock(bitbuffer *buffer, void *block, size_t bits);

/**
 * Writes the rightmost n bits of a single value to the head of
 * the bitbuffer, and advances the bitbuffer by the number of bits
 * specified
 *
 * buffer: the bitbuffer to operate on
 * valuue: the value to be copied to the bitbuffer
 * bits: the number of bits to be copied to the bitbuffer
 **/
void bitbuffer_write_int(bitbuffer *buffer, unsigned int value, size_t bits);

/**
 * Cleans up internal structures allocated during bitbuffer_init
 * or bitbuffer_init_from_buffer. Does not free the actual
 * bitbuffer object.
 *
 * b: the bitbuffer to free
 **/
void bitbuffer_free(bitbuffer *b);

#endif
