#ifndef BINSCRIPTER_LANGDEF
#define BINSCRIPTER_LANGDEF


bool check_size(arg_type type, unsigned int space,
        unsigned int requested_size);

extern const char * typenames[];
char * type_name(arg_type t);
void print_lang(language_def * l);
void print_fn(language_def *l, function_def *f);

function_def * lang_getfn(language_def * l, unsigned int binary_value);

#endif
