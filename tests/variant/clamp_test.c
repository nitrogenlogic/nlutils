/*
 * Test for nl_clamp_varvalue().
 * Copyright (C)2014 Mike Bourgeous.  Released under AGPLv3 in 2018.
 */
#include <stdio.h>
#include <stdlib.h>
#include <limits.h>
#include <float.h>
#include <math.h>

#include "nlutils.h"

void check_int(int val, int min, int max, int expected)
{
	union nl_varvalue result;

	printf("Clamping integer %d to %d..%d\n", val, min, max);
	result = nl_clamp_varvalue(
			INTEGER,
			(union nl_varvalue)val,
			(union nl_varvalue)min,
			(union nl_varvalue)max
			);

	if(result.integer != expected) {
		ERROR_OUT("Expected %d, got %d\n", expected, result.integer);
		exit(1);
	}
}

void check_float(float val, float min, float max, float expected)
{
	printf("Clamping floating %e to %e..%e\n", val, min, max);

	union nl_varvalue result = nl_clamp_varvalue(
			FLOAT,
			(union nl_varvalue)val,
			(union nl_varvalue)min,
			(union nl_varvalue)max
			);

	if(result.floating != expected) {
		ERROR_OUT("Expected %e, got %e\n", expected, result.floating);
		exit(1);
	}
}


int main()
{
	printf("Clamping integers.\n");
	check_int(0, INT_MIN, INT_MAX, 0);
	check_int(INT_MIN, INT_MIN, INT_MAX, INT_MIN);
	check_int(INT_MAX, INT_MIN, INT_MAX, INT_MAX);

	check_int(INT_MIN, -2, 2, -2);
	check_int(INT_MAX, -2, 2, 2);

	// test reversed ranges
	check_int(0, INT_MAX, INT_MIN, INT_MAX);
	check_int(0, 3, 1, 3);
	check_int(1, 3, 1, 3);
	check_int(2, 3, 1, 3);
	check_int(3, 3, 1, 1);
	check_int(4, 3, 1, 1);

	// Check zero-length range
	check_int(-1, 0, 0, 0);
	check_int(0, 0, 0, 0);
	check_int(1, 0, 0, 0);

	// Check inclusiveness of range
	check_int(0, 0, 1, 0);
	check_int(1, 0, 1, 1);
	check_int(-1, 0, 1, 0);
	check_int(2, 0, 1, 1);
	check_int(INT_MIN, 0, 1, 0);
	check_int(INT_MAX, 0, 1, 1);

	check_int(-5, -10, -1, -5);
	check_int(-15, -10, -1, -10);
	check_int(5, -10, -1, -1);


	printf("\nClamping floats.\n");
	check_float(-0.125, -INFINITY, INFINITY, -0.125);
	check_float(-0.0, -INFINITY, INFINITY, -0.0);
	check_float(0.0, -INFINITY, INFINITY, 0.0);
	check_float(0.125, -INFINITY, INFINITY, 0.125);
	check_float(-FLT_MAX, -INFINITY, INFINITY, -FLT_MAX);
	check_float(FLT_MAX, -INFINITY, INFINITY, FLT_MAX);

	// test reversed ranges
	check_float(0, INT_MAX, INT_MIN, INT_MAX);
	check_float(0, 3, 1, 3);
	check_float(1, 3, 1, 3);
	check_float(2, 3, 1, 3);
	check_float(3, 3, 1, 1);
	check_float(4, 3, 1, 1);
	check_float(0, 1, -1, 1);
	check_float(0, -2, -1, -1);
	check_float(0, INFINITY, -INFINITY, INFINITY);
	check_float(0, FLT_MAX, -FLT_MAX, FLT_MAX);

	check_float(-INFINITY, -FLT_MAX, FLT_MAX, -FLT_MAX);
	check_float(INFINITY, -FLT_MAX, FLT_MAX, FLT_MAX);

	check_float(0, -FLT_MIN, FLT_MIN, 0);

	check_float(0, 0, 0, 0);
	check_float(1, 0, 0, 0);
	check_float(0, 0, 1, 0);
	check_float(0.5, 0, 1, 0.5);
	check_float(1, 0, 1, 1);
	check_float(-1, 0, 1, 0);
	check_float(2, 0, 1, 1);
	check_float(-FLT_MAX, 0, 1, 0);
	check_float(FLT_MAX, 0, 1, 1);
	check_float(-INFINITY, 0, 1, 0);
	check_float(INFINITY, 0, 1, 1);

	check_float(-5, -10, -1, -5);
	check_float(-15, -10, -1, -10);
	check_float(5, -10, -1, -1);

	return 0;
}
