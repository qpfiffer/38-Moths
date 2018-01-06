// vim: noet ts=4 sw=4
#pragma once
#include "types.h"
#include "greshunkel.h"

/* xXx DEFINE=RESPONSE_OK xXx
* xXx DESCRIPTION=Macro used to check whether a status code is 'good'. xXx
*/
#define RESPONSE_OK(status_code) (status_code >= 200 && status_code < 400)

/* xXx FUNCTION=m38_mmap_file xXx
 * xXx DESCRIPTION=The primary way of serving static assets in 38-Moths. mmap()'s a file into memory and writes it to the requester. xXx
 * xXx RETURNS=An HTTP status code. 200 on success, 404 on not found, etc. xXx
 * xXx *file_path=The file to mmap(). xXx
 * xXx *response=The <code>m38_http_response</code> object your handler was passed. xXx
 */
int m38_mmap_file(const char *file_path, m38_http_response *response);

/* xXx FUNCTION=m38_render_file xXx
 * xXx DESCRIPTION=The easiest way to render a file with GRESHUNKEL. xXx
 * xXx RETURNS=An HTTP status code. 200 on success, 404 on not found, etc. xXx
 * xXx *ctext=The context you want your file to have. This should contain all variables, loops, etc. xXx
 * xXx *file_path=The template to render. xXx
 * xXx *response=The <code>m38_http_response</code> object your handler was passed. xXx
 */
int m38_render_file(const struct greshunkel_ctext *ctext, const char *file_path, m38_http_response *response);

/* xXx FUNCTION=m38_heap_cleanup xXx
 * xXx DESCRIPTION=Simple function that <code>free()</code>'s memory in <code>out</code>. xXx
 * xXx RETURNS=Nothing. xXx
 * xXx status_code=The status code returned from the handler. xXx
 * xXx *response=The <code>m38_http_response</code> object returned from the handler. xXx
 */
void m38_heap_cleanup(const int status_code, m38_http_response *response);

/* xXx FUNCTION=m38_mmap_cleanup xXx
 * xXx DESCRIPTION=The cleanup handler for <code>mmap_file</code>. Expects <code>*extradata</code> to be a <code>struct stat</code> object. xXx
 * xXx RETURNS=Nothing. xXx
 * xXx status_code=The status code returned from the handler. xXx
 * xXx *response=The <code>m38_http_response</code> object returned from the handler. xXx
 */
void m38_mmap_cleanup(const int status_code, m38_http_response *response);

/* xXx FUNCTION=m38_generate_response xXx
 * xXx DESCRIPTION=Generates and HTTP response from an accepted connection xXx
 * xXx RETURNS=A fully formatted HTTP response, NULL otherwise. xXx
 * xXx accept_fd=The successfully <code>accept(2)</code>'d file descriptor for the requester's socket. xXx
 * xXx *all_routes=The array of all routes for your application. xXx
 * xXx route_num_elements=The number of routes in <code>*all_routes</code>. xXx
 */
m38_handled_request *m38_generate_response(const int accept_fd, const m38_route *all_routes, const size_t route_num_elements);

/* xXx FUNCTION=m38_send_response xXx
 * xXx DESCRIPTION=Takes a handled_request object rom generate_response and sends chunks of it down the wire. xXx
 * xXx RETURNS=If there is anything left to send, an updated handled_request will be returned. NULL will be reutnred when the object has either been fully sent, or errored out. xXx
 * xXx *hreq=A handled_request object either from send_response or generate_response. xXx
 */
m38_handled_request *m38_send_response(m38_handled_request *hreq);

/* xXx FUNCTION=m38_insert_custom_header xXx
 * xXx DESCRIPTION=Adds a custom header/value pair to an m38_http_response object. Use this for redirects, cookies, etc. xXx
 * xXx RETURNS=1 on sucess, 0 on failure. xXx
 * xXx *response=The response to add the header to. xXx
 * xXx *header=The NULL-terminated string representing the header to add, eg. "Location". Note that there is no ":". xXx
 * xXx *value=The NULL-terminated string representing the header's value, eg. "/api/test". xXx
 */
int m38_insert_custom_header(m38_http_response *response, const char *header, const size_t header_len, const char *value, const size_t value_len);
