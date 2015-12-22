#ifndef BINARY_SCRIPTER
#define BINARY_SCRIPTER

#include "sweetexpressions.h"
#include "endian.h"
#include <stdbool.h>

#include "langdef.h"

language_def * parse_language(FILE * f);

#endif
