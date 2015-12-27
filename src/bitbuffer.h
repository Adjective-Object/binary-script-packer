#ifndef BINSCRIPT_BITBUFFER
#define BINSCRIPT_BITBUFFER

#include <stddef.h>

typedef struct bitbuffer{
    char * buffer;
    char * buffer_origin;
    size_t remaining_bytes;
    size_t buflen_max;
    int head_offset; 
} bitbuffer;

bool bitbuffer_next(bitbuffer * buffer);
void bitbuffer_advance(bitbuffer * buffer, size_t bits);
void bitbuffer_init_from_buffer(bitbuffer * buffer,
        char * data_buffer, size_t len);
void bitbuffer_init(bitbuffer * buffer, size_t len);

void bitbuffer_pop(void * target, bitbuffer * source, size_t bits);
void bitbuffer_print(bitbuffer *b);

#endif

