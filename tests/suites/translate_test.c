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

bool translate_test_bin2script(
        language_def * lang,
        char * name,
        char * example_input,
        char * expected_output) {

    // parse and check that the stringified version
    // of the function is the same as expected
    binscript_consumer *mem_consumer =
        binscript_mem_consumer(
            lang, example_input, BIN2SCRIPT);


    char real_output[1024];

    function_call * call;
    for (int i=0; (call = binscript_next(mem_consumer)) != NULL; i++) {
        size_t real_out_size = string_encode_function_call(real_output, call);
        if (0 != strncmp(
                    expected_output,
                    real_output,
                    real_out_size)) {
            printf("bin2script: error in function call %d of translation case %s\n", i, name);
            printf("expected '%*s'\n", (int) real_out_size, expected_output);
            printf("got '%s'\n", real_output);
            free_call(call);
            binscript_free(mem_consumer);
            return false;
        }
        expected_output += real_out_size + 1;
        free_call(call);
    }
    binscript_free(mem_consumer);

    return true;
}


bool translate_test_script2bin(
        language_def * lang,
        char * name,
        char * example_input,
        char * expected_output) {

    // parse and check that the stringified version
    // of the function is the same as expected
    binscript_consumer *mem_consumer =
        binscript_mem_consumer(
            lang, example_input, SCRIPT2BIN);

    char real_output[1024];       

    function_call * call;
    for (int i=0; (call = binscript_next(mem_consumer)) != NULL; i++) {
        print_fn_call(call);
        size_t real_out_size = binary_encode_function_call(real_output, lang, call);
        if (0 != strncmp(
                    expected_output,
                    real_output,
                    real_out_size)) {
            printf("script2bin: error in function call %d of translation case %s\n", i, name);
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



////////////////
// TEST CASES //
////////////////

struct binary_script_mapping {
    char * name;
    char * binary;
    char * strings;
};

static struct binary_script_mapping repr_map[] = {
    {
        .name = "CASE_TEST_MAP",
        .binary = (char []) {
            0x08, 
            0x00, 0x00, 0x00, 0x80, 
            0x44, 0x42, 0x71, 0x48, 

            0x08, 
            0x00, 0x00, 0x00, 0x0a,
            0x46, 0x0a, 0xe0, 0x2f,

            0x00
        },
        .strings = (char []) {
            "test(128 777.770020)\n"
            "test(10 8888.045898)"
        }
    }
};
    


/////////////////////////
// ACTUAL TEST SCRIPTS //
/////////////////////////


void mu_test_translate_test() {
    // parse the language
    language_def lang;
    PARSE_ERROR p = parse_language_from_str(&lang, 
        "meta\n"
        "    endianness big\n"
        "    namewidth 6\n"
        "    nameshift 2\n"
        "\n"
        "def 0x08 test {\n"
        "    skip2\n"
        "    uint32(intarg)\n"
        "    float32(floatarg)\n"
        "}\n"
        "\n"
        );
    mu_ensure_eq(PARSE_ERROR, NO_ERROR, p);
   
    size_t len_map = sizeof(repr_map) / sizeof(struct binary_script_mapping);
    for (size_t i=0; i < len_map; i++) {
        translate_test_bin2script(&lang, 
                repr_map[i].name,
                repr_map[i].binary,
                repr_map[i].strings);

        translate_test_script2bin(&lang, 
                repr_map[i].name,
                repr_map[i].strings,
                repr_map[i].binary);
    }

    // free the language definition
    free_lang(&lang);
}


