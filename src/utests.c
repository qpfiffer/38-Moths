// vim: noet ts=4 sw=4
#ifdef __clang__
	#pragma clang diagnostic ignored "-Wmissing-field-initializers"
#endif
#include <assert.h>
#include <stdio.h>
#include <stdint.h>
#include <string.h>

#define DEBUG
#include "grengine.h"
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

int test_vector_ptr_append() {
	vector *vec = vector_new(sizeof(uint64_t), 2);

	uint32_t i = 0;
	for (i = 0; i < 100; i++) {
		char buf[128] = {0};
		char *duped = NULL;
		snprintf(buf, sizeof(buf), "%u", i);
		duped = strndup(buf, sizeof(buf));

		vector_append_ptr(vec, duped);
	}

	for (i = 0; i < vec->count; i++) {
		char buf[128] = {0};
		snprintf(buf, sizeof(buf), "%u", i);

		char **str = (char **)vector_get(vec, i);

		assert(strncmp(buf, *str, sizeof(buf)) == 0);
		free(*str);
	}

	vector_free(vec);

	return 0;
}

int test_vector_reverse() {
	vector *vec = vector_new(sizeof(uint64_t), 2);

	uint64_t i;
	for (i = 0; i < 99999; i++)
		vector_append(vec, &i, sizeof(i));

	if (!vector_reverse(vec))
		return 1;

	for (i -= 1; i >= vec->count; i--) {
		uint64_t *j = (uint64_t *)vector_get(vec, i);
		if (i != *j)
			return 1;
	}

	return 0;
}

int test_request_without_body_is_parseable() {
	const char req[] = " GET / HTTP/1.1\r\n"
		"User-Agent: curl/7.35.0\r\n"
		"Host: localhost:8080\r\n"
		"Content-Length: 6\r\n"
		"Accept: */*\r\n\r\n"
		"Hello!";
	http_request request = {
		.verb = {0},
		.resource = {0},
		.matches = {{0}},
		.full_header = NULL,
		.body_len = 0,
		.full_body = NULL
	};
	int rc = parse_request(req, strlen(req), &request);
	if (rc != 0)
		return 1;
	if (strnlen(request.full_header, strlen(req)) != strlen(req) - 6)
		return 1;
	if (request.full_body == NULL || request.body_len != 6)
		return 1;
	if (strlen((char *)request.full_body) != 6)
		return 1;
	return 0;
}

int test_request_with_body_is_parseable() {
	const char req[] = " GET / HTTP/1.1\r\n"
		"User-Agent: curl/7.35.0\r\n"
		"Host: localhost:8080\r\n"
		"Accept: */*\r\n\r\n";
	http_request request = {
		.verb = {0},
		.resource = {0},
		.matches = {{0}},
		.full_header = NULL,
		.body_len = 0,
		.full_body = NULL
	};
	int rc = parse_request(req, strlen(req), &request);
	if (rc != 0)
		return 1;
	if (strnlen(request.full_header, strlen(req)) != strlen(req))
		return 1;
	if (request.full_body != NULL || request.body_len > 0)
		return 1;
	return 0;
}

int test_request_body_is_extracted() {
	const char req[] = " GET / HTTP/1.1\r\n"
		"User-Agent: curl/7.35.0\r\n"
		"Host: localhost:8080\r\n"
		"Content-Length: 6\r\n"
		"Accept: */*\r\n\r\n"
		"Hello!";
	http_request request = {
		.verb = {0},
		.resource = {0},
		.matches = {{0}},
		.full_header = NULL,
		.body_len = 0,
		.full_body = NULL
	};
	int rc = parse_request(req, strlen(req), &request);
	if (rc != 0)
		return 1;
	if (request.full_header == NULL || request.full_body == NULL)
		return 1;
	if (strnlen(request.full_header, strlen(req)) != strlen(req) - 6)
		return 1;
	if (request.full_body == NULL || request.body_len <= 0)
		return 1;
	if (strcmp((char *)request.full_body, "Hello!") != 0)
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

	/* Vector tests */
	run_test(test_vector);
	run_test(test_vector_ptr_append);
	run_test(test_vector_reverse);

	/* Parsing tests */
	run_test(test_request_without_body_is_parseable);
	run_test(test_request_with_body_is_parseable);
	run_test(test_request_body_is_extracted);

	log_msg(LOG_INFO, "Tests passed: (%i/%i).\n", tests_run, tests_run + tests_failed);

	if (tests_run != tests_run + tests_failed)
		return 1;
	return 0;
}
