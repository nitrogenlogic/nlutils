/*
 * Tests for nl_variant_to_string().
 * Copyright (C)2014 Mike Bourgeous.  Released under AGPLv3 in 2018.
 */
#include <stdio.h>
#include <stdlib.h>
#include <limits.h>
#include <float.h>
#include <math.h>

#include "nlutils.h"

struct str_test {
	struct nl_variant value; // Value to test
	char *match; // String that converted value must match
	unsigned int start:1; // If 1, match start only.  If 0, match entire string.
};

static struct str_test tests[] = {
	{
		.value = { INTEGER, { .integer = 0 } },
		.match = "0",
	},
	{
		.value = { INTEGER, { .integer = 1 } },
		.match = "1",
	},
	{
		.value = { INTEGER, { .integer = -1 } },
		.match = "-1",
	},
	{
		.value = { INTEGER, { .integer = 2147483647 } },
		.match = "2147483647",
	},
	{
		.value = { INTEGER, { .integer = -2147483648 } },
		.match = "-2147483648",
	},
	{
		.value = { FLOAT, { .floating = 0 } },
		.match = "0.0",
		.start = 1,
	},
	{
		.value = { FLOAT, { .floating = 1.125 } },
		.match = "1.125",
		.start = 1,
	},
	{
		.value = { FLOAT, { .floating = -1.125 } },
		.match = "-1.125",
		.start = 1,
	},
	{
		.value = { FLOAT, { .floating = INFINITY } },
		.match = "inf",
		.start = 1,
	},
	{
		.value = { FLOAT, { .floating = -INFINITY } },
		.match = "-inf",
		.start = 1,
	},
	{
		.value = { STRING, { .string = NULL } },
		.match = "(null)",
	},
	{
		.value = { STRING, { .string = "" } },
		.match = "",
	},
	{
		.value = { STRING, { .string = "Hi\nthere" } },
		.match = "Hi\nthere",
	},
	{
		.value = { DATA, { .data = NULL } },
		.match = "[NULL raw data]",
	},
	{
		.value = { DATA, { .data = &(struct nl_raw_data){ .size = 0, .data = NULL } } },
		.match = "[NULL raw data of length 0]",
	},
	{
		.value = { DATA, { .data = &(struct nl_raw_data){ .size = 0, .data = "" } } },
		.match = "[Raw data of length 0]",
	},
	{
		.value = { DATA, { .data = &(struct nl_raw_data){ .size = 6, .data = "Hello" } } },
		.match = "[Raw data of length 6]",
	},
};

static void do_str_test(struct str_test *test)
{
	char *str = nl_variant_to_string(test->value);

	if(str == NULL) {
		ERROR_OUT("Error converting %s ", NL_VARTYPE_NAME(test->value.type));
		nl_display_variant(stderr, test->value);
		ERROR_OUT_EX(" to a string (NULL was returned).\n");
		exit(1);
	}

	int mismatch = test->start ? nl_strstart(str, test->match) : strcmp(str, test->match);
	if(mismatch) {
		ERROR_OUT("Conversion mismatch for %s ", NL_VARTYPE_NAME(test->value.type));
		nl_display_variant(stderr, test->value);
		ERROR_OUT_EX(": got '%s', expected '%s'%s\n", str, test->match, test->start ? " at start" : "");
		exit(1);
	}

	free(str);
}

int main()
{
	enum nl_vartype type = INVALID;

	for(size_t i = 0; i < ARRAY_SIZE(tests); i++) {
		if(tests[i].value.type != type) {
			type = tests[i].value.type;
			printf("Checking %s conversions\n", NL_VARTYPE_NAME(type));
		}
		do_str_test(&tests[i]);
	}

	return 0;
}
