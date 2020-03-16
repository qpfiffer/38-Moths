// vim: noet ts=4 sw=4
#include <string.h>

#include <38-moths/38-moths.h>

int main_sock_fd;

static int index_handler(const m38_http_request *request, m38_http_response *response) {
	return 404;
}

static int r_404_handler(const m38_http_request *request, m38_http_response *response) {
	const unsigned char buf[] = "This is a cbustom 404 page.";
	response->out = malloc(sizeof(buf));
	memcpy(response->out, buf, sizeof(buf));

	response->outsize = sizeof(buf);
	return 404;
}

static const m38_route all_routes[] = {
	{"GET", "root_handler", "^/$", 0, &index_handler, NULL},
};

int main(int argc, char *argv[]) {
	m38_app app = {
		.main_sock_fd = &main_sock_fd,
		.port = 8080,
		.num_threads = 2,
		.routes = all_routes,
		.num_routes = sizeof(all_routes)/sizeof(all_routes[0])
	};
	m38_set_404_handler(&app, &r_404_handler);
	m38_http_serve(&app);
	return 0;
}
