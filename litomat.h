//
//  litomat.h
//  liquidreader
//
//  Created by Antony Searle on 19/10/16.
//  Copyright © 2016 Antony Searle. All rights reserved.
//

#ifndef litomat_h
#define litomat_h

#include <stdio.h> // for FILE
#include <stdint.h> // for uint64_t

#include "lireader.h"

#ifdef __cplusplus
extern "C" {
#endif

    // Convert a Liquid Instruments binary log file to a MATLAB version 5.0
    // MAT-file.  Files must be open for binary reading and writing
    // respectively.
    
    li_status li_to_mat(FILE* input,
                        FILE* output,
                        void (*callback)(void* user_ptr, uint64_t bytes_read, uint64_t bytes_written),
                        void* user_ptr);
    
    
#ifdef __cplusplus
}
#endif

#endif /* litomat_h */
