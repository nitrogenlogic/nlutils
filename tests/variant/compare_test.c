/*
 * Test for nl_varvalue comparison function.  Moved from logic_system and
 * expanded.
 * Copyright (C)2014 Mike Bourgeous.  Released under AGPLv3 in 2018.
 */
#include <stdio.h>
#include <stdlib.h>
#include <limits.h>
#include <float.h>
#include <math.h>

#include "nlutils.h"

#define signint(x) (((x) > 0) - ((x) < 0))

void check_result(int expected, int got)
{
	if(signint(expected) != signint(got)) {
		ERROR_OUT("Expected %d (%d), got %d (%d)\n", signint(expected), expected, signint(got), got);
		exit(1);
	}
}

void do_variant_check(struct nl_variant val1, struct nl_variant val2, int expected)
{
	printf("Comparing %s variant to %s variant\n",
			NL_VARTYPE_NAME(val1.type),
			NL_VARTYPE_NAME(val2.type));

	check_result(expected, nl_compare_variants(val1, val2));
}

void do_int_check(int val1, int val2, int expected)
{
	printf("Comparing int %d to %d\n", val1, val2);
	check_result(expected, nl_compare_varvalues(INTEGER, (union nl_varvalue)val1, (union nl_varvalue)val2));
	do_variant_check(
			(struct nl_variant){.type = INTEGER, .value = {.integer = val1}},
			(struct nl_variant){.type = INTEGER, .value = {.integer = val2}},
			expected
			);
}

void do_float_check(float val1, float val2, int expected)
{
	printf("Comparing float %f to %f\n", val1, val2);
	check_result(expected, nl_compare_varvalues(FLOAT, (union nl_varvalue)val1, (union nl_varvalue)val2));
	do_variant_check(
			(struct nl_variant){.type = FLOAT, .value = {.floating = val1}},
			(struct nl_variant){.type = FLOAT, .value = {.floating = val2}},
			expected
			);
}

void do_string_check(char *val1, char *val2, int expected)
{
	printf("Comparing string %s to %s\n", val1 ? val1 : "(null)", val2 ? val2 : "(null)");
	check_result(expected, nl_compare_varvalues(STRING, (union nl_varvalue)val1, (union nl_varvalue)val2));
	do_variant_check(
			(struct nl_variant){.type = STRING, .value = {.string = val1}},
			(struct nl_variant){.type = STRING, .value = {.string = val2}},
			expected
			);
}

void do_data_check(struct nl_raw_data *val1, struct nl_raw_data *val2, int expected)
{
	struct nl_variant v1 = {.type = DATA, .value = {.data = val1}};
	struct nl_variant v2 = {.type = DATA, .value = {.data = val2}};

	fputs("Comparing data ", stdout);
	nl_display_variant(stdout, v1);
	fputs(" to ", stdout);
	nl_display_variant(stdout, v2);
	fputs("\n", stdout);

	check_result(expected, nl_compare_varvalues(DATA, (union nl_varvalue)val1, (union nl_varvalue)val2));
	do_variant_check(v1, v2, expected);
}

