//
//  litonpy.h
//  liquidreader
//
//  Created by Antony Searle on 01/11/17.
//  Copyright © 2017 Liquid Instruments. All rights reserved.
//

#ifndef litonpy_h
#define litonpy_h

#include <stdio.h> // for FILE
#include <stdint.h> // for uint64_t

#include "lireader.h"

#ifdef __cplusplus
extern "C" {
#endif
    
    // Read a Liquid Instruments binary log file file and write a v1.0 NumPy
    // (NPY) file.  Files must be open for binary reading and
    // writing respectively.  Optionally provide a callback that will report
    // whenever bytes are read from input or written to output.  user_ptr is
    // passed unchanged to the callback.
    
    li_status li_to_npy(FILE* input,
                        FILE* output,
                        void (*callback)(void* user_ptr, uint64_t bytes_read, uint64_t bytes_written),
                        void* user_ptr);

#ifdef __cplusplus
}
#endif

#endif /* litonpy_h */
