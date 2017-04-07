//
//  lireader.c
//  liquidreader
//
//  Created by Antony Searle on 21/06/2016.
//  Copyright (c) 2016 Liquid Instruments. All rights reserved.
//

#include "lireader.h"

#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <ctype.h>
#include <string.h>
#include <math.h>

#include <assert.h>
#ifndef NDEBUG
#include <stdio.h>
#endif

#include "liparse.h"
#include "liutility.h"

#include "capnp_c.h"
#include "li.capnp.h"


typedef enum // What stage of reading the file are we in?
{
    INIT, // Initial state - read magic number and version
    HEAD, // Header state - read the whole header
    BODY, // Body state - read records
    BAD,  // Bad state - data read is not consistent with the file format
} State;




// Header bundles the data in the format header


typedef struct {
    int8_t number;
    double calibration;
    li_string recordFmt;
    li_string procFmt;
} li_header_channel;

static void li_header_channel_ctor(li_header_channel* self) {
    assert(self);
    self->calibration = 0;
    self->number = 0;
    li_string_ctor(&self->procFmt);
    li_string_ctor(&self->recordFmt);
}

static void li_header_channel_dtor(li_header_channel* self) {
    assert(self);
    li_string_dtor(&self->procFmt);
    li_string_dtor(&self->recordFmt);
}

li_array_define(li_header_channel);

typedef struct {
    int8_t instrumentId;
    int16_t instrumentVer;
    double timeStep;
    int64_t startTime;
    double startOffset;
    li_array(li_header_channel) channels;
    char* csvFmt;
    char* csvHeader;
} li_header;

/*
typedef struct {
    int8_t channel;
    li_array* data;
} li_data;

 */

typedef struct {
    int8_t number;
    li_array(Record) recs;
    li_array(li_array_Operation) procs;
    size_t rec_bytes;
    li_queue queue;
} Parsed;

static void Parsed_ctor(Parsed* self) {
    self->number = 0;
    li_array_ctor(li_array_Operation)(&self->procs);
    li_queue_ctor(&self->queue);
    self->rec_bytes = 0;
    li_array_ctor(Record)(&self->recs);
    
}

static void Parsed_dtor(Parsed* self) {
    li_queue_dtor(&self->queue);
    li_array_dtor(li_array_Operation)(&self->procs);
    li_array_dtor(Record)(&self->recs);
}


li_array_define(Parsed);

struct li_reader {
    State state;
    li_queue queue;
    uint64_t suggested_put;
    char version;
    li_header header;
    li_array(Parsed) parsed;
    size_t bytes_per_output;
};


const char* li_version_string() {
    return "0.2.0";
}

char* li_status_string_[] = {
    "Success",
    "Invalid argument",
    "Bad alloc",
    "Small source buffer",
    "Small destination buffer",
    "Bad format",
    "Unimplemented"
};

const char* li_status_string(li_status status) {
    return li_status_string_[MIN(status, LI_UNIMPLEMENTED)];
}








static void li_header_ctor(li_header* self) {
    assert(self);
    li_array_ctor(li_header_channel)(&self->channels);
    li_string_ctor(&self->csvFmt);
    li_string_ctor(&self->csvHeader);
    self->instrumentId = 0;
    self->instrumentVer = 0;
    self->startOffset = 0;
    self->startTime = 0;
    self->timeStep = 0;
}

static void li_header_dtor(li_header* self) {
    li_array_dtor(li_header_channel)(&self->channels);
    li_string_dtor(&self->csvHeader);
    li_string_dtor(&self->csvFmt);
}



static void li_reader_ctor(li_reader* self) {
    self->bytes_per_output = 0;
    li_header_ctor(&self->header);
    li_array_ctor(Parsed)(&self->parsed);
    li_queue_ctor(&self->queue);
    self->state = INIT;
    self->suggested_put = 3;
    self->version = 0;
}

