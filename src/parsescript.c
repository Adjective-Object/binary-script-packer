#include <stdlib.h>
#include <string.h>

#include "parsescript.h"
#include "langdef.h"
#include "util.h"

static const char* errnames[] = {
    [NO_ERROR]                       = "NO_ERROR",
    [UNKNOWN_INT_FORMAT]             = "UNKNOWN_INT_FORMAT",
    [BAD_DECIMAL_FORMAT]             = "BAD_DECIMAL_FORMAT",
    [BAD_HEX_FORMAT]                 = "BAD_HEX_FORMAT",
    [BAD_BINARY_FORMAT]              = "BAD_BINARY_FORMAT",
    [ILLEGAL_SIGN]                   = "ILLEGAL_SIGN",
    // argument parsing errors
    [UNKNOWN_ARGTYPE]                = "UNKNOWN_ARGTYPE",
    [DISALLOWED_SIZE]                = "DISALLOWED_SIZE",
    [UNSPECIFIED_SIZE]               = "UNSPECIFIED_SIZE",
    // function parsing errors
    [MISSING_DEF]                    = "MISSING_DEF",
    [MALFORMED_FUNCTION_DECL]        = "MALFORMED_FUNCTION_DECL",
    [MISSING_BINNAME]                = "MISSING_BINNAME",
    [MALFORMED_BINNAME]              = "MALFORMED_BINNAME",
    [MISSING_NAME]                   = "MISSING_NAME",
    [FUNCTION_BINNAME_PRECISION]     = "FUNCTION_BINNAME_PRECISION",
    [FUNCTION_BINNAME_SIZE]          = "FUNCTION_BINNAME_SIZE",
    [MALFORMED_ARGUMENT_DECLARATION] = "MALFORMED_ARGUMENT_DECLARATION",
    // metadata parsing errors       
    [MISPLACED_METADATA_BLOCK]       = "MISPLACED_METADATA_BLOCK",
    [UNKNOWN_METADATA_ATTRIBUTE]     = "UNKNOWN_METADATA_ATTRIBUTE",
    [MALFORMED_METADATA_ATTRIBUTE]   = "MALFORMED_METADATA_ATTRIBUTE",
    // function call parsing errors
    [DUPLICATE_METADATA_ATTRIBUTE]   = "DUPLICATE_METADATA_ATTRIBUTE",
    [UNKNOWN_FUNCTION_NAME]          = "UNKNOWN_FUNCTION_NAME",
    [ARG_VALUE_PARSE_ERROR]          = "ARG_VALUE_PARSE_ERROR",
    [MISSING_ARG]                    = "MISSING_ARG",
    [LEFTOVER_ARG]                   = "LEFTOVER_ARG",
    [ATOM_AT_ROOT]                   = "ATOM_AT_ROOT",
    [UNKNOWN_ROOT]                    = "UNKNOWN_ROOT",
};

detailed_parse_error * err(
        swexp_list_node * source,
        PARSE_ERROR primitive_err,
        const char * msg) {
    detailed_parse_error * e = malloc(sizeof(detailed_parse_error));
    e->primitive_error = primitive_err;
    e->error_message = msg;

    if (source->location != NULL) {
        e->location = malloc(sizeof(source_location));
        memcpy(e->location, source->location, sizeof(source_location));
    } else {
        e->location = NULL;
    }

    e->next_error = NULL;
    return e;
}

detailed_parse_error * wrap_err(
        detailed_parse_error * prev,
        PARSE_ERROR primitive_err,
        const char * msg) {
    detailed_parse_error * e = malloc(sizeof(detailed_parse_error));
    e->primitive_error = primitive_err;
    e->error_message = msg;
    e->next_error = prev;
    e->location = prev->location;
    return e;
}



void print_err(detailed_parse_error * e) {
    if (e->next_error != NULL)
        print_err(e->next_error);

    //printf("error @ %p\n", e);
    //printf("  primitive -> %s\n", error_message_name(e->primitive_error));
    //printf("  message -> %s\n", e->error_message);

    const char * filename = "???";
    size_t line = 0, column = 0;

    if (e->location != NULL) {
        filename = e->location->source_file_name;
        line = e->location->line;
        column = e->location->column;
    }

    printf(ANSI_COLOR_RED "Error " 
            ANSI_COLOR_BLUE "%s"  ANSI_COLOR_RED
            " in %s @ (%lu:%lu): %s\n" ANSI_COLOR_RESET,
            error_message_name(e->primitive_error),
            filename, line, column,
            e->error_message);
}

