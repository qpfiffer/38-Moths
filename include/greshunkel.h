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
#define MAX_GSHKL_STR_SIZE 256

/* AND HE DECREED: */
/* STRINGS SHALL NEVER BE MORE THAN 256 OCTETS! */
/* NUMBERS SHALL NEVER BE GREATER THAN THE VALUE OF A SINGLE INTEGER! */
/* VARIABLE NAMES SHALL NEVER BE MORE THAN 32 CHARACTERS! */
struct greshunkel_tuple;

/* xXx UNION=greshunkel_var xXx
 * xXx DESCRIPTION=A variable in GRESHUNKEL. xXx
 * xXx str[MAX_GSHKL_STR_SIZE + 1]=If this var is a string, this will hold the string. xXx
 * xXx *arr=If this var is an array, this will hold the vector for that array. xXx
 */
typedef union greshunkel_var {
	const unsigned int fuck_gcc : 1; /* This tricks GCC into doing smart things. Not used. */
	char str[MAX_GSHKL_STR_SIZE + 1];
	vector *arr;
} greshunkel_var;

/* xXx ENUM=greshunkel_type xXx
 * xXx DESCRIPTION=Used to tell <code>greshunkel_var</code>s apart. xXx
 * xXx GSHKL_ARR=A greshunkel array. xXx
 * xXx GSHKL_STR=A greshunkel string. xXx
 */
typedef enum {
	GSHKL_ARR,
	GSHKL_STR
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

/* Add things to the contexts: */
int gshkl_add_string(greshunkel_ctext *ctext, const char name[WISDOM_OF_WORDS], const char *value);
int gshkl_add_int(greshunkel_ctext *ctext, const char name[WISDOM_OF_WORDS], const int value);

/* Loop management functions: */
greshunkel_var gshkl_add_array(greshunkel_ctext *ctext, const char name[WISDOM_OF_WORDS]);
int gshkl_add_string_to_loop(greshunkel_var *loop, const char *value);
int gshkl_add_int_to_loop(greshunkel_var *loop, const int value);

/* Filters */
/* The clean_up function will be called after the result of your filter function
 * has been used. It can be NULL'd out if you don't need it. */
int gshkl_add_filter(greshunkel_ctext *ctext,
		const char name[WISDOM_OF_WORDS],
		char *(*filter_func)(const char *argument),
		void (*clean_up)(char *filter_result));
/* Commonly used filter that just free()'s the result. */
void filter_cleanup(char *result);

/* Render a string buffer: */
char *gshkl_render(const greshunkel_ctext *ctext, const char *to_render, const size_t original_size, size_t *outsize);
