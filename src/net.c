/*
 * Generic network-related functions, excluding actual network communication.
 * Copyright (C)2011 Mike Bourgeous.  Released under AGPLv3 in 2018.
 */
#include <stdlib.h>
#include <ctype.h>

#include "nlutils.h"

/*
 * Parses the MAC address in mac_in into an unbroken string of lowercase
 * hexadecimal digits in the buffer pointed to by mac_out.  If mac_out is NULL,
 * the formatting of the MAC address is verified, but the parsed MAC address is
 * not stored anywhere.  If separator is not zero, then it will be inserted
 * between each octet in the parsed MAC address.  The buffer pointed to by
 * mac_out must be able to hold at least 13 bytes, or 18 bytes if separator is
 * not zero.  mac_out will not be modified if the MAC address is invalid.
 * Returns 0 on success, -EINVAL on invalid MAC format, -EFAULT if mac_in is
 * NULL (a simple check for zero is sufficient to detect errors).
 */
int nl_parse_mac(const char *mac_in, char *mac_out, char separator)
{
	char outbuf[18];
	char *ptr;
	enum {
		WANT_DIGIT_1, // Expecting first digit of an octet
		WANT_DIGIT_1_OR_SEP, // Expecting first digit of an octet or a separator
		WANT_DIGIT_2, // Expecting second digit of an octet
		WANT_SEP, // Expecting a separator
		WANT_END, // Expecting a null byte
		INVALID, // MAC address was invalid, exit loop
		DONE, // MAC address was valid, exit loop
	} state, next;
	int expect_sep = 0; // Set to separator character if/when first separator is encountered
	int c;

	if(mac_in == NULL) {
		return -EFAULT;
	}

	for(ptr = outbuf, state = WANT_DIGIT_1; state != INVALID && state != DONE; state = next) {
		c = *mac_in++;
		next = INVALID;
		switch(state) {
			case WANT_DIGIT_1_OR_SEP: // This state is only reached after the first octet
				if(c == '-' || c == ':') {
					expect_sep = c;
					c = *mac_in++;
				}
			// Intentionally falls through
			case WANT_DIGIT_1:
				if(isxdigit(c)) {
					*ptr++ = tolower(c);
					next = WANT_DIGIT_2;
				}
				break;

			case WANT_DIGIT_2: // This state could be "unrolled" after WANT_DIGIT_1
				if(isxdigit(c)) {
					*ptr++ = tolower(c);
					if(separator && ptr - outbuf < 17) {
						*ptr++ = separator;
					}
					if(ptr - outbuf == (separator ? 17 : 12)) {
						next = WANT_END;
					} else if(ptr - outbuf > (separator ? 3 : 2)) {
						next = expect_sep ? WANT_SEP : WANT_DIGIT_1;
					} else {
						next = WANT_DIGIT_1_OR_SEP;
					}
				}
				break;

			case WANT_SEP:
				if(c == expect_sep) {
					next = WANT_DIGIT_1;
				}
				break;

			case WANT_END:
				if(c == 0) {
					next = DONE;
				}
				break;
				
			default:
				ERROR_OUT("An invalid state was reached when parsing a MAC address.\n");
				return -EIO;
		}
	}

	if(mac_out != NULL && state == DONE) {
		outbuf[separator ? 17 : 12] = 0;
		memcpy(mac_out, outbuf, separator ? 18 : 13);
	}

	return state == DONE ? 0 : -EINVAL;
}
