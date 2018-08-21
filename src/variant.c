/*
 * Variant datatype for passing arbitrary values.
 * Based on nl_vartype from logic_system.
 * Copyright (C)2014-2017 Mike Bourgeous.  Released under AGPLv3 in 2018.
 */
#include <stdlib.h>	// for malloc() etc.
#include <limits.h>	// for INT_MIN etc.
#include <ctype.h>	// for isdigit()
#include <math.h>	// for INFINITY

#include "nlutils.h"

/*
 * Serializable names of the variant data types.  These must remain constant
 * throughout a stable API version series.
 */
char *nl_vartype_names[] = {
	"any",
	"int",
	"float",
	"string",
	"raw_data",
};

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
struct nl_raw_data nl_copy_data(const struct nl_raw_data data)
{
	static const struct nl_raw_data error = { .size = SIZE_MAX, .data = NULL };
	struct nl_raw_data d;

	if(data.size != 0 && data.data == NULL) {
		ERROR_OUT("Unable to copy non-zero-sized data with null contents\n");
		return error;
	}

	d.size = data.size;

	if(d.size > 0) {
		d.data = malloc(d.size);
		if(d.data == NULL) {
			ERRNO_OUT("Unable to allocate memory for raw data");
			return error;
		}
		memcpy(d.data, data.data, d.size);
	} else {
		d.data = NULL;
	}

	return d;
}

/*
 * Duplicates the given raw data structure using a deep copy.  The return value
 * should be destroyed using nl_destroy_data().  Returns NULL on error.
 *
 * Errors include malloc() failure and trying to copy NULL data with a nonzero
 * size.
 */
struct nl_raw_data *nl_duplicate_data(const struct nl_raw_data *data)
{
	struct nl_raw_data *d;

	if(data == NULL || (data->size != 0 && data->data == NULL)) {
		ERROR_OUT("Unable to duplicate null data, or non-zero-sized data with null contents\n");
		return NULL;
	}

	d = malloc(sizeof(struct nl_raw_data));
	if(d == NULL) {
		ERRNO_OUT("Unable to allocate memory for raw data structure");
		return NULL;
	}

	*d = nl_copy_data(*data);
	if(d->size == SIZE_MAX && d->data == NULL) {
		free(d);
		return NULL;
	}

	return d;
}

/*
 * Frees the given raw data structure and its contained data.  This should only
 * be called for structures that were allocated using nl_duplicate_data() or a
 * malloc() of both the data structure and its contained data.
 */
void nl_destroy_data(struct nl_raw_data *data)
{
	if(data != NULL) {
		if(data->data != NULL) {
			free(data->data);
		}
		free(data);
	}
}

/*
 * Duplicates a struct nl_variant, copying the string or data into a newly
 * allocated buffer if the variant's type is STRING or DATA.  The returned
 * value must later be freed with nl_destroy_variant().  Returns a variant of
 * type INVALID on error.
 */
struct nl_variant nl_duplicate_variant(struct nl_variant value)
{
	if(value.type == STRING) {
		if(value.value.string != NULL) {
			value.value.string = strdup(value.value.string);
			if(value.value.string == NULL) {
				ERRNO_OUT("Error duplicating a string variant");
				goto error;
			}
		}
	} else if(value.type == DATA) {
		if(value.value.data != NULL) {
			value.value.data = nl_duplicate_data(value.value.data);
			if(value.value.data == NULL) {
				ERROR_OUT("Error duplicating a data variant.\n");
				goto error;
			}
		}
	}

	return value;

error:
	return (struct nl_variant){.type = INVALID};
}

/*
 * Frees the memory held by the given variant's contents.  The given value's
 * string or data, if type is STRING or DATA, must have been allocated by
 * malloc(), nl_duplicate_data(), or an equivalent.
 */
void nl_destroy_variant(struct nl_variant value)
{
	DEBUG_OUT("Attempting to destroy a %s variant\n", NL_VARTYPE_NAME(value.type));

	if(value.type == STRING) {
		if(value.value.string != NULL) {
			free(value.value.string);
		}
	} else if(value.type == DATA) {
		if(value.value.data != NULL) {
			nl_destroy_data(value.value.data);
		}
	}
}

/*
 * Stores the typical range and default value of the given numeric variant type
 * in the given value pointers (which may be NULL to ignore a value).  Takes no
 * action if type is not a numeric variant type.
 */
void nl_vartype_range(enum nl_vartype type, union nl_varvalue *min, union nl_varvalue *max, union nl_varvalue *def)
{
	if(type == INTEGER) {
		if(min) {
			(*min).integer = INT_MIN;
		}
		if(max) {
			(*max).integer = INT_MAX;
		}
		if(def) {
			(*def).integer = 0;
		}
	} else if(type == FLOAT) {
		if(min) {
			(*min).floating = -INFINITY;
		}
		if(max) {
			(*max).floating = INFINITY;
		}
		if(def) {
			(*def).floating = 0;
		}
	}
}

/*
 * Clamps the range of the given value to the given min and max according to
 * the given type.  Only integer and floating point types are clamped.
 */
