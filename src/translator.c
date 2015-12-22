#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "langdef.h"
#include "translator.h"
#include "util.h"
#include "bitbuffer.h"

/**
 * Initialize everything about a consumer except for the source
 **/
static binscript_consumer * binscript_undef_consumer(
        language_def * lang, binscript_parser_direction direction) {
    binscript_consumer * c = 
        (binscript_consumer *) malloc(sizeof(binscript_consumer));
    c->lang = lang;
    c->endmode = NULL_TERMINATED;
    c->direction = direction;

    c->internal_buf_len = 1;
    bitbuffer_init(&(c->internal_buf), c->internal_buf_len);

    return c;
}

/**
* Creates a consumer in a given direction
 * with null terminated ending condition
 **/
binscript_consumer * binscript_file_consumer(
        language_def * lang,
        FILE * f,
        binscript_parser_direction direction){
 
    binscript_consumer * c = binscript_undef_consumer(lang, direction);

    c->parser_source = FROM_FILE;
    c->source = (void *) f;

    return c;
}

binscript_consumer * binscript_mem_consumer(
        language_def * lang,
        void * mem,
        binscript_parser_direction direction) {

    binscript_consumer * c = binscript_undef_consumer(lang, direction);

    c->parser_source = FROM_MEMORY;
    c->source = mem;

    return c;
}

void consumer_set_size(
        binscript_consumer* c,
        binscript_endmode endmode,
        unsigned int remaining) {
   c->endmode = endmode;
   c->remaining_size = remaining;
}

// put the first <bytes> available bytes in <consumer> into <buffer>,
// without changing the position of the consumer
void binscript_peek_head(
        binscript_consumer * consumer,
        void * buffer, size_t bytes) {
    switch(consumer->parser_source) {
        case FROM_FILE:
            if(0 >= fread(buffer, 1, bytes, (FILE *) consumer->source)) {
                printf("error trying to read from file (%p) in peek_head\n",
                        consumer->source);
                exit(1);
            }
            // step back
            if(0 > fseek((FILE *) consumer->source, -bytes, SEEK_CUR)) {
                perror("fseek");
                exit(1);
            }
            break;
        case FROM_MEMORY:
            memcpy(buffer, consumer->source, bytes);
            break;
    }
}

// put the first <bytes> available bytes in <consumer> into <buffer>,
// and advance the consumer past those bytes.
void binscript_pop_head(
        binscript_consumer * consumer,
        void * buffer, size_t bytes) {
    switch(consumer->parser_source) {
        case FROM_FILE:
            if(0 >= fread(buffer, 1, bytes, (FILE *) consumer->source)) {
                printf("error trying to read from file (%p) in pop_head\n",
                        consumer->source);
                exit(1);
            }
            break;
        case FROM_MEMORY:
            memcpy(buffer, consumer->source, bytes);
            consumer->source += bytes;
            break;
    }
}

// peek a function name from the top of a consumer
unsigned int binscript_peek_fn(binscript_consumer * consumer) {

    // get the width of the function name in bytes
    size_t fname_size_bits = consumer->lang->function_name_width,
        fname_size_bytes = bits2bytes(fname_size_bits);

    // get the head of the buffer 
    char * fname_buffer = malloc(sizeof(char) * fname_size_bytes);
    binscript_peek_head(consumer, fname_buffer, fname_size_bytes);

    unsigned int fn_name = funcname_from_buffer(consumer->lang, fname_buffer);

    // free the file name buffer and then clear it
    free(fname_buffer);
    return fn_name;
}

#define MAX_FN_BUFFER_SIZE 255
function_call * binscript_next(binscript_consumer *consumer) {
    /* 
    while(true) {
        char c = 0;
        binscript_pop_head(consumer, c, 1);
        printf("%d\n", (unsigned char) c);
    }
    */

    // The id of the function being called
    unsigned int function_id = binscript_peek_fn(consumer);

    // get the body of the function based on the width
    function_def * funcdef = lang_getfn(consumer->lang, function_id);
    printf("got function: %s\n", funcdef->name);
    
    if (funcdef == NULL) {
        printf("could not look up function with id 0x%x\n", function_id);
        exit(1);
    }

    size_t func_width = bits2bytes(
            func_call_width(consumer->lang, funcdef));
    char * funcBuffer = (char *) malloc(sizeof(char) * func_width);
    binscript_pop_head(consumer, funcBuffer, func_width);
    
    printf("function buffer contents: ");
    print_hex(funcBuffer, func_width);

    function_call * call = lang_callfn(consumer->lang, funcBuffer);
    free(funcBuffer);
    return call;
}