static void li_reader_dtor(li_reader* self) {
    li_array_dtor(Parsed)(&self->parsed);
    li_queue_dtor(&self->queue);
    li_header_dtor(&self->header);
}


li_reader* li_init(void* (*alloc)(size_t),
                          void (*dealloc)(void*))
{
    li_alloc = alloc;
    li_dealloc = dealloc;
    li_reader* self = li_alloc(sizeof(li_reader));
    li_reader_ctor(self);
    return self;
}

void li_finalize(struct li_reader* self) {
    li_reader_dtor(self);
    li_dealloc(self);
    li_alloc = malloc;
    li_dealloc = free;
}

li_status li_put(struct li_reader* self, const void* src, size_t count) {
    return li_queue_put(&self->queue, src, count);
}

static char* binary_description_string(li_queue* self) {
    uint16_t n = 0;
    char* s = 0;
    if (li_queue_get(self, &n, sizeof(n)) == LI_SUCCESS) {
        s = li_alloc(n + 1);
        if (s) {
            if (li_queue_get(self, s, n) == LI_SUCCESS) {
                s[n] = 0;
            } else {
                li_dealloc(s);
                s = 0;
            }
        }
    }
    return s;
}

void li_reader_Header_derived(li_reader* self) {
    
    // Header is now valid, compute derived quantities
    
    self->bytes_per_output = 0;
 
    LI_FOR(li_header_channel, p, &self->header.channels) {
    
        Parsed x;
        Parsed_ctor(&x);
        
        x.number = p->number;
        x.recs = li_parse_Record_list(p->recordFmt);
        
        size_t bits = 0;
        LI_FOR(Record, r, &x.recs)
            bits += r->width;
        x.rec_bytes = bits / 8;
        
        x.procs = li_parse_Operation_list_list(p->procFmt, p->calibration);
        self->bytes_per_output += li_array_size(li_array_Operation)(&x.procs) * 8;
        
        li_array_push(Parsed)(&self->parsed, x);
    }    
    assert(self->bytes_per_output);
}


void li_reader_Header1(li_reader* self) {
    if (!self)
        return;
    size_t n = li_queue_size(&self->queue);
    if (n < 2)
        return;
    int16_t length = 0;
    li_queue_get(&self->queue, &length, 2);
    if ((ptrdiff_t) n < 2 + (ptrdiff_t) length) {
        li_queue_unget(&self->queue, 2);
        self->suggested_put = 2 + (size_t) length;
        return;
    }
    li_byte channelSelect;
    li_queue_get(&self->queue, &channelSelect, 1);
    for (int i = 0; i != 7; ++i)
        if ((channelSelect >> i) & 1) {
            li_header_channel c;
            li_header_channel_ctor(&c);
            c.number = i + 1;
            li_array_push(li_header_channel)(&self->header.channels, c);
        }
    
    li_queue_get(&self->queue, &self->header.instrumentId, 1);
    li_queue_get(&self->queue, &self->header.instrumentVer, 2);
    li_queue_get(&self->queue, &self->header.timeStep, 8);
    li_queue_get(&self->queue, &self->header.startTime, 8);
    self->header.startOffset = 0.0;
    
    LI_FOR(li_header_channel, p, &self->header.channels)
        li_queue_get(&self->queue, &p->calibration, 8);
    
    char* rec = binary_description_string(&self->queue);
    LI_FOR(li_header_channel, p, &self->header.channels)
        p->recordFmt = li_string_copy(rec);
    li_string_dtor(&rec);
    
    LI_FOR(li_header_channel, p, &self->header.channels)
        p->procFmt = binary_description_string(&self->queue);

    self->header.csvFmt = binary_description_string(&self->queue);
    self->header.csvHeader = binary_description_string(&self->queue);
    
    self->state = BODY;
    self->suggested_put = 3;
    
    li_reader_Header_derived(self);
    
}

