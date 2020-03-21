// vim: noet ts=4 sw=4
#define _GNU_SOURCE
#include <errno.h>
#include <limits.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdint.h>
#include <sys/socket.h>
#define __STDC_FORMAT_MACROS
#include <inttypes.h>

#include "grengine.h"
#include "parse.h"
#include "logging.h"
#include "simple_sparsehash.h"
#include "utils.h"

m38_range_header m38_parse_range_header(const char *range_query) {
	/* bytes=0- */
	/* bytes=12345-55555 */

	if (strncmp(range_query, "bytes=", strlen("bytes=")) != 0) {
		m38_log_msg(LOG_WARN, "Malformed range query: %s", range_query);
		m38_range_header rng = {
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
		m38_log_msg(LOG_WARN, "Malformed range query: %s", range_query);
		m38_range_header rng = {
			.limit = 0,
			.offset = 0
		};
		return rng;
	}

	long int _fn = strtol(actual_value, NULL, 10);
	if ((_fn == LONG_MIN || _fn == LONG_MAX) && errno == ERANGE) {
		m38_log_msg(LOG_WARN, "Malformed range query, could not parse first_num: %s", range_query);
		m38_range_header rng = {
			.limit = 0,
			.offset = 0
		};
		return rng;
	}

	const size_t first_num = (size_t)_fn;
	if (i == (strlen(actual_value) - 1)) {
		/* No second number, just return 0. */
		m38_range_header rng = {
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
		m38_range_header rng = {
			.limit = 0,
			.offset = first_num
		};
		return rng;
	}
	const size_t second_num = _sn;

	if (second_num < first_num) {
		m38_range_header rng = {
			.limit = 0,
			.offset = first_num
		};
		return rng;
	}

	m38_range_header rng = {
		.limit = second_num,
		.offset = first_num
	};

	return rng;
}

char *m38_get_header_value_raw(const char *request, const size_t request_siz, const char header[static 1]) {
	/* TODO: Make sure this function handles bad input. */
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

char *m38_get_header_value_request(const m38_http_request *req, const char header[static 1]) {
	return m38_get_header_value_raw(req->full_header, req->header_len, header);
}

int m38_parse_request(const unsigned char *to_read, const size_t num_read, m38_http_request *out) {
	/* Find the verb */
	const char *verb_end = strnstr((char *)to_read, " ", MAX_READ_LEN);
	if (verb_end == NULL)
		goto error;

	const size_t c_verb_size = verb_end - (char *)to_read;
	const size_t verb_size = c_verb_size >= sizeof(out->verb) ? sizeof(out->verb) - 1: c_verb_size;
	strncpy(out->verb, (char *)to_read, verb_size);

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
		m38_log_msg(LOG_DEBUG, "Could not find end of HTTP header.");
		goto error;
	}

	out->full_header = strndup((char *)to_read, header_length);
	out->header_len = header_length;

	return 0;

error:
	return -1;
}

int m38_parse_body(const size_t received_body_len,
			   const size_t content_length_num,
			   const unsigned char *raw_request,
			   m38_http_request *request) {

	const char *body_start = (char *)raw_request + request->header_len;
	/* We want to read the least amount. Read whatever is smallest. DO IT BECAUSE I SAY SO. */
	if (content_length_num == received_body_len) {
		request->body_len = content_length_num;
		request->full_body = (unsigned char *)strndup(body_start, content_length_num);
	} else if (received_body_len < content_length_num) {
		m38_log_msg(LOG_DEBUG, "FULL BODY: Received body length is less than clength for full_body.");
		request->body_len = received_body_len;
		request->full_body = (unsigned char *)strndup(body_start, request->body_len);
	} else if (content_length_num > 0 && content_length_num < received_body_len) {
		m38_log_msg(LOG_DEBUG, "FULL BODY: Content length is less than total received body length.");
		request->body_len = content_length_num;
		request->full_body = (unsigned char *)strndup(body_start, request->body_len);
	} else {
		m38_log_msg(LOG_DEBUG, "FULL BODY: Discrepancy between content-length and body length.");
		request->full_body = NULL;
	}

	if (!request->full_body) {
		m38_log_msg(LOG_WARN, "No body on request, even though we expect one.");
		return -1;
	}

	return 0;
}

int m38_parse_form_encoded_body(m38_http_request *request) {
	struct sparse_dict *new_dict = NULL;
	char *potential_kv_pair = NULL;
	char *duplicated_body = strndup((char *)request->full_body, request->body_len);
	if (!duplicated_body) {
		goto err;
	}

	new_dict = sparse_dict_init();

	while ((potential_kv_pair = strsep((char **)&request->full_body, "&")) != NULL) {
		char *key = NULL;
		char *value = NULL;
		key = strsep(&potential_kv_pair, "=");
		value = strsep(&potential_kv_pair, "=");

		if (!key || !value) {
			continue;
		}

		char *decoded_buffer = calloc(1, strlen(value));
		if (m38_url_decode(value, decoded_buffer) > 0) {
			sparse_dict_set(new_dict, key, strlen(key), decoded_buffer, strlen(decoded_buffer));
		} else {
			m38_log_msg(LOG_WARN, "Could not place item into form elements dict.");
		}
		free(decoded_buffer);
	}

	request->form_elements = new_dict;
	free(duplicated_body);
	return 0;

err:
	free(duplicated_body);
	free(new_dict);
	return -1;
}
