# 38-moths

![What](./screenshot.png?raw=true)

Bizarre web framework. Do not be afraid.

# Installation

You'll need to install some [dependencies.](http://vodka.shithouse.tv/) Then you
can just:

```
make
sudo make install
```

# Why

I wanted a minimal, fast webserver/framework thing that I could just install,
link with and go. No package management shit, no weird languages-of-the-week to
install, just C. It should be a minimal scaffold to build shit off of that I can
run in a tmux session _ad infinitum_. It should have zero dependencies, because
its just a goddamn webserver. I don't need boost, a weird allocator, stupid
google dependencies or anything else. It's just sockets and bytes and shit. It's
POSIX as all hell.

# Usage

```C
#include <string.h>

#include <38-moths/38-moths.h>

int main_sock_fd;

static int index_handler(const http_request *request, http_response *response) {
	const unsigned char buf[] = "Hello, World!";
	response->out = malloc(sizeof(buf));
	memcpy(response->out, buf, sizeof(buf));

	response->outsize = sizeof(buf);
	return 200;
}

static const route all_routes[] = {
	{"GET", "root_handler", "^/$", 0, &index_handler, &heap_cleanup},
};

int main(int argc, char *argv[]) {
	http_serve(&main_sock_fd, 2, all_routes, sizeof(all_routes)/sizeof(all_routes[0]));
	return 0;
}
```

See the examples directory for more, or check out [waifu.xyz](https://github.com/qpfiffer/waifu.xyz)
for a large example.

# Tests

Currently there are only tests for the `GRESHUNKEL` templating language. The
`Makefile` spits out a `greshunkel_test` binary that you can run.
