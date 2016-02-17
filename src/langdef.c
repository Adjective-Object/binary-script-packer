#include <stdbool.h>
#include <stdlib.h>
#include <stdint.h>
#include <math.h>
#include <string.h>
#include <limits.h>

#include "parsescript.h"
#include "langdef.h"
#include "translator.h"
#include "util.h"
#include "bitbuffer.h"

const char *typenames[] = {[RAW_STRING] = "raw_str",
                           [STRING] = "str",
                           [INT] = "int",
                           [UNSIGNED_INT] = "uint",
                           [FLOAT] = "float",
                           [SKIP] = "skip" };

bool check_size(arg_type type, unsigned int space, unsigned int value) {
    // check that the value provided can fit into the
    // space of the argument type defined
    switch (type) {
    case UNSIGNED_INT:
        return log2((double)value) <= space;
    default:
        printf("check_size called on unhandled type %d", (int)type);
        exit(1);
    }
}

bool validate_size(arg_type type, size_t bits) {
    // check that the size provided can be
    // used for the type of the argument
    switch (type) {
    // strings must be represented as multiples of char
    case RAW_STRING:
    case STRING:
        return bits % 8 == 0;

    // floats must be represented as IEE754 floats
    // either long double, double, or float
    case FLOAT:
        return (bits == 16 || bits == 32 || bits == 64);

    // ints and skips can be any width
    case INT:
    case SKIP:
    case UNSIGNED_INT:
        return 1;

    default:
        return false;
    }
}

size_t func_call_width(language_def *l, function_def *def) {
    size_t bitwidth = 0;
    for (unsigned int i = 0; i < def->argc; i++) {
        bitwidth += def->arguments[i]->bitwidth;
    }
    return bitwidth + l->function_name_width;
}

char *type_name(arg_type t) {
    switch (t) {
    case RAW_STRING:
        return "rawstr";
    case STRING:
        return "str";
    case INT:
        return "int";
    case UNSIGNED_INT:
        return "uint";
    case FLOAT:
        return "float";
    case SKIP:
        return "skip";
    default:
        return "??";
    }
}

void print_lang(language_def *l) {
    printf("endian=%d, width=%u, bitshift=%u\n", (int)l->target_endianness,
           l->function_name_width, l->function_name_bitshift);
    for (unsigned int i = 0; i < l->function_ct; i++) {
        print_fn(l, l->functions[i]);
    }
}

void print_fn(language_def *l, function_def *f) {
    printf("0x%x %s: ", f->function_binary_value << l->function_name_bitshift,
           f->name);
    for (unsigned int j = 0; j < f->argc; j++) {
        argument_def *a = f->arguments[j];
        printf("<%s:%u %s> ", type_name(a->type), a->bitwidth, a->name);
    }
    printf("\n");
}

void print_fn_call(function_call * call) {
    char out[1024];
    string_encode_function_call(out,call);
    printf("%s\n", out);
}

void *arg_init(language_def *l, argument_def *argdef, bitbuffer *buffer) {
    size_t buffer_len;
    int sign = 1;

    float f;
    double d;
    long double *ld;

    switch (argdef->type) {
    case RAW_STRING:
    case STRING:
        if (argdef->type == RAW_STRING)
            buffer_len = bits2bytes(argdef->bitwidth) + 1;
        else
            buffer_len = bits2bytes(argdef->bitwidth);

        char *strbuffer = malloc(buffer_len);
        bitbuffer_pop(strbuffer, buffer, buffer_len - 1);
        return strbuffer;

    case INT:
    case UNSIGNED_INT:
        buffer_len = argdef->bitwidth;

        // put the data at the front of the int
        long int *int_internal = (long int *)malloc(sizeof(long int));
        memset(int_internal, 0, sizeof(long int));
        bitbuffer_pop(int_internal, buffer, buffer_len);
        // move to the least significant bits of the long
        if (IS_BIG_ENDIAN) {
            *int_internal =
                *int_internal >> (sizeof(long int) * 8 - buffer_len);
        }

        // swap the endianness to match host endianness
        if ((IS_BIG_ENDIAN) != (l->target_endianness == BIG_ENDIAN)) {
            swap_endian_on_field(int_internal, bits2bytes(buffer_len));
        }

        // apply signdedness
        if (INT == argdef->type) {
            sign = *int_internal >> (buffer_len - 1);
            if (sign) {
                *int_internal = *int_internal & (~(1 << (buffer_len - 1)));
                *int_internal = -*int_internal;
            }
        }

        return int_internal;

    case FLOAT:
        buffer_len = argdef->bitwidth;
        ld = (long double *)malloc(sizeof(long double));
        switch (buffer_len) {
        case sizeof(float) * 8:
            bitbuffer_pop(&f, buffer, buffer_len);
            if ((IS_BIG_ENDIAN) != (l->target_endianness == BIG_ENDIAN)) {
                swap_endian_on_field(&f, sizeof(float));
            }
            *ld = f;
            break;
        case sizeof(double) * 8:
            bitbuffer_pop(&d, buffer, buffer_len);
            if ((IS_BIG_ENDIAN) != (l->target_endianness == BIG_ENDIAN)) {
                swap_endian_on_field(&d, sizeof(double));
            }
            *ld = d;
            break;
        case sizeof(long double) * 8:
            bitbuffer_pop(ld, buffer, buffer_len);
            if ((IS_BIG_ENDIAN) != (l->target_endianness == BIG_ENDIAN)) {
                swap_endian_on_field(ld, sizeof(long double));
            }
            break;
        default:
            printf("no known decoding for float of length %d\n",
                   (int)buffer_len);
            exit(1);
        }

        return ld;

    case SKIP:
        bitbuffer_advance(buffer, argdef->bitwidth);
        return NULL;

    default:
        printf("error trying to initialize unknown argtpye "
               "%d",
               argdef->type);
        exit(1);
    }
}

