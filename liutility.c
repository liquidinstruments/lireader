//
//  liutility.c
//  liquidreader
//
//  Created by Antony Searle on 17/10/16.
//  Copyright © 2016 Antony Searle. All rights reserved.
//

#include "liutility.h"

#include <assert.h>
#include <stdlib.h>
#include <string.h>

#include "liparse.h"

#include "bitcpy.h"


void* (*li_alloc)(size_t n) = malloc;
void (*li_dealloc)(void* ptr) = free;


li_status _li_on_error(const char* file, int line, const char* func, li_status result) {
    if (result != LI_SUCCESS) {
        fprintf(stderr, "%s@%d: %s error in %s\n", file, line, li_status_string(result), func);
        // This is a good place for a breakpoint
    }
    return result;
}







// li_queue operations

void li_queue_ctor(li_queue* self) {
    assert(self);
    memset(self, 0, sizeof(li_queue));
}

void li_queue_dtor(li_queue* self) {
    assert(self);
    li_dealloc(self->data);
}

size_t li_queue_size(li_queue* self) {
    assert(self);
    assert(self->begin <= self->end);
    return (size_t) (self->end - self->begin);
}

li_status li_queue_will_put(li_queue* self, size_t count) {
    assert(self);
    ptrdiff_t c = self->capacity - self->end;
    assert(c >= 0);
    if (c < (ptrdiff_t) count) {
        // Not enough space
        ptrdiff_t a = self->begin - self->data;
        ptrdiff_t b = self->end - self->begin;
        assert(a >= 0);
        assert(b >= 0);
        // If the queue would be less than half full move the data to the front
        if ((b + (ptrdiff_t) count) * 2 < (a + b + c)) {
            memmove(self->data, self->begin, b);
        } else {
            // Otherwise, reallocate growing the storage by at least 2
            ptrdiff_t n = MAX((ptrdiff_t)(b + (ptrdiff_t) count), (a + b + c) * 2);
            assert(n >= 0);
            li_byte* ptr = (li_byte*) li_alloc((size_t) n);
            if (!ptr)
                return LI_BAD_ALLOC;
            memcpy(ptr, self->begin, b);
            li_dealloc(self->data);
            self->data = ptr;
            self->capacity = self->data + n;
        }
        // Either way, the data is now at the front of the storage
        self->begin = self->data;
        self->end = self->begin + b;
    }
    return LI_SUCCESS;
}

li_status li_queue_put(li_queue* self, const void* src, size_t count) {
    assert(self && (src || !count));
    LI_DOUBT(li_queue_will_put(self, count));
    memcpy(self->end, src, count);
    self->end += count;
    return LI_SUCCESS;
}

li_status li_queue_get(li_queue* self, void* dest, size_t count) {
    assert(self && dest && count);
    size_t n = li_queue_size(self);
    if (n < count)
        return LI_SMALL_SRC;
    memcpy(dest, self->begin, count);
    self->begin += count;
    return LI_SUCCESS;
}

li_status li_queue_unput(li_queue* self, size_t count) {
    assert(self);
    if (self->end < self->begin + count)
        return LI_SMALL_SRC;
    self->end -= count;
    return LI_SUCCESS;
}

// li_queue_unget relies on the old data sill existing in queue storage.
// Calling li_queue_put or li_queue_will_put can destroy that data.

li_status li_queue_unget(li_queue* self, size_t count) {
    assert(self);
    if (self->begin - count < self->data)
        return LI_SMALL_SRC;
    self->begin -= count;
    return LI_SUCCESS;
}

void* li_queue_begin(li_queue* self) {
    assert(self);
    return self->begin;
}

void* li_queue_end(li_queue* self) {
    assert(self);
    return self->end;
}

enum li_status li_queue_drop(li_queue* self, size_t count) {
    assert(self);
    if (self->begin + count > self->end)
        return LI_SMALL_SRC;
    self->begin += count;
    return LI_SUCCESS;
}

void li_queue_clear(li_queue* self) {
    assert(self);
    // Setting end to begin doesn't invalidate unget
    self->end = self->begin;
}









// li_bit_queue provides a queue of bits to help us extract fields that aren't
// an integer number of bytes.  Currently just a wrapper around li_queue that
// uses


void li_bit_queue_ctor(li_bit_queue* self) {
    assert(self);
    li_queue_ctor(&self->queue);
    self->begin_offset = 0;
    self->end_offset = 0;
}

