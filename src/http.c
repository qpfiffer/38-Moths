// vim: noet ts=4 sw=4
#ifdef __clang__
	#pragma clang diagnostic ignored "-Wmissing-field-initializers"
#endif
#include <arpa/inet.h>
#include <netdb.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#include "utils.h"
#include "http.h"
#include "parse.h"
#include "logging.h"

char *receive_chunked_http(const int request_fd) {
	char *raw_buf = NULL;
	size_t buf_size = 0;
	int times_read = 0;

	fd_set chan_fds;
	FD_ZERO(&chan_fds);
	FD_SET(request_fd, &chan_fds);
	const int maxfd = request_fd;

	while (1) {
		times_read++;

		/* Wait for data to be read. */
		struct timeval tv = {
			.tv_sec = SELECT_TIMEOUT,
			.tv_usec = 0
		};
		select(maxfd + 1, &chan_fds, NULL, NULL, &tv);

		int count;
		/* How many bytes should we read: */
		ioctl(request_fd, FIONREAD, &count);
		if (count <= 0)
			break;
		int old_offset = buf_size;
		buf_size += count;
		if (raw_buf != NULL) {
			char *new_buf = realloc(raw_buf, buf_size);
			if (new_buf != NULL) {
				raw_buf = new_buf;
			} else {
				goto error;
			}
		} else {
			raw_buf = calloc(1, buf_size);
		}

		int recvd = recv(request_fd, raw_buf + old_offset, count, 0);
		if (recvd != count) {
			log_msg(LOG_WARN, "Could not receive entire message.");
		}
	}
	/* printf("Full message is %s\n.", raw_buf); */
	/* Check for a 200: */
	if (raw_buf == NULL || strstr(raw_buf, "200") == NULL) {
		log_msg(LOG_WARN, "Chunked: Could not find 200 return code in response.");
		goto error;
	}

	/* 4Chan throws us data as chunk-encoded HTTP. Rad. */
	char *header_end = strstr(raw_buf, "\r\n\r\n");
	char *cursor_pos = header_end  + (sizeof(char) * 4);

	size_t json_total = 0;
	char *json_buf = NULL;

	while (1) {
		/* This is where the data begins. */
		char *chunk_size_start = cursor_pos;
		char *chunk_size_end = strstr(chunk_size_start, "\r\n");
		const int chunk_size_end_oft = chunk_size_end - chunk_size_start;

		/* We cheat a little and set the first \r to a \0 so strtol will
		 * do the right thing. */
		chunk_size_start[chunk_size_end_oft] = '\0';
		const unsigned int chunk_size = strtol(chunk_size_start, NULL, 16);

		/* printf("Chunk size is %i. Thing is: %s.\n", chunk_size, chunk_size_start); */
		/* The chunk string, the \r\n after it, the chunk itself and then another \r\n: */
		cursor_pos += chunk_size + chunk_size_end_oft + 4;

		/* Copy the json into a pure buffer: */
		unsigned int old_offset = json_total;
		json_total += chunk_size;
		if (json_total >= old_offset) {
			if (json_buf != NULL) {
				char *new_buf = realloc(json_buf, json_total);
				if (new_buf != NULL) {
					json_buf = new_buf;
				} else {
					goto error;
				}
			} else {
				json_buf = calloc(1, json_total);
			}
		}
		/* Copy it from after the <chunk_size>\r\n to the end of the chunk. */
		memcpy(json_buf + old_offset, chunk_size_end + 2, chunk_size);
		/* Stop reading if we am play gods: */
		if ((unsigned int)(cursor_pos - raw_buf) > buf_size || chunk_size <= 0)
			break;
	}
	/* printf("The total json size is %zu.\n", json_total); */
	/* printf("JSON:\n%s", json_buf); */
	free(raw_buf);

	char *new_buf = realloc(json_buf, json_total + 1);
	if (new_buf == NULL) {
		free(json_buf);
		return NULL;
	}

	new_buf[json_total] = '\0';
	return new_buf;

error:
	if (raw_buf != NULL)
		free(raw_buf);
	return NULL;
}

