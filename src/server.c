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

#include "logging.h"
#include "server.h"
#include "grengine.h"
#include "greshunkel.h"

#define ACCEPTED_SOCKET_QUEUE_NAME "/mothsaqueue"
#define HANDLED_CONNECTION_QUEUE_NAME "/mothshqueue"

typedef struct acceptor_arg {
	const int main_sock_fd;
} acceptor_arg;

typedef struct handler_arg {
	const route *all_routes;
	const size_t num_routes;
	const int worker_ident;
} handler_arg;

static inline mqd_t _open_queue(const char *queue_name, int flags, const size_t size) {
	(void) flags;
	struct mq_attr asq_attr = {
		.mq_flags = 0,
		.mq_maxmsg = 10,
		.mq_msgsize = size, /* We're passing around file descriptors */
		.mq_curmsgs = 0
	};

	mqd_t to_return = mq_open(queue_name, O_WRONLY | O_CREAT, 0644, &asq_attr);
	if (to_return == -1)
		perror("Could not open queue");

	return to_return;
}
static void *acceptor(void *arg) {
	mqd_t accepted_socket_queue = 0;
	const acceptor_arg *args = arg;
	const int main_sock_fd = args->main_sock_fd;

	accepted_socket_queue = _open_queue(ACCEPTED_SOCKET_QUEUE_NAME, O_WRONLY, sizeof(int));
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

static void *responder(void *arg) {
	UNUSED(arg);

	mqd_t handled_queue = -1;
	handled_queue = mq_open(HANDLED_CONNECTION_QUEUE_NAME, O_RDWR);
	if (handled_queue == -1) {
		log_msg(LOG_ERR, "Responder: Could not open handled queue.");
		perror("Responder: ");
		return NULL;
	}

	while(1) {
		handled_request req;
		ssize_t msg_size = mq_receive(handled_queue, (char *)&req, sizeof(handled_request), NULL);

		if (msg_size == -1) {
			log_msg(LOG_ERR, "Responder: Could not read from handled_queue.");
			break;
		}else {
			int accept_fd = req.accept_fd;
			send_response(&req);
			close(accept_fd);
			log_msg(LOG_FUN, "Responder: Response sent.");
		}
	}

	mq_close(handled_queue);

	return NULL;
}

static void *handler(void *arg) {
	mqd_t accepted_socket_queue = -1;
	mqd_t handled_queue = -1;
	const handler_arg *args = arg;
	const size_t num_routes = args->num_routes;
	const route *all_routes = args->all_routes;
	const int worker_ident = args->worker_ident;

	accepted_socket_queue = mq_open(ACCEPTED_SOCKET_QUEUE_NAME, O_RDONLY);
	if (accepted_socket_queue == -1) {
		log_msg(LOG_ERR, "Worker %i: Could not open accepted_socket_queue.", worker_ident);
		perror("Worker: ");
		return NULL;
	}

	handled_queue = mq_open(HANDLED_CONNECTION_QUEUE_NAME, O_WRONLY);
	if (handled_queue == -1) {
		log_msg(LOG_ERR, "Worker %i: Could not open handled queue.", worker_ident);
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
			log_msg(LOG_FUN, "Worker %i: Handling response.", worker_ident);
			handled_request *req = generate_response(new_fd, all_routes, num_routes);

			ssize_t msg_size = mq_send(handled_queue, (char *)req, sizeof(handled_request), 0);
			if (msg_size == -1)
				log_msg(LOG_ERR, "Worker %i: Could not enqueue handled response.", worker_ident);
		}

		struct mq_attr attr = {0};
		if (mq_getattr(accepted_socket_queue, &attr) == 0) {
			log_msg(LOG_INFO, "Acceptor: Number of items on the queue: %i", attr.mq_curmsgs);
		} else {
			log_msg(LOG_INFO, "Acceptor: Could not pull items from queue.");
			perror("Acceptor MQ Problem: ");
		}
	}

	mq_close(accepted_socket_queue);

	return NULL;
}

