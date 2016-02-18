#ifndef BINSCRIPTR_TRANSLATE
#define BINSCRIPTR_TRANSLATE

#include <stdbool.h>
#include <stdio.h>

#include "langdef.h"
#include "bitbuffer.h"
#include "sweetexpressions.h"

typedef enum binscript_parser_direction {
    SCRIPT2BIN,
    BIN2SCRIPT,
} binscript_parser_direction;

typedef enum binscript_endmode {
    NULL_TERMINATED,
    SIZE_STATEMENTS,
    SIZE_BYTES,
    MANUAL_CUTOFF,
} binscript_endmode;

typedef enum binscript_source {
    FROM_FILE,
    FROM_MEMORY,
} binscript_source;

typedef struct binscript_consumer {
    language_def *lang;
    binscript_parser_direction direction;

    binscript_endmode endmode; // how to look for the end of a statement
    unsigned int remaining_size;

    binscript_source parser_source; // the type of the inputdd
    void *source;                   // NULL if parser_source = FROM_MEMORY
    swexp_list_node *nodes;

    bitbuffer internal_buf;
    size_t internal_buf_len;
} binscript_consumer;

binscript_consumer *
binscript_file_consumer(language_def *lang, FILE *f, const char * name,
                        binscript_parser_direction direction);

binscript_consumer *
binscript_mem_consumer(language_def *lang, void *mem, const char * name,
                       binscript_parser_direction direction);

void consumer_set_size(binscript_consumer *c, binscript_endmode endmode,
                       unsigned int remaining);

function_call *binscript_next(binscript_consumer *consumer);
function_call *binscript_next_fromscript(binscript_consumer *consumer);
function_call *binscript_next_frombin(binscript_consumer *consumer);

function_call *decode_function_call(language_def *l, char *databuffer,
                                    size_t databuffer_len);
unsigned int funcname_from_buffer(language_def *def, char *buffer);

size_t binary_encode_function_call(char *databuffer, language_def *l,
                                   function_call *f);

size_t string_encode_function_call(char *out, function_call *call);
size_t string_encode_function_call_keyworded(char *out, function_call *call);

void binscript_free(binscript_consumer *c);

#endif
