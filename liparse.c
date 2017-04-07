//
//  liparse.c
//  liquidreader
//
//  Created by Antony Searle on 3/2/17.
//  Copyright © 2017 Antony Searle. All rights reserved.
//

#include "liparse.h"

#include <assert.h>
#include <ctype.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

// Parsing library
//
// We compose parsers from functions with the signature
//
//     bool (*)(char**, void*)
//
// The function must return true to indicate a match, and advance the char
// pointer to the end of the match; or return false to indicate no match, and
// leave the char* undisturbed.  The second argument is for arbitrary user
// data.  The char* is null-terminated and the null character is never matched.



typedef bool (*match_type)(char**, void*);



// Single-character parsers

// Match an explict char
bool match_char(char** p, void* ch) {
    assert(p && *p && ch && *((char*) ch));
    return (**p == *((char*) ch)) && ++*p;
}

// Match any car but this
bool match_not_char(char** p, void* ch) {
    assert(p && *p && ch);
    return **p && (**p != *((char*) ch)) && ++*p;
}


// Signature of ctype.h predicates such as isalpha
typedef int (*is_type)(char);

// Match using a predicate such as isalpha
bool match_is(char** p, void* is) {
    assert(p && *p);
    return **p && ((is_type) is)(**p) && ++*p;
}

// Match using a negated predicate such as isalpha
bool match_not_is(char** p, void* is) {
    assert(p && *p);
    return **p && !((is_type) is)(**p) && ++*p;
}

// Match a single character if it appears in the string
bool match_from(char** p, void* zs) {
    assert(p && *p && zs);
    char* s = zs;
    while (*s && (*s != **p))
        ++s;
    return *s && ++*p;
}

// Match a single character if does not appear in the string
bool match_not_from(char**p, void* zs) {
    assert(p && *p && zs);
    char* s = zs;
    while (*s && (*s != **p))
        ++s;
    return !*s && **p && ++*p;
}


// Multi-character matches

// Match contiguous whitespace of any length
bool match_whitespace(char** p, void* ignored) {
    assert(p && *p);
    while (isspace(**p))
        ++*p;
    return true;
}

// Match a literal string
bool match_literal(char** p, void* zs) {
    assert(p && *p && zs);
    char* q = *p;
    char* s = zs;
    while (*s && *q && (*s == *q))
        ++q, ++s;
    return !*s && (*p = q);
}



// Composite matchers that consume other match functions as their arguments

// A struct that contains a match function and its user data
typedef struct {
    bool (*func)(char**, void*);
    void* arg;
} bind;

// Indrectly invoke a match function; useful for implemntation
bool match_indirect(char** p, void* bd) {
    assert(bd && ((bind*)bd)->func);
    return ((bind*) bd)->func(p, ((bind*) bd)->arg);
}



// Match something zero or once, zero or more, one or more times
bool match_optional(char** p, void* bd) {
    match_indirect(p, bd);
    return true;
}

bool match_star(char** p, void* bd) {
    while (match_indirect(p, bd))
        ;
    return true;
}

bool match_plus(char** p, void* bd) {
    return match_indirect(p, bd) && match_star(p, bd);
}



// Match one of two options

typedef struct {
    void* a;
    void* b;
} pair;

bool match_either(char** p, void* pr) {
    return match_indirect(p, &((pair*) pr)->a) || match_indirect(p, &((pair*) pr)->b);
}



// Match numbers

bool match_signed_integer(char** p, void* i64) {
    char* str_end;
    int64_t x = strtoll(*p, &str_end, 0);
    if (i64)
        *((int64_t*) i64) = x;
    return (str_end != *p) && (*p = str_end);
}

bool match_unsigned_integer(char** p, void* u64) {
    char* str_end;
    uint64_t x = strtoull(*p, &str_end, 0);
    if (u64)
        *((uint64_t*) u64) = x;
    return (str_end != *p) && (*p = str_end);
}

bool match_double(char** p, void* f64) {
    char* str_end;
    double x = strtod(*p, &str_end);
    if (f64)
        *((double*) f64) = x;
    return (str_end != *p) && (*p = str_end);
}




void Record_ctor(Record* self) {
    assert(self);
    self->type = 0;
    self->width = 0;
    li_number_ctor(&self->literal);
}

void Record_dtor(Record* self) {
    assert(self);
    li_number_dtor(&self->literal);
}



bool match_record(char** p, void* rc) {
    assert(p && *p && rc);
    char* q = *p;
    Record* r = rc;
    r->type = *q;
    if (!(match_from(&q, "subpf") && match_unsigned_integer(&q, &r->width)))
        return false;
    if (match_char(&q, ",")) {
        r->literal.width = r->width;
        switch (r->type) {
            case 's':
                r->literal.type = 's';
                match_signed_integer(&q, &r->literal.i64);
                break;
            case 'u':
            case 'b':
            case 'p':
                r->literal.type = 'u';
                match_unsigned_integer(&q, &r->literal.u64);
                break;
            case 'f':
                r->literal.type = 'f';
                assert(r->width == 64);
                match_double(&q, &r->literal.f64);
                break;
            default:
                assert(false);
                break;
        }
    }
    *p = q;
    return true;
}

li_array(Record) li_parse_Record_list(char* rec) {
    li_array(Record) result;
    li_array_ctor(Record)(&result);
    if (match_char(&rec, "<")) {
        Record r;
        Record_ctor(&r);
        while (match_record(&rec, &r)) {
            li_array_push(Record)(&result, r);
            Record_ctor(&r);
            match_char(&rec, ":");
        }
        match_whitespace(&rec, 0);
        if (*rec)
            li_array_clear(Record)(&result);
        Record_dtor(&r);
    }
    return result;
}