void arg_write(bitbuffer *out_buffer, language_def *l, argument_def *argdef,
               void *argval) {
    float f;
    double d;
    long double ld;

    long int *argval_longint = (long int *)argval;
    long double *argval_longdouble = (long double *)argval;
    switch (argdef->type) {
    case INT:
        bitbuffer_writebit(out_buffer, *argval_longint < 0);
        for (int i = argdef->bitwidth - 2; i >= 0; i--) {
            bitbuffer_writebit(out_buffer, (*argval_longint >> i) & 1);
        }
        return;
    case UNSIGNED_INT:
        for (int i = argdef->bitwidth - 1; i >= 0; i--) {
            bitbuffer_writebit(out_buffer, (*argval_longint >> i) & 1);
        }
        return;
    case FLOAT:
        // printf("out buffer: %d %d -> ", 
        //         out_buffer->buffer - out_buffer->buffer_origin,
        //         out_buffer->head_offset);
        switch (argdef->bitwidth) {
        case sizeof(long double) * 8:
            ld = * argval_longdouble;
            if ((IS_BIG_ENDIAN) != (l->target_endianness == BIG_ENDIAN))
                swap_endian_on_field(&ld, sizeof(long double));
            bitbuffer_writeblock(out_buffer, &ld, 8 * sizeof(long double));
            return;
        case sizeof(double) * 8:
            d = *argval_longdouble;
            if ((IS_BIG_ENDIAN) != (l->target_endianness == BIG_ENDIAN))
                swap_endian_on_field(&d, sizeof(double));
            bitbuffer_writeblock(out_buffer, &d, 8  * sizeof(double));
            return;
        case sizeof(float) * 8:
            f = *argval_longdouble;
            if ((IS_BIG_ENDIAN) != (l->target_endianness == BIG_ENDIAN)) {
                swap_endian_on_field(&f, sizeof(float));
            }
            bitbuffer_writeblock(out_buffer, &f, 8 * sizeof(float));
            return;
        default:
            printf("tried to switch on unhandled float bitwidth %u",
                   argdef->bitwidth);
            exit(1);
        }
        return;
    case SKIP:
        for (size_t i = 0; i < argdef->bitwidth; i++) {
            bitbuffer_writebit(out_buffer, 0);
        }
        return;
    default:
        printf("error trying to write unknown argtype "
               "%s",
               typenames[argdef->type]);

        exit(1);
    }
}

void lang_init(language_def *lang) {
    lang->target_endianness = LITTLE_ENDIAN;
    lang->function_name_width = 8;
    lang->function_name_bitshift = 0;
    lang->function_ct = 0;
    lang->function_capacity = 0;
    lang->functions = NULL;
}

function_def *lang_getfn(language_def *l, unsigned int binary_value) {
    unsigned int i;
    for (i = 0; i < l->function_ct; i++) {
        // printf("%d 0x%x ", i, l->functions[i]->function_binary_value);
        // printf("%s \n", l->functions[i]->name);
        if (l->functions[i]->function_binary_value == binary_value) {
            return l->functions[i];
        }
    }
    return NULL;
}

function_def *lang_getfnbyname(language_def *l, char * name) {
    unsigned int i;
    for (i = 0; i < l->function_ct; i++) {
        if (0 == strcmp(name, l->functions[i]->name)) {
            return l->functions[i];
        }    }
    return NULL;
}

void _free_lang(language_def *l, bool controlled) {
    if (controlled) {
        for (size_t i = 0; i < l->function_ct; i++) {
            free_fn(l->functions[i]);
            free(l->functions[i]);
        }
    }
    free(l->functions);
}

void free_lang(language_def *l) {
    _free_lang(l, true);
}



void free_call(function_call *call) {
    for (size_t i = 0; i < call->defn->argc; i++) {
        free(call->args[i]);
    }
    free(call->args);
    free(call);
}

void free_fn(function_def *fn) {
    for (size_t i = 0; i < fn->argc; i++) {
        free_arg(fn->arguments[i]);
    }

    if (fn->name != NULL)
        free(fn->name);

    if (fn->arguments != NULL)
        free(fn->arguments);
}

void free_arg(argument_def *argdef) {
    free(argdef->name);
    free(argdef);
}
