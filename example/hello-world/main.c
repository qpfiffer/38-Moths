// vim: noet ts=4 sw=4
#include <string.h>

#include <38-moths/38-moths.h>

int main_sock_fd;

static int index_handler(const http_request *request, http_response *response) {
	const unsigned char buf[] = "Hello, World!";
	response->out = malloc(sizeof(buf));
	memcpy(response->out, buf, sizeof(buf));

	response->outsize = sizeof(buf);
	/* We don't have to set the mimetype here, but why not? */
	strncpy(response->mimetype, "text/plain", sizeof(response->mimetype));
	return 200;
}

static const route all_routes[] = {
	{"GET", "root_handler", "^/$", 0, &index_handler, &heap_cleanup},
};

int main(int argc, char *argv[]) {
	http_serve(main_sock_fd, 2, all_routes, sizeof(all_routes)/sizeof(all_routes[0]));
	return 0;
}
