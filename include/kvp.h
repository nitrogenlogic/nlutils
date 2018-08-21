/*
 * Simple key-value pair line parser (extracted from logic_system and knc).
 * Copyright (C)2014 Mike Bourgeous.  Released under AGPLv3 in 2018.
 */
#ifndef NLUTILS_KVP_H_
#define NLUTILS_KVP_H_

#include "variant.h"

/*
 * Callback type given to nl_parse_kvp() to receive only strings for values.
 * Called with the dequoted and unescaped key and value for a pair, with
 * quoted_value nonzero if the value string was quoted (e.g. indicating that it
 * should not be interpreted as a non-string type).
 */
typedef void (*nl_kvp_cb)(void *cb_data, char *key, char *value, int quoted_value);

/*
 * Callback type wrapped in struct nl_kvp_wrap and used with nl_kvp_wrapper to
 * receive detected types for values.
 */
typedef void (*nl_typed_kvp_cb)(void *cb_data, char *key, char *strvalue, struct nl_variant value);


/*
 * Structure to pass to nl_parse_kvp()'s cb_data parameter with nl_kvp_wrapper.
 */
struct nl_kvp_wrap {
	nl_typed_kvp_cb cb;
	void *data;
};


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
void nl_kvp_wrapper(void *data, char *key, char *value, int quoted_value);

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
void nl_parse_kvp(const char *kvp_line, nl_kvp_cb callback, void *cb_data);

/*
 * Parses optionally quoted key-value pairs, converting unquoted values (but
 * not keys) to the closest matching nl_variant type.  The given callback will
 * be called for each key-value pair found, with the given callback data.  The
 * receiving function is responsible for duplicating the variant, and should
 * not keep a pointer to strings that are passed.  The unquoted strings "true"
 * and "false" will be parsed as 1 and 0 respectively.
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
void nl_parse_kvp_variant(const char *kvp_line, nl_typed_kvp_cb callback, void *cb_data);

/*
 * Adds the key-value pairs from the given string into the given associative
 * array using nl_hash_set().  Values are stored as strings.
 */
void nl_parse_kvp_hash(struct nl_hash *hash, char *kvp);

#endif /* NLUTILS_KVP_H_ */
