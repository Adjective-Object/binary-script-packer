cmake_minimum_required (VERSION 3.4)

project (binscripter)
set (binscripter_VERSION_MAJOR 0)
set (binscripter_VERSION_MINOR 0)
set(CMAKE_EXPORT_COMPILE_COMMANDS 1)

# add libsweetparse to search path
include_directories(AFTER ./src)
include_directories(AFTER ./libsweetparse/src)
link_directories(AFTER ./libsweetparse)

set(CMAKE_C_FLAGS "-g -fPIC -Wall -D_POSIX_SOURCE -std=c11")

# project source library
set(SCRIPTERLIB_SRCS
			src/bitbuffer.c src/bitbuffer.h
			src/util.c src/util.h
			src/langdef.c src/langdef.h
			src/parsescript.c src/parsescript.h
			src/translator.c src/translator.h)
add_library(ScripterLib OBJECT ${SCRIPTERLIB_SRCS})

# add the tests library
set(TESTSUITE_SRCS 
    tests/suites/bitbuffer_test.c
    tests/suites/util_test.c
    tests/suites/parsescript_test.c
    tests/suites/translate_test.c)
add_library(ScripterTestSuites OBJECT ${TESTSUITE_SRCS})



# automatically generate the suite runner main function
# add TESTSUITE_SRCS to to the dependencies instead of
# to force a rebuild because of a known bug where
# using OBJECT libraries as a dependency does not work
add_custom_command(
    OUTPUT tests/suite_runner.c
    # this is a hack because cmake sucks
    COMMAND ${CMAKE_CURRENT_SOURCE_DIR}/tests/mkmutest 
    ARGS mutest.h
        ${CMAKE_CURRENT_BINARY_DIR}${CMAKE_FILES_DIRECTORY}/ScripterTestSuites.dir/tests/suites/*.o
        > suite_runner.c
    WORKING_DIRECTORY tests
    DEPENDS ScripterTestSuites ${TESTSUITE_SRCS})



# add the executables
add_executable (scripter src/main.c $<TARGET_OBJECTS:ScripterLib>)
target_link_libraries(scripter sweetexpressions m)

add_executable (scripter_tests
    tests/suite_runner.c
    tests/mutest.h
    tests/mutest.c
    $<TARGET_OBJECTS:ScripterLib> 
    $<TARGET_OBJECTS:ScripterTestSuites>)
target_link_libraries(scripter_tests sweetexpressions m)

add_library(binscript-shared SHARED $<TARGET_OBJECTS:ScripterLib>)
set_target_properties(binscript-shared PROPERTIES OUTPUT_NAME "binscript")
add_library(binscript-static STATIC $<TARGET_OBJECTS:ScripterLib>)
set_target_properties(binscript-static PROPERTIES OUTPUT_NAME "binscript")



# Test Commands
add_custom_target(run_tests 
    COMMAND valgrind --quiet --leak-check=full --trace-children=yes ./scripter_tests -vv
    WORKING_DIRECTORY .
    DEPENDS scripter_tests)

add_custom_target(run_tests_unsafe
    COMMAND ./scripter_tests -vv
    WORKING_DIRECTORY .
    DEPENDS scripter_tests)

# Linting commands
add_custom_target(lint
    COMMAND cppcheck 
        --std=c11
        --enable=warning
        --enable=missingInclude
        --enable=performance
        -I ./src -I ./libsweetparse/src -I ./tests
        ${TESTSUITE_SRCS} ${SCRIPTERLIB_SRCS})

# format command
add_custom_target(format
    COMMAND clang-format -i ${TESTSUITE_SRCS} ${SCRIPTERLIB_SRCS})
    

