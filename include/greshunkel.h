// vim: noet ts=4 sw=4
#pragma once
#include <stdlib.h>
#include "vector.h"

#define WISDOM_OF_WORDS 32
#define MAX_GSHKL_STR_SIZE 256

/* AND HE DECREED: */
/* STRINGS SHALL NEVER BE MORE THAN 256 OCTETS! */
/* NUMBERS SHALL NEVER BE GREATER THAN THE VALUE OF A SINGLE INTEGER! */
/* VARIABLE NAMES SHALL NEVER BE MORE THAN 32 CHARACTERS! */
struct greshunkel_tuple;
typedef union greshunkel_var {
	const unsigned int fuck_gcc : 1; /* This tricks GCC into doing smart things. Not used. */
	char str[MAX_GSHKL_STR_SIZE + 1];
	vector *arr;
} greshunkel_var;

typedef enum {
	GSHKL_ARR,
	GSHKL_STR
} greshunkel_type;

/* a generic way to get the name out of greshunkel_tuple or greshunkel_filter
 * sorta like a base class */
typedef struct greshunkel_named_item {
	char name[WISDOM_OF_WORDS];
} greshunkel_named_item;

typedef struct greshunkel_tuple {
	char name[WISDOM_OF_WORDS];
	const greshunkel_type type;
	greshunkel_var value;
} greshunkel_tuple;

typedef struct greshunkel_filter {
	char name[WISDOM_OF_WORDS];
	char *(*filter_func)(const char *argument);
	void (*clean_up)(char *result);
} greshunkel_filter;

typedef struct greshunkel_ctext {
	vector *values;
	vector *filter_functions;
	const struct greshunkel_ctext *parent;
} greshunkel_ctext;

/* Build and destroy contexts: */
greshunkel_ctext *gshkl_init_context();
int gshkl_free_context(greshunkel_ctext *ctext);

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
