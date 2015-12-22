#include <stdbool.h>
#include <stdlib.h>
#include <stdint.h>
#include <math.h>
#include <string.h>

#include "parsescript.h"
#include "langdef.h"
#include "util.h"
#include "bitbuffer.h"

const char * typenames[] = {
    "raw_str",
    "str",
    "int",
    "uint",
    "float",
    "ufloat",
    "skip"
};

bool check_size(arg_type type, unsigned int space,
        unsigned int value) {
    switch(type) {
        case UNSIGNED_INT:
            return log2((double) value) <= space;
        default:
            printf("check_size called on unhandled type %d", (int) type);
            exit(1);
    }
}

size_t func_call_width(language_def * l, function_def * def) {
    size_t bitwidth = 0;
    for (unsigned int i=0; i<def->argc; i++) {
        bitwidth += def->arguments[i]->bitwidth;
    }
    return bitwidth + l->function_name_width;
}

char * type_name(arg_type t) {
    switch(t) {
        case RAW_STRING:    return "rawstr";
        case STRING:        return "str";
        case INT:           return "int";
        case FLOAT:         return "float";
        case SKIP:          return "skip";
        default:            return "??";
    }
}

void print_lang(language_def * l) {
    for (unsigned int i=0; i < l->function_ct; i++) {
        print_fn(l, l->functions[i]);
    }
}

void print_fn(language_def * l, function_def *f) {
    printf("0x%x %s: ", 
            f->function_binary_value << l->function_name_bitshift,
            f->name);
    for (unsigned int j=0; j < f->argc; j++) {
        argument_def * a = f->arguments[j];
        printf("<%s:%d %s> ",
                type_name(a->type),
                a->bitwidth,
                a->name);
    }
    printf("\n"); 
}

void print_fn_call(function_call * call) {
    printf("%s(", call->defn->name);
    for (unsigned int i=0; i < call->defn->argc; i++) {
        argument_def ** argdefs = call->defn->arguments;
        switch(argdefs[i]->type) {
            case RAW_STRING:
                printf("%*s", 
                        argdefs[i]->bitwidth / 8,
                        (char *) call->args[i]);
                break;
            case STRING:
                printf("%s", (char *) call->args[i]);
                break;
            case INT:
                printf("%d", *((int *) call->args[i]));
                break;
            case FLOAT:
                printf("%f", *((float *) call->args[i]));
                break;
            case SKIP:
                break;
            default:
                break;
        }
        if (argdefs[i]->type != SKIP && i + 1 < call->defn->argc) {
            printf(", ");
        }
    }
    printf(")\n");


}

function_def * lang_getfn(language_def * l, unsigned int binary_value) {
    unsigned int i;
    for (i=0; i<l->function_ct; i++) {
        if (l->functions[i]->function_binary_value == binary_value) {
            return l->functions[i];
        }
    }
    return NULL;
}

function_call * lang_callfn(language_def * l, char * databuffer) {
    unsigned int fn_name = funcname_from_buffer(l, databuffer);
    function_def * fn = lang_getfn(l, fn_name);

    printf("fn_name: %d\n", fn_name);

    // make a bitbuffer wrapper for the data buffer
    bitbuffer b;
    b.buffer = databuffer;
    b.buffer_origin = databuffer;
    b.buflen_max = bits2bytes(func_call_width(l, fn));
    b.remaining_bytes = b.buflen_max;
    b.head_offset = l->function_name_width;

    // create the function call object
    function_call * call = (function_call *) malloc(sizeof(function_call));
    call->defn = fn;

    // allocate an array to hold argument contents
    call->args = (void **) malloc(sizeof(char *) * fn->argc);
    char * current_arg_buffer = NULL;
    size_t bit_in_arg = 0;
    uint8_t buffer_ind = 0;

    // process bit by bit
    unsigned int current_arg_index = 0;
    while(current_arg_index < fn->argc) {
        printf("ARGUMENT (%d/%d)\n", current_arg_index + 1, fn->argc);
        printf("buffer content: ");
        bitbuffer_print(&b);
        size_t current_arg_bytesize = 
            bits2bytes(fn->arguments[current_arg_index]->bitwidth);
        
        // if there is no buffer for the current argument, make one
        if (current_arg_buffer == NULL) {
            printf("NEW ARG\n");
            current_arg_buffer = 
                (char *) malloc(sizeof(char) * current_arg_bytesize);
            memset(current_arg_buffer, 0, current_arg_bytesize);
            bit_in_arg = 0;
        }

        // get the first bit into the current arg's buffer
        shiftBuffer(current_arg_buffer, current_arg_bytesize, -1);
        current_arg_buffer[current_arg_bytesize - 1] = 
            current_arg_buffer[current_arg_bytesize - 1] | bitbuffer_next(&b);
        
        printf("current arg=%d \n", *current_arg_buffer);

        bit_in_arg++;
        buffer_ind++;
        if (buffer_ind >= 8) {
            buffer_ind = buffer_ind - 8;
            databuffer++;
        }

        // if we have reached the end of the current argument, then put it in
        // the function call at the current index & advance
        if (bit_in_arg >= fn->arguments[current_arg_index]->bitwidth) {
            call->args[current_arg_index] = current_arg_buffer;
            current_arg_buffer = NULL;
            current_arg_index++;
        }
        
    }

    
    for (unsigned int i=0; i<fn->argc; i++) {
        size_t argsize = sizeof(char) * bits2bytes(fn->arguments[i]->bitwidth);
        void * arg_content = malloc(argsize);
        memcpy(arg_content, databuffer, argsize);
        call->args[i] = arg_content;
    }
    
    return call;
}


unsigned int funcname_from_buffer(language_def * lang, char * fname_buffer) {
    
    // function sizes
    size_t fname_size_bits = lang->function_name_width,
        fname_size_bytes = bits2bytes(fname_size_bits); 

    // create the number from the buffer
    unsigned int fn_name = 0;
    for(unsigned int byte = 0; byte < fname_size_bytes; byte++) {
        unsigned int remainderbits = fname_size_bits - byte * 8;
        unsigned int firstbit;
        if (remainderbits >= 8 || remainderbits == 0) {
            firstbit = 0;
        } else {
            firstbit = 8 - remainderbits;
        }

        for (unsigned int bitoff = firstbit; bitoff < 8; bitoff++) {
            fn_name = (fn_name << 1) | ((fname_buffer[byte] >> (7 - bitoff)) & 1);
        }
    }

    return fn_name >> lang->function_name_bitshift;
}

