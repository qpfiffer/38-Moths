// vim: noet ts=4 sw=4
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include <38-moths/38-moths.h>

const char taico_id[] = "3NZGbD236fw";

int main_sock_fd;

static int index_handler(const http_request *request, http_response *response) {
	const size_t board_len = request->matches[1].rm_eo - request->matches[1].rm_so;
	char buf[128] = {0};
	strncpy(buf, request->resource + request->matches[1].rm_so, sizeof(buf));

	greshunkel_ctext *ctext = gshkl_init_context();
	if (rand() % 2) {
		gshkl_add_string(ctext, "VIDEO_ID", buf);
	} else {
		gshkl_add_string(ctext, "VIDEO_ID", taico_id);
	}

	return render_file(ctext, "./index.html", response);
}

static const route all_routes[] = {
	{"GET", "root_handler", "^/([a-zA-Z]+)$", 2, &index_handler, &heap_cleanup},
};

int main(int argc, char *argv[]) {
	srandom(time(NULL));
	http_serve(&main_sock_fd, 8081, 2, all_routes, sizeof(all_routes)/sizeof(all_routes[0]));
	return 0;
}
