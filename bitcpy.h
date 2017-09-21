//
//  bitcpy.h
//  scratch
//
//  Created by Antony Searle on 19/10/17.
//  Copyright © 2017 Liquid Instruments. All rights reserved.
//

#ifndef bitcpy_h
#define bitcpy_h

#include <assert.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef __cplusplus
extern "C" {
#endif

/* bitcpy

 In analogy to memcpy, copies n bits between locations specified by a pointer
 and an additional integer offset into the byte.

 dest  : Pointer to destnation range.  Must be non-null
 destn : Bit offset relative to pointer
 src   : Pointer to source range.  Must be non-null
 srcn  : Bit offset relative to pointer
 nbits : Number of bits to copy

 The offsets destn and srcn can take on any value as long as the address
 (((unsigned char*) pointer) + (offset >> 3)) is not outside the bounds of the
 underlying allocation.

 Assumes a little-endian architecture.

 Example:

 int src[] = { 123, 456 };
 int dest[] = { 0, 0 };
 bitcpy(dest, 1, src, 0, 63);
 assert(dest[0] == 246);
 assert(dest[1] == 912);

 */

inline void bitcpy(void* dest,
                   int destn,
                   const void* src,
                   int srcn,
                   size_t nbits);



/* _bitcpy_aligned

 Helper has fast copy for the interior of ranges, but alignment requirements
 on the endpoints

 */

inline void _bitcpy_aligned(int* dest /* dest != 0 */ ,
                            const int* src /* src != 0 */,
                            int srcn, /* 0 <= srcn <= 32 */
                            const int* end /* src < end */) {
    assert(dest);
    assert(src);
    assert((0 <= srcn) && (srcn <= 32));
    assert(src < end);
    do {
        uint64_t x;
        memcpy(&x, src, 8);  /* load into 64 bit register */
        x >>= srcn;          /* right shift */
        memcpy(dest, &x, 4); /* store from low 32 bits */
        ++dest;
        ++src;
    } while (src != end);
}



/* _bitcpy_short

 Helper implements general shifts but only for small bit counts that fit in
 a register.

 */

 inline void _bitcpy_short(unsigned char* dest, /* dest != 0 */
                          int destn, /* 0 <= destn <= 8 */
                          unsigned char* src, /* src != 0 */
                          int srcn, /* 0 <= srcn <= 8 */
                          int nbits /* 0 <= nbits <= 56 - srcn */) {
    assert(dest);
    assert((0 <= destn) && (destn <= 8));
    assert(src);
    assert((0 <= srcn) && (srcn <= 8));
    assert((0 <= nbits) && (nbits <= 56 - srcn));
    const size_t n = (srcn + nbits + 7) >> 3;
    const size_t m = (destn + nbits + 7) >> 3;
    uint64_t x, y, mask;
    memcpy(((unsigned char*) &x) + 1, src, n);
    memcpy(&y, dest, m);
    mask = (~(-1LL << nbits)) << destn;
    y = (y & ~mask) | ((x >> (srcn + 8 - destn)) & mask);
    memcpy(dest, &y, m);
}



/* bitcpy

 In analogy to memcpy, copies n bits between locations specified by a pointer
 and an additional integer offset into the byte.

 Assumes a little-endian architecture.

 */

 inline void bitcpy(void* dest,
                   int destn,
                   const void* src,
                   int srcn,
                   size_t nbits) {
    assert(dest);
    assert(src);
    assert(0 <= nbits);

    uintptr_t a = (uintptr_t) dest;
    uintptr_t b = (uintptr_t) src;
    a += destn >> 3;
    b += srcn >> 3;
    destn &= 7;
    srcn &= 7;

    // Canonical form for byte alignement

    if (!destn && !srcn && !(nbits & 7)) {
        memcpy(dest, src, nbits >> 3);
    }

    if (srcn + nbits < 56) {
        // shift can be done in one register
        _bitcpy_short((unsigned char *) a,
                      destn,
                      (unsigned char*) b,
                      srcn,
                      (int) nbits);
        return;
    }

    if (srcn == destn) {
        int m = 8 - srcn;
        // replace with specialized version for one byte, one edge
        _bitcpy_short((unsigned char*) a, destn, (unsigned char*) b, srcn, m);
        ++a;
        ++b;
        nbits -= m;
        int p = (int) (nbits >> 3);
        memcpy((void*) a, (void*) b, p);
        _bitcpy_short((unsigned char*) a + p,
                      0,
                      (unsigned char*) b + p,
                      0,
                      nbits & 7);
    }

    int m = 0;
    if (destn || (a & 3)) {
        // copy until the destination is aligned
        m = (int) (32 - (destn + ((a & 3) << 3)));
        assert(m < nbits);
        _bitcpy_short((unsigned char*) a,
                      destn,
                      (unsigned char*) b,
                      srcn,
                      m);
        destn += m;
        srcn += m;
        nbits -= m;
    }

    a += destn >> 3;
    b += srcn >> 3;
    destn = (destn & 7) | (((int) a & 3) << 3);
    srcn = (srcn & 7) | (((int) b & 3) << 3);
    a &= ~3;
    b &= ~3;

    // Canonical form for 4-byte alignment

    assert(!destn);
    assert((0 <= srcn) && (srcn < 32));

    int p = (int) nbits & ~31;

    if (p) {
        _bitcpy_aligned((int*) a, (const int*) b, srcn, ((const int*) b) + (p >> 5));
    }

    nbits -= p;
    destn += p;
    srcn += p;
    a += destn >> 3;
    b += srcn >> 3;
    destn &= 7;
    srcn &= 7;

    // Canonical form for 1-byte alignment

    _bitcpy_short((unsigned char *) a,
                  destn,
                  (unsigned char*) b,
                  srcn,
                  (int) nbits);

}


#ifdef __cplusplus
} // extern "C"
#endif


#endif /* bitcpy_h */
