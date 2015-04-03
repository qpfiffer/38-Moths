// vim: noet ts=4 sw=4
#define _GNU_SOURCE
#include <errno.h>
#include <limits.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdint.h>
#define __STDC_FORMAT_MACROS
#include <inttypes.h>

#include "parse.h"
#include "logging.h"

range_header parse_range_header(const char *range_query) {
	/* bytes=0- */
	/* bytes=12345-55555 */

	if (strncmp(range_query, "bytes=", strlen("bytes=")) != 0) {
		log_msg(LOG_WARN, "Malformed range query: %s", range_query);
		range_header rng = {
			.limit = 0,
			.offset = 0
		};
		return rng;
	}

	const char *actual_value = range_query + strlen("bytes=");
	size_t i;
	int found = 0;
	for (i = 0; i < strlen(actual_value); i++) {
		if (actual_value[i] == '-') {
			found = 1;
			break;
		}
	}

	if (!found) {
		log_msg(LOG_WARN, "Malformed range query: %s", range_query);
		range_header rng = {
			.limit = 0,
			.offset = 0
		};
		return rng;
	}

	long int _fn = strtol(actual_value, NULL, 10);
	if ((_fn == LONG_MIN || _fn == LONG_MAX) && errno == ERANGE) {
		log_msg(LOG_WARN, "Malformed range query, could not parse first_num: %s", range_query);
		range_header rng = {
			.limit = 0,
			.offset = 0
		};
		return rng;
	}

	const size_t first_num = (size_t)_fn;
	if (i == (strlen(actual_value) - 1)) {
		/* No second number, just return 0. */
		range_header rng = {
			.limit = 0,
			.offset = first_num
		};
		return rng;
	}

	/* Otherwise, assume the rest of the value is the second number.
	 * The '+1' here is for the '-' between the two numbers.
	 */
	const long int _sn = strtol(actual_value + (i + 1), NULL, 10);
	if ((_sn == LONG_MIN || _sn == LONG_MAX) && errno == ERANGE) {
		/* No second number, just return 0. */
		range_header rng = {
			.limit = 0,
			.offset = first_num
		};
		return rng;
	}
	const size_t second_num = _sn;

	if (second_num < first_num) {
		log_msg(LOG_WARN, "Malformed range query, limit smaller than offset: %s", range_query);
		range_header rng = {
			.limit = 0,
			.offset = 0
		};
		return rng;
	}

	range_header rng = {
		.limit = second_num,
		.offset = first_num
	};

	return rng;
}
