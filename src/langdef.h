#ifndef BINSCRIPTER_LANGDEF
#define BINSCRIPTER_LANGDEF

#include <stdbool.h>
#include <stddef.h>
#include "bitbuffer.h"

typedef enum arg_type {
    RAW_STRING, // non null-terminated string
    STRING, // null-terminated string
    INT, // integer
    UNSIGNED_INT,
    FLOAT, // IEEE float  
    SKIP,
    __ARG_TYPE_CT
} arg_type;

extern const char * typenames[];

typedef enum endianness {
    BIG_ENDIAN,
    LITTLE_ENDIAN
} endianness;

typedef struct argument_def {
    arg_type type;
    unsigned int bitwidth;
    char * name;
} argument_def;

typedef struct function_def {
    // value of the function call in the output binary
    unsigned int function_binary_value;
    
    // name of the function in the string repr
    char * name;
    
    // list of argument definitions;
    unsigned int argc;
    argument_def ** arguments; 
} function_def;


typedef struct function_call {
    function_def * defn;
    void ** args;
} function_call;

typedef struct language_def {
    enum endianness target_endianness;
    unsigned int function_name_width; 
    unsigned int function_name_bitshift; // bitshift applied to fn names
    unsigned int function_ct;
    unsigned int function_capacity;
    function_def ** functions;
} language_def;


bool validate_size(arg_type type, size_t bits);
bool check_size(arg_type type, unsigned int space,
        unsigned int requested_size);

extern const char * typenames[];
char * type_name(arg_type t);
void print_lang(language_def * l);
void print_fn(language_def *l, function_def *f);
void print_fn_call(function_call * call);
void stringify_fn_call(char * out, function_call * call);

function_def * lang_getfn(language_def * l, unsigned int binary_value);
function_call * func_getcall(function_def * d, void * call);

void * arg_init(language_def * l, argument_def * def, bitbuffer * buffer);
void arg_write(bitbuffer * out_buffer, language_def * l, argument_def * def, void * arg);


void lang_init(language_def * lang);

size_t func_call_width(language_def * l, function_def * def);


void free_lang(language_def *l);
void free_fn(function_def * fn);
void free_call(function_call * call);
void free_arg(argument_def * argdef);


#endif
