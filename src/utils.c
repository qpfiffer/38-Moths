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

void url_decode(const char *src, const size_t src_siz, char *dest) {
	unsigned int srcIter = 0, destIter = 0;
	char to_conv[] = "00";

	while (srcIter < src_siz) {
		if (src[srcIter] == '%' && srcIter + 2 < src_siz
				&& isxdigit(src[srcIter + 1]) && isxdigit(src[srcIter + 2])) {
			/* Theres definitely a better way to do this but I don't care
			 * right now. */
			to_conv[0] = src[srcIter + 1];
			to_conv[1] = src[srcIter + 2];

			long int converted = strtol(to_conv, NULL, 16);
			dest[destIter] = converted;

			srcIter += 3;
			destIter++;
		}

		dest[destIter] = src[srcIter];
		destIter++;
		srcIter++;
	}
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

inline char *get_full_path_for_file(const char *dir, const char file_name[static MAX_FILENAME_SIZE]) {
	const size_t siz = strlen(dir) + strlen("/") + strlen(file_name) + 1;
	char *fpath = malloc(siz);

	snprintf(fpath, siz, "%s/%s", dir, file_name);

	return fpath;
}