int connect_to_host_with_port(const char *host, const char *port) {
	struct addrinfo hints = {0};
	struct addrinfo *res = NULL;
	int request_fd = -1;

	hints.ai_family = AF_INET;
	hints.ai_socktype = SOCK_STREAM;

	const int gai_rc = getaddrinfo(host, port, &hints, &res);
	if (gai_rc != 0) {
		log_msg(LOG_ERR, "Could not get address information: %s", gai_strerror(gai_rc));
		goto error;
	}

	request_fd = socket(res->ai_family, res->ai_socktype, res->ai_protocol);
	if (request_fd == -1) {
		char buf[128] = {0};
		perror(buf);
		log_msg(LOG_ERR, "Could not create socket: %s", buf);
		goto error;
	}

	int opt = 1;
	setsockopt(request_fd, SOL_SOCKET, SO_REUSEADDR, (void*) &opt, sizeof(opt));

	int rc = connect(request_fd, res->ai_addr, res->ai_addrlen);
	if (rc == -1) {
		log_msg(LOG_ERR, "Could not connect to host %s.", host);
		goto error;
	}

	freeaddrinfo(res);
	return request_fd;

error:
	freeaddrinfo(res);
	close(request_fd);
	return -1;
}

int connect_to_host(const char *host) {
	return connect_to_host_with_port(host, "80");
}

unsigned char *receive_http(const int request_fd, size_t *out) {
	return receive_http_with_timeout(request_fd, SELECT_TIMEOUT, out);
}

char *get_header_value(const char *request, const size_t request_siz, const char header[static 1]) {
	char *data = NULL;
	const char *header_loc = strnstr(request, header, request_siz);
	if (!header_loc)
		return NULL;

	const char *header_value_start = header_loc + strlen(header) + strlen(": ");
	const char *header_value_end = strstr(header_loc, "\r\n");
	if (!header_value_end)
		return NULL;

	const size_t header_value_size = header_value_end - header_value_start;
	data = malloc(header_value_size + 1);
	data[header_value_size] = '\0';
	memcpy(data, header_value_start, header_value_size);

	return data;
}

char *receive_only_http_header(const int request_fd, const int timeout, size_t *out) {
	unsigned char *raw_buf = NULL;
	size_t buf_size = 0;
	int times_read = 0;

	fd_set chan_fds;
	FD_ZERO(&chan_fds);
	FD_SET(request_fd, &chan_fds);
	const int maxfd = request_fd;

	struct timeval tv = {
		.tv_sec = timeout,
		.tv_usec = 0
	};
	select(maxfd + 1, &chan_fds, NULL, NULL, &tv);

	unsigned char *header_end = NULL;
	size_t total_received = 0;
	while (1) {
		times_read++;

		int count;
		/* How many bytes should we read: */
		ioctl(request_fd, FIONREAD, &count);
		if (count <= 0)
			break;
		else if (count <= 0) /* Continue waiting. */
			continue;
		int old_offset = buf_size;
		buf_size += count;
		if (raw_buf != NULL) {
			unsigned char *new_buf = realloc(raw_buf, buf_size);
			if (new_buf != NULL) {
				raw_buf = new_buf;
			} else {
				goto error;
			}
		} else {
			raw_buf = calloc(1, buf_size);
		}
		/* printf("IOCTL: %i.\n", count); */

		int received = recv(request_fd, raw_buf + old_offset, count, 0);
		total_received += received;

		/* Attempt to find the header length. */
		if (header_end == NULL) {
			header_end = (unsigned char *)strstr((char *)raw_buf, "\r\n\r\n");
			if (header_end != NULL)
				break;
		}
	}

	if (raw_buf == NULL)
		goto error;

	const size_t header_size = header_end - raw_buf;
	char *to_return = malloc(header_size);
	memcpy(to_return, raw_buf, header_size);
	*out = header_size;
	free(raw_buf);

	return to_return;

error:
	if (raw_buf != NULL)
		free(raw_buf);
	return NULL;
}

