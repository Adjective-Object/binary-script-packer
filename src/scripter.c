#include "parsescript.h"

int main(int argc, char ** argv) {
    FILE * f = fopen("example.langdef", "r");
    language_def * l = define_language(f);

    print_lang(l);
}

