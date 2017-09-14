// vim: noet ts=4 sw=4
#include <assert.h>
#include <string.h>
#include "vector.h"

#define nth(I) (void *)(vec->items + (I * vec->item_size))

vector *vector_new(const size_t item_size, const size_t initial_element_count) {
	vector _vec = {
		.item_size = item_size,
		.max_size = initial_element_count,
		.count = 0,
		.items = calloc(1, initial_element_count * item_size + 1)
	};

	vector *to_return = calloc(1, sizeof(vector));
	memcpy(to_return, &_vec, sizeof(vector));

	return to_return;
}

inline int vector_append(vector *vec, const void *item, const size_t item_size) {
	if (vec == NULL)
		return 0;

	if (item_size > vec->item_size)
		return 0;

	if (vec->count == vec->max_size) {
		vec->max_size *= 2;
		void *array = realloc(vec->items, vec->max_size * vec->item_size + 1);
		if (!array)
			return 0;
		vec->items = array;
	}

	if (item_size != 0 && item != NULL) {
		memcpy(nth(vec->count), item, item_size);
		memset(nth(vec->count) + item_size, '\0', sizeof(char));
	} else {
		/* Just zero out the memory so we can check for NULLs. */
		memset(nth(vec->count), 0, vec->item_size);
	}
	vec->count++;
	return 1;
}

inline int vector_append_ptr(vector *vec, const void *pointer) {
	if (vec == NULL)
		return 0;

	if (vec->item_size != sizeof(pointer))
		return 0;

	if (vec->count == vec->max_size) {
		vec->max_size *= 2;
		void *array = realloc(vec->items, (vec->max_size * vec->item_size));
		if (!array)
			return 0;
		vec->items = array;
	}

	memcpy(nth(vec->count), &pointer, sizeof(void *));
	vec->count++;
	return 1;
}

inline const void *vector_get(const vector *vec, const unsigned int i) {
	if (vec == NULL)
		return NULL;
	if (i > vec->max_size)
		return NULL;
	return nth(i);
}

int vector_reverse(vector *vec) {
	if (vec == NULL)
		return 0;

	const size_t item_size = vec->item_size;
	unsigned int i = 0;

	unsigned char *buf = malloc(item_size);
	if (!buf)
		return 0;

	for (;i < (unsigned int)(vec->count / 2); i++) {
		const void *item = vector_get(vec, i);
		const unsigned int offset = vec->count - i - 1;
		if (!memcpy(buf, vector_get(vec, offset), item_size))
			return 0;

		if (!memcpy(nth(offset), item, item_size))
			return 0;

		if (!memcpy(nth(i), buf, item_size))
			return 0;
	}
	free(buf);

	return 1;
}

void vector_free(vector *vec) {
	if (vec == NULL)
		return;
	free(vec->items);
	free(vec);
}