void free_err(detailed_parse_error * e) {
    if (e != NULL) {
        if (e->next_error == NULL || e->next_error->location != e->location) {
            free(e->location);
        }

        free_err(e->next_error);
        free(e);
    }
}

const char * error_message_name (PARSE_ERROR err) {
    int ipe = (int) err;
    if (ipe < 0 || ipe >= sizeof(errnames) / sizeof(char*)) {
        return "FUCK";
    }
    else {
        return errnames[err];
    }
}

int scan_binary(int *out, char *str) {
    int val = 0;
    for (; *str != '\0'; str++) {
        val = val << 1;
        if (*str == '1') {
            val = val | 1;
        } else if (*str != '0') {
            return 1;
        }
    }
    *out = val;
    return 0;
}

PARSE_ERROR parse_uint(unsigned int *i, char *str) {
    int temp;
    PARSE_ERROR err = parse_int(&temp, str);
    if (err != NO_ERROR)
        return err;
    if (temp < 0)
        return ILLEGAL_SIGN;
    *i = temp;
    return NO_ERROR;
}

PARSE_ERROR parse_int(int *out, char *str) {
    // get the leading '-' for negatives
    int sign = 1;
    if (*str == '-') {
        sign = -1;
        str++;
    }

    // try hex, then binary, then decimal
    if (strncmp(str, "0x", 2) == 0) {
        if (0 == sscanf(str + 2, "%x", out))
            return BAD_HEX_FORMAT;
    } else if (strncmp(str, "0b", 2) == 0) {
        if (scan_binary(out, str + 2))
            return BAD_BINARY_FORMAT;
    } else {
        // sscanf doesn't check character ranges on %d for
        // some reason so we check manually
        for (char *s = str; *s != '\0'; s++) {
            if (((*s - '0') < 0 || (*s - '0') > 9) && *s != '.') {
                return BAD_DECIMAL_FORMAT;
            }
        }
        if (0 == sscanf(str, "%d", out))
            return BAD_DECIMAL_FORMAT;
    }

    // if any worked, return with no error
    *out = *out * sign;
    return NO_ERROR;
}

int uparse_int(char *str) {
    int out;
    PARSE_ERROR err = parse_int(&out, str);
    switch (err) {
    case NO_ERROR:
        return out;
    case UNKNOWN_INT_FORMAT:
        printf("unknown int format in parsing int\n");
        exit(1);
    case BAD_DECIMAL_FORMAT:
        printf("bad decimal format when parsing int\n");
        exit(1);
    case BAD_HEX_FORMAT:
        printf("bad hex format when parsing int\n");
        exit(1);
    case BAD_BINARY_FORMAT:
        printf("bad binary format when parsing int\n");
        exit(1);
    default:
        printf("unknown error in parsing int (errcode = %d)\n", err);
        exit(1);
    }
}

PARSE_ERROR parse_argtype(argument_def *argument, char *name) {
    arg_type argtype;
    int argwidth;

    unsigned int i;
    for (i = 0; i < __ARG_TYPE_CT; i++) {
        if (strncmp(name, typenames[i], strlen(typenames[i])) == 0) {
            argtype = (arg_type)i;
            break;
        }
    }

    // if no argument type was found, return an error
    if (i == __ARG_TYPE_CT)
        return UNKNOWN_ARGTYPE;

    // printf("%s %s %lu %lu\n", name, typenames[i], strlen(name),
    // strlen(typenames[i]));

    // if there is no size parameter, return an error
    if (strlen(typenames[i]) == strlen(name))
        return UNSPECIFIED_SIZE;

    // if an integer was not parsed sucessfully, return an error
    PARSE_ERROR int_error = parse_int(&argwidth, name + strlen(typenames[i]));

    if (int_error != NO_ERROR)
        return int_error;

    // check that the argument is defined with valid size
    if (!validate_size(argtype, argwidth))
        return DISALLOWED_SIZE;

    // if both properties were scanned sucessfully,
    // check the validity of the argument types
    argument->bitwidth = argwidth;
    argument->type = argtype;

    return NO_ERROR;
}

void add_fn_to_lang(language_def *l, function_def *def) {
    if (l->function_ct + 1 >= l->function_capacity) {
        l->functions = realloc(l->functions,
                               sizeof(function_def *) * (l->function_ct + 1));
        if (l->functions == NULL) {
            printf("error in realloc\n");
            exit(1);
        }
    }
    l->functions[l->function_ct] = def;
    l->function_ct++;
}

