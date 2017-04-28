//
//  liutility.h
//  liquidreader
//
//  Created by Antony Searle on 17/10/16.
//  Copyright © 2016 Antony Searle. All rights reserved.
//

#ifndef liutility_h
#define liutility_h

#include <assert.h>
#include <stdbool.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdint.h>

#include "lireader.h"

#ifdef __cplusplus
extern "C" {
#endif
    
    
    // min and max for partially ordered types, with well-known drawbacks
    
#define MAX(X, Y) (((X) <  (Y)) ? (Y) : (X))
#define MIN(X, Y) (((X) <= (Y)) ? (X) : (Y))


    // li_byte* is used for untyped storage that permits pointer arithmetic
    
    typedef int8_t li_byte;
    
#define LI_BITS_PER_BYTE 8
    
    
    // li_alloc and li_dealloc are used for all memory management.  They default
    // to malloc and free.  They may be altered by the user.
    
    extern void* (*li_alloc)(size_t n);
    extern void (*li_dealloc)(void* ptr);
    
    
    
    // li_ types should be constructed with _ctor and destructed with _dtor.
    // Like pointers, bitwise copies that occur in assignment, function argument
    // and function return are shallow and can be construed as moving or
    // borrowing the object, and _dtor should only be called on one copy after
    // the last use of any copy.  It is an error to assign into a constructed
    // object.  To make a deep copy for types the support it, use _copy.
    //
    // li_thing a, b, c;
    // li_thing_ctor(&a);
    // b = a;
    // c = li_thing_copy(&b);
    // li_thing_dtor(&b);
    // li_thing_dtor(&c);
    
    // Preconditions are the responsibility of the caller and are checked only
    // in debug mode with assert. Functions report runtime errors (such as bad
    // alloc) by their return value, either li_status not LI_SUCCESS, or an
    // empty li_ type.
    
    
    
    // li_array(T) is an array of a dynamic number of instances of type T.
    //
    // li_array(int) v;
    // li_array_ctor(int)(&v);
    // li_array_resize(int)(&v, 10, 2); // 10 instances of 0
    // int total = 0;
    // LI_FOR(int, p, &v)
    //     total += *p;
    // li_array_dtor(&v);
    
#ifdef __clang__
#define LI_PRAGMA_IF_CLANG(X) _Pragma(X)
#else
#define LI_PRAGMA_IF_CLANG(X)
#endif
    
#define li_array_define( T )\
    \
    /* suppress harmless unused function warnings */ \
    LI_PRAGMA_IF_CLANG("clang diagnostic push")\
    LI_PRAGMA_IF_CLANG("clang diagnostic ignored \"-Wunused-function\"")\
    \
    typedef struct {\
        T* begin;\
        T* end;\
        T* capacity;\
    } li_array_##T ;\
    \
    inline static void li_array_##T##_ctor(li_array_##T* self) {\
        self->begin = 0;\
        self->end = 0;\
        self->capacity = 0;\
    }\
    \
    inline static T* li_array_##T##_begin(li_array_##T* self) {\
        assert(self);\
        return self->begin;\
    }\
    inline static T* li_array_##T##_end(li_array_##T* self) {\
        assert(self);\
        return self->end;\
    }\
    \
    inline static void li_array_##T##_clear(li_array_##T* self) { \
        assert(self);\
        LI_FOR(T, p, self)\
            T##_dtor(p);\
        self->end = self->begin;\
    }\
    \
    inline static void li_array_##T##_dtor(li_array_##T* self) {\
        assert(self);\
        li_array_clear(T)(self);\
        li_dealloc(self->begin);\
    }\
    \
    inline static size_t li_array_##T##_size(li_array_##T* self) {\
        assert(self);\
        assert(self->begin <= self->end);\
        return (size_t) (self->end - self->begin);\
    }\
    \
    inline static bool li_array_##T##_empty(li_array_##T* self) {\
        assert(self);\
        return self->end == self->begin;\
    }\
    \
    inline static li_status li_array_##T##_resize(li_array_##T* self, size_t n, T value) {\
        assert(self);\
        assert(self->begin <= self->end);\
        size_t a = (size_t) (self->end - self->begin);\
        assert(self->begin <= self->capacity);\
        size_t b = (size_t) (self->capacity - self->begin);\
        if (n <= a) {\
            for (T* p = self->begin + n; p != self->end; ++p)\
                T##_dtor(p);\
            self->end = self->begin + n;\
        } else if (n <= b) {\
            for (T* p = self->begin + a; p != self->begin + n; ++p)\
                *p = value;\
            self->end = self->begin + n;\
        } else {\
            size_t c = MAX(n, b * 2);\
            T* ptr = li_alloc(c * sizeof(T));\
            if (!ptr)\
                return LI_BAD_ALLOC;\
            memcpy(ptr, self->begin, a * sizeof(T));\
            for (T* p = ptr + a; p != ptr + n; ++p)\
                *p = value;\
            li_dealloc(self->begin);\
            self->begin = ptr;\
            self->end = ptr + n;\
            self->capacity = ptr + c;\
        }\
        return LI_SUCCESS;\
    }\
    \
    inline static li_status li_array_##T##_push(li_array_##T* self, T value) {\
        return li_array_##T##_resize(self, (size_t) (self->end - self->begin) + 1, value);\
    }\
    \
    /* unsuppress warnings */ \
    LI_PRAGMA_IF_CLANG("clang diagnostic pop")\
    \

