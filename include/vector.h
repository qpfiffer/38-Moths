// vim: noet ts=4 sw=4
#pragma once
#include <stdlib.h>

typedef struct vector {
	const size_t item_size;
	size_t max_size;
	size_t count;
	void *items;
} vector;

/* Create a new vector. */
vector *vector_new(const size_t item_size, const size_t initial_element_count);

/* Append a new element to the end of the vector. This will create a copy of
 * the item passed in.
 * Returns 1 on success.
 */
int vector_append(vector *vec, const void *item, const size_t item_size);

/* Gets an element at i. */
const void *vector_get(const vector *vec, const unsigned int i);

/* Free a vector and it's components. */
void vector_free(vector *to_free);

