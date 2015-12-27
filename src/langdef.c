#include <stdbool.h>
#include <stdlib.h>
#include <stdint.h>
#include <math.h>
#include <string.h>
#include <limits.h>

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
    printf("endian=%d, width=%d, bitshift=%d\n",
                        (int) l->target_endianness,
                        l->function_name_width,
                        l->function_name_bitshift);
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

void * arg_init(language_def * l, argument_def * argdef, 
        bitbuffer * buffer) {
    size_t buffer_len;
    int sign = 1;

    float f;
    double d;
    long double * ld;

    printf("initting arg type \"%s\" %d\n", 
            typenames[argdef->type],
            argdef->bitwidth);

    printf("buffer contents: ");
    bitbuffer_print(buffer);

    switch(argdef->type) {
        case RAW_STRING:
        case STRING:
            if(argdef->type == RAW_STRING)
                buffer_len = bits2bytes(argdef->bitwidth) + 1;
            else 
                buffer_len = bits2bytes(argdef->bitwidth);
            
            char * strbuffer = malloc(buffer_len);
            bitbuffer_pop(strbuffer, buffer, buffer_len - 1);
            return strbuffer;

        case INT:
        case UNSIGNED_INT:
            buffer_len = argdef->bitwidth;

            // put the data at the front of the int
            long int * int_internal = 
                (long int *) malloc(sizeof(long int));
            bitbuffer_pop(int_internal, buffer, buffer_len);
            // move to the least significant bits of the long
            if (IS_BIG_ENDIAN) {
                *int_internal = *int_internal >> 
                    (sizeof(long int) * 8 - buffer_len); 
            }

            //swap the endianness to match host endianness
            if((IS_BIG_ENDIAN) != (l->target_endianness == BIG_ENDIAN)) {
                printf("integer switch endianness (%ld)\n", *int_internal);
                swap_endian_on_field(int_internal, bits2bytes(buffer_len));
            }

            // apply signdedness
            if (INT == argdef->type) {
                sign = *int_internal >> (buffer_len - 1);
                printf("sign: %d\n", sign);
                if (sign) {
                    *int_internal = *int_internal & (~(1 << (buffer_len - 1)));
                    *int_internal = - *int_internal;
                }
            }

            return int_internal;

        case FLOAT:
            buffer_len = argdef->bitwidth;
            ld = (long double *) malloc(sizeof(long double));
            switch(buffer_len) {
                case sizeof(float) * 8:
                    printf("f ");
                    bitbuffer_pop(&f, buffer, buffer_len);
                    *ld = f;
                    printf("%f\n", f);
                    break;
                case sizeof(double) * 8:
                    printf("d ");
                    bitbuffer_pop(&d, buffer, buffer_len);
                    printf("%f\n", d);
                    *ld = d;
                    break;
                case sizeof(long double) * 8:
                    printf("ld\n");
                    bitbuffer_pop(ld, buffer, buffer_len);
                    break;
                default:
                    printf("no known decoding for float of length %d\n",
                            (int) buffer_len);
                    exit(1);
            }
            // TODO endiannesss for floats?
            
            return ld;

        case SKIP:
            bitbuffer_advance(buffer, argdef->bitwidth);
            return NULL;

        default:
            printf("error trying to initialize unknown argtpye"
                   "%d", argdef->type);
            exit(1);
    }
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

