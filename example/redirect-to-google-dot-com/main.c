// vim: noet ts=4 sw=4
#include <string.h>

#include <38-moths/38-moths.h>

int main_sock_fd;

static int index_handler(const http_request *request, http_response *response) {
	insert_custom_header(response, "Location", "http://google.com/");
	return 302;
}

static const route all_routes[] = {
	{"GET", "root_handler", "^/$", 0, &index_handler, NULL},
};

int main(int argc, char *argv[]) {
	http_serve(&main_sock_fd, 8080, 2, all_routes, sizeof(all_routes)/sizeof(all_routes[0]));
	return 0;
}
