#include <stdlib.h>
#include <string.h>

#include "parsescript.h"

int parse_int(char * str) {
    int toret;
    if (strncmp(str, "0x", 2) == 0) {
        sscanf(str+2, "%x", &toret);
    } else {
        sscanf(str, "%d", &toret);
    }
    return toret;
}

const char * typenames[] = {
    "raw_str",
    "str",
    "int",
    "float",
    "skip"
};

void  get_argument_type(argument_def * argument,
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
    // TODO check the size of the byte falls into the provided size
    head = head->next;
    f->function_binary_value = 
        parse_int(head->content);
    
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
            get_argument_type(argument, head);
        } else if (head->type == LIST) {
            // handle the case of a type/name pair
            if(list_len(head) != 2) {
                printf("tried to parse an argdef w/ !=2 elems\n");
                exit(1);
            }

            swexp_list_node * lnode = list_head(head);
            get_argument_type(argument, lnode);
            argument->name = lnode->next->content;
        }
    }

    return f;
}

language_def * define_language(FILE * f) {
    swexp_list_node * head =  parse_file_to_atoms(f, 255), *current;

    language_def * language = malloc(sizeof(language_def));
    language->function_ct = 0;
    language->function_capacity = 0;
    language->functions = NULL;


    for(current = list_head(head); current != NULL;
        current = current->next) {
        if (current->type == LIST) {
            char * content = list_head(current)->content;
            if(strcmp(content, "def") == 0) {
                // parse a function definition
                add_fn_to_lang(language, parse_fn(language, current));
            } else if (strcmp(content, "meta") == 0) {
                // parse metadata block
                // parse_metadata(language, current);
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

char * type_name(arg_type t) {
    switch(t) {
        case RAW_STRING:    return "rawstr";
        case STRING:        return "str";
        case INT:           return "int";
        case FLOAT:         return "float";
        case SKIP:          return "skip";
        default:            return "??";
    }
}

void print_lang(language_def * l) {
    for (unsigned int i=0; i < l->function_ct; i++) {
        function_def * f = l->functions[i];
        printf("    0x%x %s: ", 
                f->function_binary_value,
                f->name);
        for (unsigned int j=0; j < f->argc; j++) {
            argument_def * a = f->arguments[j];
            printf("<%s:%d %s> ",
                    type_name(a->type),
                    a->bitwidth,
                    a->name);
        }
        printf("\n");
    }
}
