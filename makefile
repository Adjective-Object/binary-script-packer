##############################
# BUILDING THE PARSER EXAMPLE #
###############################

program = scripter
test = scripter_tests

lib_src = src/parsescript.c \
	  src/langdef.c src/translator.c \
	  src/util.c src/bitbuffer.c
program_src = src/main.c
mutest_src = tests/mutest.c
test_src = tests/suites/bitbuffer_test.c \
		   tests/suites/util_test.c \
		   tests/suites/parsescript_test.c

lib_obj = $(lib_src:.c=.o)
program_obj = $(program_src:.c=.o)
mutest_obj = $(mutest_src:.c=.o)
test_obj = $(test_src:.c=.o)

CFLAGS = -g -Wall -std=c99 -Isrc -Ilibsweetparse/src -D_POSIX_SOURCE
LDFLAGS = -L./libsweetparse -lsweetexpressions -lm

all: $(program) $(test)

%.o: %.c
	gcc $(CFLAGS) -c $^ -o $@

$(program): $(program_obj) $(lib_obj)
	gcc $(LDFLAGS) $^ -o $@

#########
# TESTS #
#########

# runs the tests
test: $(test)
	LD_LIBRARY_PATH="$$LD_LIBRARY_PATH:./libsweetparse" valgrind --quiet --leak-check=full --trace-children=yes \
		./$(test) -vv

# build the test executable
$(test): $(mutest_obj) $(test_obj) $(lib_obj) tests/suite_runner.o
	gcc $(LDFLAGS) $^ -o $@

# build the suite_runner thing
tests/suite_runner.c: $(test_obj)
	tests/mkmutest mutest.h $(test_obj) > $@

# format code
format: $(lib_src) $(program_src)
	clang-format -i $^

# lint code
lint: lint_src lint_tests

lint_%: $(lib_src) $(program_src)
	cppcheck \
		--std=c99 \
		--enable=warning \
		--enable=missingInclude \
		--enable=performance \
		-I ./src -I ./libsweetparse/src $*

################
# MISC UTILITY #
################

# clean up the example and lib thigs
clean:
	rm -f $(lib_objs) $(program_obj) $(test_obj) $(mutest_obj) \
		$(obj) $(program) $(test) tests/suite_runner.c

# debug helper to print makefile variables
print-%:
	@echo '$*=$($*)'