#define li_array(T) li_array_##T
#define li_array_ctor(T) li_array_##T##_ctor
#define li_array_dtor(T) li_array_##T##_dtor
#define li_array_begin(T) li_array_##T##_begin
#define li_array_end(T) li_array_##T##_end
#define li_array_size(T) li_array_##T##_size
#define li_array_empty(T) li_array_##T##_empty
#define li_array_push(T) li_array_##T##_push
#define li_array_clear(T) li_array_##T##_clear
#define li_array_resize(T) li_array_##T##_resize
    
#define LI_FOR(T, P, PA) for (T* P = li_array_begin(T)(PA); P != li_array_end(T)(PA); ++P)
#define LI_REVERSE_FOR(T, P, PA) for (T* P = li_array_end(T)(PA) - 1; P >= li_array_begin(T)(PA); --P)

    // Support simple types in arrays
    
    inline static void li_byte_dtor(li_byte* self) {};
    li_array_define(li_byte);
    
    inline static void double_dtor(double* self) {};
    li_array_define(double);
    
    
    // li_string provides the additional guarantees that it is a null-terminated
    // UTF-8 string that should be destructed with li_dealloc.  In particular,
    // it is not a string literal.
    
    typedef char li_utf8;
    typedef li_utf8* li_string;
    
    void li_string_ctor(li_string* self);
    void li_string_dtor(li_string* self);
    li_string li_string_copy(const li_utf8* zstr);
    li_string li_string_from_range(const li_utf8* begin, const li_utf8* end);
    size_t li_string_size(li_string* self);
    li_status li_string_resize(li_string* self, size_t n, char fill);
    li_status li_string_insert(li_string* self, size_t pos, const li_utf8* zstr);

    li_array_define(li_string);

    
    // li_queue is a dynamic contiguous buffer.  Data is pushed onto the end
    // and popped from the beginning.  If there is insufficient room at the end
    // then we will either move the queue to the front of the storage, or
    // expand the storage.  To give put an amortized cost of O(1), we at least
    // double the storage size when we grow, and we grow rather than move if the
    // storage is more than half full.  Without these strategies, some usage
    // patterns make _put cost O(n).
    
    typedef struct {
        li_byte* begin;    // beginning of queue
        li_byte* end;      // end of queue
        li_byte* capacity; // end of allocation
        li_byte* data;     // beginning of allocation
    } li_queue;
    
    void li_queue_ctor(li_queue* self);
    void li_queue_dtor(li_queue* self);
    
    size_t li_queue_size(li_queue* self);
    li_status li_queue_will_put(li_queue* self, size_t count); // Prepare enough space to put count bytes
    
    void* li_queue_begin(li_queue* self);
    void* li_queue_end(li_queue* self);
    
    li_status li_queue_put(li_queue* self, const void* src, size_t count);
    li_status li_queue_get(li_queue* self, void* dest, size_t count);
    li_status li_queue_unput(li_queue* self, size_t count);
    li_status li_queue_unget(li_queue* self, size_t count); // Do not _unget across calls to _put or _will_put, as these may have recycled the data
    
    li_status li_queue_drop(li_queue* self, size_t count);
    void li_queue_clear(li_queue* self); // Does not release resources or invalidate _unget
    
    
    
    // li_bit_queue adapts li_queue to put and get by bits.  When _get only
    // partially overwrites a destination byte, the rest of the byte is
    // preserved, so ensure dest is initialized apprpriately.  There is no
    // mechanism yet to put or get starting from the middle of a byte.
    
    typedef struct {
        li_queue queue;
    } li_bit_queue;
    
    void li_bit_queue_ctor(li_bit_queue* self);
    void li_bit_queue_dtor(li_bit_queue* self);
    
    size_t li_bit_queue_size(li_bit_queue* self);
    void li_bit_queue_clear(li_bit_queue* self);
    
    li_status li_bit_queue_put(li_bit_queue* self, const void* src, size_t bits);
    li_status li_bit_queue_get(li_bit_queue* self, void* dest, size_t bits);
    li_status li_bit_queue_unget(li_bit_queue* self, size_t bits);
    
    
    

    // Helper macros to implement functions using li_status error codes

    // Propagate failures by returning values other than success
#define LI_DOUBT(X) do { li_status _result = ( X ); if (_result) return _result; } while(0)
    
    // When the calling context makes failure impossible, only assert on the
    // result in debug mode
#ifdef NDEBUG
#define LI_TRUST(X) X /* Ignore the result of the expression */
#else
#define LI_TRUST(X) assert(( X ) == LI_SUCCESS) /* Trust but verify */
#endif
    
    // Logging and breakpoint for anomalous returns
#ifdef NDEBUG
#define LI_ON_ERROR
#else
#define LI_ON_ERROR _li_on_error(__FILE__, __LINE__, __func__, result)
    li_status _li_on_error(const char* file, int line, const char* func, li_status result);
#endif
    
#ifdef __cplusplus
}
#endif

#endif /* liutility_h */
