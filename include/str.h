/*
 * Generic string utility functions.
 * Copyright (C)2011 Mike Bourgeous.  Released under AGPLv3 in 2018.
 */
#ifndef NLUTILS_STR_H_
#define NLUTILS_STR_H_

#include <stdint.h>
#include <unistd.h> // For ssize_t

#include "variant.h" // For nl_raw_data

/*
 * Called by nl_split_lines() with a pointer into the source data and a length
 * that excludes the line terminator.  The string will not be 0-terminated, so
 * use something like nl_strndup_term() to copy the string.  Note: if the
 * original data passed to nl_split_lines() contained NUL bytes, they will not
 * have been removed.  Use caution with untrusted data.  Return a nonzero value
 * to stop iteration early.
 */
typedef int (*nl_line_callback)(struct nl_raw_data str, void *cb_data);


/*
 * Duplicates the given string, using malloc() to allocate new memory to fit
 * the length of the string, including the terminating NULL character.  If
 * s is null, then a newly-created zero-length string is returned.  Returns
 * NULL if malloc() fails, and errno will have been set by malloc().  Any
 * non-NULL return value must later be free()d.
 *
 * Just calls strdup() on systems where it is available.
 */
char *nl_strdup(const char *s);

/*
 * Copies n bytes from the given source string into the destination string, and
 * adds a terminating NULL character (so dest must be able to hold n+1 bytes,
 * and src must contain at least n bytes).
 */
void nl_strncpy_term(char *dest, const char *src, size_t n);

/*
 * Duplicates the given string, copying exactly n bytes from the source into a
 * newly-allocated n+1 byte array.  This differs from the GNU strndup()
 * function, as src must contain at least n bytes to copy.  Returns NULL on
 * error, and errno may have been set by malloc().
 */
char *nl_strndup_term(const char *src, size_t n);

/*
 * Returns a pointer to the first character in str that is in accept.  Returns
 * NULL if no matching characters were found within size bytes or if the end of
 * str was reached.  str is not const to allow the returned pointer to be
 * non-const.
 */
char *nl_strnpbrk(char *str, const char *accept, size_t size);

/*
 * Counts the number of occurrences of the given character in the given string.
 */
size_t nl_strcount(const char *str, char c);

/*
 * Returns a number less than, equal to, or greater than zero if the first
 * strlen(start) characters of str are less than, equal to, or greater than
 * start, as determined by strncmp().  Equivalent to calling strncmp(str,
 * start, strlen(start)).  Does not check for NULL parameters.
 */
int nl_strstart(const char *str, const char *start);

/*
 * Returns a number less than, equal to, or greater than zero if the final
 * strlen(end) characters of str are less than, equal to, or greater than end,
 * as determined by strcmp().  If strlen(str) is less than strlen(end), then
 * the result of calling strcmp() on both strings will be returned.  Does not
 * check for NULL parameters.  Both str and end should be shorter than the
 * maximum value of an ssize_t on this platform.
 */
int nl_strend(const char *str, const char *end);

/*
 * Returns the number of bytes the given strings have in common at their
 * beginnings.  Does not check for NULL parameters.
 */
size_t nl_strcommon(const char *a, const char *b);

/*
 * Converts the given data to hexadecimal, using lowercase letters for the
 * digits a-f.  The output buffer must be able to store 2*len+1 chars.  Does
 * not check for errors.
 */
void nl_to_hex(const uint8_t *data, size_t len, char *out);

/*
 * Converts the given hexadecimal string in hex to raw data in data.  Hex
 * digits are converted to raw data one octet at a time, so strlen(hex) must be
 * a multiple of two.  The output buffer must be able to hold at least
 * strlen(hex_digits) / 2 characters.  Conversion will stop when the end of the
 * string or the first non-hexadecimal character is reached.  Returns the
 * number of output bytes generated on success, -1 on error.
 */
ssize_t nl_from_hex(const char *hex, uint8_t *data);

/*
 * Modifies the given string in place to remove any non-hexadecimal-digit
 * characters.  Uppercase digits A-F are converted to lowercase.
 */
void nl_keep_only_hex(char *str);

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
int nl_split_lines(struct nl_raw_data data, nl_line_callback cb, void *cb_data);

#endif /* NLUTILS_STR_H_ */
