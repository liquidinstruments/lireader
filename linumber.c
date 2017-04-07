//
//  linumber.c
//  liquidreader
//
//  Created by Antony Searle on 4/2/17.
//  Copyright © 2017 Antony Searle. All rights reserved.
//


#include "linumber.h"

#include <string.h>
#include <assert.h>


void li_number_ctor(li_number* self) {
    memset(self, 0, sizeof(li_number));
}

void li_number_dtor(li_number* self) {
}

double li_number_double(li_number x) {
    switch (x.type) {
        case 'u':
        case 'b':
        case 'p':
            
            // Check that the high bits are empty.  Shift by width >= 64
            // is undefined behaviour so don't use it.
            assert((x.width >= 64) || !(x.u64 >> x.width));
            // Check that the value is representable as a double
            assert(x.u64 == ((uint64_t)((double) x.u64)));
            return x.u64;
        case 's':
            // Check that the high bits are all zero or all one
            assert(!(x.i64 >> x.width) || !~(x.i64 >> (x.width-1)));
            li_number_fix_sign(&x);
            assert(x.i64 == ((int64_t)((double) x.i64)));
            return x.i64;
        case 'f':
            switch (x.width) {
                case 32:
                    return x.f32;
                case 64:
                    return x.f64;
                default:
                    assert(false); // Floating point numbers must have 32 or 64 bits
            }
            break;
    }
    return 0;
}

bool li_number_equal(li_number x, li_number y) {
    return li_number_double(x) == li_number_double(y);
}

void li_number_fix_sign(li_number* self) {
    assert(self);
    if (self->type == 's') {
        // Is the high bit set?
        if (self->u64 & (1ull << (self->width - 1))) {
            // Set high bits to 1
            self->i64 |= -1ll << (self->width);
        } else {
            // Set high bits to 0
            self->i64 &= ~(-1ll << (self->width));
        }
    }
}
