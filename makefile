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
test_src = tests/suites/sample_test.c

lib_obj = $(lib_src:.c=.o)
program_obj = $(program_src:.c=.o)
mutest_obj = $(mutest_src:.c=.o)
test_obj = $(test_src:.c=.o)

CFLAGS = -g -Wall -std=c99 -Isrc -D_POSIX_SOURCE
LDFLAGS = -lsweetexpressions -lm

all: $(program) $(test)

%.o: %.c
	gcc $(CFLAGS) -c $^ -o $@

$(program): $(program_obj) $(lib_obj)
	gcc $(LDFLAGS) $^ -o $@

#########
# TESTS #
#########

$(test): $(mutest_obj) $(test_obj) $(lib_obj) tests/suite_runner.o
	gcc $(LDFLAGS) $^ -o $@

test: $(test)
	./$(test) -vv

tests/suite_runner.c: $(test_obj)
	tests/mkmutest mutest.h $(test_obj) > $@

################
# MISC UTILITY #
################

# clean up the example and lib thigs
clean:
	rm -f $(lib_objs) $(program_obj) $(test_obj) $(obj) $(program) \
		$(test) tests/suite_runner.c

# debug helper to print makefile variables
print-%:
	@echo '$*=$($*)'

