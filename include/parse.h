// vim: noet ts=4 sw=4
#pragma once
#include "types.h"

/* xXx DEFINE=MAX_READ_LEN xXx
* xXx DESCRIPTION=The maximum amount of bytes to be read when receiving a request. xXx
*/
#define MAX_READ_LEN 1024

/* Used for answering range queries on static files. */
typedef struct {
	const size_t limit;
	const size_t offset;
} m38_range_header;

/* FUCK THE MUTEABLE STATE */
m38_range_header m38_parse_range_header(const char *range_query);

/* xXx FUNCTION=m38_get_header_value_raw xXx
 * xXx DESCRIPTION=Gets the value of `header` (eg. Content-Length) from an http_request object. Wraps get_header_value_raw. xXx
 * xXx RETURNS=The char string representing the header value, or NULL. Must be free'd. xXx
 */
char *m38_get_header_value_request(const m38_http_request *req, const char header[static 1]);

/* xXx FUNCTION=m38_get_header_value_raw xXx
 * xXx DESCRIPTION=Gets the value of `header` (eg. Content-Length) from a raw http request string. xXx
 * xXx RETURNS=The char string representing the header value, or NULL. Must be free'd. xXx
 */
char *m38_get_header_value_raw(const char *request, const size_t request_siz, const char header[static 1]);

/* xXx FUNCTION=m38_parse_request xXx
 * xXx DESCRIPTION=Turns a raw string buffer into an http_request object. xXx
 * xXx RETURNS=0 on sucess, -1 on failure. xXx
 */
int m38_parse_request(const unsigned char *, const size_t, m38_http_request *);

/* xXx FUNCTION=m38_parse_request xXx
 * xXx DESCRIPTION=responsible for figuring out the content-length of an HTTP request. xXx
 * xXx RETURNS=0 on sucess, -1 on failure. xXx
 */
int m38_parse_body(const size_t received_body_len, const size_t content_length_num,
		const unsigned char *raw_request, m38_http_request *request);
