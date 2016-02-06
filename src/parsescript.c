#include <stdlib.h>
#include <string.h>

#include "parsescript.h"
#include "langdef.h"
#include "util.h"

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

PARSE_ERROR parse_uint(unsigned int *i, char * str) {
    int temp;
    PARSE_ERROR err = parse_int(&temp, str);
    if (err != NO_ERROR) return err;
    if (temp < 0) return ILLEGAL_SIGN;
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
        l->functions = realloc(l->functions, sizeof(function_def *) * (l->function_ct + 1));
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
PARSE_ERROR parse_fn(function_def *f, language_def *l, swexp_list_node *node) {
    swexp_list_node *head = list_head(node);
    PARSE_ERROR err;

    // check the first element is 'def'
    if (strcmp(head->content, "def") != 0) {
        // printf("parse_fn called on non-function object\n");
        // print_list(head);
        return MISSING_DEF;
    }

    // step over 'def' token onto bytecode value
    if (!(head = head->next) || head->type != ATOM)
        return MISSING_BINNAME;

    // parse the function binary value
    int cont, shifted_cont, final_cont;
    if ((err = parse_int(&cont, head->content))) {
        // printf("malformed binname, err = %d on string '%s'\n",
        //         err, (char *) head->content);
        return MALFORMED_BINNAME;
    }

    final_cont = cont >> l->function_name_bitshift;
    shifted_cont = final_cont << l->function_name_bitshift;

    // check that the function binary value is representable
    // with the bitshift
    if (shifted_cont != cont) {
        // printf("precision in symbol '%s' lost in bitshift "
        //        "(0x%x ->0x%x)\n",
        //        (char*) head->content, cont, shifted_cont);
        return FUNCTION_BINNAME_PRECISION;
    }

    // check that the function binary value falls into the name width
    if (!check_size(UNSIGNED_INT, l->function_name_width, final_cont)) {
        // printf("function identifier '%s' does not fit in ",
        //         (char *) head->content);
        // printf("name width %d\n", l->function_name_width);
        return FUNCTION_BINNAME_SIZE;
    }

    // step to the next atom and get the function name
    if (!(head = head->next) || head->type != ATOM)
        return MISSING_NAME;

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

        if (head->type == ATOM) {
            // handle the case of a type without a name
            if ((err = parse_argtype(argument, (char *)head->content))) {
                free_sequence(arguments, argc - 1);
                free(arguments);
                return err;
            }
        } else if (head->type == LIST) {
            // handle the case of a type/name pair
            if (list_len(head) != 2) {
                printf("tried to parse an argdef w/ !=2 elems\n");
                free_sequence(arguments, argc - 1);
                free(arguments);
                return MALFORMED_ARGUMENT_DECLARATION;
            }

            swexp_list_node *lnode = list_head(head);
            if ((err = parse_argtype(argument, (char *)lnode->content))) {
                free_sequence(arguments, argc - 1);
                free(arguments);
                return err;
            }
            char *arg_name = (char *)lnode->next->content;
            argument->name = malloc((strlen(arg_name) + 1) * sizeof(char));
            strcpy(argument->name, arg_name);
        }
    }

    f->function_binary_value = cont >> l->function_name_bitshift;
    if (argc == 0) {
        free(arguments);
        arguments = NULL;
    }

    f->arguments = arguments;
    f->argc = argc;
    return NO_ERROR;
}

/// METADATA PARSING ///
PARSE_ERROR __parse_meta_endian(language_def *l, swexp_list_node *node) {
    char *value = (char *) list_head(node)->next->content;
    if (strcmp(value, "big") == 0 || strcmp(value, "Big") == 0 ||
        strcmp(value, "BIG") == 0) {
        l->target_endianness = BIG_ENDIAN;
    } else if (strcmp(value, "little") == 0 ||
               strcmp(value, "Little") == 0 ||
               strcmp(value, "LITTLE") == 0) {
        l->target_endianness = LITTLE_ENDIAN;
    } else {
        // printf("Unrecognized value '%s' for endianness declaration\n", value);
        // printf("only values big/Big/BIG/little/Little/LITTLE are valid\n");
        return MALFORMED_METADATA_ATTRIBUTE;
    }
    return NO_ERROR;
}

