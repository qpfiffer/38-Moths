// vim: noet ts=4 sw=4
#ifdef __clang__
	#pragma clang diagnostic ignored "-Wmissing-field-initializers"
#endif
#include <assert.h>
#include <fcntl.h>
#include <regex.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <unistd.h>

#include "parse.h"
#include "grengine.h"
#include "greshunkel.h"
#include "utils.h"
#include "logging.h"
#include "vector.h"
#include "types.h"

static const char r_200[] =
	"HTTP/1.1 200 OK\r\n"
	"Content-Type: %s\r\n"
	"Content-Length: %zu\r\n"
	"Connection: close\r\n";

static const char r_301[] =
	"HTTP/1.1 301 Moved\r\n"
	"Content-Type: %s\r\n"
	"Content-Length: %zu\r\n"
	"Connection: close\r\n";

static const char r_302[] =
	"HTTP/1.1 302 Found\r\n"
	"Content-Type: %s\r\n"
	"Content-Length: %zu\r\n"
	"Connection: close\r\n";

static const char r_404[] =
	"HTTP/1.1 404 Not Found\r\n"
	"Content-Type: %s\r\n"
	"Content-Length: %zu\r\n"
	"Connection: close\r\n";

static const char r_206[] =
	"HTTP/1.1 206 Partial Content\r\n"
	"Content-Type: %s\r\n"
	"Content-Length: %zu\r\n"
	"Accept-Ranges: bytes\r\n"
	"Content-Range: bytes %zu-%zu/%zu\r\n"
	"Connection: close\r\n";

static const char r_final[] = "Server: 38-Moths\r\n\r\n";

/* internal struct, for mimetype guesses */
struct {
	char *ext;
	char *type;
} mimetype_mapping[] = {
	{".css", "text/css"},
	{".jpg", "image/jpeg"},
	{".txt", "text/plain"},
	{".html", "text/html"},
	{".svg", "image/svg+xml"},
	{".ico", "image/x-icon"},
	{".webm", "video/webm"},
	{".gif", "image/gif"},
	{".json", "application/json"},
	{".js", "text/javascript"},
	{NULL, NULL}
};

/* Default 404 handler. */
int r_404_handler(const m38_http_request *request, m38_http_response *response);
static const m38_route r_404_route = {
	.verb = "GET",
	.route_match = "^.*$",
	.handler = (&r_404_handler),
	.cleanup = NULL
};

/* This is used to map between the return codes of responses to their headers: */
typedef struct {
	const int code;
	const char *message;
} code_to_message;

static const code_to_message response_headers[] = {
	{200, r_200},
	{206, r_206},
	{301, r_301},
	{302, r_302},
	{404, r_404}
};

const code_to_message *get_response_headers() {
	return response_headers;
}

size_t get_response_headers_num_elements() {
	return sizeof(response_headers)/sizeof(response_headers[0]);
}

int r_404_handler(const m38_http_request *request, m38_http_response *response) {
	UNUSED(request);
	response->out = (unsigned char *)"<h1>\"Welcome to Die|</h1>";
	response->outsize = strlen("<h1>\"Welcome to Die|</h1>");
	return 404;
}

static void guess_mimetype(const char *ending, const size_t ending_siz, m38_http_response *response) {
	int i;
	char *type = "application/octet-stream";
	for (i = 0; mimetype_mapping[i].ext; i++) {
		if (strncasecmp(ending, mimetype_mapping[i].ext, ending_siz) == 0) {
			type = mimetype_mapping[i].type;
			break;
		}
	}
	strncpy(response->mimetype, type, sizeof(response->mimetype));
}

int m38_return_raw_buffer(const char *buf, const size_t buf_size, m38_http_response *response) {
	response->outsize = buf_size;
	response->out = (unsigned char *)buf;

	return 200;
}

int m38_render_file(const struct greshunkel_ctext *ctext, const char *file_path, m38_http_response *response) {
	int rc = m38_mmap_file(file_path, response);
	if (!RESPONSE_OK(rc))
		return rc;

	/* Render the mmap()'d file with greshunkel */
	const char *mmapd_region = (char *)response->out;
	const size_t original_size = response->outsize;

	size_t new_size = 0;
	char *rendered = gshkl_render(ctext, mmapd_region, original_size, &new_size);
	gshkl_free_context((greshunkel_ctext *)ctext);

	/* Clean up the stuff we're no longer using. */
	munmap(response->out, original_size);
	free(response->extra_data);

	/* Make sure the response is kept up to date: */
	response->outsize = new_size;
	response->out = (unsigned char *)rendered;

	return 200;
}

