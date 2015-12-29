#include "mutest.h"
void mu_run_suites() {

	do {
		mutest_suite_name = "sample_test";
		mu_print(MU_SUITE, "\nRunning suite 'sample_test'\n");
		mu_run_case(mu_test_sample);
		if (mutest_suite_failed) ++mutest_failed_suites;
		else                     ++mutest_passed_suites;
		mutest_suite_failed = 0;
	} while (0);

}
