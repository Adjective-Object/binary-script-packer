#ifndef BINARY_SCRIPTER
#define BINARY_SCRIPTER

#include "sweetexpressions.h"
#include "endian.h"
#include <stdbool.h>

#include "langdef.h"

language_def * parse_language(FILE * f);

function_call * parse_fn_call(swexp_list_node * call, language_def * l);

#endif
