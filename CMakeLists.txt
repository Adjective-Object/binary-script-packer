cmake_minimum_required (VERSION 3.4)

project (binscripter)
set (binscripter_VERSION_MAJOR 0)
set (binscripter_VERSION_MINOR 0)

# add libsweetparse to search path
include_directories(AFTER ./src)
include_directories(AFTER ./libsweetparse/src)
link_directories(AFTER ./libsweetparse)

set(CMAKE_C_FLAGS "-g -Wall -std=c99 -Isrc -Ilibsweetparse/src -D_POSIX_SOURCE")

# project source library
add_library(ScripterLib OBJECT
			src/bitbuffer.c src/bitbuffer.h
			src/util.c src/util.h
			src/langdef.c src/langdef.h
			src/parsescript.c src/parsescript.h
			src/translator.c src/translator.h)


# add the tests library
add_library(ScripterTests OBJECT
    tests/mutest.h
    tests/mutest.c
    tests/suites/bitbuffer_test.c
    tests/suites/util_test.c
    tests/suites/parsescript_test.c
    tests/suites/translate_test.c)


# automatically generate the suite runners
# and also do other stuff
add_custom_command(
    OUTPUT tests/suite_runner.c
    COMMAND ${CMAKE_CURRENT_SOURCE_DIR}/test/mkmutest
    DEPENDS tests/suites/*)


# add the executables
add_executable (scripter src/main.c $<TARGET_OBJECTS:ScripterLib>)
target_link_libraries(scripter sweetexpressions m)

add_executable (scripter_tests
    tests/suite_runner.c
    $<TARGET_OBJECTS:ScripterLib> 
    $<TARGET_OBJECTS:ScripterTests>)
target_link_libraries(scripter_tests sweetexpressions m)

# Test Commands
add_custom_target(run_tests 
    COMMAND valgrind --quiet --leak-check=full --trace-children=yes ./scripter_tests -vv
    WORKING_DIRECTORY .
    DEPENDS scripter)

add_custom_target(run_tests_unsafe
    COMMAND ./scripter_tests -vv
    WORKING_DIRECTORY .
    DEPENDS scripter_tests)


