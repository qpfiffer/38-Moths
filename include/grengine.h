// vim: noet ts=4 sw=4
#pragma once
#include <regex.h>
#include "greshunkel.h"
#include "parse.h"

/* xXx DEFINE=MAX_READ_LEN xXx
* xXx DESCRIPTION=The maximum amount of bytes to be read when receiving a request. xXx
*/
#define MAX_READ_LEN 1024

/* xXx DEFINE=VERB_SIZE xXx
* xXx DESCRIPTION=The maximum size of an HTTP verb. xXx
*/
#define VERB_SIZE 16

/* xXx DEFINE=MAX_MATCHES xXx
* xXx DESCRIPTION=The maximum number of matches one can have on a given url. xXx
*/
#define MAX_MATCHES 4

/* xXx DEFINE=RESPONSE_OK xXx
* xXx DESCRIPTION=Macro used to check whether a status code is 'good'. xXx
*/
#define RESPONSE_OK(status_code) (status_code >= 200 && status_code < 400)

/* Structs for you! */
/* ------------------------------------------------------------------------ */

/* xXx STRUCT=http_request xXx
 * xXx DESCRIPTION=A representation of an HTTP request object. This will be passed to views. xXx
 * xXx verb=The HTTP verb for the given request. xXx
 * xXx resource[128]=The path for this request. (eg. '/articles/182') xXx
 * xXx matches=Any REGEX matches from your path are stored here. xXx
 * xXx full_header=The full header text of the request. xXx
 * xXx body_len=The length of the POST body, or 0.
 * xXx full_body=The full body. xXx
 */
typedef struct {
	char verb[VERB_SIZE];
	char resource[128];
	regmatch_t matches[MAX_MATCHES];
	char *full_header;
	size_t body_len;
	unsigned char *full_body;
} http_request;

/* xXx STRUCT=http_response xXx
* xXx DESCRIPTION=Fill this out and return it, signed by your parents. Only <code>*out</code> and <code>outsize</code> are really necessary. xXx
* xXx *out=A buffer of characters that will be written back to the requester. xXx
* xXx outsize=The size of <code>out</code>.
* xXx mimetype[32]=Optional, will be inferred from the http_request's file extension if left blank. xXx
* xXx *extra_data=Optional, use this to pass things to the clean up function. For instance, mmap_file() uses <code>extra_data</code> to store the size of the file allocated. xXx
*/
typedef struct {
	unsigned char *out;
	size_t outsize;
	char mimetype[32];
	void *extra_data;
} http_response;

/* xXx STRUCT=route xXx
 * xXx DESCRIPTION=An array of these is how 38-Moths knows how to route requests. xXx
 * xXx verb[VERB_SIZE]=The verb that this route will handle. xXx
 * xXx name[64]=The name of the route. Used only logging. xXx
 * xXx route_match[256]=The POSIX regular expression used to match the route to your handler. xXx
 * xXx (*handler)=A function pointer to your route handler. xXx
 * xXx (*cleanup)=If your route needs to do any cleanup (eg. de-allocating memory), this function will becalled when 38-Moths is done with it. xXx
 */
typedef struct route {
	char verb[VERB_SIZE];
	char name[64];
	char route_match[256];
	size_t expected_matches;
	int (*handler)(const http_request *request, http_response *response);
	void (*cleanup)(const int status_code, http_response *response);
} route;

typedef struct handled_request {
	char *response_bytes;
	size_t response_len;
	int accept_fd;
	int response_code;
	size_t sent;
	const route *matching_route;
	http_response response;
} handled_request;

/* xXx FUNCTION=mmap_file xXx
 * xXx DESCRIPTION=The primary way of serving static assets in 38-Moths. mmap()'s a file into memory and writes it to the requester. xXx
 * xXx RETURNS=An HTTP status code. 200 on success, 404 on not found, etc. xXx
 * xXx *file_path=The file to mmap(). xXx
 * xXx *response=The <code>http_response</code> object your handler was passed. xXx
 */
int mmap_file(const char *file_path, http_response *response);

/* xXx FUNCTION=render_file xXx
 * xXx DESCRIPTION=The easiest way to render a file with GRESHUNKEL. xXx
 * xXx RETURNS=An HTTP status code. 200 on success, 404 on not found, etc. xXx
 * xXx *ctext=The context you want your file to have. This should contain all variables, loops, etc. xXx
 * xXx *file_path=The template to render. xXx
 * xXx *response=The <code>http_response</code> object your handler was passed. xXx
 */
int render_file(const struct greshunkel_ctext *ctext, const char *file_path, http_response *response);

/* xXx FUNCTION=heap_cleanup xXx
 * xXx DESCRIPTION=Simple function that <code>free()</code>'s memory in <code>out</code>. xXx
 * xXx RETURNS=Nothing. xXx
 * xXx status_code=The status code returned from the handler. xXx
 * xXx *response=The <code>http_response</code> object returned from the handler. xXx
 */
void heap_cleanup(const int status_code, http_response *response);

/* xXx FUNCTION=mmap_cleanup xXx
 * xXx DESCRIPTION=The cleanup handler for <code>mmap_file</code>. Expects <code>*extradata</code> to be a <code>struct stat</code> object. xXx
 * xXx RETURNS=Nothing. xXx
 * xXx status_code=The status code returned from the handler. xXx
 * xXx *response=The <code>http_response</code> object returned from the handler. xXx
 */
void mmap_cleanup(const int status_code, http_response *response);

/* xXx FUNCTION=generate_response xXx
 * xXx DESCRIPTION=Generates and HTTP response from an accepted connection xXx
 * xXx RETURNS=A fully formatted HTTP response, NULL otherwise. xXx
 * xXx accept_fd=The successfully <code>accept(2)</code>'d file descriptor for the requester's socket. xXx
 * xXx *all_routes=The array of all routes for your application. xXx
 * xXx route_num_elements=The number of routes in <code>*all_routes</code>. xXx
 */
handled_request *generate_response(const int accept_fd, const route *all_routes, const size_t route_num_elements);

/* xXx FUNCTION=send_response xXx
 * xXx DESCRIPTION=Takes a handled_request object rom generate_response and sends chunks of it down the wire. xXx
 * xXx RETURNS=If there is anything left to send, an updated handled_request will be returned. NULL will be reutnred when the object has either been fully sent, or errored out. xXx
 * xXx *hreq=A handled_request object either from send_response or generate_response. xXx
 */
handled_request *send_response(handled_request *hreq);

