//
//  LIMatFile.c
//  matfile
//
//  Created by Antony Searle on 18/07/2015.
//  Copyright (c) 2015 Antony Searle. All rights reserved.
//

#include "limatlab.h"

#include <stdio.h>
#include <stdlib.h>
#include <zlib.h>
#include <assert.h>
#include <string.h>
#include <ctype.h>
#include <limits.h>
#include <stddef.h>


#define MAX(X, Y) (((X) < (Y)) ? (Y) : (X))
#define MIN(X, Y) (((X) < (Y)) ? (X) : (Y))

// Alignment padding helper functions

uint64_t next8(uint64_t x) {
    return (x + 7) & ~((uint64_t) 7);
}

uint64_t next4(uint64_t x) {
    return (x + 3) & ~((uint64_t) 3);
}

void mat_buffer_delete(mat_buffer* self) {
    free(self->data);
    free(self);
}

void mat_buffer_compare(mat_buffer* a, mat_buffer* b) {
    int32_t m = a->size;
    int32_t n = b->size;
    if (m != n) // different sizes
        goto mismatch;
    for (int i = 0; i != n; ++i) // different values
        if (a->data[i] != b->data[i])
            goto mismatch;
    return;
mismatch:
    for (int i = 0; i != MAX(m, n); ++i)
    {
        printf("% .10i ", i);
        if (i < m)
            printf("%.2X ", (unsigned char) a->data[i]);
        else
            printf("   ");
        if (i < n)
            printf("%.2X ", (unsigned char) b->data[i]);
        else
            printf("   ");
        if (i >= MIN(m, n) || a->data[i] != b->data[i])
            printf("<--");
        printf ("\n");
    }
    exit(EXIT_FAILURE);
}

mat_buffer* mat_buffer_new() {
    return (mat_buffer*) calloc(1, sizeof(mat_buffer));
}


void mat_buffer_append_bytes(mat_buffer* self, int32_t count, void* data) {
    self->data = realloc(self->data, (size_t)(self->size + count));
    memcpy(self->data + self->size, data, count);
    self->size += count;
}

#define mat_buffer_append(PMATBUFFER, LVALUE) mat_buffer_append_bytes(PMATBUFFER, sizeof(LVALUE), &LVALUE)

void mat_buffer_pad(mat_buffer* self, int32_t size) {
    assert(size < 8);
    self->data = realloc(self->data, (size_t)(self->size + size));
    memset(self->data + self->size, 0, size);
    self->size += size;
}

void mat_buffer_pad8(mat_buffer* self) {
    mat_buffer_pad(self, ((int32_t) next8((size_t) (self->size))) - self->size);
}


void mat_buffer_deflate(mat_buffer* self);



const char* mat_type_name[/* mat_type*/] = {
    "undefined(0)",
    "int8_t",
    "uint8_t",
    "int16_t",
    "uint16_t",
    "int32_t",
    "uint32_t",
    "float",
    "undefined(8)",
    "double",
    "undefined(10)",
    "undefined(11)",
    "int64_t",
    "uint64_t",
    "matrix",
    "compressed",
    "UTF-8",
    "UTF-16",
    "UTF-32"
};

int mat_type_validate(int32_t type) {
    switch (type) {
        case miINT8:
        case miUINT8:
        case miINT16:
        case miUINT16:
        case miINT32:
        case miUINT32:
        case miSINGLE:
        case miDOUBLE:
        case miINT64:
        case miUINT64:
        case miMATRIX:
        case miCOMPRESSED:
        case miUTF8:
        case miUTF16:
        case miUTF32:
            return type;
        default:
            assert(0);
            return 0;
    }
}

const int32_t mat_type_size[/* mat_type*/] = {
    0,
    sizeof(int8_t),
    sizeof(uint8_t),
    sizeof(int16_t),
    sizeof(uint16_t),
    sizeof(int32_t),
    sizeof(uint32_t),
    sizeof(float),
    0,
    sizeof(double),
    0,
    0,
    sizeof(int64_t),
    sizeof(uint64_t),
    0,
    0,
    1, // UTF-8
    2, // UTF-16
    4  // UTF-32
};

const mat_class mat_type_class[/* mat_type */] = {
    0,
    mxINT8_CLASS,
    mxUINT8_CLASS,
    mxINT16_CLASS,
    mxUINT16_CLASS,
    mxINT32_CLASS,
    mxUINT32_CLASS,
    mxSINGLE_CLASS,
    0,
    mxDOUBLE_CLASS,
    0,
    0,
    mxINT64_CLASS,
    mxUINT64_CLASS,
    0,
    0,
    mxCHAR_CLASS,
    mxCHAR_CLASS,
    mxCHAR_CLASS
};

