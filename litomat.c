//
//  limatlab.c
//  liquidreader
//
//  Created by Antony Searle on 11/10/16.
//  Copyright © 2016 Antony Searle. All rights reserved.
//

#include <stdlib.h>
#include <stdio.h>
#include <assert.h>
#include <string.h>
#include <ctype.h>
#include <time.h>

#include "lireader.h"
#include "limatlab.h"

#include "liutility.h"
#include "liparse.h"

#define REQUIRE_ALLOC(X) do { if (! X) { result = LI_BAD_ALLOC; LI_ON_ERROR; goto cleanup; } } while(false)
#define REQUIRE_SUCCESS do { if (result != LI_SUCCESS) { LI_ON_ERROR; goto cleanup; } } while(false)
#define CONTINUE_SMALL_AFTER(CLEANUP)  { if (result != LI_SUCCESS) { { CLEANUP; } if (result == LI_SMALL_SRC) continue; else { LI_ON_ERROR; goto cleanup; } } }
#define REQUIRE_FORMAT(X) do { if (!( X )) { result = LI_BAD_FORMAT; LI_ON_ERROR; goto cleanup; } } while(false)

// .mat format is (roughly speaking) transposed relative to .li format.  To
// reduce peak memory usage, we do the transpose on disk by appending to a
// temporary file for each column, then concatenating the temporary files to
// build the main portion of the .mat file

typedef struct {
    FILE* fp;
} pTF;

void pTF_ctor(pTF* self) {
    assert(self);
    self->fp = tmpfile();
}

void pTF_dtor(pTF* self) {
    assert(self);
    if (self && self->fp)
        fclose(self->fp);
}

li_array_define(pTF);

li_status li_to_mat(FILE* input,
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
    
    li_array(pTF) files;
    li_array_ctor(pTF)(&files);

    li_reader* r = li_init(malloc, free);
    mat_header* mh = NULL;

    REQUIRE_ALLOC(r);
    
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
            
            LI_FOR(Replacement, p, &replacements) {
                pTF f;
                pTF_ctor(&f);
                li_array_push(pTF)(&files, f);
            }
            
        }
        
        if (li_array_size(double)(&doubles)) {
            while((result = li_get(r, LI_RECORD_F64V, 0, doubles.begin, li_array_size(double)(&doubles) * sizeof(double))) == LI_SUCCESS) {
                double t = startOffset + timeStep * (rows++);
                pTF* iter = files.begin;
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
                    fwrite(&d, sizeof(double), 1, (iter++)->fp);
                    assert(m == 1);
                    // We don't report I/O with the temporary files to callback
                }
            }
            if (result != LI_SMALL_SRC) // We left the loop because of an error
                goto cleanup;
        }
    }
    REQUIRE_FORMAT(rows);
    // We finished the file and read at least one row
    result = LI_SUCCESS;

    // to report incremental progress in file we use ftell
    long old_offset = 0;
    long new_offset = 0;
    
    mh = mat_header_new(csvHeader);
    REQUIRE_ALLOC(mh);
    fwrite(mh, sizeof(mat_header), 1, output);

    long token = mat_element_open(output, miMATRIX);
    mat_array_write_flags(output, mxSTRUCT_CLASS);
    mat_array_write_dims2(output, 1, 1);
    mat_array_write_name(output, "moku");
#define FIELD_NAME_LENGTH 16
    int32_t length = FIELD_NAME_LENGTH;
    mat_array_write(output, miINT32, sizeof(length), &length);
    char fields[][FIELD_NAME_LENGTH] = {
        "comment",
        "data",
        "legend",
        "version",
        "timestamp"
    };
    mat_array_write(output, miINT8, sizeof(fields), fields);
    
    
    // Moku.comment
    {
        mat_matrix_write_utf8(output, csvHeader);
    }

    if (callback) {
        new_offset = ftell(output);
        callback(user_ptr, 0, new_offset - old_offset);
        old_offset = new_offset;
    }

    size_t columns = li_array_size(pTF)(&files);

    // Moku.data
    {
        long token2 = mat_element_open(output, miMATRIX);
        mat_array_write_flags(output, mxDOUBLE_CLASS);
        mat_array_write_dims2(output, (int32_t) rows, (int32_t) columns);
        mat_array_write_name(output, "");
        {
            long token3 = mat_element_open(output, miDOUBLE);
            
            LI_FOR (pTF, p, &files) {
                fseek(p->fp, 0, SEEK_SET);
                for (int j = 0; j != rows; ++j) {
                    // append the column to the output one value at a time
                    double value;
#ifndef NDEBUG
                    size_t n =
#endif
                    fread(&value, sizeof(double), 1, p->fp);
                    assert(n == 1);
                    fwrite(&value, sizeof(double), 1, output);
                }
                // Close early to reduce maximum footprint on disk
                fclose(p->fp);
                p->fp = 0;
                if (callback) {
                    new_offset = ftell(output);
                    callback(user_ptr, 0, new_offset - old_offset);
                    old_offset = new_offset;
                }
            }
            mat_element_close(output, token3);
        }
        mat_element_close(output, token2);
    }
    
    // Moku.legend
    {
        long token2 = mat_element_open(output, miMATRIX);
        mat_array_write_flags(output, mxCELL_CLASS);
        mat_array_write_dims1(output, (int32_t) columns);
        mat_array_write_name(output, "");

        // Assumes header is something like
        // % Free text
        // % Name, Name, Name
        
        // Split header into lines
        li_array(li_string) lines = li_string_split(&csvHeader, "\n\r");
        // Split lines by commas until we find one with the right number of elements
        li_array(li_string) names;
        li_array_ctor(li_string)(&names);
        LI_REVERSE_FOR(li_string, p, &lines) {
            li_array_dtor(li_string)(&names);
            names = li_string_split(p, ",");
            if (li_array_size(li_string)(&names) == columns)
                break;
        }
        li_array_dtor(li_string)(&lines); // No longer needed
        LI_FOR(li_string, p, &names) {
            li_string_lstrip(p, "%#/ \t"); // Strip comments and whitespace from left
            li_string_rstrip(p, " \t"); // Strip whitespace from right
        }
        while (li_array_size(li_string)(&names) < columns)
            li_array_push(li_string)(&names, li_string_copy("?"));
        for (size_t i = 0; i != columns; ++i)
            mat_matrix_write_utf8(output, names.begin[i]);
        li_array_dtor(li_string)(&names);
        mat_element_close(output, token2);
    }
    
    // Moku.version
    {
        mat_matrix_write_utf8(output, li_version_string());
    }
    
    // Moku.timestamp
    {
        time_t t = time(NULL);
#       define N 64
        char buf[N];
        strftime(buf, N, "%Y-%m-%d T %H:%M:%S %z", localtime(&t));
        mat_matrix_write_utf8(output, buf);
    }
    
    mat_element_close(output, token); // close the mat_struct

    if (callback) {
        new_offset = ftell(output);
        callback(user_ptr, 0, new_offset - old_offset);
        old_offset = new_offset;
    }

cleanup:

    mat_header_delete(mh);
    li_array_dtor(pTF)(&files);
    li_array_dtor(Replacement)(&replacements);
    li_string_dtor(&csvHeader);
    li_string_dtor(&csvFmt);
    li_array_dtor(double)(&doubles);
    li_array_dtor(li_byte)(&buffer);
    li_finalize(r);
    
    fflush(output);
    
    // Read the file back
    // fseek(output, 0, SEEK_SET);
    // mat_file_print(output);
    
    return result;
    
}
