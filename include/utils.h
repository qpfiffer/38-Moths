// vim: noet ts=4 sw=4
#pragma once
#include <limits.h>
#include <math.h>
#include <stdlib.h>
#include <time.h>

/* xXx DEFINE=INT_LEN(x) xXx
* xXx DESCRIPTION=Returns the length of an integer if it were rendered as a string. xXx
*/
#define INT_LEN(x) x == 0 ? 1 : floor(log10(abs(x))) + 1

/* xXx DEFINE=UINT_LEN(x) xXx
* xXx DESCRIPTION=Returns the length of an unsigned integer if it were rendered as a string. xXx
*/
#define UINT_LEN(x) x == 0 ? 1 : floor(log10(x)) + 1

/* xXx DEFINE=UNUSED(x) xXx
* xXx DESCRIPTION=If an argument is unused, use this to avoid compiler warnings. Use with care. xXx
*/
#define UNUSED(x) (void)x

/* xXx DEFINE=HASH_STR_SIZE xXx
* xXx DESCRIPTION=The length of a 64-character hash string + NULL terminator. Used for the fnv1a function. xXx
*/
#define HASH_STR_SIZE 65

/* xXx FUNCTION=endswith xXx
* xXx DESCRIPTION=Helper function determine if a string ends with another string. xXx
* xXx RETURNS=1 if <code>string</code> ends with <code>suffix</code>. xXx
* xXx *string=The source string to test. xXx
* xXx *suffix=The suffix to check for. xXx
*/
int endswith(const char *string, const char *suffix);

/* xXx FUNCTION=strnstr xXx
* xXx DESCRIPTION=Like strstr, but only checks up to n chars. xXx
* xXx RETURNS=Whatever the hell strstr() returns. xXx
* xXx *haystack=Go look at strstr(). xXx
* xXx *needle=Go look at strstr(). xXx
* xXx len=Maximum number of characters to look through. xXx
*/
char *strnstr(const char *haystack, const char *needle, size_t len);

/* xXx FUNCTION=get_file_creation_date xXx
* xXx DESCRIPTION=Gets the file creation date. xXx
* xXx RETURNS=The file creation date. xXx
* xXx *file_path=The path of the file to get the creation date of. xXx
*/
time_t get_file_creation_date(const char *file_path);

/* xXx FUNCTION=get_file_size xXx
* xXx DESCRIPTION=Gets the file size. xXx
* xXx RETURNS=The file size. xXx
* xXx *file_path=The path of the file to get the size of. xXx
*/
size_t get_file_size(const char *file_path);

/* xXx FUNCTION=hash_string_fnv1a xXx
* xXx DESCRIPTION=Hashes a string using the FNV-1a algorithm xXx
* xXx RETURNS=1. Always. I don't care what you think. xXx
* xXx *string=The string to hash. xXx
* xXx siz=The size of the string. xXx
* xXx outbuf[static HASH_STR_SIZE]=This buffer will be filled out with the hashed string. xXx
*/
int hash_string_fnv1a(const unsigned char *string, const size_t siz, char outbuf[static HASH_STR_SIZE]);

/* xXx FUNCTION=m38_get_cookie_value xXx
* xXx DESCRIPTION=Gets a specific value out of a cookie, like the session ID. xXx
* xXx RETURNS=NULL or the value of the specific cookie value. xXx
* xXx *cookie_string=The value of the entire cookie string to parse, which comes from m38_get_header_value. xXx
* xXx cookie_string_siz=The length of cookie_string. xXx
* xXx needle=The value you're looking for inside the cookie, like `sessionid`. xXx
*/
char *m38_get_cookie_value(const char *cookie_string, const size_t cookie_string_siz, const char *needle);

/* xXx FUNCTION=m38_url_decode xXx
* xXx DESCRIPTION=Turns a url encoded string into a regular string. xXx
* xXx RETURNS=-1 on failure. xXx
* xXx *s=The string to  xXx
* xXx *dec=The outbuffer for the decoded string. xXx
*/
int m38_url_decode(const char *s, char *dec);