static int mmap_file_ol(const char *file_path, m38_http_response *response,
				 const size_t *offset, const size_t *limit) {
	response->extra_data = calloc(1, sizeof(struct stat));

	if (stat(file_path, response->extra_data) == -1) {
		response->out = (unsigned char *)"<html><body><p>No such file.</p></body></html>";
		response->outsize= strlen("<html><body><p>No such file.</p></body></html>");
		free(response->extra_data);
		response->extra_data = NULL;
		return 404;
	}
	int fd = open(file_path, O_RDONLY);
	if (fd <= 0) {
		response->out = (unsigned char *)"<html><body><p>Could not open file.</p></body></html>";
		response->outsize= strlen("<html><body><p>could not open file.</p></body></html>");
		perror("mmap_file_ol: could not open file");
		free(response->extra_data);
		response->extra_data = NULL;
		close(fd);
		return 404;
	}


	const struct stat st = *(struct stat *)response->extra_data;

	const size_t c_offset = offset != NULL ? *offset : 0;
	const size_t c_limit = limit != NULL ? (*limit - c_offset) : (st.st_size - c_offset);

	response->out = mmap(NULL, c_limit, PROT_READ, MAP_PRIVATE, fd, c_offset);
	response->outsize = c_limit - c_offset;

	if (response->out == MAP_FAILED) {
		char buf[128] = {0};
		perror(buf);
		m38_log_msg(LOG_ERR, "Could not mmap file: %s", buf);

		response->out = (unsigned char *)"<html><body><p>Could not open file.</p></body></html>";
		response->outsize= strlen("<html><body><p>could not open file.</p></body></html>");
		close(fd);
		free(response->extra_data);
		response->extra_data = NULL;
		return 404;
	}
	close(fd);

	madvise(response->out, c_limit, MADV_SEQUENTIAL | MADV_WILLNEED);

	/* Figure out the mimetype for this resource: */
	char ending[16] = {0};
	unsigned int i = sizeof(ending);
	int found_dot = 0;
	const size_t res_len = strlen(file_path);
	for (i = res_len; i > (res_len - sizeof(ending)); i--) {
		if (file_path[i] == '.') {
			found_dot = 1;
			break;
		}
	}
	if (strlen(response->mimetype) == 0) {
		if (found_dot) {
			strncpy(ending, file_path + i, sizeof(ending));
			guess_mimetype(ending, sizeof(ending), response);
		} else {
			/* Fuck it, do something smart later with render_file. */
			strncpy(response->mimetype, "text/html", sizeof(response->mimetype));
		}
	}

	return 200;
}

int m38_mmap_file(const char *file_path, m38_http_response *response) {
	return mmap_file_ol(file_path, response, NULL, NULL);
}

void m38_heap_cleanup(const int status_code, m38_http_response *response) {
	if (RESPONSE_OK(status_code))
		free(response->out);
}

void m38_mmap_cleanup(const int status_code, m38_http_response *response) {
	if (RESPONSE_OK(status_code)) {
		munmap(response->out, response->outsize);
		free(response->extra_data);
	}
}

int m38_insert_custom_header(m38_http_response *response, const char *header, const size_t header_len,
		const char *value, const size_t value_len) {
	if (!header || !value || !response)
		return 0;

	m38_header_pair *new_pair = calloc(1, sizeof(m38_header_pair));
	if (!new_pair) {
		m38_log_msg(LOG_ERR, "Could not create header_pair for http_response extra headers!");
		return 0;
	}

	const char *copied_header = strndup(header, header_len);
	const char *copied_value = strndup(value, value_len);
	if (!copied_header || !copied_value) {
		m38_log_msg(LOG_ERR, "Could not create copied values for http_response extra headers!");
		free(new_pair);
		return 0;
	}

	m38_header_pair _stack_pair = {
		.header = copied_header,
		.header_len = header_len,
		.value = copied_value,
		.value_len = value_len,
	};
	memcpy(new_pair, &_stack_pair, sizeof(m38_header_pair));
	vector_append_ptr(response->extra_headers, new_pair);

	return 0;
}

static void log_request(const m38_http_request *request, const m38_http_response *response, const int response_code) {
	char *visitor_ip_addr = m38_get_header_value_raw(request->full_header, request->header_len, "X-Real-IP");
	char *user_agent = m38_get_header_value_raw(request->full_header, request->header_len, "User-Agent");

	m38_log_msg(LOG_FUN, "%s \"%s %s\" %i %i \"%s\"",
		visitor_ip_addr == NULL ? "NOIP" : visitor_ip_addr,
		request->verb,
		request->resource,
		response_code,
		response->outsize,
		user_agent == NULL ? "NOUSERAGENT" : user_agent);

	free(visitor_ip_addr);
	free(user_agent);
}

