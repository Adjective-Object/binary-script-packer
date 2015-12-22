#include <stdlib.h>

#include "langdef.h"
#include "parsescript.h"
#include "translator.h"

int main(int argc, char ** argv) {
    FILE * lang_file = fopen("example.langdef", "r");
    if (lang_file == NULL) {
        printf("could not open file 'example.langdef'\n");
        exit(1);
    }
    language_def * l = parse_language(lang_file);

    print_lang(l);

    FILE * packed_file = fopen("example.hex", "r");
    if (packed_file == NULL) {
        printf("could not open file 'example.hex'\n");
        exit(1);
    }

    binscript_consumer * consumer = 
        binscript_file_consumer(l, packed_file, BIN2SCRIPT);
    consumer_set_size(consumer, NULL_TERMINATED, 0);

    function_call * call;
    while((call = binscript_next(consumer)) != NULL) {
        print_fn_call(call);
    }
}

