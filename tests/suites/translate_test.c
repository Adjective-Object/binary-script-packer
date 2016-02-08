#include <string.h>
#include <stdlib.h>

#include "../mutest.h"
#include "translator.h"
#include "langdef.h"
#include "parsescript.h"


void mu_test_translate_test() {
    // parse the language
    language_def lang;
    PARSE_ERROR p = parse_language_from_str(&lang, 
        "meta\n"
        "    endianness big\n"
        "    namewidth 6\n"
        "    nameshift 2\n"
        "\n"
        "\n"
        "def 0x10 graphic {\n"
        "    skip6 int4(gfx) skip4\n"
        "    int4(zoff) int4(yoff) int4(xoff)\n"
        "    int(zrange) int4(yrange) int4(xrange)\n"
        "}\n"
        "\n"
        "def 0x08 test {\n"
        "    skip2\n"
        "    uint4(intarg)\n"
        "    skip28\n"
        "    float32(floatarg)\n"
        "}\n"
        "\n"
        );
    mu_ensure_eq(PARSE_ERROR, NO_ERROR, p);

    char real_output[255];
    char example_input[] = {
        0x08, 0x80, 0xff, 0x44,
        0x42, 0x71, 0x48, 0x08,
        0x00, 0x00, 0x46, 0x0a,
        0xe0, 0x2f, 0x00
    };
    char * expected_outputs[] = {
        "test(128, 777.770020)",
        "test(10, 8888.045898)",
    };

    // parse and check that the stringified version
    // of the function is the same as expected
    binscript_consumer *mem_consumer =
        binscript_mem_consumer(
                &lang, example_input, BIN2SCRIPT);

    int i=0;
    function_call * call;
    while ((call = binscript_next(mem_consumer)) != NULL) {
        stringify_fn_call(real_output, call);
        printf("%s and  %s\n", real_output, expected_outputs[i]);
        mu_ensure_eq(int, 0, 
                strncmp(
                    expected_outputs[i],
                    real_output,
                    strlen(expected_outputs[i]) + 1));
        free_call(call);
        i++;
    }
    binscript_free(mem_consumer);

    // free the language definition
    free_lang(&lang);
}

