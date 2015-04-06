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
 */
typedef struct {
	char verb[VERB_SIZE];
	char resource[128];
	regmatch_t matches[MAX_MATCHES];
	char *full_header;
} http_request;

/* xXx STRUCT=http_request xXx
* xXx DESCRIPTION=Fill this out and return it, signed by your parents. Only <code>*out</code> and <code>outsize</code> are really necessary. xXx
* xXx *out=A buffer of characters that will be written back to the requester. xXx
* xXx outsize=The size of <code>out</code>.
* xXx mimetype[32]=Optional, will be inferred from the http_request's file extension if left blank. xXx
* xXx byte_range=If a partial content request comes in, this will b
*/
typedef struct {
	unsigned char *out;
	size_t outsize;
	char mimetype[32];
	void *extra_data;
} http_response;

typedef struct route {
	char verb[VERB_SIZE];
	char name[64];
	char route_match[256];
	size_t expected_matches;
	int (*handler)(const http_request *request, http_response *response);
	void (*cleanup)(const int status_code, http_response *response);
} route;


/* Default 404 handler. */
/* ------------------------------------------------------------------------ */
int r_404_handler(const http_request *request, http_response *response);
static const route r_404_route = {
	.verb = "GET",
	.route_match = "^.*$",
	.handler = (&r_404_handler),
	.cleanup = NULL
};

/* Utility functions for command handler tasks. */
/* ------------------------------------------------------------------------ */

/* mmap()'s a file into memory and fills out the extra_data param on
 * the response object with a struct st. This needs to be present
 * to be handled later by mmap_cleanup.
 */
int mmap_file(const char *file_path, http_response *response);
/* Renders a file with the given context. */
int render_file(const struct greshunkel_ctext *ctext, const char *file_path, http_response *response);
/* Helper function that blindly guesses the mimetype based on the file extension. */
void guess_mimetype(const char *ending, const size_t ending_siz, http_response *response);

/* Cleanup functions used after handlers have made a bunch of bullshit: */
void heap_cleanup(const int status_code, http_response *response);
void mmap_cleanup(const int status_code, http_response *response);

/* Parses a raw bytestream into an http_request object. */
int parse_request(const char to_read[MAX_READ_LEN], http_request *out);
/* Takes an accepted socket, an array of routes and the number of handlers in said array,
 * then calls the first handler to match the requested resource. It is responsible for
 * writing the result of the handler to the socket. */
int respond(const int accept_fd, const route *all_routes, const size_t route_num_elements);