uint32_t pad8(uint32_t x) {
    return (x + 7) & ~((uint32_t) 7);
}

// Read a Cap'n proto message into an LIFileElement
bool li_reader_FileElement(li_reader* self, struct capn* pc, struct LIFileElement* pfe) {

    assert(self);
    
    void* begin = li_queue_begin(&self->queue);
    size_t n = li_queue_size(&self->queue);
    if (li_queue_size(&self->queue) < 4)
        return false;
    uint32_t segments = 0;
    li_queue_get(&self->queue, &segments, 4);
    segments += 1;
    uint32_t padded = pad8(4 + segments * 4);
    if (n < padded) {
        self->suggested_put = padded;
        self->queue.begin = begin;
        return false;
    }
    uint32_t payload = 0;
    for (uint32_t i = 0; i != segments; ++i) {
        uint32_t m;
        li_queue_get(&self->queue, &m, 4);
        payload += m;
    }
    payload *= 8;
    size_t total = padded + payload;
    if (n < total) {
        self->suggested_put = total;
        self->queue.begin = begin;
        return false;
    }
    // We now have enough data to read the whole Cap'n Proto message
    
    capn_init_mem(pc, (uint8_t*) begin, total, 0);
    
    LIFileElement_list fel;
    fel.p = capn_root(pc);
    assert(fel.p.len == 1);
    
    get_LIFileElement(pfe, fel, 0);

    // Rewind the queue then drop all the daa Cap'n Proto consumed
    self->queue.begin = begin;
    li_queue_drop(&self->queue, total);

    return true;
    
}

// Read the header from a Cap'n Proto message
void li_reader_Header2(li_reader* self) {
    struct capn captain;
    struct LIFileElement file_element;
    
    if (!li_reader_FileElement(self, &captain, &file_element))
        return;
    
    assert(file_element.which == LIFileElement_header);
    
    struct LIHeader h;
    read_LIHeader(&h, file_element.header);
    
    self->header.instrumentId = h.instrumentId;
    self->header.instrumentVer = h.instrumentVer;
    self->header.timeStep = h.timeStep;
    self->header.startTime = h.startTime;
    self->header.startOffset = h.startOffset;
    
    capn_resolve(&h.channels.p);
    for (int i = 0; i != h.channels.p.len; ++i) {
        struct LIHeader_Channel hc;
        get_LIHeader_Channel(&hc, h.channels, i);
        li_header_channel c;
        li_header_channel_ctor(&c);
        c.number = hc.number;
        c.calibration = hc.calibration;
        c.recordFmt = li_string_copy(hc.recordFmt.str);
        c.procFmt = li_string_copy(hc.procFmt.str);
        li_array_push(li_header_channel)(&self->header.channels, c);
    }
    
    self->header.csvFmt = li_string_copy(h.csvFmt.str);
    self->header.csvHeader = li_string_copy(h.csvHeader.str);
    
    capn_free(&captain);
    
    self->state = BODY;
    self->suggested_put = 4;
    
    li_reader_Header_derived(self);
}

void li_reader_Data1(li_reader* self) {
    for (;;) {
        size_t n = li_queue_size(&self->queue);
        if (n < 3) {
            self->suggested_put = 3;
            return;
        }
        uint8_t channel;
        li_queue_get(&self->queue, &channel, sizeof(channel));
        
        // convert zero-based to one-based (needed for V1)
        ++channel;
        
        uint16_t length;
        li_queue_get(&self->queue, &length, sizeof(length));
        size_t total = 3 + length;
        if (n < total) {
            li_queue_unget(&self->queue, 3);
            self->suggested_put = total;
            return;
        }
        
        bool flag = false;
        LI_FOR(Parsed, p, &self->parsed)
            if (p->number == channel) {
                li_queue_put(&p->queue, self->queue.begin, length);
                li_queue_drop(&self->queue, length);
                flag = true;
            }
        assert(flag);
    }
}

