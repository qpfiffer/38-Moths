// vim: noet ts=4 sw=4
#pragma once
#include "utils.h"

#define DEFAULT_NUM_THREADS 2

struct route;
int http_serve(int main_sock_fd, const int num_threads,
		const struct route *routes, const size_t num_routes);
