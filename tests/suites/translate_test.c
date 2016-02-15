#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "../mutest.h"
#include "translator.h"
#include "langdef.h"
#include "parsescript.h"


bool translate_test(
        language_def * lang,
        char example_input[],
        char * expected_outputs[]) {

    // parse and check that the stringified version
    // of the function is the same as expected
    binscript_consumer *mem_consumer =
        binscript_mem_consumer(
            lang, example_input, BIN2SCRIPT);


    char real_output[1024];

    function_call * call;
    for (int i=0; (call = binscript_next(mem_consumer)) != NULL; i++) {
        stringify_fn_call(real_output, call);
        if (0 != strncmp(
                    expected_outputs[i],
                    real_output,
            strlen(expected_outputs[i]) + 1)) {
            printf("error in function call %d\n of test", i);
            printf("expected '%s'\n", expected_outputs[i]);
            printf("got '%s'\n", real_output);
        }
        free_call(call);
    }
    binscript_free(mem_consumer);

    return true;
}

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

    translate_test(&lang,
        (char []) {
            0x08, 
            0x00, 0x00, 0x00, 0x80, 
            0x44, 0x42, 0x71, 0x48, 

            0x08, 
            0x00, 0x00, 0x00, 0x0a,
            0x46, 0x0a, 0xe0, 0x2f,

            0x00
        }, 
        (char *[]) {
            "test(128, 777.770020)",
            "test(10, 8888.045898)"
        });
    
    // free the language definition
    free_lang(&lang);
}

