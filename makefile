##############################
# BUILDING THE PARSER EXAMPLE #
###############################

all: clitool

program = scripter
src = src/main.c src/parsescript.c \
	  src/langdef.c src/translator.c \
	  src/util.c src/bitbuffer.c

obj = $(src:.c=.o)
CFLAGS = -g -Wall -std=c99 -Isrc
LDFLAGS = -lsweetexpressions -lm

clitool: $(program)

%.o: %.c
	gcc $(CFLAGS) -c $^ -o $@

$(program): $(obj)
	gcc $(LDFLAGS) $^ -o $@

################
# MISC UTILITY #
################

# clean up the example and lib thigs
clean:
	rm -f $(libobjs) $(lib) $(obj) $(program)

# debug helper to print makefile variables
print-%:
	@echo '$*=$($*)'

###########
# TESTING #
###########

.PHONY:  tests/*.input
.SILENT: tests/*.input
test: tests/*.input

tests/*.swexp.input:
	$(program) $@ | diff - $(@:.swexp.input=.swexp.output) 
	if [ $$? = 0 ]; then echo "test '$@' passed"; else echo "test failed"; fi


