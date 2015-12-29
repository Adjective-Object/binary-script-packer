/*
 * This file is part of mutest, a simple micro unit testing framework for C.
 *
 * mutest was written by Leandro Lucarella <llucax@gmail.com> and is released
 * under the BOLA license, please see the LICENSE file or visit:
 * http://blitiri.com.ar/p/bola/
 *
 * This is the factorial module test suite. Each (public) function starting
 * with mu_test will be picked up by mkmutest as a test case.
 *
 * Please, read the README file for more details.
 */

#include "../mutest.h"
#include <stdio.h>

void mu_test_sample() {
	mu_check(1 == 1);
}

