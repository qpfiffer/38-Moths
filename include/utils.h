// vim: noet ts=4 sw=4
#pragma once
#include <math.h>
#include <stdlib.h>
#include <time.h>

#define INT_LEN(x) floor(log10(abs(x))) + 1
#define UINT_LEN(x) floor(log10(x)) + 1
#define UNUSED(x) (void)x

#define HASH_STR_SIZE 65
#define MAX_FILENAME_SIZE 255

/* Doesn't actually fully decode URLs at this point. */
void url_decode(const char *src, const size_t src_siz, char *dest);

/* String helper stuff. */
int endswith(const char *string, const char *suffix);
char *strnstr(const char *haystack, const char *needle, size_t len);

time_t get_file_creation_date(const char *file_path);
size_t get_file_size(const char *file_path);

int hash_string_fnv1a(const unsigned char *string, const size_t siz, char outbuf[static HASH_STR_SIZE]);
char *get_full_path_for_file(const char *dir, const char file_name[static MAX_FILENAME_SIZE]);
