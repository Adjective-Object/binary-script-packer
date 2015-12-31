#ifndef BINARY_SCRIPTER
#define BINARY_SCRIPTER

#include "sweetexpressions.h"
#include "endian.h"
#include <stdbool.h>

#include "langdef.h"

language_def * parse_language(FILE * f);
function_call * parse_fn_call(swexp_list_node * call, language_def * l);

void  parse_argtype(argument_def * argument,
        swexp_list_node * node);
int parse_int(char * str);

void add_fn_to_lang(language_def *l, function_def * def);

function_def * parse_fn(language_def * l, swexp_list_node * node);

void parse_metadata(language_def *l, swexp_list_node * metadata_decl) {
void parse_metadata_attr(language_def * l, swexp_list_node * node);



#endif