const char* mat_class_name[/* mat_class */] = {
    "undefined(0)",
    "cell",
    "struct",
    "object",
    "char",
    "sparse",
    "double",
    "float",
    "int8_t",
    "uint8_t",
    "int16_t",
    "uint16_t",
    "int32_t",
    "uint32_t",
    "int64_t",
    "uint64_t"
};

uint32_t mat_class_validate(uint32_t class_) {
    switch (class_) {
        case mxCELL_CLASS:
        case mxSTRUCT_CLASS:
        case mxOBJECT_CLASS:
        case mxCHAR_CLASS:
        case mxSPARSE_CLASS:
        case mxDOUBLE_CLASS:
        case mxSINGLE_CLASS:
        case mxINT8_CLASS:
        case mxUINT8_CLASS:
        case mxINT16_CLASS:
        case mxUINT16_CLASS:
        case mxINT32_CLASS:
        case mxUINT32_CLASS:
        case mxINT64_CLASS:
        case mxUINT64_CLASS:
            return class_;
        default:
            assert(0);
            return 0;
    }
}


mat_header* mat_header_new(const char* description) {
    mat_header* self = calloc(1, sizeof(mat_header));
    char* prefix = "MATLAB 5.0 MAT-file\n";
    size_t n = strlen(prefix);
    memcpy(self->description, prefix, n);
    if (description) {
        size_t m = MIN(strlen(description), sizeof(self->description) - n);
        memcpy(self->description + n, description, m);
    }
    self->version = 0x0100;
    self->endian = ('M' << 8) + 'I';
    return self;
}

void mat_header_delete(mat_header* self) {
    free(self);
}

mat_header* mat_header_load(char** begin, char** end) {
    mat_header* self = 0;
    if (((*end - *begin) >= (ptrdiff_t) sizeof(mat_header))
        && (self = malloc(sizeof(mat_header)))) {
        memcpy(self, *begin, sizeof(mat_header));
        *begin += sizeof(mat_header);
        assert(self->version == 0x0100);
        assert(self->endian == ('M' << 8) + 'I');
    }
    return self;
}

void mat_header_print(mat_header* self) {
    if (self) {
        printf("header {\n");
        printf("    description = \"%.118s\"\n", self->description);
        printf("    version     = 0x%.4X\n", self->version);
        char* endian = (char*) &self->endian;
        printf("    endian      = %.2s ", endian);
        if (endian[0] == 'M' && endian[1] == 'I')
            printf("(big)\n");
        else if (endian[0] == 'I' && endian[1] == 'M')
            printf("(little)\n");
        else
            printf("(invalid)\n");
        printf("}\n");
    }
}

int32_t mat_element_type(void* self) {
    return self ? *((int32_t*) self) : 0;
}

int32_t mat_array_count(void* self) {
    mat_array* ptr = self;
    int32_t size = mat_type_size[mat_element_type(ptr)];
    return size ? ptr->size / size : 0;
}


uint32_t mat_matrix_class(void* self)
{
    return (mat_element_type(self) == miMATRIX)
        ? (*((uint32_t*) ((mat_matrix*) self)->flags->data) & 0xFF)
        : 0;
}

int32_t mat_matrix_ndims(void* self) {
    mat_matrix* ptr = self;
    return ptr ? mat_array_count(ptr->dims) : 0;
}

int32_t* mat_matrix_size(void* self) {
    mat_matrix* ptr = self;
    return ptr->dims->data;
}

int32_t mat_matrix_numel(void* self) {
    assert(mat_element_type(self) == miMATRIX);
    int32_t n = 1;
    mat_matrix* p = self;
    for (int32_t i = 0; i != p->dims->size / (int32_t) sizeof(int32_t); ++i) {
        n *= ((int32_t*) p->dims->data)[i];
    }
    return n;
}


int32_t mat_matrix_nfields(void* self)
{
    switch (mat_matrix_class(self)) {
        case mxOBJECT_CLASS: {
            mat_object* p = self;
            return p->fieldNames->size / *((int32_t*) p->fieldNameLength->data);
        } break;
        case mxSTRUCT_CLASS: {
            mat_struct* p = self;
            return p->fieldNames->size / *((int32_t*) p->fieldNameLength->data);
        } break;
        default:
            assert(0);
            return 0;
    }
}




