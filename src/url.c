/*
 * URL escaping/unescaping functions.
 * Copyright (C)2014 Mike Bourgeous.  Released under AGPLv3 in 2018.
 *
 * Originally based on public domain code from http://www.geekhideout.com/urlcode.shtml
 *
 * Public domain notice (applies only to original; not to modified version):
 *   To the extent possible under law, Fred Bulback has waived all copyright
 *   and related or neighboring rights to URL Encoding/Decoding in C. This work
 *   is published from: United States.
 */
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#include "nlutils.h"

// TODO: Support NUL bytes and %00 using nl_variant?

/* Converts a hex character to its integer value */
static inline char nl_url_from_hex(const char ch)
{
	return isdigit(ch) ? ch - '0' : tolower(ch) - 'a' + 10;
}

/* Converts an integer value to its hex character */
static inline char nl_url_to_hex(const uint8_t code)
{
	static char hex[] = "0123456789abcdef";
	return hex[code & 15];
}

/*
 * Returns nonzero if the given character is a URL reserved character, zero
 * otherwise.
 */
static inline int nl_url_is_reserved(const char c)
{
	switch(c) {
		case '!':
		case '#':
		case '$':
		case '&':
		case '\'':
		case '(':
		case ')':
		case '*':
		case '+':
		case ',':
		case '/':
		case ':':
		case ';':
		case '=':
		case '?':
		case '@':
		case '[':
		case ']':
			return 1;

		default:
			return 0;
	}
}

/*
 * Returns zero if the given character should be escaped, nonzero otherwise.
 * If allow_reserved is nonzero, then characters like /, :, ;, and # will
 * return nonzero.
 */
static inline int nl_url_is_allowed(const char c, int allow_reserved)
{
	return isascii(c) && (
			isalnum(c) || c == '-' || c == '_' || c == '.' || c == '~' ||
			(allow_reserved && nl_url_is_reserved(c))
			);
}

/*
 * Percent-encodes reserved and non-unreserved URL characters in the given
 * string.  If encode_space is nonzero, then ASCII space characters will be
 * encoded as %20 instead of +.  If allow_reserved is nonzero, then
 * URI-permitted reserved characters, like / and ?, will not be encoded.  The
 * returned string must be freed using free().
 */
char *nl_url_encode(const char *str, int encode_space, int allow_reserved)
{
	char *buf;
	char *pbuf;

	if(CHECK_NULL(str)) {
		return NULL;
	}

	buf = malloc(strlen(str) * 3 + 1);
	if(buf == NULL) {
		ERRNO_OUT("Error allocating memory for URL-encoded string");
		return NULL;
	}
	pbuf = buf;

	while(*str) {
		if(nl_url_is_allowed(*str, allow_reserved)) {
			*pbuf++ = *str;
		} else if(*str == ' ' && !encode_space) {
			*pbuf++ = '+';
		} else {
			*pbuf++ = '%';
			*pbuf++ = nl_url_to_hex((uint8_t)*str >> 4);
			*pbuf++ = nl_url_to_hex(*str);
		}

		str++;
	}

	*pbuf = '\0';

	return buf;
}

/*
 * Percent-decodes URI escape sequences of the form %xx in the given string,
 * where x is a hexadecimal digit.  The returned string must be freed using
 * free().  Zero bytes (%00) are not decoded.  If ignore_plus is nonzero, then
 * '+' characters will not be decoded as spaces.
 */
char *nl_url_decode(const char *str, int ignore_plus)
{
	char *buf;
	char *pbuf;

	if(CHECK_NULL(str)) {
		return NULL;
	}

	buf = malloc(strlen(str) + 1);
	if(buf == NULL) {
		ERRNO_OUT("Error allocating buffer for URL-decoded string");
		return NULL;
	}
	pbuf = buf;

	while(*str) {
		if(*str == '%') {
			if(str[1] && isxdigit(str[1]) &&
					str[2] && isxdigit(str[2]) &&
					(str[1] != '0' || str[2] != '0')
					) {
				*pbuf++ = nl_url_from_hex(str[1]) << 4 | nl_url_from_hex(str[2]);
				str += 2;
			} else {
				*pbuf++ = *str;
			}
		} else if(*str == '+' && !ignore_plus) {
			*pbuf++ = ' ';
		} else {
			*pbuf++ = *str;
		}

		str++;
	}

	*pbuf = '\0';

	return buf;
}

