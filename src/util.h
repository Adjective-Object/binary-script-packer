#ifndef BINSCRIPT_UTILS
#define BINSCRIPT_UTILS

#include <stddef.h>
#include <stdint.h>

#define ANSI_COLOR_RED "\x1b[31m"
#define ANSI_COLOR_GREEN "\x1b[32m"
#define ANSI_COLOR_YELLOW "\x1b[33m"
#define ANSI_COLOR_BLUE "\x1b[34m"
#define ANSI_COLOR_MAGENTA "\x1b[35m"
#define ANSI_COLOR_CYAN "\x1b[36m"
#define ANSI_COLOR_RESET "\x1b[0m"

/**
 * General purpose helper functions for dealing with raw
 * binary data, particularly in dealing with non-byte
 * alligned data and endianness
 **/

////////////////////////
// ENDIANNESS HELPERS //
////////////////////////

#define BS_IS_BIG_ENDIAN (*(uint16_t *)"\0\xff" < 0x100)
#define BS_ENDIAN_MATCH(l)                                                     \
    ((BS_IS_BIG_ENDIAN) != (l->target_endianness == BS_BIG_ENDIAN))

/**
 * swaps the endianness of a field of specified width in place
 * addr: a pointer to the head of the data
 * size: the size of the field in bytes
 *
 * given input      01 02 0E FF, size = 4
 * produces output  FF 0E 02 01
 **/
void swap_endian_on_field(void *addr, size_t size);

/////////////////////////////////
// BIT-BASED MEMORY OPERATIONS //
/////////////////////////////////

/**
 * calculates the minium number of bytes to hold the number of
 * specified bits.
 *
 * bits2bytes(1) = 1
 * bits2bytes(8) = 1
 * bits2bytes(9) = 2
 **/
size_t bits2bytes(size_t bits);

/**
 * Compares the first <len> bits of the memory at addresses
 * a and b.
 *
 * return is in same form as memcmp
 *  return  0 -> a = b
 *  return >0 -> a > b
 *  return <0 -> a < b
 **/
int memcmp_bits(void *a, void *b, size_t len);

///////////////////
// PRINT HELPERS //
///////////////////

/**
 * prints a block of data as characters
 **/
void print_chars(void *bin, size_t size);

/**
 * prints a block of data as hexidecimal pairs
 **/
void print_hex(void *bin, size_t size);

void sprintf_hex(void *out, void *bin, size_t size);

/**
 * prints a block of data as a block of binary
 **/
void print_binary(void *bin, size_t bits);

/**
 * Frees a sequence of pointers
 * where head is the first of a list of pointers of length
 * count
 **/
void free_sequence(void *head, size_t count);

/**
 * xors two buffers and stores the result in a
 **/
void xor_buffer(void *a, void *b, size_t s);
void or_buffer(void *a, void *b, size_t s);
void and_buffer(void *a, void *b, size_t s);

#endif
