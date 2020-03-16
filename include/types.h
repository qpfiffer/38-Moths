// vim: noet ts=4 sw=4
#pragma once
#include <regex.h>

/* xXx DEFINE=VERB_SIZE xXx
* xXx DESCRIPTION=The maximum size of an HTTP verb. xXx
*/
#define VERB_SIZE 16

/* xXx DEFINE=MAX_MATCHES xXx
* xXx DESCRIPTION=The maximum number of matches one can have on a given url. xXx
*/
#define MAX_MATCHES 4

/* Structs for you! */
/* ------------------------------------------------------------------------ */

/* xXx STRUCT=m38_http_request xXx
 * xXx DESCRIPTION=A representation of an HTTP request object. This will be passed to views. xXx
 * xXx verb=The HTTP verb for the given request. xXx
 * xXx resource[128]=The path for this request. (eg. '/articles/182') xXx
 * xXx matches=Any REGEX matches from your path are stored here. xXx
 * xXx header_len=The length of the header, or 0. xXx
 * xXx full_header=The full header text of the request. xXx
 * xXx body_len=The length of the POST body, or 0. xXx
 * xXx full_body=The full body. xXx
 */
typedef struct {
	char verb[VERB_SIZE];
	char resource[512];
	regmatch_t matches[MAX_MATCHES];
	char *full_header;
	size_t header_len;
	unsigned char *full_body;
	size_t body_len;
} m38_http_request;

/* xXx STRUCT=m38_http_response xXx
* xXx DESCRIPTION=Fill this out and return it, signed by your parents. Only <code>*out</code> and <code>outsize</code> are really necessary. xXx
* xXx *out=A buffer of characters that will be written back to the requester. xXx
* xXx outsize=The size of <code>out</code>. xXx
* xXx mimetype[32]=Optional, will be inferred from the m38_http_request's file extension if left blank. xXx
* xXx *extra_data=Optional, use this to pass things to the clean up function. For instance, mmap_file() uses <code>extra_data</code> to store the size of the file allocated. xXx
* xXx *extra_headers=A vector containing extra headers. Use `insert_custom_header` to manage this parameter. xXx
*/
typedef struct {
	unsigned char *out;
	size_t outsize;
	char mimetype[32];
	void *extra_data;
	struct vector *extra_headers;
} m38_http_response;

/* xXx STRUCT=m38_header_pair xXx
 * xXx DESCRIPTION=Object used to hold extra header information in an m38_http_request object. xXx
 * xXx *header=The actual header, eg. "Content-Length" xXx
 * xXx header_len=Length of the header, in bytes. xXx
 * xXx *value=The value of the header, eg. "1762" xXx
 * xXx value_len=Length of the value, in bytes. xXx
 */
typedef struct {
	const char *header;
	const size_t header_len;
	const char *value;
	const size_t value_len;
} m38_header_pair;

/* xXx STRUCT=m38_route xXx
 * xXx DESCRIPTION=An array of these is how 38-Moths knows how to route requests. xXx
 * xXx verb[VERB_SIZE]=The verb that this route will handle. xXx
 * xXx name[64]=The name of the route. Used only logging. xXx
 * xXx route_match[256]=The POSIX regular expression used to match the route to your handler. xXx
 * xXx (*handler)=A function pointer to your route handler. xXx
 * xXx (*cleanup)=If your route needs to do any cleanup (eg. de-allocating memory), this function will becalled when 38-Moths is done with it. xXx
 */
typedef struct {
	char verb[VERB_SIZE];
	char name[64];
	char route_match[256];
	size_t expected_matches;
	int (*handler)(const m38_http_request *request, m38_http_response *response);
	void (*cleanup)(const int status_code, m38_http_response *response);
} m38_route;

typedef struct {
	char *response_bytes;
	size_t response_len;
	int accept_fd;
	int response_code;
	size_t sent;
	const m38_route *matching_route;
	m38_http_response response;
} m38_handled_request;

/* xXx STRUCT=m38_app xXx
 * xXx DESCRIPTION=State and data information for a 38-Moths instance. xXx
 * xXx *main_sock_fd=A pointer to the main socket fd. This is a pointer so you can handle SIG* cleanly and shut down the socket. xXx
 * xXx port=The main port to run the server on. Like 8080. Or something. xXx
 * xXx num_threads=The number of threads to use to handle requests. xXx
 * xXx *routes=The array of all routes for your application. xXx
 * xXx num_routes=The number of routes in <code>*routes</code>. xXx
 */
typedef struct {
	int *main_sock_fd;
	const int port;
	const int num_threads;
	const m38_route *routes;
	const int num_routes;

	int (*r_404_handler)(const m38_http_request *request, m38_http_response *response);
	int (*r_error_handler)(const m38_http_request *request, m38_http_response *response);
} m38_app;