void mat_element_delete(void* self) {
    if (self) {
        switch (mat_element_type(self)) {
            case miMATRIX: { // Compound element
                mat_matrix* m = self;
                switch (mat_matrix_class(m)) {
                    case mxOBJECT_CLASS: {
                        mat_object* p = self;
                        mat_element_delete(p->className);
                        for (int i = 0; i != mat_matrix_nfields(p); ++i)
                            mat_element_delete(p->fields[i]);
                        free(p->fields);
                        mat_element_delete(p->fieldNames);
                        mat_element_delete(p->fieldNameLength);
                    } break;
                    case mxSTRUCT_CLASS: {
                        mat_struct* p = self;
                        for (int i = 0; i != mat_matrix_nfields(p); ++i)
                            mat_element_delete(p->fields[i]);
                        free(p->fields);
                        mat_element_delete(p->fieldNames);
                        mat_element_delete(p->fieldNameLength);
                    } break;
                    case mxCELL_CLASS: {
                        mat_cell* p = self;
                        int32_t n = mat_matrix_numel(p);
                        for (int i = 0; i != n; ++i) {
                            mat_element_delete(p->cells[i]);
                        }
                        free(p->cells);
                    } break;
                    case mxSPARSE_CLASS: {
                        mat_sparse* p = self;
                        mat_element_delete(p->ir);
                        mat_element_delete(p->jc);
                        mat_element_delete(p->real);
                        mat_element_delete(p->imag);
                    } break;
                    default: {
                        mat_numeric* p = self;
                        mat_element_delete(p->imag);
                        mat_element_delete(p->real);
                    } break;
                };
                mat_element_delete(m->name);
                mat_element_delete(m->dims);
                mat_element_delete(m->flags);
            } break;
            default: { // Basic element
                mat_array* p = self;
                free(p->data);
            }
                break;
        }
        free(self);
    }
}

void mat_element_print2(void* self, int indent /* unsupported */)
{
    if (self) {
        switch (mat_element_type(self)) {
            case miINT8:
            case miUTF8: {
                mat_array* a = self;
                printf("\"");
                char* c = (char*) a->data;
                for (int i = 0; i != a->size; ++i)
                    if (isprint(c[i])) {
                        printf("%c", c[i]);
                    } else {
                        printf("\\x%.2x", c[i]);
                    }
                printf("\"");
                printf(" (%d x %s)\n", a->size, mat_type_name[a->type]);
                
            } break;
                
#define CASE(MITYPE, CTYPE, FORMAT)\
            case MITYPE: {\
                mat_array* a = self;\
                CTYPE* b = (CTYPE*) a->data;\
                size_t n = ((size_t) a->size) / sizeof(CTYPE);\
                size_t m = MIN(n, 4);\
                for (size_t i = 0; i != m; ++i) {\
                    printf(FORMAT " ", b[i]);\
                }\
                if (m != n) {\
                    printf("... ");\
                }\
                printf("(%lu x %s)\n", ((size_t) a->size) / sizeof(CTYPE), mat_type_name[(size_t) a->type]);\
            } break;

                CASE(miUINT8, uint8_t, "%u");
                CASE(miINT16, int16_t, "%i");
                CASE(miUINT16, uint16_t, "%u");
                CASE(miINT32, int32_t, "%i");
                CASE(miUINT32, uint32_t, "%u");
                CASE(miDOUBLE, double, "%g");
                CASE(miSINGLE, float, "%g");

            case miMATRIX: {
                mat_matrix* m = self;
                printf("%s matrix {\n", mat_class_name[mat_matrix_class(m)]);
                printf("    flags: "); mat_element_print(m->flags);
                printf("    dims : "); mat_element_print(m->dims);
                printf("    name : "); mat_element_print(m->name);
                switch (mat_matrix_class(m)) {
                    case mxCELL_CLASS: {
                        mat_cell* p = self;
                        int32_t n = mat_matrix_numel(self);
                        for (int i = 0; i != n; ++i)
                            mat_element_print(p->cells[i]);
                    } break;
                    case mxSTRUCT_CLASS: {
                        mat_struct* p = self;
                        printf("    fieldNameLength : "); mat_element_print(p->fieldNameLength);
                        printf("    fieldNames      : "); mat_element_print(p->fieldNames);
                        for (int i = 0; i != mat_matrix_nfields(p); ++i)
                            mat_element_print(p->fields[i]);
                    } break;
                    case mxOBJECT_CLASS: {
                        mat_object* p = self;
                        printf("    className       : "); mat_element_print(p->className);
                        printf("    fieldNameLength : "); mat_element_print(p->fieldNameLength);
                        printf("    fieldNames      : "); mat_element_print(p->fieldNames);
                        for (int i = 0; i != mat_matrix_nfields(p); ++i)
                            mat_element_print(p->fields[i]);
                    } break;
                    case mxSPARSE_CLASS: {
                        mat_sparse* p = self;
                        printf("    ir   : "); mat_element_print(p->ir);
                        printf("    jc   : "); mat_element_print(p->jc);
                        printf("    real : "); mat_element_print(p->real);
                        printf("    imag : "); mat_element_print(p->imag);
                    } break;
                    default: {
                        mat_numeric* p = self;
                        printf("    real : "); mat_element_print(p->real);
                        printf("    imag : "); mat_element_print(p->imag);
                    } break;
                }
                printf("}\n");
            } break;
            default: {
                mat_array* a = self;
                printf("%s array (%d chars)\n", mat_type_name[a->type], a->size);
            } break;
        }
    } else {
        printf("null\n");
    }
}

