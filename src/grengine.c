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

#include <oleg-http/http.h>

#include "grengine.h"
#include "greshunkel.h"
#include "utils.h"
#include "logging.h"

static const char r_200[] =
	"HTTP/1.1 200 OK\r\n"
	"Content-Type: %s\r\n"
	"Content-Length: %zu\r\n"
	"Connection: close\r\n"
	"Server: waifu.xyz/bitch\r\n\r\n";

static const char r_404[] =
	"HTTP/1.1 404 Not Found\r\n"
	"Content-Type: %s\r\n"
	"Content-Length: %zu\r\n"
	"Connection: close\r\n"
	"Server: waifu.xyz/bitch\r\n\r\n";

static const char r_206[] =
	"HTTP/1.1 206 Partial Content\r\n"
	"Content-Type: %s\r\n"
	"Content-Length: %zu\r\n"
	"Accept-Ranges: bytes\r\n"
	"Content-Range: bytes %zu-%zu/%zu\r\n"
	"Connection: close\r\n"
	"Server: waifu.xyz/bitch\r\n\r\n";

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
	{".js", "text/javascript"},
	{NULL, NULL}
};

/* Default 404 handler. */
int r_404_handler(const http_request *request, http_response *response);
static const route r_404_route = {
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
	{404, r_404}
};

const code_to_message *get_response_headers() {
	return response_headers;
}

size_t get_response_headers_num_elements() {
	return sizeof(response_headers)/sizeof(response_headers[0]);
}

int r_404_handler(const http_request *request, http_response *response) {
	UNUSED(request);
	response->out = (unsigned char *)"<h1>\"Welcome to Die|</h1>";
	response->outsize = strlen("<h1>\"Welcome to Die|</h1>");
	return 404;
}