/*
Log of changes from public domain code:

commit 5b3accf7503665bb732ad2a66669693838d79ce7
Author: Mike Bourgeous <mike@mikebourgeous.com>
Date:   Mon Aug 20 23:11:22 2018 -0700

    Updated/verified copyright and attribution notices.

commit 230aa1de8a288f97a56d942498a1cfdc728fab20
Author: Mike Bourgeous <mike@mikebourgeous.com>
Date:   Sun Sep 14 14:36:49 2014 -0600

    Added ignore_plus parameter to nl_url_decode().

commit f37572107aab8e9d14fd145dd3d925db18983e4a
Author: Mike Bourgeous <mike@mikebourgeous.com>
Date:   Sun Sep 7 16:09:19 2014 -0600

    Added initial tests for URL encoding/decoding functions.

commit 60446e7b3eeb90447b13b449ed3eb517b02f5694
Author: Mike Bourgeous <mike@mikebourgeous.com>
Date:   Sun Sep 7 14:08:45 2014 -0600

    Added URL encoding/decoding functions to the build.

commit 92d9acc411b6161c504ab0efe14072a472f425ae
Author: Mike Bourgeous <mike@mikebourgeous.com>
Date:   Sun Sep 7 14:06:19 2014 -0600

    Added url.h include file for URL encoding/decoding methods.

commit ad9fd2176d20196609d0bd3151ba136b92246d93
Author: Mike Bourgeous <mike@mikebourgeous.com>
Date:   Sun Sep 7 14:05:38 2014 -0600

    Added note to nl_url_decode() about zero bytes.

commit 13470c3a86c3604c73b673ff0b93d0a28e0b0909
Author: Mike Bourgeous <mike@mikebourgeous.com>
Date:   Sun Sep 7 13:57:16 2014 -0600

    Fixed inversion of URL escaping logic.

commit 81c4f9129ee7ca07be771de2f5e6e8bb7de19bac
Author: Mike Bourgeous <mike@mikebourgeous.com>
Date:   Sun Sep 7 13:30:34 2014 -0600

    Added a rudimentary inline test to url.c.

    Compilation command (can also use gcc):
    clang -std=c99 -D_XOPEN_SOURCE=700 -DTEST -O3 -Wall -Wextra \
            -I../include url.c -o /tmp/url -lnlutils

commit f200c496b9c9771910bf65c2a90c5a355eb99ca8
Author: Mike Bourgeous <mike@mikebourgeous.com>
Date:   Sun Sep 7 13:17:04 2014 -0600

    Documented URl encode/decode methods, added space and reserved options.

commit 25a61571cf28a6f9c47e95ee2e174c659f597664
Author: Mike Bourgeous <mike@mikebourgeous.com>
Date:   Sun Aug 31 21:53:02 2014 -0600

    URL encoding/decoding code safety and cleanup:

    Added checks for NULL parameters and failed allocations.
    Added missing ctype.h header.
    Marked hex conversion functions as inline.
    Changed to_hex function to use uint8_t (unnecessary, but seemed cleaner
      since >> does a sign extension for char).
    Added some more TODOs.

commit ad902df018a0a579e30b10717a7344a872fe52aa
Author: Mike Bourgeous <mike@mikebourgeous.com>
Date:   Sun Aug 31 21:46:24 2014 -0600

    Namespaced URL encoding/decoding functions.

commit c169426571b3cd8f39e2095ff75a49a8d7ba87b2
Author: Mike Bourgeous <mike@mikebourgeous.com>
Date:   Sun Aug 31 21:45:14 2014 -0600

    Added a missing semicolon in url.c.

commit 96cba3345dd4136a275be79441ff9e5ccc3b9179
Author: Mike Bourgeous <mike@mikebourgeous.com>
Date:   Sun Aug 31 21:44:40 2014 -0600

    Added protection against %00 sequences in URLs.

commit 197ebd5161ede37f15cf606c9af2296da41321f3
Author: Mike Bourgeous <mike@mikebourgeous.com>
Date:   Sun Aug 31 21:32:00 2014 -0600

    Pass through invalid URL escape sequences unmodified.

commit a3331e03e24974997507a340c6c29b6235526d2c
Author: Mike Bourgeous <mike@mikebourgeous.com>
Date:   Sun Aug 31 21:24:07 2014 -0600

    Formatted URL encoding code to match other nlutils code.

commit 905c11cc42c623e447788168362d585ad8897935
Author: Mike Bourgeous <mike@mikebourgeous.com>
Date:   Sun Aug 31 21:03:11 2014 -0600

    Added unmodified public domain URL escaping code.
*/