int main()
{
	printf("Integer comparison\n");

	do_int_check(-5, -4, -1);
	do_int_check(-5, 5, -1);
	do_int_check(5, 6, -1);
	do_int_check(INT_MIN, INT_MAX, -1);
	do_int_check(INT_MIN, 0, -1);
	do_int_check(0, INT_MAX, -1);

	do_int_check(0, 0, 0);
	do_int_check(INT_MIN, INT_MIN, 0);
	do_int_check(INT_MAX, INT_MAX, 0);

	do_int_check(5, -5, 1);
	do_int_check(5, 1, 1);
	do_int_check(-5, -6, 1);
	do_int_check(INT_MAX, INT_MIN, 1);
	do_int_check(INT_MAX, 0, 1);
	do_int_check(0, INT_MIN, 1);


	printf("\nFloating point comparison\n");

	do_float_check(-INFINITY, -INFINITY, 0);
	do_float_check(-INFINITY, 0, -1);
	do_float_check(-INFINITY, INFINITY, -1);
	do_float_check(0, -INFINITY, 1);
	do_float_check(0, 0, 0);
	do_float_check(0, INFINITY, -1);
	do_float_check(INFINITY, -INFINITY, 1);
	do_float_check(INFINITY, 0, 1);
	do_float_check(INFINITY, INFINITY, 0);

	do_float_check(FLT_MAX, FLT_MIN, 1);
	do_float_check(FLT_MAX, -FLT_MAX, 1);
	do_float_check(FLT_MIN, 0, 1);
	do_float_check(-FLT_MIN, 0, -1);
	do_float_check(0, FLT_MIN, -1);
	do_float_check(0, -FLT_MIN, 1);
	do_float_check(-FLT_MAX, FLT_MAX, -1);

	do_float_check(1 + FLT_EPSILON, 1, 1);
	do_float_check(1, 1 + FLT_EPSILON, -1);
	do_float_check(1 + FLT_EPSILON, 1 + FLT_EPSILON, 0);
	do_float_check(FLT_EPSILON, FLT_EPSILON, 0);

	do_float_check(1, 0, 1);
	do_float_check(0, 1, -1);
	do_float_check(1, -1, 1);
	do_float_check(-1, 1, -1);
	do_float_check(0.001, 0.001, 0);
	do_float_check(-0.001, -0.001, 0);
	do_float_check(-0.001, -0.002, 1);
	do_float_check(-0.002, -0.001, -1);


	printf("\nString comparison\n");

	do_string_check(NULL, NULL, 0);
	do_string_check(NULL, "", -1);
	do_string_check("", NULL, 1);
	do_string_check(NULL, "A", -1);
	do_string_check("A", NULL, 1);

	do_string_check("", "", 0);
	do_string_check("", "A", -1);
	do_string_check("A", "", 1);
	do_string_check("A", "A", 0);

	do_string_check("aaaaa", "aaaab", -1);
	do_string_check("aaaab", "aaaaa", 1);
	do_string_check("aaaaa", "aaaaa", 0);


	printf("\nData comparison\n");

	struct nl_raw_data *nulldata = &(struct nl_raw_data){.data = NULL, .size = 0};
	struct nl_raw_data *zerodata = &(struct nl_raw_data){.data = "", .size = 0};
	struct nl_raw_data *onedata = &(struct nl_raw_data){.data = "", .size = 1};
	struct nl_raw_data *oneother = &(struct nl_raw_data){.data = "x", .size = 1};

	do_data_check(NULL, NULL, 0);
	do_data_check(NULL, nulldata, -1);
	do_data_check(nulldata, NULL, 1);
	do_data_check(nulldata, nulldata, 0);

	do_data_check(NULL, zerodata, -1);
	do_data_check(zerodata, NULL, 1);

	do_data_check(nulldata, zerodata, -1);
	do_data_check(zerodata, nulldata, 1);
	do_data_check(zerodata, zerodata, 0);

	do_data_check(NULL, onedata, -1);
	do_data_check(onedata, NULL, 1);
	do_data_check(nulldata, onedata, -1);
	do_data_check(onedata, nulldata, 1);
	do_data_check(zerodata, onedata, -1);
	do_data_check(onedata, zerodata, 1);
	do_data_check(onedata, onedata, 0);

	// Data comparison ignores data contents
	do_data_check(onedata, oneother, 0);
	do_data_check(oneother, onedata, 0);


	printf("\nType mismatched comparison\n");

	// Equal types and zero values
	do_variant_check((struct nl_variant){.type = INTEGER}, (struct nl_variant){.type = INTEGER}, 0);
	do_variant_check((struct nl_variant){.type = FLOAT}, (struct nl_variant){.type = FLOAT}, 0);
	do_variant_check((struct nl_variant){.type = STRING}, (struct nl_variant){.type = STRING}, 0);
	do_variant_check((struct nl_variant){.type = DATA}, (struct nl_variant){.type = DATA}, 0);

	// Mismatched types
	do_variant_check((struct nl_variant){.type = INTEGER}, (struct nl_variant){.type = FLOAT}, -1);
	do_variant_check((struct nl_variant){.type = FLOAT}, (struct nl_variant){.type = INTEGER}, 1);
	do_variant_check((struct nl_variant){.type = INTEGER}, (struct nl_variant){.type = STRING}, -1);
	do_variant_check((struct nl_variant){.type = DATA}, (struct nl_variant){.type = FLOAT}, 1);
	do_variant_check((struct nl_variant){.type = STRING}, (struct nl_variant){.type = DATA}, -1);

	return 0;
}
