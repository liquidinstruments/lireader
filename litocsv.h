//
//  litocsv.h
//  liquidreader
//
//  Created by Antony Searle on 19/10/16.
//  Copyright © 2016 Antony Searle. All rights reserved.
//

#ifndef litocsv_h
#define litocsv_h

#include <stdio.h>

#include "lireader.h"

#ifdef __cplusplus
extern "C" {
#endif
    
    // Read a Liquid Instruments binary log file file and write a Comma
    // Separated Value (CSV) file.  Files must be open for binary reading and
    // writing respectively
    
    li_status li_to_csv(FILE* input, FILE* output);
    
#ifdef __cplusplus
}
#endif

#endif /* litocsv_h */
