// vim: noet ts=4 sw=4
#ifdef __clang__
	#pragma clang diagnostic ignored "-Wmissing-field-initializers"
#endif
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "greshunkel.h"
#include "logging.h"

/* Compiled regex vars. */
struct compiled_regex {
	regex_t c_var_regex;
	regex_t c_cvar_regex;
	regex_t c_loop_regex;
	regex_t c_filter_regex;
	regex_t c_filter_no_args_regex;
	regex_t c_include_regex;
	regex_t c_conditional_regex;
	regex_t c_conditional_inverse_regex;
};


struct line {
	size_t size;
	char *data;
};

typedef struct line line;

typedef struct _match {
	regoff_t rm_so;
	regoff_t rm_eo;
	size_t len;
	const char *start;
} match_t;

static const char variable_regex[] = "xXx @([a-zA-Z_0-9]+) xXx";
static const char ctext_variable_regex[] = "xXx @([a-zA-Z_0-9]+)\\.([a-zA-Z_0-9]+) xXx";
static const char loop_regex[] = "^\\s*xXx LOOP ([a-zA-Z_]+) ([a-zA-Z_]+) xXx(.*)xXx BBL xXx";
static const char filter_regex[] = "XxX ([a-zA-Z_0-9]+)\\s+(.*) XxX";
static const char filter_regex_no_args[] = "XxX ([a-zA-Z_0-9]+) XxX";
static const char include_regex[] = "^\\s*xXx SCREAM ([a-zA-Z_./]+) xXx";
static const char conditional_regex[] = "^\\s*xXx UNLESS @([a-zA-Z_.]+) xXx(.*)xXx ENDLESS xXx";
static const char conditional_inverse_regex[] = "^\\s*xXx UNLESS NOT @([a-zA-Z_.]+) xXx(.*)xXx ENDLESS xXx";