PARSE_ERROR __parse_meta_namewidth(language_def *l, swexp_list_node *node) {
    char *value = (char *) list_head(node)->next->content;
    PARSE_ERROR e = parse_uint(&(l->function_name_width), value);
    // printf("nameshift error %d\n", e);
    if (e != NO_ERROR) return MALFORMED_METADATA_ATTRIBUTE;
    return NO_ERROR;
}

PARSE_ERROR __parse_meta_nameshift(language_def *l, swexp_list_node *node) {
    char *value = (char *) list_head(node)->next->content;
    PARSE_ERROR e = parse_uint(&l->function_name_bitshift, value);
    // printf("namewidth error %d\n", e);
    if (e != NO_ERROR) return MALFORMED_METADATA_ATTRIBUTE;
    return NO_ERROR;
}


struct metadata_entry {
    char * name;
    size_t id;
    PARSE_ERROR (*parser)(language_def *l, swexp_list_node *node);
};

const struct metadata_entry metadata_entries[] = {
    {"endian",     0, *__parse_meta_endian},
    {"endianness", 0, *__parse_meta_endian},
    {"namewidth",  1, *__parse_meta_namewidth},
    {"nameshift",  2, *__parse_meta_nameshift}
};
#define NUM_META_ATTR sizeof(metadata_entries) \
    / sizeof(struct metadata_entry)

PARSE_ERROR parse_metadata(language_def *l, swexp_list_node *metadata_decl) {
    swexp_list_node *node;
    long long encountered_bitmask = 0;

    for (node = list_head(metadata_decl)->next; 
         node != NULL;
         node = node->next) {
        if (node->type != LIST) {
            // printf("encountered non-list '%s' in metadata declaration\n",
            //        (char *)node->content);
            return MALFORMED_METADATA_ATTRIBUTE;
        }

        //get the name of the current node
        char * name = (char *) list_head(node)->content;
       
        bool matched = false; 
        for (int i=0; i<NUM_META_ATTR; i++) {
            if (0 == strcmp(name, metadata_entries[i].name)) {

                if (0 == (encountered_bitmask >> metadata_entries[i].id)) {
                    encountered_bitmask = 
                        encountered_bitmask | (1 << metadata_entries[i].id);
                } else {
                    return DUPLICATE_METADATA_ATTRIBUTE;
                }

                PARSE_ERROR p;
                p = metadata_entries[i].parser(l, node);
                if (p != NO_ERROR) return p;

                matched = true;
                break;
            }
        }

        if (!matched) return UNKNOWN_METADATA_ATTRIBUTE;

    }
    return NO_ERROR;
}

PARSE_ERROR parse_language(language_def *language, swexp_list_node *head) {
    // initialie the language to holding no funcions
    // and give attributes for metadata;
    lang_init(language);

    // parse a metadata block as the first block if it exists
    swexp_list_node * current = list_head(head);
    if (current != NULL && 
            0 == strcmp("meta", list_head(current)->content)) {
        
        PARSE_ERROR meta_error = parse_metadata(language, current);
        if (meta_error != NO_ERROR) return meta_error;
        
        current = current->next;
    }

    for (; current != NULL;
         current = current->next) {
        if (current->type == LIST)  {
            char *content = list_head(current)->content;
            if (strcmp(content, "def") == 0) {
                // parse a function definition
                function_def *f = malloc(sizeof(function_def));
                parse_fn(f, language, current);
                add_fn_to_lang(language, f);
            }
            else if (strcmp(content, "meta") == 0) {
                // throw an error if we encounter a metadatablock
                // at the first block
                return MISPLACED_METADATA_BLOCK;
            } else {
                printf("unrecognized root level command '%s'\n", content);
                exit(1);
            }
        } else {
            printf("Error reading language definition:\n");
            printf("  encountered atom at root level\n");
            exit(1);
        }
    }
    return NO_ERROR;
}

PARSE_ERROR parse_language_from_file(language_def *language, FILE *f) {
    swexp_list_node *nodes = parse_file_to_atoms(f, 255);
    PARSE_ERROR p = parse_language(language, nodes);
    free_list(nodes);
    return p;
}

PARSE_ERROR parse_language_from_str(language_def *language, char *c) {
    swexp_list_node *nodes = parse_string_to_atoms(c, 255);
    PARSE_ERROR p = parse_language(language, nodes);
    // printf("lang parse error %d\n", p);
    free_list(nodes);
    return p;
}
