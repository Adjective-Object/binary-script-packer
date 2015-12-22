#ifndef BINSCRIPT_UTILS
#define BINSCRIPT_UTILS

#include <stddef.h>

size_t bits2bytes(size_t bits);

/**
 * Shift a buffer by a set amount
 **/
char * shiftBuffer(char * buffer, size_t size, size_t amount);

void print_chars(void *bin, size_t size);
void print_hex(void *bin, size_t size);

#endif