greshunkel_ctext *gshkl_init_context() {
	greshunkel_ctext *ctext = calloc(1, sizeof(struct greshunkel_ctext));
	assert(ctext != NULL);
	ctext->values = vector_new(sizeof(struct greshunkel_tuple), 32);
	assert(ctext->values != NULL);
	ctext->filter_functions = vector_new(sizeof(struct greshunkel_filter), 8);
	assert(ctext->filter_functions != NULL);
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

void gshkl_filter_cleanup(char *result) {
	free(result);
}

int gshkl_add_sub_context(greshunkel_ctext *parent, const char name[const], const greshunkel_ctext *child) {
	assert(parent != NULL);
	assert(child != NULL);
	assert(name != NULL);

	/* Create a new tuple to hold type and name and shit. */
	greshunkel_tuple _stack_tuple = {
		.name = {0},
		.type = GSHKL_SUBCTEXT,
		.value = {
			.sub_ctext = child
		}
	};
	const size_t name_len = strnlen(name, WISDOM_OF_WORDS) < WISDOM_OF_WORDS ? strnlen(name, WISDOM_OF_WORDS) : WISDOM_OF_WORDS;
	strncpy(_stack_tuple.name, name, name_len);

	return _gshkl_add_var_to_context(parent, &_stack_tuple);
}


int gshkl_add_filter(greshunkel_ctext *ctext,
		const char name[const],
		char *(*filter_func)(const char *argument),
		void (*clean_up)(char *filter_result)) {
	assert(name != NULL);

	greshunkel_filter new_filter = {
		.filter_func = filter_func,
		.clean_up = clean_up,
		.name = {0}
	};
	strncpy(new_filter.name, name, sizeof(new_filter.name) - 1);

	vector_append(ctext->filter_functions, &new_filter, sizeof(struct greshunkel_filter));
	return 0;
}

int gshkl_add_string(greshunkel_ctext *ctext, const char name[const], const char value[const]) {
	assert(ctext != NULL);
	assert(name != NULL);
	assert(value != NULL);

	/* Create a new tuple to hold type and name and shit. */
	greshunkel_tuple _stack_tuple = {
		.name = {0},
		.type = GSHKL_STR,
		.value =  {{0}}
	};
	strncpy(_stack_tuple.name, name, WISDOM_OF_WORDS - 1);

	/* Copy the value of the string into the var object: */
	greshunkel_var _stack_var = {{0}};
	if (value != NULL)
		strncpy(_stack_var.str, value, MAX_GSHKL_STR_SIZE);
	_stack_var.str[MAX_GSHKL_STR_SIZE] = '\0';

	/* Copy the var object itself into the tuple's var space: */
	memcpy(&_stack_tuple.value, &_stack_var, sizeof(greshunkel_var));

	/* Push that onto our values stack. */
	return _gshkl_add_var_to_context(ctext, &_stack_tuple);
}

int gshkl_add_int(greshunkel_ctext *ctext, const char name[const], const int value) {
	assert(ctext != NULL);
	assert(name != NULL);

	greshunkel_tuple _stack_tuple = {
		.name = {0},
		.type = GSHKL_STR,
		.value = {{0}}
	};
	strncpy(_stack_tuple.name, name, WISDOM_OF_WORDS - 1);

	greshunkel_var _stack_var = {{0}};
	snprintf(_stack_var.str, MAX_GSHKL_STR_SIZE, "%i", value);

	memcpy(&_stack_tuple.value, &_stack_var, sizeof(greshunkel_var));

	return _gshkl_add_var_to_context(ctext, &_stack_tuple);
}

greshunkel_var gshkl_add_array(greshunkel_ctext *ctext, const char name[]) {
	assert(ctext != NULL);

	greshunkel_tuple _stack_tuple = {
		.name = {0},
		.type = GSHKL_ARR,
		.value = {
			.arr = vector_new(sizeof(greshunkel_tuple), 16)
		}
	};
	const size_t name_len = strnlen(name, WISDOM_OF_WORDS) < WISDOM_OF_WORDS ? strnlen(name, WISDOM_OF_WORDS) : WISDOM_OF_WORDS;
	strncpy(_stack_tuple.name, name, name_len);

	_gshkl_add_var_to_context(ctext, &_stack_tuple);
	return _stack_tuple.value;
}

int gshkl_add_int_to_loop(greshunkel_var *loop, const int value) {
	assert(loop != NULL);

	greshunkel_tuple _stack_tuple = {
		.name = {0},
		.type = GSHKL_STR,
		.value = {{0}}
	};

	greshunkel_var _stack_var = {{0}};
	snprintf(_stack_var.str, MAX_GSHKL_STR_SIZE, "%i", value);

	memcpy(&_stack_tuple.value, &_stack_var, sizeof(greshunkel_var));

	return _gshkl_add_var_to_loop(loop, &_stack_tuple);
}

int gshkl_add_sub_context_to_loop(greshunkel_var *loop, const greshunkel_ctext *ctext) {
	assert(loop != NULL);
	assert(ctext != NULL);

	greshunkel_tuple _stack_tuple = {
		.name = {0},
		.type = GSHKL_SUBCTEXT,
		.value = {
			.sub_ctext = ctext
		}
	};

	return _gshkl_add_var_to_loop(loop, &_stack_tuple);
}

int gshkl_add_string_to_loop(greshunkel_var *loop, const char *value) {
	assert(loop != NULL);

	greshunkel_tuple _stack_tuple = {
		.name = {0},
		.type = GSHKL_STR,
		.value = {{0}}
	};

	greshunkel_var _stack_var = {{0}};
	if (value != NULL)
		strncpy(_stack_var.str, value, MAX_GSHKL_STR_SIZE);
	_stack_var.str[MAX_GSHKL_STR_SIZE] = '\0';

	memcpy(&_stack_tuple.value, &_stack_var, sizeof(union greshunkel_var));

	return _gshkl_add_var_to_loop(loop, &_stack_tuple);
}

static inline void _gshkl_free_arr(greshunkel_tuple *to_free) {
	assert(to_free->type == GSHKL_ARR);
	vector_free((vector *)to_free->value.arr);
}

void gshkl_free_context(greshunkel_ctext *ctext) {
	unsigned int i;
	for (i = 0; i < ctext->values->count; i++) {
		greshunkel_tuple *next = (greshunkel_tuple *)vector_get(ctext->values, i);
		if (next->type == GSHKL_ARR) {
			_gshkl_free_arr(next);
		} else if (next->type == GSHKL_SUBCTEXT) {
			gshkl_free_context((struct greshunkel_ctext *)next->value.sub_ctext);
		}
	}
	vector_free(ctext->values);
	vector_free(ctext->filter_functions);

	free(ctext);
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

/* I'm calling this "vishnu" because i don't actually know what it's for */
static void vishnu(line *new_line_to_add, const match_t match, const char *result, const line *operating_line) {
	char *p;
	const size_t sizes[3] = {match.rm_so, strlen(result), operating_line->size - match.rm_eo};

	new_line_to_add->size = sizes[0] + sizes[1] + sizes[2];
	new_line_to_add->data = p = calloc(1, new_line_to_add->size + 1);

	memcpy(p, operating_line->data, sizes[0]);
	p += sizes[0];
	memcpy(p, result, sizes[1]);
	p += sizes[1];
	memcpy(p, operating_line->data + match.rm_eo, sizes[2]);
}

static int regexec_2_0_beta(const regex_t *preg, const char *string, size_t nmatch, match_t pmatch[]) {
	unsigned int i;
	regmatch_t matches[nmatch];
	memset(matches, 0, sizeof(matches));

	if (regexec(preg, string, nmatch, matches, 0) != 0) {
		return 1;
	}
	for (i = 0; i < nmatch; i++) {
		pmatch[i].rm_so = matches[i].rm_so;
		pmatch[i].rm_eo = matches[i].rm_eo;
		pmatch[i].len = matches[i].rm_eo - matches[i].rm_so;
		pmatch[i].start = string + matches[i].rm_so;
	}
	return 0;
}

/* finds a variable or filter by name, returns first matching item or NULL if none found
 * find_values == 1 uses ctext->values, otherwise, ctext->filter_functions */
static const void *find_needle(const greshunkel_ctext *ctext, const char *needle, int find_values) {
	const greshunkel_ctext *current_ctext = ctext;
	while (current_ctext != NULL) {
		vector *current_vector;
		current_vector = (find_values) ? current_ctext->values : current_ctext->filter_functions;
		unsigned int i;
		for (i = 0; i < current_vector->count; i++) {
			const greshunkel_named_item *item = (greshunkel_named_item *)vector_get(current_vector, i);
			assert(item->name != NULL);
			const size_t larger = strlen(item->name) > strlen(needle) ? strlen(item->name) : strlen(needle);
			if (strncmp(item->name, needle, larger) == 0)
				return item;
		}
		current_ctext = current_ctext->parent;
	}
	return NULL;
}

static line
_filter_line(const greshunkel_ctext *ctext, const line *current_line, const struct compiled_regex *all_regex) {
	line interpolated_line = {0};
	line new_line_to_add = {0};
	const line *operating_line = current_line;
	assert(operating_line->data != NULL);

	/* Now we match template filters: */
	match_t filter_matches[3];
	while (regexec_2_0_beta(&all_regex->c_filter_regex, operating_line->data, 3, filter_matches) == 0) {
		match_t whole_match = filter_matches[0];
		const char *start_of_first_XxX = strstr(whole_match.start, " XxX");
		const char *end_of_first_XxX = start_of_first_XxX + strlen(" XxX");
		const size_t full_diff = end_of_first_XxX - whole_match.start;

		whole_match.rm_eo = (whole_match.start + full_diff) - operating_line->data;
		whole_match.len = full_diff;
		assert(start_of_first_XxX != NULL);

		const match_t function_name = filter_matches[1];
		const match_t argument = filter_matches[2];

		const greshunkel_filter *filter;
		char just_match_str[function_name.len + 1];
		just_match_str[function_name.len] = '\0';
		strncpy(just_match_str, function_name.start, function_name.len);

		if ((filter = find_needle(ctext, just_match_str, 0))) {

			const size_t new_len = start_of_first_XxX - argument.start;

			/* Render the argument out so we can pass it to the filter function. */
			char *rendered_argument = strndup(argument.start, new_len);

			/* Pass it to the filter function. */
			char *filter_result = filter->filter_func(rendered_argument);

			vishnu(&new_line_to_add, whole_match, filter_result, operating_line);

			if (filter->clean_up != NULL)
				filter->clean_up(filter_result);

			free(rendered_argument);
		}
		assert(filter != NULL);

		free(interpolated_line.data);
		interpolated_line.size = new_line_to_add.size;
		interpolated_line.data = new_line_to_add.data;
		new_line_to_add.size = 0;
		new_line_to_add.data = NULL;
		operating_line = &interpolated_line;

		/* Set the next regex check after this one. */
		memset(filter_matches, 0, sizeof(filter_matches));
	}

	/* No arguments matching. */
	match_t filter_matches_no_args[2];
	while (regexec_2_0_beta(&all_regex->c_filter_no_args_regex, operating_line->data, 2, filter_matches_no_args) == 0) {
		match_t whole_match = filter_matches_no_args[0];
		const char *start_of_first_XxX = strstr(whole_match.start, " XxX");
		const char *end_of_first_XxX = start_of_first_XxX + strlen(" XxX");
		const size_t full_diff = end_of_first_XxX - whole_match.start;

		whole_match.rm_eo = (whole_match.start + full_diff) - operating_line->data;
		whole_match.len = full_diff;
		assert(start_of_first_XxX != NULL);

		const match_t function_name = filter_matches_no_args[1];

		const greshunkel_filter *filter;
		char just_match_str[function_name.len + 1];
		just_match_str[function_name.len] = '\0';
		strncpy(just_match_str, function_name.start, function_name.len);

		if ((filter = find_needle(ctext, just_match_str, 0))) {
			char *filter_result = filter->filter_func(NULL);

			vishnu(&new_line_to_add, whole_match, filter_result, operating_line);

			if (filter->clean_up != NULL)
				filter->clean_up(filter_result);
		}
		assert(filter != NULL);

		free(interpolated_line.data);
		interpolated_line.size = new_line_to_add.size;
		interpolated_line.data = new_line_to_add.data;
		new_line_to_add.size = 0;
		new_line_to_add.data = NULL;
		operating_line = &interpolated_line;

		memset(filter_matches_no_args, 0, sizeof(filter_matches_no_args));
	}

	return *operating_line;
}

static line
_interpolate_line_subcontext(const greshunkel_ctext *ctext, const line current_line, const struct compiled_regex *all_regex) {
	/* Matches context-based variables, which are dictionary-like. */
	line interpolated_line = {0};
	line new_line_to_add = {0};
	const line *operating_line = &current_line;

	match_t cvar_match[3];
	while (regexec_2_0_beta(&all_regex->c_cvar_regex, operating_line->data, 3, cvar_match) == 0) {
		const match_t inner_match = cvar_match[1];
		const match_t subname_match = cvar_match[2];
		assert(inner_match.rm_so != -1 && inner_match.rm_eo != -1);
		assert(subname_match.rm_so != -1 && subname_match.rm_eo != -1);

		const greshunkel_tuple *tuple;
		char just_match_str[inner_match.len + 1];
		just_match_str[inner_match.len] = '\0';
		strncpy(just_match_str, inner_match.start, inner_match.len);

		/* So here we search for a sub context instead of a string. */
		if ((tuple = find_needle(ctext, just_match_str, 1)) && tuple->type == GSHKL_SUBCTEXT) {
			const greshunkel_ctext *sub_ctext = tuple->value.sub_ctext;

			char just_subname_match_str[subname_match.len + 1];
			just_subname_match_str[subname_match.len] = '\0';
			strncpy(just_subname_match_str, subname_match.start, subname_match.len);
			/* So now we need to:
			 * 1. Loop through the subcontext's values searching for subname_match
			 * 2. Inteprolate using that value.
			 */

			unsigned int i;
			for (i = 0; i < sub_ctext->values->count; i++) {
				const greshunkel_tuple *sub_tuple = vector_get(sub_ctext->values, i);
				const char *_name = sub_tuple->name;
				const size_t larger = strlen(_name) > strlen(just_subname_match_str) ?
						strlen(_name) : strlen(just_subname_match_str);

				if (strncmp(_name, just_subname_match_str, larger) == 0) {
					/* TODO: Only works on strings. */
					assert(sub_tuple->type == GSHKL_STR);
					vishnu(&new_line_to_add, cvar_match[0], sub_tuple->value.str, operating_line);
					goto done;
				}
			}

			/* Blow up if we had a variable that wasn't in the context. */
			printf("Did not match a sub-context variable that needed to be matched, or variable was None/NULL/0).\n");
			printf("Line: %s\n", operating_line->data);
			assert(1 == 0);
		} else {
			/* Blow up if we had a variable that wasn't in the context. */
			printf("Did not match a cvariable that needed to be matched.\n");
			printf("Line: %s\n", operating_line->data);
			assert(tuple != NULL);
			assert(tuple->type == GSHKL_SUBCTEXT);
		}

done:
		free(interpolated_line.data);
		interpolated_line.size = new_line_to_add.size;
		interpolated_line.data = new_line_to_add.data;
		new_line_to_add.size = 0;
		new_line_to_add.data = NULL;
		operating_line = &interpolated_line;

		/* Set the next regex check after this one. */
		memset(cvar_match, 0, sizeof(cvar_match));
	}

	return (*operating_line);
}

static line
_include_line(const greshunkel_ctext *ctext, const line *current_line, const struct compiled_regex *all_regex) {
	line interpolated_line = {0};
	const line *operating_line = current_line;
	assert(operating_line->data != NULL);

	match_t match[2] = {0};
	while (regexec_2_0_beta(&all_regex->c_include_regex, operating_line->data, 2, match) == 0) {
		const match_t inner_match = match[1];
		assert(inner_match.rm_so != -1 && inner_match.rm_eo != -1);

		/* Copy the string here so we only get the match, not everything after it. */
		char just_match_str[inner_match.len + 1];
		just_match_str[inner_match.len] = '\0';
		strncpy(just_match_str, inner_match.start, inner_match.len);


		/* Open the file here. */
		/* TODO: Make this actually the whole size of the file, not just a random buffer. */
		char template_file[2048] = {0};
		size_t template_file_siz = 0;
		FILE *include_file = fopen(just_match_str, "r");
		if (!include_file) {
			/* If we couldn't open the file, blow up with an assertion. */
			printf("Could not open SCREAM file for reading.\n");
			printf("Line: %s\n", operating_line->data);
			assert(1 == 0);
		}

		template_file_siz = fread(template_file, 1, sizeof(template_file), include_file);
		fclose(include_file);
		template_file[template_file_siz] = '\0';

		line rendered_piece = {0};
		rendered_piece.data = gshkl_render(ctext, template_file, template_file_siz, &rendered_piece.size);

		free(interpolated_line.data);
		interpolated_line.size = rendered_piece.size;
		interpolated_line.data = rendered_piece.data;
		operating_line = &interpolated_line;

		/* Set the next regex check after this one. */
		memset(match, 0, sizeof(match));
	}

	return *operating_line;
}

static line
_interpolate_line(const greshunkel_ctext *ctext, const line current_line, const struct compiled_regex *all_regex) {
	line interpolated_line = {0};
	line new_line_to_add = {0};

	/* Do context variables first: */
	const line _operating_line = _interpolate_line_subcontext(ctext, current_line, all_regex);
	const line *operating_line = &_operating_line;
	assert(operating_line->data != NULL);

	match_t match[2] = {0};
	while (regexec_2_0_beta(&all_regex->c_var_regex, operating_line->data, 2, match) == 0) {
		const match_t inner_match = match[1];
		assert(inner_match.rm_so != -1 && inner_match.rm_eo != -1);

		const greshunkel_tuple *tuple;
		/* Copy the string here so we only get the match, not everything after it. */
		char just_match_str[inner_match.len + 1];
		just_match_str[inner_match.len] = '\0';
		strncpy(just_match_str, inner_match.start, inner_match.len);

		if ((tuple = find_needle(ctext, just_match_str, 1)) && tuple->type == GSHKL_STR) {
			vishnu(&new_line_to_add, match[0], tuple->value.str, operating_line);
		} else {
			/* Blow up if we had a variable that wasn't in the context. */
			printf("Did not match a variable that needed to be matched.\n");
			printf("Line: %s\n", operating_line->data);
			assert(tuple != NULL);
			assert(tuple->type == GSHKL_STR);
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

	/* SCREAM includes */
	line included_line = _include_line(ctext, operating_line, all_regex);
	if (included_line.data != interpolated_line.data && interpolated_line.data != NULL)
		free(operating_line->data);

	line filtered_line = _filter_line(ctext, &included_line, all_regex);
	if (filtered_line.data != interpolated_line.data && interpolated_line.data != NULL)
		free(operating_line->data);

	return filtered_line;
}

static line
_interpolate_loop(const greshunkel_ctext *ctext, const char *buf, size_t *num_read, const struct compiled_regex *all_regex) {
	line to_return = {0};
	*num_read = 0;

	match_t match[4] = {{0}};
	/* TODO: Support loops inside of loops. That probably means a
	 * while loop here. */
	if (regexec_2_0_beta(&all_regex->c_loop_regex, buf, 4, match) == 0) {
		/* Variables we're going to need: */
		const match_t loop_variable = match[1];
		const match_t variable_name = match[2];
		match_t loop_meat = match[3];
		/* Make sure they were matched: */
		assert(variable_name.rm_so != -1 && variable_name.rm_eo != -1);
		assert(loop_variable.rm_so != -1 && loop_variable.rm_eo != -1);
		assert(loop_meat.rm_so != -1 && loop_meat.rm_eo != -1);

		size_t possible_dif = 0;
		const char *closest_BBL = NULL;
		closest_BBL = strstr(loop_meat.start, "xXx BBL xXx");
		possible_dif = closest_BBL - buf;
		if (possible_dif != (unsigned int)loop_meat.rm_so) {
			loop_meat.rm_eo = possible_dif;
			loop_meat.len = loop_meat.rm_eo - loop_meat.rm_so;
		}

		/* We found a fucking loop, holy shit */
		*num_read = loop_meat.rm_eo + strlen("xXx BBL xXx");

		const greshunkel_tuple *tuple;
		char just_match_str[variable_name.len + 1];
		just_match_str[variable_name.len] = '\0';
		strncpy(just_match_str, variable_name.start, variable_name.len);

		if (!((tuple = find_needle(ctext, just_match_str, 1)) && tuple->type == GSHKL_ARR)) {
			printf("Did not match a variable that needed to be matched.\n");
			printf("Line: %s\n", buf);
			assert(tuple != NULL);
			assert(tuple->type == GSHKL_ARR);
		}

		line to_render_line;
		to_render_line.data = strndup(loop_meat.start, loop_meat.len);
		to_render_line.size = loop_meat.len;

		vector *cur_vector_p = tuple->value.arr;

		/* This is the thing we're going to render over and over and over again. */
		char *loop_variable_name_rendered = strndup(loop_variable.start, loop_variable.len);

		/* Now we loop through the array incredulously. */
		unsigned int j;
		for (j = 0; j < cur_vector_p->count; j++) {
			const greshunkel_tuple *current_loop_var = vector_get(cur_vector_p, j);
			line rendered_piece = {0};

			/* Recurse contexts until my fucking mind melts. */
			greshunkel_ctext *_temp_ctext = _gshkl_init_child_context(ctext);
			if (current_loop_var->type == GSHKL_STR) {
				gshkl_add_string(_temp_ctext, loop_variable_name_rendered, current_loop_var->value.str);
			} else if (current_loop_var->type == GSHKL_SUBCTEXT) {
				gshkl_add_sub_context(_temp_ctext, loop_variable_name_rendered, current_loop_var->value.sub_ctext);
			} else {
				printf("Weird type given during loop interpolation.");
				printf("Line: %s\n", buf);
				assert(1 == 0);
			}

			rendered_piece.data = gshkl_render(_temp_ctext, to_render_line.data, to_render_line.size, &rendered_piece.size);
			gshkl_free_context(_temp_ctext);

			const size_t old_size = to_return.size;
			to_return.size += rendered_piece.size;
			to_return.data = realloc(to_return.data, to_return.size);
			memcpy(to_return.data + old_size, rendered_piece.data, rendered_piece.size);
			free(rendered_piece.data);
		}

		free(loop_variable_name_rendered);
		free(to_render_line.data);
	}

	return to_return;
}

static inline int _is_falsey(const greshunkel_tuple *tuple) {
	const int is_string = tuple->type == GSHKL_STR ? 1 : 0;
	const int is_loop = tuple->type == GSHKL_ARR ? 1 : 0;
	const int is_subcontext = tuple->type == GSHKL_SUBCTEXT ? 1 : 0;

	int is_null = 0;
	if (is_string) {
		is_null = strnlen(tuple->value.str, MAX_GSHKL_STR_SIZE) == 0 ? 1 : 0;
	} else if (is_loop) {
		is_null = tuple->value.arr == NULL;
	} else if (is_subcontext) {
		is_null = 0;
	}
	if (!is_null) {
		if (is_string) {
			const int says_false = strncmp(tuple->value.str, "FALSE", strlen("FALSE")) == 0 ? 1 : 0;
			m38_log_msg(LOG_DEBUG, "_is_falsey: is_string SF: %i", says_false);
			return says_false;
		} else if (is_loop) {
			const int has_items = tuple->value.arr->count > 0;
			m38_log_msg(LOG_DEBUG, "_is_falsey: s_string is_loop has_items: %i", !has_items);
			return !has_items;
		}
		m38_log_msg(LOG_DEBUG, "_is_falsey: !is_null is_subcontext: %i", !is_subcontext);
		return !is_subcontext;
	}

	m38_log_msg(LOG_DEBUG, "_is_falsey: return 0");
	return 0;
}

static inline int _is_truthy(const greshunkel_tuple *tuple) {
	const int is_string = tuple->type == GSHKL_STR ? 1 : 0;
	const int is_loop = tuple->type == GSHKL_ARR ? 1 : 0;
	const int is_subcontext = tuple->type == GSHKL_SUBCTEXT ? 1 : 0;
	int is_null = 0;
	if (is_string) {
		is_null = strnlen(tuple->value.str, MAX_GSHKL_STR_SIZE) == 0 ? 1 : 0;
	} else if (is_loop) {
		is_null = tuple->value.arr == NULL;
	} else if (is_subcontext) {
		is_null = 0;
	}

	if (!is_null) {
		if (is_string) {
			const int says_false = strncmp(tuple->value.str, "FALSE", strlen("FALSE")) == 0 ? 1 : 0;
			m38_log_msg(LOG_DEBUG, "_is_truthy: !is_null is_string SF: %i", !says_false);
			return !says_false;
		} else if (is_loop) {
			const int has_items = tuple->value.arr->count > 0;
			m38_log_msg(LOG_DEBUG, "_is_truthy: !is_null is_loop has_items: %i", has_items);
			return has_items;
		}
		m38_log_msg(LOG_DEBUG, "_is_truthy: !is_null  is_subcontext: %i", is_subcontext);
		return is_subcontext;
	}

	int rc = (is_string || is_subcontext) && !is_null;
	m38_log_msg(LOG_DEBUG, "_is_truthy: complex: %i %i %i %i", rc, is_string, is_subcontext, !is_null);
	return rc;
}

static line
_interpolate_conditionals(const greshunkel_ctext *ctext, const char *buf, size_t *num_read, const struct compiled_regex *all_regex) {
	line to_return = {0};
	*num_read = 0;

	match_t match[4] = {{0}};
	if (regexec_2_0_beta(&all_regex->c_conditional_inverse_regex, buf, 3, match) == 0) {
		const match_t conditional_variable = match[1];
		match_t conditional_meat = match[2];
		/* Make sure they were matched: */
		assert(conditional_variable.rm_so != -1 && conditional_variable.rm_eo != -1);
		assert(conditional_meat.rm_so != -1 && conditional_meat.rm_eo != -1);

		size_t possible_dif = 0;
		const char *closest_endif = NULL;
		closest_endif = strstr(conditional_meat.start, "xXx ENDLESS xXx");
		possible_dif = closest_endif - buf;
		if (possible_dif != (unsigned int)conditional_meat.rm_so) {
			conditional_meat.rm_eo = possible_dif;
			conditional_meat.len = conditional_meat.rm_eo - conditional_meat.rm_so;
		}

		/* We found a conditioanl. */
		*num_read = conditional_meat.rm_eo + strlen("xXx ENDLESS xXx");

		char just_match_str[conditional_variable.len + 1];
		just_match_str[conditional_variable.len] = '\0';
		strncpy(just_match_str, conditional_variable.start, conditional_variable.len);

		/* TODO: Support a boolean type. */
		/*if (!((tuple = find_needle(ctext, just_match_str, 1)) && tuple->type == GSHKL_BOOL)) { */
		const greshunkel_tuple *tuple = NULL;
		char *sub_ctext_match = NULL;
		int should_render = 0;
		if ((tuple = find_needle(ctext, just_match_str, 1))) {
			if (_is_truthy(tuple)) {
				should_render = 1;
			}
		} else if ((sub_ctext_match = strchr(just_match_str, '.')) != NULL) {
			/* TODO: Check sub-context. */
			char sub_context_name[MAX_GSHKL_STR_SIZE] = {0};
			char sub_item_name[MAX_GSHKL_STR_SIZE] = {0};
			memcpy(sub_context_name, just_match_str, sub_ctext_match - just_match_str);
			memcpy(sub_item_name, sub_ctext_match + strlen("."), conditional_variable.len - strlen(sub_context_name) + strlen("."));

			if ((tuple = find_needle(ctext, sub_context_name, 1)) && tuple->type == GSHKL_SUBCTEXT) {
				const greshunkel_ctext *sub_ctext = tuple->value.sub_ctext;

				unsigned int i;
				for (i = 0; i < sub_ctext->values->count; i++) {
					const greshunkel_tuple *sub_tuple = vector_get(sub_ctext->values, i);
					const char *_name = sub_tuple->name;
					const size_t larger = strlen(_name) > strlen(sub_item_name) ?
							strlen(_name) : strlen(sub_item_name);

					if (strncmp(_name, sub_item_name, larger) == 0 && _is_falsey(sub_tuple)) {
							should_render = 1;
					}
				}
			}
		}

		if (should_render) {
			line to_render_line;
			to_render_line.data = strndup(conditional_meat.start, conditional_meat.len);
			to_render_line.size = conditional_meat.len;

			line rendered_piece = {0};

			/* Recurse contexts until my fucking mind melts. */
			greshunkel_ctext *_temp_ctext = _gshkl_init_child_context(ctext);
			rendered_piece.data = gshkl_render(_temp_ctext, to_render_line.data, to_render_line.size, &rendered_piece.size);
			gshkl_free_context(_temp_ctext);

			const size_t old_size = to_return.size;
			to_return.size += rendered_piece.size;
			if (old_size <= 0) {
				to_return.data = calloc(1, to_return.size);
			} else {
				to_return.data = realloc(to_return.data, to_return.size);
			}

			memcpy(to_return.data + old_size, rendered_piece.data, rendered_piece.size);
			free(rendered_piece.data);
			free(to_render_line.data);
		}
	} else if (regexec_2_0_beta(&all_regex->c_conditional_regex, buf, 3, match) == 0) {
		const match_t conditional_variable = match[1];
		match_t conditional_meat = match[2];

		assert(conditional_variable.rm_so != -1 && conditional_variable.rm_eo != -1);
		assert(conditional_meat.rm_so != -1 && conditional_meat.rm_eo != -1);

		size_t possible_dif = 0;
		const char *closest_endif = NULL;
		closest_endif = strstr(conditional_meat.start, "xXx ENDLESS xXx");
		possible_dif = closest_endif - buf;
		if (possible_dif != (unsigned int)conditional_meat.rm_so) {
			conditional_meat.rm_eo = possible_dif;
			conditional_meat.len = conditional_meat.rm_eo - conditional_meat.rm_so;
		}

		/* We found a conditioanl. */
		*num_read = conditional_meat.rm_eo + strlen("xXx ENDLESS xXx");

		char just_match_str[conditional_variable.len + 1];
		just_match_str[conditional_variable.len] = '\0';
		strncpy(just_match_str, conditional_variable.start, conditional_variable.len);

		/* TODO: Support a boolean type. */
		/*if (!((tuple = find_needle(ctext, just_match_str, 1)) && tuple->type == GSHKL_BOOL)) */
		const greshunkel_tuple *tuple = NULL;
		char *sub_ctext_match = NULL;
		int should_render = 0;
		if (!(tuple = find_needle(ctext, just_match_str, 1))) {
			if ((sub_ctext_match = strchr(just_match_str, '.')) != NULL) {
				/* TODO: Check sub-context. */
				char sub_context_name[MAX_GSHKL_STR_SIZE] = {0};
				char sub_item_name[MAX_GSHKL_STR_SIZE] = {0};
				memcpy(sub_context_name, just_match_str, sub_ctext_match - just_match_str);
				memcpy(sub_item_name, sub_ctext_match + strlen("."), conditional_variable.len - strlen(sub_context_name) + strlen("."));

				if ((tuple = find_needle(ctext, sub_context_name, 1)) && tuple->type == GSHKL_SUBCTEXT) {
					const greshunkel_ctext *sub_ctext = tuple->value.sub_ctext;

					unsigned int i;
					for (i = 0; i < sub_ctext->values->count; i++) {
						const greshunkel_tuple *sub_tuple = vector_get(sub_ctext->values, i);
						const char *_name = sub_tuple->name;
						const size_t larger = strlen(_name) > strlen(sub_item_name) ?
								strlen(_name) : strlen(sub_item_name);

						if (strncmp(_name, sub_item_name, larger) == 0 &&
							_is_truthy(sub_tuple)) {
								should_render = 1;
						}
					}
				}
			} else {
				should_render = 1;
			}
		} else {
			if (_is_falsey(tuple)) {
				should_render = 1;
			} else if ((sub_ctext_match = strchr(just_match_str, '.')) != NULL) {
				/* TODO: Check sub-context. */
				char sub_context_name[MAX_GSHKL_STR_SIZE] = {0};
				char sub_item_name[MAX_GSHKL_STR_SIZE] = {0};
				memcpy(sub_context_name, just_match_str, sub_ctext_match - just_match_str);
				memcpy(sub_item_name, sub_ctext_match + strlen("."), conditional_variable.len - strlen(sub_context_name) + strlen("."));

				if ((tuple = find_needle(ctext, sub_context_name, 1)) && tuple->type == GSHKL_SUBCTEXT) {
					const greshunkel_ctext *sub_ctext = tuple->value.sub_ctext;

					unsigned int i;
					for (i = 0; i < sub_ctext->values->count; i++) {
						const greshunkel_tuple *sub_tuple = vector_get(sub_ctext->values, i);
						const char *_name = sub_tuple->name;
						const size_t larger = strlen(_name) > strlen(sub_item_name) ?
								strlen(_name) : strlen(sub_item_name);

						if (strncmp(_name, sub_item_name, larger) == 0 &&
							_is_truthy(sub_tuple)) {
								should_render = 1;
						}
					}
				}
			}
		}

		if (should_render) {
			line to_render_line;
			to_render_line.data = strndup(conditional_meat.start, conditional_meat.len);
			to_render_line.size = conditional_meat.len;

			/* TODO: This should use gshkl_render. See _interpolate_loop */
			line rendered_piece = _interpolate_line(ctext, to_render_line, all_regex);

			const size_t old_size = to_return.size;
			to_return.size += rendered_piece.size;
			if (old_size <= 0) {
				to_return.data = calloc(1, to_return.size);
			} else {
				to_return.data = realloc(to_return.data, to_return.size);
			}

			memcpy(to_return.data + old_size, rendered_piece.data, rendered_piece.size);
			if (rendered_piece.data != to_render_line.data)
				free(rendered_piece.data);
			free(to_render_line.data);
		}
	}

	return to_return;
}

static inline void _compile_regex(struct compiled_regex *all_regex) {
	/* TODO: Clean this up with Orbitz Gum. */
	int reti = regcomp(&all_regex->c_var_regex, variable_regex, REG_EXTENDED);
	assert(reti == 0);

	reti = regcomp(&all_regex->c_cvar_regex, ctext_variable_regex, REG_EXTENDED);
	assert(reti == 0);

	reti = regcomp(&all_regex->c_loop_regex, loop_regex, REG_EXTENDED);
	assert(reti == 0);

	reti = regcomp(&all_regex->c_filter_regex, filter_regex, REG_EXTENDED);
	assert(reti == 0);

	reti = regcomp(&all_regex->c_filter_no_args_regex, filter_regex_no_args, REG_EXTENDED);
	assert(reti == 0);

	reti = regcomp(&all_regex->c_include_regex, include_regex, REG_EXTENDED);
	assert(reti == 0);

	reti = regcomp(&all_regex->c_conditional_regex, conditional_regex, REG_EXTENDED);
	assert(reti == 0);

	reti = regcomp(&all_regex->c_conditional_inverse_regex, conditional_inverse_regex, REG_EXTENDED);
	assert(reti == 0);
}

static inline void _destroy_regex(struct compiled_regex *all_regex) {
	regfree(&all_regex->c_var_regex);
	regfree(&all_regex->c_cvar_regex);
	regfree(&all_regex->c_loop_regex);
	regfree(&all_regex->c_filter_regex);
	regfree(&all_regex->c_filter_no_args_regex);
	regfree(&all_regex->c_include_regex);
	regfree(&all_regex->c_conditional_regex);
	regfree(&all_regex->c_conditional_inverse_regex);
}

char *gshkl_render(const greshunkel_ctext *ctext, const char *to_render, const size_t original_size, size_t *outsize) {
	assert(to_render != NULL);
	assert(ctext != NULL);

	/* We start up a new buffer and copy the old one into it: */
	char *rendered = NULL;
	size_t intermediate_outsize = 0;

	struct compiled_regex all_regex;
	_compile_regex(&all_regex);

	size_t num_read = 0;
	while (num_read < original_size) {
		line current_line = read_line(to_render + num_read);

		line to_append = {0};
		size_t loop_readahead = 0;
		/* The loop needs to read more than the current line, so we pass
		 * in the offset and just let it go. If it processes more than the
		 * size of the current line, we know it did something.
		 * Append the whole line it gets back. */
		to_append = _interpolate_loop(ctext, to_render + num_read, &loop_readahead, &all_regex);

		/* Otherwise just interpolate the line like normal. */
		if (loop_readahead == 0) {
			/* Now let's try to interpolate an if statement, much like a loop. */
			size_t conditional_readahead = 0;
			to_append = _interpolate_conditionals(ctext, to_render + num_read, &conditional_readahead, &all_regex);

			if (conditional_readahead == 0) {
				to_append = _interpolate_line(ctext, current_line, &all_regex);
				num_read += current_line.size;
			} else {
				num_read += conditional_readahead;
			}
		} else {
			num_read += loop_readahead;
		}

		/* Fuck this */
		const size_t old_outsize = intermediate_outsize;
		intermediate_outsize += to_append.size;
		{
			char *med_buf = realloc(rendered, intermediate_outsize + 1);
			if (med_buf == NULL)
				goto error;
			med_buf[intermediate_outsize] = '\0';
			rendered = med_buf;
		}
		memcpy(rendered + old_outsize, to_append.data, to_append.size);
		if (to_append.data != current_line.data)
			free(current_line.data);
		free(to_append.data);
	}
	_destroy_regex(&all_regex);
	/* rendered[intermediate_outsize - 1] = '\0'; */

	if (outsize)
		*outsize = intermediate_outsize - 1;
	return rendered;

error:
	free(rendered);
	return NULL;
}
