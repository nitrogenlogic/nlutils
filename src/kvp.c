/*
 * Simple key-value pair line parser (extracted from logic_system and knc).
 * Copyright (C)2014 Mike Bourgeous.  Released under AGPLv3 in 2018.
 */
#include <stdlib.h>
#include <ctype.h>
#include <sys/types.h>

#include "nlutils.h"

/*
 * Sends a string to a variant-receiving callback.
 */
static inline void nl_kvp_send_string(struct nl_kvp_wrap wrapper, char *key, char *value)
{
	wrapper.cb(wrapper.data, key, value, (struct nl_variant){STRING, (union nl_varvalue)value});
}

/*
 * Sends a hexadecimal integer to a variant-receiving callback.
 */
static inline void nl_kvp_send_hex(struct nl_kvp_wrap wrapper, char *key, char *value)
{
	struct nl_variant variant = {
		.type = INTEGER,
		.value = (union nl_varvalue)(int)strtol(value + 2, NULL, 16)
	};
	wrapper.cb(wrapper.data, key, value, variant);
}

/*
 * Sends a decimal integer to a variant-receiving callback.
 */
static inline void nl_kvp_send_int(struct nl_kvp_wrap wrapper, char *key, char *value)
{
	wrapper.cb(wrapper.data, key, value, (struct nl_variant){INTEGER, (union nl_varvalue)atoi(value)});
}

/*
 * Sends a floating point number to a variant-receiving callback.
 */
static inline void nl_kvp_send_float(struct nl_kvp_wrap wrapper, char *key, char *value)
{
	wrapper.cb(wrapper.data, key, value, (struct nl_variant){.type = FLOAT, .value.floating = atof(value)});
}

/*
 * Sends the integer 1 to the variant-receiving callback.
 */
static inline void nl_kvp_send_true(struct nl_kvp_wrap wrapper, char *key, char *value)
{
	wrapper.cb(wrapper.data, key, value, (struct nl_variant){.type = INTEGER, .value.integer = 1});
}

/*
 * Sends the integer 0 to the variant-receiving callback.
 */
static inline void nl_kvp_send_false(struct nl_kvp_wrap wrapper, char *key, char *value)
{
	wrapper.cb(wrapper.data, key, value, (struct nl_variant){.type = INTEGER, .value.integer = 0});
}

/*
 * Wraps an nl_variant-receiving callback for use with nl_parse_kvp().  Pass a
 * struct nl_kvp_wrap as cb_data to nl_parse_kvp(), using nl_kvp_wrapper as the
 * callback.  The receiving function is responsible for duplicating the
 * variant, and should not keep a pointer to strings that are passed.  The
 * unquoted strings "true" and "false" will be parsed as 1 and 0 respectively.
 *
 * Example:
 *   void typed_cb(void *cb_data, char *key, char *strvalue, struct nl_variant value)
 *   {
 *     printf("Received %s=%s (type %s) in %s\n", key, strvalue, NL_VARTYPE_NAME(value.type), (char *)cb_data);
 *   }
 *
 *   void some_func(void)
 *   {
 *     nl_parse_kvp(
 *       "key1=val1 key2=0",
 *       nl_kvp_wrapper,
 *       &(struct nl_kvp_wrap){ .cb = typed_cb, .data = "test callback" }
 *     );
 *   }
 *
 *   Prints:
 *     Received key1=val1 (type string) in test callback
 *     Received key2=0 (type int) in test callback
 */