unsigned char *receive_http_with_timeout(const int request_fd, const int timeout, size_t *out) {
	unsigned char *raw_buf = NULL;
	size_t buf_size = 0;
	int times_read = 0;

	fd_set chan_fds;
	FD_ZERO(&chan_fds);
	FD_SET(request_fd, &chan_fds);
	const int maxfd = request_fd;


	/* Wait for data to be read. */
	struct timeval tv = {
		.tv_sec = timeout,
		.tv_usec = 0
	};
	select(maxfd + 1, &chan_fds, NULL, NULL, &tv);

	unsigned char *header_end = NULL, *cursor_pos = NULL;
	size_t result_size = 0;
	size_t payload_received = 0;
	size_t total_received = 0;
	while (1) {
		times_read++;

		int count;
		/* How many bytes should we read: */
		ioctl(request_fd, FIONREAD, &count);
		if (count <= 0 && result_size == payload_received)
			break;
		else if (count <= 0) /* Continue waiting. */
			continue;
		int old_offset = buf_size;
		buf_size += count;
		if (raw_buf != NULL) {
			unsigned char *new_buf = realloc(raw_buf, buf_size);
			if (new_buf != NULL) {
				raw_buf = new_buf;
			} else {
				goto error;
			}
		} else {
			raw_buf = calloc(1, buf_size);
		}
		/* printf("IOCTL: %i.\n", count); */

		int received = recv(request_fd, raw_buf + old_offset, count, 0);
		total_received += received;
		if (header_end)
			payload_received += received;

		/* Attempt to find the header length. */
		if (header_end == NULL) {
			header_end = (unsigned char *)strstr((char *)raw_buf, "\r\n\r\n");
			if (header_end != NULL) {
				cursor_pos = header_end  + (sizeof(char) * 4);
				/* Try to find a 200 or a 201: */

				const char *first_line_end_c = strstr((char *)raw_buf, "\r\n");
				const size_t first_line_end = first_line_end_c - (char *)raw_buf;
				if (strnstr((char *)raw_buf, "200", first_line_end) == NULL && strnstr((char *)raw_buf, "201", first_line_end) == NULL) {
					log_msg(LOG_WARN, "HTTP: Could not find 200 return code in response.");
					goto error;
				}

				/* We've received at least part of the payload, add it to the counter. */
				payload_received += total_received - (cursor_pos - raw_buf);

				result_size = 0;
				unsigned char *offset_for_clength = (unsigned char *)strnstr((char *)raw_buf, "Content-Length: ", buf_size);
				if (offset_for_clength == NULL) {
					continue;
				}

				char siz_buf[128] = {0};
				unsigned int i = 0;

				const unsigned char *to_read = offset_for_clength + strlen("Content-Length: ");
				while (to_read[i] != '\r' && to_read[i + 1] != '\n' && i < sizeof(siz_buf)) {
					siz_buf[i] = to_read[i];
					i++;
				}
				result_size = strtol(siz_buf, NULL, 10);
				/* log_msg(LOG_INFO, "Content length is %lu bytes.", result_size); */
			}
		}
	}

	if (!result_size)
		goto error;

	if (raw_buf == NULL)
		goto error;

	/* Make sure cursor_pos is up to date, no invalidated
	 * pointers or anything. */
	if (header_end != NULL) {
		header_end = (unsigned char *)strstr((char *)raw_buf, "\r\n\r\n");
		cursor_pos = header_end  + (sizeof(char) * 4);
	}

	unsigned char *to_return = malloc(result_size + 1);
	memcpy(to_return, cursor_pos, result_size);
	*out = result_size;
	to_return[result_size] = '\0';
	free(raw_buf);

	return to_return;

error:
	if (raw_buf != NULL)
		free(raw_buf);
	return NULL;
}
