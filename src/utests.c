// vim: noet ts=4 sw=4
#include <assert.h>
#include <stdio.h>
#include <stdint.h>
#include <string.h>

#define DEBUG
#include "logging.h"
#include "utils.h"
#include "vector.h"

/* Taken from OlegDB */
#define run_test(test) log_msg(LOG_INFO, "----- %s -----", #test);\
	test_return_val = test();\
	if (test_return_val != 0) {\
		tests_failed++;\
		log_msg(LOG_ERR, "%c[%dmFailed.%c[%dm\n", 0x1B, 31, 0x1B, 0);\
	} else {\
		tests_run++;\
		log_msg(LOG_INFO, "%c[%dmPassed.%c[%dm\n", 0x1B, 32, 0x1B, 0);\
	}

int test_vector() {
	vector *vec = vector_new(sizeof(uint64_t), 2);

	uint64_t i;
	for (i = 0; i < 10000; i++)
		vector_append(vec, &i, sizeof(i));

	uint64_t total = 0;
	for (i = 0; i < vec->count; i++) {
		uint64_t *j = (uint64_t *)vector_get(vec, i);
		total += *j;
	}

	vector_free(vec);

	if (total != 49995000)
		return 1;

	return 0;
}

int main(int argc, char *argv[]) {
	UNUSED(argc);
	UNUSED(argv);
	log_msg(LOG_INFO, "Running tests.\n");

	int test_return_val = 0 ;
	int tests_run = 0;
	int tests_failed = 0;

	run_test(test_vector);

	log_msg(LOG_INFO, "Tests passed: (%i/%i).\n", tests_run, tests_run + tests_failed);

	return 0;
}