void nl_kvp_wrapper(void *data, char *key, char *value, int quoted_value)
{
	struct nl_kvp_wrap wrapper = *(struct nl_kvp_wrap *)data;
	int exponent = 0;
	int period = 0;
	char *ptr;

	// This code would be cleaner as an explicit state machine.  Would need
	// to benchmark before deciding to switch, though.
	//
	// TODO: Convert this into a nl_parse_variant() method?

	if(quoted_value) {
		// Quoted values marks are always strings
		nl_kvp_send_string(wrapper, key, value);
	} else if(!strcmp(value, "true")) {
		nl_kvp_send_true(wrapper, key, value);
	} else if(!strcmp(value, "false")) {
		nl_kvp_send_false(wrapper, key, value);
	} else if(!isdigit(value[0]) && (
				value[1] == 0 ||
				(value[0] != '+' && value[0] != '-' && value[0] != '.')
					)) {
		// Non-numeric introductory characters are strings
		nl_kvp_send_string(wrapper, key, value);
	} else if(value[0] == '0' && value[1] == 'x') {
		// Hexadecimal integers
		for(ptr = value + 2; *ptr; ptr++) {
			if(!isxdigit(*ptr)) {
				// Non-hex after hex
				nl_kvp_send_string(wrapper, key, value);
				return;
			}
		}
		nl_kvp_send_hex(wrapper, key, value);
	} else {
		// Check for leading period (initial character was otherwise
		// checked for digit or sign by a previous conditional)
		period = value[0] == '.';

		for(ptr = value + 1; *ptr; ptr++) {
			if(*ptr == '.') {
				if(exponent || period || (!isdigit(ptr[1]) && ptr[1] != 0)) {
					// Two periods, or exponent followed by period, or period
					// followed by non-digit
					nl_kvp_send_string(wrapper, key, value);
					return;
				}
				period = 1;
			} else if(isdigit(ptr[-1]) && (*ptr == 'e' || *ptr == 'E')) {
				if(exponent) {
					// Two exponents
					nl_kvp_send_string(wrapper, key, value);
					return;
				} else if(ptr[1] == '+' || ptr[1] == '-') {
					if(!isdigit(ptr[2])) {
						// Exponent sign followed by non-digit
						nl_kvp_send_string(wrapper, key, value);
						return;
					}
					ptr++;
				} else if(!isdigit(ptr[1])) {
					// Exponent followed by non-digit/non-sign
					nl_kvp_send_string(wrapper, key, value);
					return;
				}
				exponent = 1;
			} else if(!isdigit(*ptr)) {
				// Non-digit after start of number
				nl_kvp_send_string(wrapper, key, value);
				return;
			}
		}

		if(exponent || period) {
			nl_kvp_send_float(wrapper, key, value);
		} else {
			nl_kvp_send_int(wrapper, key, value);
		}
	}
}

/*
 * Calls the given callback with a single key/value pair.
 */
static inline void send_pair(const char *kvp_line, nl_kvp_cb callback, void *cb_data, size_t key_start, size_t key_length, size_t value_start, size_t value_length)
{
	char *key;
	char *value;
	int quoted_value = 0;

	key = nl_strndup_term(kvp_line + key_start, key_length);
	if(key == NULL) {
		ERROR_OUT("Error duplicating key for de-escaping.\n");
		return;
	}

	value = nl_strndup_term(kvp_line + value_start, value_length);
	if(value == NULL) {
		ERROR_OUT("Error duplicating value for de-escaping.\n");
		free(key);
		return;
	}

	if(key[0] == '"') {
		nl_unescape_string(key, 0, ESCAPE_IF_QUOTED);
	}
	if(value[0] == '"') {
		nl_unescape_string(value, 0, ESCAPE_IF_QUOTED);
		quoted_value = 1;
	}

	callback(cb_data, key, value, quoted_value);

	free(key);
	free(value);
}

/*
 * Parses optionally quoted key-value pairs.  A key with an equal sign but no
 * value will be ignored.  The given callback will be called for each key-value
 * pair found, with the given callback data.  The callback should not store
 * pointers to the strings it is given, as they will be freed after the
 * callback returns.  The key and value will both be dequoted and unescaped.
 *
 * Example pairs:
 *   a=b "c"=d e=f=g "e"="f=g" "g \"h i j"=" k\"l\"mn "
 */
