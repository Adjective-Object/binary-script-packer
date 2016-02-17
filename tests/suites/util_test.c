#include <string.h>
#include <stdlib.h>

#include "../mutest.h"
#include "util.h"

void mu_test_bits2bytes() {
    mu_check(bits2bytes(0) == 0);
    mu_check(bits2bytes(1) == 1);
    mu_check(bits2bytes(2) == 1);
    mu_check(bits2bytes(3) == 1);
    mu_check(bits2bytes(4) == 1);
    mu_check(bits2bytes(5) == 1);
    mu_check(bits2bytes(6) == 1);
    mu_check(bits2bytes(7) == 1);
    mu_check(bits2bytes(8) == 1);
    mu_check(bits2bytes(9) == 2);
}

void mu_test_memcmp_bits() {
    unsigned char buffa[4] = { 255, 255, 255, 255 };
    unsigned char buffb[4] = { 255, 255, 255, 254 };

    mu_check(memcmp_bits(buffa, buffb, 1) == 0);
    mu_check(memcmp_bits(buffa, buffb, 31) == 0);
    mu_check(memcmp_bits(buffa, buffb, 32) > 0);
    mu_check(memcmp_bits(buffb, buffa, 32) < 0);
}

void mu_test_swap_endian_on_field() {
// test for even length buffer
#define SWAPBIT_BUFLEN_BYTES 4
    unsigned char *buff, *expt;

    // even length buffer
    buff = (unsigned char[4]){ 1, 2, 3, 4 };
    expt = (unsigned char[4]){ 4, 3, 2, 1 };
    swap_endian_on_field(buff, 4);
    mu_check(0 == memcmp(buff, expt, 4));

    // check does not alter memory after the block
    buff = (unsigned char[4]){ 3, 4, 2, 1 };
    expt = (unsigned char[4]){ 4, 3, 2, 1 };
    swap_endian_on_field(buff, 2);
    mu_check(0 == memcmp(buff, expt, 4));

    // check does not alter memory before the block
    buff = (unsigned char[4]){ 1, 2, 3, 4 };
    expt = (unsigned char[4]){ 1, 2, 4, 3 };
    swap_endian_on_field(buff + 2, 2);
    mu_check(0 == memcmp(buff, expt, 4));

    // check how it works for odd length fields
    buff = (unsigned char[4]){ 1, 2, 3, 4 };
    expt = (unsigned char[4]){ 3, 2, 1, 4 };
    swap_endian_on_field(buff, 3);
    mu_check(0 == memcmp(buff, expt, 4));
}