union nl_varvalue nl_clamp_varvalue(enum nl_vartype type, union nl_varvalue value, union nl_varvalue min, union nl_varvalue max)
{
	switch(type) {
		default:
			break;

		case INTEGER:
			if(value.integer < min.integer) {
				return min;
			}
			if(value.integer > max.integer) {
				return max;
			}
			break;

		case FLOAT:
			if(value.floating < min.floating) {
				return min;
			}
			if(value.floating > max.floating) {
				return max;
			}
			break;
	}

	return value;
}

/*
 * Prints the given variant value to output, formatted for lossless textual
 * serialization.  Raw data values are not printed.  Prints "Unknown type %d"
 * if the value's type is invalid.  Returns the number of bytes written on
 * success, negative on error.
 */
int nl_fprint_variant(FILE *output, struct nl_variant value)
{
	union nl_varvalue data = value.value;

	switch(value.type) {
		case ANY:
			return fprintf(output, "0x%08lx", (unsigned long)data.any);

		case INTEGER:
			return fprintf(output, "%d", data.integer);

		case FLOAT:
			if((data.floating < 0.001 && data.floating > -0.001) || 
					data.floating <= -1000 || data.floating >= 1000) {
				return fprintf(output, "%.12e", data.floating);
			}
			return fprintf(output, "%.15f", data.floating);

		case STRING:
			if(data.string != NULL) {
				char *tmp;
				size_t len;
				int ret;

				tmp = nl_strdup(data.string);
				if(tmp == NULL) {
					ERROR_OUT("Error duplicating string\n");
					return -1;
				}
				len = strlen(tmp);

				if(nl_escape_string(&tmp, &len)) {
					ERROR_OUT("Error escaping string\n");
					free(tmp);
					return -1;
				}

				ret = fprintf(output, "%s", tmp);
				free(tmp);
				return ret;
			}
			return fprintf(output, "\\0");

		case DATA:
			// TODO: Store data
			if(data.data == NULL) {
				return fprintf(output, "[NULL raw data]");
			}
			if(data.data->data == NULL) {
				return fprintf(output, "[NULL raw data of length %zu]", data.data->size);
			}
			return fprintf(output, "[Raw data of length %zu]", data.data->size);

		default:
			return fprintf(output, "Unknown type %d", value.type);
	}
}

/*
 * Prints the given variant value to output, formatted for display.
 */
void nl_display_variant(FILE *output, struct nl_variant value)
{
	union nl_varvalue data = value.value;

	switch(value.type) {
		case ANY:
			fprintf(output, "0x%08lx", (unsigned long)data.any);
			break;

		case INTEGER:
			fprintf(output, "%d", data.integer);
			break;

		case FLOAT:
			fprintf(output, "%f", data.floating);
			break;

		case STRING:
			fprintf(output, "%s", data.string == NULL ? "(null)" : data.string);
			break;

		case DATA:
			if(data.data == NULL) {
				fprintf(output, "[NULL raw data]");
			} else {
				if(data.data->data == NULL) {
					fprintf(output, "[NULL raw data of length %zu]", data.data->size);
				} else {
					fprintf(output, "[Raw data of length %zu]", data.data->size);
				}
			}
			break;

		default:
			fprintf(output, "Unknown type %d", value.type);
			break;
	}
}

/*
 * Stores up to length bytes of the string representation of the given variant
 * value in the given string, including the terminating null byte.  Returns the
 * full length of the string that would be stored if there were enough room,
 * including the terminating NUL byte.  Pass NULL for str and 0 for length to
 * measure the length without writing (e.g. for allocating a buffer).
 */
static int nl_store_variant(char *str, int length, struct nl_variant value)
{
	union nl_varvalue data = value.value;
	int ret; // Using ret variable instead of return to allow adding 1 at end

	switch(value.type) {
		case ANY:
			ret = snprintf(str, length, "0x%08lx", (unsigned long)data.any);
			break;

		case INTEGER:
			ret = snprintf(str, length, "%d", data.integer);
			break;

		case FLOAT:
			ret = snprintf(str, length, "%f", data.floating);
			break;

		case STRING:
			// FIXME: ambiguous "(null)" vs. NULL
			ret = snprintf(str, length, "%s", data.string == NULL ? "(null)" : data.string);
			break;

		case DATA:
			if(data.data == NULL) {
				ret = snprintf(str, length, "[NULL raw data]");
			} else {
				if(data.data->data == NULL) {
					ret = snprintf(str, length, "[NULL raw data of length %zu]", data.data->size);
				} else {
					ret = snprintf(str, length, "[Raw data of length %zu]", data.data->size);
				}
			}
			break;

		default:
			ret = snprintf(str, length, "Unknown type %d", value.type);
			break;
	}

	if(ret < 0) {
		ERRNO_OUT("Error storing %s variant in buffer", NL_VARTYPE_NAME(value.type));
		return ret;
	}

	return ret + 1;
}

