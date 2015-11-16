#include <stdbool.h>
#include <stdlib.h>
#include <math.h>

#include "parsescript.h"
#include "langdef.h"

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

function_def * lang_getfn(language_def * l, unsigned int binary_value) {
    unsigned int i;
    for (i=0; i<l->function_ct; i++) {
        if (l->functions[i]->function_binary_value == binary_value) {
            return l->functions[i];
        }
    }
    return NULL;
}
