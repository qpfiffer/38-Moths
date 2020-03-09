// vim: noet ts=4 sw=4
#ifdef __clang__
	#pragma clang diagnostic ignored "-Wmissing-field-initializers"
#endif
#include <ctype.h>
#include <fcntl.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <stdint.h>
#define __STDC_FORMAT_MACROS
#include <inttypes.h>
#include <unistd.h>

#include "logging.h"
#include "utils.h"

int endswith(const char *string, const char *suffix) {
	size_t string_siz = strlen(string);
	size_t suffix_siz = strlen(suffix);

	if (string_siz < suffix_siz)
		return 0;

	unsigned int i = 0;
	for (; i < suffix_siz; i++) {
		if (suffix[i] != string[string_siz - suffix_siz + i])
			return 0;
	}
	return 1;
}

/* Pulled from here: http://stackoverflow.com/a/25705264 */
char *strnstr(const char *haystack, const char *needle, size_t len) {
	int i;
	size_t needle_len;

	/* segfault here if needle is not NULL terminated */
	if (0 == (needle_len = strlen(needle)))
		return (char *)haystack;

	/* Limit the search if haystack is shorter than 'len' */
	len = strnlen(haystack, len);

	for (i=0; i<(int)(len-needle_len); i++)
	{
		if ((haystack[0] == needle[0]) && (0 == strncmp(haystack, needle, needle_len)))
			return (char *)haystack;

		haystack++;
	}
	return NULL;
}

time_t get_file_creation_date(const char *file_path) {
	struct stat st = {0};
	if (stat(file_path, &st) == -1)
		return 0;
	return st.st_mtime;
}

size_t get_file_size(const char *file_path) {
	struct stat st = {0};
	if (stat(file_path, &st) == -1)
		return 0;
	return st.st_size;
}

int hash_string_fnv1a(const unsigned char *key, const size_t siz, char outbuf[static HASH_STR_SIZE]) {
	/* https://en.wikipedia.org/wiki/Fowler_Noll_Vo_hash */
	const uint64_t fnv_prime = 1099511628211ULL;
	const uint64_t fnv_offset_bias = 14695981039346656037ULL;

	const int iterations = siz;

	uint8_t i;
	uint64_t hash = fnv_offset_bias;

	for(i = 0; i < iterations; i++) {
		hash = hash ^ key[i];
		hash = hash * fnv_prime;
	}

	sprintf(outbuf, "%"PRIX64, hash);
	return 1;
}

char *m38_get_cookie_value(const char *cookie_string, const size_t cookie_string_siz, const char *needle) {
	/* TODO: Make sure this function handles bad input. */
	if (!cookie_string)
		return NULL;

	const char *cookie_loc = strnstr(cookie_string, needle, cookie_string_siz);
	if (!cookie_loc)
		return NULL;

	const char *cookie_string_start = cookie_loc;
	const char *cookie_string_end = strnstr(cookie_loc, ";", cookie_string_siz - (cookie_loc - cookie_string));
	if (!cookie_string_end) {
		/* This is okay, it might be at the end of the cookie. */
		cookie_string_end = strnstr(cookie_loc, " ", cookie_string_siz - (cookie_loc - cookie_string));
		if (!cookie_string_end)
			cookie_string_end = cookie_loc + cookie_string_siz;
	}

	const size_t cookie_string_size = cookie_string_end - cookie_string_start;
	const char *cookie_string_value = strnstr(cookie_loc, "=", cookie_string_size);
	if (!cookie_string_value)
		return NULL;

	/* -1 here is for the '=': */
	const size_t cookie_string_value_size = cookie_string_end - (cookie_string_value - 1);
	char *data = calloc(sizeof(char), cookie_string_value_size + 1);
	data[cookie_string_size] = '\0';
	strncpy(data, cookie_string_value + 1, cookie_string_value_size);

	return data;
}
