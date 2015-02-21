// vim: noet ts=4 sw=4
#ifdef __clang__
	#pragma clang diagnostic ignored "-Wmissing-field-initializers"
#endif
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <regex.h>

#include "greshunkel.h"

struct line {
	size_t size;
	char *data;
};

typedef struct line line;

static const char variable_regex[] = "xXx @([a-zA-Z_0-9]+) xXx";
static const char loop_regex[] = "^\\s+xXx LOOP ([a-zA-Z_]+) ([a-zA-Z_]+) xXx(.*)xXx BBL xXx";
static const char filter_regex[] = "XxX ([a-zA-Z_0-9]+) (.*) XxX";

greshunkel_ctext *gshkl_init_context() {
	greshunkel_ctext *ctext = calloc(1, sizeof(struct greshunkel_ctext));
	ctext->values = vector_new(sizeof(struct greshunkel_tuple), 32);
	ctext->filter_functions = vector_new(sizeof(struct greshunkel_filter), 8);
	return ctext;
}
static greshunkel_ctext *_gshkl_init_child_context(const greshunkel_ctext *parent) {
	greshunkel_ctext *ctext = gshkl_init_context();
	ctext->parent = parent;
	return ctext;
}

static inline int _gshkl_add_var_to_context(greshunkel_ctext *ctext, const greshunkel_tuple *new_tuple) {
	if (!vector_append(ctext->values, new_tuple, sizeof(struct greshunkel_tuple)))
		return 1;
	return 0;
}

static inline int _gshkl_add_var_to_loop(greshunkel_var *loop, const greshunkel_tuple *new_tuple) {
	if (!vector_append(loop->arr, new_tuple, sizeof(struct greshunkel_tuple)))
		return 1;
	return 0;
}

void filter_cleanup(char *result) {
	free(result);
}

int gshkl_add_filter(greshunkel_ctext *ctext,
		const char name[WISDOM_OF_WORDS],
		char *(*filter_func)(const char *argument),
		void (*clean_up)(char *filter_result)) {

	greshunkel_filter new_filter = {
		.filter_func = filter_func,
		.clean_up = clean_up,
		.name = {0}
	};
	strncpy(new_filter.name, name, sizeof(new_filter.name));

	vector_append(ctext->filter_functions, &new_filter, sizeof(struct greshunkel_filter));
	return 0;
}

int gshkl_add_string(greshunkel_ctext *ctext, const char name[WISDOM_OF_WORDS], const char value[MAX_GSHKL_STR_SIZE]) {
	assert(ctext != NULL);

	/* Create a new tuple to hold type and name and shit. */
	greshunkel_tuple _stack_tuple = {
		.name = {0},
		.type = GSHKL_STR,
		.value =  {0}
	};
	strncpy(_stack_tuple.name, name, WISDOM_OF_WORDS);

	/* Copy the value of the string into the var object: */
	greshunkel_var _stack_var = {0};
	strncpy(_stack_var.str, value, MAX_GSHKL_STR_SIZE);
	_stack_var.str[MAX_GSHKL_STR_SIZE] = '\0';

	/* Copy the var object itself into the tuple's var space: */
	memcpy(&_stack_tuple.value, &_stack_var, sizeof(greshunkel_var));

	/* Push that onto our values stack. */
	return _gshkl_add_var_to_context(ctext, &_stack_tuple);
}

int gshkl_add_int(greshunkel_ctext *ctext, const char name[WISDOM_OF_WORDS], const int value) {
	assert(ctext != NULL);

	greshunkel_tuple _stack_tuple = {
		.name = {0},
		.type = GSHKL_STR,
		.value = {0}
	};
	strncpy(_stack_tuple.name, name, WISDOM_OF_WORDS);

	greshunkel_var _stack_var = {0};
	snprintf(_stack_var.str, MAX_GSHKL_STR_SIZE, "%i", value);

	memcpy(&_stack_tuple.value, &_stack_var, sizeof(greshunkel_var));

	return _gshkl_add_var_to_context(ctext, &_stack_tuple);
}

greshunkel_var gshkl_add_array(greshunkel_ctext *ctext, const char name[WISDOM_OF_WORDS]) {
	assert(ctext != NULL);

	greshunkel_tuple _stack_tuple = {
		.name = {0},
		.type = GSHKL_ARR,
		.value = {
			.arr = vector_new(sizeof(greshunkel_tuple), 16)
		}
	};
	strncpy(_stack_tuple.name, name, WISDOM_OF_WORDS);

	_gshkl_add_var_to_context(ctext, &_stack_tuple);
	return _stack_tuple.value;
}