void mat_element_print(void* self) {
    mat_element_print2(self, 0);
}





// Load file elements from a byte range

void* mat_element_load_numeric(mat_array* flags, mat_array* dims, mat_array* name, char** begin, char** end) {
    mat_numeric* self = calloc(1, sizeof(mat_numeric));
    self->type = miMATRIX;
    self->flags = flags;
    self->dims = dims;
    self->name = name;
    self->real = mat_element_load(begin, end); // type need not match type in flags
    self->imag = mat_element_load(begin, end); // can be null
    assert(mat_matrix_numel(self) == mat_array_count(self->real)); // count should match
    assert(!self->imag || (mat_matrix_numel(self) == mat_array_count(self->imag)));
    assert(*begin == *end);
    return self;
}

void* mat_element_load_structure(mat_array* flags, mat_array* dims, mat_array* name, char**begin, char** end) {
    mat_struct* self = calloc(1, sizeof(mat_struct));
    self->type = miMATRIX;
    self->flags = flags;
    self->dims = dims;
    self->name = name;
    self->fieldNameLength = mat_element_load(begin, end);
    self->fieldNames = mat_element_load(begin, end); // can be null
    size_t n = (size_t) mat_matrix_nfields(self);
    self->fields = calloc(n, sizeof(mat_matrix*));
    for (size_t i = 0; i != n; ++i)
        self->fields[i] = mat_element_load(begin, end);
    assert(*begin == *end);
    return self;
}

void* mat_element_load_object(mat_array* flags, mat_array* dims, mat_array* name, char**begin, char** end) {
    mat_object* self = calloc(1, sizeof(mat_object));
    self->type = miMATRIX;
    self->flags = flags;
    self->dims = dims;
    self->name = name;
    self->className = mat_element_load(begin, end);
    self->fieldNameLength = mat_element_load(begin, end);
    self->fieldNames = mat_element_load(begin, end); // can be null
    size_t n = (size_t) mat_matrix_nfields(self);
    self->fields = calloc(n, sizeof(mat_matrix*));
    for (size_t i = 0; i != n; ++i)
        self->fields[i] = mat_element_load(begin, end);
    assert(*begin == *end);
    return self;
}

void* mat_element_load_sparse(mat_array* flags, mat_array* dims, mat_array* name, char** begin, char** end) {
    mat_sparse* self = calloc(1, sizeof(mat_sparse));
    self->type = miMATRIX;
    self->flags = flags;
    self->dims = dims;
    self->name = name;
    self->ir = mat_element_load(begin, end);
    self->jc = mat_element_load(begin, end);
    self->real = mat_element_load(begin, end); // type need not match type in flags
    self->imag = mat_element_load(begin, end); // can be null
    assert(mat_matrix_numel(self) == mat_array_count(self->real)); // count should match
    assert(!self->imag || (mat_matrix_numel(self) == mat_array_count(self->imag)));
    assert(*begin == *end);
    return self;
}



void* mat_element_load_cell(mat_array* flags, mat_array* dims, mat_array* name, char** begin, char** end) {
    mat_cell* self = calloc(1, sizeof(mat_cell));
    self->type = miMATRIX;
    self->flags = flags;
    self->dims = dims;
    self->name = name;
    size_t n = (size_t) mat_matrix_numel(self);
    self->cells = calloc(n, sizeof(mat_matrix*));
    for(size_t i = 0; i != n; ++i) {
        self->cells[i] = mat_element_load(begin, end);
    }
    assert(*begin == *end);
    return self;
}

void* mat_element_load_matrix(int32_t size, char* data) {
    char* begin = data;
    char* end   = data + size;
    
    mat_array* flags = mat_element_load(&begin, &end);
    mat_array* dims = mat_element_load(&begin, &end);
    mat_array* name = mat_element_load(&begin, &end);
    
    assert(flags->type = miUINT32);
    uint32_t class_ = (*((uint32_t*) (flags->data))) & 0xFF;
    
    assert(mat_class_validate(class_));
    
    switch (class_) {
        case mxCELL_CLASS:
            return mat_element_load_cell(flags, dims, name, &begin, &end);
        case mxSTRUCT_CLASS:
            return mat_element_load_structure(flags, dims, name, &begin, &end);
        case mxOBJECT_CLASS:
            return mat_element_load_object(flags, dims, name, &begin, &end);
        case mxSPARSE_CLASS:
            return mat_element_load_sparse(flags, dims, name, &begin, &end);
        default:
            return mat_element_load_numeric(flags, dims, name, &begin, &end);
    }
}