// parses a function of the form
// (def <bytesymbol> <function name> ...args...)
detailed_parse_error * parse_fn(function_def *f, language_def *l, swexp_list_node *node) {
    swexp_list_node *head = list_head(node);
    PARSE_ERROR e;

    // print_list(node);

    // check the first element is 'def'
    if (strcmp(head->content, "def") != 0) {
        return err(head, MISSING_DEF, "missing def in function decl");
    }

    // step over 'def' token onto bytecode value
    if (!(head = head->next) || head->type != ATOM)
        return err(head, MISSING_BINNAME, "no binname in function decl");

    // parse the function binary value
    int cont, shifted_cont, final_cont;
    e = parse_int(&cont, head->content);
    if (NO_ERROR != e) {
        // printf("malformed binname, err = %d on string '%s'\n",
        //         err, (char *) head->content);
        return err(head, MALFORMED_BINNAME, "malformed binary name for function");
    }

    final_cont = cont >> l->function_name_bitshift;
    shifted_cont = final_cont << l->function_name_bitshift;

    // check that the function binary value is representable
    // with the bitshift
    if (shifted_cont != cont) {
        // printf("precision in symbol '%s' lost in bitshift "
        //        "(0x%x ->0x%x)\n",
        //        (char*) head->content, cont, shifted_cont);
        return err(head, FUNCTION_BINNAME_PRECISION,
                "information lost in function name bit shift");
    }

    // check that the function binary value falls into the name width
    if (!check_size(UNSIGNED_INT, l->function_name_width, final_cont)) {
        // printf("function identifier '%s' does not fit in ",
        //         (char *) head->content);
        // printf("name width %d\n", l->function_name_width);
        return err(head, FUNCTION_BINNAME_SIZE,
                "function identifier does not fit in language's identifier size");
    }

    // step to the next atom and get the function name
    if (!(head = head->next) || head->type != ATOM)
        return err(list_head(node), MISSING_NAME, "no function text name specifid");

    f->name = malloc(sizeof(char) * (strlen(head->content) + 1));
    strcpy(f->name, head->content);

    ///////////////////////
    // Parsing Arguments //
    ///////////////////////

    // step to the first argument
    head = head->next;

    // allocate a pointer to an argument_def on the heap
    argument_def **arguments = malloc(sizeof(argument_def *));
    size_t argc = 0;

    for (; head != NULL; head = head->next) {
        arguments = realloc(arguments, sizeof(argument_def *) * (argc + 1));
        if (arguments == NULL) {
            perror("unrecoverable error when reallocing argument array\n");
            exit(1);
        }

        argument_def *argument = malloc(sizeof(argument_def));
        argument->name = NULL;

        arguments[argc] = argument;
        argc++;

        // printf("%d ARGUMENT\n", argc);

        if (head->type == ATOM) {
            // handle the case of a type without a name
            e = parse_argtype(argument, (char *)head->content);
            if (NO_ERROR != e) {
                free_sequence(arguments, argc);
                free(arguments);
                return err(head, e, "error parsing argument declaration");
            }
        } else if (head->type == LIST) {
            // handle the case of a type/name pair
            if (list_len(head) != 2) {
                // printf("tried to parse an argdef w/ !=2 elems\n");
                free_sequence(arguments, argc);
                free(arguments);
                return err(head, MALFORMED_ARGUMENT_DECLARATION, 
                        "argument declaration is not al ist of length 2");
            }

            swexp_list_node *lnode = list_head(head);
            e = parse_argtype(argument, (char *)lnode->content);
            if (NO_ERROR != e) {
                free_sequence(arguments, argc);
                free(arguments);
                free(f->name);
                return err(lnode, e, "errr parsing argument");
            }
            char *arg_name = (char *)lnode->next->content;
            argument->name = malloc((strlen(arg_name) + 1) * sizeof(char));
            strcpy(argument->name, arg_name);

            // printf("    %s\n", arg_name);
        }
    }

    // printf("argc: %d\n", argc);

    f->function_binary_value = cont >> l->function_name_bitshift;
    if (argc == 0) {
        free(arguments);
        arguments = NULL;
    }

    f->arguments = arguments;
    f->argc = argc;
    return NULL;
}

