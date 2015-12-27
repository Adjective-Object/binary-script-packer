#include <stdlib.h>
#include <string.h>

#include "parsescript.h"
#include "langdef.h"

int parse_int(char * str) {
    int toret;
    if (strncmp(str, "0x", 2) == 0) {
        sscanf(str+2, "%x", &toret);
    } else {
        sscanf(str, "%d", &toret);
    }
    return toret;
}

void  parse_argtype(argument_def * argument,
        swexp_list_node * node) {
    unsigned int i;
    char * name = (char *) node->content;
    for (i=0; i<__ARG_TYPE_CT; i++) {
        if (strncmp(
                name, 
                typenames[i],
                strlen(typenames[i])) == 0) {
            argument->type = (arg_type) i;
            goto arg_len;
        }
    }
    
    printf("unknown argument type '%s'\n", (char *) node->content);
    exit(1);

    arg_len:
    if (strlen(typenames[i]) == strlen(name)) {
        printf("unspecified argument bit width in '%s'\n", name);
    } else {
        argument->bitwidth = parse_int(name + strlen(typenames[i]));
    }
    return;
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
function_def * parse_fn(language_def * l, swexp_list_node * node) {
    swexp_list_node * head = list_head(node);
    function_def * f = malloc(sizeof(function_def));

    // check the first element is 'def'
    if (strcmp(head->content, "def") != 0) {
        printf("parse_fn called on non-functoin object\n");
        exit(1);
    }

    // bytecode value
    head = head->next;
    if (head->type != ATOM) {
        printf("first argument to function def is not an atom\n");
        exit(1);
    }
    unsigned int cont = parse_int(head->content),
        parsed_cont = (cont >> l->function_name_bitshift) 
            << l->function_name_bitshift;
    f->function_binary_value = cont >> l->function_name_bitshift;
    if (parsed_cont != cont) {
        printf("precision in symbol '%s' lost in bitshift (0x%x ->0x%x)\n",
               (char*) head->content, cont, parsed_cont);
        exit(1);
    }
    
    if (!check_size(UNSIGNED_INT, l->function_name_width, 
                f->function_binary_value)) {
        printf("function identifier '%s' does not fit in ", 
                (char *) head->content);
        printf("name width %d\n", l->function_name_width);
        exit(1);
    }

    
    // get the name out of the node
    head = head->next;
    f->name = head->content;

    head = head->next;

    f->argc = 0;
    f->arguments = malloc(sizeof(argument_def *));

    for (; head != NULL; head = head->next) {
        f->arguments = realloc(f->arguments,
                sizeof(argument_def *) * (f->argc + 1));
        argument_def * argument = malloc(sizeof(argument_def));
        argument->name = NULL;
        
        f->arguments[f->argc] = argument;
        f->argc++;
        
        if (head->type == ATOM) {
            // handle the case of a type without a name
            parse_argtype(argument, head);
        } else if (head->type == LIST) {
            // handle the case of a type/name pair
            if(list_len(head) != 2) {
                printf("tried to parse an argdef w/ !=2 elems\n");
                exit(1);
            }

            swexp_list_node * lnode = list_head(head);
            parse_argtype(argument, lnode);
            argument->name = lnode->next->content;
        }
    }

    return f;
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
        l->function_name_width = parse_int(value);
    }
    else if (strcmp(name, "nameshift") == 0) {
        l->function_name_bitshift = parse_int(value);
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

language_def * parse_language(FILE * f) {
    swexp_list_node * head =  parse_file_to_atoms(f, 255), *current;

    language_def * language = malloc(sizeof(language_def));
    language->function_ct = 0;
    language->function_capacity = 0;
    language->functions = NULL;


    // TODO prescan for metadata blocks
    for(current = list_head(head); current != NULL;
        current = current->next) {
        if (current->type == LIST) {
            char * content = list_head(current)->content;
            if(strcmp(content, "def") == 0) {
                // parse a function definition
                add_fn_to_lang(language, parse_fn(language, current));
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

    return language;
} 