void* mat_element_load_compressed(int32_t size, char* data) {
    
    z_streamp strm = calloc(sizeof(z_stream), 1);
    
    strm->next_in = (unsigned char*) data;
    strm->avail_in = (unsigned int) size;
    
    int z = inflateInit(strm);
    assert(z == Z_OK);
    
    uLong n = (uLong) size * 2; // initial guess is double the compressed size
    Bytef* p = calloc(n, 1);
    
    for (;;) {
        strm->next_out = p + strm->total_out;
        strm->avail_out = (unsigned int) (n - strm->total_out);
        z = inflate(strm, Z_SYNC_FLUSH);
        if (z != Z_OK) // completed (or error)
            break;
        // we need to expand the buffer
        n *= 2;
        p = realloc(p, n);
    }
    assert(z == Z_STREAM_END);
    
    Bytef* begin = p;
    Bytef* end = p + strm->total_out;
    
    z = inflateEnd(strm);
    assert(z == Z_OK);
    
    void* self = mat_element_load((char**) &begin, (char**) &end);
    assert(begin == end);
    
    free(p);
    free(strm);
    
    return self;
    
}

void* mat_element_load_array(int32_t type, int32_t size, char* data) {
    mat_array* self = calloc(sizeof(mat_array), 1);
    self->type = type;
    self->size = size;
    self->data = calloc((size_t) size, 1);
    memcpy(self->data, data, size);
    return self;
}

void* mat_element_load(char** begin, char** end) {
    
    assert(*begin <= *end);
    if (*begin == *end)
        return NULL; // allowed to run out of data, for example missing imaginary part of a real matrix
    
    int32_t type;
    int32_t size;
    char*   data;
    
    //char* saved_begin = *begin;
    
    int32_t front = *((int32_t*) *begin);
    if (front & (int32_t) 0xFFFF0000) {
        // small data array format
        type = front & 0x0000FFFF;
        size = front >> 16;
        assert(size <= 4);
        data = *begin + 4;
        *begin += 8;
    } else {
        // large data array format
        type = front;
        size = *((int32_t*) ((*begin) + 4));
        data = *begin + 8;
        // MATLAB appears to not align compressed arrays
        *begin += 8 + ((type != miCOMPRESSED) ? (int32_t) next8((size_t) size) : size);
    }
    assert(*begin <= *end);

    if (!mat_type_validate(type)) {
        assert(0);
        return NULL;
    }

    
    void* result  = NULL;
    switch (type) {
        case miMATRIX:
            result = mat_element_load_matrix(size, data);
            break;
        case miCOMPRESSED:
            result = mat_element_load_compressed(size, data);
            break;
        default:
            result = mat_element_load_array(type, size, data);
            break;
    }
    
    /* Self-check: can we byte-for-byte recreate the data we parsed?
    
    //printf("Comparing mat_element:\n");
    //mat_element_print(result);
    
    mat_buffer* other = element_save(result);
    
    // If we have got the mat_element from compressed data, we need to compress 
    // it before comparing.  This happens to work because MATLAB appears to use
    // default zlib compression options.
     
    if (type == miCOMPRESSED)
        mat_buffer_deflate(other);
    
    mat_buffer tmp = { (int32_t) (*begin - saved_begin), saved_begin };
    
    mat_buffer_compare(&tmp, other);
    
    //printf("\n\n\n");
    
    
    mat_buffer_delete(other);
     
     */
    
    
    return result;
}



// File output



// Data mat_buffer that can be prepended or appended






// Serialize MATLAB file elements to a data mat_buffer


void mat_buffer_append_element(mat_buffer* self, void* other) {
    if (other) {
        mat_buffer* tmp = mat_element_save(other);
        mat_buffer_append_bytes(self, tmp->size, tmp->data);
        mat_buffer_delete(tmp);
    }
}

void mat_buffer_prepend_type(mat_buffer* self, int32_t type) {
    int32_t size = 8 + (int32_t) next8((size_t) self->size);
    char* data = calloc((size_t) size, 1);
    ((int32_t*) data)[0] = type;
    ((int32_t*) data)[1] = self->size;
    memcpy(data + 8, self->data, self->size);
    free(self->data);
    self->size = size;
    self->data = data;
}

mat_buffer* mat_array_save(mat_array* self) {
    mat_buffer* other = mat_buffer_new();
    if (self->size > 4) { // full header
        mat_buffer_append(other, self->type);
        mat_buffer_append(other, self->size);
    } else { // compact header
        int32_t hdr = (int32_t) (self->type + (self->size << 16));
        mat_buffer_append(other, hdr);
    }
    mat_buffer_append_bytes(other, self->size, self->data);
    mat_buffer_pad8(other);
    return other;
}

mat_buffer* mat_numeric_save(mat_numeric* self) {
    mat_buffer* other = mat_buffer_new();
    mat_buffer_append_element(other, self->flags);
    mat_buffer_append_element(other, self->dims);
    mat_buffer_append_element(other, self->name);
    mat_buffer_append_element(other, self->real);
    mat_buffer_append_element(other, self->imag);
    mat_buffer_prepend_type(other, miMATRIX);
    return other;
}

