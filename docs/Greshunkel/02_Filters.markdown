GRESHUNKEL has the ability to blend in custom filters (Think Django-style Python
functions that can be called while evaluating a template). Here's a trivial
example:

```HTML
<!DOCTYPE html>
<!-- index.html -->
<html>
	<body>
		<p>XxX return_z "Test Argument" XxX</p>"
		<p>XxX return_z xXx @i xXx XxX</p>"
	</body>
<html>
```

And then for the C portion:

```C
/* The filter function itself. */
char *return_z(const char *argument) {
	UNUSED(argument);
	return "z";
}

/* The template handling code. */
int template_example(const m38_http_request *request, m38_http_response *response) {
	greshunkel_ctext *ctext = gshkl_init_context();
	gshkl_add_filter(ctext, "return_z", &return_z, NULL);
	gshkl_add_int(ctext, "i", 10);
	return m38_render_file(ctext, "./index.html", response);
}
```

Theres a couple things going on here, but it's pretty simple. GRESHUNKEL filters
are things that just take string arguments and return strings. You have the
option to pass in a clean-up handler when adding the filter, which is an
opportune time to add something to `free()` memory.
