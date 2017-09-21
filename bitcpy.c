//
//  bitcpy.c
//  scratch
//
//  Created by Antony Searle on 19/10/17.
//  Copyright © 2017 Liquid Instruments. All rights reserved.
//

#include "bitcpy.h"

extern inline void _bitcpy_aligned(int* dest /* dest != 0 */ ,
                            const int* src /* src != 0 */,
                            int srcn, /* 0 <= srcn <= 32 */
                                   const int* end /* src < end */) ;


extern inline void _bitcpy_short(unsigned char* dest, /* dest != 0 */
                          int destn, /* 0 <= destn <= 8 */
                          unsigned char* src, /* src != 0 */
                          int srcn, /* 0 <= srcn <= 8 */
                                 int nbits /* 0 <= nbits <= 56 - srcn */);

extern inline void bitcpy(void* dest,
                   int destn,
                   const void* src,
                   int srcn,
                          size_t nbits);



/* Testing */

int _bitcpy_test_getbit(void* ptr, int n) {
    return (((unsigned char*) ptr)[n >> 3] >> (n & 7)) & 1;
}

void _bitcpy_test_setbit(void* ptr, int n, int value) {
    unsigned char* q = ((unsigned char*) ptr) + (n >> 3);
    unsigned char mask = 1 << (n & 7);
    if (value) {
        *q |= mask;
    } else {
        *q &= ~mask;
    }
}

void _bitcpy_test() {

    unsigned char* a = calloc(1000, 1);
    unsigned char* b = calloc(1000, 1);

    for (int i = 0; i != 1000 * 8; ++i) {
        _bitcpy_test_setbit(a, i, rand() & 1);
    }

    uint64_t t = 0;

    for (int n = 0; n != 1000; ++n) {
        for (int i = 0; i != 64; ++i) {
            for (int j = 0; j != 64; ++j) {
                int c = rand() & 1 ? 0 : -1;
                memset(b, c, 1000);
                bitcpy(a + 16, i, b + 16, j, n);
                for (int k = -64; k != 0; ++k) {
                    // check that the edges were not disturbed
                    assert(_bitcpy_test_getbit(b + 16, i + k) == (c & 1));
                }
                for (int k = 0; k != n; ++k) {
                    assert(_bitcpy_test_getbit(a + 16, i + k) == _bitcpy_test_getbit(b + 16, j + k));
                    ++t;
                }
                for (int k = n; k != n + 64; ++k) {
                    // check that the edges were not disturbed
                    assert(_bitcpy_test_getbit(b + 16, i + k) == (c & 1));
                }
            }
        }
    }

    printf("Passed %lld\n", t);

    free(b);
    free(a);
}