detailed_parse_error * parse_fn_call(function_call *call, language_def *l,
                          swexp_list_node *node) {
    if (node->type != LIST) {
        return err(node, MALFORMED_FUNCTION_DECL, 
                    "parse_fn_call called on non-list swexpr node\n");
    }
    swexp_list_node *head = list_head(node);
    swexp_list_node *function = head;
    print_list(head);

    char *name = (char *)head->content;
    head = head->next;

    function_def *fndef = lang_getfnbyname(l, name);

    if (fndef == NULL)
        return err (head, UNKNOWN_FUNCTION_NAME, "unknown function name");

    void **arguments = malloc(sizeof(void *) * fndef->argc);
    for (size_t i = 0; i < fndef->argc; i++) {
        if (fndef->arguments[i]->type != SKIP) {
            if (head == NULL) {
                for (size_t j = 0; j < i; j++) {
                    free(arguments[j]);
                }
                free(arguments);
                return err(function, MISSING_NAME, "no function name supplied");
            }

            PARSE_ERROR p = parse_arg(&arguments[i], fndef->arguments[i],
                                      (char *)head->content);

            if (p != NO_ERROR) {
                for (size_t j = 0; j < i; j++) {
                    free(arguments[j]);
                }
                free(arguments);
                return err(head, p, "error parsing argument");
            }

            head = head->next;

        } else {
            arguments[i] = NULL;
        }
    }

    if (head != NULL) {
        printf("leftover argument:\n");
        print_list(head);
        printf("\n");
        for (size_t j = 0; j < fndef->argc; j++) {
            if (arguments[j] != NULL) {
                free(arguments[j]);
            }
        }
        free(arguments);
        return err(head, LEFTOVER_ARG, "extra arguments passed to function");
    }

    // no error, set the fields
    call->args = arguments;
    call->defn = fndef;
    return NULL;
}

#define checkerrdata(a)                                                        \
    if (NO_ERROR != (err = a)) {                                               \
        free(data);                                                            \
        return err;                                                            \
    }

PARSE_ERROR parse_arg(void **result, argument_def *arg, char *str_repr) {
    void *data;
    PARSE_ERROR err;

    switch (arg->type) {
    case RAW_STRING:
    case RAW_BITSTRING:
        if (strlen(str_repr) > arg->bitwidth / 8)
            return DISALLOWED_SIZE;

        data = malloc((arg->bitwidth / 8 + 1) * sizeof(char));
        memcpy(data, str_repr, arg->bitwidth / 8);
        ((char *)data)[arg->bitwidth / 8] = '\0';
        break;
    case STRING:
        if (strlen(str_repr) > arg->bitwidth / 8)
            return DISALLOWED_SIZE;

        data = malloc(arg->bitwidth / 8 * sizeof(char));
        memcpy(data, str_repr, arg->bitwidth / 8);
        break;
    case UNSIGNED_INT:
        data = malloc(sizeof(long long int));
        unsigned int temp_uint;
        checkerrdata(parse_uint(&temp_uint, str_repr));
        *((long long *)data) = temp_uint;
        break;
    case INT:
        data = malloc(sizeof(long long int));
        int temp_int;
        checkerrdata(parse_int(&temp_int, str_repr));
        *((long long *)data) = temp_int;
        break;
    case FLOAT:
        data = malloc(sizeof(long double));
        sscanf(str_repr, "%Lf", (long double *)data);
        break;
    case SKIP:
        data = NULL;
        break;
    default:
        printf("unhandled argtype in parse_arg\n");
        exit(1);
    }
    *result = data;
    return NO_ERROR;
}

/// METADATA PARSING ///
PARSE_ERROR __parse_meta_endian(language_def *l, swexp_list_node *node) {
    char *value = (char *)list_head(node)->next->content;
    if (strcmp(value, "big") == 0 || strcmp(value, "Big") == 0 ||
        strcmp(value, "BIG") == 0) {
        l->target_endianness = BS_BIG_ENDIAN;
    } else if (strcmp(value, "little") == 0 || strcmp(value, "Little") == 0 ||
               strcmp(value, "LITTLE") == 0) {
        l->target_endianness = BS_LITTLE_ENDIAN;
    } else {
        // printf("Unrecognized value '%s' for endianness declaration\n",
        // value);
        // printf("only values big/Big/BIG/little/Little/LITTLE are valid\n");
        return MALFORMED_METADATA_ATTRIBUTE;
    }
    return NO_ERROR;
}

PARSE_ERROR __parse_meta_namewidth(language_def *l, swexp_list_node *node) {
    char *value = (char *)list_head(node)->next->content;
    PARSE_ERROR e = parse_uint(&(l->function_name_width), value);
    // printf("nameshift error %d\n", e);
    if (e != NO_ERROR)
        return MALFORMED_METADATA_ATTRIBUTE;
    return NO_ERROR;
}

