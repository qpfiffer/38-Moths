// vim: noet ts=4 sw=4
#include <assert.h>
#include <stdio.h>
#include <string.h>
#define DEBUG
#include "utils.h"
#include "greshunkel.h"

const char document[] =
"<html>\n"
"	<body>\n"
"		xXx SCREAM _include.html xXx\n"
"		xXx LOOP i LOOP_TEST xXx\n"
"			<li>xXx @TEST xXx xXx @i xXx</li>\n"
"		xXx BBL xXx\n"
"		<span>This is the real xXx @TRICKY xXx xXx @ONE xXx</span>\n"
"		<p>This is a regular string: xXx @TEST xXx</p>\n"
"		<p>This is an integer: xXx @FAKEINT xXx</p>\n"
"		<ul>\n"
"		xXx LOOP i LOOP_TEST xXx\n"
"			<li>XxX return_z xXx @i xXx XxX</li>\n"
"		xXx BBL xXx\n"
"		</ul>\n"
"		<p>Context Interpolation:</p>\n"
"		<p>xXx @sub.name xXx - xXx @sub.other xXx</p>\n"
"		xXx UNLESS NOT @sub.name xXx\n"
"		<p>You should not see this. Ctext interp.</p>\n"
"		xXx ENDLESS xXx\n"

"		xXx UNLESS @sub.name xXx\n"
"		<p>You should see this. ctext interp.</p>\n"
"		xXx ENDLESS xXx\n"
"		<p>XxX return_hello doesnt_matter_at_all XxX</p>\n"
"		xXx LOOP subs SUB_LOOP_TEST xXx\n"
"			<p>FILTERS IN FILTERS IN LOOPS: XxX return_z F XxX</p>\n"
"			<p>XxX return_hello f XxX</p>\n"
"			<p>xXx @subs.name xXx - xXx @subs.other xXx</p>\n"
"		xXx UNLESS NOT @sub.name xXx\n"
"		<p>You should not see this. Ctext interp.</p>\n"
"		xXx ENDLESS xXx\n"
"		xXx UNLESS @sub.name xXx\n"
"		<p>You should see this. ctext interp.</p>\n"
"		xXx ENDLESS xXx\n"
"		xXx BBL xXx\n"

"		xXx UNLESS @dne xXx\n"
"		<p>You should see this. 1/3</p>\n"
"		xXx ENDLESS xXx\n"

"		xXx UNLESS NOT @dne xXx\n"
"		<p>You should not see this. 1/3</p>\n"
"		xXx ENDLESS xXx\n"

"		xXx UNLESS @falsey xXx\n"
"		<p>You should see this. 2/3</p>\n"
"		xXx ENDLESS xXx\n"

"		xXx UNLESS NOT @falsey xXx\n"
"		<p>You should not see this. 2/3</p>\n"
"		xXx ENDLESS xXx\n"

"		xXx UNLESS @truthy xXx\n"
"		<p>You should not see this. 3/3</p>\n"
"		xXx ENDLESS xXx\n"

"		xXx UNLESS NOT @truthy xXx\n"
"		<p>You should see this. 3/3</p>\n"
"		xXx ENDLESS xXx\n"
"	</body>\n"
"</html>\n";

char *return_z(const char *argument) {
	UNUSED(argument);
	return "z";
}

char *return_hello(const char *arg) {
	UNUSED(arg);
	return "HELLO!";
}

void test1() {
	size_t new_size = 0;

	greshunkel_ctext *ctext = gshkl_init_context();
	gshkl_add_filter(ctext, "return_z", &return_z, NULL);
	gshkl_add_filter(ctext, "return_hello", &return_hello, NULL);

	gshkl_add_string(ctext, "TEST", "This is a test.");
	gshkl_add_int(ctext, "FAKEINT", 666);

	gshkl_add_string(ctext, "truthy", "TRUE");
	gshkl_add_string(ctext, "falsey", "FALSE");

	gshkl_add_string(ctext, "TRICKY", "TrIcKy");
	gshkl_add_int(ctext, "ONE", 1);

	greshunkel_var loop_test = gshkl_add_array(ctext, "LOOP_TEST");

	gshkl_add_string_to_loop(&loop_test, "a");
	gshkl_add_string_to_loop(&loop_test, "b");
	gshkl_add_string_to_loop(&loop_test, "c");

	gshkl_add_int_to_loop(&loop_test, 1);
	gshkl_add_int_to_loop(&loop_test, 2);
	gshkl_add_int_to_loop(&loop_test, 3);

	greshunkel_ctext *sub = gshkl_init_context();
	gshkl_add_string(sub, "name", "test_name");
	gshkl_add_string(sub, "other", "other_value");
	gshkl_add_sub_context(ctext, "sub", sub);

	greshunkel_var sub_loop_test = gshkl_add_array(ctext, "SUB_LOOP_TEST");
	unsigned int i;
	for (i = 0; i < 3; i++) {
		greshunkel_ctext *sub = gshkl_init_context();
		gshkl_add_string(sub, "name", "I AM IN A LOOP!");
		gshkl_add_int(sub, "other", i);
		gshkl_add_sub_context_to_loop(&sub_loop_test, sub);
	}

	char *rendered = gshkl_render(ctext, document, strlen(document), &new_size);
	gshkl_free_context(ctext);

	printf("%s\n", rendered);
	free(rendered);
}

void test2() {
	const char templ_a[] = "xXx @image xXx";
	const char templ_b[] = "xXx @image_date xXx";

	greshunkel_ctext *ctext = gshkl_init_context();
	gshkl_add_string(ctext, "image", "test");
	gshkl_add_string(ctext, "image_date", "definitely_not_test");

	size_t a_siz = 0, b_siz = 0;
	char *a_rendered = gshkl_render(ctext, templ_a, strlen(templ_a), &a_siz);
	char *b_rendered = gshkl_render(ctext, templ_b, strlen(templ_b), &b_siz);
	gshkl_free_context(ctext);

	int ret = strcmp(a_rendered, b_rendered);
	assert(ret != 0);

	free(a_rendered);
	free(b_rendered);
}

int main(int argc, char *argv[]) {
	UNUSED(argc);
	UNUSED(argv);

	test1();
	test2();

	return 0;
}