static int read_max_bytes_from_socket(const int accept_fd, unsigned char **to_read, size_t *num_read) {
	size_t read_this_time = 0;
	while ((read_this_time = recv(accept_fd, *to_read + *num_read, MAX_READ_LEN, 0))) {
		*num_read += read_this_time;
		if (read_this_time == 0 || read_this_time < MAX_READ_LEN)
			break;
		read_this_time = 0;

		*to_read = realloc(*to_read, *num_read + MAX_READ_LEN);
		if (!(*to_read)) {
			m38_log_msg(LOG_ERR, "Ran out of memory reading request.");
			goto error;
		}
		memset(*to_read + *num_read, '\0', MAX_READ_LEN);
		if (*num_read >= MAX_REQUEST_SIZE)
			break;
	}

	return 0;

error:
	return -1;
}

static int attempt_reread_from_socket(
		const int accept_fd, const size_t clength_num,
		const size_t post_body_len, m38_http_request *out) {

	unsigned char *to_read = NULL;
	size_t num_read = 0;

	/* If we have a post_body_len > 0, we don't have to reread from the socket. */
	/* In addition, if we have a longer content-length than we have known post-body
	 * length, than we should attempt to re-read from the client. */
	if ((post_body_len <= 0 && clength_num > 0) ||
		(post_body_len > 0 && post_body_len < clength_num)) {

		to_read = calloc(1, MAX_READ_LEN);

		m38_log_msg(LOG_WARN, "Content-Length is %i but we got a post_body_len of %i.", clength_num, post_body_len);
		m38_log_msg(LOG_WARN, "Attempting to re-read from stream.", clength_num, post_body_len);
		/* ------------------------------------- */
		/* XXX: Do a select here with a timeout. */
		/* ------------------------------------- */
		int rc = read_max_bytes_from_socket(accept_fd, &to_read, &num_read);
		if (rc != 0)
			goto error;
	}

	out->full_body = to_read;
	out->body_len = num_read;
	return 0;

error:
	free(to_read);
	return -1;
}

