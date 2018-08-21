/*
 * Variant datatype for passing arbitrary values.
 * Based on core_type from logic_system.
 * Copyright (C)2014-2017 Mike Bourgeous.  Released under AGPLv3 in 2018.
 */
#ifndef NLUTILS_VARIANT_H_
#define NLUTILS_VARIANT_H_

#include <stdio.h>
#include <stdint.h>

/*
 * Data type stored in a struct nl_variant.
 */
enum nl_vartype {
	INVALID = -1,
	ANY = 0,

	INTEGER,	// Platform's standard int type
	FLOAT,		// Platform's standard float type
	STRING,		// NUL-terminated string
	DATA,		// Raw data with length

	MAX_TYPE = DATA
};

/*
 * Structure for raw data stored within a struct nl_varvalue.
 */
struct nl_raw_data {
	size_t size;
	char *data;
};

/*
 * Data storage for a struct nl_variant.
 */
union nl_varvalue {
	intptr_t any;
	int integer;
	float floating;
	char *string;
	struct nl_raw_data *data;
};

/*
 * Variant data type that can store an integer, floating point number,
 * NUL-terminated string, or raw data with length.  It is recommended that
 * struct nl_variant be passed by value, not by pointer.
 */
struct nl_variant {
	enum nl_vartype type;
	union nl_varvalue value;
};


/*
 * Serializable names of the variant data types.  These must remain constant
 * throughout a stable API version series.
 */
extern char *nl_vartype_names[];


/*
 * Duplicates the contents given raw data structure, returning an nl_raw_data
 * struct on the stack.  Free the .data contents of the returned structure with
 * free() if it is not NULL.  See also nl_duplicate_data(), which allocates the
 * nl_raw_data struct on the heap.  Returns (struct nl_raw_data){.size =
 * SIZE_MAX, .data = NULL} on error.
 *
 * Errors include malloc() failure and trying to copy NULL data with a nonzero
 * size.
 */
struct nl_raw_data nl_copy_data(const struct nl_raw_data data);

/*
 * Duplicates the given raw data structure using a deep copy.  The return value
 * should be destroyed using nl_destroy_data().  Returns NULL on error.
 *
 * Errors include malloc() failure and trying to copy NULL data with a nonzero
 * size.
 */
struct nl_raw_data *nl_duplicate_data(const struct nl_raw_data *data);

/*
 * Frees the given raw data structure and its contained data.  This should only
 * be called for structures that were allocated using nl_duplicate_data() or a
 * malloc() of both the data structure and its contained data.
 */
void nl_destroy_data(struct nl_raw_data *data);

/*
 * Duplicates a struct nl_variant, copying the string or data into a newly
 * allocated buffer if the variant's type is STRING or DATA.  The returned
 * value must later be freed with nl_destroy_variant().  Returns a variant of
 * type INVALID on error.
 */
struct nl_variant nl_duplicate_variant(struct nl_variant value);

/*
 * Frees the memory held by the given variant's contents.  The given value's
 * string or data, if type is STRING or DATA, must have been allocated by
 * malloc(), nl_duplicate_data(), or an equivalent.
 */
void nl_destroy_variant(struct nl_variant value);

/*
 * Stores the typical range and default value of the given numeric variant type
 * in the given value pointers (which may be NULL to ignore a value).  Takes no
 * action if type is not a numeric variant type.
 */
void nl_vartype_range(enum nl_vartype type, union nl_varvalue *min, union nl_varvalue *max, union nl_varvalue *def);

/*
 * Clamps the range of the given value to the given min and max according to
 * the given type.  Only integer and floating point types are clamped.
 */
union nl_varvalue nl_clamp_varvalue(enum nl_vartype type, union nl_varvalue value, union nl_varvalue min, union nl_varvalue max);

/*
 * Prints the given variant value to output, formatted for lossless textual
 * serialization.  Raw data values are not printed.  Prints "Unknown type %d"
 * if the value's type is invalid.  Returns the number of bytes written on
 * success, negative on error.
 */
int nl_fprint_variant(FILE *output, struct nl_variant value);

/*
 * Prints the given variant value to output, formatted for display.
 */
void nl_display_variant(FILE *output, struct nl_variant value);

/*
 * Stores the given data in a newly-allocated string, formatted for display.
 * The data is formatted twice: once to calculate the length of the string, and
 * again to store the data in the string.  Returns NULL if a new string could
 * not be allocated.  The returned buffer must be free()d.
 */
char *nl_variant_to_string(struct nl_variant value);

/*
 * Attempts to convert the given string to the given variant type.  Stores the
 * result in *result and returns 0 on success.  Returns -1 and does not modify
 * *result on error.  Integer values are treated as base 16 if prefixed with
 * 0x, base 10 otherwise (leading zeros are ignored).  For integer values, the
 * string "true" is treated as 1, "false" as 0.  Initial whitespace is skipped
 * for numeric types.  Conversion of numeric types is performed by strtol and
 * strtof.  Conversion of string types stops at the first newline or carriage
 * return, or at the end of the string.  Strings are de-escaped using
 * nl_unescape_string().  If the considered part of str contains exactly "\\0",
 * then a NULL string is stored in *result.  Invalid integer and floating point
 * values will not be converted.  String and data values must later be freed
 * using nl_destroy_variant().
 */
int nl_string_to_varvalue(enum nl_vartype type, const char *str, union nl_varvalue *result);

/*
 * Compares the given variant types for equality and ordering.  Returns < 0 if
 * val1 < val2, 0 if val1 == val2, or > 0 if val1 > val2.  For numeric types,
 * the comparison is obvious.  For string types, strcmp() is used, with a NULL
 * string coming before a non-NULL string.  For data types, the size of the
 * data is compared, with a NULL data coming before a zero-sized non-NULL data.
 * For any other type, 1 is always returned.
 */
int nl_compare_varvalues(enum nl_vartype type, union nl_varvalue val1, union nl_varvalue val2);

/*
 * Compares the given variants for equality and ordering.  If the variants have
 * different types, their types will be compared as integers (e.g. INTEGER will
 * sort before FLOAT).  Otherwise, their values will be compared using the same
 * rules as nl_compare_varvalues().
 */
int nl_compare_variants(struct nl_variant val1, struct nl_variant val2);


/*
 * Macro that returns the serializable name of a variant type.  References type
 * multiple times, so do not use with an expression with side effects.
 */
#define NL_VARTYPE_NAME(type) (((type) < 0 || (type) > MAX_TYPE) ? "invalid" : nl_vartype_names[(type)])

/*
 * Macro that uses a compound initializer to return a variant for the given raw
 * data.  Do not destroy the resulting variant with nl_destroy_variant()!
 */
#define NL_DATA_TO_VARIANT(d) ((struct nl_variant){ .type = DATA, .value = { .data = &(struct nl_raw_data){ .size = d.size, .data = d.data } } })


#endif /* NLUTILS_VARIANT_H_ */
