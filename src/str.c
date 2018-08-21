/*
 * Generic string utility functions.
 * Copyright (C)2011 Mike Bourgeous.  Released under AGPLv3 in 2018.
 */
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <sys/types.h>

#include "nlutils.h"

/*
 * Duplicates the given string, using malloc() to allocate new memory to fit
 * the length of the string, including the terminating NULL character.  If
 * s is null, then a newly-created zero-length string is returned.  Returns
 * NULL if malloc() fails, and errno will have been set by malloc().  Any
 * non-NULL return value must later be free()d.
 *
 * Just calls strdup() on systems where it is available.
 */
char *nl_strdup(const char *s) // TODO: probably remove this
{
	if(s == NULL) {
		return calloc(1, sizeof(char));
	}

#if defined(_SVID_SOURCE) || defined(_BSD_SOURCE) || (_XOPEN_SOURCE >= 500)
	return strdup(s);
#else /* defined... */
	char *result;

	result = malloc(sizeof(char) * (strlen(s) + 1));
	if(result == NULL) {
		return NULL;
	}
	strcpy(result, s);

	return result;
#endif /* defined... */
}

/*
 * Copies n bytes from the given source string into the destination string, and
 * adds a terminating NULL character (so dest must be able to hold n+1 bytes,
 * and src must contain at least n bytes).
 */
void nl_strncpy_term(char *dest, const char *src, size_t n)
{
	memcpy(dest, src, n);
	dest[n] = 0;
}

/*
 * Duplicates the given string, copying exactly n bytes from the source into a
 * newly-allocated n+1 byte array.  This differs from the GNU strndup()
 * function, as src must contain at least n bytes to copy.  Returns NULL on
 * error, and errno may have been set by malloc().
 */
char *nl_strndup_term(const char *src, size_t n)
{
	char *s;

	s = malloc(n + 1);
	if(s == NULL) {
		ERRNO_OUT("Error allocating memory for duplicated string of length %zu", n);
		return NULL;
	}

	memcpy(s, src, n);
	s[n] = 0;

	return s;
}

/*
 * Returns a pointer to the first character in str that is in accept.  Returns
 * NULL if no matching characters were found within size bytes or if the end of
 * str was reached.  str is not const to allow the returned pointer to be
 * non-const.
 */
char *nl_strnpbrk(char *str, const char *accept, size_t size)
{
	const char *tmp_a;
	size_t count = 0;

	// Might be a tiny bit faster using pointer arithmetic
	while(str[count] != 0 && count < size) {
		tmp_a = accept;
		while(*tmp_a != 0) {
			if(str[count] == *tmp_a) {
				return str + count;
			}
			tmp_a++;
		}
		count++;
	}

	return NULL;
}

/*
 * Counts the number of occurrences of the given character in the given string.
 */
size_t nl_strcount(const char *str, char c)
{
	size_t count;

	for(count = 0; *str != 0; str++) {
		if(*str == c) {
			count++;
		}
	}

	return count;
}

/*
 * Returns a number less than, equal to, or greater than zero if the first
 * strlen(start) characters of str are less than, equal to, or greater than
 * start, as determined by strncmp().  Equivalent to calling strncmp(str,
 * start, strlen(start)).  Does not check for NULL parameters.
 */
int nl_strstart(const char *str, const char *start)
{
	return strncmp(str, start, strlen(start));
}

/*
 * Returns a number less than, equal to, or greater than zero if the final
 * strlen(end) characters of str are less than, equal to, or greater than end,
 * as determined by strcmp().  If strlen(str) is less than strlen(end), then
 * the result of calling strcmp() on both strings will be returned.  Does not
 * check for NULL parameters.  Both str and end should be shorter than the
 * maximum value of an ssize_t on this platform.
 */
int nl_strend(const char *str, const char *end)
{
	size_t offset = MAX_NUM((ssize_t)0, (ssize_t)strlen(str) - (ssize_t)strlen(end));
	return strcmp(str + offset, end);
}