void nl_parse_kvp(const char *kvp_line, nl_kvp_cb callback, void *cb_data)
{
	enum {
		EXPECT_KEY,	   // Skipping whitespace, looking for key
		SKIP_KEY,	   // Invalid character found in key name, looking for whitespace
		READ_KEY,	   // Read key name, looking for equals
		READ_QUOTED_KEY,   // Read key name, looking for quote
		EXPECT_EQUALS,	   // Looking for equals after quoted key
		EXPECT_VALUE,	   // Looking for quote or non-space after key
		READ_VALUE,	   // Read value, looking for whitespace
		READ_QUOTED_VALUE, // Read value, looking for quote
	} state = EXPECT_KEY;

	size_t key_start = 0;
	size_t key_length = 0;

	size_t value_start = 0;
	size_t value_length = 0;

	const char *ptr = kvp_line;
	size_t off = 0;

	if(CHECK_NULL(kvp_line) || CHECK_NULL(callback)) {
		return;
	}

	while(*ptr) {
		switch(state) {
			case EXPECT_KEY:
				// Skip whitespace.
				do {
					if(*ptr == '=') {
						state = SKIP_KEY;
					} else if(!isspace(*ptr)) {
						key_start = off;
						if(*ptr == '"') {
							state = READ_QUOTED_KEY;
						} else {
							state = READ_KEY;
						}
					}
					ptr++;
					off++;
				} while(state == EXPECT_KEY && *ptr);
				break;

			case SKIP_KEY:
				// Skip non-whitespace due to a key name starting with equals.
				while(!isspace(*ptr) && *ptr) {
					ptr++;
					off++;
				}
				state = EXPECT_KEY;
				break;

			case READ_KEY:
				// Read key characters until equals sign.
				// Ignore key if whitespace is encountered.
				do {
					if(isspace(*ptr)) {
						state = EXPECT_KEY;
					} else if(*ptr == '=') {
						state = EXPECT_VALUE;
						key_length = off - key_start;
					}
					ptr++;
					off++;
				} while(state == READ_KEY && *ptr);
				break;

			case READ_QUOTED_KEY:
				// Read key characters until non-escaped quote.
				do {
					if(*ptr == '"') {
						state = EXPECT_EQUALS;
					} else if(*ptr == '\\' && ptr[1] == '"') {
						ptr++;
						off++;
					}
					ptr++;
					off++;
				} while(state == READ_QUOTED_KEY && *ptr);
				break;

			case EXPECT_EQUALS:
				// Switch to value if equals, expect key otherwise.
				if(*ptr == '=') {
					state = EXPECT_VALUE;
					key_length = off - key_start;
				} else {
					state = EXPECT_KEY;
				}
				ptr++;
				off++;
				break;

			case EXPECT_VALUE:
				// Switch to key if whitespace, quoted value if
				// quote, unquoted value otherwise.
				if(isspace(*ptr)) {
					state = EXPECT_KEY;
				} else {
					value_start = off;
					if(*ptr == '"') {
						state = READ_QUOTED_VALUE;
					} else {
						state = READ_VALUE;
					}
				}
				ptr++;
				off++;
				break;

			case READ_VALUE:
				// Read value characters until whitespace.
				do {
					if(isspace(*ptr)) {
						value_length = off - value_start;
						send_pair(kvp_line, callback, cb_data, key_start, key_length, value_start, value_length);
						state = EXPECT_KEY;
					}
					ptr++;
					off++;
				} while(state == READ_VALUE && *ptr);
				break;

			case READ_QUOTED_VALUE:
				// Read value characters until non-escaped
				// quote.
				do {
					if(*ptr == '"') {
						value_length = off - value_start + 1;
						send_pair(kvp_line, callback, cb_data, key_start, key_length, value_start, value_length);
						state = EXPECT_KEY;
					} else if(*ptr == '\\' && ptr[1] == '"') {
						ptr++;
						off++;
					}
					ptr++;
					off++;
				} while(state == READ_QUOTED_VALUE && *ptr);
				break;
		}
	}

	// Handle final pair
	if(state == READ_VALUE || state == READ_QUOTED_VALUE) {
		value_length = off - value_start;
		send_pair(kvp_line, callback, cb_data, key_start, key_length, value_start, value_length);
	}
}

/*
 * Parses optionally quoted key-value pairs, converting unquoted values (but
 * not keys) to the closest matching nl_variant type.  The given callback will
 * be called for each key-value pair found, with the given callback data.  The
 * receiving function is responsible for duplicating the variant, and should
 * not keep a pointer to strings that are passed.
 *
 * Example pairs:
 *   a=b "c"=d e=f=g "e"="f=g" "g \"h i j"=" k\"l\"mn "
 *
 * Example:
 *   void typed_cb(void *cb_data, char *key, char *strvalue, struct nl_variant value)
 *   {
 *     printf("Received %s=%s (type %s) in %s\n", key, strvalue, NL_VARTYPE_NAME(value.type), (char *)cb_data);
 *   }
 *
 *   void some_func(void)
 *   {
 *     nl_parse_kvp_variant(
 *       "key1=val1 key2=0",
 *       typed_cb,
 *       "test callback"
 *     );
 *   }
 *
 *   Prints:
 *     Received key1=val1 (type string) in test callback
 *     Received key2=0 (type int) in test callback
 */
void nl_parse_kvp_variant(const char *kvp_line, nl_typed_kvp_cb callback, void *cb_data)
{
	nl_parse_kvp(kvp_line, nl_kvp_wrapper, &(struct nl_kvp_wrap){ .cb = callback, .data = cb_data });
}

// Callback for use by nl_parse_kvp() in nl_hash_parse()
static void nl_build_hash_cb(void *h, char *key, char *value, int quoted)
{
	struct nl_hash *hash = h;

	(void)quoted; // unused parameter

	nl_hash_set(hash, key, value);
}

/*
 * Adds the key-value pairs from the given string into the given hash table
 * using nl_hash_set().
 */
void nl_hash_parse(struct nl_hash *hash, char *kvp)
{
	if(CHECK_NULL(hash) || CHECK_NULL(kvp)) {
		return;
	}

	nl_parse_kvp(kvp, nl_build_hash_cb, hash);
}
