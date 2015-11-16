#include "parsescript.h"
#include "langdef.h"

int main(int argc, char ** argv) {
    FILE * f = fopen("example.langdef", "r");
    language_def * l = parse_language(f);

    print_lang(l);
}

