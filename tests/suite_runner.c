#include "mutest.h"
void mu_run_suites() {

	do {
		mutest_suite_name = "bitbuffer_test";
		mu_print(MU_SUITE, "\nRunning suite 'bitbuffer_test'\n");
		mu_run_case(mu_test_bitbuffer_advance_sequential);
		mu_run_case(mu_test_bitbuffer_init_free_heap);
		mu_run_case(mu_test_bitbuffer_init_free_stack);
		mu_run_case(mu_test_bitbuffer_next);
		mu_run_case(mu_test_bitbuffer_pop);
		mu_run_case(mu_test_bitbuffer_writebit);
		mu_run_case(mu_test_bitbuffer_writeblock);
		if (mutest_suite_failed) ++mutest_failed_suites;
		else                     ++mutest_passed_suites;
		mutest_suite_failed = 0;
	} while (0);

	do {
		mutest_suite_name = "util_test";
		mu_print(MU_SUITE, "\nRunning suite 'util_test'\n");
		mu_run_case(mu_test_bits2bytes);
		mu_run_case(mu_test_memcmp_bits);
		mu_run_case(mu_test_swap_endian_on_field);
		if (mutest_suite_failed) ++mutest_failed_suites;
		else                     ++mutest_passed_suites;
		mutest_suite_failed = 0;
	} while (0);

}
