#ifndef BINARY_SCRIPTER
#define BINARY_SCRIPTER

#include "sweetexpressions.h"
#include "endian.h"

typedef enum arg_type {
    RAW_STRING, // non null-terminated string
    STRING, // null-terminated string
    INT, // integer
    FLOAT, // IEEE float  
    SKIP,
    __ARG_TYPE_CT,
} arg_type;

typedef enum endianness {
    BIG_ENDIAN,
    LITTLE_ENDIAN,
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

typedef struct language_def {
    enum endianness target_endianness;
    unsigned int function_name_width; 
    unsigned int function_ct;
    unsigned int function_capacity;
    function_def ** functions;
} language_def;

language_def * define_language(FILE * f);
char * type_name(arg_type t);
void print_lang(language_def * l);

#endif
