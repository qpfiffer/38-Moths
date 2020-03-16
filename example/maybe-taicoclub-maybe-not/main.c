// vim: noet ts=4 sw=4
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include <38-moths/38-moths.h>

const char taico_id[] = "3NZGbD236fw";

int main_sock_fd;

static int index_handler(const m38_http_request *request, m38_http_response *response) {
	const size_t board_len = request->matches[1].rm_eo - request->matches[1].rm_so;
	char buf[128] = {0};
	strncpy(buf, request->resource + request->matches[1].rm_so, sizeof(buf));

	greshunkel_ctext *ctext = gshkl_init_context();
	if (rand() % 2) {
		gshkl_add_string(ctext, "VIDEO_ID", buf);
	} else {
		gshkl_add_string(ctext, "VIDEO_ID", taico_id);
	}

	return m38_render_file(ctext, "./index.html", response);
}

static const m38_route all_routes[] = {
	{"GET", "root_handler", "^/([a-zA-Z]+)$", 2, &index_handler, &m38_heap_cleanup},
};

int main(int argc, char *argv[]) {
	m38_app app = {
		.main_sock_fd = &main_sock_fd,
		.port = 8080,
		.num_threads = 2,
		.routes = all_routes,
		.num_routes = sizeof(all_routes)/sizeof(all_routes[0])
	};
	m38_http_serve(&app);
	return 0;
}