static void guess_mimetype(const char *ending, const size_t ending_siz, http_response *response) {
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

int render_file(const struct greshunkel_ctext *ctext, const char *file_path, http_response *response) {
	int rc = mmap_file(file_path, response);
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

static int mmap_file_ol(const char *file_path, http_response *response,
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
		log_msg(LOG_ERR, "Could not mmap file: %s", buf);

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

int mmap_file(const char *file_path, http_response *response) {
	return mmap_file_ol(file_path, response, NULL, NULL);
}

void heap_cleanup(const int status_code, http_response *response) {
	if (RESPONSE_OK(status_code))
		free(response->out);
}

void mmap_cleanup(const int status_code, http_response *response) {
	if (RESPONSE_OK(status_code)) {
		munmap(response->out, response->outsize);
		free(response->extra_data);
	}
}

static int parse_request(const char to_read[MAX_READ_LEN], http_request *out) {
	/* Find the verb */
	const char *verb_end = strnstr(to_read, " ", MAX_READ_LEN);
	if (verb_end == NULL)
		goto error;

	const size_t c_verb_size = verb_end - to_read;
	const size_t verb_size = c_verb_size >= sizeof(out->verb) ? sizeof(out->verb) - 1: c_verb_size;
	strncpy(out->verb, to_read, verb_size);

	if (strncmp(out->verb, "GET", verb_size) != 0) {
		log_msg(LOG_WARN, "Don't know verb %s.", out->verb);
		goto error;
	}

	const char *res_offset = verb_end + sizeof(char);
	const char *resource_end = strnstr(res_offset, " ", sizeof(out->resource));
	if (resource_end == NULL)
		goto error;

	const size_t c_resource_size = resource_end - res_offset;
	const size_t resource_size = c_resource_size >= sizeof(out->resource) ? sizeof(out->resource) : c_resource_size;
	strncpy(out->resource, res_offset, resource_size);

	return 0;

error:
	return -1;
}

static void log_request(const http_request *request, const http_response *response, const int response_code) {
	char *visitor_ip_addr = get_header_value(request->full_header, strlen(request->full_header), "X-Real-IP");
	char *user_agent = get_header_value(request->full_header, strlen(request->full_header), "User-Agent");

	if (visitor_ip_addr == NULL)
		visitor_ip_addr = "NOIP";

	if (user_agent == NULL)
		user_agent = "NOUSERAGENT";

	log_msg(LOG_FUN, "%s \"%s %s\" %i %i \"%s\"",
		visitor_ip_addr, request->verb, request->resource,
		response_code, response->outsize, user_agent);

	if (strncmp(visitor_ip_addr, "NOIP", strlen("NOIP")) != 0)
		free(visitor_ip_addr);

	if (strncmp(user_agent, "NOUSERAGENT", strlen("NOUSERAGENT")) != 0)
		free(user_agent);
}

int respond(const int accept_fd, const route *all_routes, const size_t route_num_elements) {
	char *to_read = calloc(1, MAX_READ_LEN);
	char *actual_response = NULL;
	http_response response = {
		.mimetype = {0},
		0
	};
	const route *matching_route = NULL;

	int rc = recv(accept_fd, to_read, MAX_READ_LEN, 0);
	if (rc <= 0) {
		log_msg(LOG_ERR, "Did not receive any information from accepted connection.");
		goto error;
	}

	http_request request = {
		.verb = {0},
		.resource = {0},
		.matches = {{0}},
		.full_header = to_read
	};
	rc = parse_request(to_read, &request);
	if (rc != 0) {
		log_msg(LOG_ERR, "Could not parse request.");
		goto error;
	}

	/* Find our matching route: */
	unsigned int i;
	for (i = 0; i < route_num_elements; i++) {
		const route *cur_route = &all_routes[i];
		if (strcmp(cur_route->verb, request.verb) != 0)
			continue;

		assert(cur_route->expected_matches < MAX_MATCHES);
		regex_t regex;
		int reti = regcomp(&regex, cur_route->route_match, REG_EXTENDED);
		if (reti != 0) {
			char errbuf[128];
			regerror(reti, &regex, errbuf, sizeof(errbuf));
			log_msg(LOG_ERR, "%s", errbuf);
			assert(reti == 0);
		}

		if (cur_route->expected_matches > 0)
			reti = regexec(&regex, request.resource, cur_route->expected_matches + 1, request.matches, 0);
		else
			reti = regexec(&regex, request.resource, 0, NULL, 0);
		regfree(&regex);
		if (reti == 0) {
			matching_route = &all_routes[i];
			break;
		}
	}

	/* If we didn't find one just use the 404 route: */
	if (matching_route == NULL)
		matching_route = &r_404_route;

	/* Run the handler through with the data we have: */
	log_msg(LOG_INFO, "Calling handler for %s.", matching_route->name);
	int response_code = matching_route->handler(&request, &response);

	if (response_code == 404 && (response.outsize == 0 || response.out == NULL)) {
		response_code = r_404_handler(&request, &response);
	} else {
		assert(response.outsize > 0);
		assert(response.out != NULL);
	}

	/* Embed the handler's text into the header: */
	size_t header_size = 0;
	size_t actual_response_siz = 0;

	/* Figure out if this thing needs to be partial */
	char *range_header_value = get_header_value(request.full_header, strlen(request.full_header), "Range");
	if (range_header_value && RESPONSE_OK(response_code)) {
		response_code = 206;
	}

	/* Figure out what header we need to use: */
	const code_to_message *matched_response = NULL;
	const code_to_message *response_headers = get_response_headers();
	const unsigned int num_elements = get_response_headers_num_elements();
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

	if (response_code == 200 || response_code == 404) {
		const size_t integer_length = UINT_LEN(response.outsize);
		header_size = strlen(response.mimetype) + strlen(matched_response->message)
			+ integer_length - strlen("%s") - strlen("%zu");
		actual_response_siz = response.outsize + header_size;
		actual_response = malloc(actual_response_siz + 1);
		actual_response[actual_response_siz] = '\0';

		/* snprintf the header because it's just a string: */
		snprintf(actual_response, actual_response_siz, matched_response->message, response.mimetype, response.outsize);

		/* memcpy the rest because it could be anything: */
		memcpy(actual_response + header_size, response.out, response.outsize);
	} else if (response_code == 206) {
		/* Byte range queries have some extra shit. */
		range_header byte_range = parse_range_header(range_header_value);

		log_msg(LOG_INFO, "Range header parsed: Raw: %s Limit: %zu Offset: %zu", range_header_value, byte_range.limit, byte_range.offset);
		free(range_header_value);

		const size_t c_offset = byte_range.offset;
		const size_t c_limit = byte_range.limit == 0 ?
			(response.outsize - c_offset) - 1 : (byte_range.limit - c_offset) - 1;
		const size_t full_size = c_limit + 1;
		const size_t integer_length = UINT_LEN(full_size);

		const size_t minb_len = c_offset == 0 ? 1 : UINT_LEN(c_offset);
		const size_t maxb_len = c_limit == 0 ? 1 : UINT_LEN(c_limit);
		/* Compute the size of the header */
		header_size = strlen(response.mimetype) + strlen(matched_response->message)
			+ integer_length + minb_len + maxb_len + integer_length
			- strlen("%s") - (strlen("%zu") * 4);
		actual_response_siz = full_size + header_size;
		/* malloc the full response */
		actual_response = malloc(actual_response_siz + 1);
		actual_response[actual_response_siz] = '\0';

		/* snprintf the header because it's just a string: */
		snprintf(actual_response, actual_response_siz, matched_response->message,
			response.mimetype, full_size,
			c_offset, c_limit, full_size);
		/* memcpy the rest because it could be anything: */
		memcpy(actual_response + header_size, response.out + c_offset, full_size);
	}

	log_request(&request, &response, response_code);

	/* Send that shit over the wire: */
	const size_t bytes_siz = actual_response_siz;
	rc = send(accept_fd, actual_response, bytes_siz, 0);
	if (rc <= 0) {
		log_msg(LOG_ERR, "Could not send response.");
		goto error;
	}
	if (matching_route->cleanup != NULL) {
		log_msg(LOG_INFO, "Calling cleanup for %s.", matching_route->name);
		matching_route->cleanup(response_code, &response);
	}
	free(actual_response);
	free(to_read);

	return 0;

error:
	if (matching_route != NULL)
		matching_route->cleanup(500, &response);
	free(actual_response);
	free(to_read);
	return -1;
}