m38_handled_request *m38_generate_response(const int accept_fd, const m38_app *app) {
	unsigned char *to_read = calloc(1, MAX_READ_LEN);
	char *actual_response = NULL;
	m38_http_response response = {
		.mimetype = {0},
		0
	};
	response.extra_headers = vector_new(sizeof(m38_header_pair *), 4);

	m38_handled_request *hreq = NULL;
	const m38_route *matching_route = NULL;
	size_t num_read = 0;
	int rc = 0;

	rc = read_max_bytes_from_socket(accept_fd, &to_read, &num_read);
	if (rc != 0)
		goto error;

	m38_http_request request = {
		.verb = {0},
		.resource = {0},
		.matches = {{0}},
		.header_len = 0,
		.full_header = NULL,
		.body_len = 0,
		.full_body = NULL
	};

	rc = m38_parse_request(to_read, num_read, &request);
	if (rc != 0) {
		m38_log_msg(LOG_ERR, "Could not parse request.");
		goto error;
	}

	char *clength = m38_get_header_value_raw(request.full_header, request.header_len, "Content-Length");
	size_t clength_num = 0;
	if (clength == NULL) {
		m38_log_msg(LOG_WARN, "Could not parse content length.");
	} else {
		clength_num = atoi(clength);
		free(clength);
	}

	/* Do we have a POST body or something? */
	const size_t post_body_len = num_read - request.header_len;
	if (post_body_len <= 0 && clength_num > 0) {
		/* re-read will just copy into the m38_http_request. */
		rc = attempt_reread_from_socket(accept_fd, clength_num, post_body_len, &request);
		if (rc != 0) {
			m38_log_msg(LOG_ERR, "Could not reread from socket and parse body.");
			goto error;
		}
	} else {
		rc = m38_parse_body(post_body_len, clength_num, to_read, &request);
		if (rc != 0) {
			m38_log_msg(LOG_ERR, "Could not parse body.");
			goto error;
		}
	}

	/* Parses form encoding, if applicable: */
	const char form_encoding[] = "application/x-www-form-urlencoded";
	const size_t form_encoding_siz = sizeof(form_encoding);
	char *content_type = m38_get_header_value_request(&request, "Content-Type");
	if (content_type &&
			strlen(content_type) == form_encoding_siz &&
			strncmp(form_encoding, content_type, form_encoding_siz)) {
		rc = m38_parse_form_encoded_body(&request);
		if (rc != 0) {
			/* application/x-www-form-urlencoded */
			m38_log_msg(LOG_ERR, "Could not parse form-urlencoded body.");
			goto error;
		}
	
	}

	/* Find our matching route: */
	int64_t i;
	for (i = 0; i < app->num_routes; i++) {
		const m38_route *cur_route = &app->routes[i];
		if (strcmp(cur_route->verb, request.verb) != 0)
			continue;

		assert(cur_route->expected_matches < MAX_MATCHES);
		regex_t regex;
		int reti = regcomp(&regex, cur_route->route_match, REG_EXTENDED);
		if (reti != 0) {
			char errbuf[128];
			regerror(reti, &regex, errbuf, sizeof(errbuf));
			m38_log_msg(LOG_ERR, "%s", errbuf);
			assert(reti == 0);
		}

		if (cur_route->expected_matches > 0)
			reti = regexec(&regex, request.resource, cur_route->expected_matches + 1, request.matches, 0);
		else
			reti = regexec(&regex, request.resource, 0, NULL, 0);
		regfree(&regex);
		if (reti == 0) {
			matching_route = &app->routes[i];
			break;
		}
	}

	/* If we didn't find one just use the 404 route: */
	int64_t response_code = 0;
	if (matching_route == NULL) {
		matching_route = &r_404_route;
		if (app->r_404_handler) {
			response_code = app->r_404_handler(&request, &response);
		} else {
			m38_log_msg(LOG_INFO, "Calling internal 404 handler for %s.", matching_route->name);
			response_code = matching_route->handler(&request, &response);
		}
	} else {
		/* Run the handler through with the data we have: */
		m38_log_msg(LOG_INFO, "Calling handler for %s.", matching_route->name);
		response_code = matching_route->handler(&request, &response);
	}

	if (response_code == 404 && (response.outsize == 0 || response.out == NULL)) {
		if (app->r_404_handler) {
			response_code = app->r_404_handler(&request, &response);
		} else {
			response_code = r_404_handler(&request, &response);
		}
	}

	/* Embed the handler's text into the header: */
	size_t header_size = 0;
	size_t actual_response_siz = 0;

	/* Figure out if this thing needs to be partial */
	char *range_header_value = m38_get_header_value_raw(request.full_header, request.header_len, "Range");
	if (range_header_value && RESPONSE_OK(response_code)) {
		response_code = 206;
	}

	/* Figure out what header we need to use: */
	const code_to_message *matched_response = NULL;
	const code_to_message *response_headers = get_response_headers();
	const int64_t num_elements = get_response_headers_num_elements();
	for (i = 0; i < num_elements; i++) {
		code_to_message current_response = response_headers[i];
		if (current_response.code == response_code) {
			matched_response = &response_headers[i];
			break;
		}
	}
	/* Blow up if we don't have that code. We should have them all at
	 * compile time. */
	assert(matched_response != NULL);

	if (response_code == 200 || response_code == 404 || response_code == 301 || response_code == 302) {
		const size_t integer_length = UINT_LEN(response.outsize);
		if (response.mimetype[0] == '\0')
			memcpy(response.mimetype, "text/html", strlen("text/html"));

		header_size = strlen(response.mimetype) + strlen(matched_response->message)
			+ integer_length - strlen("%s") - strlen("%zu") + strlen(r_final);

		size_t i = 0;
		for (i = 0; i < response.extra_headers->count; i++) {
			m38_header_pair **pair = (m38_header_pair **)vector_get(response.extra_headers, i);
			header_size += (*pair)->header_len + strlen(": ") + (*pair)->value_len
								    + strlen("\r\n");
		}
		actual_response_siz = response.outsize + header_size;
		actual_response = malloc(actual_response_siz + 1);
		memset(actual_response, '\0', actual_response_siz);
		actual_response[actual_response_siz] = '\0';

		/* snprintf the header because it's just a string: */
		snprintf(actual_response, actual_response_siz, matched_response->message, response.mimetype, response.outsize);
		for (i = 0; i < response.extra_headers->count; i++) {
			m38_header_pair **pair = (m38_header_pair **)vector_get(response.extra_headers, i);
			const size_t siz = (*pair)->header_len + strlen(": ") + (*pair)->value_len
								    + strlen("\r\n") + 1;
			char buf[siz];
			buf[siz] = '\0';
			snprintf(buf, actual_response_siz, "%s: %s\r\n", (*pair)->header, (*pair)->value);
			strncat(actual_response, buf, siz);
		}
		strcat(actual_response, r_final);

		/* memcpy the rest because it could be anything: */
		memcpy(actual_response + header_size, response.out, response.outsize);
	} else if (response_code == 206) {
		/* Byte range queries have some extra shit. */
		m38_range_header byte_range = m38_parse_range_header(range_header_value);
		/* TODO: Add the custom header stuff here (for 206), too. */

		m38_log_msg(LOG_INFO, "Range header parsed: Raw: %s Limit: %zu Offset: %zu", range_header_value, byte_range.limit, byte_range.offset);
		free(range_header_value);

		const size_t start_byte = byte_range.offset < response.outsize ? byte_range.offset : 0;
		const size_t end_byte = byte_range.limit == 0 ?
			response.outsize - 1: (response.outsize - byte_range.limit) - 1;
		const size_t max_size = response.outsize; /* Should be the same as file size. */
		const size_t integer_length = UINT_LEN(max_size);

		const size_t minb_len = start_byte == 0 ? 1 : UINT_LEN(start_byte);
		const size_t maxb_len = end_byte == 0 ? 1 : UINT_LEN(end_byte);
		/* Compute the size of the header */
		header_size = strlen(response.mimetype) + strlen(matched_response->message) + strlen(r_final)
			+ integer_length + minb_len + maxb_len + integer_length
			- strlen("%s") - (strlen("%zu") * 4);
		actual_response_siz = max_size + header_size;
		/* malloc the full response */
		actual_response = malloc(actual_response_siz + 1);
		actual_response[actual_response_siz] = '\0';

		/* snprintf the header because it's just a string: */
		snprintf(actual_response, actual_response_siz, matched_response->message,
			response.mimetype, max_size,
			start_byte, end_byte, max_size);
		m38_log_msg(LOG_INFO, "Sending back: Content-Range: %zu-%zu/%zu", start_byte, end_byte, max_size);

		strcat(actual_response, r_final);

		/* memcpy the rest because it could be anything: */
		memcpy(actual_response + header_size, response.out + start_byte, end_byte - start_byte);
	}

	log_request(&request, &response, response_code);

	hreq = malloc(sizeof(m38_handled_request));
	hreq->response_bytes = actual_response;
	hreq->response_len = actual_response_siz;
	hreq->response_code = response_code;
	hreq->accept_fd = accept_fd;
	hreq->sent = 0;
	hreq->matching_route = matching_route;
	memcpy(&hreq->response, &response, sizeof(response));

	free(to_read);
	free(request.full_header);
	free(request.full_body);
	free(request.form_elements);
	return hreq;

error:
	if (matching_route != NULL && matching_route->cleanup != NULL)
		matching_route->cleanup(500, &response);
	free(actual_response);
	free(to_read);
	free(request.full_header);
	free(request.full_body);
	free(request.form_elements);
	return NULL;
}

