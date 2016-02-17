#ifndef BINARY_SCRIPTER
#define BINARY_SCRIPTER

#include "sweetexpressions.h"
#include <stdbool.h>

#include "langdef.h"

typedef enum PARSE_ERROR {
    NO_ERROR = 0,

    // integer parsing errors
    UNKNOWN_INT_FORMAT = 1,
    BAD_DECIMAL_FORMAT = 2,
    BAD_HEX_FORMAT = 3,
    BAD_BINARY_FORMAT = 4,
    ILLEGAL_SIGN = 5,

    // argument parsing errors
    UNKNOWN_ARGTYPE = 6,
    DISALLOWED_SIZE = 7,
    UNSPECIFIED_SIZE = 8,

    // function parsing errors
    MISSING_DEF = 9,
    MISSING_BINNAME = 10,
    MALFORMED_BINNAME = 11,
    MISSING_NAME = 12,
    FUNCTION_BINNAME_PRECISION = 13,
    FUNCTION_BINNAME_SIZE = 14,
    MALFORMED_ARGUMENT_DECLARATION = 15,

    // metadata parsing errors
    MISPLACED_METADATA_BLOCK = 16,
    UNKNOWN_METADATA_ATTRIBUTE = 17,
    MALFORMED_METADATA_ATTRIBUTE = 18,
    DUPLICATE_METADATA_ATTRIBUTE = 19,

    // function call parsing errors
    UNKNOWN_FUNCTION_NAME = 20,
    ARG_VALUE_PARSE_ERROR = 21,
    MISSING_ARG = 22,
    LEFTOVER_ARG = 23,
} PARSE_ERROR;

typedef struct detailed_parse_error {
    PARSE_ERROR primitive_error;
    swexp_list_node *error_location;
    char *error_message;
} detailed_parse_error;

const char * error_message_name (PARSE_ERROR err);

/**
 * Parses an integer from a  decimal, hex or binary string.
 * returns a PARSE_ERROR on failure
 *
 **/
PARSE_ERROR parse_int(int *out, char *str);

/**
 * Parses an integer from a decimal, hex or binary string.
 * On failure, calls exit(1);
 * uparse_int("10") = uparse_int("0xA") = uparse_int("0b1010")
 **/
int uparse_int(char *str);

/**
 * parses an argument's type declaration. This is the argument type, as
 * well as a bitwidth (argument_def). returns a parse error on failure.
 *
 * argument_def * argument: the argument to be initialized
 * name: the argument type name (ex: int32)
 **/
PARSE_ERROR parse_argtype(argument_def *argument, char *name);

/**
 * Takes a function definition from an s expression, and initializes the
 * function_call in `function` to the equivalent value.
 * Does not add the function to the language.
 *
 * function: The function definition to be initialized
 * language_def: The language who's metadata will be used to
 *      evaluate the thing
 * description: The definition of the function as a swexp list node
 **/
PARSE_ERROR parse_fn(function_def *function, language_def *language,
                     swexp_list_node *description);

/** Adds a function to a language
 * languages "own" functions added to them:- freeing the language with
 * free_lang() will free all of the added functions.
 *
 * language: the language that `def` is added to
 * def: the function to add to `lanugage`
 */
void add_fn_to_lang(language_def *language, function_def *def);

/**
 * Parses a function call from a textual s-expression and a
 * corresponding language definiton,and returns a function_call
 * object
 *
 * call: flat swexp list of the function call w/ arguments
 * l: definition of language defining call made in `call`
 **/
PARSE_ERROR parse_fn_call(function_call *call, language_def *l,
                          swexp_list_node *nodes);
PARSE_ERROR parse_arg(void **result, argument_def *arg, char *str_repr);

/**
 * Parses a language definition out of a file into a language_def
 * object
 *
 * language: pointer to the language to be initialized
 * f: pointer to a FILE containing a series of s-expressions
 *      that define functions or that define language metadata
 **/
PARSE_ERROR parse_language(language_def *language, swexp_list_node *list);

// TODO document these

void lang_init(language_def *lang);

PARSE_ERROR parse_language_from_file(language_def *language, FILE *f);

PARSE_ERROR parse_language_from_str(language_def *language, char *c);

/**
 * parses a metadata block from a language definition and sets the
 * appropriate parameters of the language definition to their
 * corresponding values in the metadata declaration
 *
 * language: the language the metadata applies to
 * metadata_decl: the declaratin of the metadata as a swexp list
 **/
PARSE_ERROR parse_metadata(language_def *language,
                           swexp_list_node *metadata_decl);

/**
 * parses a single metadata attribute
 **/
PARSE_ERROR parse_metadata_attr(language_def *l, swexp_list_node *node);

#endif
