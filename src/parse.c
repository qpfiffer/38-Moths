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

#include "grengine.h"
#include "parse.h"
#include "logging.h"
#include "utils.h"

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
		range_header rng = {
			.limit = 0,
			.offset = first_num
		};
		return rng;
	}

	range_header rng = {
		.limit = second_num,
		.offset = first_num
	};

	return rng;
}

char *get_header_value(const char *request, const size_t request_siz, const char header[static 1]) {
	char *data = NULL;
	const char *header_loc = strnstr(request, header, request_siz);
	if (!header_loc)
		return NULL;

	const char *header_value_start = header_loc + strnlen(header, request_siz) + strlen(": ");
	const char *header_value_end = strnstr(header_loc, "\r\n", request_siz);
	if (!header_value_end)
		return NULL;

	const size_t header_value_size = header_value_end - header_value_start;
	data = calloc(sizeof(char), header_value_size + 1);
	data[header_value_size] = '\0';
	strncpy(data, header_value_start, header_value_size);

	return data;
}

int parse_request(const char to_read[MAX_READ_LEN], const size_t num_read, http_request *out) {
	/* Find the verb */
	const char *verb_end = strnstr(to_read, " ", MAX_READ_LEN);
	if (verb_end == NULL)
		goto error;

	const size_t c_verb_size = verb_end - to_read;
	const size_t verb_size = c_verb_size >= sizeof(out->verb) ? sizeof(out->verb) - 1: c_verb_size;
	strncpy(out->verb, to_read, verb_size);

	const char *res_offset = verb_end + strlen(" ");
	const char *resource_end = strnstr(res_offset, " ", sizeof(out->resource));
	if (resource_end == NULL)
		goto error;

	const size_t c_resource_size = resource_end - res_offset;
	const size_t resource_size = c_resource_size >= sizeof(out->resource) ? sizeof(out->resource) : c_resource_size;
	strncpy(out->resource, res_offset, resource_size);

	/* Parse full header here. */
	size_t header_length = 0;
	int found_end = 0;
	while (header_length + 3 <= num_read) {
		if (to_read[header_length] == '\r' &&
		to_read[header_length + 1] == '\n' &&
		to_read[header_length + 2] == '\r' &&
		to_read[header_length + 3] == '\n') {
			header_length += 4;
			found_end = 1;
			break;
		}
		header_length++;
	}

	if (!found_end) {
		log_msg(LOG_ERR, "Could not find end of HTTP header.");
		goto error;
	}

	out->full_header = strndup(to_read, header_length);

	/* Do we have a POST body or something? */
	const size_t post_body_len = num_read - header_length;
	if (post_body_len > 0) {
		/* Parse the body into something useful. */
		char *clength = get_header_value(out->full_header, header_length, "Content-Length");
		size_t clength_num = 0;
		if (clength == NULL) {
			log_msg(LOG_WARN, "Could not parse content length.");
		} else {
			clength_num = atoi(clength);
		}

		/* We want to read the least amount. Read whatever is smallest. DO IT BECAUSE I SAY SO. */
		if (post_body_len < clength_num) {
			out->body_len = num_read - header_length;
			out->full_body = (unsigned char *)strndup(to_read + header_length, out->body_len);
		} else if (clength_num > 0 && clength_num < (size_t)num_read) {
			out->body_len = clength_num;
			out->full_body = (unsigned char *)strndup(to_read + header_length, out->body_len);
		} else {
			out->full_body = NULL;
		}
	}

	return 0;

error:
	return -1;
}