int gshkl_add_int_to_loop(greshunkel_var *loop, const int value) {
	assert(loop != NULL);

	greshunkel_tuple _stack_tuple = {
		.name = {0},
		.type = GSHKL_STR,
		.value = {0}
	};

	greshunkel_var _stack_var = {0};
	snprintf(_stack_var.str, MAX_GSHKL_STR_SIZE, "%i", value);

	memcpy(&_stack_tuple.value, &_stack_var, sizeof(greshunkel_var));

	return _gshkl_add_var_to_loop(loop, &_stack_tuple);
}

int gshkl_add_string_to_loop(greshunkel_var *loop, const char *value) {
	assert(loop != NULL);

	greshunkel_tuple _stack_tuple = {
		.name = {0},
		.type = GSHKL_STR,
		.value = {0}
	};

	greshunkel_var _stack_var = {0};
	strncpy(_stack_var.str, value, MAX_GSHKL_STR_SIZE);
	_stack_var.str[MAX_GSHKL_STR_SIZE] = '\0';

	memcpy(&_stack_tuple.value, &_stack_var, sizeof(union greshunkel_var));

	return _gshkl_add_var_to_loop(loop, &_stack_tuple);
}

static inline void _gshkl_free_arr(greshunkel_tuple *to_free) {
	assert(to_free->type == GSHKL_ARR);
	vector_free((vector *)to_free->value.arr);
}

int gshkl_free_context(greshunkel_ctext *ctext) {
	unsigned int i;
	for (i = 0; i < ctext->values->count; i++) {
		greshunkel_tuple *next = (greshunkel_tuple *)vector_get(ctext->values, i);
		if (next->type == GSHKL_ARR) {
			_gshkl_free_arr(next);
			continue;
		}
	}
	vector_free(ctext->values);
	vector_free(ctext->filter_functions);

	free(ctext);
	return 0;
}

static line read_line(const char *buf) {
	char c = '\0';

	size_t num_read = 0;
	while (1) {
		c = buf[num_read];
		num_read++;
		if (c == '\0' || c == '\n' || c == '\r')
			break;
	}

	line to_return = {
		.size = num_read,
		.data = calloc(1, num_read + 1)
	};
	strncpy(to_return.data, buf, num_read);

	return to_return;
}

static line
_filter_line(const greshunkel_ctext *ctext, const line *operating_line, const regex_t *filter_regex) {
	line to_return = {0};
	/* Now we match template filters: */
	regmatch_t filter_matches[3];
	/* TODO: More than one filter per line. */
	if (regexec(filter_regex, operating_line->data, 3, filter_matches, 0) == 0) {
		int matched_at_least_once = 0;
		const regmatch_t function_name = filter_matches[1];
		const regmatch_t argument = filter_matches[2];

		/* Render the argument out so we can pass it to the filter function. */
		char rendered_argument[argument.rm_eo - argument.rm_so];
		memset(rendered_argument, '\0', sizeof(rendered_argument));
		strncpy(rendered_argument,
				operating_line->data + argument.rm_so,
				sizeof(rendered_argument));
		rendered_argument[sizeof(rendered_argument)] = '\0';

		const greshunkel_ctext *current_ctext = ctext;
		while (current_ctext != NULL) {
			vector *current_funcs = current_ctext->filter_functions;
			unsigned int i;
			for (i = 0; i < current_funcs->count; i++) {
				greshunkel_filter *filter = (greshunkel_filter *)vector_get(current_funcs, i);
				int strncmp_res = strncmp(filter->name,
						operating_line->data + function_name.rm_so,
						strlen(filter->name));
				if (strncmp_res == 0) {
					/* Pass it to the filter function. */
					char *filter_result = filter->filter_func(rendered_argument);
					const size_t result_size = strlen(filter_result);

					const size_t first_piece_size = filter_matches[0].rm_so;
					const size_t middle_piece_size = result_size;
					const size_t last_piece_size = operating_line->size - filter_matches[0].rm_eo;
					/* Sorry, Vishnu... */
					to_return.size = first_piece_size + middle_piece_size + last_piece_size;
					to_return.data = calloc(1, to_return.size);

					strncpy(to_return.data, operating_line->data, first_piece_size);
					strncpy(to_return.data + first_piece_size, filter_result, middle_piece_size);
					strncpy(to_return.data + first_piece_size + middle_piece_size,
							operating_line->data + filter_matches[0].rm_eo,
							last_piece_size);
					if (filter->clean_up != NULL)
						filter->clean_up(filter_result);
					return to_return;
				}
			}
			current_ctext = current_ctext->parent;
		}
		assert(matched_at_least_once == 1);
	}

	/* We didn't match any filters. Just return the operating line. */
	return *operating_line;
}

