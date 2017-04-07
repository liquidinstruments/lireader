#include "li.capnp.h"
/* AUTO GENERATED - DO NOT EDIT */
static const capn_text capn_val0 = {0,""};

LIHeader_ptr new_LIHeader(struct capn_segment *s) {
	LIHeader_ptr p;
	p.p = capn_new_struct(s, 32, 3);
	return p;
}
LIHeader_list new_LIHeader_list(struct capn_segment *s, int len) {
	LIHeader_list p;
	p.p = capn_new_list(s, len, 32, 3);
	return p;
}
void read_LIHeader(struct LIHeader *s, LIHeader_ptr p) {
	capn_resolve(&p.p);
	s->instrumentId = (int8_t) ((int8_t)capn_read8(p.p, 0));
	s->instrumentVer = (int16_t) ((int16_t)capn_read16(p.p, 2));
	s->timeStep = capn_to_f64(capn_read64(p.p, 8));
	s->startTime = (int64_t) ((int64_t)(capn_read64(p.p, 16)));
	s->startOffset = capn_to_f64(capn_read64(p.p, 24));
	s->channels.p = capn_getp(p.p, 0, 0);
	s->csvFmt = capn_get_text(p.p, 1, capn_val0);
	s->csvHeader = capn_get_text(p.p, 2, capn_val0);
}
void write_LIHeader(const struct LIHeader *s, LIHeader_ptr p) {
	capn_resolve(&p.p);
	capn_write8(p.p, 0, (uint8_t) (s->instrumentId));
	capn_write16(p.p, 2, (uint16_t) (s->instrumentVer));
	capn_write64(p.p, 8, capn_from_f64(s->timeStep));
	capn_write64(p.p, 16, (uint64_t) (s->startTime));
	capn_write64(p.p, 24, capn_from_f64(s->startOffset));
	capn_setp(p.p, 0, s->channels.p);
	capn_set_text(p.p, 1, s->csvFmt);
	capn_set_text(p.p, 2, s->csvHeader);
}
void get_LIHeader(struct LIHeader *s, LIHeader_list l, int i) {
	LIHeader_ptr p;
	p.p = capn_getp(l.p, i, 0);
	read_LIHeader(s, p);
}
void set_LIHeader(const struct LIHeader *s, LIHeader_list l, int i) {
	LIHeader_ptr p;
	p.p = capn_getp(l.p, i, 0);
	write_LIHeader(s, p);
}

LIHeader_Channel_ptr new_LIHeader_Channel(struct capn_segment *s) {
	LIHeader_Channel_ptr p;
	p.p = capn_new_struct(s, 16, 2);
	return p;
}
LIHeader_Channel_list new_LIHeader_Channel_list(struct capn_segment *s, int len) {
	LIHeader_Channel_list p;
	p.p = capn_new_list(s, len, 16, 2);
	return p;
}
void read_LIHeader_Channel(struct LIHeader_Channel *s, LIHeader_Channel_ptr p) {
	capn_resolve(&p.p);
	s->number = (int8_t) ((int8_t)capn_read8(p.p, 0));
	s->calibration = capn_to_f64(capn_read64(p.p, 8));
	s->recordFmt = capn_get_text(p.p, 0, capn_val0);
	s->procFmt = capn_get_text(p.p, 1, capn_val0);
}
void write_LIHeader_Channel(const struct LIHeader_Channel *s, LIHeader_Channel_ptr p) {
	capn_resolve(&p.p);
	capn_write8(p.p, 0, (uint8_t) (s->number));
	capn_write64(p.p, 8, capn_from_f64(s->calibration));
	capn_set_text(p.p, 0, s->recordFmt);
	capn_set_text(p.p, 1, s->procFmt);
}
void get_LIHeader_Channel(struct LIHeader_Channel *s, LIHeader_Channel_list l, int i) {
	LIHeader_Channel_ptr p;
	p.p = capn_getp(l.p, i, 0);
	read_LIHeader_Channel(s, p);
}
void set_LIHeader_Channel(const struct LIHeader_Channel *s, LIHeader_Channel_list l, int i) {
	LIHeader_Channel_ptr p;
	p.p = capn_getp(l.p, i, 0);
	write_LIHeader_Channel(s, p);
}

LIData_ptr new_LIData(struct capn_segment *s) {
	LIData_ptr p;
	p.p = capn_new_struct(s, 8, 1);
	return p;
}
LIData_list new_LIData_list(struct capn_segment *s, int len) {
	LIData_list p;
	p.p = capn_new_list(s, len, 8, 1);
	return p;
}
void read_LIData(struct LIData *s, LIData_ptr p) {
	capn_resolve(&p.p);
	s->channel = (int8_t) ((int8_t)capn_read8(p.p, 0));
	s->data = capn_get_data(p.p, 0);
}
void write_LIData(const struct LIData *s, LIData_ptr p) {
	capn_resolve(&p.p);
	capn_write8(p.p, 0, (uint8_t) (s->channel));
	capn_setp(p.p, 0, s->data.p);
}
void get_LIData(struct LIData *s, LIData_list l, int i) {
	LIData_ptr p;
	p.p = capn_getp(l.p, i, 0);
	read_LIData(s, p);
}
void set_LIData(const struct LIData *s, LIData_list l, int i) {
	LIData_ptr p;
	p.p = capn_getp(l.p, i, 0);
	write_LIData(s, p);
}

LIFileElement_ptr new_LIFileElement(struct capn_segment *s) {
	LIFileElement_ptr p;
	p.p = capn_new_struct(s, 8, 1);
	return p;
}
LIFileElement_list new_LIFileElement_list(struct capn_segment *s, int len) {
	LIFileElement_list p;
	p.p = capn_new_list(s, len, 8, 1);
	return p;
}
void read_LIFileElement(struct LIFileElement *s, LIFileElement_ptr p) {
	capn_resolve(&p.p);
	s->which = (enum LIFileElement_which)(int) capn_read16(p.p, 0);
	switch (s->which) {
	case LIFileElement_header:
	case LIFileElement_data:
		s->data.p = capn_getp(p.p, 0, 0);
		break;
	default:
		break;
	}
}
void write_LIFileElement(const struct LIFileElement *s, LIFileElement_ptr p) {
	capn_resolve(&p.p);
	capn_write16(p.p, 0, s->which);
	switch (s->which) {
	case LIFileElement_header:
	case LIFileElement_data:
		capn_setp(p.p, 0, s->data.p);
		break;
	default:
		break;
	}
}
void get_LIFileElement(struct LIFileElement *s, LIFileElement_list l, int i) {
	LIFileElement_ptr p;
	p.p = capn_getp(l.p, i, 0);
	read_LIFileElement(s, p);
}
void set_LIFileElement(const struct LIFileElement *s, LIFileElement_list l, int i) {
	LIFileElement_ptr p;
	p.p = capn_getp(l.p, i, 0);
	write_LIFileElement(s, p);
}
