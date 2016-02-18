#include <string.h>
#include <stdlib.h>
#include <stdio.h>

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
    mu_check(BAD_BINARY_FORMAT == parse_int(&i, "0b333"));
}

#define mu_check_unchanged()                                                   \
    mu_check(0 == memcmp(&backup_argument, &argument, sizeof(argument_def)));

void mu_test_parse_argtype() {
    argument_def argument;
    argument.name = NULL;

    // check that the number parsing works in any mode
    mu_eq(PARSE_ERROR, NO_ERROR, parse_argtype(&argument, "int32"));
    mu_eq(arg_type, INT, argument.type);
    mu_eq(int, 32, argument.bitwidth);

    mu_eq(PARSE_ERROR, NO_ERROR, parse_argtype(&argument, "int0x21"));
    mu_eq(arg_type, INT, argument.type);
    mu_eq(int, 33, argument.bitwidth);

    int p = parse_argtype(&argument, "int0b100010");
    mu_eq(PARSE_ERROR, NO_ERROR, p);
    mu_eq(arg_type, INT, argument.type);
    mu_eq(int, 34, argument.bitwidth);

    // check that each of the base types can be parsed
    mu_eq(PARSE_ERROR, NO_ERROR, parse_argtype(&argument, "raw_str32"));
    mu_eq(arg_type, RAW_STRING, argument.type);

    mu_eq(PARSE_ERROR, NO_ERROR, parse_argtype(&argument, "str32"));
    mu_eq(arg_type, STRING, argument.type);

    mu_eq(PARSE_ERROR, NO_ERROR, parse_argtype(&argument, "int32"));
    mu_eq(arg_type, INT, argument.type);

    mu_eq(PARSE_ERROR, NO_ERROR, parse_argtype(&argument, "uint32"));
    mu_eq(arg_type, UNSIGNED_INT, argument.type);

    mu_eq(PARSE_ERROR, NO_ERROR, parse_argtype(&argument, "float32"));
    mu_eq(arg_type, FLOAT, argument.type);

    mu_eq(PARSE_ERROR, NO_ERROR, parse_argtype(&argument, "skip32"));
    mu_eq(arg_type, SKIP, argument.type);

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

    mu_eq(PARSE_ERROR, DISALLOWED_SIZE, parse_argtype(&argument, "float11"));
    mu_check_unchanged();
    mu_eq(PARSE_ERROR, DISALLOWED_SIZE, parse_argtype(&argument, "float30"));
    mu_check_unchanged();
    mu_eq(PARSE_ERROR, DISALLOWED_SIZE, parse_argtype(&argument, "str3"));
    mu_check_unchanged();
    mu_eq(PARSE_ERROR, DISALLOWED_SIZE, parse_argtype(&argument, "raw_str3"));
    mu_check_unchanged();

    // Check for unspecified size
    mu_eq(PARSE_ERROR, UNSPECIFIED_SIZE, parse_argtype(&argument, "int"));
    mu_check_unchanged();
}

bool compare_function_defs(function_def *a, function_def *b) {
    // check the root level props of either function
    if (a->argc != b->argc) {
        printf("argc a=%u b=%u ", a->argc, b->argc);
        return false;
    }

    if (0 != strcmp(a->name, b->name)) {
        printf("names a=%s b=%s ", a->name, b->name);
        return false;
    }

    if (a->function_binary_value != b->function_binary_value) {
        printf("binary vals a=%x b=%x ", a->function_binary_value,
               b->function_binary_value);
        return false;
    }

    // check that each of the arguments are the same
    for (size_t i = 0; i < a->argc; i++) {
        argument_def *arga = a->arguments[i], *argb = b->arguments[i];

        if (((arga->name == NULL) != (argb->name == NULL)) ||
            (arga->name != NULL && argb->name != NULL &&
             0 != strcmp(arga->name, argb->name)) ||
            arga->type != argb->type || arga->bitwidth != argb->bitwidth)
            return false;
    }
    return true;
}