mat_buffer* mat_sparse_save(mat_sparse* self) {
    mat_buffer* other = mat_buffer_new();
    mat_buffer_append_element(other, self->flags);
    mat_buffer_append_element(other, self->dims);
    mat_buffer_append_element(other, self->name);
    mat_buffer_append_element(other, self->ir);
    mat_buffer_append_element(other, self->jc);
    mat_buffer_append_element(other, self->real);
    mat_buffer_append_element(other, self->imag);
    mat_buffer_prepend_type(other, miMATRIX);
    return other;
}

mat_buffer* mat_struct_save(mat_struct* self) {
    mat_buffer* other = mat_buffer_new();
    mat_buffer_append_element(other, self->flags);
    mat_buffer_append_element(other, self->dims);
    mat_buffer_append_element(other, self->name);
    mat_buffer_append_element(other, self->fieldNameLength);
    mat_buffer_append_element(other, self->fieldNames);
    for (int i = 0; i != mat_matrix_nfields(self); ++i)
        mat_buffer_append_element(other, self->fields[i]);
    mat_buffer_prepend_type(other, miMATRIX);
    return other;
}

mat_buffer* mat_object_save(mat_object* self) {
    mat_buffer* other = mat_buffer_new();
    mat_buffer_append_element(other, self->flags);
    mat_buffer_append_element(other, self->dims);
    mat_buffer_append_element(other, self->name);
    mat_buffer_append_element(other, self->className);
    mat_buffer_append_element(other, self->fieldNameLength);
    mat_buffer_append_element(other, self->fieldNames);
    for (int i = 0; i != mat_matrix_nfields(self); ++i)
        mat_buffer_append_element(other, self->fields[i]);
    mat_buffer_prepend_type(other, miMATRIX);
    return other;
}

mat_buffer* mat_cell_save(mat_cell* self) {
    mat_buffer* other = mat_buffer_new();
    mat_buffer_append_element(other, self->flags);
    mat_buffer_append_element(other, self->dims);
    mat_buffer_append_element(other, self->name);
    for (int i = 0; i != mat_matrix_numel(self); ++i) {
        mat_buffer_append_element(other, self->cells[i]);
    }
    mat_buffer_prepend_type(other, miMATRIX);
    return other;
}

mat_buffer* mat_matrix_save(void* self) {
    switch (mat_matrix_class(self)) {
        case mxSTRUCT_CLASS:
            return mat_struct_save(self);
        case mxCELL_CLASS:
            return mat_cell_save(self);
        case mxOBJECT_CLASS:
            return mat_object_save(self);
        case mxSPARSE_CLASS:
            return mat_sparse_save(self);
        default:
            return mat_numeric_save(self);
    }
}

mat_buffer* mat_element_save(void* self) {
    if (self) {
        switch (mat_element_type(self)) {
            case miMATRIX:
                return mat_matrix_save(self);
            default:
                return mat_array_save(self);
        }
    } else {
        return NULL;
    }
}

void mat_buffer_deflate(mat_buffer* self) {
    
    assert(self);
    assert(self->size > 0);
    assert(self->data);
    // we should only be compressing whole variables, not smaller elements
    assert(*((int32_t*) self->data) == miMATRIX);
    
    // set up stream to access the mat_buffer's data
    z_streamp strm = calloc(sizeof(z_stream), 1);
    strm->next_in = (unsigned char*) self->data;
    strm->avail_in = (unsigned int) self->size;
    
    int z = deflateInit(strm, Z_DEFAULT_COMPRESSION);
    assert(z == Z_OK);
    
    uLong n = deflateBound(strm, (uLong) self->size);
    Bytef* p = calloc(n + 8, 1);
    strm->next_out = p + 8;
    strm->avail_out = (unsigned int) n;

    // output sized by deflateBound guarantees deflation complete in one step
    z = deflate(strm, Z_FINISH);
    assert(z == Z_STREAM_END);
    
    // tag the new mat_buffer
    ((int32_t*) p)[0] = miCOMPRESSED;
    ((int32_t*) p)[1] = (int32_t) strm->total_out;

    // swap in the mat_buffer contents
    self->size = (int32_t) strm->total_out + 8;
    free(self->data);
    self->data = (char*) p;

    // no padding for compressed mat_buffers
}


long mat_element_open(FILE* file, int32_t type) {
    fwrite(&type, sizeof(type), 1, file);
    long position = ftell(file);
     // Write an invalid size that must be overwritten by mat_element_close
    int32_t size = INT_MIN;
    fwrite(&size, sizeof(size), 1, file);
    return position;
}

void mat_element_close(FILE* file, long token) {
    long position = ftell(file);
    fseek(file, token, SEEK_SET);
    int32_t size = (int32_t) (position - token - 4);
    fwrite(&size, sizeof(int32_t), 1, file);
    fseek(file, position, SEEK_SET);
    uint64_t padding = 0;
    fwrite(&padding, 1, (-size) & 7, file);
}