void Operation_ctor(Operation* self) {
    self->op = 0;
    self->value = 0;
}

void Operation_dtor(Operation* self) {
}



bool match_Operation(char** p, void* op /* Operation */) {
    assert(p && *p && op);
    char* q = *p;
    ((Operation*) op)->op = **p;
    return match_from(&q, "*/+-&s^fc") && (match_char(&q, "C") || match_double(&q, &((Operation*) op)->value)) && (*p = q);
}

bool match_Operation_list(char** p, void* pr /* { double*, li_array*} */ ) {
    Operation op;
    Operation_ctor(&op);
    op.value = *((double*) ((pair*) pr)->a);
    while (match_Operation(p, &op)) {
        li_array_push(Operation)(((pair*) pr)->b, op);
        Operation_ctor(&op);
        op.value = *((double*) ((pair*) pr)->a);
    }
    Operation_dtor(&op);
    return true;
}

li_array(li_array_Operation) li_parse_Operation_list_list(char* proc, double cal) {
    li_array(li_array_Operation) result;
    li_array_ctor(li_array_Operation)(&result);
    li_array(Operation) inner;
    li_array_ctor(Operation)(&inner);
    pair pr = { &cal, &inner };
    do {
        match_whitespace(&proc, 0);
        match_Operation_list(&proc, &pr);
        li_array_push(li_array_Operation)(&result, inner);
        li_array_ctor(Operation)(&inner);
        match_whitespace(&proc, 0);
    } while (match_char(&proc, ":"));
    li_array_dtor(Operation)(&inner);
    match_whitespace(&proc, 0);
    if (*proc)
        li_array_clear(li_array_Operation)(&result);
    return result;
}







void Replacement_ctor(Replacement* self) {
    memset(self, 0, sizeof(Replacement));
}

void Replacement_dtor(Replacement* self) {
    li_string_dtor(&self->format);
    li_string_dtor(&self->identifier);
}



// Predicates for the first and non-first characters of an identifier
int isalpha_(char ch) {
    return isalpha(ch) || (ch == '_');
}

int isalnum_(char ch) {
    return isalnum(ch) || (ch == '_');
}

bool match_identifier(char** p, li_string* str) {
    assert(p && *p && str && !*str);
    bind cr = { match_is, isalnum_ };
    char* q = *p;
    return match_is(p, isalpha_) && match_star(p, &cr) && (*str = li_string_from_range(q, *p));
}

// Bracketed digits, such as "[0]"
bool match_subscript(char** p, void* i64) {
    char* q = *p;
    return match_char(&q, "[") && match_signed_integer(&q, i64) && match_char(&q, "]") && (*p = q);
}

// Matches anything until a closing brace; we don't attempt to validate the
// complicated format specifier
bool match_format_string(char** p, li_string* str) {
    assert(p && *p && str && !*str);
    char* q = *p;
    while (**p && (**p != '}'))
        ++*p;
    *str = li_string_from_range(q, *p);
    return true;
}

// Matches a replacement string such as "{ch1[0]:09.8f}"
bool match_replacement(char** p, Replacement* repl) {
    char* q = *p;
    return match_char(&q, "{") && match_identifier(&q, &repl->identifier)
    && (match_subscript(&q, &repl->index), match_char(&q, ":")
        && match_format_string(&q, &repl->format), match_char(&q, "}"))
    && (*p = q);
}

li_array(Replacement) li_parse_Replacement_list(char* fmt) {
    // Examples:
    // {t},{ch1:.8e},{ch2:.8e}
    // "{t},{ch1[0]:09.8f},{ch1[1]:09.8f},{ch1[2]:2.8f}{ch1[3]:2.8f}\r\n"
    
    li_array(Replacement) result;
    li_array_ctor(Replacement)(&result);
    
    Replacement r;
    Replacement_ctor(&r);
    
    while (match_replacement(&fmt, &r)) {
        li_array_push(Replacement)(&result, r);
        Replacement_ctor(&r);
        match_whitespace(&fmt, 0);
        match_char(&fmt, ",");
        match_whitespace(&fmt, 0);
    }
    Replacement_dtor(&r);
    match_whitespace(&fmt, 0);
    
    if (*fmt)
        li_array_clear(Replacement)(&result);
    return result;    
}


li_array(li_string) li_string_split(li_string* self, const li_utf8* delimiters) {
    li_array(li_string) strs;
    li_array_ctor(li_string)(&strs);
    pair a = { match_from, (li_utf8*) delimiters };
    pair b = { match_not_from, (li_utf8*) delimiters };
    char* p1 = *self;
    char* p2 = 0;
    while (*p1) {
        match_star(&p1, &a);
        p2 = p1;
        match_star(&p2, &b);
        if (p2 != p1)
            li_array_push(li_string)(&strs, li_string_from_range(p1, p2));
        p1 = p2;
    }
    return strs;
}

void li_string_lstrip(li_string* self, const li_utf8* chars) {
    pair a = { match_from, (li_utf8*) chars };
    char* p = *self;
    match_star(&p, &a);
    // Move the contents of the string forward, including terminator
    memmove(*self, p, strlen(p) + 1);
}

bool isfrom(char ch, const li_utf8* str) {
    while (str && *str)
        if (*str++ == ch)
            return true;
    return false;
}

void li_string_rstrip(li_string* self, const li_utf8* chars) {
    char* p = *self;
    // Find the end
    while (*p)
        ++p;
    // Back up until not in chars (or front)
    while ((--p >= *self) && isfrom(*p, chars))
        ;
    // Terminate
    *++p = 0;
}






