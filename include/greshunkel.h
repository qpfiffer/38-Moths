// vim: noet ts=4 sw=4
#pragma once
#include <regex.h>
#include <stdlib.h>
#include "vector.h"

/* xXx DEFINE=WISDOM_OF_WORDS xXx
 * xXx DESCRIPTION=The maximum size of a greshunkel variable name. xXx
 */
#define WISDOM_OF_WORDS 32

/* xXx DEFINE=MAX_GSHKL_STR_SIZE xXx
 * xXx DESCRIPTION=The maximum size of a greshunkel string. xXx
 */
#define MAX_GSHKL_STR_SIZE 512

/* AND HE DECREED: */
/* STRINGS SHALL NEVER BE MORE THAN 256 OCTETS! */
/* NUMBERS SHALL NEVER BE GREATER THAN THE VALUE OF A SINGLE INTEGER! */
/* VARIABLE NAMES SHALL NEVER BE MORE THAN 32 CHARACTERS! */
struct greshunkel_tuple;

/* xXx STRUCT=greshunkel_ctext xXx
 * xXx DESCRIPTION=The context is a collection of variables, filters and other stuff that will be used to render a file. xXx
 * xXx *values=The values stored in this context. Strings, arrays, etc. xXx
 * xXx *filter_functions=The filter functions stored in this context. xXx
 * xXx *parent=Used internally, this is used to recurse back up to the parent context if a variable is unavailable in the current one. xXx
 */
typedef struct greshunkel_ctext {
	vector *values;
	vector *filter_functions;
	const struct greshunkel_ctext *parent;
} greshunkel_ctext;

/* xXx UNION=greshunkel_var xXx
 * xXx DESCRIPTION=A variable in GRESHUNKEL. xXx
 * xXx str[MAX_GSHKL_STR_SIZE + 1]=If this var is a string, this will hold the string. xXx
 * xXx *arr=If this var is an array, this will hold the vector for that array. xXx
 * xXx *sub_ctext=If this var is a sub context, this will hold the sub context for the variable. xXx
 */
typedef union greshunkel_var {
	char str[MAX_GSHKL_STR_SIZE + 1];
	vector *arr;
	const greshunkel_ctext *sub_ctext;
} greshunkel_var;

/* xXx ENUM=greshunkel_type xXx
 * xXx DESCRIPTION=Used to tell <code>greshunkel_var</code>s apart. xXx
 * xXx GSHKL_ARR=A greshunkel array. xXx
 * xXx GSHKL_STR=A greshunkel string. xXx
 */
typedef enum {
	GSHKL_ARR,
	GSHKL_STR,
	GSHKL_SUBCTEXT
} greshunkel_type;

/* xXx STRUCT=greshunkel_named_item xXx
 * xXx DESCRIPTION=Dirty fucking hack to have a generic baseclass-like thing for both <code>greshunkel_tuple</code> and <code>greshunkel_filter</code> objects. xXx
 * xXx name[WISDOM_OF_WORDS]=The name of this thing. Somehow works. C is fucking scary. xXx
 */
typedef struct greshunkel_named_item {
	char name[WISDOM_OF_WORDS];
} greshunkel_named_item;

/* xXx STRUCT=greshunkel_tuple xXx
 * xXx DESCRIPTION=Basically the representation of a GRESHUNKEL variable inside of a context. xXx
 * xXx name[WISDOM_OF_WORDS]=The name of this thing. xXx
 * xXx type=The type of this thing. xXx
 * xXx value=The actual value of this thing. xXx
 */
typedef struct greshunkel_tuple {
	char name[WISDOM_OF_WORDS];
	const greshunkel_type type;
	greshunkel_var value;
} greshunkel_tuple;

/* xXx STRUCT=greshunkel_filter xXx
 * xXx DESCRIPTION=A function that can be applied during template rendering. xXx
 * xXx name[WISDOM_OF_WORDS]=The name of this thing. xXx
 * xXx *(*filter_func)=Function pointer to the C function you want to have access to in the template. xXx
 * xXx (*clean_up)=If your filter needs any kind of cleanup, set this to a function other than null and you can do whatever. xXx
 */
typedef struct greshunkel_filter {
	char name[WISDOM_OF_WORDS];
	char *(*filter_func)(const char *argument);
	void (*clean_up)(char *result);
} greshunkel_filter;

/* xXx FUNCTION=gshkl_init_context xXx
 * xXx DESCRIPTION=Used to create a new, empty GRESHUNKEL context. xXx
 * xXx RETURNS=A new greshunkel context. xXx
 */
greshunkel_ctext *gshkl_init_context();

/* xXx FUNCTION=gshkl_free_context xXx
 * xXx DESCRIPTION=Frees an cleans up a previously created context. xXx
 * xXx RETURNS=Nothing. xXx
 */
void gshkl_free_context(greshunkel_ctext *ctext);

/* xXx FUNCTION=gshkl_add_string xXx
 * xXx DESCRIPTION=Adds a string with the given name to a context. xXx
 * xXx RETURNS=0 on success, 1 otherwise. xXx
 * xXx *ctext=The context to add the string to. xXx
 * xXx name[WISDOM_OF_WORDS]=The name used to reference this variable later. xXx
 * xXx *value=The NULL terminated string that will be returned later. xXx
 */
