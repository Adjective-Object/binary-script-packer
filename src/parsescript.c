#include <stdlib.h>
#include <string.h>

#include "parsescript.h"
#include "langdef.h"
#include "util.h"

int scan_binary(int * out, char * str) {
    int val = 0;
    for (;*str != '\0'; str++) {
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

PARSE_ERROR parse_int(int * out, char * str) {
    // get the leading '-' for negatives
    int sign = 1;
    if (*str == '-') {
        sign = -1;
        str++;
    }

    // try hex, then binary, then decimal
    if (strncmp(str, "0x", 2) == 0) {
        if (0 == sscanf(str+2, "%x", out))
            return BAD_HEX_FORMAT;
    } else if (strncmp(str, "0b", 2) == 0) {
        if (scan_binary(out, str+2))
            return BAD_BINARY_FORMAT;
    } else {
        // sscanf doesn't check character ranges on %d for
        // some reason so we check manually
        for (char * s = str; *s!= '\0'; s++) {
            if (((*s - '0') < 0 ||
                    (*s - '0') > 9) &&
                    *s != '.') {
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

int uparse_int(char * str) {
    int out;
    PARSE_ERROR err = parse_int(&out, str);
    switch(err) {
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
            printf("unknown error in parsing int (errcode = %d)\n",
                    err);
            exit(1);
    }
}

PARSE_ERROR parse_argtype(argument_def * argument, char * name) {
    arg_type argtype;
    int argwidth;

    unsigned int i;
    for (i=0; i<__ARG_TYPE_CT; i++) {
        if (strncmp(
                name, 
                typenames[i],
                strlen(typenames[i])) == 0) {
            argtype = (arg_type) i;
            break;
        }
    }
    
    // if no argument type was found, return an error
    if (i == __ARG_TYPE_CT) return UNKNOWN_ARGTYPE;

    // if there is no size parameter, return an error
    if (strlen(typenames[i]) == strlen(name))
        return UNSPECIFIED_SIZE;
    
    // if an integer was not parsed sucessfully, return an error
    PARSE_ERROR int_error = 
        parse_int(&argwidth,
                name + strlen(typenames[i]));

    if (int_error != NO_ERROR) return int_error;
    
    // check that the argument is defined with valid size
    if (!validate_size(argtype, argwidth))
        return DISALLOWED_SIZE;
 
    // if both properties were scanned sucessfully,
    // check the validity of the argument types
    argument->bitwidth = argwidth;
    argument->type = argtype;



    return NO_ERROR;
}

void add_fn_to_lang(language_def *l, function_def * def) {
    if (l->function_ct + 1 >= l->function_capacity) {
        l->functions = realloc(l->functions, 
                sizeof(function_def *) * (l->function_ct + 1));
    }

    function_def * old_def;
    if ((old_def = lang_getfn(l, def->function_binary_value)) != NULL) {
        printf("tried to add overlapping function definition to language\n");
        printf("previous function:\n    ");
        print_fn(l, old_def);
        printf("new function:\n    ");
        print_fn(l, def);
        exit(1);
    }
    l->functions[l->function_ct] = def;
    l->function_ct ++;
}
   
// parses a function of the form
// (def <bytesymbol> <function name> ...args...)
PARSE_ERROR parse_fn(
        function_def * f, language_def * l, swexp_list_node * node) {
    swexp_list_node * head = list_head(node);
    PARSE_ERROR err;
    // TODO checks on advancing head

    // check the first element is 'def'
    if (strcmp(head->content, "def") != 0) {
        // printf("parse_fn called on non-function object\n");
        // print_list(head);
        return MISSING_DEF;
    }

    // step over 'def' token onto bytecode value
    head = head->next;
    if (head->type != ATOM) {
        // printf("first argument to def is not an atom\n");
        return MISSING_BINNAME;
    }

    // parse the function binary value
    int cont, parsed_cont;
    if ((err = parse_int(&cont, head->content))) {
        printf("malformed binname\n");
        return MALFORMED_BINNAME;
    }
    parsed_cont = (cont >> l->function_name_bitshift) 
            << l->function_name_bitshift;
    f->function_binary_value = cont >> l->function_name_bitshift;

    // check that the function binary value is representable
    // with the bitshift
    if (parsed_cont != cont) {
        // printf("precision in symbol '%s' lost in bitshift (0x%x ->0x%x)\n",
        //        (char*) head->content, cont, parsed_cont);
        return FUNCTION_BINNAME_PRECISION;
    }
    
    // check that the function binary value falls into the name width
    if (!check_size(UNSIGNED_INT, l->function_name_width, 
                f->function_binary_value)) {
        // printf("function identifier '%s' does not fit in ", 
        //         (char *) head->content);
        // printf("name width %d\n", l->function_name_width);
        return FUNCTION_BINNAME_SIZE;
    }
  
    // step to the next atom and get the function name
    head = head->next;
    if (head->type != ATOM) {
        return MISSING_NAME;
    }

    f->name = malloc(sizeof(char) * (strlen(head->content) + 1));
    strcpy(f->name, head->content);

    // step to the first argument
    head = head->next;

    // allocate a pointer to an argument_def on the heap
    f->arguments = malloc(sizeof(argument_def *));
    f->argc = 0;

    for (; head != NULL; head = head->next) {
        f->arguments = realloc(f->arguments,
                sizeof(argument_def *) * (f->argc + 1));
        argument_def * argument = malloc(sizeof(argument_def));
        argument->name = NULL;
        
        f->arguments[f->argc] = argument;
        f->argc++;
        
        if (head->type == ATOM) {
            // handle the case of a type without a name
            if ((err = parse_argtype(argument, (char *) head->content))) {
                free_sequence(f->arguments, f->argc-1);
                free(f->arguments);
                return err;
            }
        } else if (head->type == LIST) {
            // handle the case of a type/name pair
            if(list_len(head) != 2) {
                printf("tried to parse an argdef w/ !=2 elems\n");
                free_sequence(f->arguments, f->argc - 1);
                free(f->arguments);
                return MALFORMED_ARGUMENT_DECLARATION;
            }

            swexp_list_node * lnode = list_head(head);
            if ((err = parse_argtype(argument, (char *) lnode->content))) {
                free_sequence(f->arguments, f->argc - 1);
                free(f->arguments);
                return err;
            }
            char * arg_name = (char *) lnode->next->content;
            argument->name = malloc((strlen(arg_name) + 1) * sizeof(char));
            strcpy(argument->name, arg_name);
        }
    }

    return NO_ERROR;
}

void parse_metadata_attr(language_def * l, swexp_list_node * node) {
    char * name = list_head(node)->content;
    char * value = list_head(node)->next->content;
    if (strcmp(name, "endian") == 0
        || strcmp(name, "endianness") == 0) {
       if(strcmp(value, "big") == 0
            || strcmp(value, "Big") == 0
            || strcmp(value, "BIG") == 0) {
           l->target_endianness = BIG_ENDIAN;
       } else if(strcmp(value, "little") == 0
            || strcmp(value, "Little") == 0
            || strcmp(value, "LITTLE") == 0) {
           l->target_endianness = LITTLE_ENDIAN;
       } else {
           printf("Unrecognized value %s for endianness declaration\n",
                   value);
           printf("only values big/Big/BIG/little/Little/LITTLE are valid");
       }
    }
    else if (strcmp(name, "namewidth") == 0
        || strcmp(name, "functionwidth") == 0
        || strcmp(name, "funcwidth") == 0) {
        l->function_name_width = uparse_int(value);
    }
    else if (strcmp(name, "nameshift") == 0) {
        l->function_name_bitshift = uparse_int(value);
    }
    else {
        printf("unrecgonized metadata attr '%s'", name);
        exit(1);
    }
}

void parse_metadata(language_def *l, swexp_list_node * metadata_decl) {
    swexp_list_node * node; 
    for (node = list_head(metadata_decl)->next; 
            node != NULL; node=node->next) {
        if (node->type != LIST) {
            printf("encountered non-list '%s' in metadata declaration\n",
                    (char *) node->content);
            exit(1);
        }
        parse_metadata_attr(l, node);
    }
}

void parse_language(language_def * language, FILE * f) {
    // parse the file to a list of atoms
    swexp_list_node * head = parse_file_to_atoms(f, 255), *current;

    // initialie the language to holding no funcions
    language->function_ct = 0;
    language->function_capacity = 0;
    language->functions = NULL;

    for(current = list_head(head); current != NULL;
        current = current->next) {
        if (current->type == LIST) {
            char * content = list_head(current)->content;
            if(strcmp(content, "def") == 0) {
                // parse a function definition
                function_def * f = malloc(sizeof(function_def));
                parse_fn(f, language, current);
                add_fn_to_lang(language, f);
            } else if (strcmp(content, "meta") == 0) {
                // parse metadata block
                parse_metadata(language, current);
            } else {
                printf("unrecognized root level command '%s'\n",
                        content);
                exit(1);
            }
        } else {
            printf("Error reading language definition:\n");
            printf("  encountered atom at root level\n");
            exit(1);
        }
    }

    // free the contents of the list
    free_list(head);
} 

