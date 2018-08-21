/*
 * URL escaping/unescaping functions.
 * Copyright (C)2014 Mike Bourgeous.  Released under AGPLv3 in 2018.
 */
#ifndef NLUTILS_URL_H_
#define NLUTILS_URL_H_

/*
 * Percent-encodes reserved and non-unreserved URL characters in the given
 * string.  If encode_space is nonzero, then ASCII space characters will be
 * encoded as %20 instead of +.  If allow_reserved is nonzero, then
 * URI-permitted reserved characters, like / and ?, will not be encoded.  The
 * returned string must be freed using free().
 */
char *nl_url_encode(const char *str, int encode_space, int allow_reserved);

/*
 * Percent-decodes URI escape sequences of the form %xx in the given string,
 * where x is a hexadecimal digit.  The returned string must be freed using
 * free().  Zero bytes (%00) are not decoded.  If ignore_plus is nonzero, then
 * '+' characters will not be decoded as spaces.
 */
char *nl_url_decode(const char *str, int ignore_plus);

#endif /* NLUTILS_URL_H_ */