void mat_array_write(FILE* file, int32_t type, int32_t size, const void* data) {
    if (size > 4) {
        fwrite(&type, sizeof(type), 1, file);
        fwrite(&size, sizeof(size), 1, file);
        fwrite(data, 1, (size_t) size, file);
        uint64_t padding = 0;
        fwrite(&padding, 1, (-size) &7, file);
    } else {
        int32_t hdr = type + (size << 16);
        fwrite(&hdr, sizeof(hdr), 1, file);
        fwrite(data, 1, (size_t) size, file);
        uint32_t padding = 0;
        fwrite(&padding, 1, (size_t) (4 - size), file);
        
    }
}

void mat_array_write_flags(FILE* file, uint32_t mxClass) {
    uint32_t flags[] = { mxClass, 0 };
    mat_array_write(file, miUINT32, sizeof(flags), flags);
}

void mat_array_write_dims1(FILE* file, int32_t cols) {
    mat_array_write_dims2(file, 1, cols);
}

void mat_array_write_dims2(FILE* file, int32_t rows, int32_t cols) {
    int32_t dims[] = { rows, cols };
    mat_array_write(file, miINT32, sizeof(dims), dims);
}

void mat_array_write_name(FILE* file, const char* str) {
    mat_array_write(file, miINT8, (int32_t) strlen(str), str);
}

void mat_array_write_utf8(FILE* file, const char* str) {
    mat_array_write(file, miUTF8, (int32_t) strlen(str), str);
}

void mat_matrix_write_utf8(FILE* file, const char* str) {
    long token = mat_element_open(file, miMATRIX);
    mat_array_write_flags(file, mxCHAR_CLASS);
    mat_array_write_dims1(file, (int32_t) strlen(str));
    mat_array_write_name(file, "");
    mat_array_write_utf8(file, str);
    mat_element_close(file, token);
}





void mat_file_print(FILE* input) {
    
    fseek(input, 0, SEEK_END);
    long n = ftell(input);
    char* ptr = malloc((size_t) n);
    fseek(input, 0, SEEK_SET);
    fread(ptr, 1, (size_t) n, input);
    
    char* begin = ptr;
    char* end = ptr + n;
    
    mat_header* hdr = mat_header_load(&begin, &end);
    mat_header_print(hdr);
    mat_header_delete(hdr);
    
    while (begin < end) {
        void* v = mat_element_load(&begin, &end);
        mat_element_print(v);
        mat_element_delete(v);
    }
}


void mat_element_write(void* self, FILE* file) {
    mat_buffer* buf = mat_element_save(self);
    if (buf) {
        mat_buffer_deflate(buf);
        fwrite(buf->data, 1, (size_t) buf->size, file);
        mat_buffer_delete(buf);
    }
}

mat_array* mat_array_new_int8(const char* zstr) {
    mat_array* self = calloc(1, sizeof(mat_array));
    if (self) {
        self->type = miINT8;
        self->size = (int32_t) strlen(zstr);
        self->data = malloc((size_t) self->size + 1);
        if (self->data) {
            memcpy(self->data, zstr, self->size + 1);
        } else {
            free(self);
            self = NULL;
        }
    }
    return self;
}

mat_array* mat_array_new_utf8(const char* zstr) {
    mat_array* self = calloc(1, sizeof(mat_array));
    if (self) {
        self->type = miUTF8;
        self->size = (int32_t) strlen(zstr);
        self->data = malloc((size_t) self->size + 1);
        if (self->data) {
            memcpy(self->data, zstr, self->size + 1);
        } else {
            free(self);
            self = NULL;
        }
    }
    return self;
}

mat_array* mat_array_new(mat_type type, int32_t size, const void* data) {
    mat_array* self = calloc(1, sizeof(mat_array));
    if (self) {
        self->type = type;
        self->size = size;
        self->data = malloc((size_t) size);
        if (self->data) {
            memcpy(self->data, data, size);
        } else {
            free(self);
            self = NULL;
        }
    }
    return self;
}

mat_numeric* mat_numeric_new_utf8(const char* zstr) {
    mat_numeric* self = calloc(1, sizeof(mat_numeric));
    self->type = miMATRIX;
    uint32_t flags[] = { mxCHAR_CLASS, 0 };
    self->flags = mat_array_new(miUINT32, sizeof(flags), flags);
    int32_t dims[] = { 1, (int32_t) strlen(zstr) };
    self->dims = mat_array_new(miINT32, sizeof(dims), dims);
    self->name = mat_array_new_int8("");
    self->real = mat_array_new_utf8(zstr);
    return self;
}

