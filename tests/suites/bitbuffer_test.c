#include <time.h>
#include <string.h>
#include <stdlib.h>
#include <stdint.h>

#include "../mutest.h"
#include "bitbuffer.h"
#include "util.h"

inline void check_bitbuffer_invariants(bitbuffer b)
    __attribute__((always_inline));
inline void check_bitbuffer_location(bitbuffer b, size_t bits)
    __attribute__((always_inline));

inline void check_bitbuffer_invariants(bitbuffer b) {
    // check the position in the buffer plus the remaining bytes
    // is the end of the buffer
    mu_check(b.buffer_origin + 
            (b.buflen_max - b.remaining_bytes) == b.buffer);

    // check that the head_offset is between 0 and 7
    mu_check(b.head_offset >= 0);
    mu_check(b.head_offset <= 7);
}

inline void check_bitbuffer_location(bitbuffer b, size_t bits) {
    mu_check(bits == 
            (size_t) (b.buffer - b.buffer_origin) * 8 + b.head_offset);
}

void mu_test_bitbuffer_init_free_heap() {
    size_t size = sizeof(char) * 100;
    bitbuffer b;

    bitbuffer_init(&b, size);
    
    mu_check(b.buflen_max == size);
    check_bitbuffer_invariants(b);
    mu_check(b.buffer_controlled);

    bitbuffer_free(&b); 
}

void mu_test_bitbuffer_init_free_stack() { 
    size_t size = sizeof(char) * 100;
    char buff[size];
    bitbuffer b;
    bitbuffer_init_from_buffer(&b, buff, size);

    mu_check(b.buflen_max == size);
    mu_check(b.buffer == buff);
    mu_check(!b.buffer_controlled);
    check_bitbuffer_invariants(b);
    
    bitbuffer_free(&b); 
}

void mu_test_bitbuffer_advance_sequential() {
    size_t numchars = 10;
    bitbuffer b;
    for (size_t step = 1; step<numchars; step++) {
        bitbuffer_init(&b, sizeof(char) * numchars);

        int i;
        for(i=0; i<(int)numchars; i+=step) {
            mu_check(b.buffer - b.buffer_origin == i/8);
            bitbuffer_advance(&b, step);
            check_bitbuffer_invariants(b);
        }
        mu_check(b.buffer - b.buffer_origin == i/8);

        bitbuffer_free(&b);
    }
}

void mu_test_bitbuffer_next() {
    bitbuffer b;
    char buff[] = "this is a test array";
    bitbuffer_init_from_buffer(&b, (char *) &buff, strlen(buff));

    for(int c=0; c<(int) strlen(buff); c++) {
        for (int i = 7; i>= 0; i--) {
            bool next = bitbuffer_next(&b);
            bool expected_next = ((buff[c] >> i) & 1);
            //mu_print(MU_SUITE, "%d %d : %d == %d\n",
            //    c, i, next, expected_next);

            mu_check(next == expected_next);
            // one greater than current because we already advanced
            // past current
            check_bitbuffer_location(b, c * 8 + (8 - i));
            check_bitbuffer_invariants(b);
        }
    }

    bitbuffer_free(&b);
}

void mu_test_bitbuffer_pop() {
    bitbuffer b;
    
    #define POP_BUFLEN_BYTES 21
    // initialize the destination buffers to prevent valgrind
    // from complaining becase it doesn't understand bitshifts
    char buff[POP_BUFLEN_BYTES] = "this is a test array";
    char dest[POP_BUFLEN_BYTES] = "                    ";

    for(size_t c=0; c<POP_BUFLEN_BYTES * 8; c++) {
        bitbuffer_init_from_buffer(
                &b, (char *) &buff, POP_BUFLEN_BYTES);
        bitbuffer_pop(&dest, &b, c);

        mu_check(0 == memcmp_bits(buff, dest, c+1));
        check_bitbuffer_invariants(b);
        check_bitbuffer_location(b, c);
        bitbuffer_free(&b);
    };
}

void mu_test_bitbuffer_writebit() {
    bitbuffer b;

    #define WRITEBIT_BUFLEN_BYTES 21
    // initialize the destination buffers to prevent valgrind
    // from complaining becase it doesn't understand bitshifts
    char buff[WRITEBIT_BUFLEN_BYTES] = "this is a test array";
    char dest[WRITEBIT_BUFLEN_BYTES] = "                    ";

    bitbuffer_init_from_buffer(&b, dest, WRITEBIT_BUFLEN_BYTES);

    for (size_t i = 0; i<WRITEBIT_BUFLEN_BYTES * 8; i++) {
        bitbuffer_writebit(&b, (buff[i/8] >> (7 - i%8)) & 1);
        mu_check(0 == memcmp_bits(buff, dest, i+1));
        check_bitbuffer_invariants(b);
        check_bitbuffer_location(b, i + 1);
    }

    bitbuffer_free(&b);
}

void mu_test_bitbuffer_writeblock() {
    bitbuffer b;

    #define WRITEBLOCK_BUFLEN_BYTES 21
    char buff[WRITEBLOCK_BUFLEN_BYTES] = "this is a test array";

    for(size_t i = 1; i<=WRITEBLOCK_BUFLEN_BYTES * 8; i++) {
        bitbuffer_init(&b, WRITEBLOCK_BUFLEN_BYTES);

        memset(b.buffer_origin, 1, WRITEBIT_BUFLEN_BYTES);
        bitbuffer_writeblock(&b, buff, i);

        mu_check(0 == memcmp_bits(buff, b.buffer_origin, i));
        bitbuffer_free(&b);

        check_bitbuffer_location(b, i);
        check_bitbuffer_invariants(b);
    }
}

