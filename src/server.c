// vim: noet ts=4 sw=4
#ifdef __clang__
	#pragma clang diagnostic ignored "-Wmissing-field-initializers"
#endif
#include <arpa/inet.h>
#include <assert.h>
#include <dirent.h>
#include <fcntl.h>
#include <mqueue.h>
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

#define ACCEPTED_SOCKET_QUEUE_NAME "/mothsqueue"

typedef struct acceptor_arg {
	const int main_sock_fd;
} acceptor_arg;

typedef struct worker_arg {
	const route *all_routes;
	const size_t num_routes;
	const int worker_ident;
} worker_arg;

static void *acceptor(void *arg) {
	mqd_t accepted_socket_queue = 0;
	const acceptor_arg *args = arg;
	const int main_sock_fd = args->main_sock_fd;

	struct mq_attr asq_attr = {
		.mq_flags = 0,
		.mq_maxmsg = 64,
		.mq_msgsize = sizeof(int), /* We're passing around file descriptors */
		.mq_curmsgs = 0
	};

	accepted_socket_queue = mq_open(ACCEPTED_SOCKET_QUEUE_NAME, O_WRONLY | O_CREAT, 0644, &asq_attr);
	if (accepted_socket_queue == -1) {
		log_msg(LOG_ERR, "Acceptor: Could not open accepted_socket_queue.");
		perror("Acceptor: ");
		return NULL;
	}

	while(1) {
		struct sockaddr_storage their_addr = {0};
		socklen_t sin_size = sizeof(their_addr);

		int new_fd = accept(main_sock_fd, (struct sockaddr *)&their_addr, &sin_size);

		if (new_fd == -1) {
			log_msg(LOG_ERR, "Acceptor: Could not accept new connection.");
			return NULL;
		}

		ssize_t msg_size = mq_send(accepted_socket_queue, (char *)&new_fd, sizeof(int), 0);
		if (msg_size == -1) {
			/* We want to continue from this error. Maybe the queue is full or something. */
			log_msg(LOG_ERR, "Acceptor: Could not queue new accepted connection.");
			close(new_fd);
		}
	}

	return NULL;
}

static void *worker(void *arg) {
	mqd_t accepted_socket_queue = 0;
	const worker_arg *args = arg;
	const size_t num_routes = args->num_routes;
	const route *all_routes = args->all_routes;
	const int worker_ident = args->worker_ident;

	accepted_socket_queue = mq_open(ACCEPTED_SOCKET_QUEUE_NAME, O_RDONLY);
	if (accepted_socket_queue == -1) {
		log_msg(LOG_ERR, "Worker %i: Could not open accepted_socket_queue.", worker_ident);
		perror("Worker: ");
		return NULL;
	}

	while(1) {
		int new_fd = 0;
		ssize_t msg_size = mq_receive(accepted_socket_queue, (char *)&new_fd, sizeof(int), NULL);

		if (msg_size == -1) {
			log_msg(LOG_ERR, "Worker %i: Could not read from accepted_socket_queue.", worker_ident);
			break;
		} else if (new_fd == -1) {
			log_msg(LOG_ERR, "Worker %i: Got bogus FD from accepted_socket_queue.", worker_ident);
			break;
		} else {
			respond(new_fd, all_routes, num_routes);
			close(new_fd);
		}
	}

	mq_close(accepted_socket_queue);

	return NULL;
}

int http_serve(int main_sock_fd,
		const int num_threads,
		const struct route *routes,
		const size_t num_routes) {
	/* Our acceptor pool: */
	pthread_t workers[num_threads];
	pthread_t acceptor_enqueuer;

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

	struct acceptor_arg args = {
		.main_sock_fd = main_sock_fd,
	};

	if (pthread_create(&acceptor_enqueuer, NULL, acceptor, &args) != 0) {
		goto error;
	}
	log_msg(LOG_INFO, "Acceptor thread started.");

	int i;
	for (i = 0; i < num_threads; i++) {
		struct worker_arg args = {
			.all_routes = routes,
			.num_routes = num_routes,
			.worker_ident = i
		};
		if (pthread_create(&workers[i], NULL, worker, &args) != 0) {
			goto error;
		}
		log_msg(LOG_INFO, "Worker thread %i started.", i);
	}

	for (i = 0; i < num_threads; i++) {
		pthread_join(workers[i], NULL);
		log_msg(LOG_INFO, "Worker thread %i stopped.", i);
	}
	pthread_join(acceptor_enqueuer, NULL);
	log_msg(LOG_INFO, "Acceptor thread stopped.");

	close(main_sock_fd);
	return 0;

error:
	perror("Server error: ");
	close(main_sock_fd);
	return rc;
}

