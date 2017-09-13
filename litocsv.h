//
//  litocsv.h
//  liquidreader
//
//  Created by Antony Searle on 19/10/16.
//  Copyright © 2016 Antony Searle. All rights reserved.
//

#ifndef litocsv_h
#define litocsv_h

#include <stdio.h> // for FILE
#include <stdint.h> // for uint64_t

#include "lireader.h"

#ifdef __cplusplus
extern "C" {
#endif
    
    // Read a Liquid Instruments binary log file file and write a Comma
    // Separated Value (CSV) file.  Files must be open for binary reading and
    // writing respectively.  Optionally provide a callback that will report
    // whenever bytes are read from input or written to output.  user_ptr is
    // passed unchanged to the callback.
    
    li_status li_to_csv(FILE* input,
                        FILE* output,
                        void (*callback)(void* user_ptr, uint64_t bytes_read, uint64_t bytes_written),
                        void* user_ptr);

#ifdef __cplusplus
}
#endif

#endif /* litocsv_h */
