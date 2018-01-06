// vim: noet ts=4 sw=4
#include <string.h>
#include <time.h>
#include <stdio.h>

#include <38-moths/38-moths.h>

int main_sock_fd;

static int submission_handler(const m38_http_request *request, m38_http_response *response) {
	printf("FULL HEADER:\n%s", request->full_header);
	printf("FULL REQUEST:\n%s", request->full_body);
	greshunkel_ctext *ctext = gshkl_init_context();
	return m38_render_file(ctext, "./index.html", response);
}

static int index_handler(const m38_http_request *request, m38_http_response *response) {
	greshunkel_ctext *ctext = gshkl_init_context();
	return m38_render_file(ctext, "./index.html", response);
}

static const m38_route all_routes[] = {
	{"GET", "root_handler", "^/$", 0, &index_handler, &m38_heap_cleanup},
	{"POST", "submission_handler", "^/$", 0, &submission_handler, &m38_heap_cleanup},
};

int main(int argc, char *argv[]) {
	m38_http_serve(&main_sock_fd, 8080, 2, all_routes, sizeof(all_routes)/sizeof(all_routes[0]));
	return 0;
}
