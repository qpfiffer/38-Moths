// vim: noet ts=4 sw=4
#ifdef __clang__
	#pragma clang diagnostic ignored "-Wmissing-field-initializers"
#endif
#include <arpa/inet.h>
#include <assert.h>
#include <dirent.h>
#include <fcntl.h>
#include <netdb.h>
#include <netinet/in.h>
#include <pthread.h>
#include <regex.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <unistd.h>
#include <time.h>

#include "http.h"
#include "logging.h"
#include "server.h"
#include "grengine.h"
#include "greshunkel.h"

typedef struct acceptor_arg {
	const route *all_routes;
	const size_t num_routes;
	const int main_sock_fd;
} acceptor_arg;

static void *acceptor(void *arg) {
	const acceptor_arg *args = arg;
	const int main_sock_fd = args->main_sock_fd;
	const size_t num_routes = args->num_routes;
	const route *all_routes = args->all_routes;
	while(1) {
		struct sockaddr_storage their_addr = {0};
		socklen_t sin_size = sizeof(their_addr);

		int new_fd = accept(main_sock_fd, (struct sockaddr *)&their_addr, &sin_size);

		if (new_fd == -1) {
			log_msg(LOG_ERR, "Could not accept new connection.");
			return NULL;
		} else {
			respond(new_fd, all_routes, num_routes);
			close(new_fd);
		}
	}
	return NULL;
}

int http_serve(int main_sock_fd,
		const int num_threads,
		const struct route *routes,
		const size_t num_routes) {
	/* Our acceptor pool: */
	pthread_t workers[num_threads];

	int rc = -1;
	main_sock_fd = socket(PF_INET, SOCK_STREAM, 0);
	if (main_sock_fd <= 0) {
		log_msg(LOG_ERR, "Could not create main socket.");
		goto error;
	}

	int opt = 1;
	setsockopt(main_sock_fd, SOL_SOCKET, SO_REUSEADDR, (void*) &opt, sizeof(opt));

	const int port = 8080;
	struct sockaddr_in hints = {0};
	hints.sin_family		 = AF_INET;
	hints.sin_port			 = htons(port);
	hints.sin_addr.s_addr	 = htonl(INADDR_ANY);

	rc = bind(main_sock_fd, (struct sockaddr *)&hints, sizeof(hints));
	if (rc < 0) {
		log_msg(LOG_ERR, "Could not bind main socket.");
		goto error;
	}

	rc = listen(main_sock_fd, 0);
	if (rc < 0) {
		log_msg(LOG_ERR, "Could not listen on main socket.");
		goto error;
	}
	log_msg(LOG_FUN, "Listening on http://localhost:%i/", port);

	int i;
	for (i = 0; i < num_threads; i++) {
		struct acceptor_arg args = {
			.all_routes = routes,
			.num_routes = num_routes,
			.main_sock_fd = main_sock_fd,
		};
		if (pthread_create(&workers[i], NULL, acceptor, &args) != 0) {
			goto error;
		}
		log_msg(LOG_INFO, "Thread %i started.", i);
	}

	for (i = 0; i < num_threads; i++) {
		pthread_join(workers[i], NULL);
		log_msg(LOG_INFO, "Thread %i stopped.", i);
	}


	close(main_sock_fd);
	return 0;

error:
	perror("Socket error");
	close(main_sock_fd);
	return rc;
}