static line
_interpolate_line(const greshunkel_ctext *ctext, const line current_line, const regex_t *var_regex, const regex_t *filter_regex) {
	line interpolated_line = {0};
	line new_line_to_add = {0};
	regmatch_t match[2];
	const line *operating_line = &current_line;
	assert(operating_line->data != NULL);

	while (regexec(var_regex, operating_line->data, 2, match, 0) == 0) {
		int matched_at_least_once = 0;

		/* We linearly search through our variables because I don't have
		 * a hash map. C is "fast enough" */
		const greshunkel_ctext *current_ctext = ctext;
		while (current_ctext != NULL) {
			/* We matched. */
			vector *current_values = current_ctext->values;
			unsigned int i;
			for (i = 0; i < current_values->count; i++) {
				const greshunkel_tuple *tuple = (greshunkel_tuple *)vector_get(current_values, i);
				/* This is the actual part of the regex we care about. */
				const regmatch_t inner_match = match[1];
				assert(inner_match.rm_so != -1 && inner_match.rm_eo != -1);

				assert(tuple->name != NULL);
				int strcmp_result = strncmp(tuple->name, operating_line->data + inner_match.rm_so, strlen(tuple->name));
				if (tuple->type == GSHKL_STR && strcmp_result == 0) {
					/* Do actual printing here */
					const size_t first_piece_size = match[0].rm_so;
					const size_t middle_piece_size = strlen(tuple->value.str);
					const size_t last_piece_size = operating_line->size - match[0].rm_eo;
					/* Sorry, Vishnu... */
					new_line_to_add.size = first_piece_size + middle_piece_size + last_piece_size;
					new_line_to_add.data = calloc(1, new_line_to_add.size + 1);

					strncpy(new_line_to_add.data, operating_line->data, first_piece_size);
					strncpy(new_line_to_add.data + first_piece_size, tuple->value.str, middle_piece_size);
					strncpy(new_line_to_add.data + first_piece_size + middle_piece_size,
							operating_line->data + match[0].rm_eo,
							last_piece_size);

					matched_at_least_once = 1;
					break;
				}
			}
			current_ctext = current_ctext->parent;
		}
		/* Blow up if we had a variable that wasn't in the context. */
		if (matched_at_least_once != 1) {
			printf("Did not match a variable that needed to be matched.\n");
			printf("Line: %s\n", operating_line->data);
			assert(matched_at_least_once == 1);
		}

		free(interpolated_line.data);
		interpolated_line.size = new_line_to_add.size;
		interpolated_line.data = new_line_to_add.data;
		new_line_to_add.size = 0;
		new_line_to_add.data = NULL;
		operating_line = &interpolated_line;

		/* Set the next regex check after this one. */
		memset(match, 0, sizeof(match));
	}
	line filtered_line = _filter_line(ctext, operating_line, filter_regex);
	if (filtered_line.data != interpolated_line.data && interpolated_line.data != NULL)
		free(operating_line->data);

	return filtered_line;
}

