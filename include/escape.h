/*
 * String escape/unescape functions.
 * Copyright (C)2011 Mike Bourgeous.  Released under AGPLv3 in 2018.
 */
#ifndef NLUTILS_ESCAPE_H_
#define NLUTILS_ESCAPE_H_

/*
 * Values for dequote parameter to nl_unescape_string().
 */
enum nl_unescape_dequote {
	ESCAPE_NO_DEQUOTE = 0,	// Always unescape, never dequote
	ESCAPE_DEQUOTE = 1,	// Always unescape, always dequote
	ESCAPE_IF_QUOTED = 2,	// Always dequote, unescape only if quoted
};

/*
 * Returns the number of additional bytes needed to escape the given string for
 * serialization safety.  Does not check to see if str is NULL.
 */
int nl_count_escapes(char *str);

/*
 * Escapes serialization-unsafe characters in the given string, expanding it
 * using realloc() if more than *size bytes are required (including the
 * terminating zero byte).  If the string is resized, the new size is stored in
 * *size, and the new pointer is stored in *str.  Returns 0 on success, or -1
 * if a temporary expansion buffer couldn't be allocated or the string buffer
 * couldn't be resized.  Use nl_escape_string() only for 0-terminated strings.
 */
int nl_escape_string(char **str, size_t *size);

/*
 * De-escapes the given string, using the same set of escaped characters as
 * nl_escape_string().  If include_zero is nonzero, then zero bytes (which
 * would normally indicate the end of a string) will be included in the output.
 * If the string included any escaped zero bytes, the number of removed
 * characters can be used to determine the actual length of the resulting data.
 * If dequote is ESCAPE_DEQUOTE, then surrounding quotation marks, if present,
 * will be removed.  If dequote is ESCAPE_IF_QUOTED, then unescaping will only
 * occur if the string is quoted.  Non-escaped quotation marks in the middle of
 * a string are left unmodified.  If a leading quotation mark is found but a
 * trailing quotation mark is escaped or absent, the leading quotation mark is
 * still removed.  Returns the number of characters removed from the string, or
 * -1 if str is NULL.
 */
int nl_unescape_string(char *str, int include_zero, enum nl_unescape_dequote dequote);

/*
 * Returns the number of additional bytes needed to escape all non-printable
 * characters in the given data.  Does not check to see if data is NULL.
 */
size_t nl_count_data_escapes(const char *data, size_t data_size);

/*
 * Escapes non-printable characters in the given data, expanding it using
 * realloc() if more than *buf_size bytes are required.  The new data length is
 * stored in *data_size, and the new buffer size (if it is changed) is stored
 * in *buf_size.  If add_null is nonzero, a single zero byte is appended to the
 * escaped data.  Returns 0 on success, -1 on memory allocation error or an
 * invalid parameter.
 */
int nl_escape_data(char **data, size_t *data_size, size_t *buf_size, int add_null);

#endif /* NLUTILS_ESCAPE_H_ */
