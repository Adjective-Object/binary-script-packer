#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdint.h>

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
            //printf("%p <- %p\n", buffer, consumer->source);
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
            consumer->source = (void*) ((char *)consumer->source + bytes);
            break;
    }
}

// peek a function name from the top of a consumer
unsigned int binscript_peek_fn(binscript_consumer * consumer) {

    // get the width of the function name in bytes
    size_t fname_size_bits = consumer->lang->function_name_width,
    fname_size_bytes = bits2bytes(fname_size_bits);

    // get the head of the buffer 
    char * fname_buffer = (char *) malloc(sizeof(char) * fname_size_bytes);
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

    if (function_id == 0) {
        return NULL;
    }

    // get the body of the function based on the width
    function_def * funcdef = lang_getfn(consumer->lang, function_id);

    if (funcdef == NULL) {
        printf("could not look up function with id 0x%x\n", function_id);
        exit(1);
    }

    size_t func_width = bits2bytes(
            func_call_width(consumer->lang, funcdef));
    char * funcBuffer = (char *) malloc(sizeof(char) * func_width);
    binscript_pop_head(consumer, funcBuffer, func_width);

    //printf("function buffer contents: ");
    //print_hex(funcBuffer, func_width);

    function_call * call = decode_function_call(
            consumer->lang, funcBuffer, func_width);
    free(funcBuffer);
    return call;
}

function_call * decode_function_call(language_def * l,
        char * databuffer, size_t databuffer_len) {
    // get the function name from a buffer
    unsigned int fn_name = funcname_from_buffer(l, databuffer);
    function_def * fn = lang_getfn(l, fn_name);

    // make a bitbuffer wrapper for the data buffer
    bitbuffer callbuffer, argbuffer;
    bitbuffer_init_from_buffer(&callbuffer, databuffer, databuffer_len);
    bitbuffer_advance(&callbuffer, l->function_name_width);

    // create the function call object
    function_call * call = (function_call *) malloc(sizeof(function_call));
    call->defn = fn;
    // allocate an array to hold pointers to each argument
    call->args = (void **) malloc(sizeof(char *) * fn->argc); 

    for (size_t i = 0; i<fn->argc; i++) {
        // make a bitbuffer for the current argument
        size_t arg_bits = fn->arguments[i]->bitwidth;
        bitbuffer_init_from_buffer(&argbuffer, callbuffer.buffer, 
                bits2bytes(arg_bits + callbuffer.head_offset));
        bitbuffer_advance(&argbuffer, callbuffer.head_offset);

        // initialie the current argument from that bitbuffer
        call->args[i] = arg_init(l, fn->arguments[i], &argbuffer);

        // advance the global buffer to the next argument;
        bitbuffer_advance(&callbuffer, arg_bits); 
    }

    return call;
}

void encode_function_call(bitbuffer * out_buffer,
        language_def * l, function_call * call) {
    // write the name of the function to the buffer
    unsigned int name = call->defn->function_binary_value;
    for (int i=0; i<l->function_name_width; i++) {
        bitbuffer_writebit(out_buffer, (name >> i) & 1);
    }

    // write each of the arguments
    for(size_t i=0 ;i<call->defn->argc; i++) {
        arg_write(out_buffer, 
                l, call->defn->arguments[i],
                call->args[i]);
    }
}


unsigned int funcname_from_buffer(language_def * lang, char * fname_buffer) {

    // function sizes
    size_t fname_size_bits = lang->function_name_width,
           fname_size_bytes = bits2bytes(fname_size_bits); 

    // create the number from the buffer
    unsigned int fn_name = 0;
    for(unsigned int byte = 0; byte < fname_size_bytes; byte++) {
        unsigned int remainderbits = fname_size_bits - byte * 8;
        unsigned int firstbit;
        if (remainderbits >= 8 || remainderbits == 0) {
            firstbit = 0;
        } else {
            firstbit = 8 - remainderbits;
        }

        for (unsigned int bitoff = firstbit; bitoff < 8; bitoff++) {
            fn_name = (fn_name << 1) | ((fname_buffer[byte] >> (7 - bitoff)) & 1);
        }
    }

    return fn_name >> lang->function_name_bitshift;
}

void binscript_free(binscript_consumer * c) {
    // don't invoke the free helper since the bitbuffer is
    // a field of the consumer
    free(c->internal_buf.buffer_origin);
    free(c);
}