/*
 * Stores the given data in a newly-allocated string, formatted for display.
 * The data is formatted twice: once to calculate the length of the string, and
 * again to store the data in the string.  Returns NULL if a new string could
 * not be allocated or written.  The returned buffer must be free()d.
 */
char *nl_variant_to_string(struct nl_variant value)
{
	char *buf;
	int length;

	length = nl_store_variant(NULL, 0, value);
	if(length < 0) {
		return NULL;
	}

	buf = malloc(length);
	if(buf == NULL) {
		ERRNO_OUT("Error allocating memory for variant to string conversion");
		return NULL;
	}

	if(nl_store_variant(buf, length, value) < 0) {
		free(buf);
		return NULL;
	}

	return buf;
}

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
int nl_string_to_varvalue(enum nl_vartype type, const char *str, union nl_varvalue *result)
{
	union nl_varvalue tmp = { .any = 0 };
	size_t len;

	if(str == NULL) {
		ERROR_OUT("Cannot convert a null string.\n");
		return -1;
	}
	if(result == NULL) {
		ERROR_OUT("Cannot store conversion to a null result.\n");
		return -1;
	}

	switch(type) {
		default:
		case ANY:
		case DATA:
			ERROR_OUT("Unsupported type: %s\n", nl_vartype_names[type]);
			return -1;

		case INTEGER:
			str += strspn(str, " \t\v\f");
			errno = 0;

			if(!isdigit(str[0]) && str[0] != '-' && str[0] != '+') {
				if(!strcasecmp(str, "true")) {
					tmp.integer = 1;
					*result = tmp;
					return 0;
				}
				if(!strcasecmp(str, "false")) {
					tmp.integer = 0;
					*result = tmp;
					return 0;
				}
				return -1;
			}

			if(str[0] == '0' && str[1] == 'x') {
				tmp.integer = strtol(str + 2, NULL, 16);
			} else {
				tmp.integer = strtol(str, NULL, 10);
			}
			if(errno) {
				ERROR_OUT("Error parsing string '%s' as integer: %s\n", str, strerror(errno));
				return -1;
			}
			break;

		case FLOAT:
			str += strspn(str, " \t\v\f");
			errno = 0;

			tmp.floating = strtof(str, NULL);
			if(errno) {
				ERROR_OUT("Error parsing string '%s' as floating point: %s\n", str, strerror(errno));
				return -1;
			}
			break;

		case STRING:
			// TODO: Add optional length parameter to avoid re-calling strcspn?
			len = strcspn(str, "\r\n");
			if(!strncmp(str, "\\0", MAX_NUM(2, len))) {
				// Special value for null string
				tmp.string = NULL;
			} else {
				tmp.string = nl_strndup_term(str, len);
				if(tmp.string == NULL) {
					ERROR_OUT("Error duplicating string value\n");
					return -1;
				}
				if(nl_unescape_string(tmp.string, 0, 0) < 0) {
					ERROR_OUT("Error unescaping string value\n");
					free(tmp.string);
					return -1;
				}
			}
			break;
	}

	*result = tmp;
	return 0;
}

/*
 * Compares the given variant types for equality and ordering.  Returns < 0 if
 * val1 < val2, 0 if val1 == val2, or > 0 if val1 > val2.  For numeric types,
 * the comparison is obvious.  For string types, strcmp() is used, with a NULL
 * string coming before a non-NULL string.  For data types, the size of the
 * data is compared, with a NULL data coming before a zero-sized non-NULL data.
 * For any other type, 1 is always returned.
 */
int nl_compare_varvalues(enum nl_vartype type, union nl_varvalue val1, union nl_varvalue val2)
{
	switch(type) {
		default:
			return 1;

		case INTEGER:
			return (val1.integer > val2.integer) - (val2.integer > val1.integer);

		case FLOAT:
			if(val1.floating > val2.floating) {
				return 1;
			}
			if(val1.floating == val2.floating) {
				return 0;
			}
			return -1;

		case DATA:
			if(val1.data == val2.data) {
				return 0;
			}

			if(val1.data == NULL) {
				return -1;
			}
			if(val2.data == NULL) {
				return 1;
			}

			if(val1.data->data == val2.data->data && val1.data->size == val2.data->size) {
				return 0;
			}

			if(val1.data->data == NULL) {
				return -1;
			}
			if(val2.data->data == NULL) {
				return 1;
			}

			return val1.data->size - val2.data->size;

		case STRING:
			if(val1.string == val2.string) {
				return 0;
			}

			if(val1.string == NULL) {
				return -1;
			}
			if(val2.string == NULL) {
				return 1;
			}

			return strcmp(val1.string, val2.string);
	}
}

/*
 * Compares the given variants for equality and ordering.  If the variants have
 * different types, their types will be compared as integers (e.g. INTEGER will
 * sort before FLOAT).  Otherwise, their values will be compared using the same
 * rules as nl_compare_varvalues().
 */
int nl_compare_variants(struct nl_variant val1, struct nl_variant val2)
{
	if(val1.type != val2.type) {
		return (val1.type > val2.type) - (val2.type > val1.type);
	}

	return nl_compare_varvalues(val1.type, val1.value, val2.value);
}
