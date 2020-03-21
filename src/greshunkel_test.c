// vim: noet ts=4 sw=4
#include <assert.h>
#include <stdio.h>
#include <string.h>
#define DEBUG
#include "utils.h"
#include "greshunkel.h"

/* Taken from Lair... where it was taken from OlegDB */
#define run_test(test) printf("%s: ", #test);\
	test_return_val = test();\
	if (test_return_val != 0) {\
		tests_failed++;\
		printf("%c[%dmFailed.%c[%dm\n", 0x1B, 31, 0x1B, 0);\
	} else {\
		tests_run++;\
		printf("%c[%dmPassed.%c[%dm\n", 0x1B, 32, 0x1B, 0);\
	}


char *return_z(const char *argument) {
	UNUSED(argument);
	return "z";
}

char *return_hello(const char *arg) {
	UNUSED(arg);
	return "HELLO!";
}

int test_unless() {
	size_t new_size = 0;
	const char document[] = 
		"xXx UNLESS @dne xXx\n"
		"<p>You should see this. 1/3</p>\n"
		"xXx ENDLESS xXx\n"
		"xXx UNLESS NOT @dne xXx\n"
		"<p>You should not see this. 1/3</p>\n"
		"xXx ENDLESS xXx\n"
		"xXx UNLESS @falsey xXx\n"
		"<p>You should see this. 2/3</p>\n"
		"xXx ENDLESS xXx\n"
		"xXx UNLESS NOT @falsey xXx\n"
		"<p>You should not see this. 2/3</p>\n"
		"xXx ENDLESS xXx\n"
		"xXx UNLESS @truthy xXx\n"
		"<p>You should not see this. 3/3</p>\n"
		"xXx ENDLESS xXx\n"
		"xXx UNLESS NOT @truthy xXx\n"
		"<p>You should see this. 3/3</p>\n"
		"xXx ENDLESS xXx\n";

	const char document_correct[] = "\n<p>You should see this. 1/3</p>\n"
		"\n<p>You should see this. 2/3</p>\n"
		"\n<p>You should see this. 3/3</p>\n";

	greshunkel_ctext *ctext = gshkl_init_context();
	gshkl_add_string(ctext, "truthy", "TRUE");
	gshkl_add_string(ctext, "falsey", "FALSE");
	char *rendered = gshkl_render(ctext, document, strlen(document), &new_size);
	gshkl_free_context(ctext);

	int ret = strcmp(rendered, document_correct);
	if (ret) {
		free(rendered);
		return 1;
	}

	free(rendered);

	return 0;
}
int test_template_include() {
	size_t new_size = 0;
	const char document[] = "xXx SCREAM _include.html xXx";
	const char document_correct[] = 
"<p>This is a test include file with it's own context and strings and stuff.</p>\n"
"\n"
"    <li>This is a test. a</li>\n"
"    <li>This is a test. b</li>\n"
"    <li>This is a test. c</li>\n"
"    <li>This is a test. 1</li>\n"
"    <li>This is a test. 2</li>\n"
"    <li>This is a test. 3</li>";

	greshunkel_ctext *ctext = gshkl_init_context();
	gshkl_add_string(ctext, "TEST", "This is a test.");
	greshunkel_var loop_test = gshkl_add_array(ctext, "LOOP_TEST");

	gshkl_add_string_to_loop(&loop_test, "a");
	gshkl_add_string_to_loop(&loop_test, "b");
	gshkl_add_string_to_loop(&loop_test, "c");

	gshkl_add_int_to_loop(&loop_test, 1);
	gshkl_add_int_to_loop(&loop_test, 2);
	gshkl_add_int_to_loop(&loop_test, 3);

	char *rendered = gshkl_render(ctext, document, strlen(document), &new_size);
	gshkl_free_context(ctext);

	int ret = strcmp(rendered, document_correct);
	if (ret) {
		free(rendered);
		return 1;
	}

	free(rendered);

	return 0;
}

int test_filters() {
	size_t new_size = 0;
	const char document[] = "<li>XxX return_z xXx @ONE xXx XxX XxX return_z XxX</li>";

	greshunkel_ctext *ctext = gshkl_init_context();
	gshkl_add_int(ctext, "ONE", 1);
	gshkl_add_filter(ctext, "return_z", &return_z, NULL);

	char *rendered = gshkl_render(ctext, document, strlen(document), &new_size);
	gshkl_free_context(ctext);

	int ret = strcmp(rendered, "<li>z z</li>");
	if (ret) {
		free(rendered);
		return 1;
	}

	free(rendered);
	return 0;
}