void li_bit_queue_dtor(li_bit_queue* self) {
    assert(self);
    li_queue_dtor(&self->queue);
}

size_t li_bit_queue_size(li_bit_queue* self) {
    assert(self);
    return (li_queue_size(&self->queue) << 3) + self->end_offset - self->begin_offset;
}

li_status li_bit_queue_put(li_bit_queue* self, const void* src, size_t bits) {
    assert(self && (src || !bits));
    assert((0 <= self->end_offset) && (self->end_offset < LI_BITS_PER_BYTE));
    LI_DOUBT(li_queue_will_put(&self->queue, (bits + 7) >> 3));
    bitcpy(self->queue.end, (int) self->end_offset, src, 0, bits);
    self->end_offset += bits;
    self->queue.end += (self->end_offset >> 3);
    self->end_offset &= 7;
    return LI_SUCCESS;
}


li_status li_bit_queue_get(li_bit_queue* self, void* dest, size_t bits) {
    assert(self && (dest || !bits));
    assert((0 <= self->begin_offset) && (self->begin_offset < LI_BITS_PER_BYTE));
    if (bits > li_bit_queue_size(self))
        return LI_SMALL_SRC;
    bitcpy(dest, 0, self->queue.begin, (int) self->begin_offset, bits);
    self->begin_offset += bits;
    self->queue.begin += (self->begin_offset >> 3);
    self->begin_offset &= 7;
    return LI_SUCCESS;
}

li_status li_bit_queue_unget_bits(li_bit_queue* self, size_t bits) {
    assert(self);
    assert((0 <= self->begin_offset) && (self->begin_offset < LI_BITS_PER_BYTE));
    if ((((self->queue.begin - self->queue.data) << 3) + self->begin_offset) < bits) {
        return LI_SMALL_SRC;
    }
    self->begin_offset -= bits;
    self->queue.begin += (self->begin_offset >> 3);
    self->begin_offset &= 7;
    assert(self->queue.begin >= self->queue.data);
    return LI_SUCCESS;
}

void li_bit_queue_clear(li_bit_queue* self) {
    assert(self);
    li_queue_clear(&self->queue);
    self->begin_offset = 0;
    self->end_offset = 0;
}


// li_string operations

void li_string_ctor(li_string* self) {
    assert(self);
    *self = 0;
}

void li_string_dtor(li_string* self) {
    assert(self);
    li_dealloc(*self);
}

size_t li_string_size(li_string* self) {
    assert(self);
    // strlen is not required to handle null pointer
    return *self ? strlen(*self) : 0;
}

li_status li_string_resize(li_string* self, size_t n, li_utf8 fill) {
    assert(fill);
    size_t m = li_string_size(self);
    if (m < n) {
        // we must reallocate
        li_string p = li_alloc(n + 1);
        if (!p)
            return LI_BAD_ALLOC;
        memcpy(p, *self, m);
        memset(p + m, fill, n - m);
        li_dealloc(*self);
        *self = p;
    }
    if (*self)
        (*self)[n] = 0;
    return LI_SUCCESS;
}

li_string li_string_copy(const li_utf8* zstr) {
    assert(zstr);
    size_t n = strlen(zstr) + 1;
    li_string p = li_alloc(n);
    if (p)
        memcpy(p, zstr, n);
    return p;
}

li_string li_string_from_range(const li_utf8* begin, const li_utf8* end) {
    ptrdiff_t n = end - begin;
    assert(n >= 0);
    li_string p = li_alloc((size_t) n + 1);
    if (p) {
        memcpy(p, begin, n);
        p[n] = 0;
    }
    return p;
}

li_status li_string_insert(li_string* self, size_t pos, const li_utf8* zstr) {
    assert(self);
    assert(*self);
    assert(zstr);
    size_t a = strlen(*self);
    assert(pos <= a);
    size_t b = strlen(zstr);
    // We have no way of knowing if there is more space available in the
    // allocation so we always create a new string
    li_string c = li_alloc(a + b + 1);
    if (!c)
        return LI_BAD_ALLOC;
    memcpy(c, *self, pos);
    memcpy(c + pos, zstr, b);
    memcpy(c + pos + b, *self + pos, a - pos + 1);
    li_string_dtor(self);
    *self = c;
    return LI_SUCCESS;
}