static inline int create_socket() {
	int rc = -1;
	int sock_fd = socket(PF_INET, SOCK_STREAM, 0);
	if (sock_fd <= 0) {
		log_msg(LOG_ERR, "Could not create main socket.");
		goto error;
	}

	int opt = 1;
	setsockopt(sock_fd, SOL_SOCKET, SO_REUSEADDR, (void*) &opt, sizeof(opt));

	const int port = 8080;
	struct sockaddr_in hints = {0};
	hints.sin_family		 = AF_INET;
	hints.sin_port			 = htons(port);
	hints.sin_addr.s_addr	 = htonl(INADDR_ANY);

	rc = bind(sock_fd, (struct sockaddr *)&hints, sizeof(hints));
	if (rc < 0) {
		log_msg(LOG_ERR, "Could not bind main socket.");
		goto error;
	}

	rc = listen(sock_fd, 0);
	if (rc < 0) {
		log_msg(LOG_ERR, "Could not listen on main socket.");
		goto error;
	}
	log_msg(LOG_FUN, "Listening on http://localhost:%i/", port);

	return sock_fd;

error:
	close(sock_fd);
	return -1;
}

int http_serve(int *main_sock_fd,
		const int num_threads,
		const struct route *routes,
		const size_t num_routes) {
	mqd_t po_accepted_queue = -1;
	mqd_t po_handled_queue = -1;
	/* Handlers take accepted sockets and turn them into responses: */
	pthread_t handlers[num_threads];
	/* The responder is responsible for sending bytes back over the wire: */
	pthread_t responder_worker;
	/* The acceptor listens and accepts new connections, and enqueues them: */
	pthread_t acceptor_enqueuer;

	*main_sock_fd = create_socket();
	if (*main_sock_fd < 0)
		goto error;

	/* Purge the queue if it already exists by unlinking it. Otherwise we get
	 * stale messages.
	 */
	if (mq_unlink(ACCEPTED_SOCKET_QUEUE_NAME) == 0) {
		log_msg(LOG_INFO, "Purged old socket queue.");
	}

	/* We precreate the queue here to guarantee that everyone else will have access to it. */
	po_accepted_queue = _open_queue(ACCEPTED_SOCKET_QUEUE_NAME, O_RDWR | O_CREAT, sizeof(int));
	if (po_accepted_queue == -1) {
		log_msg(LOG_ERR, "Could not preopen accepted_socket_queue.");
		perror("Main Thread");
		goto error;
	}

	po_handled_queue = _open_queue(HANDLED_CONNECTION_QUEUE_NAME, O_RDWR | O_CREAT, sizeof(handled_request));
	if (po_accepted_queue == -1) {
		log_msg(LOG_ERR, "Could not preopen handled socket queue..");
		perror("Main Thread");
		goto error;
	}

	struct acceptor_arg args = {
		.main_sock_fd = *main_sock_fd,
	};

	if (pthread_create(&acceptor_enqueuer, NULL, acceptor, &args) != 0) {
		log_msg(LOG_ERR, "Could not start acceptor thread.");
		goto error;
	}

	log_msg(LOG_INFO, "Acceptor thread started.");

	if (pthread_create(&responder_worker, NULL, responder, NULL) != 0) {
		log_msg(LOG_ERR, "Could not start responder thread.");
		goto error;
	}

	log_msg(LOG_INFO, "Responder thread started.");

	int i;
	for (i = 0; i < num_threads; i++) {
		struct handler_arg args = {
			.all_routes = routes,
			.num_routes = num_routes,
			.worker_ident = i
		};
		if (pthread_create(&handlers[i], NULL, handler, &args) != 0) {
			goto error;
		}
		log_msg(LOG_INFO, "Worker thread %i started.", i);
	}

	if (mq_close(po_accepted_queue) == -1) {
		log_msg(LOG_ERR, "Main thread: Could not close accepted queue.");
		goto error;
	}

	if (mq_close(po_handled_queue) == -1) {
		log_msg(LOG_ERR, "Main thread: Could not close handled queue.");
		goto error;
	}

	for (i = 0; i < num_threads; i++) {
		pthread_join(handlers[i], NULL);
		log_msg(LOG_INFO, "Worker thread %i stopped.", i);
	}
	pthread_join(acceptor_enqueuer, NULL);
	log_msg(LOG_INFO, "Acceptor thread stopped.");

	close(*main_sock_fd);
	return 0;

error:
	perror("Server error");
	close(*main_sock_fd);
	return -1;
}

