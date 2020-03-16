// vim: noet ts=4 sw=4
#include <string.h>
#include <time.h>

#include <38-moths/38-moths.h>

time_t start_time;
int main_sock_fd;

static int index_handler(const m38_http_request *request, m38_http_response *response) {
	time_t now;
	time(&now);

	/* Just do a dead simple uptime computation. */
	greshunkel_ctext *ctext = gshkl_init_context();
	gshkl_add_int(ctext, "UPTIME", difftime(now, start_time));
	return m38_render_file(ctext, "./index.html", response);
}

static const m38_route all_routes[] = {
	{"GET", "root_handler", "^/$", 0, &index_handler, &m38_heap_cleanup},
};

int main(int argc, char *argv[]) {
	/* So we can say how longer the server has been up */
	time(&start_time);
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