static void _clean_up_extra_headers(m38_http_response *response) {
	size_t i = 0;
	for (i = 0; i < response->extra_headers->count; i++) {
		m38_header_pair **pair = (m38_header_pair **)vector_get(response->extra_headers, i);
		free((char *)(*pair)->header);
		free((char *)(*pair)->value);
		free(*pair);
	}
	vector_free(response->extra_headers);
}

m38_handled_request *m38_send_response(m38_handled_request *hreq) {
	/* Send that shit over the wire: */
	const m38_route *matching_route = hreq->matching_route;

	const size_t bytes_left = hreq->response_len - hreq->sent;
	const size_t to_send = bytes_left < 4096 ? bytes_left : 4096;
	int rc = send(hreq->accept_fd, hreq->response_bytes + hreq->sent, to_send, 0);
	if (rc <= 0) {
		m38_log_msg(LOG_ERR, "Could not send response.");
		goto error;
	}

	hreq->sent += to_send;

	if (hreq->response_len - hreq->sent <= 0) {
		if (matching_route->cleanup != NULL) {
			m38_log_msg(LOG_INFO, "Calling cleanup for %s.", matching_route->name);
			if (matching_route->cleanup)
				matching_route->cleanup(hreq->response_code, &hreq->response);
		}
		_clean_up_extra_headers(&hreq->response);
		close(hreq->accept_fd);
		free(hreq->response_bytes);
		free(hreq);

		return NULL;
	}

	return hreq;

error:
	if (matching_route != NULL && matching_route->cleanup != NULL)
		matching_route->cleanup(500, &hreq->response);
	_clean_up_extra_headers(&hreq->response);
	close(hreq->accept_fd);
	free(hreq->response_bytes);
	free(hreq);

	return NULL;
}
