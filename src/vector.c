// vim: noet ts=4 sw=4
#include <assert.h>
#include <string.h>
#include "vector.h"

#define nth(I) (void *)(vec->items + (I * vec->item_size))

vector *vector_new(const size_t item_size, const size_t initial_element_count) {
	vector _vec = {
		.item_size = item_size + 1,
		.max_size = initial_element_count,
		.count = 0,
		.items = calloc(initial_element_count, item_size)
	};

	vector *to_return = calloc(1, sizeof(vector));
	memcpy(to_return, &_vec, sizeof(vector));

	return to_return;
}

inline int vector_append(vector *vec, const void *item, const size_t item_size) {
	if (item_size > vec->item_size)
		return 0;

	if (vec->count == vec->max_size) {
		vec->max_size *= 2;
		void *array = realloc(vec->items, vec->max_size * vec->item_size);
		if (!array)
			return 0;
		vec->items = array;
	}

	memcpy(nth(vec->count), item, item_size);
	memset(nth(vec->count) + item_size, '\0', sizeof(char));
	vec->count++;
	return 1;
}

inline const void *vector_get(const vector *vec, const unsigned int i) {
	if (i > vec->max_size)
		return NULL;
	return nth(i);
}

void vector_free(vector *vec) {
	free(vec->items);
	free(vec);
}