int test_big_test() {
	size_t new_size = 0;
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

	const char document_correct[] = 
"<html>\n"
"	<body>\n"
"<p>This is a test include file with it's own context and strings and stuff.</p>\n"
"\n"
"    <li>This is a test. a</li>\n"
"    <li>This is a test. b</li>\n"
"    <li>This is a test. c</li>\n"
"    <li>This is a test. 1</li>\n"
"    <li>This is a test. 2</li>\n"
"    <li>This is a test. 3</li>"
"\n"
"			<li>This is a test. a</li>\n"
"		\n"
"			<li>This is a test. b</li>\n"
"		\n"
"			<li>This is a test. c</li>\n"
"		\n"
"			<li>This is a test. 1</li>\n"
"		\n"
"			<li>This is a test. 2</li>\n"
"		\n"
"			<li>This is a test. 3</li>\n"
"		\n"
"		<span>This is the real TrIcKy 1</span>\n"
"		<p>This is a regular string: This is a test.</p>\n"
"		<p>This is an integer: 666</p>\n"
"		<ul>\n"
"\n"
"			<li>z</li>\n"
"		\n"
"			<li>z</li>\n"
"		\n"
"			<li>z</li>\n"
"		\n"
"			<li>z</li>\n"
"		\n"
"			<li>z</li>\n"
"		\n"
"			<li>z</li>\n"
"		\n"
"		</ul>\n"
"		<p>Context Interpolation:</p>\n"
"		<p>test_name - other_value</p>\n"
"\n"
"		<p>You should see this. ctext interp.</p>\n"
"		\n"
"		<p>HELLO!</p>\n"
"\n"
"			<p>FILTERS IN FILTERS IN LOOPS: z</p>\n"
"			<p>HELLO!</p>\n"
"			<p>I AM IN A LOOP! - 0</p>\n"
"\n"
"		<p>You should see this. ctext interp.</p>\n"
"		\n"
"		\n"
"			<p>FILTERS IN FILTERS IN LOOPS: z</p>\n"
"			<p>HELLO!</p>\n"
"			<p>I AM IN A LOOP! - 1</p>\n"
"\n"
"		<p>You should see this. ctext interp.</p>\n"
"		\n"
"		\n"
"			<p>FILTERS IN FILTERS IN LOOPS: z</p>\n"
"			<p>HELLO!</p>\n"
"			<p>I AM IN A LOOP! - 2</p>\n"
"\n"
"		<p>You should see this. ctext interp.</p>\n"
"		\n"
"		\n"
"		<p>You should see this. 1/3</p>\n"
"		\n"
"		<p>You should see this. 2/3</p>\n"
"		\n"
"		<p>You should see this. 3/3</p>\n"
"		\n"
"	</body>\n"
"</html>\n";

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

	int ret = strcmp(rendered, document_correct);
	if (ret) {
		printf("CORRECT:\n%s\n", document_correct);
		printf("INCORRECT:\n%s\n", rendered);
		free(rendered);
		return 1;
	}

	free(rendered);
	return 0;
}

int test_variable_interpolation() {
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
	if (!ret) {
		free(a_rendered);
		free(b_rendered);
		return 1;
	}

	free(a_rendered);
	free(b_rendered);

	return 0;
}

int test_subcontext_interpolation() {
	size_t new_size = 0;
	const char document[] = 
		"xXx @image.name xXx\n"
		"xXx @image.id xXx";
	const char document_correct[] = "test_name\n1";

	greshunkel_ctext *ctext = gshkl_init_context();
	greshunkel_ctext *sub = gshkl_init_context();
	gshkl_add_string(sub, "name", "test_name");
	gshkl_add_int(sub, "id", 1);
	gshkl_add_sub_context(ctext, "image", sub);

	char *rendered = gshkl_render(ctext, document, strlen(document), &new_size);
	gshkl_free_context(ctext);

	int ret = strcmp(rendered, document_correct);
	if (ret) {
		free(rendered);
		return 1;
	}

	free(rendered);

	return 0;
}

int test_loop_in_unless() {
	size_t new_size = 0;
	const char document[] =
		"xXx UNLESS NOT @errors xXx\n"
		"    xXx LOOP error errors xXx\n"
		"        xXx @error xXx\n"
		"    xXx BBL xXx\n"
		"xXx ENDLESS xXx\n";
	const char document_correct[] = "\n        1\n    \n        2\n    \n        3\n    \n";

	greshunkel_ctext *ctext = gshkl_init_context();
	greshunkel_var loop = gshkl_add_array(ctext, "errors");
	gshkl_add_string_to_loop(&loop, "1");
	gshkl_add_string_to_loop(&loop, "2");
	gshkl_add_string_to_loop(&loop, "3");

	char *rendered = gshkl_render(ctext, document, strlen(document), &new_size);
	gshkl_free_context(ctext);

	int ret = strcmp(rendered, document_correct);
	if (ret) {
		free(rendered);
		return 1;
	}

	free(rendered);

	return 0;
}

int main(int argc, char *argv[]) {
	UNUSED(argc);
	UNUSED(argv);

	int test_return_val = 0 ;
	int tests_run = 0;
	int tests_failed = 0;

	printf("Running tests.\n");

	run_test(test_filters);
	run_test(test_template_include);
	run_test(test_variable_interpolation);
	run_test(test_subcontext_interpolation);
	run_test(test_unless);
	run_test(test_big_test);

	printf("Tests passed: (%i/%i).\n", tests_run, tests_run + tests_failed);

	if (tests_run != tests_run + tests_failed)
		return 1;
	return 0;
}