mat_numeric* mat_numeric_new_float2(int32_t rows, int32_t columns, const float* real, const float* imag) {
    mat_numeric* self = calloc(1, sizeof(mat_numeric));
    self->type = miMATRIX;
    uint32_t flags[] = { mxSINGLE_CLASS, 0 };
    if (imag)
        flags[0] |= mfCOMPLEX;
    self->flags = mat_array_new(miUINT32, sizeof(flags), flags);
    int32_t dims[] = { rows, columns };
    self->dims = mat_array_new(miINT32, sizeof(dims), dims);
    self->name = mat_array_new_int8("");
    self->real = mat_array_new(miSINGLE, rows * columns * (int32_t) sizeof(float), real);
    if (imag) {
        self->imag = mat_array_new(miSINGLE, rows * columns * (int32_t) sizeof(float), imag);
    }
    return self;
}

mat_numeric* mat_numeric_new_double2(int32_t rows, int32_t columns, const double* real, const double* imag) {
    mat_numeric* self = calloc(1, sizeof(mat_numeric));
    self->type = miMATRIX;
    uint32_t flags[] = { mxDOUBLE_CLASS, 0 };
    if (imag)
        flags[0] |= mfCOMPLEX;
    self->flags = mat_array_new(miUINT32, sizeof(flags), flags);
    int32_t dims[] = { rows, columns };
    self->dims = mat_array_new(miINT32, sizeof(dims), dims);
    self->name = mat_array_new_int8("");
    self->real = mat_array_new(miDOUBLE, rows * columns * (int32_t) sizeof(double), real);
    if (imag) {
        self->imag = mat_array_new(miDOUBLE, rows * columns * (int32_t) sizeof(double), imag);
    }
    return self;
}

mat_cell* mat_cell_new() {
    mat_cell* self = calloc(1, sizeof(mat_cell));
    self->type = miMATRIX;
    uint32_t flags[] = { mxCELL_CLASS, 0 };
    self->flags = mat_array_new(miUINT32, sizeof(flags), flags);
    int32_t dims[] = { 0, 0 };
    self->dims = mat_array_new(miINT32, sizeof(dims), dims);
    self->name = mat_array_new_int8("");
    return self;
}

void mat_cell_push(void* self, void* matrix) {
    mat_cell* ptr = (mat_cell*) self;
    assert(ptr);
    assert(ptr->dims->size == 8);
    int32_t* dims = ptr->dims->data;
    if (!dims[0])
        dims[0] = 1;
    assert(dims[0] == 1);
    int32_t n = dims[0] * dims[1];
    ptr->cells = realloc(ptr->cells, sizeof(mat_matrix*) * (size_t) (n + 1));
    ptr->cells[n] = matrix;
    ++dims[1];
}

mat_struct* mat_struct_new() {
    mat_struct* self = (mat_struct*) calloc(1, sizeof(mat_struct));
    self->type = miMATRIX;
    uint32_t flags[] = { mxSTRUCT_CLASS, 0 };
    self->flags = mat_array_new(miUINT32, sizeof(flags), flags);
    int32_t dims[] = { 1, 1 };
    self->dims = mat_array_new(miINT32, sizeof(dims), dims);
    self->name = mat_array_new_int8("");
    int32_t fieldNameLength = 32;
    self->fieldNameLength = mat_array_new(miINT32, sizeof(fieldNameLength), &fieldNameLength);
    self->fieldNames = mat_array_new(miINT8, 0, NULL);
    return self;
    
}

void mat_struct_push(void* self, const char* fieldName, void* matrix) {
    mat_struct* ptr = (mat_struct*) self;
    assert(ptr);

    int32_t* fieldNameLength = ptr->fieldNameLength->data;
    const int32_t fieldCount = ptr->fieldNames->size / *fieldNameLength;
    
    // We can't realloc since the field name length might need to increase
    int32_t newFieldNameLength = (int32_t) next8(MAX((size_t) *fieldNameLength, strlen(fieldName) + 1));
    char* newFieldNames = calloc((size_t) fieldCount + 1, (size_t) newFieldNameLength);
    char* fieldNames = (char*) ptr->fieldNames->data;
    for (int32_t i = 0; i != fieldCount; ++i) {
        memcpy(newFieldNames + i * newFieldNameLength,
               fieldNames + i * *fieldNameLength,
               *fieldNameLength);
    }
    strcpy(newFieldNames + fieldCount * newFieldNameLength, fieldName);

    *fieldNameLength = (int32_t) newFieldNameLength;
    ptr->fieldNames->size = (fieldCount + 1) * *fieldNameLength;
    free(ptr->fieldNames->data);
    ptr->fieldNames->data = newFieldNames;
    
    ptr->fields = realloc(ptr->fields, sizeof(mat_matrix*) * (size_t) (fieldCount + 1));
    ptr->fields[fieldCount] = matrix;

    
    
}

void mat_header_write(FILE* file, mat_header* hdr) {
    fwrite(hdr, sizeof(mat_header), 1, file);
}




