//
//  litonpy.c
//  liquidreader
//
//  Created by Antony Searle on 01/11/17.
//  Copyright © 2016 Liquid Instruments. All rights reserved.
//

#include <stdlib.h>
#include <stdio.h>
#include <assert.h>
#include <string.h>
#include <ctype.h>
#include <time.h>

#include "lireader.h"

#include "liutility.h"
#include "liparse.h"

#define REQUIRE_ALLOC(X) do { if (! X) { result = LI_BAD_ALLOC; LI_ON_ERROR; goto cleanup; } } while(false)
#define REQUIRE_SUCCESS do { if (result != LI_SUCCESS) { LI_ON_ERROR; goto cleanup; } } while(false)
#define CONTINUE_SMALL_AFTER(CLEANUP)  { if (result != LI_SUCCESS) { { CLEANUP; } if (result == LI_SMALL_SRC) continue; else { LI_ON_ERROR; goto cleanup; } } }
#define REQUIRE_FORMAT(X) do { if (!( X )) { result = LI_BAD_FORMAT; LI_ON_ERROR; goto cleanup; } } while(false)

li_status li_to_npy(FILE* input,
                    FILE* output,
                    void (*callback)(void* user_ptr, uint64_t bytes_read, uint64_t bytes_written),
                    void* user_ptr)
{

    // Todo: reduce duplication with li_to_mat
    
    li_status result = LI_SUCCESS;
    
    li_array(li_byte) buffer;
    li_array_ctor(li_byte)(&buffer);
    
    double timeStep = 0.0;
    double startOffset = 0.0;
    
    li_array(Replacement) replacements;
    li_array_ctor(Replacement)(&replacements);
    
    li_array(double) doubles;
    li_array_ctor(double)(&doubles);
    
    li_string csvFmt;
    li_string_ctor(&csvFmt);
    
    li_string csvHeader;
    li_string_ctor(&csvHeader);
    
    long rows = 0;

    li_reader* r = li_init(malloc, free);

    REQUIRE_ALLOC(r);

#define NPY_HDR_SIZE 96
    // We need to know rows and columns to write the .npy header, so skip over it
    fseek(output, NPY_HDR_SIZE, SEEK_SET);

    while (!feof(input)) {
        // Ask the reader how much data it wants to complete the next
        // section of the file
        uint64_t n = 0;
        result = li_get(r, LI_SUGGESTED_PUT_U64, 0, &n, sizeof(n));
        REQUIRE_SUCCESS;

        if (n > li_array_size(li_byte)(&buffer)) {
            li_array_resize(li_byte)(&buffer, (size_t) n, 0);
            REQUIRE_SUCCESS;
        }
        
        // Read up to read_buffer_size bytes from the file, which is at least
        // as many as suggested, and set n to the bytes actually read
        n = fread(li_array_begin(li_byte)(&buffer), 1, li_array_size(li_byte)(&buffer), input);
        if (callback)
            callback(user_ptr, n, 0);
        // Give n bytes to the reader
        result = li_put(r, li_array_begin(li_byte)(&buffer), (size_t) n);
        REQUIRE_SUCCESS;

        if (li_array_empty(double)(&doubles)) {
            
            // As li_reader doesn't parse the header until all of it is
            // available, if the first get succeeds all following gets for
            // metadata will succeed.
            
            uint64_t bytes = 0;
            result = li_get(r, LI_RECORD_BYTES_U64, 0, &bytes, sizeof(bytes));
            CONTINUE_SMALL_AFTER();
            li_array_resize(double)(&doubles, (size_t) bytes / sizeof(double), 0.0);
            
            LI_TRUST(li_get(r, LI_TIME_STEP_F64, 0, &timeStep, sizeof(timeStep)));
            LI_TRUST(li_get(r, LI_START_OFFSET_F64, 0, &startOffset, sizeof(startOffset)));
            
            uint8_t channel_select = 0;
            LI_TRUST(li_get(r, LI_CHANNEL_SELECT_U8, 0, &channel_select, sizeof(channel_select)));
            
            LI_TRUST(li_get(r, LI_FMT_STRING_BYTES_U64, 0, &bytes, sizeof(bytes)));
            LI_TRUST(li_string_resize(&csvFmt, (size_t) bytes - 1, 'x'));
            LI_TRUST(li_get(r, LI_FMT_STRING_UTF8V, 0, csvFmt, (size_t) bytes));
            
            li_array_dtor(Replacement)(&replacements);
            replacements = li_parse_Replacement_list(csvFmt);
            // Replacement list is tuples like {"ch2", 3, ".16e"}
            // We need to convert into a flat index into doubles
            // To do this we need to know how many items per channel
            uint64_t deltas[10] = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 };
            for (size_t i = 1; i != 9; ++i) {
                // Allowed to fail for channels that don't exist, leaving deltas[i] unchanged
                li_get(r, LI_COUNT_FOR_INDEX_U64, i, deltas + i, sizeof(uint64_t));
                // Cumulative sum
                deltas[i] += deltas[i - 1];
            }
            LI_FOR (Replacement, p, &replacements) {
                if (p->identifier
                    && (p->identifier[0] == 'c')
                    && (p->identifier[1] == 'h')
                    && isdigit(p->identifier[2])
                    && !p->identifier[3]) {
                    p->index += deltas[(p->identifier[2] - '0') - 1];
                    // Index now reflects where channel starts in packed data
                }
                assert(p->index < li_array_size(double)(&doubles));
            }
            
            LI_TRUST(li_get(r, LI_HDR_STRING_BYTES_U64, 0, &bytes, sizeof(bytes)));
            li_string_resize(&csvHeader, (size_t) bytes - 1, 'x');
            LI_TRUST(li_get(r, LI_HDR_STRING_UTF8V, 0, csvHeader, (size_t) bytes));

        }
        
        if (li_array_size(double)(&doubles)) {
            long bytes_written = 0;
            while((result = li_get(r, LI_RECORD_F64V, 0, doubles.begin, li_array_size(double)(&doubles) * sizeof(double))) == LI_SUCCESS) {
                double t = startOffset + timeStep * (rows++);
                LI_FOR(Replacement, p, &replacements) {
                    double d = 123456789;
                    switch (p->identifier[0]) {
                        case 't':
                            d = t;
                            break;
                        case 'n':
                            d = rows - 1;
                            break;
                        case 'c':
                            d = doubles.begin[p->index];
                            break;
                        default:
                            d = 0;
                            assert(false);
                            break;
                    }
#ifndef NDEBUG
                    size_t m =
#endif
                    fwrite(&d, sizeof(double), 1, output);
                    assert(m == 1);
                    bytes_written += sizeof(double);
                    // We don't report I/O with the temporary files to callback
                }
            }
            if (result != LI_SMALL_SRC) // We left the loop because of an error
                goto cleanup;
            if (callback)
                callback(user_ptr, 0, bytes_written);
        }
    }
    REQUIRE_FORMAT(rows);
    // We finished the file and read at least one row
    result = LI_SUCCESS;

    long columns = li_array_size(Replacement)(&replacements);

    // We can now write the header

    fseek(output, 0, SEEK_SET);
    fwrite("\x93NUMPY\x01\x00", 1, 8, output);
    uint16_t HEADER_LEN = NPY_HDR_SIZE - 10;
    fwrite(&HEADER_LEN, 2, 1, output);
    fprintf(output, "{'descr': '<f8', 'fortran_order': False, 'shape': (%ld, %ld), }", rows, columns);
    for (long i = ftell(output); i != NPY_HDR_SIZE - 1; ++i)
        fwrite(" ", 1, 1, output);
    fwrite("\n", 1, 1, output);

    if (callback)
        callback(user_ptr, 0, NPY_HDR_SIZE);

cleanup:

    li_array_dtor(Replacement)(&replacements);
    li_string_dtor(&csvHeader);
    li_string_dtor(&csvFmt);
    li_array_dtor(double)(&doubles);
    li_array_dtor(li_byte)(&buffer);
    li_finalize(r);
    
    fflush(output);
    
    return result;
    
}
