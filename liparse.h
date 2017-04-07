//
//  liparse.h
//  liquidreader
//
//  Created by Antony Searle on 3/2/17.
//  Copyright © 2017 Antony Searle. All rights reserved.
//

#ifndef liparse_h
#define liparse_h

#include "linumber.h"
#include "liutility.h"

#ifdef __cplusplus
extern "C" {
#endif
    
    // Parse a record string into Records
    // Example: "<s32"
    
    typedef struct {
        char type;
        size_t width;
        li_number literal;
    } Record;
    
    void Record_ctor(Record* self);
    void Record_dtor(Record* self);
    
    li_array_define(Record);
    
    li_array(Record) li_parse_Record_list(char* rec);

    
    
    
    // Parse a proc string into arrays of Operations
    
    typedef struct {
        char op;
        double value;
    } Operation;
    
    void Operation_ctor(Operation* self);
    void Operation_dtor(Operation* self);
    
    li_array_define(Operation);
    li_array_define(li_array_Operation)
    
    li_array(li_array_Operation) li_parse_Operation_list_list(char* proc, double cal);

    
    
    // Parse a format string into Replacements
    
    typedef struct {
        li_string identifier;
        size_t index;
        li_string format;
    } Replacement;
    
    void Replacement_ctor(Replacement* self);
    void Replacement_dtor(Replacement* self);
    
    li_array_define(Replacement)
    
    li_array(Replacement) li_parse_Replacement_list(char* fmt);
    
    
    li_array(li_string) li_string_split(li_string* self, const li_utf8* delimiters);
    void li_string_lstrip(li_string* self, const li_utf8* chars);
    void li_string_rstrip(li_string* self, const li_utf8* chars);
    
    
#ifdef __cplusplus
} // extern "C"
#endif

#endif /* liparse_h */