void li_reader_Data2(li_reader* self) {
    for (;;) {
        
        struct capn captain;
        struct LIFileElement file_element;
        
        if (!li_reader_FileElement(self, &captain, &file_element))
            return;
        
        assert(file_element.which == LIFileElement_data);
        
        struct LIData d;
        read_LIData(&d, file_element.data);
        capn_resolve(&d.data.p);
        
        int ch = d.channel;
        
        // convert zero-based to one-based (needed for V2 prerelease files)
        // ++ch;
        
        bool flag = false;
        LI_FOR(Parsed, p, &self->parsed)
            if (p->number == ch) {
                li_queue_put(&p->queue, d.data.p.data, (size_t) d.data.p.len);
                flag = true;
            }
        assert(flag);

        capn_free(&captain);
    }
}

double Operation_apply(Operation o, double d) {
    switch (o.op) {
        case '*':
            d = d * o.value;
            break;
        case '/':
            d = d / o.value;
            break;
        case '+':
            d = d + o.value;
            break;
        case '-':
            d = d - o.value;
            break;
        case '&':
            d = ((intmax_t) d) & ((intmax_t) o.value);
            break;
        case 's':
            d = sqrt(d);
            break;
        case '^':
            d = pow(d, o.value);
            break;
        case 'f':
            d = floor(d);
            break;
        case 'c':
            d = ceil(d);
            break;
        default:
            assert(false);
            break;
    }
    return d;
}



double Operations_apply(li_array(Operation)* ops, double d) {
    LI_FOR(Operation, p, ops)
        d = Operation_apply(*p, d);
    return d;
}


