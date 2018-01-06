// vim: noet ts=4 sw=4
#pragma once
#include "utils.h"
#include "types.h"

struct route;
/* xXx FUNCTION=m38_http_serve xXx
 * xXx DESCRIPTION=Starts and runs the HTTP server. xXx
 * xXx RETURNS=0 on success, -1 on failure. xXx
 * xXx *main_sock_fd=A pointer to the main socket fd. This is a pointer so you can handle SIG* cleanly and shut down the socket. xXx
 * xXx port=The main port to run the server on. Like 8080. Or something. xXx
 * xXx num_threads=The number of threads to use to handle requests. xXx
 * xXx *routes=The array of all routes for your application. xXx
 * xXx num_routes=The number of routes in <code>*routes</code>. xXx
 */
int m38_http_serve(int *main_sock_fd, const int port, const int num_threads,
		const m38_route *routes, const size_t num_routes);
