#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "../mutest.h"
#include "translator.h"
#include "langdef.h"
#include "parsescript.h"
#include "util.h"

/////////////
// HELPERS //
/////////////

bool translate_test_bin2script(language_def *lang, char *name,
                               char *example_input, char *expected_output) {

    // parse and check that the stringified version
    // of the function is the same as expected
    binscript_consumer *mem_consumer =
        binscript_mem_consumer(lang, example_input, name, BIN2SCRIPT);

    char real_output[1024];

    function_call *call;
    for (int i = 0; (call = binscript_next(mem_consumer)) != NULL; i++) {
        size_t real_out_size = string_encode_function_call(real_output, call);
        if (0 != strncmp(expected_output, real_output, real_out_size)) {
            printf("bin2script: error in function call %d of translation case "
                   "%s\n",
                   i, name);
            printf("expected '%s'\n", expected_output);
            printf("got '%s'\n", real_output);
            free_call(call);
            binscript_free(mem_consumer);
            return false;
        }
        expected_output += strlen(expected_output) + 1;
        free_call(call);
    }
    binscript_free(mem_consumer);

    return true;
}

bool translate_test_script2bin(language_def *lang, char *name,
                               char *example_input, char *expected_output) {

    // parse and check that the stringified version
    // of the function is the same as expected
    binscript_consumer *mem_consumer =
        binscript_mem_consumer(lang, example_input, name, SCRIPT2BIN);

    char real_output[1024];

    function_call *call;
    for (int i = 0; (call = binscript_next(mem_consumer)) != NULL; i++) {
        print_fn_call(call);
        size_t real_out_size =
            binary_encode_function_call(real_output, lang, call);
        if (0 != strncmp(expected_output, real_output, real_out_size)) {
            printf("script2bin: error in function call %d of translation case "
                   "%s\n",
                   i, name);
            printf("expected ");
            print_hex(expected_output, real_out_size);
            printf("got      ");
            print_hex(real_output, real_out_size);
        }
        expected_output += real_out_size;
        free_call(call);
    }
    binscript_free(mem_consumer);

    return true;
}

/////////////////////////////
// GLOBAL TESTING LANGAUGE //
/////////////////////////////

language_def testlang;

int mu_init_translate_test() {
    // parse the language
    detailed_parse_error * e = parse_language_from_str( &testlang, 
            "meta\n"
           "    endianness big\n"
           "    namewidth 6\n"
           "    nameshift 2\n"
           "\n"
           "def 0x08 test {\n"
           "    skip2\n"
           "    uint32(intarg)\n"
           "    float32(floatarg)\n"
           "}"
           "\n"
           "def 0x0c test2 {\n"
           "    skip2\n"
           "    uint64(intarg)\n"
           "    float64(floatarg)\n"
           "}\n"
           "\n"
           "def 0x10 graphic {\n"
           "    skip2 int32(gfx) skip32\n"
           "    float64(zoff) float64(yoff) float64(xoff)\n"
           "    float64(zrange) float64(yrange) float64(xrange)\n"
           "}\n"
           "\n"
           "def 0x18 stringmethod {\n"
           "    skip2 \n"
           "    str32(name_terminated)\n"
           "    raw_str32(name_nonterminated)\n"
           "}\n"
           "\n",
        "testlang");

    if (e != NULL) {
        print_err(e);
        free_err(e);
        return 1;
    }
    return 0;
}

void mu_term_translate_test() {
    // free the language definition
    free_lang(&testlang);
}

////////////////
// TEST CASES //
////////////////

struct binary_script_mapping {
    char *name;
    char *binary;
    char *strings;
};

static struct binary_script_mapping repr_map[] = {
    {.name = "CASE_TEST_MAP",
     .binary =
         (char[]){
             // test(128, 777.77)
             0x08, 0x00, 0x00, 0x00, 0x80, 0x44, 0x42, 0x71, 0x48,

             // test(10, 888.88)
             0x08, 0x00, 0x00, 0x00, 0x0a, 0x46, 0x0a, 0xe0, 0x2f,
             0x00 // terminating char
         },
     .strings = (char[]){ "test(128 777.770020)\0"
                          "test(10 8888.045898)\0" } },
    {.name = "CASE_TEST2",
     .binary =
         (char[]){ // test2(100, 100.0)
                   0x0c, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x64, 0x40,
                   0x59, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,

                   0x00 },
     .strings = (char[]){ "test2(100 100.000000)\0" } },
    {.name = "TEST_GFX",
     .binary =
         (char[]){
             // graphic(1, <skip32>, 100.0, 100.1, 100.2, 100.3, 100.4, 100.5)
             0x10, 0x00, 0x00, 0x00, 0x01,                   // gfxid = 1
             0x00, 0x00, 0x00, 0x00,                         // skip
             0x40, 0x59, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, // 100.0
             0x40, 0x59, 0x06, 0x66, 0x66, 0x66, 0x66, 0x66, // 100.1
             0x40, 0x59, 0x0C, 0xCC, 0xCC, 0xCC, 0xCC, 0xCD, // 100.2
             0x40, 0x59, 0x13, 0x33, 0x33, 0x33, 0x33, 0x33, // 100.3
             0x40, 0x59, 0x19, 0x99, 0x99, 0x99, 0x99, 0x9A, // 100.4
             0x40, 0x59, 0x20, 0x00, 0x00, 0x00, 0x00, 0x00, // 100.5

             0x00 },
     .strings = (char[]){ "graphic(1 100.000000 100.100000 100.200000 "
                          "100.300000 100.400000 100.500000)\0" } }
};

/////////////////////////
// ACTUAL TEST SCRIPTS //
/////////////////////////

void mu_test_translate_test() {

    size_t len_map = sizeof(repr_map) / sizeof(struct binary_script_mapping);
    for (size_t i = 0; i < len_map; i++) {
        translate_test_bin2script(&testlang, repr_map[i].name,
                                  repr_map[i].binary, repr_map[i].strings);

        translate_test_script2bin(&testlang, repr_map[i].name,
                                  repr_map[i].strings, repr_map[i].binary);
    }
}


void mu_test_translate_melee() {
    language_def meleelang;
    FILE * f = fopen("./tests/languages/melee.langdef", "r");
    parse_language_from_file(&meleelang, f, "melee.langdef");
    fclose(f);

    char melee_lang_bin[] = {
        0xA0, 0x00, 0x00, 0x03,
        0xA0, 0x04, 0x00, 0x03,
        0x00, 0x00, 0x00, 0x00
    };

    char melee_lang_str[] = {
        "texswap?()\0"
        "texswap?()\0"
        "exit\0"
    };

    translate_test_bin2script(
            &meleelang,
            "melee_lang",
            melee_lang_bin,
            melee_lang_str);

    free_lang(&meleelang);
}