li_status li_get(struct li_reader* self,
                      enum li_target target,
                      size_t index,
                      void* dest,
                      size_t count) {
    
    if (!self)
        return LI_INVALID_ARGUMENT;
    
    if (self->state == BAD)
        return LI_BAD_FORMAT;
    
#define GET(LVALUE)\
do {\
enum li_status result = li_queue_get(&self->queue, &(LVALUE), sizeof(LVALUE));\
if (result != LI_SUCCESS)\
return result;\
} while(0)
    
#define PUT(LVALUE)\
do {\
if (count < sizeof(LVALUE))\
return LI_SMALL_DEST;\
memcpy(dest, &(LVALUE), sizeof(LVALUE));\
return LI_SUCCESS;\
} while(0)
    
    if (target == LI_SUGGESTED_PUT_U64) {
        uint64_t a = self->suggested_put;
        uint64_t b = li_queue_size(&self->queue);
        uint64_t c = (a > b) ? (a - b) : 0;
        PUT(c);
    }
    
    if (self->state == INIT) {
        if (li_queue_size(&self->queue) >= 3) {
            char magic[2] = {'L', 'I'};
            char buffer[2];
            li_queue_get(&self->queue, buffer, 2);
            for (int i = 0; i != 2; ++i) {
                if (buffer[i] != magic[i]) {
                    self->state = BAD;
                    li_queue_unget(&self->queue, 2);
                    return LI_BAD_FORMAT;
                }
            }
            li_queue_get(&self->queue, &self->version, 1);
            switch (self->version) {
                case '1':
                    self->suggested_put = 2;
                    break;
                default:
                    self->suggested_put = 4;
                    break;
            }
            self->state = HEAD;
        }
    }
    
    if (self->state == HEAD) {
        switch (self->version) {
            case '1':
                li_reader_Header1(self);
                break;
            default:
                li_reader_Header2(self);
                break;
        }
    }
    
    if (self->state == BODY) {
        
        // Process as much data as is available
        switch (self->version) {
            case '1':
                li_reader_Data1(self);
                break;
            default:
                li_reader_Data2(self);
                break;
        }
        
        if (target == LI_CHANNEL_SELECT_U8) {
            uint8_t x = 0;
            LI_FOR(li_header_channel, p, &self->header.channels)
                x |= 1 << (p->number - 1);
            PUT(x);
        }
        if (target == LI_INSTRUMENT_ID_U64) {
            uint64_t x = (uint64_t) self->header.instrumentId;
            PUT(x);
        }
        if (target == LI_INSTRUMENT_VERSION_U64) {
            uint64_t x = (uint64_t) self->header.instrumentVer;
            PUT(x);
        }
        if (target == LI_TIME_STEP_F64) {
            double x = self->header.timeStep;
            PUT(x);
        }
        if (target == LI_START_TIME_U64) {
            uint64_t x = (uint64_t) self->header.startTime;
            PUT(x);
        }
        if (target == LI_START_OFFSET_F64) {
            double x = self->header.startOffset;
            PUT(x);
        }
        if (target == LI_FMT_STRING_BYTES_U64) {
            uint64_t x = strlen(self->header.csvFmt) + 1;
            PUT(x);
        }
        if (target == LI_FMT_STRING_UTF8V) {
            if (strlen(self->header.csvFmt) + 1 > count)
                return LI_SMALL_DEST;
            strcpy(dest, self->header.csvFmt);
            return LI_SUCCESS;
        }
        if (target == LI_HDR_STRING_BYTES_U64) {
            uint64_t x = strlen(self->header.csvHeader) + 1;
            PUT(x);
        }
        if (target == LI_HDR_STRING_UTF8V) {
            if (strlen(self->header.csvHeader) + 1 > count)
                return LI_SMALL_DEST;
            strcpy(dest, self->header.csvHeader);
            return LI_SUCCESS;
        }
        
        if (target == LI_RECORD_BYTES_U64) {
            uint64_t x = self->bytes_per_output;
            PUT(x);
        }
        
        if (target == LI_COUNT_FOR_INDEX_U64) {
            LI_FOR(Parsed, p, &self->parsed)
                if ((size_t) p->number == index) {
                    uint64_t x = li_array_size(li_array_Operation)(&p->procs);
                    PUT(x);
                }
            // We didn't find the requested channel
            return LI_INVALID_ARGUMENT;
        }
        
        if (target == LI_RECORD_F64V) {
            
            if (count < self->bytes_per_output)
                return LI_SMALL_DEST;
            double* output = dest;
            
            // Check that we have enough data for each channel to parse a record
            LI_FOR(Parsed, p, &self->parsed)
                if (li_queue_size(&p->queue) < p->rec_bytes)
                    return LI_SMALL_SRC;
            
            LI_FOR(Parsed, p, &self->parsed) {
                li_bit_queue bits;
                li_bit_queue_ctor(&bits);
                li_bit_queue_put(&bits, p->queue.begin, p->rec_bytes * LI_BITS_PER_BYTE);
                li_queue_drop(&p->queue, p->rec_bytes);
                
                li_array(Operation)* proc_iter = li_array_begin(li_array_Operation)(&p->procs);
                LI_FOR(Record, r, &p->recs) {
                    li_number x;
                    li_number_ctor(&x);
                    li_bit_queue_get(&bits, &x, r->width);
                    x.type = r->type;
                    x.width = r->width;
                    li_number_fix_sign(&x);
                    if (r->literal.type) {
                        assert(li_number_equal(x, r->literal));
                    } else {
                        double y = li_number_double(x);
                        double z = Operations_apply(proc_iter++, y);
                        *output++ = z;
                    }
                }
                li_bit_queue_dtor(&bits);
            }
            assert((output - (double*) dest) == (self->bytes_per_output/sizeof(double)));
            return LI_SUCCESS;
        }
        
        // No more enums to match
        return LI_INVALID_ARGUMENT;
    }
    
    if (self->state == BAD) {
        return LI_BAD_FORMAT;
    }
    
    return LI_SMALL_SRC;

#undef PUT
#undef GET
    
}