PARSE_ERROR __parse_meta_nameshift(language_def *l, swexp_list_node *node) {
    char *value = (char *)list_head(node)->next->content;
    PARSE_ERROR e = parse_uint(&l->function_name_bitshift, value);
    // printf("namewidth error %d\n", e);
    if (e != NO_ERROR)
        return MALFORMED_METADATA_ATTRIBUTE;
    return NO_ERROR;
}

struct metadata_entry {
    char *name;
    size_t id;
    PARSE_ERROR (*parser)(language_def *l, swexp_list_node *node);
};

const struct metadata_entry metadata_entries[] = {
    { "endian", 0, *__parse_meta_endian },
    { "endianness", 0, *__parse_meta_endian },
    { "namewidth", 1, *__parse_meta_namewidth },
    { "nameshift", 2, *__parse_meta_nameshift }
};

#define NUM_META_ATTR sizeof(metadata_entries) / sizeof(struct metadata_entry)

detailed_parse_error * parse_metadata(language_def *l, swexp_list_node *metadata_decl) {
    swexp_list_node *node;
    long long encountered_bitmask = 0;

    for (node = list_head(metadata_decl)->next; node != NULL;
         node = node->next) {
        if (node->type != LIST) {
            // printf("encountered non-list '%s' in metadata declaration\n",
            //        (char *)node->content);
            return err(node, MALFORMED_METADATA_ATTRIBUTE,
                    "encountered key without value in metadata declaration");
        }

        // get the name of the current node
        swexp_list_node * curr_metadata = list_head(node);
        char *name = (char *)curr_metadata->content;

        bool matched = false;
        for (int i = 0; i < NUM_META_ATTR; i++) {
            if (0 == strcmp(name, metadata_entries[i].name)) {

                if (0 == (encountered_bitmask >> metadata_entries[i].id)) {
                    encountered_bitmask =
                        encountered_bitmask | (1 << metadata_entries[i].id);
                } else {
                    return err(curr_metadata, DUPLICATE_METADATA_ATTRIBUTE, "");
                }

                PARSE_ERROR p;
                p = metadata_entries[i].parser(l, node);
                if (p != NO_ERROR)
                    return err(curr_metadata, p, "error parsing metadata entry");

                matched = true;
                break;
            }
        }

        if (!matched)
            return err(curr_metadata, UNKNOWN_METADATA_ATTRIBUTE, "unknown metadata attr");
    }
    return NULL;
}

detailed_parse_error * parse_language(language_def *language, swexp_list_node *head) {
    // initialie the language to holding no funcions
    // and give attributes for metadata;
    lang_init(language);

    // parse a metadata block as the first block if it exists
    swexp_list_node *current = list_head(head);
    if (current != NULL && 0 == strcmp("meta", list_head(current)->content)) {

        detailed_parse_error * meta_error = parse_metadata(language, current);
        if (meta_error != NO_ERROR)
            return meta_error;

        current = current->next;
    }

    for (; current != NULL; current = current->next) {
        if (current->type == LIST) {
            char *content = list_head(current)->content;
            if (strcmp(content, "def") == 0) {
                // parse a function definition
                function_def *f = malloc(sizeof(function_def));
                detailed_parse_error * fn_parse_err = parse_fn(f, language, current);
                if (fn_parse_err != NULL) {
                    free(f);
                    return fn_parse_err;
                }
                add_fn_to_lang(language, f);
            } else if (strcmp(content, "meta") == 0) {
                // throw an error if we encounter a metadatablock
                // at the first block
                return err(current, MISPLACED_METADATA_BLOCK,
                        "encountered metadata block anywhere but the beginning of the file");
            } else {
                return err (current, UNKNOWN_ROOT,
                        "unrecognized root level command");
            }
        } else {
            return err(current, ATOM_AT_ROOT, "encountered atom at root level");
        }
    }
    return NULL;
}

detailed_parse_error * parse_language_from_file(language_def *language, FILE *f, const char * name) {
    swexp_list_node *nodes = parse_file_to_atoms(f, name, 255);
    detailed_parse_error * p = parse_language(language, nodes);
    free_list(nodes);
    return p;
}

detailed_parse_error * parse_language_from_str(language_def *language, char *c, const char * name) {
    swexp_list_node *nodes = parse_string_to_atoms(c, name, 255);
    detailed_parse_error * p = parse_language(language, nodes);
    // printf("lang parse error %d\n", p);
    free_list(nodes);
    return p;
}