bool test_fndef(function_def *expected, const char *str) {
    // create a reference language for parsing against
    language_def language;
    language.function_name_width = 8;
    language.function_name_bitshift = 2;
    language.function_ct = 0;
    language.function_capacity = 0;
    language.functions = NULL;
    language.byte_aligned_functions = false;

    // parse the sweet expression string to a function def
    swexp_list_node *swexp_list = parse_string_to_atoms(str, "<anon:test_fndef>", 255);

    function_def output;
    detailed_parse_error * p =
        parse_fn(&output, &language, (swexp_list_node *)swexp_list->content);
    mu_check(p == NULL);
    free_err(p);

    // compare the function defs
    bool matches = compare_function_defs(expected, &output);

    if (!matches) {
        printf("expected: ");
        print_fn(&language, expected);
        printf("received ");
        print_fn(&language, &output);
    }

    // free the swexp list and the functoin def
    free_list(swexp_list);
    free_fn(&output);

    return matches;
}

detailed_parse_error * test_parse(function_def *out, const char *str) {
    // create a reference language
    language_def lang;
    lang.function_name_width = 8;
    lang.function_name_bitshift = 2;
    lang.function_ct = 0;
    lang.function_capacity = 0;
    lang.functions = NULL;

    // parse the sweet expression string to a function def
    swexp_list_node *swexp_list = parse_string_to_atoms(str, "<anon:test_parse>", 255);
    detailed_parse_error * e = parse_fn(out, &lang, (swexp_list_node *)swexp_list->content);
    free_list(swexp_list);

    return e;
}

#define fn_err(err, str) \
    do { \
    detailed_parse_error * dd = test_parse(&o, str);\
    if (dd == NULL) \
        mu_eq(PARSE_ERROR, err, NO_ERROR); \
    else \
        mu_eq(PARSE_ERROR, err, dd->primitive_error); \
    mu_check(0 == memcmp(&o, &ref_arr, sizeof(function_def))); \
    free_err(dd);\
    } while (0);

void mu_test_parse_function() {
    ///////////////////////////
    // simple function check //
    ///////////////////////////
    argument_def a_1 = { SKIP, 6, NULL }, a_2 = { INT, 4, "gfx" };
    argument_def *argz[] = { &a_1, &a_2 };
    function_def f = {
        .function_binary_value = (0x10 >> 2),
        .name = "demofn",
        .argc = 2,
        .arguments = argz,
    };

    mu_check(test_fndef(&f, "def 0x10 demofn skip6 int4(gfx)"));

    //////////////////////////////////////
    // function with no arguments check //
    //////////////////////////////////////
    argument_def **argz_empty = NULL;
    function_def f_empty = {
        .function_binary_value = (0x10 >> 2),
        .name = "empty_def",
        .argc = 0,
        .arguments = argz_empty,
    };

    mu_check(test_fndef(&f_empty, "def 0x10 empty_def"));

    /////////////////////////////////
    // check malformed definitions //
    /////////////////////////////////

    function_def o;
    char ref_arr[sizeof(function_def)];
    memset(&ref_arr, 0, sizeof(function_def));
    memset(&o, 0, sizeof(function_def));

    fn_err(MISSING_DEF, "sef 0x10 demofn skip6 int4(gfx)");
    fn_err(MISSING_DEF, "0x10 demofn skip6 int4(gfx)");
    fn_err(MISSING_BINNAME, "def int4(gfx)");
    fn_err(MALFORMED_BINNAME, "def aa demofn int4(demofn)");

    fn_err(FUNCTION_BINNAME_PRECISION, "def 0b01");
    fn_err(FUNCTION_BINNAME_PRECISION, "def 0b10");
    fn_err(FUNCTION_BINNAME_SIZE, "def 4100 skip6");

    fn_err(MISSING_NAME, "def 0x20 int4(demofn)");
}

