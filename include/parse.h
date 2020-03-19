// vim: noet ts=4 sw=4
#pragma once
#include "types.h"

/* xXx STRUCT=m38_range_header xXx
 * xXx DESCRIPTION=Used to represent the range header in HTTP requests. xXx
 * xXx limit=The limit of the range, eg. the length. xXx
 * xXx offset=The offset, in the file, that the user has requested. xXx
 */
typedef struct {
	const size_t limit;
	const size_t offset;
} m38_range_header;

/* xXx FUNCTION=m38_parse_range_header xXx
 * xXx DESCRIPTION=Figures out the range header for a request, if present. xXx
 * xXx RETURNS=Returns a 0/0 range header on failure/absence, or the correct limit/offset if present. xXx
 * xXx *range_query=The text from the header. xXx
 */
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

/* xXx FUNCTION=m38_parse_body xXx
 * xXx DESCRIPTION=Figures out the actual body on the HTTP request and sticks it into the request object. xXx
 * xXx RETURNS=0 on sucess, -1 on failure. xXx
 */
int m38_parse_body(const size_t received_body_len, const size_t content_length_num,
		const unsigned char *raw_request, m38_http_request *request);

/* xXx FUNCTION=m38_parse_form_encoded_body xXx
 * xXx DESCRIPTION=Parses a form-encoded body into key/value pairs. xXx
 * xXx RETURNS=0 on success, -1 on failure. xXx
 * xXx *request=The m38_http_request object. Make sure to call m38_parse_body first. xXx
 */
int m38_parse_form_encoded_body(m38_http_request *request);
