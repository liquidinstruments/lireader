//
//  limatlab.h
//  matfile
//
//  Created by Antony Searle on 18/07/2015.
//  Copyright (c) 2015 Antony Searle. All rights reserved.
//

#ifndef __matfile__LIMatFile__
#define __matfile__LIMatFile__

#include <stdint.h>
#include <stdio.h>

#ifdef __cplusplus
extern "C" {
#endif
    
    // MATLAB 5.0 MAT-file I/O
    
    // Files are composed of a header followed by one or more named matrices.
    // Currently only supports files written by little-endian systems.
    
    // MAT-file header
    typedef struct mat_header
    {
        char description[116]; // Descriptive text; first 4 bytes must be nonzero
        char offset[8];        // Unused
        int16_t version;       // 0x0100
        int16_t endian;        // Check to determine endianness of file
    } mat_header;
    
    // MAT-File Data Types
    typedef enum mat_type /* : int32_t */
    {
        miINVALID       = 0,
        miINT8          = 1,
        miUINT8         = 2,
        miINT16         = 3,
        miUINT16        = 4,
        miINT32         = 5,
        miUINT32        = 6,
        miSINGLE        = 7,
        miDOUBLE        = 9,
        miINT64         = 12,
        miUINT64        = 13,
        miMATRIX        = 14,
        miCOMPRESSED    = 15,
        miUTF8          = 16,
        miUTF16         = 17,
        miUTF32         = 18
    } mat_type;
    
    // MATLAB Array Types (Classes)
    typedef enum mat_class /* : uint32_t */
    {
        mxINVALID       = 0,
        mxCELL_CLASS    = 1,
        mxSTRUCT_CLASS  = 2,
        mxOBJECT_CLASS  = 3,
        mxCHAR_CLASS    = 4,
        mxSPARSE_CLASS  = 5,
        mxDOUBLE_CLASS  = 6,
        mxSINGLE_CLASS  = 7,
        mxINT8_CLASS    = 8,
        mxUINT8_CLASS   = 9,
        mxINT16_CLASS   = 10,
        mxUINT16_CLASS  = 11,
        mxINT32_CLASS   = 12,
        mxUINT32_CLASS  = 13,
        mxINT64_CLASS   = 14,
        mxUINT64_CLASS  = 15,
    } mat_class;
    
    // Array Flags Format
    typedef enum mat_flag /* : uint32_t */
    {
        mfCOMPLEX       = 0x1000,
        mfGLOBAL        = 0x2000,
        mfLOGICAL       = 0x4000,
    } mat_flag;
    
    // MATLAB Data Elements
    //
    // Represented by a family of structs with compatible layouts.  Type and
    // then class can be deduced by inspection of the compatible members

    
    // Abstract base class of all elements provides a type for downcasting
    typedef struct mat_element
    {
        int32_t type; // mat_type
    } mat_element;
    
    // Access the array type
    int32_t mat_element_type(void* self);
    
    
    // Basic one-dimensional array of data
    typedef struct mat_array /* : mat_element */
    {
        int32_t type;
        int32_t size; // Number of bytes
        void*   data; // Stored data
    } mat_array;
    int32_t mat_array_count(void* self); // Number of elements of specified type


    
    // Abstract base class of all structred data elements.  Flags encodes the
    // mat_class value for further downcasting
    typedef struct mat_matrix /* : mat_element */
    {
        int32_t type;
        mat_array* flags; // 2 x uint32_t; includes mat_ in least signifcant byte of first uint32_t
        mat_array* dims;  // ? x  int32_t; mat_matrix dimensions
        mat_array* name;  // ? x   int8_t; variable name
    } mat_matrix;
    // Access the matrix class
    uint32_t mat_matrix_class(void* self); // mat_class
    uint32_t mat_matrix_flags(void* self); // bitwise OR of multiple mat_flags
    int32_t  mat_matrix_ndims(void* self); // number of dimensions
    int32_t* mat_matrix_size (void* self); // raw dimensions array
    int32_t  mat_matrix_numel(void* self); // number of elements *product of dimensions

    typedef struct mat_numeric /* : mat_matrix */
    {
        int32_t type;
        mat_array* flags;
        mat_array* dims;
        mat_array* name;
        mat_array* real;  // NUMEL values; type need not match class
        mat_array* imag;  // optional (per mfCOMPLEX)
    } mat_numeric;
    
    typedef struct mat_sparse /* : mat_matrix */
    {
        int32_t type;
        mat_array* flags;
        mat_array* dims;
        mat_array* name;
        mat_array* ir;    // Sparse representation; see documentation
        mat_array* jc;    //
        mat_array* real;  //
        mat_array* imag;  //
    } mat_sparse;
    
    typedef struct mat_cell /* : mat_matrix */
    {
        int32_t type;
        mat_array* flags;
        mat_array* dims;
        mat_array* name;
        mat_matrix** cells; // NUMEL matrices
    } mat_cell;
    
    typedef struct mat_struct /* : mat_matrix */
    {
        int32_t type;
        mat_array* flags;
        mat_array* dims;
        mat_array* name;
        mat_array* fieldNameLength; // Maximum length of field names
        mat_array* fieldNames;      // Packed field names
        mat_matrix** fields;        // numberOfFields matrices
    } mat_struct;
    
    typedef struct mat_object /* : mat_matrix */ {
        int32_t type;
        mat_array* flags;
        mat_array* dims;
        mat_array* name;
        mat_array* className;       // MATLAB class of the mat_object
        mat_array* fieldNameLength;
        mat_array* fieldNames;
        mat_matrix** fields;
    } mat_object;

    // Cast to pointer or null
    mat_numeric* mat_matrix_to_numeric(void* self);
    mat_sparse* mat_matrix_to_sparse(void* self);
    mat_cell* mat_matrix_to_cell(void* self);
    mat_struct* mat_matrix_to_struct(void* self);
    mat_object* mat_matrix_to_object(void* self);

    
    
    // Number of fields of a struct or object
    int32_t mat_matrix_nfields(void* self);
    
    

    // Reading from a buffer [*begin, *end).  Successful parsing moves *begin.
    
    mat_header* mat_header_load(const char** begin, const char** end);
    void* mat_element_load(const char** begin, const char** end);
    
    // Reading from a stream.
    mat_header* mat_header_read(FILE* file);
    mat_element* mat_element_read(FILE* file);
    
    // Printing debug information

    void mat_file_print(FILE* input);
    void mat_header_print(mat_header* self);
    void mat_element_print(void* self);
    
    // Release resources
    void mat_header_delete(mat_header* self);
    void mat_element_delete(void* self);
    
    
    
    // Writing to a buffer
    
    // Buffer type
    typedef struct mat_buffer {
        int32_t size;
        char*   data;
    } mat_buffer;
    // Serialize an element to a buffer
    mat_buffer* mat_element_save(void* self);
    // Delete a buffer
    void mat_buffer_delete(mat_buffer*);
    
    // Writing to a stream

    // Write file header
    
    mat_header* mat_header_new(const char* description); // Truncated if necessary
    void mat_header_write(FILE* file, mat_header* self);
    
    void mat_element_write(void* self, FILE* file);
    
    // Write a whole array element
    void mat_array_write(FILE* file, int32_t type, int32_t size, const void* data);
    void mat_array_write_utf8(FILE* file, const char* str); // string as miUTF8 array

    // Write a whole mxCHAR_CLASS matrix from a C string
    void mat_matrix_write_utf8(FILE* file, const char* str); // string as mxCHAR_CLASS matrix

    // Write members of an miMATRIX element
    void mat_array_write_flags(FILE* file, uint32_t mxClass);
    void mat_array_write_dims1(FILE* file, int32_t cols);
    void mat_array_write_dims2(FILE* file, int32_t rows, int32_t cols);
    void mat_array_write_name(FILE* file, const char* str); // string as miINT8 array
    
    // Open a tag of unknown size, identified with a token
    long mat_element_open(FILE* file, int32_t type);
    // Close the tag identified by the token
    void mat_element_close(FILE* file, long token);
    
    /* Example
     
     mat_header* hdr = mat_header_new("my_data");
     mat_header_write(f, hdr);
     mat_header_delete(hdr);
     
     long token = mat_element_open(f, miMATRIX);
     mat_array_write_flags(f, miDOUBLE_CLASS);
     mat_array_write_dims1(f, N);
     mat_array_write_name("my_data");
     
     long token2 = mat_element_open(f, miINT32); // doesn't have to match
     for (int i = 0; i != N; ++i)
        fwrite(&i, sizeof(i), 1, f); // doesn't need to exist in memory
     mat_element_close(f, token2); // close the miINT32 array, inferring size
     
     mat_element_close(f, token); // close the miMATRIX, inferring size
     
     */
    
    
    // Construction aids


    mat_numeric* mat_numeric_new_float2(int32_t rows, int32_t columns, const float* real, const float* imag);
    mat_numeric* mat_numeric_new_double2(int32_t rows, int32_t columns, const double* real, const double* imag);
    mat_numeric* mat_numeric_new_utf8(const char* zstr);
    
    mat_array* mat_array_new_int8(const char* zstr);
    
    mat_struct* mat_struct_new(void);
    mat_cell* mat_cell_new(void);
    
    void mat_struct_push(void* self, const char* fieldName, void* matrix);
    void mat_cell_push(void* self, void* matrix);
    
    void mat_matrix_set_name(void* self, const char* name);
    
    mat_matrix* mat_matrix_new_utf8(const char* zstr);
    

    
#ifdef __cplusplus
}
#endif

#endif /* defined(__matfile__LIMatFile__) */
