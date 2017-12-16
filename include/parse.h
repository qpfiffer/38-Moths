// vim: noet ts=4 sw=4
#pragma once
#include "types.h"

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

/* xXx FUNCTION=get_header_value xXx
 * xXx DESCRIPTION=Gets the value of `header` (eg. Content-Length) from a raw http request string. xXx
 * xXx RETURNS=The char string representing the header value, or NULL. Must be free'd. xXx
 */
char *get_header_value(const char *request, const size_t request_siz, const char header[static 1]);

/* xXx FUNCTION=parse_request xXx
 * xXx DESCRIPTION=Turns a raw string buffer into an http_request object. xXx
 * xXx RETURNS=0 on sucess, -1 on failure. xXx
 */
int parse_request(const unsigned char *, const size_t, http_request *);

int parse_body(const size_t received_body_len, const size_t content_length_num, const unsigned char *raw_request, http_request *request);