static line
_interpolate_loop(const greshunkel_ctext *ctext, const regex_t *lr, const regex_t *vr, const regex_t *fr, const char *buf, size_t *num_read) {
	line to_return = {0};
	*num_read = 0;

	regmatch_t match[4] = {{0}};
	/* TODO: Support loops inside of loops. That probably means a
	 * while loop here. */
	if (regexec(lr, buf, 4, match, 0) == 0) {
		/* Variables we're going to need: */
		const regmatch_t loop_variable = match[1];
		const regmatch_t variable_name = match[2];
		regmatch_t loop_meat = match[3];
		/* Make sure they were matched: */
		assert(variable_name.rm_so != -1 && variable_name.rm_eo != -1);
		assert(loop_variable.rm_so != -1 && loop_variable.rm_eo != -1);
		assert(loop_meat.rm_so != -1 && loop_meat.rm_eo != -1);

		size_t possible_dif = 0;
		const char *closest_BBL = NULL;
		closest_BBL = strstr(buf + loop_meat.rm_so, "xXx BBL xXx");
		possible_dif = closest_BBL - buf;
		if (possible_dif != (unsigned int)loop_meat.rm_so) {
			loop_meat.rm_eo = possible_dif;
		}

		/* We found a fucking loop, holy shit */
		*num_read = loop_meat.rm_eo + strlen("xXx BBL xXx");

		/* This is the thing we're going to render over and over and over again. */
		char loop_piece_to_render[loop_meat.rm_eo - loop_meat.rm_so];
		memset(loop_piece_to_render, '\0', sizeof(loop_piece_to_render));
		strncpy(loop_piece_to_render, buf + loop_meat.rm_so, sizeof(loop_piece_to_render));

		char loop_variable_name_rendered[WISDOM_OF_WORDS] = {0};
		const size_t _bigger = loop_variable.rm_eo - loop_variable.rm_so > WISDOM_OF_WORDS ? WISDOM_OF_WORDS :
			loop_variable.rm_eo - loop_variable.rm_so;
		strncpy(loop_variable_name_rendered, buf + loop_variable.rm_so, _bigger);

		line to_render_line = { .size = sizeof(loop_piece_to_render), .data = loop_piece_to_render };
		/* Now we start iterating through values in our context, looking for ARR
		 * types that have the correct name. */
		vector *current_values = ctext->values;

		/* We linearly search through our variables because I don't have
		 * a hash map. C is "fast enough" */
		int matched_at_least_once = 0;
		unsigned int i;
		for (i = 0; i < current_values->count; i++) {
			const greshunkel_tuple *tuple = vector_get(current_values, i);
			if (tuple->type != GSHKL_ARR)
				continue;

			/* Found an array. */
			assert(tuple->name != NULL);

			int strcmp_result = strncmp(tuple->name, buf + variable_name.rm_so, strlen(tuple->name));
			if (tuple->type == GSHKL_ARR && strcmp_result == 0) {
				matched_at_least_once = 1;

				vector *cur_vector_p = tuple->value.arr;
				/* Now we loop through the array incredulously. */
				unsigned int j;
				for (j = 0; j < cur_vector_p->count; j++) {
					const greshunkel_tuple *current_loop_var = vector_get(cur_vector_p, j);
					/* TODO: For now, only strings are supported in arrays. */
					assert(current_loop_var->type == GSHKL_STR);

					/* Recurse contexts until my fucking mind melts. */
					greshunkel_ctext *_temp_ctext = _gshkl_init_child_context(ctext);
					gshkl_add_string(_temp_ctext, loop_variable_name_rendered, current_loop_var->value.str);
					line rendered_piece = _interpolate_line(_temp_ctext, to_render_line, vr, fr);
					gshkl_free_context(_temp_ctext);

					const size_t old_size = to_return.size;
					to_return.size += rendered_piece.size;
					to_return.data = realloc(to_return.data, to_return.size);
					strncpy(to_return.data + old_size, rendered_piece.data, rendered_piece.size);
					free(rendered_piece.data);
				}
				break;
			}

		}
		if (matched_at_least_once != 1) {
			printf("Did not match a variable that needed to be matched.\n");
			printf("Line: %s\n", buf);
			assert(matched_at_least_once == 1);
		}
	}

	return to_return;
}

static inline void _compile_regex(regex_t *vr, regex_t *lr, regex_t *fr) {
	int reti = regcomp(vr, variable_regex, REG_EXTENDED);
	assert(reti == 0);

	reti = regcomp(lr, loop_regex, REG_EXTENDED);
	assert(reti == 0);

	reti = regcomp(fr, filter_regex, REG_EXTENDED);
	assert(reti == 0);
}

static inline void _destroy_regex(regex_t *vr, regex_t *lr, regex_t *fr) {
	regfree(vr);
	regfree(lr);
	regfree(fr);
}

char *gshkl_render(const greshunkel_ctext *ctext, const char *to_render, const size_t original_size, size_t *outsize) {
	assert(to_render != NULL);
	assert(ctext != NULL);

	/* We start up a new buffer and copy the old one into it: */
	char *rendered = NULL;
	*outsize = 0;

	regex_t var_regex, loop_regex, filter_regex;
	_compile_regex(&var_regex, &loop_regex, &filter_regex);

	size_t num_read = 0;
	while (num_read < original_size) {
		line current_line = read_line(to_render + num_read);

		line to_append = {0};
		size_t loop_readahead = 0;
		/* The loop needs to read more than the current line, so we pass
		 * in the offset and just let it go. If it processes more than the
		 * size of the current line, we know it did something.
		 * Append the whole line it gets back. */
		to_append = _interpolate_loop(ctext, &loop_regex, &var_regex, &filter_regex, to_render + num_read, &loop_readahead);

		/* Otherwise just interpolate the line like normal. */
		if (loop_readahead == 0) {
			to_append = _interpolate_line(ctext, current_line, &var_regex, &filter_regex);
			num_read += current_line.size;
		} else {
			num_read += loop_readahead;
		}

		/* Fuck this */
		const size_t old_outsize = *outsize;
		*outsize += to_append.size;
		{
			char *med_buf = realloc(rendered, *outsize);
			if (med_buf == NULL)
				goto error;
			rendered = med_buf;
		}
		strncpy(rendered + old_outsize, to_append.data, to_append.size);
		if (to_append.data != current_line.data)
			free(current_line.data);
		free(to_append.data);
	}
	_destroy_regex(&var_regex, &loop_regex, &filter_regex);
	rendered[*outsize - 1] = '\0';
	return rendered;

error:
	free(rendered);
	*outsize = 0;
	return NULL;
}
