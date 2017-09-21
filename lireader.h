//
//  lireader.h
//  liquidreader
//
//  Created by Antony Searle on 21/06/2016.
//  Copyright (c) 2016 Liquid Instruments. All rights reserved.
//

#ifndef lireader_h
#define lireader_h

#ifdef __cplusplus
extern "C" {
#endif
    
#include <stddef.h> // for size_t
    
    // Status codes returned by
    
    typedef enum li_status {
        LI_SUCCESS = 0,          // Operation completed successfully
        LI_INVALID_ARGUMENT = 1, // Arguments were inconsistent
        LI_BAD_ALLOC = 2,        // Memory allocation failed.  The state of the reader object is unchanged.
        LI_SMALL_SRC = 3,        // There is not enough input data to fulfill the request.  Provide more with li_put.
        LI_SMALL_DEST = 4,       // The destination buffer is too small to return the requested target in.
        LI_BAD_FORMAT = 5,       // The data isn't a valid Liquid Instruments binary log file
        LI_UNIMPLEMENTED = 6,    // Unsupported feature
    } li_status;
    
    // Quantities that can be extracted from the file
    
    typedef enum li_target {
        LI_SUGGESTED_PUT_U64 = 0,      // How many more bytes are needed before the parser can progress
        LI_CHANNEL_SELECT_U8 = 1,      // Bitfield showing which channels appear in the file
        LI_INSTRUMENT_ID_U64 = 2,      // ID number of the instrument that produced the data
        LI_INSTRUMENT_VERSION_U64 = 3, // Version number of the instrument that produced the data
        LI_TIME_STEP_F64 = 4,          // Sample time step
        LI_START_TIME_U64 = 5,         // Crude start time in whole seconds
        LI_RECORD_BYTES_U64 = 6,       // Size of ...
        LI_RECORD_F64V = 7,            // ... array of 64-bit floating point numbers representing all values in one record
        LI_FMT_STRING_BYTES_U64 = 8,   // Size (inculding null terminating character) of ...
        LI_FMT_STRING_UTF8V = 9,       // ... UTF8 string specifying CSV format
        LI_HDR_STRING_BYTES_U64 = 10,  // Size (including null terminating character) of ...
        LI_HDR_STRING_UTF8V = 11,      // ... UTF8 string specifying CSV header
        LI_START_OFFSET_F64 = 12,      // Sample time offset
        LI_COUNT_FOR_INDEX_U64 = 13,   // Number of records in channel[index], ncessary to interpret packing into RECORD_BYTES
    } li_target;
    
    // Forward declaration of the opaque reader object.
    
    typedef struct li_reader li_reader;
    
    
    // Initialize a reader object.  All memory management will be performed with
    // alloc and dealloc, whose signatures match C standard library malloc and
    // free
    
    li_reader* li_init(void* (*alloc)(size_t),
                       void (*dealloc)(void*));
    
    
    // Finalize a reader object and release all memory allocated during its use
    
    void li_finalize(li_reader* reader);
    
    
    // Put count bytes of data from src into reader's internal buffer for future
    // processing
    
    li_status li_put(li_reader* reader,
                          const void* src,
                          size_t count);
    
    
    // Attempt to parse the previously li_put data and return the requested
    // target in the destination buffer
    
    li_status li_get(li_reader* reader,
                     li_target target,
                     size_t index,
                     void* dest,
                     size_t count);
    
    
    // Return a human-readable interpretation of an li_status code
    
    const char* li_status_string(li_status status);
    
    // Return a the library version number as a string
    
    const char* li_version_string(void);
    
    
    
#ifdef __cplusplus
}
#endif

#endif /* lireader_h */
