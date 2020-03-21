// vim: noet ts=4 sw=4
#pragma once
#include "utils.h"
#include "types.h"

struct route;
struct m38_app;

/* xXx FUNCTION=m38_set_404_handler xXx
 * xXx DESCRIPTION=Sets the internal 404 handler. Useful for templating your 404 page. xXx
 * xXx RETURNS=0 on success, -1 on failure. xXx
 * xXx *app=The app to serve. xXx
 * xXx *handler=The function pointer to your handler function.. xXx
 */
int m38_set_404_handler(m38_app *app,
	int (*handler)(const m38_http_request *request, m38_http_response *response));

/* xXx FUNCTION=m38_set_404_cleanup xXx
 * xXx DESCRIPTION=Sets the cleanup for your custom 404 handler. xXx
 * xXx RETURNS=0 on success, -1 on failure. xXx
 * xXx *app=The app to serve. xXx
 * xXx *cleanup=The function pointer to your cleanup function. xXx
 */
int m38_set_404_cleanup(m38_app *app,
	void (*cleanup)(const int status_code, m38_http_response *response));

/* xXx FUNCTION=m38_set_500_handler xXx
 * xXx DESCRIPTION=Sets the internal error handler. Useful for templating your internal error page. xXx
 * xXx RETURNS=0 on success, -1 on failure. xXx
 * xXx *app=The app to serve. xXx
 */
//int m38_set_500_handler(m38_app *app,
//	int (*handler)(const m38_http_request *request, m38_http_response *response));

/* xXx FUNCTION=m38_http_serve xXx
 * xXx DESCRIPTION=Starts and runs the HTTP server. xXx
 * xXx RETURNS=0 on success, -1 on failure. xXx
 * xXx *app=The app to serve. xXx
 */
int m38_http_serve(m38_app *app);
