// vim: noet ts=4 sw=4
#pragma once

/* Used for answering range queries on static files. */
typedef struct range_header {
	const size_t limit;
	const size_t offset;
} range_header;

/* FUCK THE MUTEABLE STATE */
range_header parse_range_header(const char *range_query);

char *get_header_value(const char *request, const size_t request_siz, const char header[static 1]);