bool test_language(PARSE_ERROR expected_error, language_def *reference_lang,
                   char *str) {
    language_def parsed_lang;
    memset(&parsed_lang, 0, sizeof(language_def));

    // parse to nodes, convert nodes to language, then free nodes
    swexp_list_node *nodes = parse_string_to_atoms(str, "<anon:test_language>", 255);
    // print_list(nodes);
    detailed_parse_error * parse_error = parse_language(&parsed_lang, nodes);
    free_list(nodes);

    if (parse_error != NULL) {
        if (parse_error->primitive_error != expected_error) {
            printf("mismatch in error expected = %s, parsed = %s\n",
                    error_message_name(expected_error),
                    error_message_name(parse_error->primitive_error));
            print_err(parse_error);
            free_err(parse_error);
            free_lang(&parsed_lang);
            return false;
        } else {
            free_lang(&parsed_lang);
            free_err(parse_error);
            return true;
        }
    } else if (expected_error != NO_ERROR) {
        printf("no error thrown, expected %s\n",
                error_message_name(expected_error));
        free_lang(&parsed_lang);
    }

    if (reference_lang == NULL) {
        if (expected_error == NO_ERROR)
            printf("no error thrown on null language\n");
        else
            printf("no error expected on null language -- error in test cases?\n");

        free_lang(&parsed_lang);
        return false;
    }

    // Check the metadata attributes
    // and number of functions
    if (reference_lang->target_endianness != parsed_lang.target_endianness ||
        reference_lang->function_name_width !=
            parsed_lang.function_name_width ||
        reference_lang->function_name_bitshift !=
            parsed_lang.function_name_bitshift ||
        reference_lang->function_ct != parsed_lang.function_ct) {
        printf("    different metadata\n");

        printf("      endianness %d %d \n", reference_lang->target_endianness,
               parsed_lang.target_endianness);

        printf("      name width %u %u \n", reference_lang->function_name_width,
               parsed_lang.function_name_width);

        printf("      name bitwidth %u %u \n",
               reference_lang->function_name_bitshift,
               parsed_lang.function_name_bitshift);

        printf("      function count %u %u \n", reference_lang->function_ct,
               parsed_lang.function_ct);
        free_lang(&parsed_lang);
        return false;
    }

    // Check the function definitions
    for (int i = 0; i < parsed_lang.function_ct; i++) {
        if (!compare_function_defs(reference_lang->functions[i],
                                   parsed_lang.functions[i])) {
            printf("different function in slot %d\n", i);
            print_fn(reference_lang, reference_lang->functions[i]);
            print_fn(&parsed_lang, parsed_lang.functions[i]);
            free_lang(&parsed_lang);
            return false;
        }
    }

    free_lang(&parsed_lang);
    return true;
}

void mu_test_parse_language_metadata() {
    language_def lang;
    lang_init(&lang);

    ///////////////////////////////////////
    // Empty Language. no attributes set //
    ///////////////////////////////////////
    mu_check(test_language(NO_ERROR, &lang, ""));

    ///////////////////////////////////////////
    // Check the default metadata parameters //
    ///////////////////////////////////////////
    mu_check(test_language(NO_ERROR, &lang, "meta \n"
                                            "    endianness little \n"
                                            "    namewidth 8 \n"
                                            "    nameshift 0 \n"));

    mu_check(test_language(NO_ERROR, &lang, "meta \n"
                                            "    endianness LITTLE \n"
                                            "    namewidth 8 \n"
                                            "    nameshift 0 \n"));

    mu_check(test_language(NO_ERROR, &lang, "meta \n"
                                            "    endianness Little \n"
                                            "    namewidth 8 \n"
                                            "    nameshift 0 \n"));

    //////////////////////////////////////////////////////
    // Check that metadata attritbutes are set properly //
    //////////////////////////////////////////////////////

    lang.target_endianness = BS_BIG_ENDIAN;
    lang.function_name_width = 9;
    lang.function_name_bitshift = 1;

    mu_check(test_language(NO_ERROR, &lang, "meta \n"
                                            "    endianness big \n"
                                            "    namewidth 9 \n"
                                            "    nameshift 1 \n"));

    mu_check(test_language(NO_ERROR, &lang, "meta \n"
                                            "    endianness Big \n"
                                            "    namewidth 9 \n"
                                            "    nameshift 1 \n"));

    mu_check(test_language(NO_ERROR, &lang, "meta \n"
                                            "    endianness BIG \n"
                                            "    namewidth 9 \n"
                                            "    nameshift 1 \n"));

    ////////////////////////////////////////////
    // Check for errors in the metadata block //
    ////////////////////////////////////////////

    mu_check(test_language(MALFORMED_METADATA_ATTRIBUTE, NULL,
                           "meta \n"
                           "    endianness aaaaahhhhh \n"));

    mu_check(test_language(DUPLICATE_METADATA_ATTRIBUTE, NULL,
                           "meta \n"
                           "    endianness BIG \n"
                           "    endianness LITTLE \n"));

    mu_check(test_language(UNKNOWN_METADATA_ATTRIBUTE, NULL,
                           "meta \n"
                           "    fake_attr bla \n"));

    //////////////////////////////////////////
    // Errors on non-leading metadata block //
    //////////////////////////////////////////

    mu_check(test_language(MISPLACED_METADATA_BLOCK, NULL,
                           "def 0x01 emptyfn \n"
                           "def 0x02 emptyfn2 \n"
                           "meta \n"
                           "    namewidth 3 \n"));

    mu_check(test_language(MISPLACED_METADATA_BLOCK, NULL,
                           "meta \n"
                           "    endianness big \n"
                           "def 0x01 emptyfn \n"
                           "def 0x02 emptyfn2 \n"
                           "meta \n"
                           "    namewidth 3 \n"));

    free_lang(&lang);
}