int gshkl_add_string(greshunkel_ctext *ctext, const char name[WISDOM_OF_WORDS], const char *value);

/* xXx FUNCTION=gshkl_add_int xXx
 * xXx DESCRIPTION=Adds an integer with the given name to a context. xXx
 * xXx RETURNS=0 on success, 1 otherwise. xXx
 * xXx *ctext=The context to add the integer to. xXx
 * xXx name[WISDOM_OF_WORDS]=The name used to reference this variable later. xXx
 * xXx value=The integer that will be added to this context. xXx
 */
int gshkl_add_int(greshunkel_ctext *ctext, const char name[WISDOM_OF_WORDS], const int value);

/* Array management */

/* xXx FUNCTION=gshkl_add_array xXx
 * xXx DESCRIPTION=Creates a new array object inside of the given context. xXx
 * xXx RETURNS=The newly created loop object. xXx
 * xXx *ctext=The context to add the array to. xXx
 * xXx name[WISDOM_OF_WORDS]=The name used to reference this variable later. xXx
 */
greshunkel_var gshkl_add_array(greshunkel_ctext *ctext, const char name[WISDOM_OF_WORDS]);

/* xXx FUNCTION=gshkl_add_string_to_loop xXx
 * xXx DESCRIPTION=Adds a string to a greshunkel array. xXx
 * xXx RETURNS=0 on success, 1 otherwise. xXx
 * xXx *loop=A pointer to a loop created with <code>gshkl_add_array</code>. xXx
 * xXx *value=The NULL terminated string to be added. xXx
 */
int gshkl_add_string_to_loop(greshunkel_var *loop, const char *value);

/* xXx FUNCTION=gshkl_add_int_to_loop xXx
 * xXx DESCRIPTION=Adds an integer to a greshunkel array. xXx
 * xXx RETURNS=0 on success, 1 otherwise. xXx
 * xXx *loop=A pointer to a loop created with <code>gshkl_add_array</code>. xXx
 * xXx value=The integer to be added. xXx
 */
int gshkl_add_int_to_loop(greshunkel_var *loop, const int value);

/* xXx FUNCTION=gshkl_add_sub_context_to_loop xXx
 * xXx DESCRIPTION=Adds a name sub-context to a loop. xXx
 * xXx RETURNS=0 on success, 1 otherwise. xXx
 * xXx *loop=The loop to add the context to. xXx
 * xXx *child=The pre-built child context. xXx
 */
int gshkl_add_sub_context_to_loop(greshunkel_var *loop, const greshunkel_ctext *child);

/* Sub-context management */

/* xXx FUNCTION=gshkl_add_sub_context xXx
 * xXx DESCRIPTION=Adds a name sub-context to a parent context. Sub-context values can be references via the 'xXx @<name>.<value> xXx' syntax. Contexts added in this way will be freed by the parent. xXx
 * xXx RETURNS=0 on success, 1 otherwise. xXx
 * xXx *parent=The parent context to add the child to. xXx
 * xXx name=The name of the child context. xXx
 * xXx *child=The pre-built child context. xXx
 */
int gshkl_add_sub_context(greshunkel_ctext *parent, const char name[WISDOM_OF_WORDS], const greshunkel_ctext *child);

/* Filters management */

/* xXx FUNCTION=gshkl_add_filter xXx
 * xXx DESCRIPTION=Adds a filter function to the given context. xXx
 * xXx RETURNS=0 on success, 1 otherwise. xXx
 * xXx *ctext=The context that the filter will be added to. xXx
 * xXx name[WISDOM_OF_WORDS]=The name used to reference this filter function. xXx
 * xXx (*filter_func)=The function that will be called from the template. xXx
 * xXx (*clean_up)=The clean up function that will be called after GRESHUNKEL is done calling <code>filter_func</code>. xXx
 */
int gshkl_add_filter(greshunkel_ctext *ctext,
		const char name[WISDOM_OF_WORDS],
		char *(*filter_func)(const char *argument),
		void (*clean_up)(char *filter_result));

/* xXx FUNCTION=filter_cleanup xXx
 * xXx DESCRIPTION=Commonly used helper clean-up function that just calls free on the result. xXx
 * xXx RETURNS=Nothing. xXx
 * xXx *result=The value obtained from calling your filter function. xXx
 */
void filter_cleanup(char *result);

/* xXx FUNCTION=gshkl_render xXx
 * xXx DESCRIPTION=Renders a template context. xXx
 * xXx RETURNS=The rendered template. xXx
 * xXx *ctext=The context that will be used to obtain values and filters for the given template. xXx
 * xXx *to_render=The template to render. xXx
 * xXx original_size=The size of the <code>to_render</code> buffer. xXx
 * xXx *outsize=If non-null, this will be the size of the returned buffer. xXx
 */
char *gshkl_render(const greshunkel_ctext *ctext, const char *to_render, const size_t original_size, size_t *outsize);
