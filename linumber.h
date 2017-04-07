//
//  linumber.h
//  liquidreader
//
//  Created by Antony Searle on 4/2/17.
//  Copyright © 2017 Antony Searle. All rights reserved.
//

#ifndef linumber_h
#define linumber_h

#include <stdio.h>
#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif
    
    typedef struct {
        union {
            int64_t   i64;
            uint64_t  u64;
            float     f32;
            double    f64;
        };
        char    type;
        size_t width;
    } li_number;
    
    void li_number_ctor(li_number* self);
    void li_number_dtor(li_number* self);
    
    double li_number_double(li_number x);
    bool   li_number_equal(li_number x, li_number y);
    void   li_number_fix_sign(li_number* self);
    
#ifdef __cplusplus
} // extern "C"
#endif


#endif /* linumber_h */