void mu_test_parse_malformed() {
    argument_def a;
    mu_eq(PARSE_ERROR, UNSPECIFIED_SIZE, parse_argtype(&a, "int"));
    mu_check(test_language(UNSPECIFIED_SIZE, NULL, "def 0x08 test {\n"
                                                   "    uint(intarg)\n"
                                                   "}\n"));
}

void mu_test_parse_language_from_str() {
    language_def lang;
    lang_init(&lang);

    lang.function_name_width = 6;
    lang.function_name_bitshift = 2;

    // arguments and function declaration for graphic
    argument_def graphic_1 = { SKIP, 6, NULL }, graphic_2 = { INT, 4, "gfx" },
                 graphic_3 = { SKIP, 4, NULL }, graphic_4 = { INT, 4, "zoff" },
                 graphic_5 = { INT, 4, "yoff" }, graphic_6 = { INT, 4, "xoff" },
                 graphic_7 = { INT, 4, "zrange" },
                 graphic_8 = { INT, 4, "yrange" },
                 graphic_9 = { INT, 4, "xrange" };
    argument_def *graphic_args[] = { &graphic_1, &graphic_2, &graphic_3,
                                     &graphic_4, &graphic_5, &graphic_6,
                                     &graphic_7, &graphic_8, &graphic_9 };
    function_def graphic_fn = {
        .function_binary_value = (0x10 >> 2),
        .name = "graphic",
        .argc = 9,
        .arguments = graphic_args,
    };
    add_fn_to_lang(&lang, &graphic_fn);

    // arguments and function def for test
    argument_def test_1 = { SKIP, 6, NULL },
                 test_2 = { UNSIGNED_INT, 32, "intarg" },
                 test_3 = { FLOAT, 32, "floatarg" };
    argument_def *test_args[] = {
        &test_1, &test_2, &test_3,
    };
    function_def test_fn = {
        .function_binary_value = (0x08 >> 2),
        .name = "test",
        .argc = 3,
        .arguments = test_args,
    };
    add_fn_to_lang(&lang, &test_fn);

    mu_check(test_language(NO_ERROR, &lang,
                           "meta\n"
                           "    namewidth 6\n"
                           "    nameshift 2\n"
                           "\n"
                           "\n"
                           "def 0x10 graphic {\n"
                           "    skip6 int4(gfx) skip4\n"
                           "    int4(zoff) int4(yoff) int4(xoff)\n"
                           "    int4(zrange) int4(yrange) int4(xrange)\n"
                           "}\n"
                           "\n"
                           "def 0x08 test {\n"
                           "    skip6\n"
                           "    uint32(intarg)\n"
                           "    float32(floatarg)\n"
                           "}\n"
                           "\n"));

    _free_lang(&lang, false);
}
