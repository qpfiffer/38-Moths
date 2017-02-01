// vim: noet ts=4 sw=4
#pragma once

/* xXx DEFINE=MAX_READ_LEN xXx
* xXx DESCRIPTION=The maximum amount of bytes to be read when receiving a request. xXx
*/
#define MAX_READ_LEN 1024

/* Used for answering range queries on static files. */
typedef struct range_header {
	const size_t limit;
	const size_t offset;
} range_header;

/* FUCK THE MUTEABLE STATE */
range_header parse_range_header(const char *range_query);

char *get_header_value(const char *request, const size_t request_siz, const char header[static 1]);
