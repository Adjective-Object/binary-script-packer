#ifndef BINSCRIPT_UTILS
#define BINSCRIPT_UTILS

#include <stddef.h>
#include <stdint.h>

/**
 * General purpose helper functions for dealing with raw
 * binary data, particularly in dealing with non-byte
 * alligned data and endianness
 **/

////////////////////////
// ENDIANNESS HELPERS //
////////////////////////

#define IS_BIG_ENDIAN (*(uint16_t *)"\0\xff" < 0x100)

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
int memcmp_bits(void * a, void * b, size_t len);

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

/**
 * prints a block of data as a block of binary
 **/
void print_binary(void *bin, size_t bits);

#endif
