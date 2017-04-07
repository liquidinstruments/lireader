#ifndef CAPN_C6A6E02BE7124911
#define CAPN_C6A6E02BE7124911
/* AUTO GENERATED - DO NOT EDIT */
#include "capnp_c.h"

#if CAPN_VERSION != 1
#error "version mismatch between capnp_c.h and generated code"
#endif


#ifdef __cplusplus
extern "C" {
#endif

struct LIHeader;
struct LIHeader_Channel;
struct LIData;
struct LIFileElement;

typedef struct {capn_ptr p;} LIHeader_ptr;
typedef struct {capn_ptr p;} LIHeader_Channel_ptr;
typedef struct {capn_ptr p;} LIData_ptr;
typedef struct {capn_ptr p;} LIFileElement_ptr;

typedef struct {capn_ptr p;} LIHeader_list;
typedef struct {capn_ptr p;} LIHeader_Channel_list;
typedef struct {capn_ptr p;} LIData_list;
typedef struct {capn_ptr p;} LIFileElement_list;

struct LIHeader {
	int8_t instrumentId;
	int16_t instrumentVer;
	double timeStep;
	int64_t startTime;
	double startOffset;
	LIHeader_Channel_list channels;
	capn_text csvFmt;
	capn_text csvHeader;
};

static const size_t LIHeader_word_count = 4;

static const size_t LIHeader_pointer_count = 3;

static const size_t LIHeader_struct_bytes_count = 56;

struct LIHeader_Channel {
	int8_t number;
	double calibration;
	capn_text recordFmt;
	capn_text procFmt;
};

static const size_t LIHeader_Channel_word_count = 2;

static const size_t LIHeader_Channel_pointer_count = 2;

static const size_t LIHeader_Channel_struct_bytes_count = 32;

struct LIData {
	int8_t channel;
	capn_data data;
};

static const size_t LIData_word_count = 1;

static const size_t LIData_pointer_count = 1;

static const size_t LIData_struct_bytes_count = 16;
enum LIFileElement_which {
	LIFileElement_header = 0,
	LIFileElement_data = 1
};

struct LIFileElement {
	enum LIFileElement_which which;
	union {
		LIHeader_ptr header;
		LIData_ptr data;
	};
};

static const size_t LIFileElement_word_count = 1;

static const size_t LIFileElement_pointer_count = 1;

static const size_t LIFileElement_struct_bytes_count = 16;

LIHeader_ptr new_LIHeader(struct capn_segment*);
LIHeader_Channel_ptr new_LIHeader_Channel(struct capn_segment*);
LIData_ptr new_LIData(struct capn_segment*);
LIFileElement_ptr new_LIFileElement(struct capn_segment*);

LIHeader_list new_LIHeader_list(struct capn_segment*, int len);
LIHeader_Channel_list new_LIHeader_Channel_list(struct capn_segment*, int len);
LIData_list new_LIData_list(struct capn_segment*, int len);
LIFileElement_list new_LIFileElement_list(struct capn_segment*, int len);

void read_LIHeader(struct LIHeader*, LIHeader_ptr);
void read_LIHeader_Channel(struct LIHeader_Channel*, LIHeader_Channel_ptr);
void read_LIData(struct LIData*, LIData_ptr);
void read_LIFileElement(struct LIFileElement*, LIFileElement_ptr);

void write_LIHeader(const struct LIHeader*, LIHeader_ptr);
void write_LIHeader_Channel(const struct LIHeader_Channel*, LIHeader_Channel_ptr);
void write_LIData(const struct LIData*, LIData_ptr);
void write_LIFileElement(const struct LIFileElement*, LIFileElement_ptr);

void get_LIHeader(struct LIHeader*, LIHeader_list, int i);
void get_LIHeader_Channel(struct LIHeader_Channel*, LIHeader_Channel_list, int i);
void get_LIData(struct LIData*, LIData_list, int i);
void get_LIFileElement(struct LIFileElement*, LIFileElement_list, int i);

void set_LIHeader(const struct LIHeader*, LIHeader_list, int i);
void set_LIHeader_Channel(const struct LIHeader_Channel*, LIHeader_Channel_list, int i);
void set_LIData(const struct LIData*, LIData_list, int i);
void set_LIFileElement(const struct LIFileElement*, LIFileElement_list, int i);

#ifdef __cplusplus
}
#endif
#endif