/*
 * Returns the number of bytes the given strings have in common at their
 * beginnings.  Does not check for NULL parameters.
 */
size_t nl_strcommon(const char *a, const char *b)
{
	size_t common = 0;

	while(*a && *b && *a == *b) {
		a++;
		b++;
		common++;
	}

	return common;
}

/*
 * Converts the given data to hexadecimal, using lowercase letters for the
 * digits a-f.  The output buffer must be able to store 2*len+1 chars.  Does
 * not check for NULL parameters.
 */
void nl_to_hex(const uint8_t *data, size_t len, char *out)
{
	static const char hex_digits[] = "0123456789abcdef";
	size_t i;

	for(i = 0; i < len; i++) {
		out[i * 2] = hex_digits[data[i] >> 4];
		out[i * 2 + 1] = hex_digits[data[i] & 0xf];
	}
	out[len * 2] = 0;
}

/*
 * Converts the given hexadecimal string in hex to raw data in data.  Hex
 * digits are converted to raw data one octet at a time, so strlen(hex) must be
 * a multiple of two.  The output buffer must be able to hold at least
 * strlen(hex_digits) / 2 characters.  Conversion will stop when the end of the
 * string or the first non-hexadecimal character is reached.  Returns the
 * number of output bytes generated on success, -1 on error.
 */
ssize_t nl_from_hex(const char *hex, uint8_t *data)
{
	char hexbuf[3] = { 0, 0, 0 };
	ssize_t count = 0;

	if(CHECK_NULL(hex) || CHECK_NULL(data)) {
		return -1;
	}

	while(isxdigit(hex[0]) && isxdigit(hex[1])) {
		hexbuf[0] = hex[0];
		hexbuf[1] = hex[1];
		*data++ = strtoul(hexbuf, NULL, 16);
		hex += 2;
		count++;
	}

	return count;
}

/*
 * Modifies the given string in place to remove any non-hexadecimal-digit
 * characters.  Uppercase digits A-F are converted to lowercase.
 */
void nl_keep_only_hex(char *str)
{
	char *dest = str;

	while(*str) {
		if(isxdigit(*str)) {
			*dest = tolower(*str);
			dest++;
		}
		str++;
	}

	*dest = 0;
}

/*
 * Calls the given callback for each block of data separated by line ending
 * characters in the given data.  The data.size parameter should not include a
 * terminating 0 byte.  The callback will be called at least once for a
 * non-empty string, even if there are no line terminators.  The callback
 * should not expect the strings it is given to be 0-terminated, and should not
 * retain any pointers to data it is given.  The following line terminators are
 * recognized as a single line terminator: \r alone, \r followed by \n, \n
 * alone.  NULL data is treated as an empty string.  Returns the number of
 * lines on success (always >= 1 for non-empty strings), -1 if cb was NULL.
 */
int nl_split_lines(struct nl_raw_data data, nl_line_callback cb, void *cb_data)
{
	size_t start;
	size_t off;
	int count;

	if(CHECK_NULL(cb)) {
		return -1;
	}

	if(data.data == NULL) {
		return 0;
	}

	// TODO: Write a generic tokenization/splitting function that accepts
	// delimiters as parameters?

	for(count = 0, start = 0, off = 0; off < data.size; off++) {
		if(data.data[off] == '\r' || data.data[off] == '\n') {
			if(cb((struct nl_raw_data){.size = off - start, .data = data.data + start}, cb_data)) {
				return count + 1;
			}

			if(off < data.size - 1 && data.data[off] == '\r' && data.data[off + 1] == '\n') {
				off++;
			}
			start = off + 1;
			count++;
		}
	}

	// Final non-terminated line
	if(start < data.size) {
		if(cb((struct nl_raw_data){.size = data.size - start, .data = data.data + start}, cb_data)) {
			return count + 1;
		}
		count++;
	}

	return count;
}
