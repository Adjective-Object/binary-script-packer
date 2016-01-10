#include <string.h>
#include <stdlib.h>

#include "../mutest.h"
#include "parsescript.h"
#include "sweetexpressions.h"

void mu_test_parse_int() {
    // check that it can handle the 3 different parse
    // modes without error
    int i = -1;
    mu_check(NO_ERROR == parse_int(&i, "0"));
    mu_check(i == 0);
    mu_check(NO_ERROR == parse_int(&i, "0x1"));
    mu_check(i == 1);
    mu_check(NO_ERROR == parse_int(&i, "0b10"));
    mu_check(i == 2);

    // check that it properly parses a small string in each
    // of the parse modes
    mu_check(NO_ERROR == parse_int(&i, "10"));
    mu_check(i == 10);
    mu_check(NO_ERROR == parse_int(&i, "0xB"));
    mu_check(i == 11);
    mu_check(NO_ERROR == parse_int(&i, "0xc"));
    mu_check(i == 12);
    mu_check(NO_ERROR == parse_int(&i, "0b1101"));
    mu_check(i == 13);

   // the parse modes when dealing with negatives
    mu_check(NO_ERROR == parse_int(&i, "-10"));
    mu_check(i == -10);
    mu_check(NO_ERROR == parse_int(&i, "-0xB"));
    mu_check(i == -11);
    mu_check(NO_ERROR == parse_int(&i, "-0xc"));
    mu_check(i == -12);
    mu_check(NO_ERROR == parse_int(&i, "-0b1101"));
    mu_check(i == -13);

    // check for errors when dealing with unrecognized characters
    // in each parser mode, and that encountering an error does not
    // mutate the output slot
    
    mu_check(BAD_DECIMAL_FORMAT == parse_int(&i, "1aa"));
    mu_check(BAD_HEX_FORMAT == parse_int(&i, "0xiaa"));
    mu_check(BAD_BINARY_FORMAT== parse_int(&i, "0b333"));
}

#define mu_check_unchanged() \
    mu_check(0 == memcmp(\
                &backup_argument, \
                &argument, \
                sizeof(argument_def)));

void mu_test_parse_argtype() {
    argument_def argument;
    argument.name = NULL;

    // check that the number parsing works in any mode
    mu_check(NO_ERROR == parse_argtype(&argument, "int32"));
    mu_check(argument.type == INT);
    mu_check(argument.bitwidth == 32);

    mu_check(NO_ERROR == parse_argtype(&argument, "int0x21"));
    mu_check(argument.type == INT);
    mu_check(argument.bitwidth == 33);

    int p = parse_argtype(&argument, "int0b100010");
    mu_check(NO_ERROR == p);
    mu_check(argument.type == INT);
    mu_check(argument.bitwidth == 34);

    // check that each of the base types can be parsed
    mu_check(NO_ERROR == parse_argtype(&argument, "raw_str32"));
    mu_check(argument.type == RAW_STRING);
    
    mu_check(NO_ERROR == parse_argtype(&argument, "str32"));
    mu_check(argument.type == STRING);

    mu_check(NO_ERROR == parse_argtype(&argument, "int32"));
    mu_check(argument.type == INT);

    mu_check(NO_ERROR == parse_argtype(&argument, "uint32"));
    mu_check(argument.type == UNSIGNED_INT);

    mu_check(NO_ERROR == parse_argtype(&argument, "float32"));
    mu_check(argument.type == FLOAT);

    mu_check(NO_ERROR == parse_argtype(&argument, "skip32"));
    mu_check(argument.type == SKIP);

    // Check that constraints on each type's sizes are enforced
    //
    // floats must be of size 64, 32, or 16
    // string must be multiples of size 8 (one char)
    // int, uint, and skip can all be of any size
    //
    // after each failed one, we also check that the argument type
    // is not mutated by the failed parsing
    
    argument_def backup_argument;
    memcpy(&backup_argument, &argument, sizeof(argument_def));

    mu_check(DISALLOWED_SIZE == parse_argtype(&argument, "float11"));
    mu_check_unchanged();
    mu_check(DISALLOWED_SIZE == parse_argtype(&argument, "float30"));
    mu_check_unchanged();
    mu_check(DISALLOWED_SIZE == parse_argtype(&argument, "str3"));
    mu_check_unchanged();
    mu_check(DISALLOWED_SIZE == parse_argtype(&argument, "raw_str3"));
    mu_check_unchanged();
}

void mu_test_parse_function() {
    // TODO compare interpreted functions to their expected outputs
    swexp_list_node * demofn_test = parse_string_to_atoms(
        "(def 0x10 demofn skip6 (int4 gfx))", 255);
    free_list(demofn_test);
}



