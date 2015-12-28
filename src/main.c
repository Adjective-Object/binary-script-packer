#include <stdlib.h>
#include <stdio.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <stdint.h>

#include "langdef.h"
#include "parsescript.h"
#include "translator.h"

int main(int argc, char ** argv) {
 
    ////////////////////////////////////
    // Open the File & Parse Language //
    ////////////////////////////////////

    FILE * lang_file = fopen("example.langdef", "r");
    if (lang_file == NULL) {
        printf("could not open file 'example.langdef'\n");
        exit(1);
    }

    language_def * l = parse_language(lang_file);

    printf("Language definition:\n");
    print_lang(l);

    fclose(lang_file);

    //////////////////////////////////////////////
    // Test the output from the stream consumer //
    //////////////////////////////////////////////

    FILE * packed_file = fopen("example.hex", "r");
    if (packed_file == NULL) {
        printf("could not open file 'example.hex'\n");
        exit(1);
    }

    binscript_consumer * consumer = 
        binscript_file_consumer(l, packed_file, BIN2SCRIPT);
    consumer_set_size(consumer, NULL_TERMINATED, 0);


    printf("\nHex file contents: \n");
    function_call * call;
    while((call = binscript_next(consumer)) != NULL) {
        print_fn_call(call);
        free_call(call);
    }

    binscript_free(consumer);

    //////////////////////////////////////////////
    // Test the output from the buffer consumer //
    //////////////////////////////////////////////

    // stat the file to find out the fsize
    int fno = fileno(packed_file);
    struct stat packed_file_stats;
    fstat(fno, &packed_file_stats);


    printf("\nmmapping file..\n");
    fseek(packed_file, 0, SEEK_SET);
    char * map_origin = (char *) mmap(
            NULL, packed_file_stats.st_size,
            PROT_READ, MAP_SHARED, fno, 0);
    if (((intptr_t) map_origin) == -1) {
        perror("mmap");
        exit(1);
    }

    printf("map origin: %p\n", map_origin);
    
    // create a script translator and translate it
    binscript_consumer * mem_consumer = 
        binscript_mem_consumer(l, map_origin, BIN2SCRIPT);

    printf("\nHex file contents: \n");
    while((call = binscript_next(mem_consumer)) != NULL) {
        print_fn_call(call);
        free_call(call);
    }

    fclose(packed_file);
    binscript_free(mem_consumer);
    free_lang(l);

}

